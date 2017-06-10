cmake_minimum_required(VERSION 3.7)
project(FAT16)

set ( CMAKE_C_FLAGS "-lm")

set(SOURCE_FILES
        main.c structs.h prints.h prints.c fileutils.c fileutils.h)

add_executable(FAT16 ${SOURCE_FILES})