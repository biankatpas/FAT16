//
// Created by edgar on 07/06/17.
//

#ifndef FAT16_PRINTS_H
#define FAT16_PRINTS_H

#include <stdio.h>


int printPartitionTable(PartitionTable pt[]);

void printBootSector(Fat16BootSector bs, FILE * in);

int printRootDirectory( Fat16BootSector bs, FILE * in);

void printFileInfo(Fat16Entry *entry);



#endif //FAT16_PRINTS_H
