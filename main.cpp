/* 
 * File:   main.cpp
 * Author: biankatpas
 * Author: eddiebrissow
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
    char filename[8];
    char ext[3];
    unsigned char attributes;
    unsigned char reserved[10];
    unsigned short creation_time;
    unsigned short creation_date;
    unsigned short starting_cluster;
    unsigned int file_size;
} __attribute((packed)) Fat16Entry;

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

int printRootDirectory(Fat16Entry entry, Fat16BootSector bs, FILE * in) {
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

void readFile(FILE * in, FILE * out,
        unsigned long fat_start,
        unsigned long data_start,
        unsigned long cluster_size,
        unsigned short cluster,
        unsigned long file_size) {
    unsigned char buffer[4096];
    size_t bytes_read, bytes_to_read,
            file_left = file_size, cluster_left = cluster_size;

    // Go to first data cluster
    fseek(in, data_start + cluster_size * (cluster - 2), SEEK_SET);

    // Read until we run out of file or clusters
    while (file_left > 0 && cluster != 0xFFFF) {
        bytes_to_read = sizeof (buffer);

        // don't read past the file or cluster end
        if (bytes_to_read > file_left)
            bytes_to_read = file_left;
        if (bytes_to_read > cluster_left)
            bytes_to_read = cluster_left;

        // read data from cluster, write to file
        bytes_read = fread(buffer, 1, bytes_to_read, in);
        fwrite(buffer, 1, bytes_read, out);
        printf("Copied %d bytes\n", bytes_read);

        // decrease byte counters for current cluster and whole file
        cluster_left -= bytes_read;
        file_left -= bytes_read;

        // if we have read the whole cluster, read next cluster # from FAT
        if (cluster_left == 0) {
            fseek(in, fat_start + cluster * 2, SEEK_SET);
            fread(&cluster, 2, 1, in);

            printf("End of cluster reached, next cluster %d\n", cluster);

            fseek(in, data_start + cluster_size * (cluster - 2), SEEK_SET);
            cluster_left = cluster_size; // reset cluster byte counter
        }
    }
}

int main(int argc, char** argv) {
    FILE * in = fopen("disco2.IMA", "rb"), * out;
    PartitionTable pt[4];
    Fat16BootSector bs;
    Fat16Entry entry;
    short op;
    unsigned long fat_start, root_start, data_start;
    char filename[9] = "        "; // initially pad with spaces
    char filename_aux[8];

    int i;

    fseek(in, 0x1BE, SEEK_SET); // go to partition table start
    fread(pt, sizeof (PartitionTable), 4, in); // read all four entries
    printPartitionTable(pt);

    fseek(in, 0x000, SEEK_SET);
    fread(&bs, sizeof (Fat16BootSector), 1, in);
    printBootSector(bs, in);

    // Calculate start offsets of FAT, root directory and data
    fat_start = ftell(in) + (bs.reserved_sectors - 1) * bs.sector_size;
    root_start = fat_start + bs.sectors_per_fat * bs.number_of_fats *
            bs.sector_size;
    data_start = root_start + bs.root_dir_entries * sizeof (Fat16Entry);

    printf("FAT start at %08X, root dir at %08X, data at %08X\n\n",
            fat_start, root_start, data_start);

    fseek(in, root_start, SEEK_SET);
    printRootDirectory(entry, bs, in);

    printf("\n---------- Informe o arquivo para extrair os dados ----------\n");
    printf("Nome: ");
    scanf("%s", filename_aux);

    for (i = 0; i < 8 && filename_aux[i] && filename_aux[i] != 0; i++)
        filename[i] = filename_aux[i];

    fseek(in, root_start, SEEK_SET);

    for (i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof (entry), 1, in);
        if (entry.filename[0] != '\0') {
            if (0 == memcmp(entry.filename, filename, 8)) {
                printf("%s\n", entry.filename);
                printf("File found!\n");
                break;
            }
        }
    }

    out = fopen("out.txt", "wb"); // write the file contents to disk
    readFile(in, out, fat_start, data_start, bs.sectors_per_cluster *
            bs.sector_size, entry.starting_cluster, entry.file_size);

    fclose(out);
    fclose(in);

    return 0;
}

