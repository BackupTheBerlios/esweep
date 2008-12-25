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

#include "esweep.h"
#include "fft.h"

double esweep_max(esweep_data *a) {
	Wave *wave;
	Polar *polar;
	Surface *surf;
	
	double max;
	long i, size;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	
	switch ((*a).type) {
		case WAVE:
			size=(*a).size;
			wave=(Wave*) (*a).data;
			for (max=wave[0], i=1;i<size;i++) {
				if (max < wave[i]) max=wave[i];
			}
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			for (max=polar[0].abs, i=1;i<(*a).size;i++) {
				if (max < polar[i].abs) max=polar[i].abs;
			}
			break;
		case SURFACE:
			surf=(Surface*) (*a).data;
			size=(*surf).xsize*(*surf).ysize;
			for (max=(*surf).z[0], i=1;i<size;i++) {
				if (max < (*surf).z[i]) max=(*surf).z[i];
			}
			break;
		case COMPLEX:
		case AUDIO:
		default:
			return 0.0;
	}
	return max;
}

double esweep_min(esweep_data *a) {
	Wave *wave;
	Polar *polar;
	Surface *surf;
	
	double min;
	long i, size;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			size=(*a).size;
			wave=(Wave*) (*a).data;
			for (min=wave[0], i=1;i<size;i++) {
				if (min > wave[i]) min=wave[i];
			}
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			for (min=polar[0].abs, i=1;i<(*a).size;i++) {
				if (min > polar[i].abs) min=polar[i].abs;
			}
			break;
		case SURFACE:
			surf=(Surface*) (*a).data;
			size=(*surf).xsize*(*surf).ysize;
			for (min=(*surf).z[0], i=1;i<size;i++) {
				if (min > (*surf).z[i]) min=(*surf).z[i];
			}
			break;
		case COMPLEX:
		case AUDIO:
		default:
			return 0.0;
	}
	return min;
}

double esweep_mean(esweep_data *a) {
	Wave *wave;
	Polar *polar;
	
	double mean=0.0;
	long i;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return 0.0;
	
	switch ((*a).type) {
		case WAVE:
			wave=(Wave*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				mean+=wave[i];
			}
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				mean+=polar[i].abs;
			}
			break;
		case COMPLEX:
		case SURFACE:
		case AUDIO:
		default:
			return 0.0;
			break;
	}
	
	return mean/(*a).size;
}

/*
sums up a WAVE in the range given by the indices lo and hi
*/

double esweep_sum(esweep_data *a, long lo, long hi) {
	Wave *wave;
	long i;
	double sum;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	if ((*a).type!=WAVE) return ERR_NOT_ON_THIS_TYPE;
	
	if ((lo<0) || (lo>hi) || (hi>(*a).size)) return ERR_BAD_ARGUMENT;
	
	wave=(Wave*) (*a).data;
	sum=0.0;
	for (i=lo;i<hi;i++) {
		sum+=wave[i];
	}
	
	return sum;
}

