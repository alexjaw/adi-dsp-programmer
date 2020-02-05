//
// Created by alexander on 2020-01-03.
//

#ifndef ADI_DSP_PROGRAMMER_SIGMASTUDIOFW_H
#define ADI_DSP_PROGRAMMER_SIGMASTUDIOFW_H

/*
 * TODO: Update for your system's data type
 */
typedef unsigned short ADI_DATA_U16;
typedef unsigned char  ADI_REG_TYPE;

/*
 * Parameter data format
 */
#define SIGMASTUDIOTYPE_FIXPOINT 	0
#define SIGMASTUDIOTYPE_INTEGER 	1

/*
 * Write to a single Device register
 */
#define SIGMA_WRITE_REGISTER( devAddress, address, dataLength, data ) {/*TODO: implement macro or define as function*/}

/*
 * TODO: CUSTOM MACRO IMPLEMENTATION
 * Write to multiple Device registers
 */
//#define SIGMA_WRITE_REGISTER_BLOCK( devAddress, address, length, pData ) {/*TODO: implement macro or define as function*/}
void SIGMA_WRITE_REGISTER_BLOCK( int devAddress, int address, int length, ADI_REG_TYPE *pData );

/*
 * TODO: CUSTOM MACRO IMPLEMENTATION
 * Writes delay (in ms)
 */
//#define SIGMA_WRITE_DELAY( devAddress, length, pData ) {/*TODO: implement macro or define as function*/}
void SIGMA_WRITE_DELAY( int devAddress, int length, ADI_REG_TYPE *pData );

/*
 * Read device registers
 */
//#define SIGMA_READ_REGISTER( devAddress, address, length, pData ) {/*TODO: implement macro or define as function*/}
void SIGMA_READ_REGISTER( int devAddress, int address, int length, ADI_REG_TYPE *pData );

/*
 * Set a register field's value
 */
#define SIGMA_SET_REGSITER_FIELD( regVal, fieldVal, fieldMask, fieldShift )  \
		{ (regVal) = (((regVal) & (~(fieldMask))) | (((fieldVal) << (fieldShift)) && (fieldMask))) }

/*
 * Get the value of a register field
 */
#define SIGMA_GET_REGSITER_FIELD( regVal, fieldMask, fieldShift )  \
		{ ((regVal) & (fieldMask)) >> (fieldShift) }

/*
 * Convert a floating-point value to SigmaDSP (5.23) fixed point format
 *    This optional macro is intended for systems having special implementation
 *    requirements (for example: limited memory size or endianness)
 */
#define SIGMASTUDIOTYPE_FIXPOINT_CONVERT( _value ) {/*TODO: IMPLEMENT MACRO*/}

/*
 * Convert integer data to system compatible format
 *    This optional macro is intended for systems having special implementation
 *    requirements (for example: limited memory size or endianness)
 */
#define SIGMASTUDIOTYPE_INTEGER_CONVERT( _value ) {/*TODO: IMPLEMENT MACRO*/}

#endif //ADI_DSP_PROGRAMMER_SIGMASTUDIOFW_H
