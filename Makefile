# Uncomment for manual build
BUILD_MODE = run
LWP = -lwiringPi


PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJS = spi.o gpio.o time.o NRF24.o rf.o core.o utils.o gate_in.o so_receiver.o malloc.o threads.o lbq.o rings.o logger.o rf_receiver.o rf_transmitter.o map2.o list.o wsd_packet.o so_transmitter.o gate_out.o exb_packet.o

ifeq ($(BUILD_MODE),debug)
	CFLAGS += -g
else ifeq ($(BUILD_MODE),run)
	CFLAGS += -O2
else
	$(error Build mode $(BUILD_MODE) not supported by this Makefile)
endif

all:	core
	@echo $(BUILD_MODE)

core:	$(OBJS)
	$(CC) -Wall -o $@ $^ -pthread $(LWP) -lm

%.o:	$(PROJECT_ROOT)%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

rings.o: 
	$(CC) -c $(CFLAGS) ${PROJECT_ROOT}libs/collections/src/rings.c
	
lbq.o: 
	$(CC) -c $(CFLAGS) ${PROJECT_ROOT}libs/collections/src/lbq.c

map2.o: 
		$(CC) -c $(CFLAGS) ${PROJECT_ROOT}libs/collections/src/map2.c

list.o: 
		$(CC) -c $(CFLAGS) ${PROJECT_ROOT}libs/collections/src/list.c
	
threads.o: 
	$(CC) -c $(CFLAGS) ${PROJECT_ROOT}libs/oscl/src/threads.c
	
malloc.o: 
	$(CC) -c $(CFLAGS) ${PROJECT_ROOT}libs/oscl/src/malloc.c

clean:
	rm -fr core $(OBJS)
