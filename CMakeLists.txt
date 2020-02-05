cmake_minimum_required(VERSION 3.7)
project(adi_dsp_programmer C)

set(CMAKE_C_STANDARD 99)

add_executable(adi_dsp_programmer
        main.c
        i2c.c
        i2c.h
        system_files/SigmaStudioFW.c
        system_files/SigmaStudioFW.h
        download.c
        download.h)

target_link_libraries(adi_dsp_programmer m)