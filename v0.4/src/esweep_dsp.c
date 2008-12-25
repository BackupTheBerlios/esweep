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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dsp.h"
#include "esweep.h"
#include "fft.h"

/* 
compute the Fast Fourier Transform of a
output type is COMPLEX
"table" may be used to speed up the transformation, mostly useful in real-time applications
or when many FFTs are to performed; otherwise set "NULL"

WAVE: automatically zero-padded to a power of two
COMPLEX/POLAR: simple FFT
AUDIO/SURFACE: not available
*/

long esweep_fft(esweep_data *a, esweep_data *table) {
	Wave *wave;
	Complex *complex;
	Polar *polar;
	
	long fft_size;
	Complex *fft_table;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			/* set fft size to power of two */
			fft_size=(long) (pow(2, ceil(log((*a).size)/log(2)))+0.5);
			/* zero-padding */
			complex=(Complex*) calloc(fft_size, sizeof(Complex));
			wave=(Wave*) (*a).data;
			r2c(complex, wave, (*a).size);
			free(wave);
			(*a).data=complex;
			break;
		case POLAR:
			fft_size=(*a).size;
			polar=(Polar*) (*a).data;
			complex=(Complex*) (*a).data;
			p2c(complex, polar, fft_size);
			break;
		case COMPLEX:
			fft_size=(*a).size;
			complex=(Complex*) (*a).data;
			break;
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	/* adjust size and type */
	(*a).size=fft_size;
	(*a).type=COMPLEX;
	
	/* create FFT table if not given */
	
	if ((table==NULL) || ((*table).size!=fft_size/2) || ((*table).data==NULL) || (*table).type!=COMPLEX) fft_table=fft_create_table(fft_size);
	else fft_table=(Complex*) (*table).data;
	
	/* do FFT */
	
	fft(complex, fft_table, fft_size, FFT_FORWARD);
	
	/* if we created the fft table inside this function free it */
	
	if ((table==NULL) || ((*table).size!=fft_size/2) || ((*table).data==NULL) || (*table).type!=COMPLEX) free(fft_table);
	
	return ERR_OK;
}

/* inverse FFT; see above */

long esweep_ifft(esweep_data *a, esweep_data *table) {
	Wave *wave;
	Complex *complex;
	Polar *polar;
	
	long fft_size;
	Complex *fft_table;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			/* set fft size to power of two */
			fft_size=(long) (pow(2, ceil(log((*a).size)/log(2)))+0.5);
			/* zero-padding */
			complex=(Complex*) calloc(fft_size, sizeof(Complex));
			wave=(Wave*) (*a).data;
			r2c(complex, wave, (*a).size);
			free(wave);
			(*a).data=complex;
			break;
		case POLAR:
			fft_size=(*a).size;
			polar=(Polar*) (*a).data;
			complex=(Complex*) (*a).data;
			p2c(complex, polar, fft_size);
			break;
		case COMPLEX:
			fft_size=(*a).size;
			complex=(Complex*) (*a).data;
			break;
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	/* adjust size and type */
	(*a).size=fft_size;
	(*a).type=COMPLEX;
	
	/* create FFT table if not given */
	
	if ((table==NULL) || ((*table).size!=fft_size/2) || ((*table).data==NULL) || (*table).type!=COMPLEX) fft_table=fft_create_table(fft_size);
	else fft_table=(Complex*) (*table).data;
	
	/* do IFFT */
	
	fft(complex, fft_table, fft_size, FFT_BACKWARD);
	
	/* if we created the fft table inside this function free it */
	
	if ((table==NULL) || ((*table).size!=fft_size/2) || ((*table).data==NULL) || (*table).type!=COMPLEX) free(fft_table);
	
	return ERR_OK;
}

/* 
creates an FFT table for FFT/IFFT/(De)Convolution 
output is always COMPLEX, size ist automatically adjusted to a power of 2
*/

