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
#include <time.h>

#include "esweep.h"
#include "fft.h"

/* generate a single sine */

double esweep_genSine(esweep_data *output, double freq, double phase) {
	Wave *wave;
	double df, phi;
	double *period;
	long i, j, step, offset;
	
	if ((output==NULL) || ((*output).size<=0) || ((*output).data==NULL)) return ERR_EMPTY_CONTAINER;
	if ((*output).type!=WAVE) return ERR_NOT_ON_THIS_TYPE;
	
	if ((freq>=(*output).samplerate/2) || (freq<0.0)) {
		return ERR_BAD_ARGUMENT;
	}
	
	wave=(Wave*) (*output).data;
	
	/* sample one period */
	period=(double*) calloc((*output).size, sizeof(double));
	for (i=0;i<(*output).size;i++) {
		period[i]=sin(2*M_PI*i/(*output).size);
	}
	
	df=(double)(*output).samplerate/(*output).size;
	step=(long) (freq/df+0.5);
	if (phase<0.0) phi=-phase+M_PI;
	else phi=phase;
	offset=(long) (phi*(*output).size/(2*M_PI)+0.5);
	for (i=0, j=offset;i<(*output).size; i++, j+=step) {
		wave[i]=period[j%(*output).size];
	}
	
	free(period);
	
	return step*df;
}

#define WHITE 0
#define PINK 1
#define RED 2

#ifdef TAU_START
#undef TAU_START
#endif

#define TAU_START 0.1

double esweep_genLogsweep(esweep_data *output, double f1, double f2, const char *spec) {
	Wave *wave;
	Polar *polar;
	Complex *complex, *fft_table;
	
	double sweep_rate;
	double time;
	double df;
	double tau_offset;
	double scale;
	long env; /* spectral envelope */
	long size, hermitian_size;
	long i, i_f1, i_f2;
	
	if ((output==NULL) || ((*output).size<=0) || ((*output).data==NULL)) return ERR_EMPTY_CONTAINER;
	if ((*output).type!=WAVE) return ERR_NOT_ON_THIS_TYPE;
	if ((f1>f2) || (f2>=(*output).samplerate/2)) return ERR_BAD_ARGUMENT; /* Nyquist */
	
	env=-1;
	if (strcmp("white", spec)==0) env=WHITE;
	if (strcmp("pink", spec)==0) env=PINK;
	if (strcmp("red", spec)==0) env=RED;
	if (env==-1) return ERR_BAD_ARGUMENT;
	
	if ((time=(double) (*output).size/(*output).samplerate)<0.5) return ERR_BAD_ARGUMENT;
	
	size=(long) (2*pow(2, ceil(log((*output).size)/log(2)))+0.5);
	df=(double) (*output).samplerate/size;
	sweep_rate=(time-TAU_START)/log((*output).samplerate/(2*df));
	tau_offset=TAU_START-sweep_rate*log(df);
	
	polar=(Polar*) calloc(size, sizeof(Polar));
	hermitian_size=size;
	size=size/2+1; /* hermitian redundancy */
	
	/* create the phase */
	for (i=1;i<size;i++) {
		polar[i].arg=polar[i-1].arg-2*M_PI*df*(tau_offset+sweep_rate*log(i*df));
	}
	
	/* DC and fs/2 must be pure real, we have to add a small offset */
	tau_offset=polar[size-1].arg-M_PI*floor(polar[size-1].arg/M_PI);
	for (i=1;i<size;i++) {
		polar[i].arg-=tau_offset*2*i*df/(*output).samplerate;
	}
	
	/* create the magnitude spectrum */
	i_f1=(long) (0.5+f1/df);
	i_f2=(long) (0.5+f2/df);
	switch (env) {
		case WHITE:
			for (i=0;i<i_f2;i++) polar[i].abs=1.0; /* white */
			scale=i*df;
			for (;i<size;i++) polar[i].abs=scale/(i*df); /* red */
			break;
		case PINK:
			for (i=0;i<i_f1;i++) polar[i].abs=1.0; /* white */
			scale=sqrt(i*df);
			for (;i<i_f2;i++) polar[i].abs=scale/sqrt(i*df); /* pink*/
			scale=i*df;
			for (;i<size;i++) polar[i].abs=scale*polar[i_f2-1].abs/(i*df); /* red */
			break;
		case RED:
			for (i=0;i<i_f1;i++) polar[i].abs=1.0; /* white */
			scale=i*df;
			for (;i<size;i++) polar[i].abs=scale/((i-i_f1+1)*df); /* red */
			break;
		default: 
			break;
	}
	
	/* convert to complex */
	
	complex=(Complex*) polar;
	p2c(complex, polar, size);
	
	/* restore the hermitian redundancy */
	for (i=1;i<size-1;i++) {
		complex[hermitian_size-i].real=complex[i].real;
		complex[hermitian_size-i].imag=-complex[i].imag;
	}
	
	/* IFFT */
	fft_table=fft_create_table(hermitian_size);
	fft(complex, fft_table, hermitian_size, FFT_BACKWARD);
	free(fft_table);
	
	/* convert to waveform */
	wave=(Wave*) (*output).data;
	
	for (i=0;i<(*output).size;i++) wave[i]=complex[i].real;
	free(complex);
	
	/* downscaling */
	scale=wave[0];
	for (i=1;i<(*output).size;i++) {
		if (scale<fabs(wave[i])) scale=fabs(wave[i]);
	}
	for (i=0;i<(*output).size;i++) wave[i]/=scale;
	
	return sweep_rate;
}

