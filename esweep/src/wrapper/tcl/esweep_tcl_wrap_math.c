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
 * esweep_tcl_wrap_math.c
 * Wraps the esweep_math.c source file
 * 04.10.2011, jfab:	initial creation
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

typedef int (EsweepMathFunc) (esweep_object *a, const esweep_object *b);

int esweepExpr(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *a=NULL, *b=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *operator=NULL; 
	EsweepMathFunc *mathFunc=NULL; 
	double vald; 
	Real valr; 
	int scalar=0;

	CHECK_NUM_ARGS(objc == 4, "objVarName operator \"obj or number\""); 

	/* the first parameter is the name of an esweep object*/
	CHECK_ESWEEP_OBJECT2(1, tclObj, a); 
	/* the third parameter can be an esweep object*/
	if (objv[3]->typePtr == &tclEsweepObjType) {
		b=(esweep_object*) objv[3]->internalRep.otherValuePtr;
	} else {
		/* but it may also be a pure number, so lets try to extract it */
		if (Tcl_GetDoubleFromObj(NULL, objv[3], &vald)==TCL_ERROR) {
			Tcl_SetResult(interp, "Parameter 3 neither an esweep object nor a number", TCL_STATIC);
			return TCL_ERROR;
		}
		/* convert it into an esweep object */
		b=esweep_create("wave", a->samplerate, 1);
		valr=(Real) vald; 
		esweep_buildWave(b, &valr, 1);
		scalar=1; // remember to delete b manually
	}
	
	/* the second parameter is the operator, allowed are +, -, *, / */
	if ((operator=Tcl_GetString(objv[2]))==NULL) {
		/* almost impossible */
		Tcl_SetResult(interp, "invalid operator", TCL_STATIC);
		return TCL_ERROR;
	}

	if (strcmp(operator, "+")==0) mathFunc=&esweep_add; 
	else if (strcmp(operator, "-")==0) mathFunc=&esweep_sub; 
	else if (strcmp(operator, "*")==0) mathFunc=&esweep_mul; 
	else if (strcmp(operator, "/")==0) mathFunc=&esweep_div; 
	else Tcl_SetResult(interp, "unknown operator", TCL_STATIC);

	DUPLICATE_WHEN_SHARED(tclObj, a);
	ESWEEP_TCL_ASSERT(mathFunc(a, b) == ERR_OK); 
	if (scalar) esweep_free(b); 
	Tcl_SetObjResult(interp, tclObj); 
	Tcl_InvalidateStringRep(tclObj);  
	return TCL_OK; 
}

