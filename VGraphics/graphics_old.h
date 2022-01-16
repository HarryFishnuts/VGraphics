/******************************************************************************
* <graphics.h>
* Bailey Jia-Tao Brown
* 2021
* 
*	Header file for primitive graphics library
*	Contents:
*		- Header guard
*		- API definition
*		- Definitions
*		- Typedefs
*		- Init function
*		- Module init and terminate functions
*		- Module update functions
*		- Misc rendering functions
*		- Clear, swap and fill functions
*		- Basics draw functions
*		- Float variants
*		- Advanced draw functions
*		- ITex functions
*		- Texture editing functions
*		- Input related functions
*		- Texture loading and saving functions
*		- Debug functions
* 
******************************************************************************/

#ifndef __VGRAPHICS_INCLUDE__
#define __VGRAPHICS_INCLUDE__

/* API DEFINITION */
#ifdef VGRAPHICS_EXPORTS
#define VAPI __declspec(dllexport)
#else
#define VAPI __declspec(dllimport)
#endif

/* DEFINITIONS */
#define VG_TRUE (int)1
#define VG_FALSE (int)0
#define VG_TEXTURES_MAX 0x400
#define VG_SHAPES_MAX 0x300
#define VG_WINDOW_SIZE_MIN 500
#define VG_ITEX_COLORS_MAX 0x10
#define VG_ITEX_SIZE_MAX 0x40
#define VG_FLUSH_THRESHOLD 0x100

/* TYPEDEFS */
typedef unsigned short vgTexture;
typedef unsigned short vgShape;

/* INIT AND TERMINATE FUNCTIONS */
VAPI void vgInit(int window_w, int window_h, int resolution_w,
	int resolution_h, int decorated, int resizeable, int linear);
VAPI void vgTerminate(void);

/* MODULE UPDATE FUNCTIONS */
VAPI void vgUpdate(void);
VAPI int vgWindowShouldClose(void);
VAPI unsigned long long vgUpdateCount(void);

/* MISC RENDERING FUNCTIONS */
VAPI void vgSetWindowSize(int window_w, int window_h);
VAPI void vgGetResolution(int* w, int* h);
VAPI void vgSetWindowTitle(const char* title);
VAPI void vgGetScreenSize(int* width, int* height);

/* CLEAR AND SWAP FUNCTIONS */
VAPI void vgClear(void);
VAPI void vgFill(int r, int g, int b);
VAPI void vgSwap(void);

/* BASIC DRAW FUNCTIONS */
VAPI void vgColor3(int r, int g, int b);
VAPI void vgColor4(int r, int g, int b, int a);
VAPI void vgRect(int x, int y, int w, int h);
VAPI void vgLineSize(float size);
VAPI void vgLine(int x1, int y1, int x2, int y2);
VAPI void vgPointSize(float size);
VAPI void vgPoint(int x, int y);
VAPI void vgViewport(int x, int y, int w, int h);
VAPI void vgViewportReset(void);

/* FLOAT VARIANTS */
VAPI void vgRectf(float x, float y, float w, float h);
VAPI void vgLinef(float x1, float y1, float x2, float y2);
VAPI void vgPointf(float x, float y);

/* ADVANCED DRAW FUNCTIONS */
VAPI vgTexture vgCreateTexture(int w, int h, int linear, int repeat,
	void* data);
VAPI void vgDestroyTexture(vgTexture tex);
VAPI void vgUseTexture(vgTexture target);
VAPI void vgTextureFilter(int r, int g, int b, int a);
VAPI void vgTextureFilterReset(void);
VAPI void vgRectTexture(int x, int y, int w, int h);
VAPI void vgRectTextureOffset(int x, int y, int w, int h, float s, float t);
VAPI vgShape vgCompileShape(float* f2d_data, int size);
VAPI vgShape vgCompileShapeTextured(float* f2d_data, float* t2d_data,
	int size);
VAPI void vgDrawShape(vgShape shape, float x, float y, float r, float s);
VAPI void vgDrawShapeTextured(vgShape shape, float x, float y, float r,
	float s);
VAPI void vgRenderScale(float scale);
VAPI void vgUseRenderScaling(int value);
VAPI void vgRenderOffset(float x, float y);
VAPI void vgUseRenderOffset(int value);
VAPI void vgRenderLayer(unsigned char layer);

/* ITEX FUNCTIONS */
VAPI void vgITexDataClear(void);
VAPI void vgITexDataColor(unsigned short index, int r, int g, int b, int a);
VAPI void vgITexDataIndex(unsigned short index, int x, int y);
VAPI void vgITexDataIndexArray(unsigned short index, int* vx, int* vy,
	int size);
VAPI vgTexture vgITexDataCompile(int width, int height, int repeat,
	int linear);

/* TEXTURE EDITING FUNCTIONS */
VAPI void vgEditTexture(vgTexture target, int w, int h);
VAPI void vgEditColor(int r, int g, int b, int a);
VAPI void vgEditPoint(int x, int y);
VAPI void vgEditLine(int x1, int y1, int x2, int y2);
VAPI void vgEditRect(int x, int y, int w, int h);
VAPI void vgEditShape(vgShape shape, float x, float y, float r, float s);
VAPI void vgEditUseTexture(vgTexture tex);
VAPI void vgEditShapeTextured(vgShape shape, float x, float y, float r,
	float s);
VAPI void vgEditSetData(int width, int height, void* data);
VAPI void vgEditClear(void);
VAPI void* vgGetTextureData(vgTexture tex, int w, int h);


/* CURSOR RELATED FUNCTIONS */
VAPI void vgGetCursorPos(int* x, int* y);
VAPI void vgGetCursorPosScaled(int* x, int* y);
VAPI void vgGetCursorPosScaledT(int* rx, int* ry, int x, int y, int w, int h,
	int sub_w, int sub_h);
VAPI int vgOnLeftClick(void);
VAPI int vgOnRightClick(void);
VAPI int vgCursorOverlap(int x, int y, int w, int h);

/* TEXTURE LOADING AND SAVING FUNCTIONS */
VAPI void vgSaveTexture(vgTexture texture, const char* file, int w, int h);
VAPI vgTexture vgLoadTexture(const char* file, int w, int h, int linear,
	int repeat);
VAPI void* vgLoadTextureData(const char* file, int w, int h);

/* DEBUG FUNCTIONS */
VAPI unsigned int _vgDebugGetTextureName(vgTexture texture);
VAPI unsigned int _vgDebugGetShapeName(vgShape shape);
VAPI unsigned int _vgDebugGetFramebuffer(void);
VAPI void* _vgDebugGetWindowHandle(void);

#endif