#include <stdio.h>
#include <string.h>
#include <unistd.h>  //usleep
#include <math.h>
#include "i2c.h"
#include "download.h"

#define ARG_RW       1  // index in the arguments list
#define ARG_ADDR8    2
#define ARG_REG      3
#define ARG_N_BYTES  4
#define ARG_VOL      5
#define ARG_DOWNLOAD 1

unsigned char rw     = 0;  // read = 0
unsigned char addr8  = 0;  // 8-bit i2c addr for read
unsigned int reg     = 0;  // dsp reg addr
unsigned int n_bytes = 0;  // number of bytes to read/write

#define F824_ONE   0x01000000  //32-bit
#define F824_SIGN  0xF0000000  //-16 in 8.24
#define F824_M60DB_MASK 0xFFFFC00  // todo: AJ what should this be for 8.24???
//adau1452
static float conv523toFloat(int i523) {
    float dec;
    // sign extend
    if ( i523 & 0x8000000 )  //#define F523_SIGN  0x8000000  //-16 in 5.23
        i523 |= 0xF0000000; // sign extend
    // and devide
    dec =  (float)i523/(float)(1<<23);
    //DBGMSG(LOG_INFO,"0x%08x (8.24: 0x%08x) = %f\n",i523,convFloatTo824(dec),dec);
    return dec;
}
static int convFloatTo523(float dec) {
    int t523;
    t523 = (dec * (float)(1<<23));
    // sign shorten
    t523 &= 0xFFFFFFFF;
    //DBGMSG(LOG_INFO,"%f = 0x%08x (8.24: 0x%08x)\n",dec,t523, convFloatTo824(dec));
    return t523;
}

//adau1467
float conv824toFloat(int i824){
    float dec;
    // sign extend
    if ( i824 & F824_SIGN )
        i824 |= 0xF0000000; // sign extend
    // and devide
    dec =  (float)i824/(float)(1<<24);
    printf("0x%08x (5.23: 0x%08x) = %f\n",i824,convFloatTo523(dec),dec);
    return dec;
}
int convFloatTo824(float dec){
    int t824;
    t824 = (dec * (float)(1<<24));
    // sign shorten
    t824 &= 0x0FFFFFFF;
    printf("%f = 0x%08x (5.23: 0x%08x)\n",dec,t824, convFloatTo523(dec));
    return t824;
}

void dec2hex(double x, unsigned char buf[], int buf_sz){
    unsigned int r, r523;
    int a = 8, b = 24;

    if (x >= 0) {
        r = (unsigned int)round(pow(2, b) * x);
        r523 = (unsigned int)round(pow(2, 23) * x);
    }else{
        r = (unsigned int)round(pow(2, a+b) - (pow(2, b) * fabs(x)));
        r523 = (unsigned int)round(pow(2, 5+23) - (pow(2, 23) * fabs(x)));
    }
    buf[0] = r >> 24;
    buf[1] = r >> 16;
    buf[2] = r >> 8;
    buf[3] = r;
    printf("dec2hex(%f) in 8.24: %i, 0x%08x. In 5.23: %i, 0x%08x\n", (float)x, r, r, r523, r523);
    //printf("dec2hex: 0x%02x 0x%02x 0x%02x 0x%02x\n", buf[0], buf[1], buf[2], buf[3]);
}

int gain2bytes(double g, unsigned char buf[]){
    if(g >= 0 && g <= 1){
        dec2hex(g, buf, 4);
        return 0;
    }else{
        printf("ERROR. Allowed gain: 0 <= gain <= 1\n");
        return -1;
    }
}

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

        // temp for gain adjustment
        if(argc == 6){
#define BUF_BYTES       4
#define ALPHA_REG_BYTES 8
            int err = 0;
            float vol = 0.0, g = 0.0;
            unsigned short gain_reg = 1242;
            printf("using fixed gain adj reg: 0x%04x\n", gain_reg);
            unsigned short alpha_reg = 1243;
            unsigned char alpha_reg_data[ALPHA_REG_BYTES] = {0x00, 0xff, 0xfb, 0xd5, 0x00, 0x00, 0x04, 0x2b};
            sscanf(argv[ARG_VOL], "%f", &vol);
            if(vol < 0 || vol > 100){
                printf("ERROR. vol: 0 <= vol <= 100\n");
                return -1;
            }
            printf("arg %i: %f\n", ARG_VOL, vol);
            g = pow(10,(vol-100.0)/20.0);
            printf("vol2gain(%f): %f\n", vol, g);
            unsigned char buf[BUF_BYTES];
            if(gain2bytes(g, buf)){
                printf("ERROR\n");
                return -1;
            }
            printf("gain2bytes: 0x%02x 0x%02x 0x%02x 0x%02x\n", buf[0], buf[1], buf[2], buf[3]);
            //printf("OBSERVE, skipping i2c execution of command\n");
            //return 0;
            i2cOpen();
            addr8 = addr8>>1;
            err = write_i2c_block_data(addr8, gain_reg, buf, BUF_BYTES);
            if(err){
                printf("Failed to set gain\n");
            }
            err = write_i2c_block_data(addr8, alpha_reg, alpha_reg_data, ALPHA_REG_BYTES);
            if(err){
                printf("Failed to set alpha\n");
            }
            i2cClose();
            return 0;
        }
    }

    // IO
    unsigned char inbuf[n_bytes];
    i2cOpen();
    addr8 = (addr8>>1);
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