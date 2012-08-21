/*
 * Copyright (c) 2007,2009,2010 Jochen Fabricius <jfab@berlios.de>
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

/* Functions for controlling audio I/O. Not directly called from esweep. */

#ifndef __AUDIO_DEVICE_H
#define __AUDIO_DEVICE_H

#include "esweep_priv.h"

/* 
 * Open the soundcard. Except for the samplerate, all parameters are set to defaults. 
 * Use audio_device_configure() to set them to your needs. 
 * INPUT PARAMETERS: 
 * const char *device: name of the device; platform dependant, sometime only default audio device possible
 * u_int samplerate: desired samplerate. If the samplerate is not supported, 
 * opening the soundcard will fail
 * RETURN VALUES: a pointer to an esweep_audio structure, or NULL if open failed
 * EXAMPLE: esweep_audio *hdl=audio_device_open("audio:/dev/audio"); 
 */

esweep_audio* audio_device_open(const char *device); 

/* 
 * Query the current audio parameters. 
 * INPUT PARAMETERS: 
 * esweep_audio *handle: device, the return value from audio_device_open(). 
 * const char *query: name of the parameter, possible values (case sensitive): 
 * 	"samplerate": sample rate of the soundcard
 * 	"encoding": encoding of the soundcard, like AUDIO_ENCODING_SLINEAR on OpenBSD. 
 * 	"channels": opened channels on the soundcard, NOT the possible number of channels
 * 	"precision": bits per sample; currently only 16 bits/sample are supported
 * 	"blocksize": the blocksize of the soundcard
 * u_int *result: the result of the query
 * RETURN VALUES: 0 if query succeed, -1 if fail
 * EXAMPLE: 	u_int samplerate; 
 * 		audio_device_query(hdl, "samplerate", &samplerate); 
 */

int audio_device_query(const esweep_audio *handle, const char *query, int *result); 

/* 
 * Set selected audio parameters. 
 * INPUT PARAMETERS: 
 * esweep_audio *handle: device, the return value from audio_device_open(). 
 * const char *param: name of the parameter, possible values (case sensitive): 
 * 	"samplerate": sample rate of the soundcard
 * 	"channels": opened channels on the soundcard, NOT the possible number of channels
 * 	"blocksize": the blocksize of the soundcard
 * u_int value: the new value of the parameter
 * RETURN VALUES: 0 if configure succeed, -1 if fail
 * EXAMPLE: 	u_int samplerate; 
 * 		audio_device_query(hdl, "samplerate", &samplerate); 
 * NOTE: 
 * - audio_device_out()/audio_device_in() are block based, i. e. they play/record so much blocks of size "blocksize", 
 *   until all samples are played/record. This implies, that you may play/record some silence. 
 *   If you want to play/record exactly the samples of the esweep container, you can either create this
 *   container with the default blocksize (use audio_device_query() to get it), or you can set "blocksize" to the 
 *   size of the container. This is useful when performing a realtime FFT. E. g., if your FFT length is 8192, 
 *   set the "blocksize" to 8192, too. 
 * - on some platforms (e. g. OpenBSD) the "blocksize" parameter gets sticky, and will not change 
 *   when other parameters are changed. For example, when you reduce the samplerate, 
 *   the sticky blocksize may lead to a higher latency than you expect. The latency of the soundcard input
 *   and output is simply calculated by blocksize/samplerate. 
 */

int audio_device_configure(esweep_audio *handle, const char *param, int value); 
/* 
 * Synchronize play and record. After audio_device_open(), the play and record buffers are out of sync. 
 * This function plays silence and discards the record buffer. Should be called after every open. 
 * INPUT PARAMETERS: 
 * esweep_audio *handle: device, the return value from audio_device_open(). 
 * RETURN VALUES: 0 if sync succeed, -1 if fail
 * EXAMPLE: audio_device_sync(hdl); 
 */

int audio_device_sync(esweep_audio *handle); 


/* 
 * Pauses play and record, after every remaining samples are played. Play or record starts automatically
 * whenn calling audio_device_sync(), audio_device_out() or audio_device_in(). 
 * INPUT PARAMETERS: 
 * esweep_audio *handle: device, the return value from audio_device_open(). 
 * RETURN VALUES: 0 if pause succeed, -1 if fail
 * EXAMPLE: audio_device_pause(hdl); 
 */

int audio_device_pause(esweep_audio *handle); 

int audio_device_seek(esweep_audio *handle, int offset); 
/*
 * Audio output. Plays only one audio block, which is system dependant. For a correct usage see EXAMPLE. 
 * These function performs absolutely no checks on the input parameters. This should be done by the calling instance. 
 * INPUT PARAMETERS: 
 * esweep_audio *handle: device, the return value from audio_device_open(). 
 * esweep_object *out[]: array of esweep_object* containers. Every container must be of type WAVE
 * u_int channels: number of channels to play. Must be smaller or equal to the size of out[]
 * int offset: the sample where to start playback. See EXAMPLE. 
 * RETURN VALUES: samples played, -1 on error
 * EXAMPLE: to play a big container, you have to call audio_device_out() as int as samples are left in the containers: 
 * 	u_int samples=0; 
 *	// ... generate audio containers and open soundcard (don't forget to sync)
 *	while ((samples=audio_device_out(out, 2, samples, hdl)<size_of_biggest_container); 
 */

int audio_device_out(esweep_audio *handle, esweep_object *out[], u_int channels, int offset); 

/*
 * Audio input. Records only one audio block, which is system dependant. For a correct usage see EXAMPLE. 
 * These function performs absolutely no checks on the input parameters. This should be done by the calling instance. 
 * INPUT PARAMETERS: 
 * esweep_audio *handle: device, the return value from audio_device_open(). 
 * esweep_object *in[]: array of esweep_object* containers. Every container must be of type WAVE
 * u_int channels: number of channels to record. Must be smaller or equal to the size of out[]
 * int offset: the sample where to start record. See EXAMPLE. 
 * RETURN VALUES: samples recorded, -1 on error
 * EXAMPLE: to record a big container, you have to call audio_device_in() as int as all samples are recorded: 
 * 	u_int samples=0; 
 *	// ... generate audio containers and open soundcard (don't forget to sync)
 *	while ((samples=audio_device_in(in, 2, samples, hdl)<size_of_biggest_container); 
 *
 * To record while playback, you can simply do this: 
 *	while ((samples=audio_device_out(out, 2, samples, hdl)<size_of_biggest_container) {
 *		audio_device_in(in, 1, samples, hdl); 
 *	}
 *		
 */

int audio_device_in(esweep_audio *handle, esweep_object *in[], u_int channels, int offset); 

/* 
 * Close the soundcard. If there are any more samples left in the audio buffer, they are played. 
 * INPUT PARAMETERS: 
 * esweep_audio *handle: device, the return value from audio_device_open(). 
 * RETURN VALUES: no return value
 * EXAMPLE: audio_device_close(hdl); 
 */

void audio_device_close(esweep_audio *handle); 
#endif /* __AUDIO_DEVICE_H */
