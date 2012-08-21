/*
 * Copyright (c) 2007-2009 Jochen Fabricius <jfab@berlios.de>
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

/* Esweep high-level audio I/O. */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esweep_priv.h"
#include "audio_device.h"
#include "audio_file.h"

esweep_audio *esweep_audioOpen(const char *device) {
	esweep_audio *handle;
	char *type, *path=(char*) calloc(strlen(device)+1, sizeof(char));

	ESWEEP_ASSERT(strlen(device) > 0, NULL); 

	STRCPY(path, device, strlen(device)+1); 
	type=strtok(path, ":"); 
	path=strtok(NULL, ":"); 

	if (strcmp(type, "audio") == 0) {
		/* open soundcard */
		ESWEEP_ASSERT((handle=audio_device_open(path))!=NULL, NULL); 
	} else if (strcmp(type, "file") == 0) {
		/* open file */
		ESWEEP_ASSERT((handle=audio_file_open(path))!=NULL, NULL); 
	} else {
		snprintf(errmsg, 256, "%s:%i: esweep_audioOpen(): unknown audio type \"%s\"\n", __FILE__, __LINE__, type);
		fprintf(stderr, errmsg);
		handle=NULL; 
	}

	free(type); 
	return handle;
}

int esweep_audioQuery(const esweep_audio *handle, const char *query, int *result) {
	ESWEEP_ASSERT(handle != NULL, ERR_BAD_ARGUMENT);
        ESWEEP_ASSERT(handle->au_hdl >= 0, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(query != NULL, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(result != NULL, ERR_BAD_ARGUMENT); 
	
	ESWEEP_ASSERT(handle->audio_query((void*) handle, query, result) == 0, ERR_UNKNOWN);
	return ERR_OK; 
}

int esweep_audioConfigure(esweep_audio *handle, const char *param, int value) {
	ESWEEP_ASSERT(handle != NULL, ERR_BAD_ARGUMENT);
        ESWEEP_ASSERT(handle->au_hdl >= 0, ERR_BAD_ARGUMENT);
	 
	ESWEEP_ASSERT(param != NULL, ERR_BAD_ARGUMENT); 
	
	ESWEEP_ASSERT(handle->audio_configure((void*) handle, param, value) == 0, ERR_UNKNOWN);
	return ERR_OK; 
}

int esweep_audioSync(esweep_audio *handle) {
	ESWEEP_ASSERT(handle != NULL, ERR_BAD_ARGUMENT);
        ESWEEP_ASSERT(handle->au_hdl >= 0, ERR_BAD_ARGUMENT);

	ESWEEP_ASSERT(handle->audio_sync((void*) handle) == 0, ERR_UNKNOWN);
	return ERR_OK; 
}

int esweep_audioPause(esweep_audio *handle) {
	ESWEEP_ASSERT(handle != NULL, ERR_BAD_ARGUMENT);
        ESWEEP_ASSERT(handle->au_hdl >= 0, ERR_BAD_ARGUMENT);

	ESWEEP_ASSERT(handle->audio_pause((void*) handle) == 0, ERR_UNKNOWN);
	return ERR_OK; 
}

int esweep_audioOut(esweep_audio *handle, esweep_object *out[], int channels, int *offset) {
	u_int i; 
	ESWEEP_ASSERT(handle != NULL, ERR_BAD_ARGUMENT);
        ESWEEP_ASSERT(handle->au_hdl >= 0, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(out != NULL, ERR_OBJ_IS_NULL); 
	ESWEEP_ASSERT(channels > 0, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(*offset >= 0, ERR_BAD_ARGUMENT); 

	for (i=0; i<channels; i++) {
		ESWEEP_OBJ_NOTEMPTY(out[i], ERR_EMPTY_OBJECT); 
		ESWEEP_ASSERT(out[i]->type == WAVE || out[i]->type == COMPLEX, ERR_NOT_ON_THIS_TYPE);
	}

	ESWEEP_ASSERT((*offset=handle->audio_out((void*) handle, out, channels, *offset)) >= 0, ERR_UNKNOWN);
	return ERR_OK;
}

int esweep_audioIn(esweep_audio *handle, esweep_object *in[], int channels, int *offset) {
	u_int i; 

	ESWEEP_ASSERT(handle != NULL, ERR_BAD_ARGUMENT);
        ESWEEP_ASSERT(handle->au_hdl >= 0, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(in != NULL, ERR_OBJ_IS_NULL); 
	ESWEEP_ASSERT(channels > 0, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(*offset >= 0, ERR_BAD_ARGUMENT); 
	for (i=0; i<channels; i++) {
		ESWEEP_OBJ_NOTEMPTY(in[i], ERR_EMPTY_OBJECT); 
		ESWEEP_ASSERT(in[i]->type == WAVE || in[i]->type ==COMPLEX, ERR_NOT_ON_THIS_TYPE);
	}

	ESWEEP_ASSERT((*offset=handle->audio_in((void*) handle, in, channels, *offset)) >= 0, ERR_UNKNOWN);
	return ERR_OK;
}

int esweep_audioClose(esweep_audio *handle) {
	ESWEEP_ASSERT(handle != NULL, ERR_BAD_ARGUMENT);
        ESWEEP_ASSERT(handle->au_hdl >= 0, ERR_BAD_ARGUMENT);
	handle->audio_close((void*) handle);
	return ERR_OK;
}
