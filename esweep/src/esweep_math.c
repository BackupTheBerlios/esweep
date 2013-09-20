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


/*
 * Math functions
 */

/*
 * src/esweep_math.c:
 * 05.04.2010, jfab: - use new data types and definitions, man reworks
 * 28.12.2010, jfab: - use the new error checking macros
 * 04.01.2011, jfab: - fixed some errors, changed copyright year
 */

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esweep_priv.h"
#include "fft.h"


static inline Real __esweep_intern__sum(const esweep_object *obj);

int esweep_max(const esweep_object *obj, int from, int to, Real *max) { /* UNTESTED */
	Wave *wave;
	Polar *polar;
	Surface *surf;
	Complex *cplx;

	Real tmp;
	int i, size;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	ESWEEP_ASSERT(from < obj->size, ERR_BAD_PARAMETER);
	ESWEEP_ASSERT(to < obj->size, ERR_BAD_PARAMETER);

	if (from < 0) from=0;
	if (to < 0) to=obj->size-1;

	ESWEEP_ASSERT(to > from, ERR_BAD_PARAMETER);

	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=from+1, *max=wave[from]; i <= to; i++)
				if (wave[i]>*max) *max=wave[i];
			break;
		case COMPLEX:
			cplx=(Complex*) obj->data;
			for (i=from+1, *max=CABS(cplx[from]); i <= to; i++) {
				tmp=CABS(cplx[i]);
				if (tmp>*max) *max=tmp;
			}
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=from+1, *max=polar[from].abs; i <= to; i++)
				if (polar[i].abs>*max) *max=polar[i].abs;
			break;
		case SURFACE:
			surf=(Surface*) obj->data;
			for (i=1, *max=surf->z[0], size=surf->xsize*surf->ysize; i < size; i++)
				if (surf->z[i]>*max) *max=surf->z[i];
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	return ERR_OK;
}

int esweep_min(const esweep_object *obj, int from, int to, Real *min) { /* UNTESTED */
	Wave *wave;
	Polar *polar;
	Surface *surf;
	Complex *cplx;

	Real tmp;
	int i, size;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	ESWEEP_ASSERT(from < obj->size, ERR_BAD_PARAMETER);
	ESWEEP_ASSERT(to < obj->size, ERR_BAD_PARAMETER);

	if (from < 0) from=0;
	if (to < 0) to=obj->size-1;

	ESWEEP_ASSERT(to > from, ERR_BAD_PARAMETER);

	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=from+1, *min=wave[from];i<= to; i++)
				if (wave[i]< *min) *min=wave[i];
			break;
		case COMPLEX:
			cplx=(Complex*) obj->data;
			for (i=from+1, *min=CABS(cplx[from]);i<= to; i++) {
				tmp=CABS(cplx[i]);
				if (tmp<*min) *min=tmp;
			}
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=from+1, *min=polar[from].abs;i<= to; i++)
				if (polar[i].abs<*min) *min=polar[i].abs;
			break;
		case SURFACE:
			surf=(Surface*) obj->data;
			for (i=1, *min=surf->z[0], size=surf->xsize*surf->ysize;i<size;i++)
				if (surf->z[i]<*min) *min=surf->z[i];
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	return ERR_OK;
}


int esweep_maxPos(const esweep_object *obj, int from, int to, int *pos) { /* UNTESTED */
	Wave *wave;
	Polar *polar;
	Complex *cplx;

	Real max, tmp;
	int i;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	ESWEEP_ASSERT(from < obj->size, ERR_BAD_PARAMETER);
	ESWEEP_ASSERT(to < obj->size, ERR_BAD_PARAMETER);

	if (from < 0) from=0;
	if (to < 0) to=obj->size-1;

	ESWEEP_ASSERT(to > from, ERR_BAD_PARAMETER);

	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=from+1, max=wave[from], *pos=0;i<= to; i++)
				if (wave[i]>max) {
					max=wave[i];
					*pos=i;
				}
			break;
		case COMPLEX:
			cplx=(Complex*) obj->data;
			for (i=from+1, *pos=0, max=CABS(cplx[from]);i<= to; i++) {
				tmp=CABS(cplx[i]);
				if (tmp>max) {
					max=tmp;
					*pos=i;
				}
			}
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=from+1, *pos=0, max=polar[from].abs;i<= to; i++) {
				if (polar[i].abs>max) {
					max=polar[i].abs;
					*pos=i;
				}
			}
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	return ERR_OK;
}

