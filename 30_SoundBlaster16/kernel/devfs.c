#include "devfs.h"
#include "mm.h"
#include "vfs.h"
#include "util.h"
#include "vesa.h"
#include "main.h"
#include "kbd.h"
#include "isr.h"
#include "sb16.h"

devfs_t * get_devfs(void * p) {
    // All FS initializers must take a partition, but devfs doesn't NEED it
    (void)p;

    devfs_t * out = (devfs_t*) kmalloc(sizeof(devfs_t));
    out->devs = create_ll(sizeof(devfs_dev_t));
    out->id_counter = 0; // Each devices gets a unique ID upon registering

    devfs_dev_t dev;

    // The "/dev/"-directory itself
    memcpy(dev.name, "", 1);
    dev.type      = FILE_DIR;
    dev.size      = 0;
    dev.avl_modes = O_RDONLY;
    dev.read      = NULL;
    dev.write     = NULL;
    dev.readdir   = devfs_readdir_rootdir;
    devfs_register_dev(out, &dev);

    memcpy(dev.name, "tty", 4);
    dev.type      = FILE_DEV;
    dev.size      = 0;
    dev.avl_modes = O_RDWR;
    dev.read      = devfs_read_tty;
    dev.write     = devfs_write_tty;
    dev.readdir   = NULL;
    devfs_register_dev(out, &dev);

    memcpy(dev.name, "vesa", 5);
    dev.type      = FILE_BLK;
    dev.size      = bpob->vbe_mode_info.bpp / 8 \
                  * bpob->vbe_mode_info.width   \
                  * bpob->vbe_mode_info.height;
    dev.avl_modes = O_RDWR;
    dev.read      = devfs_read_vesa;
    dev.write     = devfs_write_vesa;
    dev.readdir   = NULL;
    devfs_register_dev(out, &dev);

    memcpy(dev.name, "mem", 4);
    dev.type      = FILE_BLK;
    dev.size      = 0;
    dev.avl_modes = O_RDWR;
    dev.read      = devfs_read_mem;
    dev.write     = devfs_write_mem;
    dev.readdir   = NULL;
    devfs_register_dev(out, &dev);

    return (void*)out;
}

devfs_id_t devfs_register_dev(devfs_t * fs, devfs_dev_t * dev) {
    if (fs->id_counter == UINT32_MAX) {
        kwarn(__FILE__,__func__,"what the fuck"); // Oops, 4294967295 devices...
    }

    devfs_id_t new_id = fs->id_counter++;

    devfs_dev_t * new_dev = (devfs_dev_t*) ll_push(fs->devs);

    memcpy(new_dev, dev, sizeof(devfs_dev_t));

    new_dev->id = new_id;

    return new_id;
}

void devfs_unregister_dev(devfs_t * fs, devfs_id_t dev) {
    // Delete the linked list entry with that id
    // (doesn't do anything if the device is not present)

    for (size_t i = 0; i < ll_len(fs->devs); i++) {
        if (((devfs_dev_t*)ll_get(fs->devs, i))->id == dev) {
            ll_del(fs->devs, i);
            return;
        }
    }
}

void * devfs_getfile(void * internal_fs, char * path, uint16_t m) {
    devfs_t * fs          = internal_fs;
    mode_t mode           = m;
    filehandle_t * out    = (filehandle_t *) kmalloc(sizeof(filehandle_t));
    devfs_file_t * intern = (devfs_file_t *) kmalloc(sizeof(devfs_file_t));

    out->internal_file = intern;

    devfs_dev_t * dev = NULL;

    for (size_t i = 0; i < ll_len(fs->devs); i++) {
        if (!strcmp(((devfs_dev_t*)ll_get(fs->devs, i))->name, path)) {
            dev = ll_get(fs->devs, i);
            break;
        }
    }

    if (dev == NULL) {
        kwarn(__FILE__,__func__,"file not found");
    }

    // There has got to be a cleaner way
    if (!((mode & O_RDWR) == (mode & dev->avl_modes & O_RDWR))) {
        kwarn(__FILE__,__func__,"requested RW mode(s) not available");
    }

    intern->id = dev->id;
    out->curr  = 0;
    out->type  = dev->type;
    out->size  = dev->size;

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

    devfs_t * fs = (devfs_t *) mountpoints[fh->mountpoint].internal_fs;

    devfs_dev_t * dev = NULL;
    for (size_t i = 0; i < ll_len(fs->devs); i++) {
        if (((devfs_dev_t*)ll_get(fs->devs, i))->id == intern->id) {
            dev = (devfs_dev_t*) ll_get(fs->devs, i);
            break;
        }
    }

    if (dev == NULL) {
        kwarn(__FILE__,__func__,"device not registered");
    }

    return dev->read(f, buf, count);
}
/*
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

        case DEVFS_DSP: {
            memcpy(buf, sb16_player, sizeof(sb16_player_t));

            return sizeof(sb16_player_t);
        }

        default:
            kwarn(__FILE__,__func__,"cannot read (no impl?)");
            return 0;
    }
*/

