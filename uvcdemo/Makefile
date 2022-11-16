PRO_NAME = demo
PRO_ROOT = $(shell pwd)/..
CC = $(CROSS_COMPILE)gcc
INC := -I$(PRO_ROOT)
CFLAGS += $(INC)
LIB = -L$(SRC_DIR) -lpthread -lrtsp
SRC_DIR := $(PRO_ROOT)/src

SRC = \
	example.c

all:
	 $(CC) $(CFLAGS)  -o $(PRO_NAME) $(SRC) $(LIB_SRC) $(LIB)

clean:
	@rm demo -rf