int esweep_minPos(const esweep_object *obj, int from, int to, int *pos) { /* UNTESTED */
	Wave *wave;
	Polar *polar;
	Complex *cplx;

	Real min, tmp;
	int i;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	ESWEEP_ASSERT(from < obj->size, ERR_BAD_PARAMETER);
	ESWEEP_ASSERT(to < obj->size, ERR_BAD_PARAMETER);

	if (from < 0) from=0;
	if (to < 0) to=obj->size-1;

	ESWEEP_ASSERT(to > from, ERR_BAD_PARAMETER);

	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=from+1, *pos=0, min=wave[from];i<obj->size;i++)
				if (wave[i]<min) {
					min=wave[i];
					*pos=i;
				}
			break;
		case COMPLEX:
			cplx=(Complex*) obj->data;
			for (i=from+1, *pos=0, min=CABS(cplx[from]);i<obj->size;i++) {
				tmp=CABS(cplx[i]);
				if (tmp<min) {
					min=tmp;
					*pos=i;
				}
			}
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=from+1, *pos=0, min=polar[from].abs;i<obj->size;i++) {
				if (polar[i].abs<min) {
					min=polar[i].abs;
					*pos=i;
				}
			}
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	return ERR_OK;
}

int esweep_avg(const esweep_object *obj, Real *avg) { /* UNTESTED */
	Surface *surf;
	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	if (obj->type == SURFACE) {
		surf=(Surface*) obj->data;
		ESWEEP_OBJ_ISVALID_SURFACE(obj, surf, ERR_NOT_ON_THIS_TYPE, ERR_OBJ_NOT_VALID);
		*avg= __esweep_intern__sum(obj)/(surf->xsize*surf->ysize);
	} else 	*avg= __esweep_intern__sum(obj)/obj->size;
	return ERR_OK;
}

int esweep_sum(const esweep_object *obj, Real *sum) { /* UNTESTED */
	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	*sum= __esweep_intern__sum(obj);
	return ERR_OK;
}

int esweep_sqsum(const esweep_object *obj, Real *sqsum) { /* UNTESTED */
	Wave *wave;
	Polar *polar;
	Complex *cplx;
	Surface *surf;

	int i, size;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	*sqsum=0.0;
	size=obj->size;
	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=0;i<size;i++)
				*sqsum+=wave[i]*wave[i];
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=0;i<size;i++)
				*sqsum+=polar[i].abs*polar[i].abs;
			break;
		case COMPLEX:
			cplx=(Complex*) obj->data;
			for (i=0;i<size;i++)
				*sqsum+=cplx[i].real*cplx[i].real+cplx[i].imag*cplx[i].imag;;
			break;
		case SURFACE:
			surf=(Surface*) obj->data;
			for (i=0, size=surf->xsize*surf->ysize;i<size;i++)
				*sqsum+=surf->z[i]*surf->z[i];
			break;
		default:
			*sqsum=__NAN("");
	}

	return ERR_OK;
}

int esweep_real(esweep_object *obj) { /* UNTESTED */
	Complex *cpx;
	Polar *polar;
	int i;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case COMPLEX:
			cpx=(Complex*) obj->data;
			for (i=0;i<obj->size;i++) cpx[i].imag=0;
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			cpx=(Complex*) polar;
			for (i=0;i<obj->size;i++) {
				cpx[i].real=polar[i].abs*COS(polar[i].arg);
				cpx[i].imag=0;
			}
			obj->type=COMPLEX;
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	ESWEEP_ASSERT(correctFpException(obj), ERR_FP);
	return ERR_OK;
}

int esweep_imag(esweep_object *obj) { /* UNTESTED */
	Complex *cpx;
	Polar *polar;
	int i;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case COMPLEX:
			cpx=(Complex*) obj->data;
			for (i=0;i<obj->size;i++) cpx[i].real=0;
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			cpx=(Complex*) polar;
			for (i=0;i<obj->size;i++) {
				cpx[i].imag=polar[i].abs*SIN(polar[i].arg);
				cpx[i].real=0;
			}
			obj->type=COMPLEX;
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	ESWEEP_ASSERT(correctFpException(obj), ERR_FP);
	return ERR_OK;
}

int esweep_abs(esweep_object *obj) { /* UNTESTED */
	Complex *cpx;
	Polar *polar;
	Wave *wave;
	Surface *surf;
	int i, size;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=0;i<obj->size;i++) wave[i]=FABS(wave[i]);
			break;
		case COMPLEX:
			cpx=(Complex*) obj->data;
			polar=(Polar*) cpx;
			for (i=0;i<obj->size;i++) {
				polar[i].abs=CABS(cpx[i]);
				polar[i].arg=0.0;
			}
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=0;i<obj->size;i++) polar[i].arg=0;
			break;
		case SURFACE:
			surf=(Surface*) obj->data;
			ESWEEP_OBJ_ISVALID_SURFACE(obj, surf, ERR_NOT_ON_THIS_TYPE, ERR_OBJ_NOT_VALID);
			for (i=0, size=surf->xsize*surf->ysize;i < size; i++)
				surf->z[i]=FABS(surf->z[i]);
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	ESWEEP_ASSERT(correctFpException(obj), ERR_FP);
	return ERR_OK;
}

int esweep_arg(esweep_object *obj) { /* UNTESTED */
	Complex *cpx;
	Polar *polar;
	int i;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case COMPLEX:
			cpx=(Complex*) obj->data;
			polar=(Polar*) cpx;
			for (i=0;i<obj->size;i++) {
				polar[i].arg=ATAN2(cpx[i]);
				polar[i].abs=0.0;
			}
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=0;i<obj->size;i++) polar[i].abs=0;
			break;
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	ESWEEP_ASSERT(correctFpException(obj), ERR_FP);
	return ERR_OK;
}

