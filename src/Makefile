# Makefile for Baja DAQ logger program by Ryan Carl, Nathanael Smith

make: logger.o serial.o mci.o
	gcc -pthread -o daq logger.o serial.o mci.o

debug:
	gcc -g -c logger.c
	gcc -g -c serial.c
	gcc -g -c mci.c

logger.o: logger.c
	gcc -c logger.c

serial.o: serial.c
	gcc -c serial.c

mci.o: mci.c
	gcc -c mci.c

clean:
	rm *.o daq *.csv

wrapup:
	tar -cvf daq.tar *.c *.h daq
