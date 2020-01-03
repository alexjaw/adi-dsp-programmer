#include <stdio.h>
#include <string.h>
#include <unistd.h>  //usleep
#include "i2c.h"
#include "download.h"

#define ARG_RW       1  // index in the arguments list
#define ARG_ADDR8    2
#define ARG_REG      3
#define ARG_N_BYTES  4
#define ARG_DOWNLOAD 1

unsigned char rw     = 0;  // read = 0
unsigned char addr8  = 0;  // 8-bit i2c addr for read
unsigned int reg     = 0;  // dsp reg addr
unsigned int n_bytes = 0;  // number of bytes to read/write

int main(int argc, char *argv[]) {
    // Parse arguments
    // Only 2 arguments = download dsp configuration
    if(argc == 2){
        if(!strcmp(argv[ARG_DOWNLOAD], "download")){
            printf("arg %i: download\n", ARG_DOWNLOAD);
            i2cOpen();
            usleep(1000000);
            download();
            i2cClose();
        }else{
            printf("ERROR. arg %i: UNKNOWN\n", ARG_DOWNLOAD);
            return 1;
        }
        return 0;
    }
    // More than 2 args = read/write I/O to individual regs
    if(argc > 2){
        //RW
        if(!strcmp(argv[ARG_RW], "r")) {
            printf("arg %i: r\n", ARG_RW);
            rw = 0;
        }else if(!strcmp(argv[ARG_RW], "w")){
            printf("arg %i: w\n", ARG_RW);
            rw = 1;
        }else{
            printf("ERROR. arg %i: %s\n", ARG_RW, argv[ARG_RW]);
            return 1;
        }
        //8 bit address
        sscanf(argv[ARG_ADDR8], "%xu", &addr8);
        printf("arg %i: 0x%02x\n", ARG_ADDR8, addr8);
        sscanf(argv[ARG_REG], "%xu", &reg);
        printf("arg %i: 0x%04x\n", ARG_REG, reg);
        sscanf(argv[ARG_N_BYTES], "%iu", &n_bytes);
        printf("arg %i: %i\n", ARG_N_BYTES, n_bytes);
    }

    // IO
    unsigned char inbuf[n_bytes];
    i2cOpen();
    addr8 = (addr8>>1)+rw;
    int err = read_i2c_block_data(addr8, reg, inbuf, n_bytes);
    if(err){
        printf("Failed to read\n");
    }
    if(n_bytes == 2){
        printf("RECV: 0x%02x 0x%02x\n", inbuf[0], inbuf[1]);
    }else{
        for(int n=0; n<n_bytes; n+=4){
            printf("RECV: 0x%02x 0x%02x 0x%02x 0x%02x\n", inbuf[n], inbuf[n+1], inbuf[n+2], inbuf[n+3]);
        }
    }
    i2cClose();

    return 0;
}