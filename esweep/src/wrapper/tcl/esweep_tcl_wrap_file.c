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
 * esweep_tcl_wrap_file.c
 * Wraps the esweep_file.c source file
 * 03.10.2011, jfab:	initial creation
 * 18.10.2011, jfab: 	added load and save
 */

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <tcl.h>
#include "esweep_tcl_wrap.h"

int esweepToAscii(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *in=NULL;
	const char *opts[] = {"-obj", "-filename", "-comment", NULL};
	int optMask[] = {1, 1, 0}; // necessary options
	enum optIdx {objIdx, fileIdx, commIdx};
	int obji;
	int index;
	char *filename=NULL;
	char *comment=NULL;

	CHECK_NUM_ARGS(objc == 5 || objc == 7, "-obj obj -filename value ?-comment string?");
	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR;
		}
		switch (index) {
			case objIdx:
				CHECK_ESWEEP_OBJECT(obji+1, in);
				break;
			case fileIdx:
				if ((filename=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -filename invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case commIdx:
				if ((comment=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -comment invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;

		}
		optMask[index]=0;
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index);

	ESWEEP_TCL_ASSERT(esweep_toAscii(filename, in, comment) == ERR_OK);
	Tcl_SetObjResult(interp, Tcl_NewStringObj(filename, -1));
	return TCL_OK;
}

int esweepSave(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *in=NULL;
	const char *opts[] = {"-obj", "-filename", "-meta", NULL};
	int optMask[] = {1, 1, 0}; // necessary options
	enum optIdx {objIdx, fileIdx, metaIdx};
	int obji;
	int index;
	char *filename=NULL;
	char *meta=NULL;

	CHECK_NUM_ARGS(objc == 5 || objc == 7, "-obj obj -filename value ?-meta value?");
	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR;
		}
		switch (index) {
			case objIdx:
				CHECK_ESWEEP_OBJECT(obji+1, in);
				break;
			case fileIdx:
				if ((filename=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -filename invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case metaIdx:
				if ((meta=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -meta invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
		}
		optMask[index]=0;
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index);

	ESWEEP_TCL_ASSERT(esweep_save(filename, in, meta) == ERR_OK);
	Tcl_SetObjResult(interp, Tcl_NewStringObj(filename, -1));
	return TCL_OK;
}

int esweepLoad(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *out=NULL;
	Tcl_Obj *ret=NULL;
	Tcl_Obj *metaObj=NULL;
	Tcl_Obj *metaObjName=NULL;
	const char *opts[] = {"-filename", "-meta", NULL};
	int optMask[] = {1, 0}; // necessary options
	enum optIdx {fileIdx, metaIdx};
	int obji;
	int index;
	char *filename=NULL;
	char *meta=NULL;

	CHECK_NUM_ARGS(objc == 3 || objc == 5, "-filename value ?-meta varName?");
	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR;
		}
		switch (index) {
			case fileIdx:
				if ((filename=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -filename invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case metaIdx:
				metaObjName=objv[obji+1];
				metaObj=Tcl_ObjGetVar2(interp, objv[obji+1], NULL, TCL_LEAVE_ERR_MSG);
				break;
		}
		optMask[index]=0;
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index);

	/* create a new object, size, type and samplerate are irrelevant */
	ESWEEP_TCL_ASSERT((out=esweep_create("wave", 1000, 0)) != NULL);
	ESWEEP_DEBUG_PRINT("Creating object: %p\n", out);


	ESWEEP_TCL_ASSERT(esweep_load(out, filename, &meta) == ERR_OK);

	if (metaObj != NULL) {
		metaObj=Tcl_DuplicateObj(metaObj);
    Tcl_SetStringObj(metaObj, meta, -1);
    Tcl_ObjSetVar2(interp, metaObjName, NULL, metaObj, TCL_LEAVE_ERR_MSG);
	}
	free(meta);

	ret=Tcl_NewObj();
	ret->internalRep.otherValuePtr=out;
	ret->typePtr = (Tcl_ObjType*) &tclEsweepObjType;
	Tcl_InvalidateStringRep(ret);
	Tcl_SetObjResult(interp, ret);
	return TCL_OK;
}

