#ifndef INTERFACE_H
#define INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WINDOW_D 2

	typedef unsigned long int  uint32;
	typedef unsigned short int uint16;
	typedef unsigned char      uint8;
	typedef signed long int    int32;
	typedef signed short int   int16;
	typedef signed char        int8;

	typedef struct threshold
	{
		uint8 uthr[4]; //shrinkage threshold for different scale
		uint8 vthr[4];
		uint8 *udw[3];// distance weight for different scale
		uint8 *vdw[3];
		uint8 *urw[3];// range weight for different scale
		uint8 *vrw[3];
	} thr;

	int cnr(uint8 * img,int32 srcw,int32 srch, thr *curthr);
	int cnr_init();
	int cnr_destroy();

#ifdef __cplusplus
}
#endif

#endif
