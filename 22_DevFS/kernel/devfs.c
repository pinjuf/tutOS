#include "devfs.h"
#include "mm.h"
#include "vfs.h"
#include "util.h"
#include "vesa.h"
#include "main.h"

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
        out->size = bpob->vbe_mode_info.bpp \
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

    } else {
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
                                  * bpob->vbe_mode_info.bpp;

                 const size_t full_rows = fh->curr \
                                        / bpob->vbe_mode_info.width \
                                        / bpob->vbe_mode_info.bpp;

                 size_t actual = fh->curr + full_rows * pad;
                 ((char*)buf)[i] = *((char*) ((size_t)VESA_VIRT_FB + actual));

                 fh->curr++;
             }

             return to_read;
        }
        default:
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
                                  * bpob->vbe_mode_info.bpp;

                 const size_t full_rows = fh->curr \
                                        / bpob->vbe_mode_info.width \
                                        / bpob->vbe_mode_info.bpp;

                 size_t actual = fh->curr + full_rows * pad;
                 *((char*) ((size_t)VESA_VIRT_FB + actual)) = ((char*)buf)[i];

                 fh->curr++;
             }

             return to_write;
        }
        case DEVFS_PCSPK: {

        }
        default:
            return 0;
    }
}
