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
 * include/esweep.h:
 * Main definitions and declarations.
 * 07.12.2009, jfab: 	- esweep can now be compiled with single precision floating point arithmetic
 * 			- added macro ESWEEP_OBJ_ISEMPTY(obj)
 * 			- the main structure is now named esweep_object
 * 21.12.2009, jfab:	changed audio handle to esweep_audio structure
 * 10.01.2010, jfab: 	- added a lot of documentation
 * 			- moved many internal parts to esweep_priv.h
 * 27.12.2010, jfab: 	- added esweep_getSurface()
 * 28.12.2010, jfab:	- changed esweep_get() that a range can be copied from the object
 * 			- merged esweep_getSurface() into esweep_get()
 * 04.01.2011, jfab:	- corrected documentation of esweep_add()/esweep_sub()
 * 			- changed copyright year
 */


#ifndef ESWEEP_H
#define ESWEEP_H

#include "esweep_priv.h"

/******************************************************
 * Creating, copying and destroying of esweep objects *
 ******************************************************/

/*
 * esweep_create()
 * Create an esweep object.
 *
 * PARAMETERS:
 * const char *type: name of the object type, maybe "wave", "complex", "polar" or "surface" (case sensitive)
 * int samplerate: sample rate of the new object (must be > 0)
 * int size: number of samples (must be >= 0 && <= ESWEEP_MAX_SIZE).
 *
 * RETURN:
 * Returns an esweep object or NULL if the creation failed.
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("polar", 44100, 1000);
 */
esweep_object *esweep_create(const char *type, int samplerate, int size);

/*
 * esweep_sparseSurface()
 * Creates a sparse surface inside an esweep object
 *
 * PARAMETERS:
 * esweep_object *obj: an esweep object, only type SURFACE is allowed
 * int xsize, ysize: size of the surface.
 *
 * DESCRIPTION:
 * The rows (y) and columns (x) must not be equally spaced. Each item of the surface will be initally set to 0.
 *
 * RETURN:
 * Returns an error code.
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("surface", 44100, 1000);
 * esweep_sparseSurface(obj, 1000, 100);
 */
int esweep_sparseSurface(esweep_object *obj, int xsize, int ysize);

/*
 * esweep_free()
 * Free an esweep object.
 *
 * PARAMETERS:
 * esweep_object *obj: an esweep object
 *
 * RETURN:
 * Returns always NULL
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("complex", 44100, 1000);
 * obj=esweep_free(obj); // obj=NULL
 */
esweep_object *esweep_free(esweep_object *obj);

/*
 * esweep_move()
 * Move the data of an esweep object. src and dst may be identical.
 *
 * PARAMETERS:
 * esweep_object *dst: destination object
 * esweep_object *src: src object
 * int dst_pos: start index to which the data is moved
 * int src_pos: start index from which the data is moved
 * int len: number of samples to move. The boundaries of src and dst are kept, i. e.
 * len is automatically reduced.
 *
 * DESCRIPTION:
 * The object type may not be Surface.
 *
 * RETURN:
 * Returns an error code.
 *
 * EXAMPLE:
 * esweep_object *dst=esweep_create("complex", 44100, 1000);
 * esweep_object *src=esweep_create("complex", 44100, 1000);
 * esweep_move(dst, src, 0, 100, 880)
 */
int esweep_move(esweep_object *dst, const esweep_object *src, int dst_pos, int src_pos, int *len);

/*
 * esweep_copy()
 * Same as esweep_move(), but the samples are copied instead of moved.
 */
int esweep_copy(esweep_object *dst, const esweep_object *src, int dst_pos, int src_pos, int *len);

/* TODO
 * esweep_rotate()
 * Rotates the object obj in a given dir by samples
 */
int esweep_rotate(esweep_object *obj, int samples, int dir);

