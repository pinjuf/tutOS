#include "devfs.h"
#include "mm.h"
#include "vfs.h"
#include "util.h"
#include "vesa.h"
#include "main.h"
#include "kbd.h"
#include "isr.h"
#include "sb16.h"
#include "rnd.h"

devfs_t * get_devfs(void * p) {
    // All FS initializers must take a mountfile, but devfs doesn't NEED it
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

    // Fake ".." entry, VFS will handle this
    memcpy(dev.name, "..", 3);
    dev.type      = FILE_DIR;
    dev.size      = 0;
    dev.avl_modes = O_RDONLY;
    dev.read      = NULL;
    dev.write     = NULL;
    dev.readdir   = NULL;
    devfs_register_dev(out, &dev);

    // stdin, stdout etc.
    memcpy(dev.name, "tty", 4);
    dev.type      = FILE_DEV;
    dev.size      = 0;
    dev.avl_modes = O_RDWR;
    dev.read      = devfs_read_tty;
    dev.write     = devfs_write_tty;
    dev.readdir   = NULL;
    devfs_register_dev(out, &dev);

    // VESA/VBE framebuffer
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

    // entire RAM
    memcpy(dev.name, "mem", 4);
    dev.type      = FILE_BLK;
    dev.size      = 0;
    dev.avl_modes = O_RDWR;
    dev.read      = devfs_read_mem;
    dev.write     = devfs_write_mem;
    dev.readdir   = NULL;
    devfs_register_dev(out, &dev);

    // HDDs
    ata_checkdrives();
    for (size_t i = 0; i < ATA_DRIVES; i++) {
        if (!((1<<i) & drive_bitmap)) {
            continue;
        }

        memcpy(dev.name, "hd", 2);
        dev.name[2]   = 'a' + i;
        dev.type      = FILE_BLK;
        dev.size      = 0;
        dev.avl_modes = O_RDWR;
        dev.read      = devfs_read_hdd;
        dev.write     = devfs_write_hdd;
        dev.readdir   = NULL;

        part_t * part = get_part(i, GPT_WHOLEDISK);
        memcpy(&dev.spec.p, part, sizeof(part_t));
        kfree(part);

        devfs_register_dev(out, &dev);

        if (!gpt_hasmagic(i)) {
            continue;
        }

        for (size_t j = 0; j < get_part_count(i); j++) {
            memcpy(dev.name, "hd", 2); // Some of this is redundant because of above, IK/IDC
            dev.name[2]   = 'a' + i;
            itoa(j+1, &dev.name[3], 10);
            dev.type      = FILE_BLK;
            dev.size      = 0;
            dev.avl_modes = O_RDWR;
            dev.read      = devfs_read_hdd;
            dev.write     = devfs_write_hdd;
            dev.readdir   = NULL;

            part_t * part = get_part(i, j);
            memcpy(&dev.spec.p, part, sizeof(part_t));
            kfree(part);

            devfs_register_dev(out, &dev);
        }
    }

    // PIT0 ticks
    memcpy(dev.name, "pit0", 5);
    dev.type      = FILE_DEV;
    dev.size      = 0;
    dev.avl_modes = O_RDONLY;
    dev.read      = devfs_read_pit0;
    dev.write     = NULL;
    dev.readdir   = NULL;
    devfs_register_dev(out, &dev);

    // SoundBlaster16/DSP
    memcpy(dev.name, "dsp", 4);
    dev.type      = FILE_DEV;
    dev.size      = 0;
    dev.avl_modes = O_RDWR;
    dev.read      = devfs_read_dsp;
    dev.write     = devfs_write_dsp;
    dev.readdir   = NULL;
    devfs_register_dev(out, &dev);

    // PC PIT2 Speaker
    memcpy(dev.name, "pcspk", 6);
    dev.type      = FILE_DEV;
    dev.size      = 0;
    dev.avl_modes = O_WRONLY;
    dev.read      = NULL;
    dev.write     = devfs_write_pcspk;
    dev.readdir   = NULL;
    devfs_register_dev(out, &dev);

    // QEMU debug port
    memcpy(dev.name, "qemudbg", 8);
    dev.type      = FILE_DEV;
    dev.size      = 0;
    dev.avl_modes = O_WRONLY;
    dev.read      = NULL;
    dev.write     = devfs_write_qemudbg;
    dev.readdir   = NULL;
    devfs_register_dev(out, &dev);

    // Random number generator
    memcpy(dev.name, "random", 7);
    dev.type      = FILE_DEV;
    dev.size      = 0;
    dev.avl_modes = O_RDONLY;
    dev.read      = devfs_read_rnd;
    dev.write     = NULL;
    dev.readdir   = NULL;
    devfs_register_dev(out, &dev);

    return (void*)out;
}

