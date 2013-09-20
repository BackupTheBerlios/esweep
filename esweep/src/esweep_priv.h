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
 * include/esweep_priv.h:
 * Internal definitions and declarations
 */

#ifndef __ESWEEP_PRIV_H
#define __ESWEEP_PRIV_H

#include <errno.h>
#include <fenv.h>
#include <float.h>
#include <math.h>

/* Errors */
#define ERR_OK			0
#define ERR_UNKNOWN		128
#define ERR_NOT_ON_THIS_TYPE 	1
#define ERR_EMPTY_OBJECT	2
#define ERR_DIFF_MAPPING	3
#define ERR_BAD_ARGUMENT	4
#define ERR_BAD_PARAMETER	4
#define ERR_DIFF_TYPES		5
#define ERR_OP_NOT_ALLOWED	6
#define ERR_MAX_SIZE		7
#define ERR_OBJ_NOT_VALID	8
#define ERR_OBJ_IS_NULL		9
#define ERR_SIZE_MISMATCH 	10
#define ERR_MALLOC		11
#define ERR_INDEX_OUTOFRANGE	12
#define ERR_MAX_SIZE_EXCEEDED	13
#define ERR_FP	14

extern char errmsg[256];

/*
 * Define the maximum size of an object in samples
 * The default is 2^24, which gives an complex object of 2 GB size.
 */
#ifndef ESWEEP_MAX_SIZE
	#define ESWEEP_MAX_SIZE 0x00F00000
#endif

/*
 * Helper macros
 */

/*
 * Own version of assert(), does not end the program, but puts an error message in errmsg and prints it on stderr
 */
#ifndef NDEBUG
	#if __ISO_C_VISIBLE >= 1999
		#ifdef ESWEEP_ERROR_NOEXIT
			#define	ESWEEP_ASSERT(e, ret) if (!(e)) {__esweep_assert2(__FILE__, __LINE__, __func__, #e); return ret;}
		#else
			#define	ESWEEP_ASSERT(e, ret) if (!(e))
		#endif
	#else
		#ifdef ESWEEP_ERROR_NOEXIT
			#define	ESWEEP_ASSERT(e, ret) if (!(e)) {__esweep_assert(__FILE__, __LINE__, #e); return ret;}
		#else
			#define	ESWEEP_ASSERT(e, ret) if (!(e)) {__esweep_assert(__FILE__, __LINE__, #e); exit((int) ret);}
		#endif
	#endif /* __ISO_C_VISIBLE */
#else
	#define ESWEEP_ASSERT(e, ret)
#endif /* NDEBUG */

int __esweep_assert2(const char *file, int line, const char *func, const char *assertion);
int __esweep_assert(const char *file, int line, const char *assertion);

/*
 * Shortcuts for ESWEEP_ASSERT
 */

/*
 * Check if the given esweep object is an empty container. Check the samplerate, too, so this is a stricter variant of ESWEEP_OBJ_ISVALID
 */
#define ESWEEP_OBJ_NOTEMPTY(obj, ret) ESWEEP_ASSERT(obj!=NULL \
						 && obj->data!=NULL \
						 && obj->size>0 \
						 && obj->samplerate > 0, ret);

/* Check if ptr is NULL */
#define ESWEEP_NOTNULL(ptr, ret) ESWEEP_ASSERT(ptr != NULL, ret);

/* Validate an esweep object. A valid object's pointer is non-NULL, and its size is 0 and data pointer is NULL,
 * or its size is non-zero and the data pointer is non-NULL, and the samplerate is >0 */
#define ESWEEP_OBJ_ISVALID(obj, ret) ESWEEP_ASSERT(obj!=NULL && \
					((obj->data==NULL && obj->size==0) || \
					(obj->data!=NULL && obj->size>0)) && \
					obj->samplerate>0, ret);

#define ESWEEP_OBJ_ISVALID_SURFACE(obj, surf, ret1, ret2) ESWEEP_ASSERT(obj->type==SURFACE, ret1); \
						    ESWEEP_ASSERT(obj->data!=NULL && obj->size==1, ret2); \
						    ESWEEP_ASSERT( \
						    (surf->xsize>0 && surf->ysize>0 \
							&& surf->x!=NULL && surf->y!=NULL && surf->z!=NULL) || \
						    (surf->xsize==0 && surf->ysize==0 \
							&& surf->x==NULL && surf->y==NULL && surf->z==NULL), ret2);
/*
 * Check if the two objects a and b have the same mapping (i. e. samplerate)
 */
