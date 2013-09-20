/*
 * Copyright (c) 2007, 2011 Jochen Fabricius <jfab@berlios.de>
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

#include "esweep_priv.h"
#include "esweep.h"

#define FILE_ID "esweep"

static int close_on_error(FILE *fp) {
	fclose(fp);
	return ERR_UNKNOWN;
}

typedef union {
	double r;
	u_int64_t ui64;
} conv_t;

/* save: even if compiled with REAL32, we always write double data */
int esweep_save(const char *filename, const esweep_object *input, const char *meta) {
	FILE *fp;
	const char *id=FILE_ID;
	int i;
	Real *real=NULL;
	Complex *cpx=NULL;
	Polar *polar=NULL;
	conv_t conv;
	u_int32_t meta_size;

	/* network byte-order variable */
	u_int32_t nl;

	ESWEEP_ASSERT(filename != NULL, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(strlen(filename) > 0, ERR_BAD_ARGUMENT);
	ESWEEP_OBJ_NOTEMPTY(input, ERR_EMPTY_OBJECT);
	ESWEEP_ASSERT(input->type != SURFACE, ERR_NOT_ON_THIS_TYPE);


	/* open file */

	ESWEEP_ASSERT((fp = fopen(filename, "wb")) != NULL, ERR_UNKNOWN);

	/* write file id */
	ESWEEP_ASSERT(fwrite(id, strlen(id)*sizeof(char), 1, fp) == 1, close_on_error(fp));

	/* write meta data size */
	if (meta == NULL) {
		meta_size=0;
	} else {
		meta_size=strlen(meta);
	}
	nl=htonl(meta_size);
	ESWEEP_ASSERT(fwrite(&nl, sizeof(nl), 1, fp) == 1, close_on_error(fp));

	/* write meta data */
	if (meta_size > 0) {
		ESWEEP_ASSERT(fwrite(meta, sizeof(char), meta_size, fp) == meta_size, close_on_error(fp));
	}

	/* write data type */
	nl=htonl(input->type);
	ESWEEP_ASSERT(fwrite(&nl, sizeof(nl), 1, fp) == 1, close_on_error(fp));

	/* write samplerate */
	nl=htonl(input->samplerate);
	ESWEEP_ASSERT(fwrite(&nl, sizeof(nl), 1, fp) == 1, close_on_error(fp));

	/* write number of samples */
	nl=htonl(input->size);
	ESWEEP_ASSERT(fwrite(&nl, sizeof(nl), 1, fp) == 1, close_on_error(fp));

	/* write data */
	switch (input->type) {
		case WAVE:
			real=(Real*) input->data;
			for (i=0; i < input->size; i++) {
				conv.r=(double) real[i];
				conv.ui64=htole64(conv.ui64);
				ESWEEP_ASSERT(fwrite(&(conv.ui64), sizeof(u_int64_t), 1, fp) == 1, close_on_error(fp));
			}
			break;
		case COMPLEX:
			cpx=(Complex*) input->data;
			for (i=0; i < input->size; i++) {
				conv.r=(double) cpx[i].real;
				conv.ui64=htole64(conv.ui64);
				ESWEEP_ASSERT(fwrite(&(conv.ui64), sizeof(u_int64_t), 1, fp) == 1, close_on_error(fp));
				conv.r=(double) cpx[i].imag;
				conv.ui64=htole64(conv.ui64);
				ESWEEP_ASSERT(fwrite(&(conv.ui64), sizeof(u_int64_t), 1, fp) == 1, close_on_error(fp));
			}
			break;
		case POLAR:
			polar=(Polar*) input->data;
			for (i=0; i < input->size; i++) {
				conv.r=(double) polar[i].abs;
				conv.ui64=htole64(conv.ui64);
				ESWEEP_ASSERT(fwrite(&(conv.ui64), sizeof(u_int64_t), 1, fp) == 1, close_on_error(fp));
				conv.r=(double) polar[i].arg;
				conv.ui64=htole64(conv.ui64);
				ESWEEP_ASSERT(fwrite(&(conv.ui64), sizeof(u_int64_t), 1, fp) == 1, close_on_error(fp));
			}
			break;
		case SURFACE:
		default:
			ESWEEP_NOT_THIS_TYPE(input->type, ERR_NOT_ON_THIS_TYPE);
	}

	fclose(fp);
	return ERR_OK;
}

int esweep_load(esweep_object *output, const char *filename, char **meta) {
	FILE *fp;
	int id_length=strlen(FILE_ID);
	char *id=(char*) calloc(id_length+1, sizeof(char));
	esweep_object obj; // temporary object
	Real *real=NULL;
	Complex *cpx=NULL;
	Polar *polar=NULL;
	char *file_data=NULL;
	int i;
	u_int64_t *d;
	conv_t conv;
	u_int32_t meta_size;

	/* network byte-order variable */
	u_int32_t nl;

	ESWEEP_ASSERT(filename != NULL, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(strlen(filename) > 0, ERR_BAD_ARGUMENT);
	ESWEEP_OBJ_ISVALID(output, ERR_EMPTY_OBJECT);

	/* open input file */

	ESWEEP_ASSERT((fp = fopen(filename, "rb")) != NULL, ERR_UNKNOWN);

	/* read file id */
	ESWEEP_ASSERT(fread(id, sizeof(char), id_length, fp) == id_length, close_on_error(fp));
	ESWEEP_ASSERT(strcmp(id, FILE_ID) == 0, close_on_error(fp));
	free(id);

	/* read meta data size */
	ESWEEP_ASSERT(fread(&nl, sizeof(nl), 1, fp) == 1, close_on_error(fp));
	meta_size=ntohl(nl);

	if (meta_size > 0) {
		ESWEEP_MALLOC(*meta, meta_size+1, sizeof(char), close_on_error(fp));
		/* read meta data */
		ESWEEP_ASSERT(fread(*meta, sizeof(char), meta_size, fp) == meta_size, close_on_error(fp));
	} else {
		*meta=NULL;
	}

	/* read type */
	ESWEEP_ASSERT(fread(&nl, sizeof(nl), 1, fp) == 1, close_on_error(fp));
	obj.type=ntohl(nl);

	/* read samplerate */
	ESWEEP_ASSERT(fread(&nl, sizeof(nl), 1, fp) == 1, close_on_error(fp));
	obj.samplerate=ntohl(nl);

	/* read number of samples */
	ESWEEP_ASSERT(fread(&nl, sizeof(nl), 1, fp) == 1, close_on_error(fp));
	obj.size=ntohl(nl);

	/* copy data */
	if (output->data != NULL) free(output->data);
	switch (obj.type) {
		case WAVE:
			/* create temporary data */
			ESWEEP_MALLOC(file_data, obj.size, sizeof(u_int64_t), ERR_MALLOC);

			/* read data */
			ESWEEP_ASSERT(fread(file_data, sizeof(u_int64_t), obj.size, fp) == obj.size, \
					close_on_error(fp));

			ESWEEP_MALLOC(output->data, obj.size, sizeof(Real), ERR_MALLOC);
			real=(Real*) output->data;
			d=(u_int64_t*) file_data;
			for (i=0; i < obj.size; i++) {
				conv.ui64=d[i];
				conv.ui64=letoh64(conv.ui64);
				real[i]=(Real) conv.r;
			}
			free(file_data);
			break;
		case COMPLEX:
			/* create temporary data */
			ESWEEP_MALLOC(file_data, obj.size, 2*sizeof(u_int64_t), ERR_MALLOC);

			/* read data */
			ESWEEP_ASSERT(fread(file_data, 2*sizeof(u_int64_t), obj.size, fp) == obj.size, \
					close_on_error(fp));

			ESWEEP_MALLOC(output->data, obj.size, sizeof(Complex), ERR_MALLOC);
			cpx=(Complex*) output->data;
			d=(u_int64_t*) file_data;
			for (i=0; i < obj.size; i++) {
				conv.ui64=d[2*i];
				conv.ui64=letoh64(conv.ui64);
				cpx[i].real=(Real) conv.r;
				conv.ui64=d[2*i+1];
				conv.ui64=letoh64(conv.ui64);
				cpx[i].imag=(Real) conv.r;
			}
			free(file_data);

			break;
		case POLAR:
			/* create temporary data */
			ESWEEP_MALLOC(file_data, obj.size, 2*sizeof(u_int64_t), ERR_MALLOC);

			/* read data */
			ESWEEP_ASSERT(fread(file_data, 2*sizeof(u_int64_t), obj.size, fp) == obj.size, \
					close_on_error(fp));

			ESWEEP_MALLOC(output->data, obj.size, sizeof(Polar), ERR_MALLOC);
			polar=(Polar*) output->data;
			d=(u_int64_t*) file_data;
			for (i=0; i < obj.size; i++) {
				conv.ui64=d[2*i];
				conv.ui64=letoh64(conv.ui64);
				polar[i].abs=(Real) conv.r;
				conv.ui64=d[2*i+1];
				conv.ui64=letoh64(conv.ui64);
				polar[i].arg=(Real) conv.r;
			}
			free(file_data);

			break;
		default:
			free(file_data);
			if (meta != NULL) free(meta);
			ESWEEP_NOT_THIS_TYPE(input->type, ERR_NOT_ON_THIS_TYPE);
	}

	output->size=obj.size;
	output->type=obj.type;
	output->samplerate=obj.samplerate;

	/* we're done */
	fclose(fp);

	return ERR_OK;
}

int esweep_toAscii(const char *filename, const esweep_object *obj, const char *comment) {
	Wave *wave;
	Complex *cpx;
	Polar *polar;
	Surface *surface;
	FILE *fp;
	int i, j;
	Real df;
	int zoffset;

	ESWEEP_ASSERT(filename != NULL && strlen(filename) > 0, ERR_BAD_ARGUMENT);
	ESWEEP_OBJ_NOTEMPTY(obj, ERR_BAD_ARGUMENT);

	ESWEEP_ASSERT((fp=fopen(filename, "w"))!=NULL, ERR_UNKNOWN);
	if (comment != NULL && strlen(comment) > 0) {
		fprintf(fp, "%s\n", comment);
	}
	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=0;i<obj->size;i++) {
				fprintf(fp, "%e\t%e\n", (Real) 1000*i/obj->samplerate, wave[i]); /* time in milliseconds */
			}
			break;
		case POLAR:
			df=(Real) obj->samplerate/obj->size;
			polar=(Polar*) obj->data;
#ifdef ESWEEP_FILE_FULL_POLAR_ASCII_OUTPUT
			for (i=0;i<obj->size;i++) {
#else
			for (i=0;i<obj->size/2+1;i++) {
#endif
				fprintf(fp, "%e\t%e\t%e\n", i*df, polar[i].abs, polar[i].arg);
			}
			break;
		case COMPLEX:
			cpx=(Complex*) obj->data;
			for (i=0;i<obj->size;i++) {
				fprintf(fp, "%e\t%e\n", cpx[i].real, cpx[i].imag);
			}
			break;
		case SURFACE:
			surface=(Surface*) obj->data;
			for (i=0;i<surface->xsize;i++) {
				zoffset=i*surface->ysize;
				for (j=0;j<surface->ysize;j++) {
					fprintf(fp, "%e\t%e\t%e\n", surface->x[i], surface->y[j], surface->z[j+zoffset]);
				}
				fprintf(fp, "\n");
			}
			break;
		default:
			return ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}
	ESWEEP_ASSERT(fclose(fp)==0, ERR_UNKNOWN);
	return ERR_OK;
}
