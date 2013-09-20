PREFIX=/usr/local/
ESWEEP_SRC=../../src

GCC=gcc
CFLAGS=-O2 -Wall -I$(ESWEEP_SRC) -DOPENBSD -DHAVE_UNISTD_H -DESWEEP_ERROR_NOEXIT -msse -mfpmath=sse -fpic -DALTFFT
LFLAGS=-L/usr/local/lib -lm 

INSTALL_PATH=$(PREFIX)/lib

all: clean inout

inout: 	
	$(GCC) $(LFLAGS) $(CFLAGS) -o inout \
						$(ESWEEP_SRC)/esweep_base.c \
						$(ESWEEP_SRC)/esweep_priv.c \
						$(ESWEEP_SRC)/esweep_conv.c \
						$(ESWEEP_SRC)/esweep_mem.c \
						$(ESWEEP_SRC)/esweep_dsp.c \
						$(ESWEEP_SRC)/esweep_sound.c \
						$(ESWEEP_SRC)/sound_openbsd.c \
						$(ESWEEP_SRC)/audio_file.c \
						$(ESWEEP_SRC)/fft.c \
						$(ESWEEP_SRC)/dsp.c \
						inout.c

clean:
	rm -f inout
