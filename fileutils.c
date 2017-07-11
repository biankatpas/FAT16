//
// Created by edgar on 09/06/17.
//

#include <stdio.h>
#include "fileutils.h"
#include "string.h"
#include <math.h>
#include <stdlib.h>
#include "time.h"
#include "structs.h"


void parseInput(char *in, int size, char *file_name, char *extension) {
    int i, j;
    int count = 0;
    for (i = size - 1; i > 0; i--) {
        if (in[i] == '/') {
            for (j = 1; j < size - i; j++) {
                if (in[i + j] == '.' || in[i + j] == '\0') {
                    if (in[i + j] == '.') {
                        extension[0] = in[i + j + 1];
                        extension[1] = in[i + j + 2];
                        extension[2] = in[i + j + 3];
                    }
                    break;
                } else {
                    if (count < 8) {
                        file_name[count] = in[i + j];
                    }
                    count++;
                }
            }
            break;
        }

    }

    while (count < 8) {
        file_name[count] = ' ';
        count++;
    }
}


void readFile(FILE *in, FILE *out,
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
        bytes_to_read = sizeof(buffer);

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

void writeFile(FILE *dest,
               FILE *src,
               int root_start,
               int data_start,
               Fat16BootSector bs,
               char file_name[8],
               char extension[3], int fat_start) {


    short cluster = 0xFFFF;
    int j;
    int second_fat = bs.sectors_per_fat * bs.sector_size + fat_start;

    short start_cluster = 0;
    for (j = fat_start; j < second_fat; j += 2) {
        fseek(dest, j, SEEK_SET);
        fread(&cluster, 2, 1, dest);
        if (cluster == 0x0000) {
            break;
        }
        start_cluster++;
    }

    fseek(src, 0L, SEEK_END);
    int sz = ftell(src);
    fseek(src, 0x0, SEEK_SET);

    int file_clusters = ceill(sz / 512.f);
    int i;
    j = fat_start;
    short cluster_map[file_clusters + 1];
    int map_count_cluster = -1;

    for (i = 0; i < file_clusters; i++) {
        short tmpc;
        for (j; j < second_fat; j += 2) {
            ++map_count_cluster;
            fseek(dest, fat_start + map_count_cluster * 2, SEEK_SET);
            fread(&tmpc, 2, 1, dest);
            if (tmpc == 0x0000) {
                break;
            }
        }
        cluster_map[i] = map_count_cluster;
        j += 2;
    }

    cluster_map[file_clusters] = 0xFFFF;


    for (i = 0; i < file_clusters; i++) {
        fseek(dest, fat_start + cluster_map[i] * 2, SEEK_SET);
        fwrite(&cluster_map[i + 1], 2, 1, dest);
        fseek(dest, second_fat + cluster_map[i] * 2, SEEK_SET);
        fwrite(&cluster_map[i + 1], 2, 1, dest);
    }

    Fat16Entry file = setEntry(file_name, extension, start_cluster, sz);
    int tst = root_start + sizeof(Fat16Entry) * countEntries(bs, dest, root_start);

    fseek(dest, tst, SEEK_SET);
    fwrite(&file, sizeof(Fat16Entry), 1, dest);

    char **buffers = 0;
    int cluster_size = bs.sectors_per_cluster * bs.sector_size;
    buffers = malloc(file_clusters * sizeof(char) * cluster_size);

    for (i = 0; i < file_clusters; i++) {
        buffers[i] = malloc(cluster_size * sizeof(char));
    }


    int temp_sz = sz;
    for (i = 0; i < file_clusters; i++) {
        fseek(src, cluster_size * i, SEEK_SET);
        if (temp_sz - cluster_size >= 0) {
            fread(buffers[i], cluster_size, 1, src);
            temp_sz -= cluster_size;
        } else {
            fread(buffers[i], temp_sz, 1, src);
            for (j = temp_sz; j < cluster_size; j++)
                buffers[i][j] = 0;
        }
    }


    int current_pos = data_start + ((start_cluster - 2) * cluster_size);
    fseek(dest, current_pos, SEEK_SET);
    fwrite(buffers[0], cluster_size, 1, dest);

    for (i = 1; i < file_clusters; i++) {
        current_pos = data_start + ((cluster_map[i] - 2) * cluster_size);
        fseek(dest, current_pos, SEEK_SET);
        fwrite(buffers[i], cluster_size, 1, dest);

    }

    for (i = 0; i < file_clusters; i++) {
        free(buffers[i]);
    }
    free(buffers);

}

void deleteFile(FILE *in,
                char name[],
                Fat16BootSector bs,
                unsigned long fat_start,
                unsigned long root_start) {
    char filename[8] = "        ";
    int i;
    if (strlen(name) > 8) {
        printf("FILENAME INVALID");
        return;
    }

    for (i = 0; i < 8 && name[i] && name[i] != 0; i++)
        filename[i] = name[i];

    fseek(in, root_start, SEEK_SET);
    Fat16Entry entry;
    for (i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof(entry), 1, in);;
        if (entry.filename[0] != '\0') {
            if (0 == memcmp(entry.filename, filename, 8)) {
                break;
            }
        }
    }

    unsigned short cluster = entry.starting_cluster;
    fseek(in, root_start + (sizeof(Fat16Entry)) * i, SEEK_SET);

    short v[sizeof(Fat16Entry)] = {0x0000};
    fwrite(v, sizeof(Fat16Entry), 1, in);

    short zero = 0x0000;


    unsigned short temp_c;
    while (cluster != 0xFFFF && cluster != 0) {
        temp_c = cluster;
        fseek(in, fat_start + (cluster * 2), SEEK_SET);
        fread(&cluster, 2, 1, in);
        fseek(in, fat_start + (temp_c * 2), SEEK_SET);
        fwrite(&zero, 2, 1, in);

    }
}

