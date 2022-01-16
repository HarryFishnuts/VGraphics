/******************************************************************************
* <graphics.h>
* Bailey Jia-Tao Brown
* 2022
* 
*	Header file for general-purpose graphics library
*	Contents:
*		- Header Guard
*		- Preprocessor Defs
*		- API Definition
*		- Macros and Definitions
*		- Typedefs
* 
******************************************************************************/

/* HEADER GUARD */
#ifndef __VGRAPHICS_INCLUDE__
#define __VGRAPHICS_INCLUDE__

	/* PREPROCESSOR DEFINITIONS */
#pragma pack(0) /* pack structs tightly */
#define VC_EXTRALEAN 
#define WIN32_LEAN_AND_MEAN 

/* API DEFINITION */
#ifdef VGRAPHICS_EXPORTS
#define VGAPI __stdcall __declspec(dllexport)
#else
#define VGAPI __stdcall __declspec(dllimport)
#endif

#endif /* HEADER GUARD END */