int del_devfs(void * m) {
    mountpoint_t * mnt = m;
    devfs_t * fs = mnt->internal_fs;

    destroy_ll(fs->devs);
    kfree(fs);

    return 0;
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

int devfs_unlink_dev(void * mn, char * path) {
    // Unregisters a device by path

    mountpoint_t * mnt = mn;
    devfs_t * fs       = mnt->internal_fs;

    devfs_dev_t * dev = NULL;

    for (size_t i = 0; i < ll_len(fs->devs); i++) {
        if (!strcmp(((devfs_dev_t*)ll_get(fs->devs, i))->name, path)) {
            dev = ll_get(fs->devs, i);
            break;
        }
    }

    if (dev == NULL) {
        kwarn(__FILE__,__func__,"file not found");
        return -1;
    }

    if (dev->type == FILE_DIR) {
        kwarn(__FILE__,__func__,"cannot unlink directory");
        return -1;
    }

    devfs_unregister_dev(fs, dev->id);

    return 0;
}

void * devfs_getfile(void * mn, char * path, uint16_t m) {
    mountpoint_t * mnt    = mn;
    devfs_t * fs          = mnt->internal_fs;
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
        return NULL;
    }

    // There has got to be a cleaner way
    if (!((mode & O_RDWR) == (mode & dev->avl_modes & O_RDWR))) {
        kwarn(__FILE__,__func__,"requested RW mode(s) not available");
        return NULL;
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
        return 0;
    }

    return dev->read(f, buf, count);
}

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
        kwarn(__FILE__,__func__,"device not registered");
        return 0;
    }

    return dev->write(f, buf, count);
}

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
        return 0;
    }

    return dev->readdir(f);
}

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

    // Little optimisation if we can ignore pitch
    if (bpob->vbe_mode_info.pitch == bpob->vbe_mode_info.width * bpob->vbe_mode_info.bpp/8) {
        memcpy(buf, (void*)(VESA_VIRT_FB + fh->curr), to_read);
        fh->curr += to_read;
        return to_read;
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

    // Little optimisation if we can ignore pitch
    if (bpob->vbe_mode_info.pitch == bpob->vbe_mode_info.width * bpob->vbe_mode_info.bpp/8) {
        memcpy((void*)(VESA_VIRT_FB + fh->curr), buf, to_write);
        fh->curr += to_write;
        return to_write;
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

size_t devfs_read_hdd(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;
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
    // If dev still is NULL, we messed up big time
    // (this means a HDD was unregistered but a filehandle still wants use it)

    int res = part_read(&dev->spec.p, fh->curr, count, buf);

    fh->curr += count;

    return res ? 0 : count;
}

size_t devfs_write_hdd(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;
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

    int res = part_write(&dev->spec.p, fh->curr, count, buf);

    fh->curr += count;

    return res ? 0 : count;
}

size_t devfs_read_pit0(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;

    uint64_t ticks = pit0_ticks;
    memcpy(buf, &ticks, sizeof(pit0_ticks));

    return sizeof(pit0_ticks); // yes, we force this, no, we won't tell you why
}

size_t devfs_read_dsp(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;
    memcpy(buf, sb16_player, sizeof(sb16_player_t));

    return sizeof(sb16_player_t);
}

size_t devfs_write_dsp(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;
    memcpy(sb16_player, buf, sizeof(sb16_player_t));

    if (sb16_player->playing)
        sb16_start_play();
    else
        sb16_stop_play();

    return sizeof(sb16_player_t);
}

size_t devfs_write_pcspk(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;
    init_pit2(*(uint32_t*)buf);

    return 4;
}

size_t devfs_write_qemudbg(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;
    for (size_t i = 0; i < count; i++) {
        qemu_putc(((char*)buf)[i]);
    }

    return count;
}

size_t devfs_read_rnd(void * f, void * buf, size_t count) {
    (void) f, (void) buf, (void) count;

    size_t dwords = count / sizeof(uint32_t);
    size_t rem    = count % sizeof(uint32_t);

    for (size_t i = 0; i < dwords; i++) {
        ((uint64_t*)buf)[i] = rand();
    }

    for (size_t i = 0; i < rem; i++) {
        ((char*)buf)[dwords*sizeof(uint32_t) + i] = rand();
    }

    return count;
}

int devfs_exists(void * m, char * path) {
    mountpoint_t * mnt = m;
    devfs_t * fs = mnt->internal_fs;

    for (size_t i = 0; i < ll_len(fs->devs); i++) {
        if (!strcmp(((devfs_dev_t*)ll_get(fs->devs, i))->name, path)) {
            return ((devfs_dev_t*)ll_get(fs->devs, i))->type;
        }
    }

    return -1;
}