#define ESWEEP_SAME_MAPPING(a, b, ret) ESWEEP_ASSERT(a->samplerate == b->samplerate, ret);

/*
 * Check that index is in range
 */
#define ESWEEP_INDEX_INRANGE(a, index, ret) ESWEEP_ASSERT(index >= 0 && index < a->size, ret);

/*
 * This macro creates a valid error message if a function can not handle the type of an object
 */
#if __ISO_C_VISIBLE >= 1999
	#define ESWEEP_NOT_THIS_TYPE(type, ret) snprintf(errmsg, 256, "%s:%i: %s: Type %s not allowed", __FILE__, __LINE__, __func__, #type); \
	fprintf(stderr, errmsg); \
	return ret;
#else
	#define ESWEEP_NOT_THIS_TYPE(type, ret) snprintf(errmsg, 256, "%s:%i: Type %s not allowed", __FILE__, __LINE__, #type); \
	fprintf(stderr, errmsg); \
	return ret;
#endif


/*
 * Allocate memory and check if the resulting pointer is NULL. If it is NULL, exit the application with ENOMEM.
 * This macro should always be used when allocating new memory. Only for internal usage, external developers
 * shall use esweep_create();
 */

#define ESWEEP_MALLOC(ptr, nmemb, size, ret) \
	ESWEEP_ASSERT(nmemb*size > 0 && nmemb < ESWEEP_MAX_SIZE, ret); \
	ptr=calloc(nmemb, size);\
	ESWEEP_ASSERT(ptr != NULL, ret);

/*
 * Allocate the internal surface structure
 */
#define ESWEEP_MALLOC_SURFACE(x, y, z, xsize, ysize, zsize, ret) \
	ESWEEP_MALLOC(x, xsize, sizeof(Real), ret); \
	ESWEEP_MALLOC(y, ysize, sizeof(Real), ret); \
	ESWEEP_MALLOC(z, zsize, sizeof(Real), ret);

#ifndef NDEBUG_PRINT
#define ESWEEP_DEBUG_PRINT(msg, ...) fprintf(stderr, "%s: "msg, __func__, __VA_ARGS__);
#else
#define ESWEEP_DEBUG_PRINT(msg, ...)
#endif

/*
 * esweep may be compiled with single precision floating point arithmetic.
 */
#ifdef REAL32
	typedef float Real;
#else
	typedef double Real;
#endif /* REAL32 */

#ifndef u_int
       #define u_int unsigned int
#endif

#ifndef u_char
       #define u_char unsigned char
#endif

#if !(__BSD_VISIBLE || __XPG_VISIBLE >= 400)
#ifndef u_int16_t
		typedef unsigned __int16 u_int16_t; /* MinGW env */
#endif

#ifndef u_int32_t
		typedef unsigned __int32 u_int32_t; /* MinGW env */
#endif

#ifndef u_int64_t
		typedef unsigned __int64 u_int64_t; /* MinGW env */
#endif
#ifndef _STDINT_H
#ifndef int16_t
		typedef __int16 int16_t; /* MinGW env */
#endif

#ifndef int32_t
		typedef __int32 int32_t; /* MinGW env */
#endif

#ifndef int64_t
		typedef __int64 int64_t; /* MinGW env */
#endif
#endif /* STDINT_H */
#endif /* BSD_VISIBLE */

#if !(__BSD_VISIBLE || __XPG_VISIBLE >= 400)
		#define STRCPY(dst, src, size) strncpy(dst, src, size);
		#define STRCAT(dst, src, size) nstrncat(dst, src, size);
#else
		#define STRCPY(dst, src, size) strlcpy(dst, src, size);
		#define STRCAT(dst, src, size) strlcat(dst, src, size);
#endif

#ifdef _WIN32
		#define STRLEN(str, size) strlen(str);
#else
    #define STRLEN(str, size) strnlen(str, size);
#endif



/* endianess handling */

/* generic byte swapper */
/*
 * see LICENSE
 */

#if !(__BSD_VISIBLE || __XPG_VISIBLE >= 400)

#define _LITTLE_ENDIAN	1234
#define _BIG_ENDIAN	4321

#define __swap16gen(x) ({				\
	u_int16_t __swap16gen_x = (x);					\
 			   				   						\
	(u_int16_t)((__swap16gen_x & 0xff) << 8 |		\
	    (__swap16gen_x & 0xff00) >> 8);				\
})

#define __swap32gen(x) ({				\
	u_int32_t __swap32gen_x = (x);					\
													\
	(u_int32_t)((__swap32gen_x & 0xff) << 24 |		\
	    (__swap32gen_x & 0xff00) << 8 |				\
	    (__swap32gen_x & 0xff0000) >> 8 |			\
	    (__swap32gen_x & 0xff000000) >> 24);		\
})

