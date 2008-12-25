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

/* convert a to type COMPLEX 
WAVE: the values are copied in the real part
POLAR: calculate the standard Euler identity
AUDIO/SURFACE: not available
*/


long esweep_toComplex(esweep_data *a) {
	Complex *complex;
	Wave *wave;
	Polar *polar;
	long size;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	
	switch ((*a).type) {
		case WAVE:
			size=(long) (pow(2, ceil(log((*a).size)/log(2)))+0.5);
			wave=(Wave*) (*a).data;
			complex=(Complex*) calloc(size, sizeof(Complex)); /* zero padding */
			r2c(complex, wave, (*a).size);
			free(wave);
			(*a).data=complex;
			break;
		case POLAR:
			size=(*a).size;
			polar=(Polar*) (*a).data;
			complex=(Complex*) (*a).data; 
			p2c(complex, polar, (*a).size);
			free(polar);
			(*a).data=complex;
			break;
		case COMPLEX:
		case SURFACE:
		case AUDIO:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	(*a).size=size;
	(*a).type=COMPLEX;
	
	return ERR_OK;
}

/* convert a to type POLAR
COMPLEX: calculate the standard Euler identity
AUDIO/WAVE/SURFACE: not available
*/

long esweep_toPolar(esweep_data *a) {
	Complex *complex;
	Polar *polar;
	
	if ((a==NULL) || ((*a).data==NULL) || ((*a).size<=0)) return ERR_EMPTY_CONTAINER;
	switch ((*a).type) {
		case COMPLEX:
			complex=(Complex*) (*a).data;
			polar=(Polar*) (*a).data;
			c2p(polar, complex, (*a).size); /* complex2polar */
			(*a).type=POLAR;
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
