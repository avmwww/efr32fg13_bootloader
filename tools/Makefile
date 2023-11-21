#
# Makefile for bootloader update project
#

CC=$(CROSS_COMPILE)gcc

TARGET = btlctl

SRCDIR = src
OBJDIR = obj
INCDIR = include

LIBRARY=../../../library

PROGOPT=$(LIBRARY)/progopt
UTILS=$(LIBRARY)/utils

SRCMISC = $(PROGOPT_SRCS) $(UTILS_SRS)

PROGOPT_SRCS = $(PROGOPT)/progopt.c
UTILS_SRS = $(UTILS)/failure.c

SRCS_BTL = main.c \
	       btlproto.c \
	       zalloc.c \
	       dump_hex.c \

ifeq ($(findstring mingw,$(CROSS_COMPILE)),)
SRCS_BTL += serial.c
else
SRCS_BTL += serialport.c
endif

SRCS_BTL += $(SRCMISC)

OBJS = $(addprefix $(OBJDIR)/, $(notdir $(SRCS_BTL:.c=.o)))

CFLAGS ?= $(C_FLAGS)
CFLAGS += -I$(INCDIR) -I$(PROGOPT) -I$(UTILS) -I../include -I.
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

vpath %.c $(PROGOPT) $(UTILS)
