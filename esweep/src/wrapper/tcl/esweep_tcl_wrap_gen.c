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
 * esweep_tcl_wrap_gen.c
 * Wraps the esweep_gen.c source file
 * 03.10.2011, jfab:	initial creation
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

static int __esweepGenerateSine(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], esweep_object *obj); 
static int __esweepGenerateLogsweep(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], esweep_object *obj); 
static int __esweepGenerateNoise(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], esweep_object *obj); 
static int __esweepGenerateDirac(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], esweep_object *obj); 

int esweepGenerate(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *types[] = {"sine", "logsweep", "linsweep", "noise", "dirac", "ramp", "saw", "tri", "rect", NULL};
	enum typeIdx {sineIdx, logsIdx, linsIdx, noiseIdx, diracIdx, rampIdx, sawIdx, triIdx, rectIdx};
	int index; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int ret; 

	CHECK_NUM_ARGS(objc >= 4 && objc%2 == 0, "type -obj objVarName ?option value ...?"); 

	/* get the object */
	for (obji=2; obji < objc; obji+=2) {
		/* get the options, ignore anyone except -obj */
		Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index); 
		if (index == objIdx) {
			CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj);
			optMask[index]=0; 
			break;
		}
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	DUPLICATE_WHEN_SHARED(tclObj, obj);

	/* the first parameter names the function to generate */
	if (Tcl_GetIndexFromObj(interp, objv[1], types, "option", 0, &index) != TCL_OK) {
		return TCL_ERROR; 
	}

	switch (index) {
		case sineIdx: 
			ret=__esweepGenerateSine(interp, objc-2, &(objv[2]), obj); 
			break; 
		case logsIdx:
			ret=__esweepGenerateLogsweep(interp, objc-2, &(objv[2]), obj); 
			break; 
		case diracIdx: 
			ret=__esweepGenerateDirac(interp, objc-2, &(objv[2]), obj); 
			break; 
		case noiseIdx:
			ret=__esweepGenerateNoise(interp, objc-2, &(objv[2]), obj); 
			break; 
		case linsIdx:
		case rampIdx:
		case sawIdx:
		case triIdx:
		case rectIdx:
		default: 
			Tcl_SetResult(interp, "Generator not yet implemented", TCL_STATIC);
			return TCL_ERROR;
	}
	if (ret == TCL_OK) {
		Tcl_InvalidateStringRep(tclObj);  
		/*
		tclObj = Tcl_ObjSetVar2( interp, objv[objIdx], NULL, tclObj,
                              TCL_LEAVE_ERR_MSG );
		if ( tclObj == NULL ) {
			return TCL_ERROR;
		}
		*/
	}

	return ret; 
}

static int __esweepGenerateSine(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], esweep_object *obj) {
	const char *opts[] = {"-frequency", "-phase", "-obj", NULL};
	int optMask[] = {1, 0, 0}; // necessary options
	enum optIdx {freqIdx, phaseIdx, objIdx};
	int obji;
	int index; 
	double freq=0.0, phase=0.0; 
	int periods=0; 

	CHECK_NUM_ARGS(objc == 4 || objc == 6, "-frequency value ?-phase value?"); 

	for (obji=0; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case freqIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &freq)!=TCL_OK) {
					Tcl_SetResult(interp, "option -frequency invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case phaseIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &phase)!=TCL_OK) {
					Tcl_SetResult(interp, "option -phase invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT(esweep_genSine(obj, freq, phase, &periods) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewLongObj(periods)); 
	return TCL_OK; 
}

static int __esweepGenerateLogsweep(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], esweep_object *obj) {
	const char *opts[] = {"-locut", "-hicut", "-spectrum", "-obj", NULL};
	int optMask[] = {1, 1, 0, 0}; // necessary options
	enum optIdx {locutIdx, hicutIdx, specIdx, objIdx};
	int obji;
	int index; 
	double locut=-1.0, hicut=-1.0; 
	double sweep_rate=0.0; 
	char *spectrum="pink"; 

	CHECK_NUM_ARGS(objc == 6 || objc == 8, "-locut value -hicut value ?-spectrum value?"); 

	for (obji=0; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case locutIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &locut)!=TCL_OK) {
					Tcl_SetResult(interp, "option -locut invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case hicutIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &hicut)!=TCL_OK) {
					Tcl_SetResult(interp, "option -hicut invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case specIdx: 
				if ((spectrum=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -spectrum invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT(esweep_genLogsweep(obj, locut, hicut, spectrum, &sweep_rate) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(sweep_rate)); 
	return TCL_OK; 
}

static int __esweepGenerateDirac(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], esweep_object *obj) {
	const char *opts[] = {"-delay", "-obj", NULL};
	int optMask[] = {0, 0}; // necessary options
	enum optIdx {delayIdx, objIdx};
	int obji;
	int index; 
	double delay=0.0; 

	CHECK_NUM_ARGS(objc == 2 || objc == 4, "?-delay value?"); 

	for (obji=0; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case delayIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &delay)!=TCL_OK) {
					Tcl_SetResult(interp, "option -delay invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT(esweep_genDirac(obj, delay) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(delay)); 
	return TCL_OK; 
}


static int __esweepGenerateNoise(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], esweep_object *obj) {
	const char *opts[] = {"-locut", "-hicut", "-spectrum", "-obj", NULL};
	int optMask[] = {1, 1, 0, 0}; // necessary options
	enum optIdx {locutIdx, hicutIdx, specIdx, objIdx};
	int obji;
	int index; 
	double locut=-1.0, hicut=-1.0; 
	char *spectrum="pink"; 

	CHECK_NUM_ARGS(objc == 6 || objc == 8, "-locut value -hicut value ?-spectrum value?"); 

	for (obji=0; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case locutIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &locut)!=TCL_OK) {
					Tcl_SetResult(interp, "option -locut invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case hicutIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &hicut)!=TCL_OK) {
					Tcl_SetResult(interp, "option -hicut invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case specIdx: 
				if ((spectrum=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -spectrum invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT(esweep_genNoise(obj, locut, hicut, spectrum) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewIntObj(obj->size)); 
	return TCL_OK; 
}


