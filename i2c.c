//
// Created by alexander on 2020-01-03.
//

#include "i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //memcpy
#include <unistd.h>  //close
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include "i2c.h"

#define I2C_BUS "/dev/i2c-1"
#define REG_SIZE 2     //number of bytes for a dsp register address
// todo: how set baudrate? max 400kHz

// I2C Linux device handle
static int g_i2cFile;

// function prototypes
static int send_data(struct i2c_rdwr_ioctl_data* packets);
static int set_reg_addr(unsigned short reg, unsigned char* outbuf, unsigned short reg_length);

// open the Linux device
int i2cOpen(){
    static int initDone = 0;

    if (initDone) {
        return 0;
    }

    g_i2cFile = open(I2C_BUS, O_RDWR);
    if (g_i2cFile < 0) {
        perror("i2cOpen");
        return 1;
    }

    initDone = 1;

    return 0;
}

// close the Linux device
int i2cClose(){
    printf("Closing...\n");
    close(g_i2cFile);
    return 0;
}

int read_i2c_byte(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *data_buf){
    enum {REG_SZ = 1, DATA_SZ = 1, MSG_SZ=2};
    uint8_t  reg_buf[REG_SZ] = {reg_addr};

    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[MSG_SZ];

    messages[0].addr  = i2c_addr;
    messages[0].flags = 0;
    messages[0].len   = REG_SZ;
    messages[0].buf   = reg_buf;

    messages[1].addr  = i2c_addr;
    messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/;
    messages[1].len   = DATA_SZ;
    messages[1].buf   = data_buf;

    packets.msgs      = messages;
    packets.nmsgs     = MSG_SZ;

    if (send_data(&packets)) {
        return 1;
    }

    return 0;
}

/*
 * In order to read a register, we first do a "dummy write" by writing
 * 0 bytes to the register we want to read from.  This is similar to
 * the packet in set_i2c_register, except it's 1 byte rather than 2.
 */
int read_i2c_block_data(
        unsigned char addr,
        unsigned short reg,
        unsigned char *val,
        unsigned short val_length)
{
    unsigned char isError = 0;
    unsigned char outbuf[REG_SIZE];
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    // Set the reg address
    outbuf[0] = (unsigned char)((reg >> 8) & 0xFF);
    outbuf[1] = (unsigned char)(reg & 0xFF);

    if (!isError){
        messages[0].addr  = addr;
        messages[0].flags = 0;
        messages[0].len   = REG_SIZE;
        messages[0].buf   = outbuf;

        /* The data will get returned in this structure */
        messages[1].addr  = addr;
        messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/;
        messages[1].len   = val_length;
        messages[1].buf   = val;

        /* Send the request to the kernel and get the result back */
        packets.msgs      = messages;
        packets.nmsgs     = 2;

        if (send_data(&packets)) {
            isError = 1;
        }
    }

    if (isError){
        return 1;
    } else {
        return 0;
    }
}


/*
    generate an i2c message package out of an array of chars
    the function cares not about the content of the char array.
    it should contain the register + data, split up into bytes

    f.X: to write to a 255x16 register (16 bit addresses with
    8 bit values), you would send: (pseudo code)

    (addr, outbuf, outbuf_size) =
    (addr, {register, (value >> 8), (value & 0xff)}, 3)

*/

int write_i2c_block_data_raw(
        unsigned char addr,
        unsigned char *outbuf,
        unsigned short outbuf_size) {

    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    messages[0].addr  = addr;
    messages[0].flags = 0;
    messages[0].len   = outbuf_size;
    messages[0].buf   = outbuf;

    packets.msgs  = messages;
    packets.nmsgs = 1;

    return send_data(&packets);
}

/*
    wrapper function for write_i2c_block_data_raw
    to format i2c messages for FFxFFFF (address x value)
    organized registers.

    it assumes that the register is 2 bytes (unsigned short)
    and that the value is 1 byte (unsigned char)

    On linux user space max val_length <= 8192 (0x2000) bytes
    see: https://www.raspberrypi.org/forums/viewtopic.php?t=116311
    So here we split the buffer up if val_length is too large.
*/

