CC = cc

ROOTPATH=../..
INCLUDE = -I./applications -I$(ROOTPATH)/inc
LIB=-lpthread

OBJ += $(patsubst %.c, %.o, $(wildcard applications/*.c))
OBJ += $(patsubst %.c, %.o, $(wildcard $(ROOTPATH)/src/*.c))
OBJ += $(patsubst %.c, %.o, $(wildcard $(ROOTPATH)/samples/*.c))

CFLAGS = -O0 -g3 -Wall
target = FlashDBLinuxDemo

all:$(OBJ)
	$(CC) out/*.o -o $(target) $(LIB)
	mv $(target) out
%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)
	mv $@ out
clean:
	rm -rf out/*
