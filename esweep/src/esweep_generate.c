/*
 * Copyright (c) 2011 Jochen Fabricius <jfab@berlios.de>
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

#include "esweep_priv.h"
#include "fft.h"

/* generate a single sine */

int esweep_genSine(esweep_object *obj, Real freq, Real phase, int *periods) {
	Wave *wave;
	Complex *cpx; 
	Polar *polar; 
	Real dphi, f;
	int i, period;
	int p; 

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT); 

	ESWEEP_ASSERT(freq < obj->samplerate/2, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(freq > 0, ERR_BAD_ARGUMENT);

	p=(int) (obj->size*freq/obj->samplerate); // number of periods, rounded
	ESWEEP_ASSERT(p >= 1, ERR_BAD_ARGUMENT);
	f=1.0*p*obj->samplerate/obj->size; // the truly generated frequency
	if (periods != NULL) *periods=p; 
		
	dphi=2*M_PI*f/obj->samplerate; // 
	/* length of one period in samples */
	period=(int) (0.5+obj->samplerate/freq); 

	switch (obj->type) {
		case WAVE: 
			wave=(Wave*) obj->data;
			/* create one period as a lookup table */
			for (i=0; i < period || i < obj->size; i++) {
				wave[i]=sin(i*dphi+phase); 
			}
			/* now create the rest of the waveform */
			for (; i < obj->size; i++) {
				wave[i]=wave[i%period];
			}
			break; 
		case COMPLEX: 
			cpx=(Complex*) obj->data;
			/* create one period as a lookup table */
			for (i=0; i < period || i < obj->size; i++) {
				cpx[i].real=sin(i*dphi+phase); 
				cpx[i].imag=cos(i*dphi+phase); 
			}
			/* now create the rest of the waveform */
			for (; i < obj->size; i++) {
				cpx[i].real=cpx[i%period].real;
				cpx[i].imag=cpx[i%period].imag;
			}
			break; 
		case POLAR: 
			polar=(Polar*) obj->data;
			/* create one period as a lookup table */
			for (i=0; i < period || i < obj->size; i++) {
				polar[i].abs=1.0; 
				/* We need to adjust the phase so that cos(arg) gives the desired sine wave */
				polar[i].arg=i*dphi+phase-M_PI_2; 
			}
			/* now create the rest of the waveform */
			for (; i < obj->size; i++) {
				polar[i].abs=1.0; 
				polar[i].arg=polar[i%period].arg;
			}
			break; 
		default: 
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	return ERR_OK; 
}

#define WHITE 0
#define PINK 1
#define RED 2

#ifdef TAU_START
#undef TAU_START
#endif

#define TAU_START 0.1

int esweep_genLogsweep(esweep_object *obj, Real locut, Real hicut, const char *spec, Real *sweep_rate) {
	Wave *wave;
	Complex *cpx; 
	Polar *polar; 
	Complex *fft_table;

	Real time;
	Real df;
	Real tau_offset;
	Real scale;
	int env; /* spectral envelope */
	int size, hermitian_size;
	int i, i_locut, i_hicut;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT); 

	ESWEEP_ASSERT(locut < hicut, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(hicut < obj->samplerate/2, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(locut > 0.0, ERR_BAD_ARGUMENT);

	if (obj->type == SURFACE) {
		ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE); 
	}

	env=-1;
	if (strcmp("white", spec)==0) env=WHITE;
	if (strcmp("pink", spec)==0) env=PINK;
	if (strcmp("red", spec)==0) env=RED;
	ESWEEP_ASSERT(env > -1, ERR_BAD_ARGUMENT);

	/* at least 500 ms */
	ESWEEP_ASSERT(obj->samplerate/obj->size < 2, ERR_BAD_ARGUMENT);
	time=(Real) obj->size/obj->samplerate; 

	size=(int) (2*pow(2, ceil(log(obj->size)/log(2)))+0.5);
	df=(Real) obj->samplerate/size;
	*sweep_rate=(time-TAU_START)/log(obj->samplerate/(2*df));
	tau_offset=TAU_START-*sweep_rate*log(df);

	ESWEEP_MALLOC(polar, size, sizeof(Polar), ERR_MALLOC);
	hermitian_size=size;
	size=size/2+1; /* hermitian redundancy */

	/* create the phase */
	for (i=1;i<size;i++) {
		polar[i].arg=polar[i-1].arg-2*M_PI*df*(tau_offset+*sweep_rate*log(i*df));
	}

	/* DC and fs/2 must be pure real, we have to add a small offset */
	tau_offset=polar[size-1].arg-M_PI*floor(polar[size-1].arg/M_PI);
	for (i=1;i<size;i++) {
		polar[i].arg-=tau_offset*2*i*df/obj->samplerate;
	}

	/* create the magnitude spectrum */
	i_locut=(int) (0.5+locut/df);
	i_hicut=(int) (0.5+hicut/df);
	switch (env) {
		case WHITE:
			for (i=0;i<i_hicut;i++) polar[i].abs=1.0; /* white */
			scale=i*df;
			for (;i<size;i++) polar[i].abs=scale/(i*df); /* red */
			break;
		case PINK:
			for (i=0;i<i_locut;i++) polar[i].abs=1.0; /* white */
			scale=sqrt(i*df);
			for (;i<i_hicut;i++) polar[i].abs=scale/sqrt(i*df); /* pink*/
			scale=i*df;
			for (;i<size;i++) polar[i].abs=scale*polar[i_hicut-1].abs/(i*df); /* red */
			break;
		case RED:
			for (i=0;i<i_locut;i++) polar[i].abs=1.0; /* white */
			scale=i*df;
			for (;i<size;i++) polar[i].abs=scale/((i-i_locut+1)*df); /* red */
			break;
		default:
			break;
	}

	/* restore the hermitian redundancy */
	for (i=1;i<size-1;i++) {
		polar[hermitian_size-i].abs=polar[i].abs;
		polar[hermitian_size-i].arg=-polar[i].arg;
	}

	/* if the object is Polar, then the polar spectrum is used as the result */
	if (obj->type == POLAR) {
		obj->data=(void*) polar; 
		obj->size=hermitian_size; 
		return ERR_OK; 
	}

	/* otherwise convert to Complex */
	cpx=(Complex*) polar;
	p2c(cpx, polar, hermitian_size);

	/* IFFT */
	fft_table=fft_create_table(hermitian_size);
	fft(cpx, fft_table, hermitian_size, FFT_BACKWARD);
	free(fft_table);

	/* if the object is Complex, then the complex waveform is used as the result */

	/* The total size of the complex object is hermitian_size, but we keep the original object
	 * size, as we are now in the time domain */
	scale=cpx[i].real;
	for (i=1; i < obj->size; i++) {
		if (cpx[i].real > scale) scale=cpx[i].real; 
		if (-cpx[i].real > scale) scale=-cpx[i].real; 
	}
	for (i=0; i < obj->size; i++) {
		cpx[i].real=cpx[i].real/scale; 
		cpx[i].imag=cpx[i].imag/scale; 
	}
	if (obj->type == COMPLEX) {
		free(obj->data);
		obj->data=(void*) cpx; 
		return ERR_OK; 
	}

	/* convert to waveform */
	wave=(Wave*) obj->data;

	for (i=0;i<obj->size;i++) wave[i]=cpx[i].real;
	free(cpx);

	return ERR_OK;
}

int esweep_genDirac(esweep_object *obj, Real delay) {
	int dSamples, i; 
	Real t; 
	Wave *wave; 
	Polar *polar; 
	Complex *cpx; 
	 
	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT); 
	ESWEEP_ASSERT(delay >= 0, ERR_BAD_ARGUMENT); 
	dSamples=(int) (delay/1000*obj->samplerate); 
	ESWEEP_ASSERT(dSamples < obj->size, ERR_BAD_ARGUMENT); 
	switch (obj->type) {
		case WAVE: 
			wave=(Wave*) obj->data; 
			memset(wave, 0, obj->size*sizeof(Wave)); 
			wave[dSamples]=1.0; 
			break; 
		case COMPLEX: 
			cpx=(Complex*) obj->data; 
			memset(cpx, 0, obj->size*sizeof(Complex)); 
			cpx[dSamples].real=1.0; 
			break; 
		case POLAR: 
			polar=(Polar*) obj->data; 
			t=2*M_PI*obj->samplerate*delay/1000; 
			for (i=0; i<obj->size; i++) {
				polar[i].abs=1.0; 
				polar[i].arg=i*t/obj->size; 
			}
			break; 
		default: 
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE); 

	}
	return ERR_OK; 
}