#ifdef TAU_START
#undef TAU_START
#endif
#define TAU_START 0.05

double esweep_genLinsweep(esweep_data *output, double f1, double f2, const char *spec) {
	Wave *wave;
	Polar *polar;
	Complex *complex, *fft_table;
	
	double sweep_rate;
	double time;
	double df;
	double tau_offset;
	double scale;
	long env; /* spectral envelope */
	long size, hermitian_size;
	long i, i_f1, i_f2;
	
	if ((output==NULL) || ((*output).size<=0) || ((*output).data==NULL)) return ERR_EMPTY_CONTAINER;
	if ((*output).type!=WAVE) return ERR_NOT_ON_THIS_TYPE;
	if ((f1>f2) || (f2>=(*output).samplerate/2)) return ERR_BAD_ARGUMENT; /* Nyquist */
	
	env=-1;
	if (strcmp("white", spec)==0) env=WHITE;
	if (strcmp("pink", spec)==0) env=PINK;
	if (strcmp("red", spec)==0) env=RED;
	if (env==-1) return ERR_BAD_ARGUMENT;
	
	if ((time=(double) (*output).size/(*output).samplerate)<0.5) return ERR_BAD_ARGUMENT;
	
	size=(long) (2*pow(2, ceil(log((*output).size)/log(2)))+0.5);
	df=(double) (*output).samplerate/size;
	sweep_rate=2*(time-TAU_START)/(*output).samplerate;
	tau_offset=TAU_START-sweep_rate*df;
	
	polar=(Polar*) calloc(size, sizeof(Polar));
	hermitian_size=size;
	size=size/2+1; /* hermitian redundancy */
	
	/* create the phase */
	for (i=1;i<size;i++) {
		polar[i].arg=polar[i-1].arg-2*M_PI*df*(tau_offset+sweep_rate*i*df);
	}
	
	/* DC and fs/2 must be pure real, we have to add a small offset */
	tau_offset=polar[size-1].arg-M_PI*floor(polar[size-1].arg/M_PI);
	for (i=1;i<size;i++) {
		polar[i].arg-=tau_offset*2*i*df/(*output).samplerate;
	}
	
	/* create the magnitude spectrum */
	i_f1=(long) (0.5+f1/df);
	i_f2=(long) (0.5+f2/df);
	switch (env) {
		case WHITE:
			for (i=0;i<i_f2;i++) polar[i].abs=1.0; /* white */
			scale=i*df;
			for (;i<size;i++) polar[i].abs=scale/(i*df); /* red */
			break;
		case PINK:
			for (i=0;i<i_f1;i++) polar[i].abs=1.0; /* white */
			scale=sqrt(i*df);
			for (;i<i_f2;i++) polar[i].abs=scale/sqrt(i*df); /* pink*/
			scale=i*df;
			for (;i<size;i++) polar[i].abs=scale*polar[i_f2-1].abs/(i*df); /* red */
			break;
		case RED:
			for (i=0;i<i_f1;i++) polar[i].abs=1.0; /* white */
			scale=i*df;
			for (;i<size;i++) polar[i].abs=scale/((i-i_f1+1)*df); /* red */
			break;
		default: 
			break;
	}
	
	/* convert to complex */
	
	complex=(Complex*) polar;
	p2c(complex, polar, size);
	
	/* restore the hermitian redundancy */
	for (i=1;i<size-1;i++) {
		complex[hermitian_size-i].real=complex[i].real;
		complex[hermitian_size-i].imag=-complex[i].imag;
	}
	
	/* IFFT */
	fft_table=fft_create_table(hermitian_size);
	fft(complex, fft_table, hermitian_size, FFT_BACKWARD);
	free(fft_table);
	
	/* convert to waveform */
	wave=(Wave*) (*output).data;
	
	for (i=0;i<(*output).size;i++) wave[i]=complex[i].real;
	free(complex);
	
	/* downscaling */
	scale=wave[0];
	for (i=1;i<(*output).size;i++) {
		if (scale<fabs(wave[i])) scale=fabs(wave[i]);
	}
	for (i=0;i<(*output).size;i++) wave[i]/=scale;
	
	return sweep_rate;
}

