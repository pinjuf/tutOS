#include "devfs.h"
#include "mm.h"
#include "vfs.h"
#include "util.h"
#include "vesa.h"
#include "main.h"
#include "kbd.h"
#include "isr.h"

void * devfs_getfile(void * internal_fs, char * path, int m) {
    (void) internal_fs;

    enum FILEMODE mode = m;

    filehandle_t * out = (filehandle_t *) kmalloc(sizeof(filehandle_t));
    devfs_file_t * intern = (devfs_file_t *) kmalloc(sizeof(devfs_file_t));

    out->internal_file = intern;

    if (!strcmp(path, "vesafb")) {
        intern->type = DEVFS_VESA;
        out->curr = 0;
        out->type = FILE_BLK;
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
        out->curr = 2; // 0 is UNKN device, 1 is the /DEV dir, our devices start at 2
        out->type = FILE_DIR;
        out->size = 0;

        memset(&intern->p, 0, sizeof(part_t));
        intern->p.n = GPT_WHOLEDISK;

        if (mode == FILE_W) {
            kwarn(__FILE__,__func__,"cannot write to devfs root dir");
            return NULL;
        }

    } else if (!strcmp(path, "mem")) {
        intern->type = DEVFS_MEM;
        out->curr = 0;
        out->type = FILE_BLK;
        out->size = 0;

    } else if (!strcmp(path, "qemudbg")) {
        intern->type = DEVFS_QEMUDBG;
        out->curr = 0;
        out->type = FILE_DEV;
        out->size = 0;

        if (mode == FILE_R) {
            kwarn(__FILE__,__func__,"cannot read from qemu dbg io");
            return NULL;
        }

    } else if (!strcmp(path, "pit0")) {
        // Lists pit0 ticks
        intern->type = DEVFS_PIT0;
        out->curr = 0;
        out->type = FILE_DEV;
        out->size = 0;

        if (mode == FILE_W) {
            kwarn(__FILE__,__func__,"cannot write to pit0");
            return NULL;
        }

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

        if (part_n != GPT_WHOLEDISK && !gpt_hasmagic(drive_n)) {
            kwarn(__FILE__,__func__,"no gpt");
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

        if (part_n != GPT_WHOLEDISK)
            out->size = intern->p.size * SECTOR_SIZE;

    } else {
        kwarn(__FILE__,__func__,"file not found");
        return NULL;
    }

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

        case DEVFS_PIT0: {
            memcpy(buf, &pit0_ticks, sizeof(size_t));

            return sizeof(size_t); // yes, we force this, no, we won't tell you why
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

        case DEVFS_QEMUDBG: {
            for (size_t i = 0; i < count; i++) {
                qemu_putc(((char*)buf)[i]);
            }

            return count;
        }

        default:
            kwarn(__FILE__,__func__,"cannot write (no impl?)");
            return 0;
    }
}

void * devfs_readdir(void * f) {
    filehandle_t * fh = f;
    devfs_file_t * intern = fh->internal_file;

    if (intern->type != DEVFS_DIR) {
        kwarn(__FILE__,__func__,"trying to dir-read not /dev");
        return NULL;
    }

    enum DEVFS_DEV current = fh->curr;

    dirent * out = kcalloc(sizeof(dirent));

    // This SHOULD be in enum DEVFS_DEV order
    switch (current) {
        case DEVFS_VESA:
            out->d_type = FILE_DEV;
            out->d_namlen = 6;
            memcpy(out->d_name, (char*)"vesafb", 7); // Satan fears me
            out->d_size = bpob->vbe_mode_info.bpp/8 \
                  * bpob->vbe_mode_info.height \
                  * bpob->vbe_mode_info.width;
            fh->curr++;
            return out;

        case DEVFS_PCSPK:
            out->d_type = FILE_DEV;
            out->d_namlen = 5;
            memcpy(out->d_name, (char*)"pcspk", 6);
            out->d_size = 0;
            fh->curr++;
            return out;

        case DEVFS_HDD:
            out->d_type = FILE_BLK;
            // This one is a little bit autistic...
            ata_checkdrives();

            out->d_namlen = 3;
            out->d_size = 0;
            memcpy(out->d_name, (char*)"hd", 3);

            bool has_gpt = gpt_hasmagic(intern->p.d);
            size_t partitions = 1;
            if (has_gpt)
                partitions = 1 + get_part_count(intern->p.d);

            if (intern->p.n >= (partitions-1) && intern->p.n != GPT_WHOLEDISK) {
                intern->p.n = GPT_WHOLEDISK;
                intern->p.d++;
            }

            while (!(drive_bitmap & (1 << intern->p.d))  && intern->p.d < 8)
                intern->p.d++;

            if (intern->p.d < 8) {
                out->d_name[2] = intern->p.d + 'a';

                char buf[6] = {0};
                if (intern->p.n != GPT_WHOLEDISK)
                    itoa(intern->p.n + 1, buf, 10);

                out->d_namlen += strlen(buf);
                memcpy(out->d_name + 3, buf, strlen(buf));

                intern->p.n++;

                return out;
            }

            // Notice how we fall through into the next one
            fh->curr++;
            /* fall through */
            // Holy shit, a marker comment!
 
        case DEVFS_TTY:
            out->d_type = FILE_DEV;
            out->d_namlen = 3;
            memcpy(out->d_name, (char*)"tty", 4);
            out->d_size = 0;
            fh->curr++;
            return out;

        case DEVFS_QEMUDBG:
            out->d_type = FILE_DEV;
            out->d_namlen = 7;
            memcpy(out->d_name, (char*)"qemudbg", 8);
            out->d_size = 0;
            fh->curr++;
            return out;

        case DEVFS_MEM:
            out->d_type = FILE_DEV;
            out->d_namlen = 3;
            memcpy(out->d_name, (char*)"mem", 4);
            out->d_size = 0;
            fh->curr++;
            return out;

        case DEVFS_PIT0:
            out->d_type = FILE_DEV;
            out->d_namlen = 4;
            memcpy(out->d_name, (char*)"pit0", 5);
            out->d_size = 0;
            fh->curr++;
            return out;

        default:
            return NULL;
    }
}
