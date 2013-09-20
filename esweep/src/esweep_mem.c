/*
 * Copyright (c) 2007, 2009, 2010 Jochen Fabricius <jfab@berlios.de>
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
 * src/esweep_mem.c:
 * Create, copy, move and free esweep objects.
 * 27.12.2010, jfab:	PRE-FREEZE, TEST OK
 * 28.12.2010, jfab: changed macro names, TEST OK
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esweep_priv.h"

esweep_object *esweep_create(const char *type, int samplerate, int size) { /* TEST: OK */
	esweep_object *obj;
	int t;
	Surface *surf;

	ESWEEP_ASSERT(type != NULL, NULL);
	ESWEEP_ASSERT(samplerate > 0, NULL);
       	ESWEEP_ASSERT(size <= ESWEEP_MAX_SIZE, NULL);
       	ESWEEP_ASSERT(size >= 0, NULL);

	t=-1;
	if (strcmp(type, "wave")==0) t=WAVE;
	if (strcmp(type, "polar")==0) t=POLAR;
	if (strcmp(type, "complex")==0) t=COMPLEX;
	if (strcmp(type, "surface")==0) t=SURFACE;

	/* ESWEEP_ASSERT is not much useful here */
	if (t==-1) {
		snprintf(errmsg, 256, "%s:%i: esweep_create(): unknown type %s\n", __FILE__, __LINE__, type);
		fprintf(stderr, errmsg);
		return NULL;
	}

	/* no check for NULL pointer necessary; can only happen when out-of-memory */
	ESWEEP_MALLOC(obj, 1, sizeof(esweep_object), NULL);
	obj->samplerate=samplerate;
	obj->type=t;

	/* defaults */
	obj->size=size;
	obj->data=NULL;
	switch (obj->type) {
		case WAVE:
			/* how to correctly check for NULL pointer?
			 * size may be zero */
			if (size > 0) {
				ESWEEP_MALLOC(obj->data, size, sizeof(Wave), NULL);
			}
			break;
		case COMPLEX:
			/* how to correctly check for NULL pointer?
			 * size may be zero */
			if (size > 0) {
				ESWEEP_MALLOC(obj->data, size, sizeof(Complex), NULL);
			}
			break;
		case POLAR:
			/* how to correctly check for NULL pointer?
			 * size may be zero */
			if (size > 0) {
				ESWEEP_MALLOC(obj->data, size, sizeof(Polar), NULL);
			}
			break;
		case SURFACE:
			/* surface size is always 1 */
			obj->size=1;
			/* allocate a surface structure */
			/* no check for NULL pointer necessary; can only happen when out-of-memory */
			ESWEEP_MALLOC(obj->data, obj->size, sizeof(Surface), NULL);
			surf=(Surface*) obj->data;
			surf->xsize=0;
			surf->ysize=0;
			surf->x=surf->y=surf->z=NULL;
			break;
		default:
			break;
		}
	return obj;
}