/*
 * esweep_clone()
 * Creates a new esweep_object which is a identical copy of src
 *
 * PARAMETERS:
 * esweep_object *src: object to clone
 *
 * RETURN:
 * Returns NULL or a new esweep_object.
 *
 * EXAMPLE:
 * esweep_object *src=esweep_create("complex", 44100, 1000);
 * esweep_object *dst=esweep_clone(src); // content of dst and src are now identical
 */
esweep_object *esweep_clone(esweep_object *src);

/***********************************
 * Get information about an object *
 ***********************************/

/*
 * esweep_type()
 * Get the name of the type of obj.
 *
 * PARAMETERS:
 * const esweep_object *obj: esweep_object
 *
 * RETURN:
 * Returns an empty string or the name of the type
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("complex", 44100, 1000);
 * printf("%s", esweep_type(obj)); // prints "complex"
 */
int esweep_type(const esweep_object *obj, const char *type[]);

/*
 * esweep_samplerate()
 * Get the samplerate of obj
 *
 * PARAMETERS:
 * const esweep_object *obj: esweep_object
 *
 * RETURN:
 * Returns the samplerate of obj or an error code
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("complex", 44100, 1000);
 * printf("%u", esweep_samplerate(obj)); // prints "44100"
 */
int esweep_samplerate(const esweep_object *obj, int *samplerate);

/*
 * esweep_size()
 * Get the size of obj
 *
 * PARAMETERS:
 * const esweep_object *obj: esweep_object
 *
 * RETURN:
 * Returns the size of obj or an error code
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("complex", 44100, 1000);
 * printf("%i", esweep_size(obj)); // prints "1000"
 */
int esweep_size(const esweep_object *obj, int *size);

/*
 * esweep_surfaceSize()
 * Get the size of a surface object
 *
 * PARAMETERS:
 * const esweep_object *obj: esweep_object
 * int *x: variable where size of the x-dimension is stored
 * int *y: variable where size of the y-dimension is stored
 *
 * RETURN:
 * Returns an error code
 *
 * EXAMPLE:
 * esweep_surfaceSize(obj, &x, &y);
 */
int esweep_surfaceSize(const esweep_object *obj, int *x, int *y);

/*******************************************************
 * Modify or convert attributes and types of an object *
 *******************************************************/

/*
 * esweep_setSamplerate()
 * Set the samplerate of obj
 *
 * PARAMETERS:
 * esweep_object *obj: esweep_object
 * int samplerate: new samplerate, must be >0
 *
 * RETURN:
 * Returns an error code
 *
 * SEE ALSO:
 * esweep_resample()
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("complex", 44100, 1000);
 * esweep_setSamplerate(obj, 48000);
 * printf("%u", esweep_samplerate(obj)); // prints "48000"
 */
int esweep_setSamplerate(esweep_object *obj, int samplerate);

/*
 * esweep_setSize()
 * Set the size of obj
 *
 * PARAMETERS:
 * esweep_object *obj: esweep_object, does not work on type SURFACE
 * int size: new size
 *
 * DESCRIPTION:
 * If size is greater than the size of the object, the object will be zero-padded.
 *
 * RETURN:
 * Returns an error code
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("complex", 44100, 1000);
 * esweep_setSize(obj, 2000);
 * printf("%u", esweep_size(obj)); // prints "48000"
 */
int esweep_setSize(esweep_object *obj, int size);

/*
 * esweep_toWave()
 * Convert obj to type Wave
 *
 * PARAMETERS:
 * esweep_object *obj: esweep_object
 *
 * RETURN:
 * Returns an error code
 *
 * DESCRIPTION:
 * Conversion is only possible for the input types Complex and Polar. Surfaces cannot be converted.
 * From complex objects, the real part is used to create a Wave, from polar objects, the absolute is taken.
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("polar", 44100, 1000);
 * esweep_toWave(obj);
 * printf("%s", esweep_type(obj)); // prints "wave"
 */
int esweep_toWave(esweep_object *obj);

