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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esweep.h"

/*
 * src/esweep.c:
 * Object information and manipulation
 *
 * 28.12.2010, jfab: PRE-FREEZE, TEST OK
 * 28.12.2010, jfab: changed macro names, TEST OK
*/

/* internal functions */
static esweep_object* esweep_getSurface(const esweep_object *obj, int x, int y);

/* return the type of obj as a string */
int esweep_type(const esweep_object *obj, const char *type[]) { /* TEST: OK */
	ESWEEP_OBJ_ISVALID(obj, ERR_OBJ_NOT_VALID);

	switch (obj->type) {
		case WAVE: *type="wave"; break; 
		case POLAR: *type="polar"; break; 
		case COMPLEX: *type="complex"; break; 
		case SURFACE: *type="surface"; break; 
		default:
			*type=NULL; 
			return ERR_UNKNOWN; 
	}
	return ERR_OK; 
}

/* return the samplerate of a */
int esweep_samplerate(const esweep_object *obj, int *samplerate) { /* TEST: OK */ 
	ESWEEP_OBJ_ISVALID(obj, ERR_OBJ_NOT_VALID);
	ESWEEP_ASSERT(samplerate != NULL, ERR_BAD_ARGUMENT); 

	*samplerate=obj->samplerate; 
	return ERR_OK;
}

/* set the samplerate */
int esweep_setSamplerate(esweep_object *obj, int samplerate) { /* TEST: OK */
	ESWEEP_OBJ_ISVALID(obj, ERR_OBJ_NOT_VALID);
	ESWEEP_ASSERT(samplerate > 0, ERR_BAD_ARGUMENT);
	obj->samplerate=samplerate;

	return ERR_OK;
}

/* return the size of a */
int esweep_size(const esweep_object *obj, int *size) { /* TEST: OK */
	ESWEEP_OBJ_ISVALID(obj, ERR_OBJ_NOT_VALID);
	ESWEEP_ASSERT(size != NULL, ERR_BAD_ARGUMENT); 
	*size=obj->size; 
	return ERR_OK;
}

