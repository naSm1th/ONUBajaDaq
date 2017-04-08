/*****************************************************************************
 * serial.h - methods for serial sensors                                     *
 *                                                                           *
 * By Nathanael A. Smith - nasmith@olivet.edu                                *
 * Copyright 2017 Olivet Nazarene University                                 *
 *                                                                           *
 * This file is part of the Baja DAQ System for Olivet Nazarene University   *
 * License: MIT License (see LICENSE in project root)                        *
 *****************************************************************************/

/* the serial port to use for GPS UART communication */
#define SERIALPORT "/dev/serial0"

struct accelAxes {
    int x;
    int y;
    int z;
};

int initSerial();
void readAccel(struct accelAxes *vals);
int readAccelX();
int readAccelY();
int readAccelZ();
void rwSPI(char *buffer);
int closeSerial(int fd);