int esweep_clipLower(esweep_object *a, const esweep_object *b) {
	Complex *cpx_a, *cpx_b;
	Polar *polar_a, *polar_b;
	Wave *wave_a, *wave_b;
	Surface *surf_a;
	int i, size;

	ESWEEP_OBJ_NOTEMPTY(a, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_NOTEMPTY(b, ERR_EMPTY_OBJECT);

	ESWEEP_SAME_MAPPING(a, b, ERR_DIFF_MAPPING);

  if (a->type != SURFACE) {
    ESWEEP_ASSERT(a->type == b->type, ERR_DIFF_TYPES);
  } else {
    ESWEEP_ASSERT(b->type == WAVE, ERR_OP_NOT_ALLOWED);
  }

  if (b->size > 1) {
    size = a->size > b->size ? b->size : a->size;
    switch (a->type) {
      case WAVE:
        wave_a=(Wave*) a->data;
        wave_b=(Wave*) b->data;
        for (i=0; i < size; i++) {
          wave_a[i] = wave_a[i] > wave_b[i] ? wave_b[i] : wave_a[i];
        }
        break;
      case COMPLEX:
        cpx_a=(Complex*) a->data;
        cpx_b=(Complex*) b->data;
        for (i=0; i < size; i++) {
          cpx_a[i].real = cpx_a[i].real > cpx_b[i].real ? cpx_b[i].real : cpx_a[i].real;
          cpx_a[i].imag = cpx_a[i].imag > cpx_b[i].imag ? cpx_b[i].imag : cpx_a[i].imag;
        }
        break;
      case POLAR:
        polar_a=(Polar*) a->data;
        polar_b=(Polar*) b->data;
        for (i=0; i < size; i++) {
          polar_a[i].abs = polar_a[i].abs > polar_b[i].abs ? polar_b[i].abs : polar_a[i].abs;
          polar_a[i].arg = polar_a[i].arg > polar_b[i].arg ? polar_b[i].arg : polar_a[i].arg;
        }
        break;
      case SURFACE:
      default:
        return ERR_NOT_ON_THIS_TYPE;
    }
  } else {
    size=a->size;
    switch (a->type) {
      case WAVE:
        wave_a=(Wave*) a->data;
        wave_b=(Wave*) b->data;
        for (i=0; i < size; i++) {
          wave_a[i] = wave_a[i] > wave_b[0] ? wave_b[0] : wave_a[i];
        }
        break;
      case COMPLEX:
        cpx_a=(Complex*) a->data;
        cpx_b=(Complex*) b->data;
        for (i=0; i < size; i++) {
          cpx_a[i].real = cpx_a[i].real > cpx_b[0].real ? cpx_b[0].real : cpx_a[i].real;
          cpx_a[i].imag = cpx_a[i].imag > cpx_b[0].imag ? cpx_b[0].imag : cpx_a[i].imag;
        }
        break;
      case POLAR:
        polar_a=(Polar*) a->data;
        polar_b=(Polar*) b->data;
        for (i=0; i < size; i++) {
          polar_a[i].abs = polar_a[i].abs > polar_b[0].abs ? polar_b[0].abs : polar_a[i].abs;
          polar_a[i].arg = polar_a[i].arg > polar_b[0].arg ? polar_b[0].arg : polar_a[i].arg;
        }
        break;
      case SURFACE:
        surf_a=(Surface*) a->data;
        wave_b=(Wave*) b->data;
        ESWEEP_OBJ_ISVALID_SURFACE(a, surf_a, ERR_NOT_ON_THIS_TYPE, ERR_OBJ_NOT_VALID);
        for (i=0, size=surf_a->xsize*surf_a->ysize; i < size; i++) {
          surf_a->z[i] = surf_a->z[i] > wave_b[i] ? wave_b[i] : surf_a->z[i];
        }
        break;
      default:
        return ERR_NOT_ON_THIS_TYPE;
    }
  }

  return ERR_OK;
}


int esweep_clipUpper(esweep_object *a, const esweep_object *b) {
	Complex *cpx_a, *cpx_b;
	Polar *polar_a, *polar_b;
	Wave *wave_a, *wave_b;
	Surface *surf_a;
	int i, size;

	ESWEEP_OBJ_NOTEMPTY(a, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_NOTEMPTY(b, ERR_EMPTY_OBJECT);

	ESWEEP_SAME_MAPPING(a, b, ERR_DIFF_MAPPING);

  if (a->type != SURFACE) {
    ESWEEP_ASSERT(a->type == b->type, ERR_DIFF_TYPES);
  } else {
    ESWEEP_ASSERT(b->type == WAVE, ERR_OP_NOT_ALLOWED);
  }

  if (b->size > 1) {
    size = a->size > b->size ? b->size : a->size;
    switch (a->type) {
      case WAVE:
        wave_a=(Wave*) a->data;
        wave_b=(Wave*) b->data;
        for (i=0; i < size; i++) {
          wave_a[i] = wave_a[i] < wave_b[i] ? wave_b[i] : wave_a[i];
        }
        break;
      case COMPLEX:
        cpx_a=(Complex*) a->data;
        cpx_b=(Complex*) b->data;
        for (i=0; i < size; i++) {
          cpx_a[i].real = cpx_a[i].real < cpx_b[i].real ? cpx_b[i].real : cpx_a[i].real;
          cpx_a[i].imag = cpx_a[i].imag < cpx_b[i].imag ? cpx_b[i].imag : cpx_a[i].imag;
        }
        break;
      case POLAR:
        polar_a=(Polar*) a->data;
        polar_b=(Polar*) b->data;
        for (i=0; i < size; i++) {
          polar_a[i].abs = polar_a[i].abs < polar_b[i].abs ? polar_b[i].abs : polar_a[i].abs;
          polar_a[i].arg = polar_a[i].arg < polar_b[i].arg ? polar_b[i].arg : polar_a[i].arg;
        }
        break;
      case SURFACE:
      default:
        return ERR_NOT_ON_THIS_TYPE;
    }
  } else {
    size=a->size;
    switch (a->type) {
      case WAVE:
        wave_a=(Wave*) a->data;
        wave_b=(Wave*) b->data;
        for (i=0; i < size; i++) {
          wave_a[i] = wave_a[i] < wave_b[0] ? wave_b[0] : wave_a[i];
        }
        break;
      case COMPLEX:
        cpx_a=(Complex*) a->data;
        cpx_b=(Complex*) b->data;
        for (i=0; i < size; i++) {
          cpx_a[i].real = cpx_a[i].real < cpx_b[0].real ? cpx_b[0].real : cpx_a[i].real;
          cpx_a[i].imag = cpx_a[i].imag < cpx_b[0].imag ? cpx_b[0].imag : cpx_a[i].imag;
        }
        break;
      case POLAR:
        polar_a=(Polar*) a->data;
        polar_b=(Polar*) b->data;
        for (i=0; i < size; i++) {
          polar_a[i].abs = polar_a[i].abs < polar_b[0].abs ? polar_b[0].abs : polar_a[i].abs;
          polar_a[i].arg = polar_a[i].arg < polar_b[0].arg ? polar_b[0].arg : polar_a[i].arg;
        }
        break;
      case SURFACE:
        surf_a=(Surface*) a->data;
        wave_b=(Wave*) b->data;
        ESWEEP_OBJ_ISVALID_SURFACE(a, surf_a, ERR_NOT_ON_THIS_TYPE, ERR_OBJ_NOT_VALID);
        for (i=0, size=surf_a->xsize*surf_a->ysize; i < size; i++) {
          surf_a->z[i] = surf_a->z[i] < wave_b[i] ? wave_b[i] : surf_a->z[i];
        }
        break;
      default:
        return ERR_NOT_ON_THIS_TYPE;
    }
  }

  return ERR_OK;
}

/* elementary math */

#define CC(op, a, b, size_a, size_b, N) if (size_b == 1) { \
						for (i=0;i < size_a;i++) { \
							ESWEEP_MATH_CC_##op(a[i], b[0]); \
						} \
					} else { \
						N=size_a > size_b ? size_b : size_a; \
						for (i=0;i < N;i++) { \
							ESWEEP_MATH_CC_##op(a[i], b[i]); \
						} \
					}

#define CP(op, a, b, size_a, size_b, N)	if (size_b == 1) { \
						for (i=0;i < size_a;i++) { \
							ESWEEP_MATH_CP_##op(a[i], b[0]); \
						} \
					} else { \
						N=size_a > size_b ? size_b : size_a; \
						for (i=0;i < N;i++) { \
							ESWEEP_MATH_CP_##op(a[i], b[i]); \
						} \
					}

