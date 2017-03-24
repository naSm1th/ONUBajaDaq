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
#define SERIALPORT "/dev/ttyS0i"

struct accelAxes {
    int x;
    int y;
    int z;
}
