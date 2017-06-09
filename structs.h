//
// Created by edgar on 07/06/17.
//

#ifndef FAT16_STRUCTS_H
#define FAT16_STRUCTS_H


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
    char filename[8];
    char ext[3];
    unsigned char attributes;
    unsigned char reserved[10];
    unsigned short creation_time;
    unsigned short creation_date;
    unsigned short starting_cluster;
    unsigned int file_size;
} __attribute((packed)) Fat16Entry;

typedef struct {
    unsigned char first_byte;
    unsigned char start_chs[3];
    unsigned char partition_type;
    unsigned char end_chs[3];
    unsigned long start_sector;
    unsigned long length_sectors;
} __attribute((packed)) PartitionTable;

#endif //FAT16_STRUCTS_H