long esweep_real(esweep_data *a) {
	Wave *wave;
	Complex *complex;
	long i;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case COMPLEX:
			complex=(Complex*) (*a).data;
			wave=(Wave*) calloc((*a).size, sizeof(Wave));
			for (i=0;i<(*a).size;i++) wave[i]=complex[i].real;
			free(complex);
			(*a).data=(void*)wave;
			(*a).type=WAVE;
			break;
		case WAVE:
		case POLAR:
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

long esweep_imag(esweep_data *a) {
	Wave *wave;
	Complex *complex;
	long i;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case COMPLEX:
			complex=(Complex*) (*a).data;
			wave=(Wave*) calloc((*a).size, sizeof(Wave));
			for (i=0;i<(*a).size;i++) wave[i]=complex[i].imag;
			free(complex);
			(*a).data=(void*)wave;
			(*a).type=WAVE;
			break;
		case WAVE:
		case POLAR:
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

long esweep_abs(esweep_data *a) {
	Wave *wave;
	Complex *complex;
	Polar *polar;
	long i, size;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	size=(*a).size/2+1;
	switch ((*a).type) {
		case WAVE:
			wave=(Wave*) (*a).data;
			for (i=0;i<(*a).size;i++) wave[i]=fabs(wave[i]);
			break;
		case COMPLEX:
			complex=(Complex*) (*a).data;
			wave=(Wave*) calloc((*a).size, sizeof(Wave));
			c2r(wave, complex, (*a).size);
			free(complex);
			(*a).data=(void*)wave;
			(*a).type=WAVE;
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			wave=(Wave*) calloc((*a).size, sizeof(Wave));
			for (i=0;i<size;i++) wave[i]=polar[i].abs;
			free(polar);
			(*a).data=(void*)wave;
			(*a).type=WAVE;
			(*a).size=size;
			break;
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

long esweep_arg(esweep_data *a) {
	Wave *wave;
	Complex *complex;
	Polar *polar;
	long i, size;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	size=(*a).size/2+1;
	switch ((*a).type) {
		case COMPLEX:
			complex=(Complex*) (*a).data;
			wave=(Wave*) calloc(size, sizeof(Wave));
			for (i=0;i<(*a).size;i++) wave[i]=atan2(complex[i].imag, complex[i].real);
			free(complex);
			(*a).data=(void*)wave;
			(*a).type=WAVE;
			(*a).size=size;
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			wave=(Wave*) calloc(size, sizeof(Wave));
			for (i=0;i<size;i++) wave[i]=polar[i].arg;
			free(polar);
			(*a).data=(void*)wave;
			(*a).type=WAVE;
			(*a).size=size;
			break;
		case WAVE:
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

/* basic math */

long esweep_add(esweep_data *a, esweep_data *b) {
	Wave *wave_out, *wave_a, *wave_b;
	Complex *complex_a, *complex_b;
	Polar *polar_a, *polar_b;
	long i;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	if ((b==NULL) || ((*b).data==NULL) || ((*b).size<=0)) return ERR_EMPTY_CONTAINER;
	
	if (((*a).samplerate!=(*b).samplerate) || ((*a).type!=(*b).type)) return ERR_DIFF_MAPPING;
	
	switch ((*a).type) {
		case WAVE:
			wave_a=(Wave*) (*a).data;
			wave_b=(Wave*) (*b).data;
			if ((*a).size<(*b).size) {
				wave_out=(Wave*) calloc((*b).size, sizeof(Wave));
				for (i=0;i<(*a).size;i++) {
					wave_out[i]=wave_a[i]+wave_b[i];
				}
				for (;i<(*b).size;i++) {
					wave_out[i]=wave_b[i];
				}
				free(wave_a);
				(*a).data=wave_out;
				(*a).size=(*b).size;
			} else {
				for (i=0;i<(*b).size;i++) {
					wave_a[i]+=wave_b[i];
				}
			}
			break;
			
		case COMPLEX:
			if ((*a).size!=(*b).size) return ERR_DIFF_MAPPING;
			
			complex_a=(Complex*) (*a).data;
			complex_b=(Complex*) (*b).data;
			
			for (i=0;i<(*a).size;i++) {
				complex_a[i].real+=complex_b[i].real;
				complex_a[i].imag+=complex_b[i].imag;
			}
			break;
		case POLAR:
			if ((*a).size!=(*b).size) return ERR_DIFF_MAPPING;
			
			polar_a=(Polar*) (*a).data;
			polar_b=(Polar*) (*b).data;
			complex_a=(Complex*) (*a).data;
			complex_b=(Complex*) (*b).data;
			
			/* convert to complex */
			
			p2c(complex_a, polar_a, (*a).size);
			p2c(complex_b, polar_b, (*b).size);
			
			/* complex addition */
			
			for (i=0;i<(*a).size;i++) {
				complex_a[i].real+=complex_b[i].real;
				complex_a[i].imag+=complex_b[i].imag;
			}
			
			/* convert to polar */
			
			c2p(polar_a, complex_a, (*a).size);
			c2p(polar_b, complex_b, (*b).size);
			
			break;
			
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

long esweep_sub(esweep_data *a, esweep_data *b) {
	Wave *wave_out, *wave_a, *wave_b;
	Complex *complex_a, *complex_b;
	Polar *polar_a, *polar_b;
	long i;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	if ((b==NULL) || ((*b).data==NULL) || ((*b).size<=0)) return ERR_EMPTY_CONTAINER;
	
	if (((*a).samplerate!=(*b).samplerate) || ((*a).type!=(*b).type)) return ERR_DIFF_MAPPING;
	
	switch ((*a).type) {
		case WAVE:
			wave_a=(Wave*) (*a).data;
			wave_b=(Wave*) (*b).data;
			if ((*a).size<(*b).size) {
				wave_out=(Wave*) calloc((*b).size, sizeof(Wave));
				for (i=0;i<(*a).size;i++) {
					wave_out[i]=wave_a[i]-wave_b[i];
				}
				for (;i<(*b).size;i++) {
					wave_out[i]=-wave_b[i];
				}
				free(wave_a);
				(*a).data=wave_out;
				(*a).size=(*b).size;
			} else {
				for (i=0;i<(*b).size;i++) {
					wave_a[i]-=wave_b[i];
				}
			}
			break;
		case COMPLEX:
			if ((*a).size!=(*b).size) return ERR_DIFF_MAPPING;
			
			complex_a=(Complex*) (*a).data;
			complex_b=(Complex*) (*b).data;
			
			for (i=0;i<(*a).size;i++) {
				complex_a[i].real-=complex_b[i].real;
				complex_a[i].imag-=complex_b[i].imag;
			}
			break;
		case POLAR:
			if ((*a).size!=(*b).size) return ERR_DIFF_MAPPING;
			
			polar_a=(Polar*) (*a).data;
			polar_b=(Polar*) (*b).data;
			complex_a=(Complex*) (*a).data;
			complex_b=(Complex*) (*b).data;
			
			/* convert to complex */
			
			p2c(complex_a, polar_a, (*a).size);
			p2c(complex_b, polar_b, (*b).size);
			
			/* complex subtraction */
			
			for (i=0;i<(*a).size;i++) {
				complex_a[i].real-=complex_b[i].real;
				complex_a[i].imag-=complex_b[i].imag;
			}
			
			/* convert to polar */
			
			c2p(polar_a, complex_a, (*a).size);
			c2p(polar_b, complex_b, (*b).size);
			
			break;
			
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

long esweep_mul(esweep_data *a, esweep_data *b) {
	Wave *wave_out, *wave_a, *wave_b;
	Complex *complex_a, *complex_b;
	Polar *polar_a, *polar_b;
	long i;
	double real, imag;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	if ((b==NULL) || ((*b).data==NULL) || ((*b).size<=0)) return ERR_EMPTY_CONTAINER;
	
	if (((*a).samplerate!=(*b).samplerate) || ((*a).type!=(*b).type)) return ERR_DIFF_MAPPING;
	
	switch ((*a).type) {
		case WAVE:
			wave_a=(Wave*) (*a).data;
			wave_b=(Wave*) (*b).data;
			if ((*a).size<(*b).size) {
				wave_out=(Wave*) calloc((*b).size, sizeof(Wave));
				for (i=0;i<(*a).size;i++) {
					wave_out[i]=wave_a[i]*wave_b[i];
				}
				free(wave_a);
				(*a).data=wave_out;
				(*a).size=(*b).size;
			} else {
				for (i=0;i<(*b).size;i++) {
					wave_a[i]*=wave_b[i];
				}
			}
			break;
		case COMPLEX:
			if ((*a).size!=(*b).size) return ERR_DIFF_MAPPING;
			
			complex_a=(Complex*) (*a).data;
			complex_b=(Complex*) (*b).data;
			
			for (i=0;i<(*a).size;i++) {
				real=complex_a[i].real;
				imag=complex_a[i].imag;
				complex_a[i].real=real*complex_b[i].real-imag*complex_b[i].imag;
				complex_a[i].imag=imag*complex_b[i].real+real*complex_b[i].imag;
			}
			break;
		case POLAR:
			if ((*a).size!=(*b).size) return ERR_DIFF_MAPPING;
			
			polar_a=(Polar*) (*a).data;
			polar_b=(Polar*) (*b).data;
			complex_a=(Complex*) (*a).data;
			complex_b=(Complex*) (*b).data;
			
			/* convert to complex */
			
			p2c(complex_a, polar_a, (*a).size);
			p2c(complex_b, polar_b, (*b).size);
			
			/* complex multiplication */
			
			for (i=0;i<(*a).size;i++) {
				real=complex_a[i].real;
				imag=complex_a[i].imag;
				complex_a[i].real=real*complex_b[i].real-imag*complex_b[i].imag;
				complex_a[i].imag=imag*complex_b[i].real+real*complex_b[i].imag;
			}
			
			/* convert to polar */
			
			c2p(polar_a, complex_a, (*a).size);
			c2p(polar_b, complex_b, (*b).size);
			
			break;
			
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

long esweep_div(esweep_data *a, esweep_data *b) {
	Complex *complex_a, *complex_b;
	Polar *polar_a, *polar_b;
	long i;
	double abs;
	double real, imag;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	if ((b==NULL) || ((*b).data==NULL) || ((*b).size<=0)) return ERR_EMPTY_CONTAINER;
	
	if (((*a).samplerate!=(*b).samplerate) || ((*a).type!=(*b).type)) return ERR_DIFF_MAPPING;
	
	switch ((*a).type) {
		case COMPLEX:
			if ((*a).size!=(*b).size) return ERR_DIFF_MAPPING;
			
			complex_a=(Complex*) (*a).data;
			complex_b=(Complex*) (*b).data;
			
			for (i=0;i<(*a).size;i++) {
				abs=complex_b[i].real*complex_b[i].real+complex_b[i].imag*complex_b[i].imag;
				real=complex_a[i].real;
				imag=complex_a[i].imag;
				complex_a[i].real=(real*complex_b[i].real+imag*complex_b[i].imag)/abs;
				complex_a[i].imag=(imag*complex_b[i].real-real*complex_b[i].imag)/abs;
			}
			break;
		case POLAR:
			if ((*a).size!=(*b).size) return ERR_DIFF_MAPPING;
			
			polar_a=(Polar*) (*a).data;
			polar_b=(Polar*) (*b).data;
			complex_a=(Complex*) (*a).data;
			complex_b=(Complex*) (*b).data;
			
			/* convert to complex */
			
			p2c(complex_a, polar_a, (*a).size);
			p2c(complex_b, polar_b, (*b).size);
			
			/* complex division */
			
			for (i=0;i<(*a).size;i++) {
				abs=complex_b[i].real*complex_b[i].real+complex_b[i].imag*complex_b[i].imag;
				real=complex_a[i].real;
				imag=complex_a[i].imag;
				complex_a[i].real=(real*complex_b[i].real+imag*complex_b[i].imag)/abs;
				complex_a[i].imag=(imag*complex_b[i].real-real*complex_b[i].imag)/abs;
			}
			
			/* convert to polar */
			
			c2p(polar_a, complex_a, (*a).size);
			c2p(polar_b, complex_b, (*b).size);
			
			break;
			
		case SURFACE:
		case AUDIO:
		case WAVE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

/* scalar math */

long esweep_addScalar(esweep_data *a, double re, double im) {
	Wave *wave;
	Complex *complex;
	Polar *polar;
	Surface *surface;
	long i, zsize;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			wave=(Wave*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				wave[i]+=re;
			}
			break;
		case COMPLEX:
			complex=(Complex*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				complex[i].real+=re;
				complex[i].imag+=im;
			}
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				polar[i].abs+=re;
				polar[i].arg+=im;
			}
			break;
		case SURFACE:
			surface=(Surface*) (*a).data;
			zsize=(*surface).xsize*(*surface).ysize;
			for (i=0;i<zsize;i++) (*surface).z[i]+=re;
			break;
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

long esweep_subScalar(esweep_data *a, double re, double im) {
	Wave *wave;
	Complex *complex;
	Polar *polar;
	Surface *surface;
	long i, zsize;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			wave=(Wave*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				wave[i]-=re;
			}
			break;
		case COMPLEX:
			complex=(Complex*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				complex[i].real-=re;
				complex[i].imag-=im;
			}
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				polar[i].abs-=re;
				polar[i].arg-=im;
			}
			break;
		case SURFACE:
			surface=(Surface*) (*a).data;
			zsize=(*surface).xsize*(*surface).ysize;
			for (i=0;i<zsize;i++) (*surface).z[i]-=re;
			break;
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

long esweep_mulScalar(esweep_data *a, double re, double im) {
	Audio *audio;
	Wave *wave;
	Complex *complex;
	Polar *polar;
	Surface *surface;
	long i, zsize;
	double real, imag;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case AUDIO:
			audio=(Audio*) (*a).data;
			for (i=0;i<2*(*a).size;i+=2) {
				audio[i]*=re;
				audio[i+1]*=re;
			}
			break;
		case WAVE:
			wave=(Wave*) (*a).data;
			for (i=0;i<(*a).size;i++) wave[i]*=re;
			break;
		case COMPLEX:
			complex=(Complex*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				real=complex[i].real;
				imag=complex[i].imag;
				complex[i].real=real*re-imag*im;
				complex[i].imag=real*im+imag*re;
			}
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			for (i=0;i<(*a).size;i++) polar[i].abs*=re;
			break;
		case SURFACE:
			surface=(Surface*) (*a).data;
			zsize=(*surface).xsize*(*surface).ysize;
			for (i=0;i<zsize;i++) (*surface).z[i]*=re;
			break;
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

long esweep_divScalar(esweep_data *a, double re, double im) {
	Audio *audio;
	Wave *wave;
	Complex *complex;
	Polar *polar;
	Surface *surface;
	long i, zsize;
	double real, imag, abs;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case AUDIO:
			audio=(Audio*) (*a).data;
			for (i=0;i<2*(*a).size;i+=2) {
				audio[i]/=re;
				audio[i+1]/=re;
			}
			break;
		case WAVE:
			wave=(Wave*) (*a).data;
			for (i=0;i<(*a).size;i++) wave[i]/=re;
			break;
		case COMPLEX:
			complex=(Complex*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				real=complex[i].real;
				imag=complex[i].imag;
				abs=re*re+im*im;
				complex[i].real=(real*re+imag*im)/abs;
				complex[i].imag=(real*im-imag*re)/abs;
			}
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			for (i=0;i<(*a).size;i++) polar[i].abs/=re;
			break;
		case SURFACE:
			surface=(Surface*) (*a).data;
			zsize=(*surface).xsize*(*surface).ysize;
			for (i=0;i<zsize;i++) (*surface).z[i]/=re;
			break;
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

/* power, exponentiation, logarithm */

long esweep_ln(esweep_data *a) { /* natural logarithm */
	Wave *wave;
	Polar *polar;
	Surface *surface;
	long i, zsize;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			wave=(Wave*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				wave[i]=log(fabs(wave[i]));
			}
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				polar[i].abs=log(polar[i].abs);
			}
			break;
		case SURFACE:
			surface=(Surface*) (*a).data;
			zsize=(*surface).xsize*(*surface).ysize;
			for (i=0;i<zsize;i++) (*surface).z[i]=log((*surface).z[i]);
			break;
		case COMPLEX:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

long esweep_lg(esweep_data *a) { /* logarithm to base 10 */
	Wave *wave;
	Polar *polar;
	Surface *surface;
	long i, zsize;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			wave=(Wave*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				wave[i]=log10(fabs(wave[i]));
			}
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				polar[i].abs=log10(polar[i].abs);
			}
			break;
		case SURFACE:
			surface=(Surface*) (*a).data;
			zsize=(*surface).xsize*(*surface).ysize;
			for (i=0;i<zsize;i++) (*surface).z[i]=log10((*surface).z[i]);
			break;
		case COMPLEX:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

long esweep_exp(double x, esweep_data *a) { /* x^a */
	Wave *wave;
	Polar *polar;
	Surface *surface;
	long i, zsize;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			wave=(Wave*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				wave[i]=pow(x, wave[i]);
			}
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				polar[i].abs=pow(x, polar[i].abs);
			}
			break;
		case SURFACE:
			surface=(Surface*) (*a).data;
			zsize=(*surface).xsize*(*surface).ysize;
			for (i=0;i<zsize;i++) (*surface).z[i]=pow(x, (*surface).z[i]);
			break;
		case COMPLEX:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

long esweep_pow(esweep_data *a, double x) { /* a^x */
	Wave *wave;
	Polar *polar;
	Surface *surface;
	long i, zsize;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			wave=(Wave*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				wave[i]=pow(wave[i], x);
			}
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			for (i=0;i<(*a).size;i++) {
				polar[i].abs=pow(polar[i].abs, x);
			}
			break;
		case SURFACE:
			surface=(Surface*) (*a).data;
			zsize=(*surface).xsize*(*surface).ysize;
			for (i=0;i<zsize;i++) (*surface).z[i]=pow((*surface).z[i], x);
			break;
		case COMPLEX:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	return ERR_OK;
}

/* 
calculates the schroeder integral of a WAVE 
this is not the real schroeder equation, but the backward integral
to get the energy decay curve you have to square the impulse response with
esweep_pow(a, 2)
*/

long esweep_schroeder(esweep_data *a) {
	Wave *wave, *wave_out;
	long i, j;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	if ((*a).type!=WAVE) return ERR_NOT_ON_THIS_TYPE;
	
	wave=(Wave*) (*a).data;
	wave_out=(Wave*) calloc((*a).size, sizeof(Wave));
	
	for (i=0;i<(*a).size;i++) {
		for (j=i;j<(*a).size;j++) {
			wave_out[i]+=wave[j];
		}
	}
	
	free((*a).data);
	(*a).data=wave_out;
	
	return ERR_OK;
}
