/******************************************************************************
* <graphics.c>
* Bailey Jia-Tao Brown
* 2021
*
*	Source file for primitive graphics library
*	Contents:
*		- Preprocessor defs
*		- Includes
*		- Definitions
*		- Internal resources
*		- Internal helper functions
*		- Module init and terminate functions
*		- Module update functions
*		- Misc rendering functions
*		- Clear, swap and fill functions
*		- Basic draw functions
*		- Float variants
*		- Advanced draw functions
*		- ITex functions
*		- Texture editing functions
*		- Input related functions
*		- Texture loading and saving functions
*		- Debug functions
*
******************************************************************************/

/* PREPROCESSOR DEFS */
#define GLEW_STATIC
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

/* INCLUDES */
#include <stdio.h> /* I/O */
#include <stdlib.h> /* Memory allocation */

#include <Windows.h> /* OpenGL dependency */

#include <glew.h>  /* OpenGL extension library */
#include <gl/GL.h> /* Graphics library */
#include <math.h>  /* Math functions */

#include "graphics.h" /* Header */

/* DEFINITIONS */
#define RENDERSKIP(and) if (_renderSkip && and) return

/* ========INTERNAL RESOURCES======== */

/* window and rendering data */
static HWND  _window;
static HDC   _deviceContext;
static HGLRC _glContext;

static GLuint _framebuffer;
static GLuint _texture;
static GLuint _depth;

static int _swapTime;
static int _renderSkip;
static int _useRenderSkip;

static int _vpx, _vpy, _vpw, _vph;
static int _windowWidth;
static int _windowHeight;
static int _resW;
static int _resH;

static float _rScale;
static int _useRScale;
static float _layer;
static float _rOffsetX;
static float _rOffsetY;
static int _useROffset;

/* buffer data */
static GLuint _texBuffer[VG_TEXTURES_MAX] = { 0 };
static int _texCount = 0;
static GLuint _shapeBuffer[VG_SHAPES_MAX] = { 0 };

/* update data */
static unsigned long long _updates = 0;

/* color and size related data */
static int _colR, _colG, _colB, _colA = 0;
static int _tcolR, _tcolG, _tcolB, _tcolA = 255;
static float _lineW = 1;
static float _pointW = 1;
static vgTexture _useTex;

/* itex data */
static unsigned char _icolorR[VG_ITEX_COLORS_MAX] = { 0 };
static unsigned char _icolorG[VG_ITEX_COLORS_MAX] = { 0 };
static unsigned char _icolorB[VG_ITEX_COLORS_MAX] = { 0 };
static unsigned char _icolorA[VG_ITEX_COLORS_MAX] = { 0 };
static unsigned short _indexes[VG_ITEX_SIZE_MAX][VG_ITEX_SIZE_MAX] = { 0 };

/* texture editing data */
static vgTexture _eTex;
static GLuint    _eFrameBuffer = 0;
static GLuint    _rFrameBuffer = 0;
static int _ecolR, _ecolG, _ecolB, _ecolA = 0;
static int _eWidth, _eHeight = 0;
static vgTexture _euTex;

/* windowstate */
static int _winState = 0;

/* ================================== */

/* INTERNAL HELPER FUNCTIONS */

static inline void psetup(void)
{
	/* bind to framebuffer */
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

	/* set to projection matrix mode */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	/* generic projection */
	float ratio = (float)_windowHeight / (float)_windowWidth;
	glOrtho(-_rScale, _rScale, -(_rScale * ratio),
		(double)_rScale * ratio, 0, 0xFFFF);

	/* reset if not using scaling */
	if (!_useRScale)
	{
		glLoadIdentity();
		glOrtho(-1.0, 1.0, -ratio, ratio, 0, 0xFFFF);
	}
		

	/* setup viewport and color */
	glViewport(_vpx, _vpy, _vpw, _vph);
	glColor4ub(_colR, _colG, _colB, _colA);
	
	/* change to modelview */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0, 0, _layer);

	if (_useROffset)
		glTranslatef(-_rOffsetX, -_rOffsetY, 0);
}

