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
#include "i2c.h"

#define I2C_BUS "/dev/i2c-1"
#define REG_SIZE 2     //number of bytes for a dsp register address

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
*/

int write_i2c_block_data(
        unsigned char addr,
        unsigned short reg,
        const unsigned char *val,
        unsigned short val_length)
{
    unsigned char isError = 0;
    unsigned short outbuf_size = REG_SIZE + val_length;  //reg is 2 bytes
    unsigned char* outbuf;
    outbuf = (unsigned char*) calloc(outbuf_size, sizeof(unsigned char));  //todo: Should outbuf have a fixed size?

    // Set the reg address
    outbuf[0] = (unsigned char)((reg >> 8) & 0xFF);
    outbuf[1] = (unsigned char)(reg & 0xFF);

    /*
     * The second byte indicates the value to write.  Note that for many
     * devices, we can write multiple, sequential registers at once by
     * simply making outbuf bigger.
     *
     * With memcpy we can efficiently copy the content of val to outbuf,
     * but its important to start the copy AFTER the reg_addr.
     */
    if (!memcpy(&(outbuf[REG_SIZE]), val, val_length)) {
        perror("Failed to copy val to outbuf\n");
        isError = 1;
    }

    /* Transfer the i2c packets to the kernel and verify it worked */
    if (!isError){
        if (write_i2c_block_data_raw(addr, outbuf, outbuf_size)) {
            fprintf(stderr, "Unable to send data over i2c for device: 0x%02x\n", addr);
            isError = 1;
        }
    }

    if (outbuf) {
        free(outbuf);
    }
    if (isError){
        return 1;
    } else {
        return 0;
    }
}

// The onlu porpose of this function is to separate the actual call of OS
// ioctl()
static int send_data(struct i2c_rdwr_ioctl_data* packets) {
    if(ioctl(g_i2cFile, I2C_RDWR, packets) < 0) {
        return 1;
    }
    return 0;
}