/*
 * esweep_toComplex()
 * Convert obj to type Complex
 *
 * PARAMETERS:
 * esweep_object *obj: esweep_object
 *
 * RETURN:
 * Returns an error code
 *
 * DESCRIPTION:
 * Conversion is only possible for the input types Wave and Polar. Surfaces cannot be converted.
 * Waves will be set to the real part of the new complex data, the imaginary part will be set to zero.
 * Polar samples will be converted according to the euler identity.
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("polar", 44100, 1000);
 * esweep_toComplex(obj);
 * printf("%s", esweep_type(obj)); // prints "complex"
 */
int esweep_toComplex(esweep_object *obj);

/*
 * esweep_toPolar()
 * Convert obj to type Polar
 *
 * PARAMETERS:
 * esweep_object *obj: esweep_object
 *
 * RETURN:
 * Returns an error code
 *
 * DESCRIPTION:
 * Conversion is only possible for the input types Wave and Complex. Surfaces cannot be converted.
 * Waves will be set to the absolute part of the new polar data, the argument (phase) will be set to zero.
 * Complex samples will be converted according to the euler identity.
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("complex", 44100, 1000);
 * esweep_toPolar(obj);
 * printf("%s", esweep_type(obj)); // prints "polar"
 */
int esweep_toPolar(esweep_object *obj);

int esweep_compress(esweep_object *obj, int factor);
/*
 * esweep_switchRI()
 * Exchange real and imaginary part of a complex object
 *
 * PARAMETERS:
 * esweep_object *obj: esweep_object
 *
 * RETURN:
 * Returns an error code
 *
 * DESCRIPTION:
 * Possible for types Complex and Polar. In case of Polar, the new phase (arg) is
 * arg:=pi/2-arg
 * This is similar to mirroring at the pi/4-diagonal. As this method is fast, it
 * bears a caveat: if the phase is unwrapped before, this may lead to wrong results
 * (see esweep_unwrapPhase()).
 */
int esweep_switchRI(esweep_object *obj);

/**************************************
 * Get specific values from an object *
 **************************************/

/*
 * esweep_unbuildWave()
 * Splits a WAVE-object into time and level
 *
 * PARAMETERS:
 * esweep_object *obj: esweep_object
 * Real *t[]: pointer to an array where the time is placed into
 * Real *a[]: pointer to an array where the level is placed into
 *
 * RETURN:
 * Returns an error code
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("wave", 48000, 1000);
 * Real *t=NULL, *a=NULL;
 * esweep_generate(obj, "ramp", 1, 1000); // generate a ramp from 1 to 1000
 * esweep_unbuildWave(obj, &t, &a); // t: 0, 1/48000, 2/48000, ..., 1000/48000; a: 1, 2, 3, ..., 1000
 */
int esweep_unbuildWave(const esweep_object *obj, Real *t[], Real *a[]);

/*
 * esweep_buildWave()
 * Build a Wave-object from real data
 *
 * PARAMETERS:
 * esweep_object *obj: esweep object
 * Real wave[]: array with the real data
 * int size: size of array
 *
 * DESCRIPTION:
 * size must be > 0. The function does a full copy of wave[]. The size of the object
 * will be adjusted. Only Wave-objects are allowed as input.
 *
 * RETURN:
 * Returns an error code
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("wave", 48000, 1000);
 * Real *wave=(Real*) calloc(2000, sizeof(Real));
 * // place some stuff in wave...
 * esweep_buildWave(obj, wave, 2000); // new size: 2000
 */
int esweep_buildWave(esweep_object *obj, const Real wave[], int size);

/*
 * esweep_unbuildComplex()
 * Splits a COMPLEX-object into real and imaginary part
 *
 * PARAMETERS:
 * esweep_object *obj: esweep_object
 * Real *real[]: pointer to an array where the real part is placed into
 * Real *imag[]: pointer to an array where the imaginary part is placed into
*
 * DESCRIPTION:
 * If real or imag are not empty, they are freed. In the end real and imag point to an internally generated array.
 *
 * RETURN:
 * Returns an error code
 */