#define CR(op, a, b, size_a, size_b, N)	if (size_b == 1) { \
						for (i=0;i < size_a;i++) { \
							ESWEEP_MATH_CR_##op(a[i], b[0]); \
						} \
					} else { \
						N=size_a > size_b ? size_b : size_a; \
						for (i=0;i < N;i++) { \
							ESWEEP_MATH_CR_##op(a[i], b[i]); \
						} \
					}

#define PP(op, a, b, size_a, size_b, N)	if (size_b == 1) { \
						for (i=0;i < size_a;i++) { \
							ESWEEP_MATH_PP_##op(a[i], b[0]); \
						} \
					} else { \
						N=size_a > size_b ? size_b : size_a; \
						for (i=0;i < N;i++) { \
							ESWEEP_MATH_PP_##op(a[i], b[i]); \
						} \
					}

#define PR(op, a, b, size_a, size_b, N)	if (size_b == 1) { \
						for (i=0;i < size_a;i++) { \
							ESWEEP_MATH_PR_##op(a[i], b[0]); \
						} \
					} else { \
						N=size_a > size_b ? size_b : size_a; \
						for (i=0;i < N;i++) { \
							ESWEEP_MATH_PR_##op(a[i], b[i]); \
						} \
					}


int esweep_add(esweep_object *a, const esweep_object *b) {
	Wave *wave_a, *wave_b;
	Complex *cpx_a, *cpx_b;
	Polar *polar_a, *polar_b;
	Surface *surf;
	int i, N;

	ESWEEP_OBJ_NOTEMPTY(a, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_NOTEMPTY(b, ERR_EMPTY_OBJECT);

	ESWEEP_SAME_MAPPING(a, b, ERR_DIFF_MAPPING);

	switch (a->type) {
		case WAVE:
			wave_a=(Wave*) a->data;
			switch (b->type) {
				case WAVE:
					wave_b=(Wave*) b->data;
					if (b->size == 1) {
						for (i=0;i < a->size;i++) {
							wave_a[i] += wave_b[0];
						}
					} else {
						N=a->size > b->size ? b->size : a->size;
						for (i=0;i < N;i++) {
							wave_a[i] += wave_b[i];
						}
					}
					break;
				case COMPLEX:
					ESWEEP_CONV_WAVE2COMPLEX(a, cpx_a);
					cpx_b=(Complex*) b->data;
					CC(ADD, cpx_a, cpx_b, a->size, b->size, N);
					break;
				case POLAR:
					ESWEEP_CONV_WAVE2COMPLEX(a, cpx_a);
					polar_b=(Polar*) b->data;
					CP(ADD, cpx_a, polar_b, a->size, b->size, N);
					c2p((Polar*) cpx_a, cpx_a, a->size);
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
				}
			break;
		case COMPLEX:
			switch (b->type) {
				case WAVE:
					cpx_a=(Complex*) a->data;
					wave_b=(Wave*) b->data;
					CR(ADD, cpx_a, wave_b, a->size, b->size, N);
					break;
				case COMPLEX:
					cpx_a=(Complex*) a->data;
					cpx_b=(Complex*) b->data;
					CC(ADD, cpx_a, cpx_b, a->size, b->size, N);
					break;
				case POLAR:
					cpx_a=(Complex*) a->data;
					polar_b=(Polar*) b->data;
					CP(ADD, cpx_a, polar_b, a->size, b->size, N);
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
			}
			break;
		case POLAR:
			polar_a=(Polar*) a->data;
			switch (b->type) {
				case WAVE:
					cpx_a=(Complex*) polar_a;
					p2c(cpx_a, polar_a, a->size);
					wave_b=(Wave*) b->data;
					CR(ADD, cpx_a, wave_b, a->size, b->size, N);
					c2p(polar_a, cpx_a, a->size);
					break;
				case COMPLEX:
					cpx_a=(Complex*) polar_a;
					p2c(cpx_a, polar_a, a->size);
					cpx_b=(Complex*) b->data;
					CC(ADD, cpx_a, cpx_b, a->size, b->size, N);
					c2p(polar_a, cpx_a, a->size);
					break;
				case POLAR:
					polar_b=(Polar*) b->data;
					PP(ADD, polar_a, polar_b, a->size, b->size, N);
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
			}
			break;
		case SURFACE:
			switch (b->type) {
				case WAVE:
					ESWEEP_ASSERT(b->size == 1, ERR_SIZE_MISMATCH);
					wave_b=(Wave*) b->data;
					surf=(Surface*) a->data;
					N=surf->xsize*surf->ysize;
					for (i=0; i < N; i++) {
						(surf->z)[i]+=wave_b[0];
					}
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
			}
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(a->type, ERR_NOT_ON_THIS_TYPE);
	}
	ESWEEP_ASSERT(correctFpException(a), ERR_FP);
	return ERR_OK;
}

int esweep_sub(esweep_object *a, const esweep_object *b) {
	Wave *wave_a, *wave_b;
	Complex *cpx_a, *cpx_b;
	Polar *polar_a, *polar_b;
	Surface *surf;
	int i, N;

	ESWEEP_OBJ_NOTEMPTY(a, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_NOTEMPTY(b, ERR_EMPTY_OBJECT);

	ESWEEP_SAME_MAPPING(a, b, ERR_DIFF_MAPPING);

	switch (a->type) {
		case WAVE:
			wave_a=(Wave*) a->data;
			switch (b->type) {
				case WAVE:
					wave_b=(Wave*) b->data;
					if (b->size == 1) {
						for (i=0;i < a->size;i++) {
							wave_a[i] -= wave_b[0];
						}
					} else {
						N=a->size > b->size ? b->size : a->size;
						for (i=0;i < N;i++) {
							wave_a[i] -= wave_b[i];
						}
					}
					break;
				case COMPLEX:
					ESWEEP_CONV_WAVE2COMPLEX(a, cpx_a);
					cpx_b=(Complex*) b->data;
					CC(SUB, cpx_a, cpx_b, a->size, b->size, N);
					break;
				case POLAR:
					ESWEEP_CONV_WAVE2COMPLEX(a, cpx_a);
					polar_b=(Polar*) b->data;
					CP(SUB, cpx_a, polar_b, a->size, b->size, N);
					c2p((Polar*) cpx_a, cpx_a, a->size);
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
				}
			break;
		case COMPLEX:
			switch (b->type) {
				case WAVE:
					cpx_a=(Complex*) a->data;
					wave_b=(Wave*) b->data;
					CR(SUB, cpx_a, wave_b, a->size, b->size, N);
					break;
				case COMPLEX:
					cpx_a=(Complex*) a->data;
					cpx_b=(Complex*) b->data;
					CC(SUB, cpx_a, cpx_b, a->size, b->size, N);
					break;
				case POLAR:
					cpx_a=(Complex*) a->data;
					polar_b=(Polar*) b->data;
					CP(SUB, cpx_a, polar_b, a->size, b->size, N);
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
			}
			break;
		case POLAR:
			polar_a=(Polar*) a->data;
			switch (b->type) {
				case WAVE:
					cpx_a=(Complex*) polar_a;
					p2c(cpx_a, polar_a, a->size);
					wave_b=(Wave*) b->data;
					CR(SUB, cpx_a, wave_b, a->size, b->size, N);
					c2p(polar_a, cpx_a, a->size);
					break;
				case COMPLEX:
					cpx_a=(Complex*) polar_a;
					p2c(cpx_a, polar_a, a->size);
					cpx_b=(Complex*) b->data;
					CC(SUB, cpx_a, cpx_b, a->size, b->size, N);
					c2p(polar_a, cpx_a, a->size);
					break;
				case POLAR:
					polar_b=(Polar*) b->data;
					PP(SUB, polar_a, polar_b, a->size, b->size, N);
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
			}
			break;
		case SURFACE:
			switch (b->type) {
				case WAVE:
					ESWEEP_ASSERT(b->size == 1, ERR_SIZE_MISMATCH);
					wave_b=(Wave*) b->data;
					surf=(Surface*) a->data;
					N=surf->xsize*surf->ysize;
					for (i=0; i < N; i++) {
						(surf->z)[i]-=wave_b[0];
					}
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
			}
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(a->type, ERR_NOT_ON_THIS_TYPE);
	}
	ESWEEP_ASSERT(correctFpException(a), ERR_FP);
	return ERR_OK;
}

int esweep_mul(esweep_object *a, const esweep_object *b) {
	Wave *wave_a, *wave_b;
	Complex *cpx_a, *cpx_b;
	Polar *polar_a, *polar_b;
	Surface *surf;
	int i, N;
	Real tmp; // necessary for *_MUL macros

	ESWEEP_OBJ_NOTEMPTY(a, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_NOTEMPTY(b, ERR_EMPTY_OBJECT);

	ESWEEP_SAME_MAPPING(a, b, ERR_DIFF_MAPPING);

	switch (a->type) {
		case WAVE:
			wave_a=(Wave*) a->data;
			switch (b->type) {
				case WAVE:
					wave_b=(Wave*) b->data;
					if (b->size == 1) {
						for (i=0;i < a->size;i++) {
							wave_a[i] *= wave_b[0];
						}
					} else {
						N=a->size > b->size ? b->size : a->size;
						for (i=0;i < N;i++) {
							wave_a[i] *= wave_b[i];
						}
					}
					break;
				case COMPLEX:
					ESWEEP_CONV_WAVE2COMPLEX(a, cpx_a);
					cpx_b=(Complex*) b->data;
					CC(MUL, cpx_a, cpx_b, a->size, b->size, N);
					break;
				case POLAR:
					ESWEEP_CONV_WAVE2POLAR(a, polar_a);
					polar_b=(Polar*) b->data;
					PP(MUL, polar_a, polar_b, a->size, b->size, N);
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
				}
			break;
		case COMPLEX:
			cpx_a=(Complex*) a->data;
			switch (b->type) {
				case WAVE:
					wave_b=(Wave*) b->data;
					CR(MUL, cpx_a, wave_b, a->size, b->size, N);
					break;
				case COMPLEX:
					cpx_b=(Complex*) b->data;
					CC(MUL, cpx_a, cpx_b, a->size, b->size, N);
					break;
				case POLAR:
					polar_a=(Polar*) cpx_a;
					c2p(polar_a, cpx_a, a->size);
					polar_b=(Polar*) b->data;
					PP(MUL, polar_a, polar_b, a->size, b->size, N);
					p2c(cpx_a, polar_a, a->size);
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
			}
			break;
		case POLAR:
			polar_a=(Polar*) a->data;
			switch (b->type) {
				case WAVE:
					wave_b=(Wave*) b->data;
					PR(MUL, polar_a, wave_b, a->size, b->size, N);
					break;
				case COMPLEX:
					cpx_b=(Complex*) b->data;
					polar_b=(Polar*) b->data;
					c2p(polar_b, cpx_b, b->size);
					PP(MUL, polar_a, polar_b, a->size, b->size, N);
					p2c(cpx_b, polar_b, b->size);
					break;
				case POLAR:
					polar_b=(Polar*) b->data;
					PP(MUL, polar_a, polar_b, a->size, b->size, N);
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
			}
			break;
		case SURFACE:
			switch (b->type) {
				case WAVE:
					ESWEEP_ASSERT(b->size == 1, ERR_SIZE_MISMATCH);
					wave_b=(Wave*) b->data;
					surf=(Surface*) a->data;
					N=surf->xsize*surf->ysize;
					for (i=0; i < N; i++) {
						(surf->z)[i]*=wave_b[0];
					}
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
			}
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(a->type, ERR_NOT_ON_THIS_TYPE);
	}
	ESWEEP_ASSERT(correctFpException(a), ERR_FP);
	return ERR_OK;
}

int esweep_div(esweep_object *a, const esweep_object *b) {
	Wave *wave_a, *wave_b;
	Complex *cpx_a, *cpx_b;
	Polar *polar_a, *polar_b;
	Surface *surf;
	int i, N;
	Real tmp, denom; // necessary for *_DIV macros

	ESWEEP_OBJ_NOTEMPTY(a, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_NOTEMPTY(b, ERR_EMPTY_OBJECT);

	ESWEEP_SAME_MAPPING(a, b, ERR_DIFF_MAPPING);

	switch (a->type) {
		case WAVE:
			wave_a=(Wave*) a->data;
			switch (b->type) {
				case WAVE:
					wave_b=(Wave*) b->data;
					if (b->size == 1) {
						for (i=0;i < a->size;i++) {
							wave_a[i] /= wave_b[0];
						}
					} else {
						N=a->size > b->size ? b->size : a->size;
						for (i=0;i < N;i++) {
							wave_a[i] /= wave_b[i];
						}
					}
					break;
				case COMPLEX:
					ESWEEP_CONV_WAVE2COMPLEX(a, cpx_a);
					cpx_b=(Complex*) b->data;
					CC(DIV, cpx_a, cpx_b, a->size, b->size, N);
					break;
				case POLAR:
					ESWEEP_CONV_WAVE2POLAR(a, polar_a);
					polar_b=(Polar*) b->data;
					PP(DIV, polar_a, polar_b, a->size, b->size, N);
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
				}
			break;
		case COMPLEX:
			cpx_a=(Complex*) a->data;
			switch (b->type) {
				case WAVE:
					wave_b=(Wave*) b->data;
					CR(DIV, cpx_a, wave_b, a->size, b->size, N);
					break;
				case COMPLEX:
					cpx_b=(Complex*) b->data;
					CC(DIV, cpx_a, cpx_b, a->size, b->size, N);
					break;
				case POLAR:
					polar_a=(Polar*) cpx_a;
					c2p(polar_a, cpx_a, a->size);
					polar_b=(Polar*) b->data;
					PP(DIV, polar_a, polar_b, a->size, b->size, N);
					p2c(cpx_a, polar_a, a->size);
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
			}
			break;
		case POLAR:
			polar_a=(Polar*) a->data;
			switch (b->type) {
				case WAVE:
					wave_b=(Wave*) b->data;
					PR(DIV, polar_a, wave_b, a->size, b->size, N);
					break;
				case COMPLEX:
					cpx_b=(Complex*) b->data;
					polar_b=(Polar*) b->data;
					c2p(polar_b, cpx_b, b->size);
					PP(DIV, polar_a, polar_b, a->size, b->size, N);
					p2c(cpx_b, polar_b, b->size);
					break;
				case POLAR:
					polar_b=(Polar*) b->data;
					PP(DIV, polar_a, polar_b, a->size, b->size, N);
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
			}
			break;
		case SURFACE:
			switch (b->type) {
				case WAVE:
					ESWEEP_ASSERT(b->size == 1, ERR_SIZE_MISMATCH);
					wave_b=(Wave*) b->data;
					surf=(Surface*) a->data;
					N=surf->xsize*surf->ysize;
					for (i=0; i < N; i++) {
						(surf->z)[i]*=wave_b[0];
					}
					break;
				default:
					ESWEEP_NOT_THIS_TYPE(b->type, ERR_NOT_ON_THIS_TYPE);
			}
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(a->type, ERR_NOT_ON_THIS_TYPE);
	}
	ESWEEP_ASSERT(correctFpException(a), ERR_FP);
	return ERR_OK;
}

/* power, exponentiation, logarithm */

int esweep_ln(esweep_object *obj) { /* natural logarithm */
	Wave *wave;
	Polar *polar;
	Surface *surface;
	Complex *cpx;
	int i, zsize;
	Real abs, arg;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=0;i<obj->size;i++) {
				wave[i]=LN(FABS(wave[i]));
			}
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=0;i<obj->size;i++) {
				polar[i].abs=LN(polar[i].abs);
			}
			break;
		case SURFACE:
			surface=(Surface*) obj->data;
			zsize=surface->xsize*(surface->ysize);
			for (i=0;i<zsize;i++) surface->z[i]=LN(surface->z[i]);
			break;
		case COMPLEX:
			cpx=(Complex*) obj->data;
			for (i=0;i<obj->size;i++) {
				abs=CABS(cpx[i]);
				arg=ATAN2(cpx[i]);
				cpx[i].real=LN(abs);
				cpx[i].imag=arg;
			}
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	ESWEEP_ASSERT(correctFpException(obj), ERR_FP);
	return ERR_OK;
}

int esweep_lg(esweep_object *obj) { /* logarithm to base 10 */
	Wave *wave;
	Polar *polar;
	Surface *surface;
	Complex *cpx;
	int i, zsize;
	Real abs, arg;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=0;i<obj->size;i++) {
				wave[i]=LG(FABS(wave[i]));
			}
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=0;i<obj->size;i++) {
				polar[i].abs=LG(polar[i].abs);
			}
			break;
		case SURFACE:
			surface=(Surface*) obj->data;
			zsize=surface->xsize*(surface->ysize);
			for (i=0;i<zsize;i++) surface->z[i]=LG(surface->z[i]);
			break;
		case COMPLEX:
			cpx=(Complex*) obj->data;
			for (i=0;i<obj->size;i++) {
				abs=CABS(cpx[i]);
				arg=ATAN2(cpx[i]);
				cpx[i].real=LG(abs);
				cpx[i].imag=arg;
			}
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	ESWEEP_ASSERT(correctFpException(obj), ERR_FP);

	return ERR_OK;
}

