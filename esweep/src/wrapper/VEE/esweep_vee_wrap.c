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
 * esweep_vee_wrap.c
 * Wrapper around the esweep library for Tcl
 * 03.10.2011: initial creation
*/

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
#include "esweep_vee_wrap.h"

#ifdef __cplusplus
extern "C" {
#endif

DLL_EXPORT int32_t STDCALL esweepCreate(const char *type, int32_t samplerate, int32_t size) {
	return (int32_t) esweep_create(type, samplerate, size); 
}

DLL_EXPORT int32_t STDCALL esweepBuildWave(int32_t obj, const double *wave, int32_t size) {
	return (int32_t) esweep_buildWave((esweep_object*) obj, wave, size); 
}

DLL_EXPORT int32_t STDCALL esweepAudioOpen(const char *device) {
	return (int32_t) esweep_audioOpen(device); 
}

DLL_EXPORT int32_t STDCALL esweepAudioQuery(int32_t hdl, const char *query) {
	int result=-1; 
	esweep_audioQuery((const esweep_audio*) hdl, query, &result); 
	return result; 
}

DLL_EXPORT int32_t STDCALL esweepAudioConfigure(int32_t hdl, const char *param, int32_t value) {
	return esweep_audioConfigure((esweep_audio*) hdl, param, value); 
}

DLL_EXPORT int32_t STDCALL esweepAudioSync(int32_t hdl) {
	return esweep_audioSync((esweep_audio*) hdl); 
}
	
DLL_EXPORT int32_t STDCALL esweepAudioPause(int32_t hdl) {
	return esweep_audioPause((esweep_audio*) hdl); 
}
	
DLL_EXPORT int32_t STDCALL esweepAudioOut(int32_t hdl, int32_t **out, int32_t channels, int *offset) {
	return esweep_audioOut((esweep_audio*) hdl, (esweep_object**) out, channels, offset); 
}
	
DLL_EXPORT int32_t STDCALL esweepAudioIn(int32_t hdl, int32_t **in, int32_t channels, int *offset) {
	return esweep_audioOut((esweep_audio*) hdl, (esweep_object**) in, channels, offset); 
}
	
DLL_EXPORT int32_t STDCALL esweepAudioClose(int32_t hdl) {
	return esweep_audioClose((esweep_audio*) hdl); 
}

#ifdef __cplusplus
}
#endif
