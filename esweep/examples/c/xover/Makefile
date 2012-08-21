PREFIX=/usr/local/

GCC=gcc
ESWEEP_SRC=../../src
CFLAGS=-O2 -Wall -I$(ESWEEP_SRC) -DOPENBSD -DHAVE_UNISTD_H -msse -mfpmath=sse -fpic
LFLAGS=-L/usr/local/lib -lm

INSTALL_PATH=$(PREFIX)/lib

all: clean xover

xover!	
	$(GCC) $(LFLAGS) $(CFLAGS) -DESWEEP_ERROR_NOEXIT -o xover \
						$(ESWEEP_SRC)/esweep_base.c \
						$(ESWEEP_SRC)/esweep_priv.c \
						$(ESWEEP_SRC)/esweep_conv.c \
						$(ESWEEP_SRC)/esweep_mem.c \
						$(ESWEEP_SRC)/esweep_generate.c \
						$(ESWEEP_SRC)/esweep_dsp.c \
						$(ESWEEP_SRC)/esweep_file.c \
						$(ESWEEP_SRC)/esweep_math.c \
						$(ESWEEP_SRC)/esweep_filter.c \
						$(ESWEEP_SRC)/fft.c \
						$(ESWEEP_SRC)/dsp.c \
						$(ESWEEP_SRC)/esweep_sound.c \
						$(ESWEEP_SRC)/sound_openbsd.c \
						xover.c

clean:
	rm -f xover