/* sets up a sparse surface where every value is zero */
int esweep_sparseSurface(esweep_object *obj, int xsize, int ysize) { /* TEST: OK */
	Surface *surf;
	Real *x, *y, *z;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	surf=(Surface*) obj->data;
	ESWEEP_OBJ_ISVALID_SURFACE(obj, surf, ERR_NOT_ON_THIS_TYPE, ERR_OBJ_NOT_VALID);

	ESWEEP_ASSERT(xsize > 0, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(ysize > 0, ERR_BAD_ARGUMENT);

	ESWEEP_MALLOC_SURFACE(x, y, z, xsize, ysize, xsize*ysize, ERR_MALLOC);
	/* free any old surface data */
	free(surf->x);
	free(surf->y);
	free(surf->z);

	/* set new surface data */
	surf->x = x;
	surf->y = y;
	surf->z = z;

	surf->xsize=xsize;
	surf->ysize=ysize;

	return ERR_OK;
}

esweep_object *esweep_free(esweep_object *a) { /* TEST: OK */
	ESWEEP_ASSERT(a!=NULL, NULL);

	Surface *surface;
	if (a->data!=NULL) {
		if (a->type==SURFACE) {
			surface=(Surface*) a->data;
			free(surface->x);
			free(surface->y);
			free(surface->z);
		}
		free(a->data);
	}
	free(a);
	return NULL;
}

/*
 * Move len samples from src to dst, starting at src_pos. dst_pos gives the position where to put the samples in dst.c
 */

int esweep_move(esweep_object *dst, const esweep_object *src, int dst_pos, int src_pos, int *len) { /* TEST: OK */
	Complex *src_cpx, *dst_cpx;
	Wave *src_wave, *dst_wave;
	Polar *src_polar, *dst_polar;

	ESWEEP_OBJ_NOTEMPTY(dst, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_NOTEMPTY(src, ERR_EMPTY_OBJECT);

	/*
	 * Both containers must have the same type.
	 */
	ESWEEP_ASSERT(src->type == dst->type, ERR_DIFF_TYPES);
	ESWEEP_INDEX_INRANGE(dst, dst_pos, ERR_INDEX_OUTOFRANGE);
	ESWEEP_INDEX_INRANGE(src, src_pos, ERR_INDEX_OUTOFRANGE);
	ESWEEP_ASSERT(len > 0, ERR_BAD_ARGUMENT);

	/* If the range would exceed one of the object sizes, reduce len */
	if (src_pos+(*len) > src->size) *len=src->size-src_pos;
	if (dst_pos+(*len) > dst->size) *len=dst->size-dst_pos;

	switch (src->type) {
		case WAVE:
			src_wave=(Wave*) (src->data);
			dst_wave=(Wave*) (dst->data);
			memmove(&(dst_wave[dst_pos]), &(src_wave[src_pos]), *len*sizeof(Wave));
			break;
		case COMPLEX:
			src_cpx=(Complex*) (src->data);
			dst_cpx=(Complex*) (dst->data);
			memmove(&(dst_cpx[dst_pos]), &(src_cpx[src_pos]), *len*sizeof(Complex));
			break;
		case POLAR:
			src_polar=(Polar*) (src->data);
			dst_polar=(Polar*) (dst->data);
			memmove(&(dst_polar[dst_pos]), &(src_polar[src_pos]), *len*sizeof(Polar));
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(src->type, ERR_NOT_ON_THIS_TYPE);
	}
	return ERR_OK;
}

/*
 * Copy len samples from src to dst, starting at src_pos. dst_pos gives the position where to put the samples in dst
 */
int esweep_copy(esweep_object *dst, const esweep_object *src, int dst_pos, int src_pos, int *len) { /* TEST: OK */
	Complex *src_cpx, *dst_cpx;
	Wave *src_wave, *dst_wave;
	Polar *src_polar, *dst_polar;

	ESWEEP_OBJ_NOTEMPTY(dst, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_NOTEMPTY(src, ERR_EMPTY_OBJECT);

	/*
	 * Both containers must have the same type.
	 */
	ESWEEP_ASSERT(src->type == dst->type, ERR_DIFF_TYPES);
	ESWEEP_INDEX_INRANGE(dst, dst_pos, ERR_INDEX_OUTOFRANGE);
	ESWEEP_INDEX_INRANGE(src, src_pos, ERR_INDEX_OUTOFRANGE);
	ESWEEP_ASSERT(len > 0, ERR_BAD_ARGUMENT);

	/* If the range would exceed one of the object sizes, reduce len */
	if (src_pos+(*len) > src->size) *len=src->size-src_pos;
	if (dst_pos+(*len) > dst->size) *len=dst->size-dst_pos;

	switch (src->type) {
		case WAVE:
			src_wave=(Wave*) (src->data);
			dst_wave=(Wave*) (dst->data);
			memcpy(&(dst_wave[dst_pos]), &(src_wave[src_pos]), *len*sizeof(Wave));
			break;
		case COMPLEX:
			src_cpx=(Complex*) (src->data);
			dst_cpx=(Complex*) (dst->data);
			memcpy(&(dst_cpx[dst_pos]), &(src_cpx[src_pos]), *len*sizeof(Complex));
			break;
		case POLAR:
			src_polar=(Polar*) (src->data);
			dst_polar=(Polar*) (dst->data);
			memcpy(&(dst_polar[dst_pos]), &(src_polar[src_pos]), *len*sizeof(Polar));
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(src->type, ERR_NOT_ON_THIS_TYPE);
	}

	return ERR_OK;
}

esweep_object *esweep_clone(const esweep_object *src) { /* TEST: OK */
	esweep_object *dst;
	Surface *surf_src, *surf_dst;

	ESWEEP_OBJ_ISVALID(src, NULL);

	switch (src->type) {
		case WAVE:
			ESWEEP_MALLOC(dst, 1, sizeof(esweep_object), NULL);
			if (src->size > 0) {
				ESWEEP_MALLOC(dst->data, src->size, sizeof(Wave), NULL);
				memcpy(dst->data, src->data, src->size*sizeof(Wave));
			}
			break;
		case POLAR:
			ESWEEP_MALLOC(dst, 1, sizeof(esweep_object), NULL);
			if (src->size > 0) {
				ESWEEP_MALLOC(dst->data, src->size, sizeof(Polar), NULL);
				memcpy(dst->data, src->data, src->size*sizeof(Polar));
			}
			break;
		case COMPLEX:
			ESWEEP_MALLOC(dst, 1, sizeof(esweep_object), NULL);
			if (src->size > 0) {
				ESWEEP_MALLOC(dst->data, src->size, sizeof(Complex), NULL);
				memcpy(dst->data, src->data, src->size*sizeof(Complex));
			}
			break;
		case SURFACE:
			/* allocate surface structure */
			ESWEEP_OBJ_NOTEMPTY(src, NULL);
			surf_src=(Surface*) src->data;
			ESWEEP_OBJ_ISVALID_SURFACE(src, surf_src, NULL, NULL);

			ESWEEP_MALLOC(dst, 1, sizeof(esweep_object), NULL);
			ESWEEP_MALLOC(dst->data, 1, sizeof(Surface), NULL);
			dst->size=1;
			dst->samplerate=src->samplerate;
			dst->type=SURFACE;

			surf_dst=(Surface*) dst->data;
			surf_dst->xsize=surf_src->xsize;
			surf_dst->ysize=surf_src->ysize;

			ESWEEP_MALLOC_SURFACE(surf_dst->x, surf_dst->y, surf_dst->z, \
					surf_dst->xsize, \
					surf_dst->ysize, \
					surf_dst->xsize*surf_dst->ysize, NULL);
			memcpy(surf_dst->x, surf_src->x, surf_src->xsize);
			memcpy(surf_dst->y, surf_src->y, surf_src->ysize);
			memcpy(surf_dst->z, surf_src->z, surf_src->xsize*surf_src->ysize);
			break;

		default:
			return NULL;
	}

	dst->samplerate=src->samplerate;
	dst->size=src->size;
	dst->type=src->type;
	return dst;
}

