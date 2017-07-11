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

void waitEnter() {
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
    if (c == EOF) {
        // input stream ended, do something about it, exit perhaps
    } else {
        printf("\nPressione Enter para continuar..\n");
        getchar();
    }
}

int parseArgs(int argc, char **argv) {
    int i;
    for (i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-f") == 0) {
                strcpy(disk_image, argv[i + 1]);
            }
            if (strcmp(argv[i], "-i") == 0) {
                strcpy(input_dir, argv[i + 1]);
            }

            if (strcmp(argv[i], "-o") == 0) {
                strcpy(output_dir, argv[i + 1]);
            }

            if (strcmp(argv[i], "-help") == 0) {
                printf("Usage:\n\t-f\tImage File\n\t-i\tInput Folder\n\t-o\tOutput Folder\n\t-help\tShows this help\n");
                return 0;
            }
        }
    }
}


int main(int argc, char **argv) {

    if (!parseArgs(argc, argv))
        return 1;

    FILE *in = fopen(disk_image, "r+"), *write;
    if (!in) {
        printf("Arquivo de imagem nao foi encontrado");
        exit(1);
    }

    PartitionTable pt[4];
    Fat16BootSector bs;
    short op = 0;
    unsigned long fat_start, root_start, data_start;
    int control = 1;

    fseek(in, 0x1BE, SEEK_SET); // go to partition table start
    fread(pt, sizeof(PartitionTable), 4, in); // read all four entries

    fseek(in, 0x000, SEEK_SET);
    fread(&bs, sizeof(Fat16BootSector), 1, in);

    // Calculate start offsets of FAT, root directory and data
    fat_start = ftell(in) + (bs.reserved_sectors - 1) * bs.sector_size;
    root_start = fat_start + bs.sectors_per_fat * bs.number_of_fats *
                             bs.sector_size;
    data_start = root_start + bs.root_dir_entries * sizeof(Fat16Entry);


    while (control == 1) {


        printf("*********** FAT 16 ***********\n");
        printf("1 - Informacoes do boot sector\n");
        printf("2 - Listar arquivos\n");
        printf("3 - Extrair arquivo\n");
        printf("4 - Escrever arquivo\n");
        printf("5 - Remover arquivo \n");
        printf("6 - Sair\n");
        printf("******************************\n");
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
                char filename_aux[8] = {0};
                scanf("%s", filename_aux);
                // write the file contents to disk
                extractFile(in, filename_aux, bs, root_start, fat_start, data_start, output_dir);
                break;
            case 4:
                printf("\n---------- Informe o nome do arquivo para gravar os dados ----------\n");
                char *name = malloc(256 * sizeof(char));
                char *tname = malloc(256 * sizeof(char));
                int pass = 0;

                do {
                    strcpy(tname, input_dir);
                    scanf("%s", name);
                    strcat(tname, name);
                    write = fopen(tname, "rb");
                    if (write == 0)
                        printf("Arquivo nao encontrado \nDigite novamente:");
                    else
                        pass = 1;
                } while (pass == 0);


                char extension[3] = {0};
                char file_name[8] = {0};
                parseInput(tname, 256, file_name, extension);

                writeFile(in, write, root_start, data_start, bs, file_name, extension, fat_start);
                printf("finished");
                fclose(write);
                free(name);
                free(tname);
                break;

            case 5:
                printf("\n---------- Informe o arquivo para deletar ----------\n");
                char filename_del[8] = {0};
                scanf("%s", filename_del);
                deleteFile(in, filename_del, bs, fat_start, root_start);
                break;

            case 6:
                printf("\nEncerrando....");
                control = 0;
                break;

            default:
                printf("Informe uma op valida");
                break;
        }
        waitEnter();
        int i;
        for (i = 0; i < 50; i++)
            printf("\n");
    }

    fclose(in);
    return 0;
}



