/*
 * Copyright (c) 2007 Jochen Fabricius <jfab@berlios.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DSP_H
#define DSP_H

#include <stdlib.h>
#include <math.h>

#include "fft.h" 
#include "dsp.h"

/* for both functions the kernel must be pre-transformed */

void dsp_convolve(Complex *signal, Complex *kernel, Complex *table, int size) {
	int i;
	Real t_real, t_imag; /* temp variables */
	
	/* in-place forward transform */
	
	fft(signal, table, size, FFT_FORWARD);
	
	/* complex multiplication */
	
	for (i=0;i<size;i++) {
		t_real=signal[i].real/size; /* correct for fft scaling */
		t_imag=signal[i].imag/size;
		
		signal[i].real=t_real*kernel[i].real-t_imag*kernel[i].imag;
		signal[i].imag=t_real*kernel[i].imag+t_imag*kernel[i].real;
	}
	
	/* in-place backward transform */
	
	fft(signal, table, size, FFT_BACKWARD);
}

void dsp_deconvolve(Complex *signal, Complex *kernel, Complex *table, int size) { 
	int i;
	Real t_real, t_imag; /* temp variables */
	Real t_abs;
	
	/* in-place forward transform */
	
	fft(signal, table, size, FFT_FORWARD);
	
	/* complex division */
	
	for (i=0;i<size;i++) {
		t_real=signal[i].real;
		t_imag=signal[i].imag;
		
		t_abs=kernel[i].real*kernel[i].real+kernel[i].imag*kernel[i].imag;
		t_abs*=size; /* correct for fft scaling */
		
		signal[i].real=(t_real*kernel[i].real+t_imag*kernel[i].imag)/t_abs;
		signal[i].imag=(t_imag*kernel[i].real-t_real*kernel[i].imag)/t_abs;
	}
	
	/* in-place backward transform */
	
	fft(signal, table, size, FFT_BACKWARD);
}

void dsp_hilbert(Complex *signal, Complex *table, int size) {
	int i, herm_size=size/2;
	Real tmp;
	
	/* in-place forward transform */
	
	fft(signal, table, size, FFT_FORWARD);
	
	for (i=1;i<herm_size;i++) {
		/* positive frequencies get shifted with -90° */
		tmp=-signal[i].real;
		signal[i].real=signal[i].imag/size;
		signal[i].imag=tmp/size;
		/* negative frequencies get shifted with 90° */
		tmp=signal[size-i].real;
		signal[size-i].real=-signal[size-i].imag/size;
		signal[size-i].imag=tmp/size;
	}
	
	/* DC is zero */
	
	signal[0].real=signal[0].imag;
	
	/* but what is with Nyquist? Seems that we don't need to touch it */
	//signal[size/2].real=signal[size/2].imag=0.0;
	
	/* in-place backward transform */
	
	fft(signal, table, size, FFT_BACKWARD);
}

void dsp_unwrapPhase(Polar *polar, int size) {
	int i, k;
	Real tmp;
	tmp=polar[0].arg;
	for (i=1, k=0;i<size;i++) {
		if ((polar[i].arg-tmp)>M_PI) k--;
		if ((polar[i].arg-tmp)<-M_PI) k++;
		tmp=polar[i].arg;
		polar[i].arg=polar[i].arg+k*2*M_PI;
	}
}

void dsp_wrapPhase(Polar *polar, int size) {
	int i, k;
	
	for (i=1, k=0;i<size;i++) {
		if ((polar[i].arg+k*2*M_PI)>M_PI) k--;
		if ((polar[i].arg+k*2*M_PI)<-M_PI) k++;

		polar[i].arg=polar[i].arg+k*2*M_PI;
	}
}

#endif /* DSP_H */
