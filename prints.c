//
// Created by edgar on 09/06/17.
//

#include "structs.h"
#include "prints.h"

int printPartitionTable(PartitionTable pt[]) {
    int i = 0;
    printf("---------- Partition Table ----------\n\n");
    for (i = 0; i < 4; i++) {
        printf("Partition %d, type %02X\n", i, pt[i].partition_type);
        printf("  Start sector %08X, %d sectors long\n",
               pt[i].start_sector, pt[i].length_sectors);
    }

    for (i = 0; i < 4; i++) {
        if (pt[i].partition_type == 4 || pt[i].partition_type == 6 ||
            pt[i].partition_type == 14) {
            printf("\nFAT16 filesystem found from partition %d\n", i);
            break;
        }
    }

    if (i == 4) {
        printf("No FAT16 filesystem found, exiting...\n");
        return -1;
    }
}


void printBootSector(Fat16BootSector bs, FILE * in) {
    printf("\n\n---------- Fat16 Boot Sector ----------\n\n");
    printf("  Jump code: %02X:%02X:%02X\n", bs.jmp[0], bs.jmp[1], bs.jmp[2]);
    printf("  OEM code: [%.8s]\n", bs.oem);
    printf("  sector_size: %d\n", bs.sector_size);
    printf("  sectors_per_cluster: %d\n", bs.sectors_per_cluster);
    printf("  reserved_sectors: %d\n", bs.reserved_sectors);
    printf("  number_of_fats: %d\n", bs.number_of_fats);
    printf("  root_dir_entries: %d\n", bs.root_dir_entries);
    printf("  total_sectors_short: %d\n", bs.total_sectors_short);
    printf("  media_descriptor: %02X\n", bs.media_descriptor);
    printf("  sectors_per_fat: %d\n", bs.sectors_per_fat);
    printf("  sectors_per_track: %d\n", bs.sectors_per_track);
    printf("  number_of_heads: %d\n", bs.number_of_heads);
    printf("  hidden_sectors: %d\n", bs.hidden_sectors);
    printf("  total_sectors_long: %d\n", bs.total_sectors_long);
    printf("  drive_number: %02X\n", bs.bios_drive);
    printf("  unused: %d\n", bs.reserved);
    printf("  boot_signature: %02X\n", bs.boot_signature);
    printf("  volume_serial_number_dec: %d\n", bs.volume_serial_number_dec);
    printf("  volume_serial_number_hex: %08X\n", bs.volume_serial_number_dec);
    printf("  Volume label: [%.11s]\n", bs.volume_label);
    printf("  Filesystem type: [%.8s]\n", bs.fs_type);
    printf("  Boot sector signature: %04X\n", bs.signature);


    printf("\nNow at 0x%X, sector size %d, FAT size %d sectors, %d FATs\n",
           ftell(in), bs.sector_size, bs.sectors_per_fat, bs.number_of_fats);

}


int printRootDirectory(Fat16BootSector bs, FILE * in) {
    Fat16Entry entry;
    int i = 0, j = 0;
    for (i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof (entry), 1, in);

        if (entry.filename[0] != '\0') {
            printFileInfo(&entry);
            j++;
        }
    }

    if (i == j) {
        printf("File not found!");
        return -1;
    }

    printf("\nRoot directory read, now at 0x%X\n", ftell(in));
}


void printFileInfo(Fat16Entry *entry) {
    switch (entry->filename[0]) {
        case 0x00:
            return; // unused entry
        case 0xE5:
            printf("Deleted file: [?%.7s.%.3s]\n", entry->filename + 1, entry->ext);
            return;
        case 0x05:
            printf("File starting with 0xE5: [%c%.7s.%.3s]\n", 0xE5, entry->filename + 1, entry->ext);
            break;
        case 0x2E:
            printf("Directory: [%.8s.%.3s]\n", entry->filename, entry->ext);
            break;
        default:
            printf("File: [%.8s.%.3s]\n", entry->filename, entry->ext);
    }

    printf("  Modified: %04d-%02d-%02d %02d:%02d.%02d    Start: [%04X]    Size: %d\n",
           1980 + (entry->creation_time >> 9), (entry->creation_time >> 5) & 0xF, entry->creation_time & 0x1F,
           (entry->creation_time >> 11), (entry->creation_time >> 5) & 0x3F, entry->creation_time & 0x1F,
           entry->starting_cluster, entry->file_size);
}