#if 0
#ifdef TAU_START
#undef TAU_START
#endif
#define TAU_START 0.05

Real esweep_genLinsweep(esweep_object *output, Real f1, Real f2, const char *spec, Real *sweep_rate) {
	Wave *wave;
	Polar *polar;
	Complex *complex, *fft_table;

	Real time;
	Real df;
	Real tau_offset;
	Real scale;
	int env; /* spectral envelope */
	int size, hermitian_size;
	int i, i_f1, i_f2;

	if ((output==NULL) || (obj->size<=0) || (obj->data==NULL)) return ERR_EMPTY_OBJECT;
	if (obj->type!=WAVE) return ERR_NOT_ON_THIS_TYPE;
	if ((f1>f2) || (f2>=obj->samplerate/2)) return ERR_BAD_ARGUMENT; /* Nyquist */

	env=-1;
	if (strcmp("white", spec)==0) env=WHITE;
	if (strcmp("pink", spec)==0) env=PINK;
	if (strcmp("red", spec)==0) env=RED;
	if (env==-1) return ERR_BAD_ARGUMENT;

	if ((time=(Real) obj->size/obj->samplerate)<0.5) return ERR_BAD_ARGUMENT;

	size=(int) (2*pow(2, ceil(log(obj->size)/log(2)))+0.5);
	df=(Real) obj->samplerate/size;
	*sweep_rate=2*(time-TAU_START)/obj->samplerate;
	tau_offset=TAU_START-*sweep_rate*df;

	polar=(Polar*) calloc(size, sizeof(Polar));
	hermitian_size=size;
	size=size/2+1; /* hermitian redundancy */

	/* create the phase */
	for (i=1;i<size;i++) {
		polar[i].arg=polar[i-1].arg-2*M_PI*df*(tau_offset+*sweep_rate*i*df);
	}

	/* DC and fs/2 must be pure real, we have to add a small offset */
	tau_offset=polar[size-1].arg-M_PI*floor(polar[size-1].arg/M_PI);
	for (i=1;i<size;i++) {
		polar[i].arg-=tau_offset*2*i*df/obj->samplerate;
	}

	/* create the magnitude spectrum */
	i_f1=(int) (0.5+f1/df);
	i_f2=(int) (0.5+f2/df);
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
	wave=(Wave*) obj->data;

	for (i=0;i<obj->size;i++) wave[i]=complex[i].real;
	free(complex);

	/* downscaling */
	scale=wave[0];
	for (i=1;i<obj->size;i++) {
		if (scale<fabs(wave[i])) scale=fabs(wave[i]);
	}
	for (i=0;i<obj->size;i++) wave[i]/=scale;

	return ERR_OK; 
}

