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

esweep_data *esweep_create(const char *type, long samplerate, long size) {
	long sample_size=0;
	esweep_data *a;
	
	if (samplerate<=0) return NULL;
	if (size<0) return NULL;
	
	a=(esweep_data*) calloc(1, sizeof(esweep_data));
	(*a).samplerate=samplerate;
	
	(*a).type=-1;
	if (strcmp(type, "audio")==0) (*a).type=AUDIO;
	if (strcmp(type, "wave")==0) (*a).type=WAVE;
	if (strcmp(type, "polar")==0) (*a).type=POLAR;
	if (strcmp(type, "complex")==0) (*a).type=COMPLEX;
	if (strcmp(type, "surface")==0) (*a).type=SURFACE;
	
	/* special treatment for type SURFACE */
	
	if ((*a).type==SURFACE) {
		(*a).size=1;
		(*a).data=NULL;
		return a;
	}
	
	if (size>0) {
		switch ((*a).type) {
			case AUDIO:
				(*a).size=2*size; /* double size for allocating memory */
				sample_size=sizeof(Audio);
				break;
			case WAVE:
				(*a).size=size;
				sample_size=sizeof(Wave);
				break;
			case COMPLEX:
				(*a).size=(long) (pow(2, ceil(log(size)/log(2)))+0.5);
				sample_size=sizeof(Complex);
				break;
			case POLAR:
				(*a).size=(long) (pow(2, ceil(log(size)/log(2)))+0.5);
				sample_size=sizeof(Polar);
				break;
			case -1:
			default:
				free(a);
				return NULL;
				break;
		}
		if (sample_size>0) (*a).data=(void*) calloc((*a).size, sample_size);
		if ((*a).type==AUDIO) (*a).size/=2; /* set size back to correct value */
	} else {
		if ((*a).type==-1) {
			free(a);
			return NULL;
		} else {
			(*a).size=0;
			(*a).data=NULL;
		}
	}
	
	return a;
}

esweep_data *esweep_free(esweep_data *a) {
	if (a==NULL) return NULL;
	
	Surface *surface;
	if ((*a).data!=NULL) {
		if ((*a).type==SURFACE) {
			surface=(Surface*) (*a).data;
			free((*surface).x);
			free((*surface).y);
			free((*surface).z);
		} 
		free((*a).data);
	}
	(*a).type=UNKNOWN;
	(*a).size=0;
	(*a).samplerate=0;
	(*a).data=NULL;
	free(a);
	return NULL;
}

esweep_data *esweep_copy(esweep_data *src) {
	esweep_data *dst;
	Audio *audio;
	Wave *wave;
	Polar *polar;
	Complex *complex;
	Surface *surface;
	Surface *src_surface;
	
	if (src==NULL) return NULL;
	
	if ((dst=(esweep_data*) calloc(1, sizeof(esweep_data)))==NULL) return NULL;
	(*dst).samplerate=(*src).samplerate;
	(*dst).size=(*src).size;
	
	switch ((*src).type) {
		case AUDIO:
			if ((audio=(Audio*) calloc(2*(*src).size, sizeof(Audio)))==NULL) {
				free(dst);
				return NULL;
			}
			memcpy(audio, (*src).data, 2*(*src).size*sizeof(Audio));
			(*dst).data=audio;
			(*dst).type=AUDIO;
			break;
		case WAVE:
			if ((wave=(Wave*) calloc((*src).size, sizeof(Wave)))==NULL) {
				free(dst);
				return NULL;
			}
			memcpy(wave, (*src).data, (*src).size*sizeof(Wave));
			(*dst).data=wave;
			(*dst).type=WAVE;
			break;
		case POLAR:
			if ((polar=(Polar*) calloc((*src).size, sizeof(Polar)))==NULL) {
				free(dst);
				return NULL;
			}
			memcpy(polar, (*src).data, (*src).size*sizeof(Polar));
			(*dst).data=polar;
			(*dst).type=POLAR;
			break;
		case COMPLEX:
			if ((complex=(Complex*) calloc((*src).size, sizeof(Complex)))==NULL) {
				free(dst);
				return NULL;
			}
			memcpy(complex, (*src).data, (*src).size*sizeof(Complex));
			(*dst).data=complex;
			(*dst).type=COMPLEX;
			break;
		case SURFACE:
			/* allocate surface structure */
			src_surface=(Surface*) (*src).data;
			if ((surface=(Surface*) calloc(1, sizeof(Surface)))==NULL) {
				free(dst);
				return NULL;
			}
			/* allocate x-vector */
			if (((*surface).x=(double*) calloc((*src_surface).xsize, sizeof(double)))==NULL) {
				free(dst);
				free(surface);
				return NULL;
			}
			(*surface).xsize=(*src_surface).xsize;
			/* allocate y-vector */
			if (((*surface).y=(double*) calloc((*src_surface).ysize, sizeof(double)))==NULL) {
				free(dst);
				free((*surface).x);
				free(surface);
				return NULL;
			}
			(*surface).ysize=(*src_surface).ysize;
			/* allocate z-vector */
			if (((*surface).z=(double*) calloc((*src_surface).xsize*(*src_surface).ysize, sizeof(double)))==NULL) {
				free(dst);
				free((*surface).x);
				free((*surface).y);
				free(surface);
				return NULL;
			}
			
			/* copy vectors */
		default:
			return NULL;
	}
	
	return dst;
}