void extractFile(FILE *in,

                 char name[],
                 Fat16BootSector bs,
                 int root_start,
                 unsigned long fat_start,
                 unsigned long data_start,
                 char dir[]) {
    char filename[8] = "        ";
    int i;
    if (strlen(name) > 8) {
        printf("FILENAME INVALID");
        return;
    }

    for (i = 0; i < 8 && name[i] && name[i] != 0; i++)
        filename[i] = name[i];

    fseek(in, root_start, SEEK_SET);
    Fat16Entry entry;
    for (i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof(entry), 1, in);;
        if (entry.filename[0] != '\0') {
            if (0 == memcmp(entry.filename, filename, 8)) {
                break;
            }
        }
    }

    char *out_filename[12] = {0};
    if (entry.ext[0]) {
        char ext[4] = {0};
        strncpy(ext,entry.ext,3);
        ext[3] = '\0';
        strcat(out_filename, name);
        strcat(out_filename, ".");
        strcat(out_filename, ext);
    } else {
        strcat(out_filename, name);
    }

    char out_dir[256] = {0};
    strcpy(out_dir, dir);

    strcat(out_dir, out_filename);
    FILE *out = fopen(out_dir, "ab+");

    readFile(in, out, fat_start, data_start, bs.sectors_per_cluster *
                                             bs.sector_size, entry.starting_cluster, entry.file_size);

    fclose(out);
}

int countEntries(Fat16BootSector bs, FILE *in, int root_start) {
    Fat16Entry entry;
    int i, j = 0;
    fseek(in, root_start, SEEK_SET);
    for (i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof(entry), 1, in);
        if (entry.filename[0] == '\0') {
            break;
        }
        j++;
    }

    return j;
}

Fat16Entry setEntry(char file_name[], char extension[], int count_cluster, int sz) {
    Fat16Entry file;
    int i = 0;
    file.creation_date = time(NULL);
    file.file_size = sz;
    file.creation_time = 8;
    for (i = 0; i < 8; i++)
        file.filename[i] = file_name[i];
    for (i = 0; i < 3; i++)
        file.ext[i] = extension[i];
    file.starting_cluster = count_cluster;
    strcmp(file.reserved, "          ");
    file.attributes = 2;

    return file;
}