/* 
 * Tcl-Module as a wrapper around the esweep library
 * the functions are added when they are needed
 */

#ifndef DLL_EXPORT
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#	define DLL_EXPORT  extern __declspec(dllexport)
# else
# 	define DLL_EXPORT 
# endif
#endif

#ifndef STDCALL
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   define STDCALL __stdcall
# else
#   define STDCALL
# endif 
#endif
#define STDCALL 

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "esweep.h"
#include "esweep_priv.h"

#ifndef __ESWEEP_VEE_WRAP_H
#define __ESWEEP_VEE_WRAP_H

/* often used macros */

/* Command prototypes */

/* mem */
DLL_EXPORT int32_t STDCALL esweepCreate(const char *type, int32_t samplerate, int32_t size); 
DLL_EXPORT int32_t STDCALL esweepBuildWave(int32_t obj, const double *wave, int32_t size);

/* base */

/* conv */

/* generate */

/* dsp */

/* filter */

/* file */

/* math */

/* audio */
DLL_EXPORT int32_t STDCALL esweepAudioOpen(const char *device);
DLL_EXPORT int32_t STDCALL esweepAudioQuery(int32_t hdl, const char *query);
DLL_EXPORT int32_t STDCALL esweepAudioConfigure(int32_t hdl, const char *param, int32_t value);
DLL_EXPORT int32_t STDCALL esweepAudioSync(int32_t hdl);
DLL_EXPORT int32_t STDCALL esweepAudioPause(int32_t hdl);
DLL_EXPORT int32_t STDCALL esweepAudioOut(int32_t hdl, int32_t **out, int32_t channels, int *offset);
DLL_EXPORT int32_t STDCALL esweepAudioIn(int32_t hdl, int32_t **in, int32_t channels, int *offset);
DLL_EXPORT int32_t STDCALL esweepAudioClose(int32_t hdl);

#endif // __ESWEEP_VEE_WRAP