int esweep_exp(esweep_object *obj) { /* e^obj */
	Wave *wave;
	Polar *polar;
	Surface *surface;
	Complex *cpx;
	int i, zsize;
	Real abs, arg;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=0;i<obj->size;i++) {
				wave[i]=EXP(wave[i]);
			}
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=0;i<obj->size;i++) {
				polar[i].abs=EXP(polar[i].abs);
			}
			break;
		case SURFACE:
			surface=(Surface*) obj->data;
			zsize=surface->xsize*(surface->ysize);
			for (i=0;i<zsize;i++) surface->z[i]=EXP(surface->z[i]);
			break;
		case COMPLEX:
			cpx=(Complex*) obj->data;
			for (i=0;i<obj->size;i++) {
				abs=EXP(cpx[i].real);
				arg=cpx[i].imag;
				cpx[i].real=abs*COS(arg);
				cpx[i].imag=abs*SIN(arg);
			}
			break;
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	ESWEEP_ASSERT(correctFpException(obj), ERR_FP);
	return ERR_OK;
}

int esweep_pow(esweep_object *obj, Real x) { /* obj^x */
	Wave *wave;
	Polar *polar;
	Surface *surface;
	int i, zsize;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=0;i<obj->size;i++) {
				wave[i]=POW(wave[i], x);
			}
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=0;i<obj->size;i++) {
				polar[i].abs=POW(polar[i].abs, x);
			}
			break;
		case SURFACE:
			surface=(Surface*) obj->data;
			zsize=surface->xsize*(surface->ysize);
			for (i=0;i<zsize;i++) surface->z[i]=POW(surface->z[i], x);
			break;
		case COMPLEX:
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	ESWEEP_ASSERT(correctFpException(obj), ERR_FP);
	return ERR_OK;
}

