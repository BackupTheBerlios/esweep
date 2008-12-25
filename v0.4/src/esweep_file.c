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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esweep.h"

#define FILE_ID "esweep"

/* helper functions */

static long convert_wave(const char *output, esweep_data *input_file);
static long convert_audio(const char *output, esweep_data *input_file);
static long convert_polar(const char *output, esweep_data *input);
static long convert_complex(const char *output, esweep_data *input);
static long convert_surface(const char *output, esweep_data *input);

void error_open(const char *filename) {
	printf("Can't open %s\n", filename);
}

void error_read(const char *filename) {
	printf("Error reading file %s\n", filename);
}

void error_write(const char *filename) {
	printf("Error writing file %s\n", filename);
}

long esweep_save(const char *output, esweep_data *input) {
	FILE *fp;
	const char *id=FILE_ID;
	
	if (strlen(output)<1) return ERR_BAD_ARGUMENT;
	if ((input==NULL) || ((*input).size<=0) || ((*input).data==NULL)) return ERR_EMPTY_CONTAINER;
	
	/* open output file */
	
	if ((fp = fopen(output, "wb"))==NULL) {
		error_open(output);
		return ERR_UNKNOWN;
	}
	
	/* write file id */
	if (fwrite(id, strlen(id)*sizeof(char), 1, fp)!=1) {
		error_write(output);
		fclose(fp);
		return ERR_UNKNOWN;
	}
	
	/* write type */
	if (fwrite(&(*input).type, sizeof((*input).type), 1, fp)!=1) {
		error_write(output);
		fclose(fp);
		return ERR_UNKNOWN;
	}
	
	/* write samplerate */
	if (fwrite(&(*input).samplerate, sizeof((*input).samplerate), 1, fp)!=1) {
		error_write(output);
		fclose(fp);
		return ERR_UNKNOWN;
	}
	
	/* write samples */
	if (fwrite(&(*input).size, sizeof((*input).size), 1, fp)!=1) {
		error_write(output);
		fclose(fp);
		return ERR_UNKNOWN;
	}
	/* write size */
	switch ((*input).type) {
		case AUDIO:
			/* write data */
			if (fwrite((*input).data, sizeof(Audio), 2*(*input).size, fp)!=2*(*input).size) {
				error_write(output);
				fclose(fp);
				return ERR_UNKNOWN;
			}
			break;
		case WAVE:
			/* write data */
			if (fwrite((*input).data, sizeof(Wave), (*input).size, fp)!=(*input).size) {
				error_write(output);
				fclose(fp);
				return ERR_UNKNOWN;
			}
			break;
		case COMPLEX:
			/* write data */
			if (fwrite((*input).data, sizeof(Complex), (*input).size, fp)!=(*input).size) {
				error_write(output);
				fclose(fp);
				return ERR_UNKNOWN;
			}
			break;
		case POLAR:
			/* write data */
			if (fwrite((*input).data, sizeof(Polar), (*input).size, fp)!=(*input).size) {
				error_write(output);
				fclose(fp);
				return ERR_UNKNOWN;
			}
			break;
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	fclose(fp);
	return ERR_OK;
}

long esweep_load(esweep_data *output, const char *input) {
	FILE *fp;
	long id_length=strlen(FILE_ID);
	char *id=(char*) calloc(id_length+1, sizeof(char));
	
	if (strlen(input)<1) return ERR_BAD_ARGUMENT;
	if (output==NULL) return ERR_EMPTY_CONTAINER;
	
	/* open input file */
	
	if ((fp = fopen(input, "rb"))==NULL) {
		error_open(input);
		return ERR_UNKNOWN;
	}
	
	/* read file id */
	if (fread(id, sizeof(char), id_length, fp)!=id_length) {
		error_read(input);
		fclose(fp);
		return ERR_UNKNOWN;
	}
	if (strcmp(id, FILE_ID)!=0) {
		fclose(fp);
		return ERR_UNKNOWN;
	}
	free(id);
	/* free data */
	if ((*output).data!=NULL) free((*output).data);
	(*output).data=NULL;
	(*output).size=0;
	
	/* read type */
	if (fread(&(*output).type, sizeof((*output).type), 1, fp)!=1) {
		error_read(input);
		fclose(fp);
		return ERR_UNKNOWN;
	}
	
	/* read samplerate */
	if (fread(&(*output).samplerate, sizeof((*output).samplerate), 1, fp)!=1) {
		error_read(input);
		fclose(fp);
		return ERR_UNKNOWN;
	}
	
	/* read size */
	if (fread(&(*output).size, sizeof((*output).size), 1, fp)!=1) {
		error_read(input);
		fclose(fp);
		return ERR_UNKNOWN;
	}
	
	/* read data */
	switch ((*output).type) {
		case AUDIO:
			(*output).data=(Audio*) calloc(2*(*output).size, sizeof(Audio));
			if (fread((*output).data, sizeof(Audio), 2*(*output).size, fp)!=2*(*output).size) {
				error_read(input);
				fclose(fp);
				free((*output).data);
				(*output).data=NULL;
				(*output).size=0;
				return ERR_UNKNOWN;
			}
			break;
		case WAVE:
			(*output).data=(Wave*) calloc((*output).size, sizeof(Wave));
			if (fread((*output).data, sizeof(Wave), (*output).size, fp)!=(*output).size) {
				error_read(input);
				fclose(fp);
				free((*output).data);
				(*output).data=NULL;
				(*output).size=0;
				return ERR_UNKNOWN;
			}
			break;
		case COMPLEX:
			(*output).data=(Complex*) calloc((*output).size, sizeof(Complex));
			if (fread((*output).data, sizeof(Complex), (*output).size, fp)!=(*output).size) {
				error_read(input);
				fclose(fp);
				free((*output).data);
				(*output).data=NULL;
				(*output).size=0;
				return ERR_UNKNOWN;
			}
			break;
		case POLAR:
			(*output).data=(Polar*) calloc((*output).size, sizeof(Polar));
			if (fread((*output).data, sizeof(Polar), (*output).size, fp)!=(*output).size) {
				error_read(input);
				fclose(fp);
				free((*output).data);
				(*output).data=NULL;
				(*output).size=0;
				return ERR_UNKNOWN;
			}
			break;
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
	fclose(fp);
	return ERR_OK;
}

long esweep_toAscii(const char *output, esweep_data *input) {
	
	if (strlen(output)<1) return ERR_BAD_ARGUMENT;
	if ((input==NULL) || ((*input).size<=0) || ((*input).data==NULL)) return ERR_EMPTY_CONTAINER;
	
	switch ((*input).type) {
		case WAVE:
			return convert_wave(output, input);
		case POLAR:
			return convert_polar(output, input);
		case COMPLEX:
			return convert_complex(output, input);
		case SURFACE:
			return convert_surface(output, input);
		case AUDIO:
			return convert_audio(output, input);
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}
}

static long convert_wave(const char *output, esweep_data *input) {
	Wave *wave;
	FILE *fp;
	long i;
	
	if ((fp=fopen(output, "w"))==NULL) {
		return ERR_UNKNOWN;
	}
	
	wave=(Wave*) (*input).data;
	for (i=0;i<(*input).size;i++) {
		fprintf(fp, "%e\t%e\n", (double) 1000*i/(*input).samplerate, wave[i]); /* time in milliseconds */
	}
	
	fclose(fp);
	return ERR_OK;
}

static long convert_audio(const char *output, esweep_data *input) {
	Audio *audio;
	FILE *fp;
	long i;
	
	if ((fp=fopen(output, "w"))==NULL) {
		return ERR_UNKNOWN;
	}
	
	audio=(Audio*) (*input).data;
	for (i=0;i<2*(*input).size;i+=2) {
		fprintf(fp, "%e\t%i\n", (double) 0.5*i/(*input).samplerate, audio[i]+audio[i+1]);
	}
	
	fclose(fp);
	return ERR_OK;
}

static long convert_polar(const char *output, esweep_data *input) {
	Polar *polar;
	FILE *fp;
	double df=(double) (*input).samplerate/(*input).size;
	long i;
	
	if ((fp=fopen(output, "w"))==NULL) {
		return ERR_UNKNOWN;
	}
	
	polar=(Polar*) (*input).data;
	for (i=0;i<(*input).size/2+1;i++) {
		fprintf(fp, "%e\t%e\t%e\n", i*df, polar[i].abs, polar[i].arg);
	}
	
	fclose(fp);
	return ERR_OK;
}

static long convert_complex(const char *output, esweep_data *input) {
	Complex *complex;
	FILE *fp;
	long i;
	
	if ((fp=fopen(output, "w"))==NULL) {
		return ERR_UNKNOWN;
	}
	
	complex=(Complex*) (*input).data;
	for (i=0;i<(*input).size/2+1;i++) {
		fprintf(fp, "%e\t%e\n", complex[i].real, complex[i].imag);
	}
	
	fclose(fp);
	return ERR_OK;
}

static long convert_surface(const char *output, esweep_data *input) {
	Surface *surface=(Surface*) (*input).data;
	FILE *fp;
	long i, j;
	long zoffset;

	if ((fp=fopen(output, "w"))==NULL) {
		return ERR_UNKNOWN;
	}
	
	for (i=0;i<(*surface).xsize;i++) {
		zoffset=i*(*surface).ysize;
		for (j=0;j<(*surface).ysize;j++) {
			fprintf(fp, "%e\t%e\t%e\n", (*surface).x[i], (*surface).y[j], (*surface).z[j+zoffset]);
		}
		fprintf(fp, "\n");
	}
	
	fclose(fp);
	return ERR_OK;
}
