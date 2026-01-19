cmake_minimum_required(VERSION 4.0)
project(labip C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(labip
        bmpreader.c
        bmpreader.h
        filter.c
        filter.h
        main.c
        )