/*
calculates the schroeder integral of a WAVE
this is not the real schroeder equation, but the backward integral
to get the energy decay curve you have to square the impulse response with
esweep_pow(a, 2)
*/
int esweep_schroeder(esweep_object *obj) {
	Wave *wave, *wave_out;
	int i, j;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	if (obj->type != WAVE) ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);

	wave=(Wave*) obj->data;
	wave_out=(Wave*) calloc(obj->size, sizeof(Wave));

	for (i=0;i<obj->size;i++) {
		for (j=i;j<obj->size;j++) {
			wave_out[i]+=wave[j];
		}
	}

	free(obj->data);
	obj->data=wave_out;

	ESWEEP_ASSERT(correctFpException(obj), ERR_FP);
	return ERR_OK;
}

static inline Real __esweep_intern__sum(const esweep_object *obj) {
	Wave *wave;
	Polar *polar;
	Complex *cplx;
	Surface *surf;

	int i, size;
	Real sum=0.0;

	size=obj->size;
	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=0;i<size;i++)
				sum+=wave[i];
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			for (i=0;i<size;i++)
				sum+=polar[i].abs;
			break;
		case COMPLEX:
			cplx=(Complex*) obj->data;
			for (i=0;i<size;i++)
				sum+=CABS(cplx[i]);
			break;
		case SURFACE:
			surf=(Surface*) obj->data;
			for (i=0, size=surf->xsize*surf->ysize;i<size;i++)
				sum+=surf->z[i];
			break;
		default:
			return __NAN("");
	}
	return sum;
}