int esweep_unbuildComplex(const esweep_object *obj, Real *real[], Real *imag[]);

/*
 * esweep_buildComplex()
 * Build a Complex-object from real data
 *
 * PARAMETERS:
 * esweep_object *obj: esweep object
 * Real real[]: array with the real part
 * Real imag[]: array with the imaginary part
 * int size: size of the arrays
 *
 * DESCRIPTION:
 * size must be > 0. The function does a full copy of real[] and imag[]. The size of the object
 * will be adjusted. Only Complex-objects are allowed as input.
 * Either real[] or imag[] may be NULL. Then only the non-NULL array is used, the other part is set to zero
 *
 * RETURN:
 * Returns an error code
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("complex", 48000, 1000);
 * Real *real=(Real*) calloc(2000, sizeof(Real));
 * Real *imag=(Real*) calloc(2000, sizeof(Real));
 * // place some stuff in real and imag...
 * esweep_buildComplex(obj, real, imag, 2000); // new size: 2000
 */
int esweep_buildComplex(esweep_object *obj, const Real real[], const Real imag[], int size);

/*
 * esweep_unbuildPolar()
 * Splits a Polar-object into absolute and argument
 *
 * PARAMETERS:
 * esweep_object *obj: esweep_object
 * Real *abs[]: pointer to an array where the absolute is placed into
 * Real *arg[]: pointer to an array where the argument is placed into
 *
 * DESCRIPTION:
 * If abs or arg are not empty, they are freed. In the end abs and arg point to an internally generated array.
 *
 * RETURN:
 * Returns an error code
 */
int esweep_unbuildPolar(const esweep_object *obj, Real *abs[], Real *arg[]);

/*
 * esweep_buildPolar()
 * Build a Polar-object from real data
 *
 * PARAMETERS:
 * esweep_object *obj: esweep object
 * Real abs[]: array with the absolute
 * Real arg[]: array with the argument (phase)
 * int size: size of the arrays
 *
 * DESCRIPTION:
 * size must be > 0. The function does a full copy of abs[] and arg[]. The size of the object
 * will be adjusted. Only Polar-objects are allowed as input.
 * Either abs[] or arg[] may be NULL. Then only the non-NULL array is used, the other part is set to zero
 *
 * RETURN:
 * Returns an error code
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("polar", 48000, 1000);
 * Real *abs=(Real*) calloc(2000, sizeof(Real));
 * Real *arg=(Real*) calloc(2000, sizeof(Real));
 * // place some stuff in abs and arg...
 * esweep_buildPolar(obj, abs, arg, 2000); // new size: 2000
 */
int esweep_buildPolar(esweep_object *obj, const Real abs[], const Real arg[], int size);

/*
 * esweep_unbuildSurface()
 * Splits a SURFACE-object into absolute and argument
 *
 * PARAMETERS:
 * esweep_object *obj: esweep_object
 * Real *x[]: pointer to an array where the x-dimension is placed into
 * Real *y[]: pointer to an array where the y-dimension is placed into
 * Real *z[]: pointer to an array where the z-dimension is placed into
 *
 * DESCRIPTION:
 * If x, y or z are not empty, they are freed. In the end x, y and z point to an internally generated array.
 *
 * RETURN:
 * Returns an error code
 */
int esweep_unbuildSurface(const esweep_object *obj, Real *x[], Real *y[], Real *z[]);

