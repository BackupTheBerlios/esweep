/*
 * Copyright (c) 2007 Jochen Fabricius <jfab@berlios.de>
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esweep.h"
#include "sound.h"

long esweep_audioOpen(const char *device, long samplerate) {
	long handle;
	if ((handle=snd_open(device, samplerate))<0) return ERR_UNKNOWN;
	else return handle;
}

long esweep_audioSetData(esweep_data *out, esweep_data *left, esweep_data *right) {
	long size;
	long samplerate;
	long i, j;
	Audio *wave_out;
	Wave *wave_left, *wave_right;
	
	if ((out==NULL) || ((left==NULL) && (right==NULL))) return ERR_EMPTY_CONTAINER;
	
	if ((left!=NULL) && (((*left).size<=0) || ((*left).data==NULL))) return ERR_EMPTY_CONTAINER;
	if ((right!=NULL) && (((*right).size<=0) || ((*right).data==NULL))) return ERR_EMPTY_CONTAINER;
	
	/* check type */
	
	if ((left!=NULL) && ((*left).type!=WAVE)) return ERR_NOT_ON_THIS_TYPE;
	if ((right!=NULL) && ((*right).type!=WAVE)) return ERR_NOT_ON_THIS_TYPE;
	
	if ((left!=NULL) && (right!=NULL)) 
		if ((*left).samplerate!=(*right).samplerate) return ERR_DIFF_MAPPING;
	
	if ((left!=NULL) && (right!=NULL)) {
		size=(*left).size+(*right).size?(*left).size:(*right).size;
		samplerate=(*left).samplerate;
	} else {
		if (left!=NULL) {
			size=(*left).size;
			samplerate=(*left).samplerate;
		}
		else {
			size=(*right).size;
			samplerate=(*right).samplerate;
		}
	}
	
	if ((*out).data!=NULL) free((*out).data);
	(*out).data=(Audio*) calloc(2*size, sizeof(Audio)); /* factor 2 for interleaved stereo */
	wave_out=(Audio*) (*out).data;
	(*out).type=AUDIO;
	(*out).size=size;
	(*out).samplerate=samplerate;
	
	/* merge the input data */
	
	if (left!=NULL) {
		wave_left=(Wave*) (*left).data;
		for (i=0, j=0;j<(*left).size;i+=2, j++) {
			wave_out[i]=(Audio) (32768*wave_left[j]);
		}
	}
	
	if (right!=NULL) {
		wave_right=(Wave*) (*right).data;
		for (i=1, j=0;j<(*right).size;i+=2, j++) {
			wave_out[i]=(Audio) (32768*wave_right[j]);
		}
	}
	return ERR_OK;
}

long esweep_audioPlay(esweep_data *out, long handle) {
	Audio *audio_out;
	if ((out==NULL) || ((*out).data==NULL) || ((*out).size<=0)) return ERR_EMPTY_CONTAINER;
	if ((*out).type!=AUDIO) return ERR_NOT_ON_THIS_TYPE;
	audio_out=(Audio*) (*out).data;
	if (snd_play(audio_out, 2*(*out).size, handle)==1) return ERR_UNKNOWN;
	return ERR_OK;
}

long esweep_audioRecord(esweep_data *in, long handle) {
	Audio *audio_in;
	if ((in==NULL) || ((*in).size<=0) || ((*in).data==NULL)) return ERR_EMPTY_CONTAINER;
	if ((*in).type!=AUDIO) return ERR_NOT_ON_THIS_TYPE;
	audio_in=(Audio*) (*in).data;
	if (snd_rec(audio_in, 2*(*in).size, handle)==1) return ERR_UNKNOWN;
	else return ERR_OK;
}

long esweep_audioMeasure(esweep_data *in, esweep_data *out, long handle) {
	Audio *audio_in, *audio_out;
	if ((out==NULL) || ((*out).data==NULL) || ((*out).size<=0)) return ERR_EMPTY_CONTAINER;
	if ((in==NULL) || ((*in).data==NULL) || ((*out).size<=0)) return ERR_EMPTY_CONTAINER;
	
	if ((*in).size!=(*out).size) return ERR_DIFF_MAPPING;
	if (((*in).type!=AUDIO) || ((*out).type!=AUDIO)) return ERR_NOT_ON_THIS_TYPE;
	audio_in=(Audio*) (*in).data;
	audio_out=(Audio*) (*out).data;
	if (snd_pnr(audio_in, audio_out, 2*(*in).size, handle)==1) return ERR_UNKNOWN;
	else return ERR_OK;
}

long esweep_audioGetData(esweep_data *left, esweep_data *right, esweep_data *in) {
	Wave *left_wave, *right_wave;
	Audio *audio;
	long i, j, size=0;
	
	if ((in==NULL) || ((*in).data==NULL) || ((*in).size<=0) || ((left==NULL) && (right==NULL))) return ERR_EMPTY_CONTAINER;
	
	audio=(Audio*) (*in).data;
	
	if (left!=NULL) {
		if ((*left).type!=WAVE) return ERR_NOT_ON_THIS_TYPE;
		(*left).samplerate=(*in).samplerate;
		if ((*left).data==NULL) {
			left_wave=(Wave*) calloc((*in).size, sizeof(Wave));
			(*left).data=left_wave;
			size=(*left).size=(*in).size;
		} else {
			left_wave=(*left).data;
			size=(*left).size>(*in).size?(*in).size:(*left).size;
		}
		for (i=0, j=0; i<size;i++, j+=2) left_wave[i]=(Wave) audio[j]/32768;
		for (;i<(*left).size;i++) left_wave[i]=0.0;
	}
	
	if (right!=NULL) {
		if ((*right).type!=WAVE) return ERR_NOT_ON_THIS_TYPE;
		(*right).samplerate=(*right).samplerate;
		if ((*right).data==NULL) {
			right_wave=(Wave*) calloc((*in).size, sizeof(Wave));
			(*right).data=right_wave;
			size=(*right).size=(*in).size;
		} else {
			right_wave=(*right).data;
			size=(*right).size>(*in).size?(*in).size:(*right).size;
		}
		for (i=0, j=1; i<size;i++, j+=2) right_wave[i]=(Wave) audio[j]/32768;
		for (;i<(*right).size;i++) right_wave[i]=0.0;
	}
	
	return ERR_OK;
}

long esweep_audioClose(long handle) {
	snd_close(handle);
	return ERR_OK;
}
