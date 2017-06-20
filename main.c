/* 
 * File:   main.cpp
 * Author: biankatpas
 * Author: edgarbrissow
 * Created on 28 de Maio de 2017, 21:34
 *
 */


#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "prints.h"
#include "fileutils.h"


int main(int argc, char** argv) {
    FILE * in = fopen("/home/edgar/workspace/FAT16/build/disco2.IMA", "r+"), * out;
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
    printRootDirectory(bs, in);


    printf("\n---------- Informe o arquivo para extrair os dados ----------\n");
    printf("Nome: ");
    scanf("%s", filename_aux);
    out = fopen("out.txt", "wb"); // write the file contents to disk
    extractFile(in, out, filename_aux, bs, root_start, fat_start, data_start);

//    FILE*  f = fopen("/home/edgar/workspace/FAT16/build/lixo.txt", "rb");
//    writeFile(in, f, root_start, data_start, bs);
//    fclose(in);
//    fclose(f);
    fclose(out);
    return 0;
}



