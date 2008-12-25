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

/* return the type of a */

const char *esweep_type(esweep_data *a) {
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return "";
	
	switch ((*a).type) {
		case WAVE: return "wave";
		case POLAR: return "polar";
		case COMPLEX: return "complex";
		case SURFACE: return "surface";
		case AUDIO: return "audio";
		default: return NULL;
	}
}

/* return the samplerate of a */

long esweep_samplerate(esweep_data *a) {
	
	if (a==NULL) return ERR_EMPTY_CONTAINER;
	
	return (*a).samplerate;
}

/* set the samplerate, may be useful, especially with the farina method */

long esweep_setSamplerate(esweep_data *a, long samplerate) {
	if (a==NULL) return ERR_EMPTY_CONTAINER;
	
	if (samplerate <=0) return ERR_BAD_ARGUMENT;
	
	(*a).samplerate=samplerate;
	
	return ERR_OK;
}

/* return the size of a */

long esweep_size(esweep_data *a) {
	
	if (a==NULL) return ERR_EMPTY_CONTAINER;
	
	return (*a).size;
}

/* return the index of the maximum of a */

long esweep_maxPos(esweep_data *a) {
	Wave *wave;
	Polar *polar;
	long i;
	long max;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0) || ((*a).samplerate<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			wave=(Wave*) (*a).data;
			max=0;
			for (i=1;i<(*a).size;i++) if (wave[max]<wave[i]) max=i;
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			max=0;
			for (i=1;i<(*a).size;i++) if (polar[max].abs<polar[i].abs) max=i;
			break;
		case COMPLEX:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	return max;
}

/* return the x-coordinate at the the index i */

double esweep_getX(esweep_data *a, long i) {
	Surface *surface;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0) || ((*a).samplerate<=0)) return 0.0;
	
	switch ((*a).type) {
		case WAVE:
		case AUDIO:
			if ((i<0) || (i>=(*a).size)) return 0.0;
			else return 1000.0*i/(*a).samplerate;
			break;
		case COMPLEX:
		case POLAR:
			if ((i<0) || (i>=(*a).size)) return 0.0;
			else return (double) i*(*a).samplerate/(*a).size;
			break;
		case SURFACE:
			surface=(Surface*) (*a).data;
			if ((i<0) || (i>=(*surface).xsize)) return 0.0;
			else return (*surface).x[i];
			break;
		default:
			return 0.0;
	}
}

/* 
AUDIO/WAVE/SURFACE: return the y-coordinate at the the index i 
COMPLEX: return the real part at index i
POLAR: return the magnitude at index i
*/

double esweep_getY(esweep_data *a, long i) {
	Audio *audio;
	Wave *wave;
	Complex *complex;
	Polar *polar;
	Surface *surface;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0) || ((*a).samplerate<=0)) return 0.0;
	
	switch ((*a).type) {
		case AUDIO:
			audio=(Audio*) (*a).data;
			if ((i<0) || (i>=(*a).size)) return 0.0;
			else return (double) audio[i]+audio[i+1];
			break;
		case WAVE:
			wave=(Wave*) (*a).data;
			if ((i<0) || (i>=(*a).size)) return 0.0;
			else return wave[i];
			break;
		case COMPLEX:
			complex=(Complex*) (*a).data;
			if ((i<0) || (i>=(*a).size)) return 0.0;
			else return complex[i].real;
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			if ((i<0) || (i>=(*a).size)) return 0.0;
			else return polar[i].abs;
			break;
		case SURFACE:
			surface=(Surface*) (*a).data;
			if ((i<0) || (i>=(*surface).ysize)) return 0.0;
			else return (*surface).y[i];
			break;
		default:
			return 0.0;
	}
}

/* 
AUDIO/WAVE: not available 
COMPLEX: return the imaginary part at index x
POLAR: return the argument at index x
SURFACE: return the z-coordinate at the index pair x/y
*/

double esweep_getZ(esweep_data *a, long x, long y) {
	Complex *complex;
	Polar *polar;
	Surface *surface;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0) || ((*a).samplerate<=0)) return 0.0;
	
	switch ((*a).type) {
		case COMPLEX:
			complex=(Complex*) (*a).data;
			if ((x<0) || (x>=(*a).size)) return 0.0;
			else return complex[x].imag;
			break;
		case POLAR:
			polar=(Polar*) (*a).data;
			if ((x<0) || (x>=(*a).size)) return 0.0;
			else return polar[x].arg;
			break;
		case SURFACE:
			surface=(Surface*) (*a).data;
			if ((x<0) || (x>=(*surface).xsize) || (y<0) || (y>=(*surface).ysize)) return 0.0;
			else return (*surface).y[y+x*(*surface).ysize];
			break;
		case WAVE:
		case AUDIO:
		default:
			return 0.0;
	}
}