/*
 * esweep_buildSurface()
 * Build a Polar-object from real data
 *
 * PARAMETERS:
 * esweep_object *obj: esweep object
 * Real x[]: array with the x-vector
 * Real y[]: array with the y-vector
 * Real z[]: array with the z-vector
 * int xsize, ysize: size of the arrays
 *
 * DESCRIPTION:
 * x/ysize must be > 0, the size of the z-vector is calcualted by xsize*ysize.
 * The function does a full copy of x[], y[] and z[]. The size of the object
 * will be adjusted. Only Surface-objects are allowed as input.
 * Any vector may be NULL. The internal vectors are then initiliased with 0.
 * If all vectors are NULL, this function is similar to esweep_sparseSurface().
 *
 * RETURN:
 * Returns an error code
 *
 * EXAMPLE:
 * esweep_object *obj=esweep_create("surface", 48000, 1);
 * Real *x=(Real*) calloc(20, sizeof(Real));
 * Real *y=(Real*) calloc(10, sizeof(Real));
 * Real *z=(Real*) calloc(200, sizeof(Real));
 * // place some stuff in x, y, z...
 * esweep_buildSurface(obj, x, y, z, 20, 10); // new size: 20 x 10
 */
int esweep_buildSurface(esweep_object *obj,
			const Real x[],
			const Real y[],
			const Real z[],
			int xsize,
			int ysize);

/*
 * esweep_get()
 * Returns a part of an object.
 *
 * PARAMETERS:
 * const esweep_object *obj: esweep_object
 * int m: index, must be in range 0 <= m < obj->size (but see below)
 * int n: index, must be in range 0 <= n < obj->size (but see below)
 *
 * DESCRIPTION:
 * For Wave, Complex and Polar objects, it returns the part of obj between indices m and n, inclusive.
 *
 * For Surface objects, m corresponds to columns, and n to rows. The limits of above are not applying in this case.
 * If m is negative, then the n'th row is extracted. If n is negative, then m'th column is extracted.
 * If both m and n are positive, then the single value denoted by these coordinates is extracted
 *
 * RETURN:
 * esweep_object*
 *
 * SEE ALSO:
 * esweep_rotate()
 *
 * EXAMPLE1:
 * esweep_object *obj=esweep_create("wave", 48000, 1000);
 * Real t, a;
 * esweep_generate(obj, "ramp", 0, 100); // generate a ramp from 1 to 100
 * esweep_get(obj, 480);
 * printf("%s", esweep_type(obj)); // prints "wave"
 * esweep_unbuildWave(obj, &t, &a);
 * printf("Time: %f ms, Amplitude: %f", 1000*t, a); // prints "Time: 10 ms, Level: 48"
 *
 * EXAMPLE2:
 * esweep_object *obj=esweep_create("surface", 48000, 1);
 * esweep_sparseSurface(obj, 1000, 1000); // 1000 (cols) x 1000 (rows) surface
 * esweep_object *col=esweep_getSurface(obj, 100, -1); // 100th column, size 1 x 1000
 * esweep_object *row=esweep_getSurface(obj, -1, 5); // 5th row, size 1000 x 1
 * esweep_object *point=esweep_getSurface(obj, 100, 5); // 100th column and 5th row, size 1 x 1
 */
esweep_object* esweep_get(const esweep_object *obj, int m, int n);
int esweep_index(const esweep_object *obj, int index, Real *x, Real *a, Real *b);
/* TODO: create a corresponding function esweep_set() \
 * jfab: Do we really need it? */

/********
 * Math *
 ********/

/*
 * esweep_max()
 * esweep_min()
 * find the maximum/minimum of the given object
 *
 * PARAMETERS:
 * const esweep_object *obj: esweep_object
 *
 * RETURN:
 * Returns the maximum/minimum of obj or NaN
 *
 * DESCRIPTION:
 * For complex objects, the magnitude (sqrt(Re**2+Im**2)) is returned.
 * For polar objects, the absolute is returned.
 *
 * EXAMPLE:
 */
int esweep_max(const esweep_object *obj, int from, int to, Real *max);
int esweep_min(const esweep_object *obj, int from, int to, Real *min);