#define __swap64gen(x) ({					 \
	u_int64_t __swap64gen_x = (x);						 \
 			   				   						 	 \
	(u_int64_t)((__swap64gen_x & 0xff) << 56 |			 \
	    (__swap64gen_x & 0xff00ULL) << 40 |				 \
	    (__swap64gen_x & 0xff0000ULL) << 24 |			 \
	    (__swap64gen_x & 0xff000000ULL) << 8 |			 \
	    (__swap64gen_x & 0xff00000000ULL) >> 8 |		 \
	    (__swap64gen_x & 0xff0000000000ULL) >> 24 |		 \
	    (__swap64gen_x & 0xff000000000000ULL) >> 40 |	 \
	    (__swap64gen_x & 0xff00000000000000ULL) >> 56);	 \
})

#if _BYTE_ORDER == _LITTLE_ENDIAN

#define htobe16(x) __swap16gen(x)
#define htobe32(x) __swap32gen(x)
#define htobe64(x) __swap64gen(x)
#define betoh16(x) __swap16gen(x)
#define betoh32(x) __swap32gen(x)
#define betoh64(x) __swap64gen(x)

#define htole16(x) ((u_int16_t)(x))
#define htole32(x) ((u_int32_t)(x))
#define htole64(x) ((u_int64_t)(x))
#define letoh16(x) ((u_int16_t)(x))
#define letoh32(x) ((u_int32_t)(x))
#define letoh64(x) ((u_int64_t)(x))

#define htons(x) __swap16gen(x)
#define htonl(x) __swap32gen(x)
#define ntohs(x) __swap16gen(x)
#define ntohl(x) __swap32gen(x)

#endif /* _BYTE_ORDER */

#if _BYTE_ORDER == _BIG_ENDIAN

#define htole16(x) __swap16gen(x)
#define htole32(x) __swap32gen(x)
#define htole64(x) __swap64gen(x)
#define letoh16(x) __swap16gen(x)
#define letoh32(x) __swap32gen(x)
#define letoh64(x) __swap64gen(x)

#define htobe16(x) ((u_int16_t)(x))
#define htobe32(x) ((u_int32_t)(x))
#define htobe64(x) ((u_int64_t)(x))
#define betoh16(x) ((u_int16_t)(x))
#define betoh32(x) ((u_int32_t)(x))
#define betoh64(x) ((u_int64_t)(x))

#define htons(x) ((u_int16_t)(x))
#define htonl(x) ((u_int32_t)(x))
#define ntohs(x) ((u_int16_t)(x))
#define ntohl(x) ((u_int32_t)(x))

#endif /* _BYTE_ORDER */
#endif /* BSD_VISIBLE */

#ifdef __GNUC__
	#if (__GNUC__ > 4 && __GNUC_MINOR__ > 2)
		#define __EXTERN_FUNC__ extern
	#else
		#define __EXTERN_FUNC__
	#endif
#endif

/* specials for FP values
 * you need to use a union for this to work:
 * typedef union {
 *	Real r;
 *	u_int32_t ui32_t;
 *	u_int64_t ui64_t;
 * } conv_t;
 * Then do this:
 * conv_t conv;
 * conv.r = 2f;
 * conv.ui32=htonf(conv.ui32);
 * This avoids strict-aliasing warnings
 */
#if 0
#define htond(x) htobe64(x);
#define ntohd(x) betoh64(x);
#define htonf(x) htonl(x);
#define ntohf(x) ntohl(x);

#ifdef REAL32
#define htonr(x) htonf(x)
#define ntohr(x) ntohf(x)
#else
#define htonr(x) htond(x)
#define ntohr(x) ntohd(x)
#endif
#endif
/* Random number generator */
#if !(__BSD_VISIBLE || __XPG_VISIBLE >= 400)
		#define random(x) rand(x)
		#define srandom(x) srand(x)
#endif

/* The types that esweep can handle */

enum __esweep_type {
	WAVE,
	POLAR,
	COMPLEX,
	SURFACE,
	UNKNOWN
};

/* The main data structure */

typedef struct __esweep_object {
	/* Type */
	enum __esweep_type type;
	/* Samplerate */
	int samplerate;
	/*
	Number of samples
	For type SURFACE always 1
	*/
	int size;
	/* The raw data, my be NULL, but not for surface. Then a struct surface is allocated.  */
	void *data;
} esweep_object;

