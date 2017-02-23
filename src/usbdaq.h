/*****************************************************************************
 * usbdaq.h - methods to interface with the MCC USB-201 interface            *
 *                                                                           *
 * Author: Nathanael A. Smith - nasmith@olivet.edu                           *
 *                                                                           *
 * This file is part of the Baja DAQ System for Olivet Nazarene University   *
 * Copyright 2017 Nathanael A. Smith & Ryan Carl                             *
 * License: MIT License (see LICENSE for more details)                       *
 *****************************************************************************/

/* initializes and runs the USB DAQ process */
void *initUSBDaq(void *in);
/* exit handler to allow for proper termination of device driver */
void exitHandler(int sig);
/* constantly reads DIO/updates counts */
void scanDIO(int *counts, int *run);

/* struct for parameters to initUSBDaq() */
struct usbParams {
    int *counts;
    int *run;
};
