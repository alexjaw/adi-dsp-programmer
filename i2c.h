/*
 * i2c.h
 *
 *  Functions for i2c NI ADAU dsp (1442, 1452) communication on Linux/RPi
 *
 *	The dsp is capable of single-word (2-byte) and burst mode communication
 *	Byte
 *	0    : device address, 7 bit, + 1 r/w bit
 *	1..2 : register address, always 2 bytes
 *	3..n : data, the number of bytes depends on the type of data that is being written
 *	       - control registers are 2 bytes wide
 *	       - memories are 4 bytes wide
 *	       In burst mode, the master can send a start register address and then send
 *	       a big chunk of data. The dsp will auto-increment the register addresses. The
 *	       dsp does also know when it receives data for control and memory address.
 *
 *  Logic analyzer output when writing to control register 0xE000 (2 bytes wide):
 *  START|h38 WR|ACK|he0|ACK|h00|ACK|h24|ACK|h42|ACK|STOP
 *
 *  START     : master initiates i2c communication
 *  h38 WR    : master addresses dsp with address 0x70 (0x38<<1) in write mode
 *  ACK       : acknowledgment from slave device, sent from slave after every byte
 *  he000     : reg addr is 0xe000
 *  h2442     : data, 2 bytes
 *  STOP      : master end the write cycle
 *
 *
 *  Logic analyzer output when reading from control register 0xE000:
 *  START|h38 WR|ACK|he0|ACK|h00|ACK|RESTART|h38 RD|ACK|h24|ACK|h42|NAK|STOP
 *
 *  - Master starts by sending a WR + reg addr of the register to be read from, in this case
 *  master is addressing dsp 0x70 and reg addr 0x001c
 *  - Master sends a re-start (essential for reading from ADAU) in order to start reading
 *  - Master send the read instruction
 *  - Slave sends the data 0x2442 from the requested reg addr
 *  - Master sends NAK to stop slave from sending
 *  - Master send STOP to end the reading cycle
 *
 *	Bug on RPi:
 *  These two lines make it possible to execute i2c RESTART on RPi
 *  If you find that your I2C commands stop working, with missing,
 *  RESTART, replaced with STOP...START. Then execute these commands:
 *      sudo chmod 666 /sys/module/i2c_bcm2708/parameters/combined
 *      sudo echo -n 1 > /sys/module/i2c_bcm2708/parameters/combined
 *  ref: https://github.com/alexjaw/Adafruit_Python_GPIO/blob/master/Adafruit_GPIO/I2C.py
 *
 *  TODO:
 *  - timeout: to be investigated how to handle timeouts
 *  - more than 1 dsp: does it work?
 *  - dsp programming: to be tested
 *
 *  Created on: May 31, 2017
 *      Author: alexander
 */

#ifndef ADI_DSP_PROGRAMMER_I2C_H
#define ADI_DSP_PROGRAMMER_I2C_H
#include <stdint.h>

/*
 int i2cOpen(void)

 * Init the dsp driver.
 * After the init it is possible to read and write to the dsp device using its id.
 * For ADAU there are four dspId addresses, 0x00..0x03, see below
 */
extern int i2cOpen();

/*
 int i2cClose(void)

 * Release the i2c file descriptor
 */
extern int i2cClose();

/* Read a byte from a 8-byte register, i.e. cannot be used for DSP communication.
 * Needed for synth com
 * param i2c_addr,   i2c address for the device, synth might have 0x77
 * param reg_addr,   the register that we want to read, right now only 1-byte registers
 * param *buf,       pointer to buffer that will get the data (byte) in the register
 * */
int read_i2c_byte(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *buf);

/*
 read_i2c_block_data(unsigned char addr, unsigned short reg, unsigned char *data, unsigned short data_size)

 * Read data from dsp.
 *
 * param addr, the dsp addr in 7 bit notation. 0x74 -> 0x3A
 *
 * param reg, the register address we want to read from, always 2 bytes wide
 *
 * param data, buffer for the content in the register address
 *
 * param data_size, number of chars that data buffer should be able to handle
 *
 * return 0 upon success
 * */
extern int read_i2c_block_data(
        unsigned char addr,
        unsigned short reg,
        unsigned char *data,
        unsigned short data_size);

/*
 write_i2c_block_data(unsigned char addr, unsigned short reg, unsigned char *data, unsigned short data_size)

 * Write data to dsp.
 *
 * param addr, the dsp addr in 7 bit notation. 0x74 -> 0x3A
 *
 * param reg, the register address we want to write, always 2 bytes wide
 *
 * param data, buffer with content
 *
 * param data_size, number of chars in the data buffer
 *
 * return 0 upon success
 * */
extern int write_i2c_block_data(
        unsigned char addr,
        unsigned short reg,
        const unsigned char *data,
        unsigned short data_size);

extern int write_i2c_block_data_raw(
        unsigned char addr,
        unsigned char *buf,
        unsigned short val_len);

#endif //ADI_DSP_PROGRAMMER_I2C_H
