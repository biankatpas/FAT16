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


void parseInput(char* in, int size, char* file_name, char * extension){
    int i, j;
    int count = 0;
    for(i=size-1 ; i > 0; i--){
        if(in[i] == '/'){
            for(j=1; j < size - i ; j++){
                if(in[i + j] == '.' || in[i+j] == '\0') {
                    if(in[i+j] == '.'){
                        extension[0] = in[i+j+1];
                        extension[1] = in[i+j+2];
                        extension[2] = in[i+j+3];
                    }
                    break;
                }else{
                    if(count < 8) {
                        file_name[count] = in[i+j];
                    }
                    count ++;
                }
            }
            break;
        }

    }

    while(count < 8){
        file_name[count] = ' ';
        count++;
    }
}

void int2Hex(int quotient, char ret[]) {
    int temp, i = 3;


    while (quotient != 0) {
        temp = quotient % 16;

        //To convert integer into character
        if (temp < 10)
            temp = temp + 48;
        else
            temp = temp + 55;

        if (i == -1)
            break;

        ret[i--] = temp;
        quotient = quotient / 16;
    }

}

int multiply512(int a, int *multiply) {
    int count = 0;
    *multiply = 0;
    while (count < a) {
        count += 512;
        *multiply += 1;
    }
    return count;
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
               char extension[3]) {


    /// Gravar o endereço do proximo
    short cluster = 0xFFFF;
    int fat_start = 512;
    int count_cluster = 0;

    int j;
    int second_fat = bs.sectors_per_fat * bs.sector_size + fat_start;



    short start_cluster = 0;
    for (j = fat_start; j < second_fat; j += 2) {
        fseek(dest, j, SEEK_SET);
        fread(&cluster, 2, 1, dest);
        if (cluster == 0x0000){
            break;
        }
        start_cluster++;
    }


//    for (j = fat_start; j < second_fat; j += 2) {
//        fseek(dest, j, SEEK_SET);
//        fread(&cluster, 2, 1, dest);
//        if (cluster != 0x0000)
//            count_cluster++;
//    }


    fseek(src, 0L, SEEK_END);
    int sz = ftell(src);
    fseek(src, 0x0, SEEK_SET);

    int file_clusters = ceill(sz / 512.f);
    int i;

    short a = 0xFFFF;
    j = fat_start;
    int temp_number = 0;


    short cluster_map[file_clusters+1];
    int map_count_cluster = -1;
    short temp_start_cluster = start_cluster;

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
        j+=2;
    }

    cluster_map[file_clusters] = 0xFFFF;

    int confia = 1;

    for(i = 0 ;i< file_clusters; i++){
        fseek(dest, fat_start + cluster_map[i] * 2, SEEK_SET);
        fwrite(&cluster_map[i+1], 2, 1, dest);
        fseek(dest, second_fat + cluster_map[i] * 2, SEEK_SET);
        fwrite(&cluster_map[i+1], 2, 1, dest);
    }
//        short t = 0xFFFF;
//        int x = temp_start_cluster;
//        fseek(dest, fat_start + (temp_number - 1) * 2, SEEK_SET);
//        fwrite(&x, 2, 1, dest);
//        fseek(dest, second_fat + (temp_number - 1) * 2, SEEK_SET);
//        fwrite(&x, 2, 1, dest);
//        j += 2;
//    }
//
//    fseek(dest, fat_start + (temp_number - 1) * 2, SEEK_SET);
//    fwrite(&a, 2, 1, dest);
//    fseek(dest, second_fat + (temp_number - 1) * 2, SEEK_SET);
//    fwrite(&a, 2, 1, dest);

    /*Fat16Entry file;
    file.attributes = 2;
    file.creation_date = time(NULL);
    file.file_size = sz;
    file.creation_time = 8;
    strcpy(file.filename, file_name);
    strcpy(file.ext, extension);
    ///
    file.starting_cluster = count_cluster;
    strcmp(file.reserved, "          ");
    */

    Fat16Entry file = setEntry(file_name, extension, start_cluster, sz);
    int tst = root_start + sizeof(Fat16Entry) * countEntries(bs, dest, root_start);

    fseek(dest, tst, SEEK_SET);
    fwrite(&file, sizeof(Fat16Entry), 1, dest);


    int current_pos = data_start + ((start_cluster - 2) * 512);

    char buffer[512 * file_clusters];
    memset(buffer, 0, 512 * file_clusters * sizeof(char));
    fseek(dest, current_pos, SEEK_SET);

    fread(buffer, sz, 1, src);
    fwrite(buffer, sz, 1, dest);
}


void extractFile(FILE *in,

                 char name[],
                 Fat16BootSector bs,
                 int root_start,
                 unsigned long fat_start,
                 unsigned long data_start,
                 char dir[]   ) {
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

    char * out_filename[12] = {0};
    if(entry.ext[0]){
        strcat(out_filename,name);
        strcat(out_filename,".");
        strcat(out_filename,entry.ext);
    }

    strcat(dir, out_filename);
    FILE * out = fopen(dir,
                       "ab+");


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

        if (entry.filename[0] != '\0') {
            j++;
        }
    }

    return j;
}

Fat16Entry setEntry(char file_name[], char extension[], int count_cluster, int sz) {
    Fat16Entry file;

    file.attributes = 2;
    file.creation_date = time(NULL);
    file.file_size = sz;
    file.creation_time = 8;
    strcpy(file.filename, file_name);
    strcpy(file.ext, extension);
    ///
    file.starting_cluster = count_cluster;
    strcmp(file.reserved, "          ");

    return file;
}