int esweepMax(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	const char *opts[] = {"-obj", "-from", "-to", NULL};
	int optMask[] = {1, 0, 0}; // necessary options
	enum optIdx {objIdx, fromIdx, toIdx};
	int obji;
	int index;
	Real max; 
	int from=-1, to=-1; 

	CHECK_NUM_ARGS(objc >= 3 && objc <= 7 && (objc-1)%2 == 0, "-obj esweepObj ?-from index? ?-to index?"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break;
			case fromIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &from)!=TCL_OK) {
					Tcl_SetResult(interp, "option -from invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case toIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &to)!=TCL_OK) {
					Tcl_SetResult(interp, "option -to invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 

		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_max(obj, from, to, &max) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj((double) max)); 
	return TCL_OK; 
}

int esweepMin(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	const char *opts[] = {"-obj", "-from", "-to", NULL};
	int optMask[] = {1, 0, 0}; // necessary options
	enum optIdx {objIdx, fromIdx, toIdx};
	int obji;
	int index;
	Real min; 
	int from=-1, to=-1; 

	CHECK_NUM_ARGS(objc >= 3 && objc <= 7 && (objc-1)%2 == 0, "-obj esweepObj ?-from index? ?-to index?"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break;
			case fromIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &from)!=TCL_OK) {
					Tcl_SetResult(interp, "option -from invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case toIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &to)!=TCL_OK) {
					Tcl_SetResult(interp, "option -to invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_min(obj, from, to, &min) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj((double) min)); 
	return TCL_OK; 
}

int esweepMaxPos(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	const char *opts[] = {"-obj", "-from", "-to", NULL};
	int optMask[] = {1, 0, 0}; // necessary options
	enum optIdx {objIdx, fromIdx, toIdx};
	int obji;
	int index;
	int pos; 
	int from=-1, to=-1; 

	CHECK_NUM_ARGS(objc >= 3 && objc <= 7 && (objc-1)%2 == 0, "-obj esweepObj ?-from index? ?-to index?"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break;
			case fromIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &from)!=TCL_OK) {
					Tcl_SetResult(interp, "option -from invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case toIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &to)!=TCL_OK) {
					Tcl_SetResult(interp, "option -to invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_maxPos(obj, from, to, &pos) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewLongObj(pos)); 
	return TCL_OK; 
}

int esweepMinPos(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	const char *opts[] = {"-obj", "-from", "-to", NULL};
	int optMask[] = {1, 0, 0}; // necessary options
	enum optIdx {objIdx, fromIdx, toIdx};
	int obji;
	int index;
	int pos; 
	int from=-1, to=-1; 

	CHECK_NUM_ARGS(objc >= 3 && objc <= 7 && (objc-1)%2 == 0, "-obj esweepObj ?-from index? ?-to index?"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break;
			case fromIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &from)!=TCL_OK) {
					Tcl_SetResult(interp, "option -from invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case toIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &to)!=TCL_OK) {
					Tcl_SetResult(interp, "option -to invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_minPos(obj, from, to, &pos) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewLongObj(pos)); 
	return TCL_OK; 
}

int esweepAvg(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	Real avg; 

	CHECK_NUM_ARGS(objc == 3, "-obj esweepObj"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_avg(obj, &avg) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj((double) avg)); 
	return TCL_OK; 
}

int esweepSum(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	Real sum; 

	CHECK_NUM_ARGS(objc == 3, "-obj esweepObj"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_sum(obj, &sum) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj((double) sum)); 
	return TCL_OK; 
}

int esweepSqsum(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	Real sum; 

	CHECK_NUM_ARGS(objc == 3, "-obj esweepObj"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_sqsum(obj, &sum) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj((double) sum)); 
	return TCL_OK; 
}

int esweepReal(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	CHECK_NUM_ARGS(objc == 3, "-obj varName"); 

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
	ESWEEP_TCL_ASSERT(esweep_real(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	return TCL_OK; 
}

int esweepImag(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	CHECK_NUM_ARGS(objc == 3, "-obj varName"); 

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
	ESWEEP_TCL_ASSERT(esweep_imag(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	return TCL_OK; 
}

int esweepAbs(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	CHECK_NUM_ARGS(objc == 3, "-obj varName"); 

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
	ESWEEP_TCL_ASSERT(esweep_abs(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	return TCL_OK; 
}

int esweepArg(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	CHECK_NUM_ARGS(objc == 3, "-obj varName"); 

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
	ESWEEP_TCL_ASSERT(esweep_arg(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	return TCL_OK; 
}

int esweepLg(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	CHECK_NUM_ARGS(objc == 3, "-obj varName"); 

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
	ESWEEP_TCL_ASSERT(esweep_lg(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	return TCL_OK; 
}

int esweepLn(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	CHECK_NUM_ARGS(objc == 3, "-obj varName"); 

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
	ESWEEP_TCL_ASSERT(esweep_ln(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	return TCL_OK; 
}

int esweepExp(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	CHECK_NUM_ARGS(objc == 3, "-obj varName"); 

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
	ESWEEP_TCL_ASSERT(esweep_exp(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	return TCL_OK; 
}

int esweepPow(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", "-exp", NULL};
	int optMask[] = {1, 1}; // necessary options
	enum optIdx {objIdx, expIdx};
	int obji;
	int index;
	double exp; 
	CHECK_NUM_ARGS(objc == 5, "-obj varName -exp value"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj); 
				break;
			case expIdx:
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &exp)==TCL_ERROR) {
					Tcl_SetResult(interp, "option -exp invalid", TCL_STATIC); 
					return TCL_ERROR;
				}

		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	DUPLICATE_WHEN_SHARED(tclObj, obj);
	ESWEEP_TCL_ASSERT(esweep_pow(obj, exp) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	return TCL_OK; 
}

int esweepSchroeder(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index;
	CHECK_NUM_ARGS(objc == 3, "-obj varName"); 

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
	ESWEEP_TCL_ASSERT(esweep_schroeder(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	return TCL_OK; 
}

