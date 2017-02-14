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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

/* this method configures the UART and SPI interfaces for GPS
     and the accelerometer modules */
int initSerial(int writeFD, int readFD) {
    /* initialize GPS UART */
    
    /* initialize accelerometer SPI */
    return 0;
}
