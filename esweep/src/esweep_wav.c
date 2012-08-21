/*
 * Copyright (c) 2009 Jochen Fabricius <jfab@berlios.de>
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
#include <time.h>

#include "esweep.h"

int fpwrite(int data, int size, FILE *fp) {
	return fwrite(&data, size, 1, fp);
}

/* write a wav file pcm
 * supported sample sizes are 16 and 32 bits
 */

/* RAND_MAX is usually defined as 0x7FFF. Use the preprocessor to make this damn sure */
#ifdef RAND_MAX
	#if (RAND_MAX != 0x7FFF)
		#undef RAND_MAX
		#define RAND_MAX 0x7FFF
	#endif
#else
	#define RAND_MAX 0x7FFF
#endif

static int readChannel(esweep_object *wave, void *chunk, int ch, int channels, int samplerate, int samples, int bitsPerSample, Real scale);

int esweep_writeWav(const char *output, esweep_object *left, esweep_object *right, int bitsPerSample) {
	FILE *fp;
	Real scale;
	Real dither, eta;
	void *data;
	short *short_data;
	int *int_data;
	Wave *wave_left;
	Wave *wave_right;
	unsigned int size;
	int interleaved=1;
	unsigned int samplerate;
	unsigned short block_align;
	unsigned int i, j;

	if ((left==NULL) && (right==NULL)) return ERR_EMPTY_OBJECT;

	if ((left!=NULL) && (((*left).size<=0) || ((*left).data==NULL))) return ERR_EMPTY_OBJECT;
	if ((right!=NULL) && (((*right).size<=0) || ((*right).data==NULL))) return ERR_EMPTY_OBJECT;

	/* check type */

	if ((left!=NULL) && ((*left).type!=WAVE)) return ERR_NOT_ON_THIS_TYPE;
	if ((right!=NULL) && ((*right).type!=WAVE)) return ERR_NOT_ON_THIS_TYPE;

	if ((left!=NULL) && (right!=NULL))
		if ((*left).samplerate!=(*right).samplerate) return ERR_DIFF_MAPPING;

	if ((bitsPerSample!=16) && (bitsPerSample!=32)) return ERR_UNKNOWN;

	/* open output file */
	if ((fp = fopen(output, "wb"))==NULL) {
		return ERR_UNKNOWN;
	}

	if ((left!=NULL) && (right!=NULL)) {
		size=(*left).size>(*right).size?(*left).size:(*right).size;
		samplerate=(*left).samplerate;
		interleaved=2;
	} else {
		if (left!=NULL) {
			size=(*left).size;
			samplerate=(*left).samplerate;
			interleaved=1;
		}
		else {
			size=(*right).size;
			samplerate=(*right).samplerate;
			interleaved=1;
		}
	}


	/* seed the random number generator for dithering; we use a simple
	 * High-Pass TPDF dither algorithm */
	srand(time(NULL));
	dither=0.0;
	if (bitsPerSample==16) {
		data=(void*) calloc(interleaved*size, sizeof(short));
		short_data=data;

		scale=0x8000; /* 2^15 */
		/* this gives a random number below 1/2 LSB */
		eta=(rand()-0x3FFF)/((Real)0x8000);

		/* merge the input data */
		if (left!=NULL) {
			wave_left=(Wave*) (*left).data;
			for (i=0, j=0;j<(*left).size;i+=interleaved, j++) {
				dither=-eta;
				eta=(rand()-0x3FFF)/((Real)0x8000);
				dither+=eta;
				dither=0.0;
				short_data[i]=(short) (dither+scale*wave_left[j]);
			}
		}

		if (right!=NULL) {
			wave_right=(Wave*) (*right).data;
			for (i=interleaved-1, j=0;j<(*right).size;i+=interleaved, j++) {
				dither=-eta;
				eta=(rand()-0x3FFF)/((Real)0x8000);
				dither+=eta;
				dither=0.0;
				short_data[i]=(short) (dither+scale*wave_right[j]);
			}
		}
	} else if (bitsPerSample==32) {
		data=(void*) calloc(interleaved*size, sizeof(int));
		int_data=data;

		scale=0x80000000; /* 2^31 */
		/* this gives a random number below 1/2 LSB */
		eta=(rand()-0x3FFF)/0x8000;
		/* merge the input data */
		if (left!=NULL) {
			wave_left=(Wave*) (*left).data;
			for (i=0, j=0;j<(*left).size;i+=interleaved, j++) {
				dither=-eta;
				eta=(rand()-0x3FFF)/((Real)0x8000);
				dither+=eta;
				int_data[i]=(int) (dither+scale*wave_left[j]);
			}
		}

		if (right!=NULL) {
			wave_right=(Wave*) (*right).data;
			for (i=interleaved-1, j=0;j<(*right).size;i+=interleaved, j++) {
				dither=-eta;
				eta=(rand()-0x3FFF)/((Real)0x8000);
				dither+=eta;
				int_data[i]=(int) (dither+scale*wave_right[j]);
			}
		}
	}

	/* write the wav file */

	/* File ID */
	if (fwrite("RIFF", 4, 1, fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* file size
	 * the residual headers have a size of 36 bytes
	 * we must add this to the length of the wave data in bytes
	 */
	if (fpwrite((unsigned int) (36+interleaved*size*bitsPerSample/8), sizeof(unsigned int), fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* file type */
	if (fwrite("WAVE", 4, 1, fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* fmt chunk */
	if (fwrite("fmt ", 4, 1, fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* length of fmt data */
	if (fpwrite((unsigned int) 16, sizeof(unsigned int), fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* format tag: 1 (PCM) */
	if (fpwrite((unsigned short) 1, sizeof(unsigned short), fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* channels */
	if (fpwrite((unsigned short) interleaved, sizeof(unsigned short), fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* samplerate */
	if (fpwrite((unsigned int) samplerate, sizeof(unsigned int), fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* bytes per second */
	block_align=(unsigned short) (interleaved*bitsPerSample/8);
	if (fpwrite((unsigned int) (samplerate*block_align), sizeof(unsigned int), fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* block align */
	if (fpwrite((unsigned short) block_align, sizeof(unsigned short), fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* bits per sample */
	if (fpwrite((unsigned short) bitsPerSample, sizeof(unsigned short), fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* data chunk */
	if (fwrite("data", 4, 1, fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* size of data chunk */
	if (fpwrite((unsigned int) (interleaved*size*bitsPerSample/8), sizeof(unsigned int), fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}

	/* write wave */
	if (fwrite(data, (unsigned int) (interleaved*size*bitsPerSample/8), 1, fp)!=1) {
		free(data);
		fclose(fp);
		return ERR_UNKNOWN;
	}


	free(data);
	fclose(fp);
	return ERR_OK;
}

/* reads up to 6 Channels from a wave file */
int esweep_loadWav(esweep_object *one,
		    esweep_object *two,
		    esweep_object *three,
		    esweep_object *four,
		    esweep_object *five,
		    esweep_object *six,
		    const char *input) {
	FILE *fp;
	Wave *w_one, *w_two, *w_three, *w_four, *w_five, *w_six;
	unsigned int i;
	char fileID[5], fileType[5], chunk_name[5];
	unsigned int file_size, chunk_size;
	void *file_data;
	void *chunk_data;
	void *file_pos;
	void *chunk_pos;
	unsigned short format;
	unsigned short channels;
	unsigned int samplerate;
	unsigned short bitsPerSample;
	Real scale;
	short short_tmp;
	int int_tmp;
	unsigned int size;

	/* open input file */
	if ((fp = fopen(input, "rb"))==NULL) {
		return 1;
	}

	/* read the first 4 bytes and check for "RIFF" */
	if (fread(fileID, sizeof(char), 4, fp)!=4) {
		fclose(fp);
		return 2;
	}
	fileID[4]='\0';
	if (strcmp(fileID, "RIFF")!=0) {
		fclose(fp);
		return 3;
	}

	/* file size */
	if (fread(&file_size, sizeof(unsigned int), 1, fp)!=1) {
		fclose(fp);
		return 4;
	}

	/* read the complete file at once, we can parse it later */
	file_data=(void*) calloc(file_size, 1);
	if (fread(file_data, file_size, 1, fp)!=1) {
		free(file_data);
		fclose(fp);
		return 5;
	}
	fclose(fp);

	/* file type "WAVE" */
	file_pos=file_data;
	memcpy(fileType, file_pos, 4);
	file_pos+=4;
	fileType[4]='\0';
	if (strcmp(fileType, "WAVE")!=0) {
		free(file_data);
		return 6;
	}

	/* fmt chunk */
	memcpy(chunk_name, file_pos, 4);
	file_pos+=4;
	chunk_name[4]='\0';
	if (strcmp(chunk_name, "fmt ")!=0) {
		free(file_data);
		return 7;
	}

	/* fmt chunk size */
	memcpy(&chunk_size, file_pos, 4);
	file_pos+=4;
	chunk_data=(void*) calloc(chunk_size, 1);
	memcpy(chunk_data, file_pos, chunk_size);
	chunk_pos=chunk_data;

	/* format tag, must be PCM (1) */
	memcpy(&format, chunk_pos, 2);
	chunk_pos+=2;
	if (format!=1) {
		/* ignore other formats and hope for the best */
		/*
		free(file_data);
		free(chunk_data);
		return ERR_UNKNOWN;
		*/
	}

	/* channels */
	memcpy(&channels, chunk_pos, 2);
	chunk_pos+=2;
	if ((channels<1) || (channels>6)) {
		free(file_data);
		free(chunk_data);
		return 8;
	}

	/* samplerate */
	memcpy(&samplerate, chunk_pos, 4);
	chunk_pos+=4;

	/* we don't need the next 6 bytes (bytes/second and block align) */
	chunk_pos+=6;

	/* bits per sample, supported are 16 and 32 bits */
	memcpy(&bitsPerSample, chunk_pos, 2);
	if ((bitsPerSample!=16) && (bitsPerSample!=32)) {
		free(file_data);
		free(chunk_data);
		return 9;
	}
	/* there may be additional data, but we ignore it */
	free(chunk_data);

	/* set the file pointer to the end of the chunk */
	file_pos+=chunk_size;
	/* find data chunk, other chunks, will be ignored */
	do {
		memcpy(chunk_name, file_pos, 4);
		file_pos+=4;
		chunk_name[4]='\0';
		memcpy(&chunk_size, file_pos, 4);
		file_pos+=4;
		if (strcmp(chunk_name, "data")==0) break;
		file_pos+=chunk_size;
	} while (!feof(fp));

	if (feof(fp)) {
		free(file_data);
		return 10;
	}

	/* data chunk */
	chunk_data=(void*) calloc(chunk_size, 1);
	memcpy(chunk_data, file_pos, chunk_size);

	/* the wave data is now in chunk_data, we can delete file_data */
	free(file_data);

	scale=pow(2, bitsPerSample)/2;
	size=chunk_size/(channels*bitsPerSample/8);

	switch (channels) {
		case 6: readChannel(six, chunk_data, 5, channels, samplerate, size, bitsPerSample, scale);
		case 5: readChannel(five, chunk_data, 4, channels, samplerate, size, bitsPerSample, scale);
		case 4: readChannel(four, chunk_data, 3, channels, samplerate, size, bitsPerSample, scale);
		case 3: readChannel(three, chunk_data, 2, channels, samplerate, size, bitsPerSample, scale);
		case 2: readChannel(two, chunk_data, 1, channels, samplerate, size, bitsPerSample, scale);
		case 1: readChannel(one, chunk_data, 0, channels, samplerate, size, bitsPerSample, scale);
		default: break;
	}

	free(chunk_data);
	return ERR_OK;
}

static int readChannel(esweep_object *wave, void *chunk, int ch, int channels, int samplerate, int samples, int bitsPerSample, Real scale) {
	Wave *w;
	void *chunk_pos;
	short short_tmp;
	int int_tmp;
	int i;
	if (wave!=NULL) {
		if (wave->type!=WAVE) return ERR_NOT_ON_THIS_TYPE;
		wave->samplerate=samplerate;
		if (wave->data!=NULL) {
			free(wave->data);
		}
		w=(Wave*) calloc(samples, sizeof(Wave));
		wave->data=w;
		wave->size=samples;
		chunk_pos=chunk;
		if (bitsPerSample==16) {
			chunk_pos+=ch*sizeof(short);
			for (i=0;i<samples;i++) {
				memcpy(&short_tmp, chunk_pos, sizeof(short));
				chunk_pos+=channels*sizeof(short);
				w[i]=(Wave) (short_tmp/scale);
			}
		} else if (bitsPerSample==32) {
			chunk_pos+=ch*sizeof(int);
			for (i=0;i<samples;i++) {
				memcpy(&int_tmp, chunk_pos, sizeof(int));
				chunk_pos+=channels*sizeof(int);
				w[i]=(Wave) (int_tmp/scale);
			}
		}
	}
	return ERR_OK;
}


