P_NAME		:= p2pClient
P_SRCS		:= main.c $(wildcard ../*.c)
P_INCLUDE	:= ../
CC 			:= gcc
CFLAGS		:= $(addprefix -I,$(P_INCLUDE))
LDFLAGS 	:= -O2 -lpthread

.PHONY: all clean
all: $(P_NAME)

clean:
	rm -f $(P_NAME)

$(P_NAME): $(P_SRCS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) 