static inline void rsetup(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, NULL);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _windowWidth, 0, _windowHeight, 0, 0xFFFF);

	glViewport(0, 0, _windowWidth, _windowHeight);

	glColor4ub(255, 255, 255, 255);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
}

static inline void esetup(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, _eFrameBuffer);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _eWidth, 0, _eHeight, 0, 0xFFFF);

	glViewport(0, 0, _eWidth, _eHeight);

	glColor4ub(_ecolR, _ecolG, _ecolB, _ecolA);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static inline vgTexture findFreeTexture(void)
{
	for (int i = 0; i < VG_TEXTURES_MAX; i++)
	{
		int indexActual = (_texCount / 2) + i;
		if (_texBuffer[i % VG_TEXTURES_MAX] == NULL)
			return i;
	}
}

static inline vgShape findFreeShape(void)
{
	for (int i = 0; i < VG_SHAPES_MAX; i++)
	{
		if (_shapeBuffer[i] == NULL)
			return i;
	}
}

/* WINDOW CALLBACK */
static LRESULT CALLBACK vgWProc(HWND hWnd, UINT message,
	WPARAM wParam, LPARAM lParam)
{
	PIXELFORMATDESCRIPTOR pfd = { 0 };

	switch (message)
	{
	case WM_CREATE:
		/* configure pixelformat */
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
			PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 16;
		pfd.cStencilBits = 16;

		/* get device context */
		_deviceContext = GetDC(hWnd);

		/* init pixelformat and bind */
		int formatHandle = ChoosePixelFormat(_deviceContext,
			&pfd);
		SetPixelFormat(_deviceContext, formatHandle, &pfd);

		/* create rendering context */
		_glContext = wglCreateContext(_deviceContext);
		wglMakeCurrent(_deviceContext, _glContext);

		/* set size properly */
		
		break;

	/* on destroy */
	case WM_DESTROY:

		/* set windowstate to false */
		_winState = FALSE;

		/* free all openGL objects */
		glDeleteFramebuffers(1, &_framebuffer);
		glDeleteFramebuffers(1, &_eFrameBuffer);
		glDeleteFramebuffers(1, &_rFrameBuffer);
		glDeleteRenderbuffers(1, &_depth);
		glDeleteTextures(1, &_texture);

		for (int i = 0; i < VG_TEXTURES_MAX; i++)
		{
			glDeleteTextures(1, &_texBuffer[i]);
			_texBuffer[i] = NULL;
		}

		for (int i = 0; i < VG_SHAPES_MAX; i++)
		{
			glDeleteLists(_shapeBuffer[i], 1);
			_shapeBuffer[i] = NULL;
		}

		/* release DC */
		ReleaseDC(_window, _deviceContext);

		/* destroy gl context */
		wglDeleteContext(_glContext);

		return DefWindowProc(hWnd, message, wParam, lParam);
		break;

	/* on default */
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

/* INIT AND TERMINATE FUNCTIONS */

VAPI void vgInit(int window_w, int window_h, int resolution_w,
	int resolution_h, int linear)
{
	/* setup update count and layer */
	_updates = 0;
	_layer = 1.0f;
	_swapTime = VG_SWAP_TIME_MIN;
	_renderSkip    = FALSE;
	_useRenderSkip = TRUE;
	_texCount = 0;

	/* enable DPI awareness */
	SetProcessDPIAware();

	/* create window */

	/* setup and register window class */
	WNDCLASSA wClass = { 0 };
	wClass.lpfnWndProc = vgWProc;
	wClass.lpszClassName = "vgWindow";
	int regResult = RegisterClassA(&wClass);

	/* check if reg failed */
	/* 1410 means class already exists, so it's ok */
	int errCode = GetLastError();
	if (!regResult && errCode != 1410)
	{
		char cBuff[0xFF];
		sprintf(cBuff, "Register Window Class!\nError Code: %d\n",
			errCode);
		MessageBoxA(NULL, cBuff, "CRITICAL ENGINE FAILURE", MB_OK);
		exit(1);
	}

	/* ensure window size is big enough */
	RECT clientRect = { 0, 0, window_w, window_h };
	AdjustWindowRectExForDpi(&clientRect,
		WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZEBOX |
		WS_THICKFRAME, TRUE,
		WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZEBOX |
		WS_THICKFRAME,
		GetDpiForSystem());
	int winWidth = clientRect.right - clientRect.left;
	int winHeight = clientRect.bottom - clientRect.top;

	/* create window */
	_window = CreateWindowA(wClass.lpszClassName, " ",
		WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZEBOX, CW_USEDEFAULT,
		CW_USEDEFAULT, winWidth, winHeight, 0, 0, NULL, 0);

	/* window err handling */
	if (_window == NULL)
	{
		char cBuff[0xFF];
		sprintf(cBuff, "Window Creation Failed!\nError Code: %d\n",
			GetLastError());
		MessageBoxA(NULL, cBuff, "CRITICAL ENGINE FAILURE", MB_OK);
		exit(1);
	}

	int glewStatus = glewInit();
	if (glewStatus != GLEW_OK)
	{
		MessageBoxA(NULL, "Could not locate OpenGL extensions!", "FATAL ERROR",
			MB_OK);
		exit(1);
	}

	/* check for missing support */
	if (glBindFramebuffer == NULL)
	{
		const char* msg = "Your OpenGL does not support Framebuffers\n"
			"This is a crucial feature used in VGraphics.dll and cannot"
			"be skipped.";
		MessageBoxA(NULL, msg, "FATAL ERROR", MB_OK);
		exit(1);
	}

	/* clear and swap to remove artifacts */
	glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	glClear(GL_COLOR_BUFFER_BIT);
	SwapBuffers(_deviceContext);

	/* create framebuffer and texture */
	glGenFramebuffers(1, &_framebuffer);
	glGenTextures(1, &_texture);
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
	glBindTexture(GL_TEXTURE_2D, _texture);
	glTexImage2D(GL_TEXTURE_2D, NULL, GL_RGB, resolution_w, resolution_h,
		NULL, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	/* set texture filter params */
	switch (linear)
	{
	case 0:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	case 1:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		break;
	default:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	}

	/* add depth to framebuffer */
	glGenRenderbuffers(1, &_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, _depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, resolution_w,
		resolution_h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER, _depth);

	/* enable depth */
	glEnable(GL_DEPTH_TEST);

	/* connect framebuffer with texture */
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		_texture, NULL);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	/* setup texture filter params */
	_tcolR = 255;
	_tcolG = 255;
	_tcolB = 255;
	_tcolA = 255;

	/* setup projection */
	_windowWidth = window_w;
	_windowHeight = window_h;
	_resW = resolution_w;
	_resH = resolution_h;

	_vpx = 0;
	_vpy = 0;
	_vpw = resolution_w;
	_vph = resolution_h;

	_rScale = 1; _useRScale = 1;
	_rOffsetX = 0; _rOffsetY = 0; _useROffset = 1;

	/* init texture editing data */
	glGenFramebuffers(1, &_eFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, _eFrameBuffer);

	/* init texture reading framebuffer */
	glGenFramebuffers(1, &_rFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, _rFrameBuffer);

	/* setup blend funcs */
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		GL_ONE, GL_ONE);

	/* set winstate to true */
	_winState = TRUE;
}

VAPI void vgTerminate(void)
{
	/* if window is open, close it */
	if (_winState) DestroyWindow(_window);
}

/* MODULE UPDATE FUNCTIONS */

static long long _lastTick = 0;
VAPI void vgUpdate(void)
{
	/* dispatch messages */
	MSG messageCheck;
	PeekMessageA(&messageCheck, NULL, NULL, NULL,
		PM_REMOVE);
	DispatchMessageA(&messageCheck);

	/* flush openGL */
	if (GetTickCount64() > _lastTick + 
		VG_FLUSH_THRESHOLD)
	{
		glFlush();
		_lastTick = GetTickCount64();
	}
}

VAPI unsigned long long vgUpdateCount(void)
{
	return _updates;
}

/* MISC RENDERING FUNCTIONS */

VAPI void vgSetWindowSize(int window_w, int window_h)
{
	/* calculate target rect */
	RECT tRect = { 0, 0, window_w, window_h };
	AdjustWindowRectExForDpi(&tRect,
		WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZEBOX |
		WS_THICKFRAME, TRUE,
		WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZEBOX |
		WS_THICKFRAME,
		GetDpiForSystem());
	int winWidth  = tRect.right - tRect.left;
	int winHeight = tRect.bottom - tRect.top;

	/* set new window pos (NOMOVE) */
	SetWindowPos(_window,
		NULL, 0, 0,
		winWidth,
		winHeight,
		SWP_NOMOVE);

	/* clear and swap to remove artifacts */
	glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SwapBuffers(_deviceContext);

	/* update window dimensions */
	_windowWidth = window_w;
	_windowHeight = window_h;
}

VAPI void vgGetResolution(int* w, int* h)
{
	*w = _resW; *h = _resH;
}

VAPI void vgSetWindowTitle(const char* title)
{
	SetWindowTextA(_window, title);
}

VAPI void vgGetScreenSize(int* width, int* height)
{
	int screenX = GetSystemMetrics(SM_CXSCREEN);
	int screenY = GetSystemMetrics(SM_CYSCREEN);
	*width  = screenX;
	*height = screenY;
}

VAPI int vgWindowIsClosed(void)
{
	return (!_winState);
}

VAPI void vgSetSwapTime(int swapTime)
{
	/* check for bad state */
	if (swapTime < VG_SWAP_TIME_MIN) return;

	/* set new swaptime */
	_swapTime = swapTime;
}

VAPI void vgUseRenderSkip(int state)
{
	_useRenderSkip = state;
}

VAPI int vgGetRenderSkipState(void)
{
	return _renderSkip && _useRenderSkip;
}

/* CLEAR AND SWAP FUNCTIONS */

VAPI void vgClear(void)
{
	RENDERSKIP(_useRenderSkip);

	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
	glViewport(0, 0, _resW, _resH);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

VAPI void vgFill(int r, int g, int b)
{
	RENDERSKIP(_useRenderSkip);

	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
	glViewport(0, 0, _resW, _resH);
	glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static ULONGLONG __lastSwap = 0;
VAPI void vgSwap(void)
{
	/* limit swap time */
	ULONGLONG currentTime = GetTickCount64();
	if ((currentTime - __lastSwap) < _swapTime)
	{
		_renderSkip = TRUE;
		return;
	}

	__lastSwap  = currentTime;
	_renderSkip = FALSE;

	/* perform swap */
	rsetup();
	
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, _texture);

	glColor4ub(255, 255, 255, 255);

	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2i(0, 0);
	glTexCoord2i(0, 1); glVertex2i(0, _windowHeight);
	glTexCoord2i(1, 1); glVertex2i(_windowWidth, _windowHeight);
	glTexCoord2i(1, 0); glVertex2i(_windowWidth, 0);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	SwapBuffers(_deviceContext);
}

/* BASIC DRAW FUNCTIONS */

VAPI void vgColor3(int r, int g, int b)
{
	_colR = r;
	_colG = g;
	_colB = b;
	_colA = 255;
}

VAPI void vgColor4(int r, int g, int b, int a)
{
	_colR = r;
	_colG = g;
	_colB = b;
	_colA = a;
}

VAPI void vgRect(int x, int y, int w, int h)
{
	RENDERSKIP(_useRenderSkip); psetup();

	glBegin(GL_QUADS);
	glVertex2i(x, y);
	glVertex2i(x, y + h);
	glVertex2i(x + w, y + h);
	glVertex2i(x + w, y);
	glEnd();
}

VAPI void vgLineSize(float size)
{
	_lineW = size;
}

VAPI void vgLine(int x1, int y1, int x2, int y2)
{
	RENDERSKIP(_useRenderSkip); psetup();

	glLineWidth(_lineW);

	glBegin(GL_LINES);
	glVertex2i(x1, y1);
	glVertex2i(x2, y2);
	glEnd();
}

VAPI void vgPointSize(float size)
{
	_pointW = size;
}

VAPI void vgPoint(int x, int y)
{
	RENDERSKIP(_useRenderSkip); psetup();

	glPointSize(_pointW);
	
	glBegin(GL_POINTS);
	glVertex2i(x, y);
	glEnd();
}

VAPI void vgViewport(int x, int y, int w, int h)
{
	_vpx = x;
	_vpy = y;
	_vpw = w;
	_vph = h;
}

VAPI void vgViewportReset(void)
{
	_vpx = 0;
	_vpy = 0;
	_vpw = _resW;
	_vph = _resH;
}

/* FLOAT VARIANTS */

VAPI void vgRectf(float x, float y, float w, float h)
{
	RENDERSKIP(_useRenderSkip); psetup();

	glBegin(GL_QUADS);
	glVertex2f(x, y);
	glVertex2f(x, y + h);
	glVertex2f(x + w, y + h);
	glVertex2f(x + w, y);
	glEnd();
}

VAPI void vgLinef(float x1, float y1, float x2, float y2)
{
	RENDERSKIP(_useRenderSkip); psetup();

	glLineWidth(_lineW);

	glBegin(GL_LINES);
	glVertex2f(x1, y1);
	glVertex2f(x2, y2);
	glEnd();
}

VAPI void vgPointf(float x, float y)
{
	RENDERSKIP(_useRenderSkip); psetup();

	glPointSize(_pointW);

	glBegin(GL_POINTS);
	glVertex2f(x, y);
	glEnd();
}

/* ADVANCED DRAW FUNCTIONS */

VAPI vgTexture vgCreateTexture(int w, int h, int linear, int repeat,
	void* data)
{
	vgTexture handle = findFreeTexture();

	glGenTextures(1, &_texBuffer[handle]);
	glBindTexture(GL_TEXTURE_2D, _texBuffer[handle]);

	glTexImage2D(GL_TEXTURE_2D, NULL, GL_RGBA, w, h, NULL, GL_RGBA,
		GL_UNSIGNED_BYTE, data);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	switch (repeat)
	{
	case 0:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		break;

	case 1:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		break;

	default:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		break;
	}

	switch (linear)
	{
	case 0:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;

	case 1:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;

	default:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	}

	return handle;
}

VAPI void vgDestroyTexture(vgTexture tex)
{
	glDeleteTextures(1, &_texBuffer[tex]);
	_texBuffer[tex] = NULL;
	_texCount--;
}

VAPI void vgUseTexture(vgTexture target)
{
	_useTex = target;
}

VAPI void vgTextureFilter(int r, int g, int b, int a)
{
	_tcolR = r;
	_tcolG = g;
	_tcolB = b;
	_tcolA = a;
}

VAPI void vgTextureFilterReset(void)
{
	_tcolR = 255;
	_tcolG = 255;
	_tcolB = 255;
	_tcolA = 255;
}

VAPI void vgRectTexture(int x, int y, int w, int h)
{
	RENDERSKIP(_useRenderSkip); psetup();

	glBindTexture(GL_TEXTURE_2D, _texBuffer[_useTex]);
	glColor4ub(_tcolR, _tcolG, _tcolB, _tcolA);

	glEnable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2i(x, y);
	glTexCoord2i(0, 1); glVertex2i(x, y + h);
	glTexCoord2i(1, 1); glVertex2i(x + w, y + h);
	glTexCoord2i(1, 0); glVertex2i(x + w, y);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

VAPI void vgRectTextureOffset(int x, int y, int w, int h, float s, float t)
{
	RENDERSKIP(_useRenderSkip); psetup();

	glBindTexture(GL_TEXTURE_2D, _texBuffer[_useTex]);
	glColor4ub(_tcolR, _tcolG, _tcolB, _tcolA);

	/* apply texture coordinate offsets */
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(s, t, 0);

	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2i(x, y);
	glTexCoord2f(0, 1); glVertex2i(x, y + h);
	glTexCoord2f(1, 1); glVertex2i(x + w, y + h);
	glTexCoord2f(1, 0); glVertex2i(x + w, y);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

VAPI vgShape vgCompileShape(float* f2d_data, int size)
{
	vgShape handle = findFreeShape();

	_shapeBuffer[handle] =  glGenLists(1);

	glNewList(_shapeBuffer[handle], GL_COMPILE);
	glBegin(GL_POLYGON);
	for (int i = 0; i < size * 2; i += 2)
	{
		glVertex2f(f2d_data[i], f2d_data[i + 1]);
	}
	glEnd();
	glEndList();

	return handle;
}

VAPI vgShape vgCompileShapeTextured(float* f2d_data, float* t2d_data,
	int size)
{
	vgShape handle = findFreeShape();

	_shapeBuffer[handle] = glGenLists(1);

	glNewList(_shapeBuffer[handle], GL_COMPILE);
	glBegin(GL_POLYGON);
	for (int i = 0; i < size * 2; i += 2)
	{
		glTexCoord2f(t2d_data[i], t2d_data[i + 1]);
		glVertex2f(f2d_data[i], f2d_data[i + 1]);
	}
	glEnd();
	glEndList();

	return handle;
}

VAPI void vgDrawShape(vgShape shape, float x, float y, float r, float s)
{
	RENDERSKIP(_useRenderSkip); psetup();

	glTranslatef(x, y, 0); /* lastly, transalate */
	glRotatef(r, 0, 0, 1); /* second, rotate */
	glScalef(s, s, 1); /* first, scale */

	glCallList(_shapeBuffer[shape]);
}

VAPI void vgDrawShapeTextured(vgShape shape, float x, float y, float r,
	float s)
{
	RENDERSKIP(_useRenderSkip); psetup();

	glTranslatef(x, y, 0); /* lastly, transalate */
	glRotatef(r, 0, 0, 1); /* second, rotate */
	glScalef(s, s, 1); /* first, scale */

	glBindTexture(GL_TEXTURE_2D, _texBuffer[_useTex]);
	glColor4ub(_tcolR, _tcolG, _tcolB, _tcolA);

	glEnable(GL_TEXTURE_2D);
	glCallList(_shapeBuffer[shape]);
	glDisable(GL_TEXTURE_2D);
}

VAPI void vgRenderScale(float scale)
{
	_rScale = scale;
}

VAPI void vgUseRenderScaling(int value) 
{
	_useRScale = value;
}

VAPI void vgRenderOffset(float x, float y)
{
	_rOffsetX = x;
	_rOffsetY = y;
}

VAPI void vgUseRenderOffset(int value)
{
	_useROffset = value;
}

VAPI void vgRenderLayer(float layer)
{
	_layer = min(0, -layer);
}

VAPI int vgCheckIfViewable(float x, float y, float extra)
{
	/* undo scale and transform */
	if (_useROffset)
	{
		x -= _rOffsetX;
		y -= _rOffsetY;
	}
	if (_useRScale)
	{
		x /= _rScale;
		y /= _rScale;
	}

	/* object is now in "normalized screenspace" */
	/* compare if object is seeable */
	if (fabsf(x) > (1.0f + extra)) return 0;
	if (fabsf(y) > (1.0f + extra)) return 0;

	return 1;
}

/* ITEX FUNCTIONS */

VAPI void vgITexDataClear(void)
{
	for (int i = 0; i < VG_ITEX_COLORS_MAX; i++)
	{
		_icolorR[i] = NULL;
		_icolorG[i] = NULL;
		_icolorB[i] = NULL;
		_icolorA[i] = NULL;
	}

	for (int i = 0; i < VG_ITEX_SIZE_MAX; i++)
	{
		for (int j = 0; j < VG_ITEX_SIZE_MAX; j++)
		{
			_indexes[i][j] = NULL;
		}
	}
}

VAPI void vgITexDataColor(unsigned short index, int r, int g, int b, int a)
{
	_icolorR[index] = r;
	_icolorG[index] = g;
	_icolorB[index] = b;
	_icolorA[index] = a;
}

VAPI void vgITexDataIndex(unsigned short index, int x, int y)
{
	_indexes[x][y] = index;
}

VAPI void vgITexDataIndexArray(unsigned short index, int* vx, int* vy,
	int size)
{
	for (int i = 0; i < size; i++)
	{
		_indexes[vx[i]][vy[i]] = index;
	}
}

VAPI vgTexture vgITexDataCompile(int width, int height, int repeat,
	int linear)
{
	/* create 2D compacted array buffer for color data to be stored */
	unsigned char* colorBuffer;
	colorBuffer = malloc(sizeof(unsigned char) * width * height * 4);
	if (colorBuffer == NULL) return 0;

	int writeIndex = 0;

	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			int colorIndex = _indexes[i][j];
			colorBuffer[writeIndex + 0] = (unsigned char)_icolorR[colorIndex];
			colorBuffer[writeIndex + 1] = (unsigned char)_icolorG[colorIndex];
			colorBuffer[writeIndex + 2] = (unsigned char)_icolorB[colorIndex];
			colorBuffer[writeIndex + 3] = (unsigned char)_icolorA[colorIndex];
			writeIndex += 4;
		}
	}

	vgTexture handle = vgCreateTexture(width, height, linear, repeat,
		colorBuffer);

	free(colorBuffer);

	return handle;
}

/* TEXTURE EDITING FUNCTIONS */

VAPI void vgEditTexture(vgTexture target, int w, int h)
{
	/* bind editing framebuffer to target texture */
	glBindFramebuffer(GL_FRAMEBUFFER, _eFrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		_texBuffer[target], NULL);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	/* setup other data */
	_eWidth = w;
	_eHeight = h;
}

VAPI void vgEditColor(int r, int g, int b, int a)
{
	_ecolR = r;
	_ecolG = g;
	_ecolB = b;
	_ecolA = a;
}

VAPI void vgEditPoint(int x, int y)
{
	esetup();

	glPointSize(1);

	glBegin(GL_POINTS);
	glVertex2i(x, y);
	glEnd();
}

VAPI void vgEditLine(int x1, int y1, int x2, int y2)
{
	esetup();

	glLineWidth(1);

	glBegin(GL_LINES);
	glVertex2i(x1, y1);
	glVertex2i(x2, y2);
	glEnd();
}

VAPI void vgEditRect(int x, int y, int w, int h)
{
	esetup();

	glBegin(GL_QUADS);
	glVertex2i(x, y);
	glVertex2i(x, y + h);
	glVertex2i(x + w, y + h);
	glVertex2i(x + w, y);
	glEnd();
}

VAPI void vgEditShape(vgShape shape, float x, float y, float r, float s)
{
	esetup();

	glTranslatef(x, y, 0); /* third, transalate */
	glScalef(s, s, 1); /* second, scale */
	glRotatef(r, 0, 0, 1); /* first, rotate */

	glCallList(_shapeBuffer[shape]);
}

VAPI void vgEditUseTexture(vgTexture tex)
{
	_euTex = tex;
}

VAPI void vgEditShapeTextured(vgShape shape, float x, float y, float r,
	float s)
{
	esetup();

	glTranslatef(x, y, 0); /* third, transalate */
	glScalef(s, s, 1); /* second, scale */
	glRotatef(r, 0, 0, 1); /* first, rotate */

	glBindTexture(GL_TEXTURE_2D, _texBuffer[_euTex]);

	glColor4ub(_tcolR, _tcolG, _tcolB, _tcolA);

	glEnable(GL_TEXTURE_2D);
	glCallList(_shapeBuffer[shape]);
	glDisable(GL_TEXTURE_2D);
}

VAPI void vgEditSetData(int width, int height, void* data)
{
	esetup();

	glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

VAPI void vgEditClear(void)
{
	esetup();
	
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}

VAPI void* vgGetTextureData(vgTexture tex, int w, int h)
{
	/* bind FB and texture */
	glBindFramebuffer(GL_FRAMEBUFFER, _rFrameBuffer);
	glBindTexture(GL_TEXTURE_2D, _texBuffer[tex]);

	/* connect the two */
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		_texBuffer[tex], NULL);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
	int size = (w * h * 4);

	void* data = calloc(1, sizeof(unsigned char) * size);

	if (data == NULL) return NULL;

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);

	return data;
}

/* CURSOR RELATED FUNCTIONS */

VAPI void vgGetCursorPos(int* x, int* y)
{
	POINT p; /* cursor point */

	/* get cursor position and convert to window pos */
	GetCursorPos(&p);
	ScreenToClient(_window, &p);
	
	/* set params*/
	*x = p.x; *y = p.y;
}

VAPI void vgGetCursorPosScaled(float* x, float* y)
{
	/* get mouse coordinates */
	int   mx, my;
	float fx, fy;
	vgGetCursorPos(&mx, &my);
	fx = (float)mx;
	fy = (float)my;

	/* first, offset to center so that (W/2, H/2) maps to (0, 0)*/
	fx -= ((float)_windowWidth  / 2.0f);
	fy -= ((float)_windowHeight / 2.0f);

	/* normalize position so that (W,H) maps to (1, 1) */
	fx /= (float)((float)_windowWidth / 2.0f);
	fy /= (float)((float)_windowHeight / 2.0f);
	fy *= -1; /* flip y coord */

	/* scale and transform */
	if (_useRScale)
	{
		fx *= _rScale;
		fy *= _rScale;
	}
	if (_useROffset)
	{
		fx += _rOffsetX;
		fy += _rOffsetY;
	}

	/* convert back to int and return */
	*x = fx; *y = fy;
}

VAPI int vgOnLeftClick(void)
{
	return GetKeyState(VK_LBUTTON) < 0;
}

VAPI int vgOnRightClick(void)
{
	return GetKeyState(VK_RBUTTON) < 0;
}

VAPI int vgCursorOverlap(float x, float y, float w, float h)
{
	float mx, my;
	vgGetCursorPosScaled(&mx, &my);
	return (mx > x && mx < x + w) && (my > y && my < y + h);
}

/* TEXTURE LOADING AND SAVING FUNCTIONS */

VAPI void vgSaveTexture(vgTexture texture, const char* file, int w, int h)
{
	/* create file and open */
	FILE* output;
	output = fopen(file, "w");
	
	/* get texture data */
	unsigned char* tData = vgGetTextureData(texture, w, h);

	/* write to file */
	fwrite(tData, sizeof(unsigned char), w * h * 4, output);

	/* free memory and close file */
	free(tData);
	fflush(output); fclose(output);
}

VAPI vgTexture vgLoadTexture(const char* file, int w, int h, int linear,
	int repeat)
{
	/* allocate data buffer */
	unsigned char* buffer = calloc(1, w * h * 4);
	if (buffer == 0) return NULL;

	/* open file and read */
	FILE* rFile = fopen(file, "r");

	fread(buffer, sizeof(unsigned char), w * h * 4, rFile);

	/* create texture, free and close */
	vgTexture rTex = vgCreateTexture(w, h, linear, repeat, buffer);

	free(buffer);
	fflush(rFile);
	fclose(rFile);

	return rTex;
}

VAPI void* vgLoadTextureData(const char* file, int w, int h)
{
	/* allocate data buffer */
	unsigned char* buffer = calloc(1, w * h * 4);
	if (buffer == 0) return NULL;

	/* open file and read */
	FILE* rFile = fopen(file, "r");

	fread(buffer, sizeof(unsigned char), w * h * 4, rFile);

	return buffer;
}

/* DEBUG FUNCTIONS */

VAPI unsigned int _vgDebugGetTextureName(vgTexture texture)
{
	return _texBuffer[texture];
}

VAPI unsigned int _vgDebugGetShapeName(vgShape shape)
{
	return _shapeBuffer[shape];
}

VAPI unsigned int _vgDebugGetFramebuffer(void)
{
	return _framebuffer;
}

VAPI void* _vgDebugGetWindowHandle(void)
{
	return _window;
}

