PREFIX=/usr/local/
ESWEEP_SRC=../../esweep/src
ESWEEP_INC=../../esweep/include

GCC=gcc
CFLAGS=-O2 -Wall -I$(ESWEEP_INC) -DOPENBSD -DUSESSE -DALTFFT -msse2 -mfpmath=sse -fpic
LFLAGS=-shared -lm 

INSTALL_PATH=$(PREFIX)/lib

all: clean fftbench

fftbench: 	
	$(GCC) -L$(PREFIX)/lib -lm $(CFLAGS) -o fftbench \
						$(ESWEEP_SRC)/esweep.c \
						$(ESWEEP_SRC)/fft.c \
						$(ESWEEP_SRC)/dsp.c \
						$(ESWEEP_SRC)/esweep_conv.c \
						$(ESWEEP_SRC)/esweep_math.c \
						$(ESWEEP_SRC)/esweep_mem.c \
						$(ESWEEP_SRC)/esweep_generate.c \
						$(ESWEEP_SRC)/esweep_file.c \
						$(ESWEEP_SRC)/esweep_sound.c \
						$(ESWEEP_SRC)/sound_openbsd.c \
						$(ESWEEP_SRC)/esweep_dsp.c \
						$(ESWEEP_SRC)/esweep_surface.c \
						fftbench.c

clean:
	rm -f fftbench
