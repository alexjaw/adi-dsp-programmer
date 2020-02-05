//
// Created by alexander on 2020-01-03.
//

#include "SigmaStudioFW.h"
#include <unistd.h>
#include "../i2c.h"
#include <stdio.h>

void SIGMA_READ_REGISTER( int devAddress, int address, int length, ADI_REG_TYPE *pData ){
    // TODO: implement
}

void SIGMA_WRITE_REGISTER_BLOCK( int devAddress8, int address, int length, ADI_REG_TYPE *pData ){
    //printf("In SIGMA_WRITE_REGISTER_BLOCK\n");
    write_i2c_block_data(devAddress8>>1, address, pData, length);
}

void SIGMA_WRITE_DELAY( int devAddress, int length, ADI_REG_TYPE *pData ){
    if(pData[1] == 0xFF){
        usleep(10000);
    }else if(pData[1] == 0x01){
        usleep(1);
    }else{
        usleep(100000);
    }
}