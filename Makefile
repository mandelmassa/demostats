OBJS	+= demostats.o

OS	 = $(shell uname -o)

LIBS	+= -L../libdemo
LIBS	+= -ldemo

CFLAGS	+= -I../libdemo/inc
CFLAGS	+= -Wall

ifeq ($(OS),Cygwin)
CC	 = i686-pc-mingw32-gcc
LD	 = i686-pc-mingw32-gcc
EXE	 = .exe
endif

all: demostats$(EXE)

demostats$(EXE): $(OBJS)
	$(LD) $(OBJS) $(LIBS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm demostats$(EXE) $(OBJS)
