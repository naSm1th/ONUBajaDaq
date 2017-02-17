/*****************************************************************************
 * serial.c - methods to configure serial sensors                            *
 *                                                                           *
 * Author: Nathanael A. Smith - nasmith@olivet.edu                           *
 *                                                                           *
 * This file is part of the Baja DAQ System for Olivet Nazarene University   *
 * Copyright 2017 Nathanael A. Smith & Ryan Carl                             *
 * License: MIT License (see LICENSE for more details)                       *
 *****************************************************************************/


/* this method is run in the new process after forking */
int initSerial(int writeFD, int readFD) {
    /* initialize all sensors */

    /* wait for data from GPS */

    /* received data from GPS */
    /* process all data, send to parent process */
    return 0;
}
