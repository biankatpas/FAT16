/* 
 * File:   main.cpp
 * Author: biankatpas
 * Created on 28 de Maio de 2017, 21:34
 *
 */


#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "prints.h"
#include "fileutils.h"

char disk_image[256] = {0};
char input_dir[256] = {0};
char output_dir[256] = {0};
int parseArgs(int argc, char** argv){
    int i;
    for(i = 0; i< argc; i++){
        if(argv[i][0] == '-'){
            if(strcmp(argv[i], "-f") == 0 ){
                strcpy(disk_image, argv[i+1]);
                return 1;
            }
            if(strcmp(argv[i], "-i") == 0){
                strcpy(input_dir, argv[i+1]);
                return 1;
            }

            if(strcmp(argv[i], "-o") == 0){
                strcpy(output_dir, argv[i+1]);
                return 1;
            }

            if(strcmp(argv[i], "-help") == 0){
                printf("Usage:\n\t-f\tImage File\n\t-i\tInput Folder\n\t-o\tOutput Folder\n\t-help\tShows this help\n");
                return 0;
            }




        }

    }

}



int main(int argc, char **argv) {

    if(!parseArgs(argc, argv))
        return 1;

    FILE *in = fopen(disk_image, "r+"), *out, *write;
    PartitionTable pt[4];
    Fat16BootSector bs;
    short op = 0;
    unsigned long fat_start, root_start, data_start;
    char filename_aux[8];

    fseek(in, 0x1BE, SEEK_SET); // go to partition table start
    fread(pt, sizeof(PartitionTable), 4, in); // read all four entries

    fseek(in, 0x000, SEEK_SET);
    fread(&bs, sizeof(Fat16BootSector), 1, in);

    // Calculate start offsets of FAT, root directory and data
    fat_start = ftell(in) + (bs.reserved_sectors - 1) * bs.sector_size;
    root_start = fat_start + bs.sectors_per_fat * bs.number_of_fats *
                             bs.sector_size;
    data_start = root_start + bs.root_dir_entries * sizeof(Fat16Entry);

    printf("********** FAT 16 **********\n");
    printf("1 - info\n");
    printf("2 - ls\n");
    printf("3 - cat\n");
    printf("4 - wr\n");
    printf("5 - rm\n");
    printf("****************************\n");
    scanf("%hd", &op);

    switch (op) {
        case 1:
            printPartitionTable(pt);
            printBootSector(bs, in);
            printf("FAT start at %08X, root dir at %08X, data at %08X\n\n", fat_start, root_start, data_start);
            break;

        case 2:
            fseek(in, root_start, SEEK_SET);
            printRootDirectory(bs, in);
            break;

        case 3:
            printf("\n---------- Informe o arquivo para extrair os dados ----------\n");
            scanf("%s", filename_aux);
            out = fopen("/home/biankatpas/Dropbox/NetBeansProjects/Fat16/out.txt",
                        "wb"); // write the file contents to disk
            extractFile(in, out, filename_aux, bs, root_start, fat_start, data_start);
            fclose(out);
            break;
        case 4:
            printf("\n---------- Informe o caminho total (FULL_PATH) do arquivo para gravar os dados ----------\n");
            char * name = malloc(256 * sizeof(char));
            write = fopen(name, "rb");

            char extension[3] = {0};
            char file_name[8] = {0};
            parseInput(name, 256, file_name, extension);

            writeFile(in, write, root_start, data_start, bs, file_name, extension);
            printf("finished");
            fclose(write);
            break;
        default:
            printf("Informe uma op valida");
            break;
    }


    fclose(in);

    return 0;
}