#endif

int esweep_genNoise(esweep_object *obj, Real locut, Real hicut, const char *spec) {
	Wave *wave;
	Polar *polar;
	Complex *cpx, *fft_table;

	Real df;
	Real scale;
	int env; /* spectral envelope */
	int size, hermitian_size;
	int i, i_f1, i_f2;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT); 

	ESWEEP_ASSERT(locut < hicut, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(hicut < obj->samplerate/2, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(locut > 0.0, ERR_BAD_ARGUMENT);

	if (obj->type == SURFACE) {
		ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE); 
	}

	ESWEEP_ASSERT(obj->size >= 4, ERR_BAD_ARGUMENT);

	env=-1;
	if (strcmp("white", spec)==0) env=WHITE;
	if (strcmp("pink", spec)==0) env=PINK;
	if (strcmp("red", spec)==0) env=RED;
	if (env==-1) return ERR_BAD_ARGUMENT;

	size=(int) (2*pow(2, ceil(log(obj->size)/log(2)))+0.5);
	df=(Real) obj->samplerate/size;

	/* unlike the sweeps, the noise must have a 2^n length, so we can free the data block anyways */
	free(obj->data); 
	obj->size=size; 

	ESWEEP_MALLOC(polar, size, sizeof(Polar), ERR_MALLOC);

	hermitian_size=size;
	size=size/2+1; /* hermitian redundancy */

	/* create the phase */
	srand(time(NULL)); /* seed for the RNG */
	for (i=1;i<size;i++) {
		polar[i].arg=(Real) (rand() % 359 - 180);
		/*
		 * the phase is in the range -180° -> 180°
		 * thus, we must convert it to radians
		*/

		polar[i].arg=M_PI*polar[i].arg/180;
	}

	/* the phase at DC and Nyquist must be 0 */
	polar[0].arg=polar[size-1].arg=0.0;

	/* create the magnitude spectrum */
	i_f1=(int) (0.5+locut/df);
	i_f2=(int) (0.5+hicut/df);
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

	/* restore the hermitian redundancy */
	for (i=1;i<size-1;i++) {
		polar[hermitian_size-i].abs=polar[i].abs;
		polar[hermitian_size-i].arg=-polar[i].arg;
	}

	/* if the object is Polar, then the polar spectrum is used as the result */
	if (obj->type == POLAR) {
		obj->data=(void*) polar; 
		obj->size=hermitian_size; 
		return ERR_OK; 
	}

	/* otherwise convert to Complex */
	cpx=(Complex*) polar;
	p2c(cpx, polar, hermitian_size);

	/* IFFT */
	fft_table=fft_create_table(hermitian_size);
	fft(cpx, fft_table, hermitian_size, FFT_BACKWARD);
	free(fft_table);

	/* scale to 1 */
	scale=cpx[i].real;
	for (i=1; i < obj->size; i++) {
		if (cpx[i].real > scale) scale=cpx[i].real; 
		if (-cpx[i].real > scale) scale=-cpx[i].real; 
	}
	for (i=0; i < obj->size; i++) {
		cpx[i].real=cpx[i].real/scale; 
		cpx[i].imag=cpx[i].imag/scale; 
	}
	
	/* if the object is Complex, then the complex waveform is used as the result */
	if (obj->type == COMPLEX) {
		obj->data=(void*) cpx; 
		return ERR_OK; 
	}

	/* convert to waveform */
	ESWEEP_MALLOC(wave, obj->size, sizeof(Wave), ERR_MALLOC);

	for (i=0;i<obj->size;i++) wave[i]=cpx[i].real;
	obj->data= (void*) wave; 
	free(cpx);

	return ERR_OK;
}