long esweep_genNoise(esweep_data *output, double f1, double f2, const char *spec) {
	Wave *wave;
	Polar *polar;
	Complex *complex, *fft_table;
	
	double df;
	double scale;
	long env; /* spectral envelope */
	long size, hermitian_size;
	long i, i_f1, i_f2;
	
	if ((output==NULL) || ((*output).size<=0) || ((*output).data==NULL)) return ERR_EMPTY_CONTAINER;
	if ((*output).type!=WAVE) return ERR_NOT_ON_THIS_TYPE;
	if ((f1>f2) || (f2>=(*output).samplerate/2)) return ERR_BAD_ARGUMENT; /* Nyquist */
	
	env=-1;
	if (strcmp("white", spec)==0) env=WHITE;
	if (strcmp("pink", spec)==0) env=PINK;
	if (strcmp("red", spec)==0) env=RED;
	if (env==-1) return ERR_BAD_ARGUMENT;
	
	/* output size must be a power of 2 */
	size=(long) (pow(2, ceil(log((*output).size)/log(2)))+0.5);
	if (size!=(*output).size) return ERR_BAD_ARGUMENT;
	df=(double) (*output).samplerate/size;
	
	polar=(Polar*) calloc(size, sizeof(Polar));
	hermitian_size=size;
	size=size/2+1; /* hermitian redundancy */
	
	/* create the phase */
	srand(time(NULL)); /* seed for the RNG */
	for (i=1;i<size;i++) {
		polar[i].arg=(double) (rand() % 359 - 180);
		/* 
		 * the phase is in the range -180° -> 180°
		 * thus, we must convert it to radians
		*/
		
		polar[i].arg=M_PI*polar[i].arg/180;
	}
	
	/* the phase at DC and Nyquist must be 0 */
	polar[0].arg=polar[size-1].arg=0.0;
	
	/* create the magnitude spectrum */
	i_f1=(long) (0.5+f1/df);
	i_f2=(long) (0.5+f2/df);
	switch (env) {
		case WHITE:
			for (i=0;i<i_f2;i++) polar[i].abs=1.0; /* white */
			scale=i*df;
			for (;i<size;i++) polar[i].abs=scale/(i*df); /* red */
			break;
		case PINK:
			for (i=0;i<i_f1;i++) polar[i].abs=1.0; /* white */
			scale=sqrt(i*df);
			for (;i<i_f2;i++) polar[i].abs=scale/sqrt(i*df); /* pink*/
			scale=i*df;
			for (;i<size;i++) polar[i].abs=scale*polar[i_f2-1].abs/(i*df); /* red */
			break;
		case RED:
			for (i=0;i<i_f1;i++) polar[i].abs=1.0; /* white */
			scale=i*df;
			for (;i<size;i++) polar[i].abs=scale/((i-i_f1+1)*df); /* red */
			break;
		default: 
			break;
	}
	
	/* convert to complex */
	
	complex=(Complex*) polar;
	p2c(complex, polar, size);
	
	/* restore the hermitian redundancy */
	for (i=1;i<size-1;i++) {
		complex[hermitian_size-i].real=complex[i].real;
		complex[hermitian_size-i].imag=-complex[i].imag;
	}
	
	/* IFFT */
	fft_table=fft_create_table(hermitian_size);
	fft(complex, fft_table, hermitian_size, FFT_BACKWARD);
	free(fft_table);
	
	/* convert to waveform */
	wave=(Wave*) (*output).data;
	
	for (i=0;i<(*output).size;i++) wave[i]=complex[i].real;
	free(complex);
	
	/* downscaling */
	scale=wave[0];
	for (i=1;i<(*output).size;i++) {
		if (scale<fabs(wave[i])) scale=fabs(wave[i]);
	}
	for (i=0;i<(*output).size;i++) wave[i]/=scale;
	
	return ERR_OK;
}

/* concatenates src starting at src_pos to dst ending at dst_pos 
 * complex or polar data will be concatenated at dst_pos
*/

