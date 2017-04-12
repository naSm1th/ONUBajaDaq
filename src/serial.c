/*****************************************************************************
 * serial.c - methods to configure serial sensors (currently accelerometer)  *
 *                                                                           *
 * Author: Nathanael A. Smith - nasmith@olivet.edu                           *
 *                                                                           *
 * This file is part of the Baja DAQ System for Olivet Nazarene University   *
 * Copyright 2017 Nathanael A. Smith & Ryan Carl                             *
 * License: MIT License (see LICENSE for more details)                       *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wiringPiSPI.h>

#include "serial.h"

/* the file descriptor for the accelerometer */
int fd;

/* buffer for all commands/data */
unsigned char buffer[100];

/* this method configures the SPI interface for the accelerometer module
    returns 0 on success, non-zero on error (see serial.h) */
int initSerial() {
    /* return value */
    int ret;

    /* initialize the SPI interface */
    fd = wiringPiSPISetup(CHANNEL, SPEED);
    
    if (fd < 0) {
        printf("Error: failed to open channel!!!\n\n");
        return -1;
    }

    //printf("fd = %d\n", fd);
    
    /* enable axes, set data rate to 400 Hz */
    memset(buffer, 0, 100);
    buffer[0] = 0x20 | 0b00000000;
    buffer[1] = 0b01110111;
    ret = wiringPiSPIDataRW(CHANNEL, buffer, 2);
    
    if (ret < 0) {
        printf("Error: failed to read/write!!!\n\n");
        return -1;
    }


    /* set full-scale measurement to +/- 4G */
    memset(buffer, 0, 100);
    buffer[0] = 0x23 | 0b00000000;
    buffer[1] = 0b00010000;
    ret = wiringPiSPIDataRW(CHANNEL, buffer, 2);
    
    if (ret < 0) {
        printf("Error: failed to read/write!!!\n\n");
        return -1;
    }

    return 0;
}

/* reads values from 3 axes of accelerometer via SPI */
/* struct should already be initialized */
int readAccel(struct accelAxes *vals) {
    /* read from all 6 registers */
    memset(buffer, 0, 100);
    buffer[0] = 0x28 | 0b11000000;
    int ret = wiringPiSPIDataRW(CHANNEL, buffer, 7);
    
    if (ret < 0) {
        printf("Error: failed to read/write!!!\n\n");
        return -1;
    }
    
    /* store data in struct */
    vals->x = ((float)((short)(buffer[1] | (buffer[2]<<8))))/8190*CONST_GRAVITY;
    vals->y = ((float)((short)(buffer[3] | (buffer[4]<<8))))/8190*CONST_GRAVITY;
    vals->z = ((float)((short)(buffer[5] | (buffer[6]<<8))))/8190*CONST_GRAVITY;

    return 0;
}
