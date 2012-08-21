/*
 * Copyright (c) 2007, 2010 Jochen Fabricius <jfab@berlios.de>
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

/* 
 * Type conversion
 */

/* 
 * src/esweep_conv.c: 
 * 11.01.2010, jfab: 	adaptation to the new data structures and types
 * 28.12.2010, jfab:	moved internal conversion functions to esweep_priv.c
 */

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esweep_priv.h"

int esweep_toWave(esweep_object *obj) { /* TEST: OK */
	Complex *cpx;
	Wave *wave;
	Polar *polar;
	
	ESWEEP_OBJ_ISVALID(obj, ERR_OBJ_NOT_VALID); 
	ESWEEP_ASSERT(obj->type != SURFACE, ERR_NOT_ON_THIS_TYPE);
	
	if (obj->size==0) { /* no need for conversion, simply change type */
		obj->type=WAVE; 
		return ERR_OK; 
	}
	if (obj->type==WAVE) { /* no need for conversion */
		return ERR_OK; 
	}

	ESWEEP_MALLOC(wave, obj->size, sizeof(Wave), ERR_MALLOC);

	/* Convert data */
	switch (obj->type) {
		case COMPLEX:
			cpx=(Complex*) obj->data;
			c2r(wave, cpx, obj->size);
			free(cpx);
			obj->data=wave;
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			abs2r(wave, polar, obj->size);
			free(polar);
			obj->data=wave;
			break;
		default:
			free(wave); 
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	
	obj->type=WAVE;
	
	return ERR_OK;
}


/* convert a to type COMPLEX 
WAVE: the values are copied in the real part
POLAR: calculate the standard Euler identity
*/
int esweep_toComplex(esweep_object *obj) { /* TEST: OK */
	Complex *cpx;
	Wave *wave;
	Polar *polar;
	
	ESWEEP_OBJ_ISVALID(obj, ERR_OBJ_NOT_VALID); 
	ESWEEP_ASSERT(obj->type != SURFACE, ERR_NOT_ON_THIS_TYPE);
	
	if (obj->size==0) { /* no need for conversion, simply change type */
		obj->type=COMPLEX; 
		return ERR_OK; 
	}
	if (obj->type==COMPLEX) { /* no need for conversion */
		return ERR_OK; 
	}

	/* Convert data */
	switch (obj->type) {
		case WAVE:
			ESWEEP_MALLOC(cpx, obj->size, sizeof(Complex), ERR_MALLOC);
			wave=(Wave*) obj->data;
			cpx=(Complex*) calloc(obj->size, sizeof(Complex)); 
			r2c(cpx, wave, obj->size);
			free(wave);
			obj->data=cpx;
			break;
		case POLAR:
			/* Polar and Complex data types have the same internal shape, 
			 * we can convert in-place */
			polar=(Polar*) obj->data;
			cpx=(Complex*) obj->data; 
			p2c(cpx, polar, obj->size);
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
			return ERR_NOT_ON_THIS_TYPE;
	}
	
	obj->type=COMPLEX;
	
	return ERR_OK;
}

/* convert a to type POLAR
COMPLEX: calculate the standard Euler identity
*/

int esweep_toPolar(esweep_object *obj) { /* TEST: OK */
	Complex *cpx;
	Polar *polar;
	Wave *wave; 
	
	ESWEEP_OBJ_ISVALID(obj, ERR_OBJ_NOT_VALID); 
	ESWEEP_ASSERT(obj->type != SURFACE, ERR_NOT_ON_THIS_TYPE);

	if (obj->size==0) { /* no need for conversion, simply change type */
		obj->type=POLAR; 
		return ERR_OK; 
	}
	if (obj->type==POLAR) { /* no need for conversion */
		return ERR_OK; 
	}

	switch (obj->type) {
		case COMPLEX:
			/* Polar and Complex data types have the same internal shape, 
			 * we can convert in-place */
			cpx=(Complex*) obj->data;
			polar=(Polar*) obj->data;
			c2p(polar, cpx, obj->size); 
			break;
		case WAVE:
			ESWEEP_MALLOC(polar, obj->size, sizeof(Complex), ERR_MALLOC);
			wave=(Wave*) obj->data; 
			r2p(polar, wave, obj->size);
			free(wave);
			obj->data=polar; 
			break; 
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
			return ERR_NOT_ON_THIS_TYPE;
	}

	obj->type=POLAR;
	return ERR_OK;
}

int esweep_switchRI(esweep_object *obj) { /* TEST: OK */
	Complex *cpx;
	Polar *polar;
	Real tmp; 
	int i; 
	
	ESWEEP_OBJ_ISVALID(obj, ERR_OBJ_NOT_VALID); 
	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT); 
	
	/* Convert data */
	switch (obj->type) {
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=0; i < obj->size; i++) polar[i].arg=M_PI_2-polar[i].arg; 
			break;
		case COMPLEX: 
			cpx=(Complex*) obj->data; 
			for (i=0; i < obj->size; i++) {
				tmp=cpx[i].real; 
				cpx[i].real=cpx[i].imag; 
				cpx[i].imag=tmp;
			}
			break; 
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	
	obj->type=COMPLEX;
	
	return ERR_OK;
}

