
TCL=/usr/local

CC=gcc
CFLAGS=-Wall -I$(TCL)/include/tcl8.5 -DHAVE_UNISTD_H -msse -mfpmath=sse -DNDEBUG_PRINT -DOPENBSD -I./src/portaudio/include -DESWEEP_ERROR_NOEXIT -fpic
# The code breaks on windows with -O2, but -O3 works...
OPTS=-O2
LFLAGS=-L/usr/local/lib
LIBS=-ltcl85 -lm

CSRC  =  src/esweep_priv.c src/dsp.c src/esweep_base.c src/esweep_conv.c src/esweep_dsp.c src/esweep_file.c src/esweep_filter.c src/esweep_generate.c src/esweep_math.c src/esweep_mem.c src/audio_file.c src/esweep_audio.c src/audio_pa.c src/audio_openbsd.c src/fft.c src/wrapper/tcl/esweep_tcl_wrap.c src/wrapper/tcl/esweep_tcl_wrap_base.c src/wrapper/tcl/esweep_tcl_wrap_conv.c src/wrapper/tcl/esweep_tcl_wrap_disp.c src/wrapper/tcl/esweep_tcl_wrap_dsp.c src/wrapper/tcl/esweep_tcl_wrap_file.c src/wrapper/tcl/esweep_tcl_wrap_gen.c src/wrapper/tcl/esweep_tcl_wrap_math.c src/wrapper/tcl/esweep_tcl_wrap_mem.c src/wrapper/tcl/esweep_tcl_wrap_filter.c src/wrapper/tcl/esweep_tcl_wrap_audio.c 

OBJS = $(CSRC:.c=.o)
DLL = tcl/esweep.so


$(DLL): $(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LFLAGS) $(LIBS)

$(OBJS): $(CSRC)
	$(CC) $(CFLAGS) $(OPTS) -o $@ -c $<

clean:
	rm -f $(OBJS) $(DLL)

.PHONY: clean
