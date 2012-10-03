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
 * esweep_tcl_wrap_disp.c
 * This is actually no wrapper file, but performs the time-consuming
 * labour to calculate display coordinates from esweep data
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

#define ABS_COORDS 0
#define ARG_COORDS 1
#define REAL_COORDS 0
#define IMAG_COORDS 1
#define COMPLEX_COORDS 2

static void inline waveLinLin(const esweep_object *obj, Tcl_Obj *listPtr, const double screen[], const double world[]); 
static void inline waveLogLin(const esweep_object *obj, Tcl_Obj *listPtr, const double screen[], const double world[]); 
static void inline polarLogLin(const esweep_object *obj, Tcl_Obj *listPtr, const double screen[], const double world[], int option); 
static void inline polarLinLin(const esweep_object *obj, Tcl_Obj *listPtr, const double screen[], const double world[], int option); 

int esweepGetCoords(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	const char *opts[] = {"-obj", "-screen", "-world", "-log", "-opt", NULL};
	int optMask[] = {1, 1, 1, 0, 0}; // necessary options
	enum optIdx {objIdx, screenIdx, worldIdx, logIdx, optIdx};
	int obji;
	int index;
	int i; 

	int xlog=0, ylog=0; // define which scale is logarithmic
	const char *logscale=""; 

	const char *option=""; 
	int optDef=ABS_COORDS; 

	Tcl_Obj *listPtr=NULL; 

	Tcl_Obj *rectObj; // the outer rectangle
	double screen[4]; // screen coordinates of the outer rectangle, 0=x0, 1=y0, 2=x1, 3=y1
	double world[4]; // screen coordinates of the outer rectangle 
	int rectListLength=0;

	esweep_object *obj=NULL; 
	int objSize=0; 

	/* 
	 * Option -log: define which axis is logarithmic. Allowed are "", "x", "y" or "xy", default is ""
	 * Option -opt: for Polar and Complex objects you can select if "abs", "arg", "real" or "imag" should be converted.
	 * 	Default is "abs"/"real", respectively. If -opt is "complex", than the x-axis will be "real"/"abs" and the y-axis will be "imag"/"arg"
	 */
	CHECK_NUM_ARGS(objc >= 7 && objc <= 11 && objc % 2 == 1, "-obj esweepObj -screen coordList -world coordList ?-log axes? ?-opt value?"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break;
			case screenIdx:
				if (Tcl_ListObjLength(NULL, objv[obji+1], &rectListLength)!=TCL_OK) {
					Tcl_SetResult(interp, "-screen coordList is not a list", TCL_STATIC);
					return TCL_ERROR; 
				}
				if (rectListLength != 4) {
					Tcl_SetResult(interp, "screen rectangle not defined correctly", TCL_STATIC);
					return TCL_ERROR; 
				}
				for (i=0; i < rectListLength; i++) {
					Tcl_ListObjIndex(interp, objv[obji+1], i, &rectObj); 
					if (Tcl_GetDoubleFromObj(NULL, rectObj, &(screen[i]))!=TCL_OK) {
						Tcl_SetResult(interp, "-screen coordList contains non-integer values", TCL_STATIC);
						return TCL_ERROR;
					}
				}
				break; 
			case worldIdx:
				if (Tcl_ListObjLength(NULL, objv[obji+1], &rectListLength)!=TCL_OK) {
					Tcl_SetResult(interp, "-world coordList is not a list", TCL_STATIC);
					return TCL_ERROR; 
				}
				if (rectListLength != 4) {
					Tcl_SetResult(interp, "world rectangle not defined correctly", TCL_STATIC);
					return TCL_ERROR; 
				}
				for (i=0; i < rectListLength; i++) {
					Tcl_ListObjIndex(interp, objv[obji+1], i, &rectObj); 
					if (Tcl_GetDoubleFromObj(NULL, rectObj, &(world[i]))!=TCL_OK) {
						Tcl_SetResult(interp, "-world coordList contains non-numeric values", TCL_STATIC);
						return TCL_ERROR;
					}
				}
				break; 
			case logIdx: 
				if ((logscale=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -log invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				if (strcmp(logscale, "")==0) {
					xlog=0;
					ylog=0;
				} else if (strcmp(logscale, "x")==0) xlog=1; 
					else if (strcmp(logscale, "y")==0) ylog=1;
					else if (strcmp(logscale, "xy")==0) {
						xlog=1; 
						ylog=1; 
					}
					else Tcl_SetResult(interp, "unknown logscale", TCL_STATIC);

				break;
			case optIdx: 
				if ((option=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -opt invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				if (strcmp(option, "abs")==0) optDef=ABS_COORDS; 
				else if (strcmp(option, "arg")==0) optDef=ARG_COORDS; 
				else if (strcmp(option, "real")==0) optDef=REAL_COORDS; 
				else if (strcmp(option, "imag")==0) optDef=IMAG_COORDS; 
				else if (strcmp(option, "complex")==0) optDef=COMPLEX_COORDS; 
				else Tcl_SetResult(interp, "unknown option -opt", TCL_STATIC);
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT(esweep_size(obj, &objSize)==ERR_OK); 
	ESWEEP_TCL_ASSERT(objSize > 0); 

	listPtr=Tcl_NewListObj(0, NULL); 
	switch (obj->type) {
		case WAVE:
			if (xlog == 0 && ylog == 0) waveLinLin(obj, listPtr, screen, world);
			else if (xlog == 1 && ylog == 0) waveLogLin(obj, listPtr, screen, world);
			break; 
		case POLAR: 
			if (xlog == 0 && ylog == 0) polarLinLin(obj, listPtr, screen, world, optDef);
			else if (xlog == 1 && ylog == 0) polarLogLin(obj, listPtr, screen, world, optDef);
			break; 
		default: 
			Tcl_SetResult(interp, "Unknown type of esweep object", TCL_STATIC);
			return TCL_ERROR; 
	}

	Tcl_IncrRefCount(listPtr); 
	Tcl_SetObjResult(interp, listPtr); 
	Tcl_DecrRefCount(listPtr); 
	return TCL_OK; 
}

static void inline waveLinLin(const esweep_object *obj, Tcl_Obj *listPtr, const double screen[], const double world[]) {
	int i; 
	double xscale, yscale; 
	double dt; 
	int x, y; 
	Wave *wave=(Wave*) obj->data; 

	xscale=(screen[2]-screen[0])/(world[2]-world[0]); 
	yscale=(screen[3]-screen[1])/(world[3]-world[1]); 
	dt=1000.0/obj->samplerate; 
	for (i=0;i < obj->size; i++) {
		x=(int) (0.5+xscale*(i*dt-world[0])+screen[0]);
		y=(int) (0.5+yscale*(world[3]-wave[i])+screen[1]);
		Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(x));
		Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(y));
	}
}

static void inline waveLogLin(const esweep_object *obj, Tcl_Obj *listPtr, const double screen[], const double world[]) {
	int i; 
	double xscale, yscale; 
	double dt; 
	int x, y; 
	Wave *wave=(Wave*) obj->data; 

	xscale=(screen[2]-screen[0])/(log10(world[2]/world[0])); 
	yscale=(screen[3]-screen[1])/(world[3]-world[1]); 
	dt=1000.0/obj->samplerate; 
	x=(int) dt; 
	y=0; // avoid compiler warning
	for (i=1;i < obj->size; i++) {
		x=(int) (0.5+xscale*log10(i*dt/world[0])+screen[0]);
		y=(int) (0.5+yscale*(world[3]-wave[i])+screen[1]);
		Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(x));
		Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(y));
	}
}

static void inline polarLogLin(const esweep_object *obj, Tcl_Obj *listPtr, const double screen[], const double world[], int option) {
	int i, i0, i1; 
	double xscale, yscale; 
	double df; 
	int x, y; 
	Polar *polar=(Polar*) obj->data; 

	xscale=(screen[2]-screen[0])/(log10(world[2]/world[0])); 
	yscale=(screen[3]-screen[1])/(world[3]-world[1]); 

	df=1.0*obj->samplerate/obj->size; 
	x=(int) df; 
	y=0; // avoid compiler warning
	// get the drawable indices
	i0=(int) (world[0]/df); i0=i0 < 1 ? 1 : i0; // do not round up
	i1=(int) (world[2]/df+0.5); i1=i1 >= obj->size ? obj->size-1 : i1; 
	switch (option) {
		case ABS_COORDS: 
			for (i=i0; i < i1; i++) {
				x=(int) (0.5+xscale*log10(i*df/world[0])+screen[0]);
				y=(int) (0.5+yscale*(world[3]-polar[i].abs)+screen[1]);
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(x));
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(y));
			}
			/* the size of the list must be a multiple of 4 */
			Tcl_ListObjLength(NULL, listPtr, &i); 
			if ((i % 4)) {
				// simply add the last 2 elements
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(x));
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(y));
			}
			break; 
		case ARG_COORDS: 
			for (i=i0;i < i1; i++) {
				x=(int) (0.5+xscale*log10(i*df/world[0])+screen[0]);
				y=(int) (0.5+yscale*(world[3]-polar[i].arg)+screen[1]);
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(x));
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(y));
			}
			/* the size of the list must be a multiple of 4 */
			Tcl_ListObjLength(NULL, listPtr, &i); 
			if ((i % 4)) {
				// simply add the last 2 elements
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(x));
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(y));
			}
			break; 
		case COMPLEX_COORDS: 
			for (i=0;i < obj->size; i++) {
				x=(int) (0.5+xscale*log10(polar[i].abs/world[0])+screen[0]);
				y=(int) (0.5+yscale*(world[3]-polar[i].arg)+screen[1]);
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(x));
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(y));
			}
			break; 
	}
}
static void inline polarLinLin(const esweep_object *obj, Tcl_Obj *listPtr, const double screen[], const double world[], int option) {
	int i; 
	int x, y;
        double xscale, yscale; 
	double df; 
	Polar *polar=(Polar*) obj->data; 

	xscale=(screen[2]-screen[0])/(world[2]-world[0]); 
	yscale=(screen[3]-screen[1])/(world[3]-world[1]); 

	df=1.0*obj->samplerate/obj->size; 
	x=(int) df; 
	y=0; // avoid compiler warning
	switch (option) {
		case ABS_COORDS: 
			for (i=0;i < obj->size; i++) {
				x=(int) (0.5+xscale*(i*df-world[0])+screen[0]);
				y=(int) (0.5+yscale*(world[3]-polar[i].abs)+screen[1]);
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(x));
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(y));
			}
			break; 
		case ARG_COORDS: 
			for (i=0;i < obj->size; i++) {
				x=(int) (0.5+xscale*(i*df-world[0])+screen[0]);
				y=(int) (0.5+yscale*(world[3]-polar[i].arg)+screen[1]);
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(x));
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(y));
			}
			break; 
		case COMPLEX_COORDS: 
			for (i=0;i < obj->size; i++) {
				x=(int) (0.5+xscale*(polar[i].abs-world[0])+screen[0]);
				y=(int) (0.5+yscale*(world[3]-polar[i].arg)+screen[1]);
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(x));
				Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewIntObj(y));
			}
			break; 
	}
}

