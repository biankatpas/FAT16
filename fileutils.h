//
// Created by edgar on 09/06/17.
//

#ifndef FAT16_FILEUTILS_H
#define FAT16_FILEUTILS_H

#include <stdio.h>
#include "structs.h"

void readFile(FILE * in, FILE * out,
              unsigned long fat_start,
              unsigned long data_start,
              unsigned long cluster_size,
              unsigned short cluster,
              unsigned long file_size);

void deleteFile(FILE *in,
                char name[],
                Fat16BootSector bs,
                unsigned long fat_start,
                unsigned long root_start);

void writeFile(FILE * dest,
               FILE * src ,
               int root_start,
               int data_start,
               Fat16BootSector bs,
               char file_name[8],
               char extension[3]);

void extractFile(FILE * in,
                 char name[],
                 Fat16BootSector bs,
                 int root_start,
                 unsigned long fat_start,
                 unsigned  long data_start,
                 char dir[]);

int countEntries(Fat16BootSector bs, FILE * in, int root_start);

void parseInput(char* in, int size, char* file_name, char * extension);

Fat16Entry setEntry(char file_name[], char extension[], int count_cluster, int sz);

#endif //FAT16_FILEUTILS_H
