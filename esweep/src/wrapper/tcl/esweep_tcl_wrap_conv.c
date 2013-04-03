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
 * esweep_tcl_wrap_conv.c
 * Wraps the esweep_conv.c source file
 * 08.10.2011, jfab:	initial creation
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

int esweepToWave(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL;
	Tcl_Obj *tclObj=NULL;
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	CHECK_NUM_ARGS(objc == 3, "-obj objVarName");

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR;
		}
		switch (index) {
			case objIdx:
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj);
				break;
		}
		optMask[index]=0;
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index);

	DUPLICATE_WHEN_SHARED(tclObj, obj);
	ESWEEP_TCL_ASSERT(esweep_toWave(obj) == ERR_OK);
	Tcl_InvalidateStringRep(tclObj);
	Tcl_SetObjResult(interp, tclObj);
	return TCL_OK;
}

int esweepToComplex(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL;
	Tcl_Obj *tclObj=NULL;
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	CHECK_NUM_ARGS(objc == 3, "-obj objVarName");

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR;
		}
		switch (index) {
			case objIdx:
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj);
				break;
		}
		optMask[index]=0;
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index);

	DUPLICATE_WHEN_SHARED(tclObj, obj);
	ESWEEP_TCL_ASSERT(esweep_toComplex(obj) == ERR_OK);
	Tcl_InvalidateStringRep(tclObj);
	Tcl_SetObjResult(interp, tclObj);
	return TCL_OK;
}

int esweepToPolar(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL;
	Tcl_Obj *tclObj=NULL;
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	CHECK_NUM_ARGS(objc == 3, "-obj objVarName");

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR;
		}
		switch (index) {
			case objIdx:
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj);
				break;
		}
		optMask[index]=0;
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index);

	DUPLICATE_WHEN_SHARED(tclObj, obj);
	ESWEEP_TCL_ASSERT(esweep_toPolar(obj) == ERR_OK);
	Tcl_InvalidateStringRep(tclObj);
	Tcl_SetObjResult(interp, tclObj);
	return TCL_OK;
}

int esweepCompress(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL;
	Tcl_Obj *tclObj=NULL;
	const char *opts[] = {"-obj", "-factor", NULL};
	int optMask[] = {1, 1}; // necessary options
	enum optIdx {objIdx, factorIdx};
	int obji;
	int index;
  int factor;
	CHECK_NUM_ARGS(objc == 5, "-obj objVarName -factor number");

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR;
		}
		switch (index) {
			case objIdx:
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj);
				break;
      case factorIdx:
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &factor)!=TCL_OK) {
					Tcl_SetResult(interp, "option -factor invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;

		}
		optMask[index]=0;
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index);

	DUPLICATE_WHEN_SHARED(tclObj, obj);
	ESWEEP_TCL_ASSERT(esweep_compress(obj, factor) == ERR_OK);
	Tcl_InvalidateStringRep(tclObj);
	Tcl_SetObjResult(interp, tclObj);
	return TCL_OK;
}