/*
 * esweep_maxPos()
 * esweep_minPos()
 * find the position of the maximum/minimum of the given object
 *
 * PARAMETERS:
 * const esweep_object *obj: esweep_object
 *
 * RETURN:
 * Returns the index requested by flags or an error code (<0)
 *
 * DESCRIPTION:
 * See esweep_max()
 *
 * EXAMPLE:
 */
int esweep_maxPos(const esweep_object *obj, int from, int to, int *pos);
int esweep_minPos(const esweep_object *obj, int from, int to, int *pos);

/*
 * esweep_avg()
 * esweep_sum()
 * Calculate the average and sum of the given obejct
 *
 * PARAMETERS:
 * const esweep_object *obj: esweep_object
 *
 * RETURN:
 * Returns the average/sum of obj or NaN
 *
 * DESCRIPTION:
 * For complex objects, the magnitude (sqrt(Re**2+Im**2)) is used.
 * For polar objects, the absolute is used.
 *
 * EXAMPLE:
 */
int esweep_avg(const esweep_object *obj, Real *avg);
int esweep_sum(const esweep_object *obj, Real *sum);
int esweep_sqsum(const esweep_object *obj, Real *sqsum);

/*
 * esweep_real()
 * esweep_imag()
 * Sets the imaginary or real part of the complex object to zero
 *
 * PARAMETERS:
 * const esweep_object *obj: esweep_object
 *
 * RETURN:
 * Returns an error code.
 *
 * DESCRIPTION:
 * Polar objects will be implicitly converted to complex objects (with real/imaginary part set to zero)
 *
 * EXAMPLE:
 */

int esweep_real(esweep_object *obj);
int esweep_imag(esweep_object *obj);

/*
 * esweep_abs()
 * esweep_arg()
 * Set the argument or absolute part of the object to zero
 *
 * PARAMETERS:
 * const esweep_object *obj: esweep_object
 *
 * RETURN:
 * Returns an error code.
 *
 * DESCRIPTION:
 * esweep_arg() works similar to esweep_imag(). esweep_abs() works similar to esweep_real(), except that
 * Surfaces and Waves are also allowed. Their type will not be converted,
 * so that it works like a common abs() function. With Surfaces, only the z-coordinate is affected.
 *
 * EXAMPLE:
 */

int esweep_abs(esweep_object *obj);
int esweep_arg(esweep_object *obj);

int esweep_clipLower(esweep_object *a, const esweep_object *b);
int esweep_clipUpper(esweep_object *a, const esweep_object *b);

/*
 * esweep_add()
 * esweep_sub()
 * Adds or subtracts object b to/from object a.
 *
 * PARAMETERS:
 * esweep_object *a: esweep_object
 * const esweep_object *b: esweep_object
 *
 * RETURN:
 * Returns an error code.
 *
 * DESCRIPTION:
 * These functions operate highly dependant of the type and size of their input. The mapping must always match.
 * operation independent of types (except SURFACE):
 * 	- If a (size N) is smaller than b (size M), then N samples of b are added/sub'ed to/from a,
 * 	sample by sample, starting from n=0.
 *	- If a (size N) is larger than b (size M), then M samples of b are added/sub'ed to/from a,
 *	sample by sample, starting from n=0.
 *	- If b has size 1, then this single sample is added/sub'ed to/from every sample of a.
 *	- If both objects are of the same type, then this will also be the resulting type.
 *	- Surfaces cannot be added/sub'ed to/from any other objects (including other Surfaces)
 *
 * Operation with different types:
 * a Complex, b Wave: b is added/sub'ed to/from the real part of a.
 * a Complex, b Polar: b is complex added/sub'ed to/from a
 *
 * a Wave, b Complex: b is complex added/sub'ed to/from a. Resulting type is Complex.
 * a Wave, b Polar: a is treated as the real part of a complex object,
 * 	thus the same behaviour as "a Complex, b Polar". Resulting type is Polar.
 *
 * a Polar, b another type: same as when a is Complex, except that a stays Polar.
 * 	a must be internally converted to Complex, and then back to Polar. This is very slow.
 * 	You should avoid using this constellation when possible.
 *
 * a Polar, b Polar: a and b are simplex added/sub'ed (ABS(a)+ABS(b) and ARG(a)+ARG(b)).
 * 	THIS IS NOT CORRECT MATH! But very helpful in lots of situations. If you want to really add two
 *	polar objects, convert a to Complex first, then after the operation back to Polar.
 *
 * a Surface, b another type:
 *	Only Wave-objects of size 1 may be added/sub'ed to/from a.
 *
 * EXAMPLE:
 */

