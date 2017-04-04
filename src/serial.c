/*****************************************************************************
 * serial.c - methods to configure serial sensors                            *
 *                                                                           *
 * Author: Nathanael A. Smith - nasmith@olivet.edu                           *
 *                                                                           *
 * This file is part of the Baja DAQ System for Olivet Nazarene University   *
 * Copyright 2017 Nathanael A. Smith & Ryan Carl                             *
 * License: MIT License (see LICENSE for more details)                       *
 *                                                                           *
 * This file contains code copied from the Serial Programming Guide for      *
 * Posix Operating Systems (https://www.cmrr.umn.edu/~strupp/serial.html) by *
 * John Paul Strupp. Thanks to him and the Center for Magnetic Resonance     *
 * Research, University of Minnesota Medical School.                         *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>

#include "serial.h"

/* this method configures the UART and SPI interfaces for GPS
     and the accelerometer modules 
    takes file descriptor to be used as argument
    returns 0 on success, non-zero on error (see serial.h) */
int initSerial(int *gpsfd) {
    /* initialize accelerometer SPI */
    /* initialize wiringpi library to use SPI interface */

    /* set FIFO mode to bypass */
    /* set FIFO_CTRL_REG (2Eh) to 0 ? */
    /* set FS bit to 11 */
    /* set CTRL_REG1[3] (LPen bit) to 0 */
    /* set CTRL_REG4[3] (HR bit) to 1 */
    /* set CTRL_REG1[0-3] = 0010 */
    return 0;
}

/* reads values from 3 axes of accelerometer via SPI */
/* struct should already be initialized */
void readAccel(struct accelAxes *vals) {
    vals->x = readAccelX();
    vals->y = readAccelY();
    vals->z = readAccelZ();
}

/* reads value of x axis of accelerometer via SPI */
int readAccelX() {
    char buffer[100];
    strcpy(buffer, ""); /* TODO: put characters in buffer */;
    rwSPI(buffer);
    char upper = 0;
    strcpy(buffer, ""); /* TODO: put characters in buffer */;
    rwSPI(buffer);
    char lower = 0;

    return (lower || upper<<8);;
}

/* reads value of y axis of accelerometer via SPI */
int readAccelY() {
    char buffer[100];
    strcpy(buffer, ""); /* TODO: put characters in buffer */;
    rwSPI(buffer);
    char upper = 0;
    strcpy(buffer, ""); /* TODO: put characters in buffer */;
    rwSPI(buffer);
    char lower = 0;

    return (lower || upper<<8);;
}

/* reads value of z axis of accelerometer via SPI */
int readAccelZ() {
    char buffer[100];
    strcpy(buffer, ""); /* TODO: put characters in buffer */;
    rwSPI(buffer);
    char upper = 0;
    strcpy(buffer, ""); /* TODO: put characters in buffer */;
    rwSPI(buffer);
    char lower = 0;

    return (lower || upper<<8);;
}

/* writes from buffer to SPI and reads response into same buffer */
void rwSPI(char *buffer) {
    /* TODO: write I/O stuff for SPI */
}

/* cleans up serial interface */
int closeSerial(int fd) {
    return close(fd);
}