typedef	int (*audio_query_ptr)(const void*, const char*, int*);
typedef	int (*audio_configure_ptr)(void*, const char*, int);
typedef	int (*audio_sync_ptr)(const void*);
typedef	int (*audio_pause_ptr)(const void*);
typedef	int (*audio_seek_ptr)(const void*, int);
typedef	int (*audio_out_ptr)(const void*, esweep_object**, u_int, int);
typedef	int (*audio_in_ptr)(const void*, esweep_object**, u_int, int);
typedef	int (*audio_close_ptr)(void*);
/*
 * This structure is used during Audio I/O.
 */
typedef struct __esweep_audio {
	/* Handle of the soundcard, file or disc */
	int au_hdl;

	/* Pointer to an implementation defined data structure */
	char *pData;

	/* These variables map the internal state of the audio device driver */
	u_int samplerate;
#define ESWEEP_MAX_CHANNELS 32
	u_int play_channels;
	u_int rec_channels;

	/* precision in bits/sample */
	u_int precision;

	/*
	 * Size of each audio block in samples
	 */
	u_int framesize;

	/*
	 * Now follows low level information, normally not accesible by the user
	 */

	/* bytes / sample */
	u_int bps;
	/* alignment, 1 means aligned to the MSB */
	u_int msb;

	/*
	 * Block sizes in Bytes. This is the size which is actually written to the device.
	 */
	u_int play_blocksize;
	u_int rec_blocksize;

	/*
	 * Hold info if play or record are paused.
	 */
	u_char play_pause;
	u_char rec_pause;

	/*
	 * Dither values, which are kept through successive calls to
	 * esweep_audioOut(), independent for each channel
	 */
	Real *dither;

	/* Audio buffer */
	char *ibuf, *obuf;

	/* file information */
	u_int32_t file_size;
	u_int32_t data_size;
	u_int32_t data_block_start;

	/* callback function pointers, set by the corresponding open function*/
	audio_query_ptr audio_query;
	audio_configure_ptr audio_configure;
	audio_sync_ptr audio_sync;
	audio_pause_ptr audio_pause;
	audio_seek_ptr audio_seek;
	audio_out_ptr audio_out;
	audio_in_ptr audio_in;
	audio_close_ptr audio_close;
} esweep_audio;

typedef struct __Complex {
	Real real;
	Real imag;
} Complex;

/* struct Polar */
typedef struct __Polar {
	Real abs;
	Real arg;
} Polar;

/* struct Surface
 * x: columns, y: rows
 * get the element of z in the i'th column and j'th row:
 * z[i+j*xsize]
 * */
typedef struct __Surface {
	/* data vectors; may be NULL when xsize=ysize=0 */
	Real *x;
	Real *y;
	Real *z;
	int xsize;
	int ysize;
	/* zsize=xsize*ysize */
} Surface;

/* Typedef for Real */
typedef Real Wave;

/* Conversion between Complex, Polar and Real data */
__EXTERN_FUNC__ void c2r(Real* output, Complex* input, int input_size); /* Complex to Real */
__EXTERN_FUNC__ void r2c(Complex* output, Real* input, int input_size); /* Real to Complex */

__EXTERN_FUNC__ void p2r(Real* output, Polar* input, int input_size); /* Polar to Real */
__EXTERN_FUNC__ void r2p(Polar* output, Real* input, int input_size); /* Real to Polar */
__EXTERN_FUNC__ void abs2r(Real* output, Polar* input, int input_size); /* absolute of Polar to Real */

/* these two functions may be used in-place */
__EXTERN_FUNC__ void p2c(Complex *output, Polar *input, int input_size); /* Polar 2 Complex */
__EXTERN_FUNC__ void c2p(Polar *output, Complex *input, int input_size); /* Complex 2 Polar */

/* And some conversion macros */
#define ESWEEP_CONV_WAVE2COMPLEX(obj, cpx) 	ESWEEP_MALLOC(cpx, obj->size, sizeof(Complex), ERR_MALLOC); \
						r2c(cpx, (Wave*) obj->data, obj->size); \
					 	free(obj->data); \
						obj->type=COMPLEX; \
						obj->data=cpx;

#define ESWEEP_CONV_WAVE2POLAR(obj, polar) 	ESWEEP_MALLOC(polar, obj->size, sizeof(Polar), ERR_MALLOC); \
						r2p(polar, (Wave*) obj->data, obj->size); \
					 	free(obj->data); \
						obj->type=POLAR; \
						obj->data=polar;

/* Math macros */

/* test for and correct floating point exceptions */
__EXTERN_FUNC__ int correctFpException(esweep_object* obj);

