//
// Created by edgar on 09/06/17.
//

#include <stdio.h>
#include "fileutils.h"
#include "string.h"
#include <math.h>
#include "time.h"
#include "structs.h"


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
               char file_name[],
               char extension[]) {

    short cluster = 0xFFFF;
    int fat_start = 512;
    int count_cluster = 0;

    int j;
    int second_fat = bs.sectors_per_fat * bs.sector_size + fat_start;

    for (j = fat_start; j < second_fat; j += 2) {
        fseek(dest, j, SEEK_SET);
        fread(&cluster, 2, 1, dest);
        if (cluster != 0x0000)
            count_cluster++;
    }

    count_cluster;

    fseek(src, 0L, SEEK_END);
    int sz = ftell(src);
    fseek(src, 0x0, SEEK_SET);

    int file_clusters = ceill(sz / 512.f);
    int i;

    short a = 0xFFFF;
    j = fat_start;
    int temp_number = 0;
    for (i = 0; i < file_clusters; i++) {
        short tmpc;
        for (j; j < second_fat; j += 2) {
            fseek(dest, fat_start + temp_number * 2, SEEK_SET);
            temp_number++;
            fread(&tmpc, 2, 1, dest);
            if (tmpc == 0x0000) {

                break;
            }
        }
        short t = 0xFFFF;
        int x = count_cluster + i + 1;
        fseek(dest, fat_start + (temp_number - 1) * 2, SEEK_SET);
        fwrite(&x, 2, 1, dest);
        fseek(dest, second_fat + (temp_number - 1) * 2, SEEK_SET);
        fwrite(&x, 2, 1, dest);
        j += 2;
    }
    fseek(dest, fat_start + (temp_number - 1) * 2, SEEK_SET);
    fwrite(&a, 2, 1, dest);
    fseek(dest, second_fat + (temp_number - 1) * 2, SEEK_SET);
    fwrite(&a, 2, 1, dest);

//
//    if(file_clusters == 1){
//        short a = 0xFFFF;
//        fseek(dest, fat_start + count_cluster  * 2, SEEK_SET);
//        fwrite(&a, 2, 1, dest);
//        fseek(dest, second_fat + count_cluster  * 2, SEEK_SET);
//        fwrite(&a, 2, 1, dest);
//    }else{
//        short c = count_cluster;
//        short cluster_test;
//        int z = 0;
//        for(z ; z < 2 ; z++) {
//            short cluster2 = cluster;
//            for(i = 0; i < file_clusters; i++){
//                fseek(src, (fat_start + ((second_fat - 512) * z)) + ((c + i) * 2), SEEK_SET);
//                fread(&cluster_test, 2, 1, dest);
//                if(cluster_test == 0x0000){
//                    fwrite(&cluster2, 2, 1, dest);
//                    cluster2++;
//                } else {
//                    c++;
//                    i--;
//                }
//            }
//        }
//    }

    Fat16Entry file;
    file.attributes = 2;
    file.creation_date = time(NULL);
    file.file_size = sz;
    file.creation_time = 8;
    ///TODO pass name
    strcpy(file.filename, file_name);
    strcpy(file.ext, extension);
    ///
    file.starting_cluster = count_cluster;

    strcmp(file.reserved, "          ");

    int tst = root_start + sizeof(Fat16Entry) * countEntries(bs, dest, root_start);

    fseek(dest, tst, SEEK_SET);
    fwrite(&file, sizeof(Fat16Entry), 1, dest);


    int current_pos = data_start + ((count_cluster - 2) * 512);

    char buffer[512 * file_clusters];
    memset(buffer, 0, 512 * file_clusters * sizeof(char));
    fseek(dest, current_pos, SEEK_SET);

//    if(sz < 512) {
    fread(buffer, sz, 1, src);
    fwrite(buffer, sz, 1, dest);

//    }
//    else{
//        for(i = 0 ; i < file_clusters ; i++){
//            fseek(src, 512 * i, SEEK_SET);
//            fread(buffer, 512, 1, src);
//            fseek(dest, current_pos + 512 * i, SEEK_SET);
//            fwrite(buffer, 512, 1, dest);
//
//        }
//    }

//    char bianka_vai_dar_uma_olhada_nisso_no_final_de_semana[8] = "12345678";







//
//    fseek(dest, root_start, SEEK_SET);
//
//    short sizes = 0;
//    int entry_count = 0;
//    int lastm = 0;
//    char reservedl[10];
//    int i;
//    Fat16Entry entry;
//    for (i = 0; i < bs.root_dir_entries; i++) {
//        fread(&entry, sizeof (entry), 1, dest);
//        entry_count++;
//        if (entry.filename[0] == '\0') {
//            break;
//        }
//
//        strcpy(reservedl ,  entry.reserved);
//        sizes += multiply512(entry.file_size, &lastm);
//    }
//

//
//
//    Fat16Entry  file ;
//    file.creation_date = time(NULL);
//    file.file_size = sz;
//    file.creation_time = 8;
//    strcpy(file.filename, "lixo    ");
//    strcpy(file.ext , "txt");
//    file.starting_cluster = 22;
//
//    strcmp(file.reserved, "          ");
//
//
//    int tst = root_start + sizeof(Fat16Entry) * (entry_count - 1);
//
//    fseek(dest, tst, SEEK_SET);
//    fwrite(&file, sizeof(Fat16Entry), 1, src);
//
//
//    int bianka = data_start + sizes;
//    fseek(dest, bianka, SEEK_SET);
//    fwrite(src, 8 * sizeof(char), 1, dest);
}


void extractFile(FILE *in,
                 FILE *out,
                 char name[],
                 Fat16BootSector bs,
                 int root_start,
                 unsigned long fat_start,
                 unsigned long data_start) {
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


    readFile(in, out, fat_start, data_start, bs.sectors_per_cluster *
                                             bs.sector_size, entry.starting_cluster, entry.file_size);


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


