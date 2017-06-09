//
// Created by edgar on 09/06/17.
//

#include <stdio.h>
#include "fileutils.h"
#include "string.h"
#include "time.h"
#include "structs.h"

int multiply512(int a, int* multiply ){
    int count = 0;
    *multiply = 0;
    while(count < a){
        count += 512;
        *multiply += 1;
    }
    return count;
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

void writeFile(FILE * dest, FILE * src , int root_start, int data_start, Fat16BootSector bs ){

    fseek(dest, root_start, SEEK_SET);



    short sizes = 0;
    int entry_count = 0;
    int lastm = 0;
    char reservedl[10];
    int i;
    Fat16Entry entry;
    for (i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof (entry), 1, dest);
        entry_count++;
        if (entry.filename[0] == '\0') {
            break;
        }

        strcpy(reservedl ,  entry.reserved);
        sizes += multiply512(entry.file_size, &lastm);
    }

    fseek(src, 0L, SEEK_END);
    int sz = ftell(src);
    fseek(src, 0x0, SEEK_SET);


    Fat16Entry  file ;
    file.creation_date = time(NULL);
    file.file_size = sz;
    file.creation_time = 8;
    strcpy(file.filename, "lixo    ");
    strcpy(file.ext , "txt");
    file.starting_cluster = 22;

    strcmp(file.reserved, "          ");


    int tst = root_start + sizeof(Fat16Entry) * (entry_count - 1);

    fseek(dest, tst, SEEK_SET);
    fwrite(&file, sizeof(Fat16Entry), 1, src);


    int bianka = data_start + sizes;
    fseek(dest, bianka, SEEK_SET);
    fwrite(src, 8 * sizeof(char), 1, dest);
}

void extractFile(FILE * in,
                 FILE * out,
                 char name[],
                 Fat16BootSector bs,
                 int root_start,
                 unsigned long fat_start,
                 unsigned  long data_start){
    char filename[8] = "        ";
    int i;
    if(strlen(name) > 8){printf("FILENAME INVALID");return;}

    for (i = 0; i < 8 && name[i] && name[i] != 0; i++)
        filename[i] = name[i];

    fseek(in, root_start, SEEK_SET);
    Fat16Entry entry;
    for (i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof (entry), 1, in);;
        if (entry.filename[0] != '\0') {
            if(0 == memcmp(entry.filename, filename, 8)){
                break;
            }
        }
    }


    readFile(in, out, fat_start, data_start, bs.sectors_per_cluster *
            bs.sector_size, entry.starting_cluster, entry.file_size);

}