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
    /* initialize GPS UART */
    *gpsfd = open(SERIALPORT, O_RDWR | O_NOCTTY | O_NDELAY);

    if (*gpsfd == -1) {
        /* error - cannot open port */
        return -1;
    }
    
    /* get current serial port parameters */
    struct termios options;
    tcgetattr(*gpsfd, &options);
    
    /* set baud rate */
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    /* set other necessary parameters */
    options.c_cflag |= (CLOCAL | CREAD);

    /* apply new parameters */
    tcsetattr(*gpsfd, TCSANOW, &options);

    char buffer[6];
    int valid = 0;
    /* test settings */
    /* give it 5 attempts */
    int i, len;
    for (i = 0; i < 5; i ++) {
        ioctl(*gpsfd, TCIOFLUSH, 2);
        memset(buffer, 0, 6);
        len = 0;
        /* attempt to read in string */
        len = read(*gpsfd, buffer, 6);
        if (len != 6) {
            continue;
        }
        if (!strcmp(buffer, "$GPRMC")) {
            valid = 1;
            break;
        }
        if (!strcmp(buffer, "$GPGGA")) {
            valid = 1;
            break;
        }
    }

    /* check if we are still at 9600 baud */
    /* if so, change baud rate of device and serial port */
    /* also change update rate of device */
    if (valid == 1) {
        /* change baud rate of device */
        if (write(*gpsfd, "$PMTK251,115200*1F\r\n", 20)) {
            /* error writing */
        }

        /* change baud rate of serial port */
        /* get current settings */
        tcgetattr(*gpsfd, &options);
    
        /* set baud rate */
        cfsetispeed(&options, B115200);
        cfsetospeed(&options, B115200);
        
        /* save config */
        tcsetattr(*gpsfd, TCSANOW, &options);
    }

    valid = 0;

    /* verify we are at 115200 baud rate */
    /* give it 5 attempts */
    for (i = 0; i < 5; i ++) {
        ioctl(*gpsfd, TCIOFLUSH, 2);
        memset(buffer, 0, 6);
        len = 0;
        /* attempt to read in string */
        len = read(*gpsfd, buffer, 6);
        if (len != 6) {
            continue;
        }
        if (!strcmp(buffer, "$GPRMC")) {
            valid = 1;
            break;
        }
        if (!strcmp(buffer, "$GPGGA")) {
            valid = 1;
            break;
        }
    }

    /* check to see if we still can't read data */
    if (valid != 0) {
        return -1;
    }

    /* change update rate of device */
    if (write(*gpsfd, "$PMTK220,100*2F\r\n", 17)) {
        /* error writing */
    }


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
    char upper = rwSPI(buffer);
    strcpy(buffer, ""); /* TODO: put characters in buffer */;
    char lower = rwSPI(buffer);

    return (lower || upper<<8);;
}

/* reads value of y axis of accelerometer via SPI */
int readAccelY() {
    char buffer[100];
    strcpy(buffer, ""); /* TODO: put characters in buffer */;
    char upper = rwSPI(buffer);
    strcpy(buffer, ""); /* TODO: put characters in buffer */;
    char lower = rwSPI(buffer);

    return (lower || upper<<8);;
}

/* reads value of z axis of accelerometer via SPI */
int readAccelZ() {
    char buffer[100];
    strcpy(buffer, ""); /* TODO: put characters in buffer */;
    char upper = rwSPI(buffer);
    strcpy(buffer, ""); /* TODO: put characters in buffer */;
    char lower = rwSPI(buffer);

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
