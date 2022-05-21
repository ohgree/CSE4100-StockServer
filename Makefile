CC = gcc
CFLAGS=-O2 -Wall
LDLIBS = -lpthread

all: multiclient stockclient stockserver

debug: CFLAGS += -DDEBUG
debug: all tests

tests: CFLAGS += -DDEBUG
tests: test_stock

multiclient: multiclient.c csapp.c
stockclient: stockclient.c csapp.c
stockserver: stockserver.c echo.c csapp.c misc.c stock.c command.c

test_stock: test_stock.c csapp.c stock.c 

clean:
	rm -rf *~ multiclient stockclient stockserver test_stock *.o