/*
 * Math function definitions for single or double FP arithmetic
 */

#ifndef M_PI
	#define M_PI 3.14159265358979323846264338327
#endif

#ifndef M_PI_2
	#define M_PI_2 6.28318530717958647692528676654
#endif

#ifdef REAL32

#define CABS(cpx) hypotf(cpx.real, cpx.imag)
#define FABS(real) fabsf(real)
#define COS(real) cosf(real)
#define SIN(real) sinf(real)
#define ATAN2(cpx) atan2f(cpx.imag, cpx.real)
#define LN(real) logf(real)
#define LG(real) log10f(real)
#define EXP(real) expf(real)
#define POW(x, y) powf(x, y)
#define __NAN(x) nanf(x)

#else

#define CABS(cpx) hypot(cpx.real, cpx.imag)
#define FABS(real) fabs(real)
#define COS(real) cos(real)
#define SIN(real) sin(real)
#define ATAN2(cpx) atan2(cpx.imag, cpx.real)
#define LN(real) log(real)
#define LG(real) log10(real)
#define EXP(real) exp(real)
#define POW(x, y) pow(x, y)
#define __NAN(x) nan(x)

#endif /* REAL32 */

/*********************
 * Complex with Real *
 *********************/

/* add real number b to complex number a; result is in a */
#define ESWEEP_MATH_CR_ADD(a, b) a.real=a.real + b;
/* subtract real number b from complex number a; result is in a */
#define ESWEEP_MATH_CR_SUB(a, b) a.real=a.real - b;
/* multiply complex number a with real number b; result is in a */
#define ESWEEP_MATH_CR_MUL(a, b) a.imag=a.imag * b; a.real=a.real * b;
/* divide complex number a by real number b; result is in a */
#define ESWEEP_MATH_CR_DIV(a, b) a.real=a.real / b; a.imag=a.imag / b;

/************************
 * Complex with Complex *
 ************************/

/* add complex number b to complex number a; result is in a */
#define ESWEEP_MATH_CC_ADD(a, b) a.real=a.real + b.real; a.imag=a.imag + b.imag;
/* subtract complex number b from complex number a; result is complex and in out */
#define ESWEEP_MATH_CC_SUB(a, b) a.real=a.real - b.real; a.imag=a.imag - b.imag;
/* multiply complex number a with complex number b; result is in a; needs a temporary real storage */
#define ESWEEP_MATH_CC_MUL(a, b) tmp=a.real * b.real - a.imag * b.imag; a.imag=a.imag * b.real + a.real * b.imag; a.real=tmp;
/* divide complex number a by complex number b; result is complex and in out; needs two temporary real storages (denom, tmp)*/
#define ESWEEP_MATH_CC_DIV(a, b) denom=b.real * b.real - b.imag * b.imag; \
						tmp=(a.real * b.real + a.imag * b.imag) / denom; \
						a.imag=(a.imag * b.real - a.real * b.imag) / denom; \
						a.real=tmp;

/************************
 * Complex with Polar *
 ************************/

/* add polar number b to complex number a; result is in a */
#define ESWEEP_MATH_CP_ADD(a, b) a.real=a.real + b.abs*cos(b.arg); a.imag=a.imag + b.abs*sin(b.arg);
/* subtract polar number b from complex number a; result is in a */
#define ESWEEP_MATH_CP_SUB(a, b) a.real=a.real - b.abs*cos(b.arg); a.imag=a.imag - b.abs*sin(b.arg);

/********************
 * Polar with Polar *
 ********************/

/* multiply two polar numbers; result is in a */
#define ESWEEP_MATH_PP_MUL(a, b) a.abs=a.abs * b.abs; a.arg = a.arg + b.arg;
/* divide two polar numbers; result is in a */
#define ESWEEP_MATH_PP_DIV(a, b) a.abs=a.abs / b.abs; a.arg = a.arg - b.arg;
/* divide two polar numbers; result is in a */
#define ESWEEP_MATH_PP_SUB(a, b) a.abs=a.abs - b.abs; a.arg = a.arg - b.arg;
/* divide two polar numbers; result is in a */
#define ESWEEP_MATH_PP_ADD(a, b) a.abs=a.abs + b.abs; a.arg = a.arg + b.arg;

/*******************
 * Polar with Real *
 *******************/

#define ESWEEP_MATH_PR_MUL(a, b) a.abs=a.abs * b;
#define ESWEEP_MATH_PR_DIV(a, b) a.abs=a.abs / b;

#endif /* __ESWEEP_PRIV_H */