/* get the size of a surface; it will be stored on *x and *y */
int esweep_surfaceSize(const esweep_object *obj, int *x, int *y) { /* TEST: OK */
	Surface *surf;
	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	surf=(Surface*) obj->data;
	ESWEEP_OBJ_ISVALID_SURFACE(obj, surf, ERR_NOT_ON_THIS_TYPE, ERR_OBJ_NOT_VALID);
	ESWEEP_ASSERT(x != NULL, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(y != NULL, ERR_BAD_ARGUMENT); 

	surf=(Surface*) obj->data;
	*x=surf->xsize;
	*y=surf->ysize;

	return ERR_OK;
}

/* set the size*/
int esweep_setSize(esweep_object *obj, int size) { /* TEST: OK */
	char *new_data;
	size_t samplesize; 

	ESWEEP_OBJ_ISVALID(obj, ERR_OBJ_NOT_VALID);
	ESWEEP_ASSERT(size >= 0, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(obj->type != SURFACE, ERR_NOT_ON_THIS_TYPE);

	if (size == 0) {
		free(obj->data);
		obj->data=NULL;
		obj->size=0;
		return ERR_OK;
	} else {
		switch (obj->type) {
			case WAVE:
				samplesize=sizeof(Wave); 
				break;
			case COMPLEX:
				samplesize=sizeof(Complex); 
				break;
			case POLAR:
				samplesize=sizeof(Polar); 
				break;
			default:
				ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
		}
		ESWEEP_MALLOC(new_data, size, samplesize, ERR_MALLOC);
		if (obj->data!=NULL) {
			if (size > obj->size) {
				memcpy(new_data, obj->data, obj->size*samplesize);
			} else {
				memcpy(new_data, obj->data, size*samplesize);
			}
			free(obj->data);
		}
		obj->data=new_data;
		obj->size=size;
	}

	return ERR_OK;
}

/* Splits a WAVE-object into time and level */
int esweep_unbuildWave(const esweep_object *obj, Real *t[], Real *a[]) { /* TEST: OK */
	int i;
	Wave *wave;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	ESWEEP_ASSERT(obj->type == WAVE, ERR_NOT_ON_THIS_TYPE);

	ESWEEP_ASSERT(t != NULL, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(a != NULL, ERR_BAD_ARGUMENT); 
	free(*t);
	free(*a);
	ESWEEP_MALLOC(*t, obj->size, sizeof(Real), ERR_MALLOC);
	ESWEEP_MALLOC(*a, obj->size, sizeof(Real), ERR_MALLOC);

	wave=(Wave*) obj->data;
	for (i=0; i<obj->size; i++) {
		(*t)[i]=(Real) i/obj->samplerate;
		(*a)[i]=wave[i];
	}

	return ERR_OK;
}

/* Build a WAVE-object from real data */
int esweep_buildWave(esweep_object *obj, const Real wave[], int size) { /* TEST: OK */
	Wave *a;

	ESWEEP_OBJ_ISVALID(obj, ERR_OBJ_NOT_VALID);
	ESWEEP_ASSERT(obj->type == WAVE, ERR_NOT_ON_THIS_TYPE); 
	ESWEEP_ASSERT(wave != NULL, ERR_BAD_ARGUMENT); 

	ESWEEP_ASSERT(size > 0, ERR_BAD_ARGUMENT);

	ESWEEP_MALLOC(a, size, sizeof(Wave), ERR_MALLOC);

	memcpy(a, wave, size*sizeof(Wave));
	free(obj->data);
	obj->data=a;
	obj->size=size; 

	return ERR_OK;
}

/* Splits a COMPLEX-object into real and imaginary part */
int esweep_unbuildComplex(const esweep_object *obj, Real *real[], Real *imag[]) { /* TEST: OK */
	int i;
	Complex *cpx;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	ESWEEP_ASSERT(obj->type == COMPLEX, ERR_NOT_ON_THIS_TYPE);

	ESWEEP_ASSERT(real != NULL, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(imag != NULL, ERR_BAD_ARGUMENT); 
	free(*real);
	free(*imag);
	ESWEEP_MALLOC(*real, obj->size, sizeof(Complex), ERR_MALLOC);
	ESWEEP_MALLOC(*imag, obj->size, sizeof(Complex), ERR_MALLOC);

	cpx=(Complex*) obj->data;
	for (i=0; i<obj->size; i++) {
		(*real)[i]=cpx[i].real;
		(*imag)[i]=cpx[i].imag;
	}

	return ERR_OK;
}

/* Build a Complex-object from real data */
int esweep_buildComplex(esweep_object *obj, const Real real[], const Real imag[], int size) { /* TEST: OK */
	int i;
	Complex *a; 

	ESWEEP_OBJ_ISVALID(obj, ERR_OBJ_NOT_VALID);
	ESWEEP_ASSERT(obj->type == COMPLEX, ERR_NOT_ON_THIS_TYPE); 
	ESWEEP_ASSERT(real != NULL || imag != NULL, ERR_BAD_ARGUMENT); 

	ESWEEP_ASSERT(size > 0, ERR_BAD_ARGUMENT);

	ESWEEP_MALLOC(a, size, sizeof(Complex), ERR_MALLOC);

	if (real != NULL && imag != NULL) {
		for (i=0; i < size; i++) {
			a[i].real=real[i];
			a[i].imag=imag[i];
		}
	} else if (real != NULL && imag == NULL) { /* real != NULL */
		for (i=0; i < size; i++) {
			a[i].real=real[i];
		}
	} else if (real == NULL && imag != NULL) { /* imag != NULL */
		for (i=0; i < size; i++) {
			a[i].imag=imag[i];
		}
	}

	free(obj->data);
	obj->data=a;
	obj->size=size; 

	return ERR_OK;
}


/* Splits a POLAR-object into absolute and argument */
int esweep_unbuildPolar(const esweep_object *obj, Real *abs[], Real *arg[]) { /* TEST: OK */
	int i;
	Polar *polar;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	ESWEEP_ASSERT(obj->type == POLAR, ERR_NOT_ON_THIS_TYPE);

	ESWEEP_ASSERT(abs != NULL, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(arg != NULL, ERR_BAD_ARGUMENT); 
	free(*abs);
	free(*arg);
	ESWEEP_MALLOC(*abs, obj->size, sizeof(Complex), ERR_MALLOC);
	ESWEEP_MALLOC(*arg, obj->size, sizeof(Complex), ERR_MALLOC);

	polar=(Polar*) obj->data;
	for (i=0; i<obj->size; i++) {
		(*abs)[i]=polar[i].abs;
		(*arg)[i]=polar[i].arg;
	}

	return ERR_OK;
}

/* Build a Polar-object from real data */
int esweep_buildPolar(esweep_object *obj, const Real abs[], const Real arg[], int size) { /* TEST: OK */
	int i;
	Polar *a; 

	ESWEEP_OBJ_ISVALID(obj, ERR_OBJ_NOT_VALID);
	ESWEEP_ASSERT(obj->type == POLAR, ERR_NOT_ON_THIS_TYPE); 
	ESWEEP_ASSERT(abs != NULL || arg != NULL, ERR_BAD_ARGUMENT); 

	ESWEEP_ASSERT(size > 0, ERR_BAD_ARGUMENT);

	ESWEEP_MALLOC(a, size, sizeof(Polar), ERR_MALLOC);

	if (abs != NULL && arg != NULL) {
		for (i=0; i < size; i++) {
			a[i].abs=abs[i];
			a[i].arg=arg[i];
		}
	} else if (abs != NULL && arg == NULL) { /* abs != NULL */
		for (i=0; i < size; i++) {
			a[i].abs=abs[i];
		}
	} else if (abs == NULL && arg != NULL) { /* arg != NULL */
		for (i=0; i < size; i++) {
			a[i].arg=arg[i];
		}
	}

	free(obj->data);
	obj->data=a;
	obj->size=size; 

	return ERR_OK;
}

/* Splits a SURFACE-object into its coordinates */
int esweep_unbuildSurface(const esweep_object *obj, Real *x[], Real *y[], Real *z[]) { /* TEST: OK */
	int zsize;
	Surface *surf;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	surf=(Surface*) obj->data;
	ESWEEP_OBJ_ISVALID_SURFACE(obj, surf, ERR_NOT_ON_THIS_TYPE, ERR_OBJ_NOT_VALID);

	ESWEEP_ASSERT(x != NULL, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(y != NULL, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(z != NULL, ERR_BAD_ARGUMENT); 
	free(*x);
	free(*y);
	free(*z);

	zsize=surf->xsize*surf->ysize;

	ESWEEP_MALLOC_SURFACE(*x, *y, *z, surf->xsize, surf->ysize, zsize, ERR_MALLOC);

	memcpy(*x, surf->x, surf->xsize*sizeof(Real));
	memcpy(*y, surf->y, surf->ysize*sizeof(Real));
	memcpy(*z, surf->z, zsize*sizeof(Real));

	return ERR_OK;
}

/* Builds a Surface-object from real data */
int esweep_buildSurface(esweep_object *obj, 
			const Real x[], 
			const Real y[], 
			const Real z[], 
			int xsize, 
			int ysize) { /* TEST: OK */
	int zsize;
	Surface *surf;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	surf=(Surface*) obj->data;
	ESWEEP_OBJ_ISVALID_SURFACE(obj, surf, ERR_NOT_ON_THIS_TYPE, ERR_OBJ_NOT_VALID);

	ESWEEP_ASSERT(xsize > 0, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(ysize > 0, ERR_BAD_ARGUMENT);

	zsize=xsize*ysize;

	free(surf->x);
	free(surf->y);
	free(surf->z);

	ESWEEP_MALLOC_SURFACE(surf->x, surf->y, surf->z, xsize, ysize, zsize, ERR_MALLOC);

	if (x != NULL) memcpy(surf->x, x, xsize*sizeof(Real));
	if (y != NULL) memcpy(surf->y, y, ysize*sizeof(Real));
	if (z != NULL) memcpy(surf->z, z, zsize*sizeof(Real));

	surf->xsize=xsize;
	surf->ysize=ysize;

	return ERR_OK;
}

/*
returns an object constructed from index m to index n, inclusive
*/

esweep_object* esweep_get(const esweep_object *obj, int m, int n) { /* TEST: OK */
	Wave *wave;
	Complex *cpx;
	Polar *polar;
	esweep_object *ret;

	ESWEEP_OBJ_NOTEMPTY(obj, NULL);

	/* if the object is a surface, use the special function for this */
	if (obj->type == SURFACE) {
	       return esweep_getSurface(obj, m, n); 
	}

	/* limits */
	ESWEEP_INDEX_INRANGE(obj, m, NULL);
	ESWEEP_ASSERT(n >= m && n < obj->size, NULL);

	ESWEEP_MALLOC(ret, 1, sizeof(esweep_object), NULL);
	ret->type=obj->type;
	ret->size=n-m+1;
	ret->samplerate=obj->samplerate;
	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			ESWEEP_MALLOC(ret->data, ret->size, sizeof(Wave), NULL);
			memcpy(ret->data, &(wave[m]), ret->size*sizeof(Wave)); 
			break;
		case COMPLEX:
			cpx=(Complex*) obj->data;
			ESWEEP_MALLOC(ret->data, ret->size, sizeof(Complex), NULL);
			memcpy(ret->data, &(cpx[m]), ret->size*sizeof(Complex)); 
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			ESWEEP_MALLOC(ret->data, ret->size, sizeof(Polar), NULL);
			memcpy(ret->data, &(polar[m]), ret->size*sizeof(Polar)); 
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, NULL);
	}
	return ret;
}

static esweep_object* esweep_getSurface(const esweep_object *obj, int x, int y) { /* TEST: OK */
	Surface *surf, *surf_ret; 
	esweep_object *ret;
	Real *px, *py, *pz; 
	int i, xsize, ysize; 

	ESWEEP_OBJ_NOTEMPTY(obj, NULL);
	surf=(Surface*) obj->data;
	ESWEEP_OBJ_ISVALID_SURFACE(obj, surf, NULL, NULL);

	ESWEEP_ASSERT(x < surf->xsize, NULL);
	ESWEEP_ASSERT(y < surf->ysize, NULL);
	ESWEEP_ASSERT(x >= 0 || y >= 0, NULL);

	if (x < 0) {
		xsize=surf->xsize;
		ysize=1;
	} else if (y < 0) {
		ysize=surf->ysize;
		xsize=1;
	} else {
		xsize=1;
		ysize=1;
	}
	/* allocate surface structure */
	ESWEEP_MALLOC_SURFACE(px, py, pz, xsize, ysize, xsize*ysize, NULL);

	/* create output object */
	ESWEEP_MALLOC(ret, 1, sizeof(esweep_object), NULL);
	ESWEEP_MALLOC(ret->data, 1, sizeof(Surface), NULL);
	ret->type=SURFACE;
	ret->size=1;
	ret->samplerate=obj->samplerate;
	surf_ret=(Surface*) ret->data; 
	surf_ret->xsize=xsize; 
	surf_ret->ysize=ysize; 
	surf_ret->x=px; 
	surf_ret->y=py; 
	surf_ret->z=pz; 

	if (x < 0) {
		for (i=0;i<surf->xsize;i++) {
			px[i]=surf->x[i];
			pz[i]=surf->z[y*surf->xsize+i];
		}
		py[0]=surf->y[y];
	} else if (y < 0) {
		for (i=0;i<surf->ysize;i++) {
			py[i]=surf->y[i];
			pz[i]=surf->z[x+i*surf->xsize];
		}
		px[0]=surf->x[x];
	} else {
		px[0]=surf->x[x];
		py[0]=surf->y[y];
		pz[0]=surf->z[x+y*surf->xsize];
	}

	return ret; 
}

int esweep_index(const esweep_object *obj, int index, Real *x, Real *a, Real *b) {
	Real *real;
	Complex *cpx;
	Polar *polar;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	/* limits */
	ESWEEP_INDEX_INRANGE(obj, index, ERR_INDEX_OUTOFRANGE);
	ESWEEP_ASSERT(a != NULL, ERR_BAD_ARGUMENT); 

	switch (obj->type) {
		case WAVE:
			real=(Real*) obj->data;
			*x=1.0*index/obj->samplerate; 
			*a=real[index]; 
			break;
		case COMPLEX:
			ESWEEP_ASSERT(b != NULL, ERR_BAD_ARGUMENT); 
			cpx=(Complex*) obj->data;
			*x=1.0*index/obj->samplerate; 
			*a=cpx[index].real; 
			*b=cpx[index].imag; 
			break;
		case POLAR:
			ESWEEP_ASSERT(b != NULL, ERR_BAD_ARGUMENT); 
			polar=(Polar*) obj->data;
			*x=1.0*index*obj->samplerate/obj->size; 
			*a=polar[index].abs; 
			*b=polar[index].arg; 
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	return ERR_OK;
}

