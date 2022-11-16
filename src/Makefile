PRO_NAME = librtsp.a
PRO_ROOT = $(shell pwd)
CXX = $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar
INC := -I$(PRO_ROOT)/..
CFLAGS += $(INC)

LIB = -lpthread -lrt

SRC_DIR := $(PRO_ROOT)
LIB_SRC = \
	$(SRC_DIR)/rtsp_parse.cpp 		\
	$(SRC_DIR)/rtsp_handler.cpp 		\
	$(SRC_DIR)/rtp.cpp				\
	$(SRC_DIR)/h264.cpp				\
	$(SRC_DIR)/tcp.cpp				\
	$(SRC_DIR)/udp.cpp				\

obj = $(LIB_SRC:%.cpp=%.o)


all:$(obj)
	$(AR)  -rcs  $(PRO_NAME) $(obj)
clean:
	@rm $(obj)  $(PRO_NAME) -rf
