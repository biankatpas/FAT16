/* 
 * File:   main.cpp
 * Author: biankatpas
 * Created on 28 de Maio de 2017, 21:34
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned char first_byte;
    unsigned char start_chs[3];
    unsigned char partition_type;
    unsigned char end_chs[3];
    unsigned long start_sector;
    unsigned long length_sectors;
} __attribute((packed)) PartitionTable;

typedef struct {
    unsigned char jmp[3];
    char oem[8];
    unsigned short sector_size;
    unsigned char sectors_per_cluster;
    unsigned short reserved_sectors;
    unsigned char number_of_fats;
    unsigned short root_dir_entries;
    unsigned short total_sectors_short; // if zero, later field is used
    unsigned char media_descriptor;
    unsigned short sectors_per_fat;
    unsigned short sectors_per_track;
    unsigned short number_of_heads;
    unsigned int hidden_sectors;
    unsigned int total_sectors_long;

    unsigned char bios_drive;
    unsigned char reserved;
    unsigned char boot_signature;
    unsigned int volume_serial_number_dec;
    char volume_label[11];
    char fs_type[8];
    char executable_code[448];
    unsigned short signature;
} __attribute((packed)) Fat16BootSector;

typedef struct {
    unsigned char filename[8];
    //char* filename = (char* ) malloc(sizeof(8));
    unsigned char ext[3];
    unsigned char attributes;
    unsigned char reserved[10];
    unsigned short creation_time;
    unsigned short creation_date;
    //unsigned short last_access_date;
    //unsigned short last_write_time;
    //unsigned short last_write_date;
    unsigned short starting_cluster;
    unsigned int file_size;
} __attribute((packed)) Fat16Entry;

/*void print_file_info(Fat16Entry *entry) {
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
            1980 + (entry->last_write_time >> 9), (entry->last_write_time >> 5) & 0xF, entry->last_write_time & 0x1F,
            (entry->creation_time >> 11), (entry->creation_time >> 5) & 0x3F, entry->creation_time & 0x1F,
            entry->starting_cluster, entry->file_size);
}*/

int main(int argc, char** argv) {

    FILE * in = fopen("disco2.IMA", "rb");
    int i;
    PartitionTable pt[4];
    Fat16BootSector bs;
    Fat16Entry entry;
    unsigned long fat_start, root_start, data_start;

    fseek(in, 0x1BE, SEEK_SET); // go to partition table start
    fread(pt, sizeof (PartitionTable), 4, in); // read all four entries

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

    //fseek(in, 512 * pt[i].start_sector, SEEK_SET);
    fseek(in, pt[0].start_sector, SEEK_SET);
    fread(&bs, sizeof (Fat16BootSector), 1, in);

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


    // Calculate start offsets of FAT, root directory and data
    fat_start = ftell(in) + (bs.reserved_sectors - 1) * bs.sector_size;
    root_start = fat_start + bs.sectors_per_fat * bs.number_of_fats *
            bs.sector_size;
    data_start = root_start + bs.root_dir_entries * sizeof (Fat16Entry);

    printf("FAT start at %08X, root dir at %08X, data at %08X\n\n",
            fat_start, root_start, data_start);

    fseek(in, root_start, SEEK_SET);

    for (i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof (entry), 1, in);
        //print_file_info(&entry);
        if (entry.filename[0] != '\0')
            printf("Filename: %.8s.%.3s\n", entry.filename, entry.ext);
    }

    if (i == bs.root_dir_entries) {
        printf("File not found!");
        return -1;
    }

    printf("\nRoot directory read, now at 0x%X\n", ftell(in));

    fclose(in);
    return 0;
}

