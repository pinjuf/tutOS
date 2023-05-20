#include "devfs.h"
#include "mm.h"
#include "vfs.h"
#include "util.h"
#include "vesa.h"
#include "main.h"
#include "kbd.h"

void * devfs_getfile(void * internal_fs, char * path, int m) {
    (void) internal_fs;

    enum FILEMODE mode = m;

    filehandle_t * out = (filehandle_t *) kmalloc(sizeof(filehandle_t));
    devfs_file_t * intern = (devfs_file_t *) kmalloc(sizeof(devfs_file_t));

    out->internal_file = intern;

    if (!strcmp(path, "vesafb")) {
        intern->type = DEVFS_VESA;
        out->curr = 0;
        out->type = FILE_DEV;
        out->size = bpob->vbe_mode_info.bpp/8 \
                  * bpob->vbe_mode_info.height \
                  * bpob->vbe_mode_info.width;

    } else if (!strcmp(path, "pcspk")) {
        intern->type = DEVFS_PCSPK;
        out->curr = 0;
        out->type = FILE_DEV;
        out->size = 0;

        if (mode == FILE_R) {
            kwarn(__FILE__,__func__,"cannot read sound");
            return NULL;
        }

    } else if (!strcmp(path, "tty")) {
        intern->type = DEVFS_TTY;
        out->curr = 0;
        out->type = FILE_DEV;
        out->size = 0;

    } else if (!strcmp(path, "")) {
        intern->type = DEVFS_DIR;
        out->curr = 0;
        out->type = FILE_DIR;
        out->size = 0;

        if (mode == FILE_W) {
            kwarn(__FILE__,__func__,"cannot write to devfs root dir");
        }

    } else if (!strcmp(path, "mem")) {
        intern->type = DEVFS_MEM;
        out->curr = 0;
        out->type = FILE_BLK;
        out->size = 0;

    } else if (strlen(path) > 2 \
            && path[0] == 'h' \
            && path[1] == 'd') {

        intern->type = DEVFS_HDD;
        ata_checkdrives();

        drive_t drive_n = path[2] - 'a';
        size_t part_n = strlen(path+3) ? atoi(path+3, 10)-1 : GPT_WHOLEDISK; // Partition numbering starts at #1

        if ((drive_bitmap & (1<<drive_n)) == 0) {
            kwarn(__FILE__,__func__,"drive not present");
            return NULL;
        }

        if (part_n != GPT_WHOLEDISK && part_n >= get_part_count(drive_n)) {
            // We assume that there are no holes (unused entries) in the GPT
            kwarn(__FILE__,__func__,"partition non-existant");
            return NULL;
        }

        part_t * p = get_part(drive_n, part_n);
        memcpy(&intern->p, p, sizeof(part_t));
        kfree(p);

        out->curr = 0;
        out->type = FILE_BLK;
        out->size = 0;

    } else {
        kwarn(__FILE__,__func__,"file not found");
        return NULL;
    }


    // TODO: HDD devices

    return out;
}

void devfs_closefile(void * f) {
    filehandle_t * fh = f;

    kfree(fh->internal_file);
    kfree(fh);
}

size_t devfs_readfile(void * f, void * buf, size_t count) {
    filehandle_t * fh = f;
    devfs_file_t * intern = fh->internal_file;

    switch (intern->type) {
        case DEVFS_VESA: {
             if (fh->curr > fh->size) {
                 return 0;
             }

             size_t to_read = count;
             if (fh->curr + to_read > fh->size) {
                 to_read = fh->size - fh->curr;
             }

             for (size_t i = 0; i < to_read; i++) {
                 // We need to take pitch into account!
                 // <=====WIDTH * BPP=========>
                 // | ACTUAL ROW              | PAD |
                 // <=============PITCH=============>

                 const size_t pad = bpob->vbe_mode_info.pitch \
                                  - bpob->vbe_mode_info.width \
                                  * bpob->vbe_mode_info.bpp/8;

                 const size_t full_rows = fh->curr \
                                        / bpob->vbe_mode_info.width \
                                        / bpob->vbe_mode_info.bpp/8;

                 size_t actual = fh->curr + full_rows * pad;
                 ((char*)buf)[i] = *((char*) ((size_t)VESA_VIRT_FB + actual));

                 fh->curr++;
             }

             return to_read;
        }

        case DEVFS_TTY: {
            for (size_t i = 0; i < count; i++) {
                ((char*)buf)[i] = kbd_get_last_ascii();
            }

            return count;
        }

        case DEVFS_MEM: {
            for (size_t i = 0; i < count; i++) {
                ((char*)buf)[i] = *((char*)(fh->curr++));
            }

            return count;
        }

        case DEVFS_HDD: {
            int res = part_read(&intern->p, fh->curr, count, buf);

            fh->curr += count;

            return res ? 0 : count;
        }

        default:
            kwarn(__FILE__,__func__,"cannot read (no impl?)");
            return 0;
    }
}

size_t devfs_writefile(void * f, void * buf, size_t count) {
    filehandle_t * fh = f;
    devfs_file_t * intern = fh->internal_file;

    switch (intern->type) {
        case DEVFS_VESA: {
            if (fh->curr > fh->size) {
                return 0;
            }

            size_t to_write = count;
            if (fh->curr + to_write > fh->size) {
                to_write = fh->size - fh->curr;
            }

            for (size_t i = 0; i < to_write; i++) {
                // We need to take pitch into account!
                // <=====WIDTH * BPP=========>
                // | ACTUAL ROW              | PAD |
                // <=============PITCH=============>

                const size_t pad = bpob->vbe_mode_info.pitch \
                                 - bpob->vbe_mode_info.width \
                                 * bpob->vbe_mode_info.bpp/8;

                const size_t full_rows = fh->curr \
                                       / bpob->vbe_mode_info.width \
                                       / bpob->vbe_mode_info.bpp/8;

                size_t actual = fh->curr + full_rows * pad;
                *((char*) ((size_t)VESA_VIRT_FB + actual)) = ((char*)buf)[i];

                fh->curr++;
            }

            return to_write;
        }
        case DEVFS_PCSPK: {
            init_pit2(*(uint32_t*)buf);

            return 4;
        }
        case DEVFS_TTY: {
            for (size_t i = 0; i < count; i++) {
                kputc(((char*)buf)[i]);
            }

            return count;
        }
        case DEVFS_MEM: {
            for (size_t i = 0; i < count; i++) {
                *((char*)(fh->curr++)) = ((char*)buf)[i];
            }

            return count;
        }
        case DEVFS_HDD: {
            int res = part_write(&intern->p, fh->curr, count, buf);

            fh->curr += count;

            return res ? 0 : count;
        }
        default:
            kwarn(__FILE__,__func__,"cannot write (no impl?)");
            return 0;
    }
}
