SKIN=alchemy
MAIN_SRC=driver
TARGET=irb6-driver

LM=-lm -lwiringPi

CFLAGS := $(shell xeno-config --skin=alchemy --cflags)
LDFLAGS := $(LM) $(shell xeno-config --skin=alchemy --ldflags)
CC := $(shell xeno-config --cc)

$(TARGET): $(MAIN_SRC).c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