#if 0

/* concatenates src starting at src_pos to dst ending at dst_pos
 * complex or polar data will be concatenated at dst_pos
*/

int esweep_concat(esweep_object *dst, esweep_object *src, int dst_pos, int src_pos) {
	Wave *wave;
	Polar *dst_polar, *src_polar;
	Complex *dst_complex, *src_complex;

	if ((dst==NULL) || (src==NULL)) return ERR_EMPTY_OBJECT; /* no input data */
	if ((*dst).type!=(*src).type) return ERR_DIFF_TYPES; /* must be of same type */
	if ((dst_pos>=(*dst).size) || (dst_pos<0)) return ERR_BAD_ARGUMENT; /* bad dst position */
	if ((*dst).samplerate!=(*src).samplerate) return ERR_DIFF_MAPPING; /* same samplerate? */

	switch ((*src).type) {
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

esweep_object *esweep_cut(esweep_object *a, int start, int stop) {
	Wave *wave_out, *wave_in;
	esweep_object *dst=NULL;
	int size;
	int i;

	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return NULL;

	size=stop-start;
	if ((size<=0) || (size>(*a).size)) return NULL;
	if (start<0) start=(*a).size+start;
	if (stop<0) stop=(*a).size+stop;

	/* contain start and stop in the array */
	start%=(*a).size;
	stop%=(*a).size;

	/* allocate a new esweep_object structure */
	dst=(esweep_object*) calloc(1, sizeof(esweep_object));
	(*dst).size=size;
	(*dst).samplerate=(*a).samplerate;
	(*dst).type=(*a).type;

	switch ((*a).type) {
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

int esweep_pad(esweep_object *a, int size) { /* zero padding */
	Wave *wave;

	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_OBJECT;
	if (size<=(*a).size) return ERR_BAD_ARGUMENT;

	switch ((*a).type) {
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
#endif 
