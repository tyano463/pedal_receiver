bin_PROGRAMS = pedal_receiver

pedal_receiver_SOURCES = src/main.c \
	src/v_log.c \
	src/v_ipc.c

C_INC =

PKGCONF_LIBS = glib-2.0 gio-2.0 libxml-2.0

AM_CFLAGS = $(C_INC) -g -O0 `pkg-config --cflags $(PKGCONF_LIBS)`
AM_LDFLAGS = `pkg-config --libs $(PKGCONF_LIBS)`

SOURCES = $(pedal_receiver_SOURCES )
am_pedal_receiver_OBJECTS = $(pedal_receiver_SOURCES:.c=.o)

