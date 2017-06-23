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

void writeFile(FILE * dest,
               FILE * src ,
               int root_start,
               int data_start,
               Fat16BootSector bs,
               char file_name[],
               char extension[]);

void extractFile(FILE * in,
                 char name[],
                 Fat16BootSector bs,
                 int root_start,
                 unsigned long fat_start,
                 unsigned  long data_start);

int countEntries(Fat16BootSector bs, FILE * in, int root_start);

#endif //FAT16_FILEUTILS_H