int esweep_add(esweep_object *a, const esweep_object *b);
int esweep_sub(esweep_object *a, const esweep_object *b);

/*
 * esweep_mul()
 * esweep_div()
 * Multiplies or divides two objects
 *
 * PARAMETERS:
 * esweep_object *a: esweep_object
 * const esweep_object *b: esweep_object
 *
 * RETURN:
 * Returns an error code.
 *
 * DESCRIPTION:
 * These functions operate highly dependant of the type and size of their input. The mapping must always match.
 * operation independent of types (except SURFACE):
 * 	- If a (size N) is smaller than b (size M), then N samples of b are multiplied/divided with/through a,
 * 	sample by sample, starting from n=0.
 *	- If a (size N) is larger than b (size M), then M samples of b are multiplied/divided with/through a,
 *	sample by sample, starting from n=0.
 *	- If b has size 1, then every sample of a multiplied/divided with/through this single sample.
 *	- If both objects are of the same type, then this will also be the resulting type.
 *	- Surfaces cannot be multiplied/divided with/through any other objects (including other Surfaces)

 * Operation with different types:
 * a Complex, b Wave: the real part of a is multiplied/divided with/through b.
 * a Complex, b Polar: a is complex multiplied/divided with/through b.
 *
 * a Wave, b Complex: a is complex multiplied/divided with/through b. Resulting type is Complex.
 * a Wave, b Polar: a is treated as the real part of a complex object,
 * 	thus the same behaviour as "a Complex, b Polar". Resulting type is Polar.
 *
 * a Polar, b another type: same as when a is Complex, except that a stays Polar.
 *
 * a Surface, b another type:
 *	a may only be multiplied/divided with/through Wave-objects with size 1.
 *
 * EXAMPLE:
 */

int esweep_mul(esweep_object *a, const esweep_object *b);
int esweep_div(esweep_object *a, const esweep_object *b);

int esweep_ln(esweep_object *obj);
int esweep_lg(esweep_object *obj);

int esweep_exp(esweep_object *obj);
int esweep_pow(esweep_object *obj, Real x);

int esweep_schroeder(esweep_object *obj);

/* dsp */

int esweep_fft(esweep_object *out, esweep_object *in, esweep_object *table);
int esweep_ifft(esweep_object *out, esweep_object *in, esweep_object *table);
int esweep_createFFTTable(esweep_object *table, int size);

int esweep_convolve(esweep_object *in, esweep_object *filter, esweep_object *table);
int esweep_deconvolve(esweep_object *in, esweep_object *filter, esweep_object *table);

int esweep_delay(esweep_object *signal, esweep_object *line, int *offset);

int esweep_hilbert(esweep_object *obj, esweep_object *table);
int esweep_analytic(esweep_object *obj, esweep_object *table);

int esweep_integrate(esweep_object *obj);
int esweep_differentiate(esweep_object *obj);

int esweep_wrapPhase(esweep_object *obj);
int esweep_unwrapPhase(esweep_object *obj);

int esweep_groupDelay(esweep_object *obj);
int esweep_gDelayToPhase(esweep_object *obj);

int esweep_smooth(esweep_object *obj, Real factor);

int esweep_window(esweep_object *obj, const char *left_win, Real left_width, const char *right_win, Real right_width);
int esweep_restoreHermitian(esweep_object *obj);

