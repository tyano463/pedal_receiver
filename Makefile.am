bin_PROGRAMS = pedal_receiver

pedal_receiver_SOURCES = src/main.c \
	src/v_log.c

C_INC =

AM_CFLAGS = $(C_INC) -g -O0 `pkg-config --cflags glib-2.0 gio-2.0`
AM_LDFLAGS = `pkg-config --libs glib-2.0 gio-2.0`

SOURCES = $(pedal_receiver_SOURCES )
am_pedal_receiver_OBJECTS = $(pedal_receiver_SOURCES:.c=.o)

