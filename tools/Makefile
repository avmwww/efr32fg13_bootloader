#
# Makefile for bootloader console tool project
#

CC=$(CROSS_COMPILE)gcc

TARGET = btlctl

SRCDIR = src
OBJDIR = obj
INCDIR = include

LIBRARY ?= ../../../library

ifeq ($(findstring mingw,$(CROSS_COMPILE)),)
	TARGET_OS := Linux
else
	TARGET_OS := MINGW
endif

PROGOPT=$(LIBRARY)/progopt
UTILS=$(LIBRARY)/utils
ifeq ($(TARGET_OS),MINGW)
SERIAL=$(LIBRARY)/serial/windows
else
SERIAL=$(LIBRARY)/serial/posix
endif

PROGOPT_SRCS = $(PROGOPT)/progopt.c
UTILS_SRS = $(UTILS)/failure.c
SERIAL_SRC = $(SERIAL)/serial.c

SRCMISC = $(PROGOPT_SRCS) $(UTILS_SRS) $(SERIAL_SRC)

SRCS_BTL = main.c \
	   btlproto.c \
	   zalloc.c \
	   dump_hex.c \
	   serial.c \

SRCS_BTL += $(SRCMISC)

OBJS = $(addprefix $(OBJDIR)/, $(notdir $(SRCS_BTL:.c=.o)))

CFLAGS ?= $(C_FLAGS)
CFLAGS += -I$(INCDIR) -I$(PROGOPT) -I$(UTILS) -I$(SERIAL) -I../include -I.
CFLAGS += -Wall
CFLAGS += -D_GNU_SOURCE -g

LDFLAGS +=

all: $(OBJDIR) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	rm -rf $(TARGET) $(OBJDIR)

$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

vpath %.c $(SRCDIR)
vpath %.h $(INCDIR)

vpath %.o $(OBJDIR)

vpath %.c $(PROGOPT) $(UTILS) $(SERIAL)
