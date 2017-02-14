# Makefile for Baja DAQ logger program by Ryan Carl, Nathanael Smith

make: logger.o mci.o
	gcc -pthread -o daq logger.o mci.o

debug:
	gcc -g -c logger.c
	gcc -g -c mci.c

logger.o: logger.c
	gcc -c logger.c

mci.o: mci.c
	gcc -c mci.c

clean:
	rm -r 1* *.o daq 

wrapup:
	tar -cvf daq.tar *.c *.h daq