int esweep_peakDetect(esweep_object *obj, Real threshold, int *peaks, int *n);
/* filter */
int esweep_filter(esweep_object *obj, esweep_object *filter[]);
int esweep_resample(esweep_object *out, esweep_object *in, esweep_object *filter[], Complex *carry);


esweep_object** esweep_createFilterFromCoeff(const char *type, Real gain, Real Qp, Real Fp, Real Qz, Real Fz, int samplerate);
esweep_object **esweep_createFilterFromArray(Real *num, Real *denom, int size, int samplerate);

esweep_object** esweep_cloneFilter(esweep_object *src[]);
int esweep_appendFilter(esweep_object *dst[], esweep_object *src[]);
int esweep_resetFilter(esweep_object *filter[]);
int esweep_freeFilter(esweep_object *filter[]);
int esweep_saveFilter(const char *filename, esweep_object *filter[]);
esweep_object **esweep_loadFilter(const char *filename);


/* generate */

int esweep_concat(esweep_object *dst, esweep_object *src, int dst_pos, int src_pos);
esweep_object *esweep_cut(esweep_object *obj, int start, int stop);
int esweep_pad(esweep_object *obj, int size);

/*
 * TODO: merge these functions in a single function with variable arguments
 * jfab, 09.01.2011: maybe with varargs; otherwise all functions must accept the same number of arguments
 * 		     and drop unused
 * jfab, 16.03.2011: no, this can be done in the Tcl interface. In plain C, we use one function for each generator
 */

/*
 * esweep_genSine()
 * Generate a sine wave
 *
 * PARAMETERS:
 * esweep_object *obj: esweep_object
 * Real freq: frequency
 * Real phase: phase
 *
 * RETURN:
 * Returns the number of generated periods or an error code.
 *
 * DESCRIPTION:
 * The frequency is automatically adjusted so that always an integer multiple of one period
 * matches in obj. If obj is Complex the analytical wave is created (Real part: sine, Imaginary part: cosine).
 */

int esweep_genSine(esweep_object *obj, Real freq, Real phase, int *periods);
int esweep_genLogsweep(esweep_object *obj, Real locut, Real hicut, const char *spec, Real *sweep_rate);
int esweep_genDirac(esweep_object *obj, Real delay);
int esweep_genLinsweep(esweep_object *obj, Real locut, Real hicut, const char *spec, Real *sweep_rate);
int esweep_genNoise(esweep_object *output, Real locut, Real hicut, const char *spec);

/* file handling */

/*
 * esweep_toAscii()
 * Write the object's data in an ASCII file
 *
 * PARAMETERS:
 * const char *filename: name of output file
 * const esweep_object *obj: esweep object
 * const char *comment: comment placed in the file before the data, may be NULL
 */
int esweep_toAscii(const char *filename, const esweep_object *obj, const char *comment);
int esweep_save(const char *filename, const esweep_object *input, const char *meta);
int esweep_load(esweep_object *output, const char *filename, char **meta);

/* audio */

esweep_audio* esweep_audioOpen(const char *device);
int esweep_audioQuery(const esweep_audio *handle, const char *query, int *result);
int esweep_audioConfigure(esweep_audio *handle, const char *param, int value);
int esweep_audioSync(esweep_audio *handle);
int esweep_audioPause(esweep_audio *handle);
int esweep_audioOut(esweep_audio *handle, esweep_object *out[], int channels, int *offset);
int esweep_audioIn(esweep_audio *handle, esweep_object *in[], int channels, int *offset);
int esweep_audioClose(esweep_audio *handle);

/* surface */

int esweep_csd(esweep_object *out, esweep_object *in, Real time_step, int steps, Real rise_time, Real smooth_factor);
int esweep_cbsd(esweep_object *out, esweep_object *in, Real f1, Real f2, int resolution, int periods, int steps, char time_shift);

int esweep_addToSurface(esweep_object *obj, esweep_object *b, char axis, int index, Real dep);

#endif /* ESWEEP_H */