long esweep_createFftTable(esweep_data *table, long size) {
	long fft_size;
	
	if (table==NULL) return ERR_EMPTY_CONTAINER;
	if ((*table).data!=NULL) free((*table).data);
	
	fft_size=(long) (pow(2, ceil(log(size)/log(2)))+0.5);
	
	(*table).data=(void*) fft_create_table(fft_size);
	(*table).size=fft_size/2;
	(*table).type=COMPLEX;
	
	return ERR_OK;
}

/*
compute a fast _linear_ convolution of a with filter (a*filter)
output type is COMPLEX
see esweep_fft for table 
both "a" and "filter" may be of types WAVE and COMPLEX
if "filter" is COMPLEX it is assumed as pre-transformed
this saves 1 FFT hence gives a speed-up
*/

long esweep_convolve(esweep_data *a, esweep_data *filter, esweep_data *table) {
	Complex *complex, *complex_filter, *fft_table;
	long fft_size;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	if ((filter==NULL) || ((*filter).data==NULL) || ((*filter).size<=0)) return ERR_EMPTY_CONTAINER;
	
	if ((*a).samplerate!=(*filter).samplerate) return ERR_DIFF_MAPPING;
	
	switch ((*filter).type) {
		case WAVE:
			fft_size=(long) (pow(2, ceil(log((*a).size+(*filter).size-1)/log(2)))+0.5); /* L>=M+N-1 */
			complex_filter=(Complex*) calloc(fft_size, sizeof(Complex));
			r2c(complex_filter, (Wave*) (*filter).data, (*filter).size);
			break;
		case COMPLEX: /* pre-transformed filter kernel */
			if ((*a).size>=(*filter).size) return ERR_DIFF_MAPPING; /* unpossible */
			else {
				fft_size=(*filter).size;
				complex_filter=(Complex*) (*filter).data;
			}
			break;
		case POLAR:
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	/* zero-padding signal */
	complex=(Complex*) calloc(fft_size, sizeof(Complex));
	
	switch ((*a).type) {
		case WAVE:
			r2c(complex, (Wave*) (*a).data, (*a).size);
			break;
		case COMPLEX:
			memcpy(complex, (Complex*) (*a).data, (*a).size*sizeof(Complex));
			break;
		case POLAR:
		case SURFACE:
		case AUDIO:
		default:
			free(complex);
			if ((*filter).type!=COMPLEX) free(complex_filter);
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	if ((table==NULL) || ((*table).size!=fft_size/2) || ((*table).data==NULL) || ((*table).type!=COMPLEX)) fft_table=fft_create_table(fft_size);
	else fft_table=(Complex*) (*table).data;
	
	/* pre-transform filter kernel */
	
	if ((*filter).type==WAVE) fft(complex_filter, fft_table, fft_size, FFT_FORWARD);
	
	dsp_convolve(complex, complex_filter, fft_table, fft_size);
	
	/* clean up */
	if ((table==NULL) || ((*table).size!=fft_size/2) || ((*table).data==NULL) || ((*table).type!=COMPLEX)) free(fft_table);
	free(complex_filter);
	
	/* make complex the new data for a */
	
	(*a).type=COMPLEX;
	(*a).size=fft_size;
	free((*a).data);
	(*a).data=(void*) complex;
	return ERR_OK;
}

/* 
fast _linear_ deconvolution
see esweep_convolve
*/

long esweep_deconvolve(esweep_data *a, esweep_data *filter, esweep_data *table) {
	Complex *complex, *complex_filter, *fft_table;
	long fft_size;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	if ((filter==NULL) || ((*filter).data==NULL) || ((*filter).size<=0)) return ERR_EMPTY_CONTAINER;
	
	if ((*a).samplerate!=(*filter).samplerate) return ERR_DIFF_MAPPING;
	
	fft_size=(long) (pow(2, ceil(log((*a).size+(*filter).size-1)/log(2)))+0.5); /* L>=M+N+1 */
	
	switch ((*filter).type) {
		case WAVE:
			fft_size=(long) (pow(2, ceil(log((*a).size+(*filter).size-1)/log(2)))+0.5); /* L>=M+N-1 */
			complex_filter=(Complex*) calloc(fft_size, sizeof(Complex));
			r2c(complex_filter, (Wave*) (*filter).data, (*filter).size);
			break;
		case COMPLEX: /* pre-transformed filter kernel */
			if ((*a).size>=(*filter).size) return ERR_DIFF_MAPPING; /* unpossible */
			else {
				fft_size=(*filter).size;
				complex_filter=(Complex*) (*filter).data;
			}
			break;
		case POLAR:
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	/* zero-padding signal */
	complex=(Complex*) calloc(fft_size, sizeof(Complex));
	
	switch ((*a).type) {
		case WAVE:
			r2c(complex, (Wave*) (*a).data, (*a).size);
			break;
		case COMPLEX:
			memcpy(complex, (Complex*) (*a).data, (*a).size*sizeof(Complex));
			break;
		case POLAR:
		case SURFACE:
		case AUDIO:
		default:
			free(complex);
			if ((*filter).type!=COMPLEX) free(complex_filter);
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	if ((table==NULL) || ((*table).size!=fft_size/2) || ((*table).data==NULL) || ((*table).type!=COMPLEX)) fft_table=fft_create_table(fft_size);
	else fft_table=(Complex*) (*table).data;
	
	/* pre-transform filter kernel */
	if ((*filter).type==WAVE) fft(complex_filter, fft_table, fft_size, FFT_FORWARD);
	
	dsp_deconvolve(complex, complex_filter, fft_table, fft_size);
	
	/* clean up */
	if ((table==NULL) || ((*table).size!=fft_size/2) || ((*table).data==NULL) || ((*table).type!=COMPLEX)) free(fft_table);
	free(complex_filter);
	
	/* make complex the new data for a */
	
	(*a).type=COMPLEX;
	(*a).size=fft_size;
	free((*a).data);
	(*a).data=(void*) complex;
	return ERR_OK;
}

/*
Hilbert transformation of "a"
WAVE: zero-padded to power of 2
COMPLEX: simple transform
AUDIO/POLAR/SURFACE: not available
*/

long esweep_hilbert(esweep_data *a, esweep_data *table) {
	Complex *complex, *fft_table;
	long fft_size;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			fft_size=(long) (pow(2, ceil(log((*a).size)/log(2)))+0.5);
			complex=(Complex*) calloc(fft_size, sizeof(Complex));
			r2c(complex, (Wave*) (*a).data, (*a).size);
			free((*a).data);
			(*a).size=fft_size;
			(*a).type=COMPLEX;
			(*a).data=complex;
			break;
		case COMPLEX:
			complex=(Complex*) (*a).data;
			fft_size=(*a).size;
			break;
		case POLAR:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	if ((table==NULL) || ((*table).size!=fft_size/2) || ((*table).data==NULL) || ((*table).type!=COMPLEX)) fft_table=fft_create_table((*a).size);
	else fft_table=(Complex*) (*table).data;
	
	dsp_hilbert(complex, fft_table, (*a).size);
	
	if ((table==NULL) || ((*table).size!=fft_size/2) || ((*table).data==NULL) || ((*table).type!=COMPLEX)) free(fft_table);
	
	return ERR_OK;
}

long esweep_analytic(esweep_data *a, esweep_data *table) {
	Complex *complex, *fft_table, *analytic;
	long fft_size, i;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			fft_size=(long) (pow(2, ceil(log((*a).size)/log(2)))+0.5);
			analytic=(Complex*) calloc(fft_size, sizeof(Complex));
			r2c(analytic, (Wave*) (*a).data, (*a).size);
			free((*a).data);
			(*a).size=fft_size;
			(*a).type=COMPLEX;
			(*a).data=analytic;
			break;
		case COMPLEX:
		case POLAR:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	if ((table==NULL) || ((*table).size!=fft_size/2) || ((*table).data==NULL) || ((*table).type!=COMPLEX)) fft_table=fft_create_table((*a).size);
	else fft_table=(Complex*) (*table).data;
	
	complex=(Complex*) calloc(fft_size, sizeof(Complex));
	memcpy(complex, analytic, fft_size);
	
	dsp_hilbert(complex, fft_table, fft_size);
	
	/* the real part of complex is the imaginary part of the analytic signal */
	for (i=0;i<fft_size;i++) {
		analytic[i].imag=complex[i].real;
	}
	free(complex);
	
	if ((table==NULL) || ((*table).size!=fft_size/2) || ((*table).data==NULL) || ((*table).type!=COMPLEX)) free(fft_table);
	
	return ERR_OK;
}

/* time integration implemented as an IIR-Filter

WAVE: simple accumulation
POLAR/COMPLEX: division by 1/(2*pi*f); DC is set to the value at df
AUDIO/SURFACE: not available

This integrator has a gain of 1/Fs
*/

long esweep_integrate(esweep_data *a) {
	Wave *wave;
	Complex *complex;
	Polar *polar;
	long i;
	long size;
	double dw;
	double temp=0.0;
	double pi_2=M_PI/2;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	dw=2*M_PI*(*a).samplerate/(*a).size; /* angular frequency */
	
	switch ((*a).type) {
		case WAVE:
			wave=(Wave*) (*a).data;
			wave[0]/=(*a).samplerate;
			for (i=1;i<(*a).size;i++) wave[i]=wave[i]/(*a).samplerate+wave[i-1];
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			size=(*a).size/2+1; /* hermitian redundancy */
			polar[0].abs/=dw;
			polar[0].arg+=pi_2;
			for (i=1;i<size;i++) {
				polar[i].abs/=(i*dw);
				temp=polar[i].arg+=pi_2; /* +90° */
				/* restore redundancy */
				polar[(*a).size-i].abs=polar[i].abs;
				polar[(*a).size-i].arg=-polar[i].arg;
			}
			polar[size-1].arg=temp;
			break;
		case COMPLEX:
			complex=(Complex*) (*a).data;
			size=(*a).size/2; /* hermitian redundancy */
			temp=complex[0].real/dw;
			complex[0].real=-complex[0].imag/dw;
			complex[0].imag=temp;
			temp=complex[size].real/dw;
			complex[size].real=-complex[size].imag/dw;
			complex[size].imag=temp;
			for (i=1;i<size;i++) {
				temp=complex[i].real/(i*dw);
				complex[i].real=-complex[i].imag/(i*dw);
				complex[i].imag=temp;
				complex[(*a).size-i].real=complex[i].real;
				complex[(*a).size-i].imag=-complex[i].imag;
			}
			break;
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	return ERR_OK;
}

/* time differentiation

WAVE: simple differentiator; first sample is extrapolated from the slope at the second sample
POLAR/COMPLEX: multiplication with 2*pi*f
AUDIO/SURFACE: not available

This difeerentiator has a gain of Fs
*/

long esweep_differentiate(esweep_data *a) { /* d/dt */
	Wave *wave;
	Complex *complex;
	Polar *polar;
	long i;
	long size;
	double dw;
	double temp=0.0;
	double pi_2=M_PI/2;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	dw=2*M_PI*(*a).samplerate/(*a).size; /* angular frequency */
	
	switch ((*a).type) {
		case WAVE:
			wave=(Wave*) (*a).data;
			for (i=(*a).size-1;i>0;i--) wave[i]=(*a).samplerate*(wave[i]-wave[i-1]);
			wave[0]*=(*a).samplerate;
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			size=(*a).size/2+1; /* hermitian redundancy */
			
			polar[0].abs=0;
			polar[0].arg=0;
			for (i=1;i<size;i++) {
				polar[i].abs*=i*dw;
				temp=polar[i].arg-=pi_2;
				/* restore redundancy */
				polar[(*a).size-i].abs=polar[i].abs;
				polar[(*a).size-i].arg=-polar[i].arg;
			}
			polar[size-1].arg=temp;
			break;
		case COMPLEX:
			complex=(Complex*) (*a).data;
			size=(*a).size/2; /* hermitian redundancy */
			complex[0].real=0;
			complex[0].imag=0;
			temp=-complex[size].real*size*dw;
			complex[size].real=complex[size].imag*size*dw;
			complex[size].imag=temp;
			for (i=1;i<size;i++) {
				temp=-complex[i].real*i*dw;
				complex[i].real=complex[i].imag*i*dw;
				complex[i].imag=temp;
				/* restore redundancy */
				complex[(*a).size-i].real=complex[i].real;
				complex[(*a).size-i].imag=-complex[i].imag;
			}
			break;
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	return ERR_OK;
}

/* 
wraps the argument of "a" so it is in the range [-pi:+pi]
only POLAR
*/

long esweep_wrapPhase(esweep_data *a) {
	Polar *polar;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case POLAR:
			polar=(Polar*) (*a).data;
			dsp_wrapPhase(polar, (*a).size);
			break;
		case COMPLEX:
		case WAVE:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	return ERR_OK;
}

/*
unwraps the phase of "a"
only POLAR
*/

long esweep_unwrapPhase(esweep_data *a) {
	Polar *polar;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case POLAR:
			polar=(Polar*) (*a).data;
			dsp_unwrapPhase(polar, (*a).size);
			break;
		case COMPLEX:
		case WAVE:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	return ERR_OK;
}

/* 
calculate the group delay of "a"
only POLAR
*/

long esweep_groupDelay(esweep_data *a) {
	Polar *polar;
	Wave *wave;
	Complex *complex, *fft_table, *wave_diff;
	long i, size;
	double dw;
	double tmp;
	double t_real, t_imag, t_abs;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case POLAR:
			polar=(Polar*) (*a).data;
			/* differentiate phase response */
			dw=2*M_PI*(*a).samplerate/(*a).size;
			/* the group delay at Fs */
			tmp=polar[0].arg-polar[(*a).size-1].arg;
			if (polar[(*a).size-1].arg>M_PI) tmp-=2*M_PI;
			else if (polar[(*a).size-1].arg<-M_PI) tmp+=2*M_PI;
			tmp/=-dw;
			/* unwrap phase */
			dsp_unwrapPhase(polar, (*a).size);
			for (i=0;i<(*a).size-1;i++) {
				polar[i].arg=-(polar[i+1].arg-polar[i].arg)/dw;
			}
			polar[(*a).size-1].arg=tmp;
			break;
		case WAVE:
			wave=(Wave*) (*a).data;
			size=(long) (pow(2, ceil(log((*a).size)/log(2)))+0.5);
			
			complex=(Complex*) calloc(size, sizeof(Complex));
			wave_diff=(Complex*) calloc(size, sizeof(Complex));
			
			r2c(complex, wave, (*a).size);
			for (i=0;i<(*a).size;i++) wave_diff[i].real=i*wave[i]/(*a).samplerate;
			
			fft_table=fft_create_table(size);
			fft(complex, fft_table, size, FFT_FORWARD);
			fft(wave_diff, fft_table, size, FFT_FORWARD);
			free(fft_table);
			
			/* complex division */
			for (i=0;i<size;i++) {
				t_real=wave_diff[i].real;
				t_imag=wave_diff[i].imag;
				
				t_abs=complex[i].real*complex[i].real+complex[i].imag*complex[i].imag;
				
				wave_diff[i].real=(t_real*complex[i].real+t_imag*complex[i].imag)/t_abs;
				wave_diff[i].imag=(t_imag*complex[i].real-t_real*complex[i].imag)/t_abs;
			}
			/* the group delay is the real part of wave_diff, but we need the amplitude calculated from complex */
			polar=(Polar*) complex;
			c2p(polar, complex, size);
			/* copy the real part of wave_diff into the argument of polar */
			for (i=0;i<size; i++) polar[i].arg=wave_diff[i].real;
			
			free(wave_diff);
			free(wave);
			
			(*a).data=polar;
			(*a).size=size;
			(*a).type=POLAR;
			break;
		case COMPLEX:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	return ERR_OK;
}

/*
convert group delay to phase
only POLAR
*/

long esweep_gDelayToPhase(esweep_data *a) {
	Polar *polar;
	long i;
	double dw;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case POLAR:
			polar=(Polar*) (*a).data;
			/* integrate phase response */
			dw=2*M_PI*(*a).samplerate/(*a).size;
			polar[0].arg*=dw;
			for (i=1;i<(*a).size;i++) { 
				polar[i].arg=polar[i-1].arg-polar[i].arg*dw;
			}
			/* wrap phase */
			dsp_wrapPhase(polar, (*a).size);
			break;
		case COMPLEX:
		case WAVE:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	return ERR_OK;
}

/*
smooth magnitude and argument of "a" in the range Octave/factor
only POLAR
*/

long esweep_smooth(esweep_data *a, double factor) {
	Polar *polar;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	if (factor<0.1) return ERR_BAD_ARGUMENT;
	
	switch ((*a).type) {
		case POLAR:
			polar=(Polar*) (*a).data;
			smooth(polar, factor, (*a).size);
			break;
		case COMPLEX:
		case WAVE:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	return ERR_OK;
}

/* 
applies a window to a WAVE
posibble window types are:

- Rectangle
- von Hann ("hanning")
- Hamming
- Bartlett (triangle)
- Blackman

both left and right may be different, width in percent
*/


long esweep_window(esweep_data *a, const char *left_win, double left_width, const char *right_win, double right_width) {
	long win_type;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	if ((*a).type!=WAVE) return ERR_NOT_ON_THIS_TYPE;
	
	if ((left_width<0.0) || (left_width > 100.0) || (right_width < 0.0) || (right_width > 100.0)) return ERR_BAD_ARGUMENT;
	
	/* left window */
	
	if (strlen(left_win)>0) {
		/* set window type */
		
		win_type=WIN_NOWIN;
		if (strcmp("rect", left_win)==0) { win_type=WIN_RECT; }
		if (strcmp("bart", left_win)==0) { win_type=WIN_BART; }
		if (strcmp("hann", left_win)==0) { win_type=WIN_HANN; }
		if (strcmp("hamm", left_win)==0) { win_type=WIN_HAMM; }
		if (strcmp("black", left_win)==0) { win_type=WIN_BLACK; }
		
		/* apply window */
		
		window((Wave*) (*a).data, 0, (long) ((*a).size*left_width/100+0.5), WIN_LEFT, win_type);
	}
	
	/* right window */
	
	if (strlen(right_win)>0) {
		/* set window type */
		
		win_type=WIN_NOWIN;
		if (strcmp("rect", right_win)==0) { win_type=WIN_RECT; }
		if (strcmp("bart", right_win)==0) { win_type=WIN_BART; }
		if (strcmp("hann", right_win)==0) { win_type=WIN_HANN; }
		if (strcmp("hamm", right_win)==0) { win_type=WIN_HAMM; }
		if (strcmp("black", right_win)==0) { win_type=WIN_BLACK; }
		
		/* apply window */
		
		window((Wave*) (*a).data, (long) ((*a).size-(*a).size*right_width/100+0.5), (*a).size, WIN_RIGHT, win_type);
	}
	
	return ERR_OK;
}