long esweep_concat(esweep_data *dst, esweep_data *src, long dst_pos, long src_pos) {
	Audio *audio;
	Wave *wave;
	Polar *dst_polar, *src_polar;
	Complex *dst_complex, *src_complex;
	
	if ((dst==NULL) || (src==NULL)) return ERR_EMPTY_CONTAINER; /* no input data */
	if ((*dst).type!=(*src).type) return ERR_DIFF_TYPES; /* must be of same type */
	if ((dst_pos>=(*dst).size) || (dst_pos<0)) return ERR_BAD_ARGUMENT; /* bad dst position */
	if ((*dst).samplerate!=(*src).samplerate) return ERR_DIFF_MAPPING; /* same samplerate? */
	
	switch ((*src).type) {
		case AUDIO:
			if ((src_pos>=(*src).size) || (src_pos<0)) return ERR_BAD_ARGUMENT; /* bad src position */
			audio=(Audio*) calloc(2*(dst_pos+1+(*src).size-src_pos), sizeof(Audio));
			memcpy(audio, (*dst).data, 2*(dst_pos+1)*sizeof(Audio));
			memcpy(&(audio[2*(dst_pos+1)]), (*src).data, 2*((*src).size-src_pos)*sizeof(Audio));
			free((*dst).data);
			(*dst).data=audio;
			(*dst).size=dst_pos+(*src).size-src_pos;
			break;
		case WAVE:
			if ((src_pos>=(*src).size) || (src_pos<0)) return ERR_BAD_ARGUMENT; /* bad src position */
			wave=(Wave*) calloc(dst_pos+1+(*src).size-src_pos, sizeof(Wave));
			memcpy(wave, (*dst).data, (dst_pos+1)*sizeof(Wave));
			memcpy(&(wave[dst_pos+1]), (*src).data, ((*src).size-src_pos)*sizeof(Wave));
			free((*dst).data);
			(*dst).data=wave;
			(*dst).size=dst_pos+(*src).size-src_pos;
			break;
		case POLAR:
			if ((*dst).size!=(*src).size) return ERR_DIFF_MAPPING; /* same size, i. e. same delta_f because of same fs */
			dst_polar=(Polar*) (*dst).data;
			src_polar=(Polar*) (*src).data;
			memcpy(&(dst_polar[dst_pos]), &(src_polar[dst_pos]), ((*src).size-dst_pos)*sizeof(Polar));
			break;
		case COMPLEX:
			if ((*dst).size!=(*src).size) return ERR_DIFF_MAPPING; /* same size, i. e. same delta_f because of same fs */
			dst_complex=(Complex*) (*dst).data;
			src_complex=(Complex*) (*src).data;
			memcpy(&(dst_complex[dst_pos]), &(src_complex[dst_pos]), ((*src).size-dst_pos)*sizeof(Complex));
			break;
		case SURFACE:
		default:
			return 1;
	}
	
	return 0;
}

esweep_data *esweep_cut(esweep_data *a, long start, long stop) {
	Audio *audio_out, *audio_in;
	Wave *wave_out, *wave_in;
	esweep_data *dst=NULL;
	long size;
	long i;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return NULL;
	
	size=stop-start;
	if ((size<=0) || (size>(*a).size)) return NULL;
	if (start<0) start=(*a).size+start;
	if (stop<0) stop=(*a).size+stop;
	
	/* contain start and stop in the array */
	start%=(*a).size;
	stop%=(*a).size;
	
	/* allocate a new esweep_data structure */
	dst=(esweep_data*) calloc(1, sizeof(esweep_data));
	(*dst).size=size;
	(*dst).samplerate=(*a).samplerate;
	(*dst).type=(*a).type;
	
	switch ((*a).type) {
		case AUDIO:
			audio_out=(Audio*) calloc(2*size, sizeof(Audio));
			audio_in=(Audio*) (*a).data;
			for (i=0;i<2*size;i+=2) {
				audio_out[i]=audio_in[(start+i)%(2*(*a).size)];
				audio_out[i+1]=audio_in[(start+i+1)%(2*(*a).size)];
			}
			(*dst).data=audio_out;
			break;
		case WAVE:
			wave_out=(Wave*) calloc(2*size, sizeof(Wave));
			wave_in=(Wave*) (*a).data;
			for (i=0;i<size;i++) wave_out[i]=wave_in[(start+i)%(*a).size];
			(*dst).data=wave_out;
			break;
		case COMPLEX:
		case POLAR:
		case SURFACE:
		default:
			free(dst);
			return NULL;
	}
	return dst;
}

long esweep_pad(esweep_data *a, long size) { /* zero padding */
	Audio *audio;
	Wave *wave;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	if (size<=(*a).size) return ERR_BAD_ARGUMENT;
	
	switch ((*a).type) {
		case AUDIO:
			audio=(Audio*) calloc(2*size, sizeof(Audio));
			memcpy(audio, (Audio*) (*a).data, 2*(*a).size*sizeof(Audio));
			free((*a).data);
			(*a).data=audio;
			(*a).size=size;
			break;
		case WAVE:
			wave=(Wave*) calloc(size, sizeof(Wave));
			memcpy(wave, (Wave*) (*a).data, (*a).size*sizeof(Wave));
			free((*a).data);
			(*a).data=wave;
			(*a).size=size;
			break;
		case COMPLEX:
		case POLAR:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}