size_t devfs_writefile(void * f, void * buf, size_t count) {
    filehandle_t * fh = f;
    devfs_file_t * intern = fh->internal_file;

    devfs_t * fs = (devfs_t *) mountpoints[fh->mountpoint].internal_fs;

    devfs_dev_t * dev = NULL;
    for (size_t i = 0; i < ll_len(fs->devs); i++) {
        if (((devfs_dev_t*)ll_get(fs->devs, i))->id == intern->id) {
            dev = (devfs_dev_t*) ll_get(fs->devs, i);
            break;
        }
    }

    if (dev == NULL) {
        kwarn(__FILE__,__func__,"device not registerd");
    }

    return dev->write(f, buf, count);
}

/*
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

        case DEVFS_DSP: {
            memcpy(sb16_player, buf, sizeof(sb16_player_t));

            if (sb16_player->playing)
                sb16_start_play();

            return sizeof(sb16_player_t);
        }

        default:
            kwarn(__FILE__,__func__,"cannot write (no impl?)");
            return 0;
    }
    */

void * devfs_readdir(void * f) {
    filehandle_t * fh = f;
    devfs_file_t * intern = fh->internal_file;

    devfs_t * fs = (devfs_t *) mountpoints[fh->mountpoint].internal_fs;

    devfs_dev_t * dev = NULL;
    for (size_t i = 0; i < ll_len(fs->devs); i++) {
        if (((devfs_dev_t*)ll_get(fs->devs, i))->id == intern->id) {
            dev = (devfs_dev_t*) ll_get(fs->devs, i);
            break;
        }
    }

    if (dev == NULL) {
        kwarn(__FILE__,__func__,"device not registered");
    }

    return dev->readdir(f);
}
/*
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
            fall through 
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

        case DEVFS_DSP:
            out->d_type = FILE_DEV;
            out->d_namlen = 3;
            memcpy(out->d_name, (char*)"dsp", 4);
            out->d_size = 0;
            fh->curr++;
            return out;

        default:
            return NULL;
    }
*/

void * devfs_readdir_rootdir(void * f) {
    filehandle_t * fh     = f;
    devfs_t * fs = (devfs_t *) mountpoints[fh->mountpoint].internal_fs;

    if (fh->curr >= ll_len(fs->devs))
        return NULL;

    devfs_dev_t * dev = ll_get(fs->devs, fh->curr++);

    dirent * out  = (dirent *) kmalloc(sizeof(dirent));
    out->d_type   = dev->type;
    out->d_size   = dev->size;
    out->d_namlen = strlen(dev->name);
    memcpy(out->d_name, dev->name, out->d_namlen + 1);

    // Hacky hecky hicky hocky hucky rooty directory
    if (out->d_namlen == 0) {
        out->d_namlen = 1;
        memcpy(out->d_name, ".", 2);
    }

    return out;
}

size_t devfs_read_tty(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;

    for (size_t i = 0; i < count; i++) {
        ((char*)buf)[i] = kbd_get_last_ascii();
    }

    return count;
}

size_t devfs_write_tty(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;

    for (size_t i = 0; i < count; i++) {
        kputc(((char*)buf)[i]);
    }

    return count;
}

size_t devfs_read_vesa(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;
    filehandle_t * fh = f;

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

size_t devfs_write_vesa(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;
    filehandle_t * fh = f;

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

size_t devfs_read_mem(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;
    filehandle_t * fh = f;

    for (size_t i = 0; i < count; i++) {
        ((char*)buf)[i] = *((char*)(fh->curr++));
    }

    return count;
}

size_t devfs_write_mem(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;
    filehandle_t * fh = f;

    for (size_t i = 0; i < count; i++) {
        *((char*)(fh->curr++)) = ((char*)buf)[i];
    }

    return count;
}