int write_i2c_block_data(
        unsigned char addr,
        unsigned short reg,
        const unsigned char *val,
        unsigned short val_length)
{
    const unsigned char DSP_WORD = 4;  // i.e. 4 bytes
    const unsigned short VAL_LENGTH_MAX = 8188;//1024;//8188; // must be a value divisible with 4, ie 8188
    unsigned char isError = 0;
    unsigned short outbuf_size = REG_SIZE + val_length;  //reg is 2 bytes
    unsigned char* outbuf;
    unsigned short sent = 0;
    unsigned short remains = val_length;
    unsigned short val_length_to_send = 0;
    const unsigned char *val_start = val;
    int err_tmp = 0;

    while (sent != val_length){
        if (remains > VAL_LENGTH_MAX){
            outbuf_size = REG_SIZE + VAL_LENGTH_MAX;
            val_length_to_send = VAL_LENGTH_MAX;
        } else {
            outbuf_size = REG_SIZE + remains;
            val_length_to_send = remains;
        }
        outbuf = (unsigned char*) calloc(outbuf_size, sizeof(unsigned char));  //todo: Should outbuf have a fixed size?
        if (outbuf == NULL){
            fprintf(stderr, "ERROR, calloc returned NULL-pointer\n");
            isError = 1;
        }

        // Set the reg address
        // OBSERVE THAT address increment is in word size
        outbuf[0] = (unsigned char)(((reg+sent/DSP_WORD) >> 8) & 0xFF);
        outbuf[1] = (unsigned char)((reg+sent/DSP_WORD) & 0xFF);

        /*
         * The second byte indicates the value to write.  Note that for many
         * devices, we can write multiple, sequential registers at once by
         * simply making outbuf bigger.
         *
         * With memcpy we can efficiently copy the content of val to outbuf,
         * but its important to start the copy AFTER the reg_addr.
         */
        if (!memcpy(&(outbuf[REG_SIZE]), val_start, val_length_to_send)) {
            perror("Failed to copy val to outbuf\n");
            isError = 1;
        }

        /* Transfer the i2c packets to the kernel and verify it worked */
        /*if (((val_length > VAL_LENGTH_MAX) )) {
            fprintf(stderr, "TO SEND: reg = 0x%02x%02x, outbuf_size = %d, outbuf[%d] = {%02x,%02x,%02x,%02x}\n",
                    outbuf[0], outbuf[1], outbuf_size, sent+2, outbuf[2], outbuf[3], outbuf[4], outbuf[5]);
            //fprintf(stderr, "val[%d] = {%02x,%02x,%02x,%02x}\n", sent, val[sent], val[sent+1], val[sent+2], val[sent+3]);
        }*/
        if (!isError){
            err_tmp = write_i2c_block_data_raw(addr, outbuf, outbuf_size);
            if (err_tmp) {
                fprintf(stderr, "Unable to send data over i2c for device: 0x%02x\n", addr);
                fprintf(stderr, "reg: 0x%04x, val_length: %d\n", reg, val_length);
                isError = 1;
            }
        }

        if (outbuf) {
            free(outbuf);
        }

        sent += val_length_to_send;
        remains = val_length - sent;
        val_start += VAL_LENGTH_MAX;
    }

    if (isError){
        return 1;
    } else {
        return 0;
    }
}

// The only purpose of this function is to separate the actual call of OS ioctl()
static int send_data(struct i2c_rdwr_ioctl_data* packets) {
    /* messages[0].addr  = addr;
     * messages[0].flags = 0;
     * messages[0].len   = outbuf_size;
     * messages[0].buf   = outbuf;
     * packets.msgs  = messages;
     * packets.nmsgs = 1;
     * */
    if(ioctl(g_i2cFile, I2C_RDWR, packets) < 0) {
        fprintf(stderr, "ERROR, ioctl returned errno %s\n", strerror(errno));
        fprintf(stderr, "len: %d\n", packets->msgs->len);
        return 1;
    }
    return 0;
}