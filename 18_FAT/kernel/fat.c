#include "fat.h"
#include "util.h"
#include "mm.h"

fat32fs_t * get_fat32fs(part_t * p) {
    fat32fs_t * fs = (fat32fs_t *) kcalloc(sizeof(fat32fs_t));

    memcpy(&fs->p, p, sizeof(part_t));
    part_read(p, 0, sizeof(fat_bpb_t), &fs->bpb);
    part_read(p, sizeof(fat_bpb_t), sizeof(fat_ebr32_t), &fs->ebr);

    fs->sectors = fs->bpb.sectors_sm;
    if (fs->sectors == 0)
        fs->sectors = fs->bpb.sectors_lg;

    fs->cluster_size = fs->bpb.sectors_per_cluster * fs->bpb.sectorsize;
    fs->fat_size     = fs->ebr.sectors_per_fat * fs->bpb.sectorsize;
    fs->data_sector  = fs->bpb.res_sectors + fs->ebr.sectors_per_fat * fs->bpb.fat_n; // First data sector

    // Create an artificial root diretory
    fs->root_dir.size       = fat32_get_clusters(fs, fs->ebr.root_cluster) * fs->cluster_size;
    fs->root_dir.attr       = FAT_DIR;
    fs->root_dir.cluster_lo = fs->ebr.root_cluster & 0xFFFF;
    fs->root_dir.cluster_hi = fs->ebr.root_cluster >> 16;

    return fs;
}

// Gets the cluster count from a starting point
size_t fat32_get_clusters(fat32fs_t * fs, uint32_t start) {
    uint32_t * fat = (uint32_t *) kmalloc(fs->fat_size);
    part_read(&fs->p, fs->bpb.res_sectors * fs->bpb.sectorsize, fs->fat_size, fat);

    size_t count = 0;
    while (start < 0x0FFFFFF8) {
        start = fat[start];
        count++;
    }

    kfree(fat);

    return count;
}

void fat32_read(fat32fs_t * fs, fat_dirent83_t * entry, void * buf) {
    uint32_t * fat = (uint32_t *) kmalloc(fs->fat_size);
    part_read(&fs->p, fs->bpb.res_sectors * fs->bpb.sectorsize, fs->fat_size, fat);

    uint32_t curr = (entry->cluster_hi << 16) | (entry->cluster_lo);
    size_t read = 0;

    while (curr < 0x0FFFFFF8) {
        size_t sector_offset = (curr - 2) * fs->bpb.sectors_per_cluster + fs->data_sector;

        size_t to_read = fs->cluster_size;
        if (entry->size - read < to_read)
            to_read = entry->size - read;

        part_read(&fs->p, sector_offset * fs->bpb.sectorsize, to_read, buf);

        buf = (char*) buf + to_read;
        read += to_read;

        curr = fat[curr];

        if (read >= entry->size)
            break;
    }

    kfree(fat);
}

char * fat32_lsdir(fat32fs_t * fs, fat_dirent83_t * entry) {
    void * buf = kmalloc(entry->size);
    char * out = kmalloc(entry->size);
    char * filename = kmalloc(256);

    if (!(entry->attr & FAT_DIR)) {
        kwarn(__FILE__,__func__,"non-directory direntry");
    }

    fat32_read(fs, entry, buf);

    fat_dirent83_t * curr = buf;
    char * curr_out = out;

    while (curr->name[0]) {
        memset(filename, 0, 256);

        // Unused entry
        if (curr->name[0] == FAT_UNUSED) {
            curr++;
            continue;
        }

        // Long filename entry
        if (curr->attr == FAT_LFN) {
            while (curr->attr == FAT_LFN) {
                fat_longname_t * lfn = (fat_longname_t *) curr;

                size_t offset = ((lfn->seq & 0xF) - 1) * 13;

                for (size_t i = 0; i < 5; i++) {
                    filename[offset + i] = lfn->name_0[i];
                }

                for (size_t i = 0; i < 6; i++) {
                    filename[offset + 5 + i] = lfn->name_1[i];
                }

                for (size_t i = 0; i < 2; i++) {
                    filename[offset + 11 + i] = lfn->name_2[i];
                }

                curr++;
            }
        } else { // Normal entry
            char * c = filename;

            // Name
            for (size_t i = 0; i < 8; i++) {
                if (curr->name[i] == ' ') // discard padding
                    break;

                *(c++) = curr->name[i];
            }

            *(c++) = '.';

            // Extension
            for (size_t i = 0; i < 3; i++) {
                if (curr->ext[i] == ' ')
                    break;

                *(c++) = curr->ext[i];
            }
        }

        memcpy(curr_out, filename, strlen(filename) + 1);
        curr_out += strlen(filename) + 1;

        curr++;
    }

    kfree(buf);
    kfree(out);
    kfree(filename);

    return out;
}
