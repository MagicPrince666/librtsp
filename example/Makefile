PRO_NAME = demo
PRO_ROOT = $(shell pwd)/..
CXX = $(CROSS_COMPILE)g++
INC := -I$(PRO_ROOT)
CFLAGS += $(INC)

LIB = -L$(SRC_DIR) -lpthread -lrtsp

SRC_DIR := $(PRO_ROOT)/src

SRC = \
	example.cpp

all:
	 $(CXX) $(CFLAGS)  -o $(PRO_NAME) $(SRC) $(LIB_SRC) $(LIB)

clean:
	@rm demo -rf
