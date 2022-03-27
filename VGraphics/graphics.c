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

#include <glew.h> /* OpenGL extension library */
#include <gl/GL.h> /* Graphics library */
#include <gl/GLU.h> /* Extened graphics library */
#include <glfw3.h> /* Window handler library */

#include "graphics.h" /* Header */

/* DEFINITIONS */
#undef NULL
#define NULL 0

/* ========INTERNAL RESOURCES======== */

/* window and rendering data */
static GLFWwindow* _window;
static GLuint _framebuffer;
static GLuint _texture;
static GLuint _depth;
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
static GLuint _eFrameBuffer = 0;
static GLuint _rFrameBuffer = 0;
static int _ecolR, _ecolG, _ecolB, _ecolA = 0;
static int _eWidth, _eHeight = 0;
static vgTexture _euTex;

/* ================================== */

/* INTERNAL HELPER FUNCTIONS */

static inline void psetup(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	/* recall that the camera has to cover twice as much rendering space */
	/* in order to make the rendered "image" twice as small */
	/* so a 0.5 scale would require a rendering space 2 times larger */

	if (_useRScale && _rScale != 0)
	{
		const float resScaleX = (float)_resW * (1.0f / _rScale); /* rightmost */
		const float resScaleY = (float)_resH * (1.0f / _rScale); /* topmost */
		const float resOffsetW = resScaleX - (float)_resW; /* leftmost */
		const float resOffsetH = resScaleY - (float)_resH; /* bottommost */

		gluOrtho2D(-resOffsetW, resScaleX, -resOffsetH, resScaleY);
	}
	else
	{
		gluOrtho2D(0, _resW, 0, _resH);
		
	}

	glViewport(_vpx, _vpy, _vpw, _vph);
	glColor4ub(_colR, _colG, _colB, _colA);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, _layer);

	if (_useROffset)
	{
		glTranslatef(-_rOffsetX, -_rOffsetY, 0);
	}
}

static inline void rsetup(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, NULL);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, _windowWidth, 0, _windowHeight);

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
	gluOrtho2D(0, _eWidth, 0, _eHeight);

	glViewport(0, 0, _eWidth, _eHeight);

	glColor4ub(_ecolR, _ecolG, _ecolB, _ecolA);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static inline vgTexture findFreeTexture(void)
{
	for (int i = 0; i < VG_TEXTURES_MAX; i++)
	{
		if (_texBuffer[i] == NULL)
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

static inline void resizeCallback(GLFWwindow* win, int w, int h)
{
	_windowWidth = w;
	_windowHeight = h;
}

/* INIT AND TERMINATE FUNCTIONS */

VAPI void vgInit(int window_w, int window_h, int resolution_w,
	int resolution_h, int decorated, int resizeable, int linear)
{
	_updates = 0;
	_layer = 1.0f;

	int status = glfwInit();

	/* err checking */
	if (status == GLFW_FALSE)
	{
		MessageBox(NULL, L"GLFW failed to initialize",
			L"FATAL ERROR", MB_OK);
		exit(1);
	}
	int err = glfwGetError(NULL);
	if (err != GLFW_NO_ERROR)
	{
		wchar_t buff[255];
		swprintf(buff, 255, L"GLFW ERR: %d", err);
		MessageBox(NULL, buff, L"FATAL ERROR", MB_OK);
		exit(1);
	}

	glfwWindowHint(GLFW_DECORATED, decorated);
	glfwWindowHint(GLFW_RESIZABLE, resizeable);

	_window = glfwCreateWindow(window_w, window_h, " ", NULL, NULL);

	/* window err handling */
	if (_window == NULL)
	{
		int errcode = glfwGetError(NULL);
		wchar_t buff[255];
		swprintf(buff, 255, L"Window creation failed!\nGLFW err code: %d", errcode);
		MessageBox(NULL, buff, L"FATAL ERROR", MB_OK);
		exit(1);
	}

	glfwSetWindowSizeLimits(_window, VG_WINDOW_SIZE_MIN, VG_WINDOW_SIZE_MIN,
		-1, -1);
	glfwMakeContextCurrent(_window);
	glfwSwapInterval(1);

	int glewStatus = glewInit();
	if (glewStatus != GLEW_OK)
	{
		MessageBox(NULL, L"Could not locate OpenGL extensions!", L"FATAL ERROR",
			MB_OK);
		exit(1);
	}

	/* check for missing support */
	if (glBindFramebuffer == NULL)
	{
		const wchar_t* msg = L"Your OpenGL does not support Framebuffers\n"
			L"This is a crucial feature used in VGraphics.dll and cannot"
			L"be skipped.";
		MessageBox(NULL, msg, L"FATAL ERROR", MB_OK);
		exit(1);
	}

	/* clear and swap to remove artifacts */
	glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	glClear(GL_COLOR_BUFFER_BIT);
	glfwSwapBuffers(_window);

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
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* setup window resize callback */
	glfwSetWindowSizeCallback(_window, resizeCallback);
}

VAPI void vgTerminate(void)
{
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

	glfwTerminate();
}

/* MODULE UPDATE FUNCTIONS */

static long long _lastTick = 0;
VAPI void vgUpdate(void)
{
	glfwPollEvents();
	_updates++;

	if (GetTickCount64() > _lastTick + 
		VG_FLUSH_THRESHOLD)
	{
		glFlush();
		_lastTick = GetTickCount64();
	}
}

VAPI int vgWindowShouldClose(void)
{
	return glfwWindowShouldClose(_window);
}

VAPI unsigned long long vgUpdateCount(void)
{
	return _updates;
}

/* MISC RENDERING FUNCTIONS */

VAPI void vgSetWindowSize(int window_w, int window_h)
{
	glfwSetWindowSize(_window, window_w, window_h);
	_windowWidth = window_w;
	_windowHeight = window_h;

	/* clear and swap to remove artifacts */
	glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glfwSwapBuffers(_window);
}

VAPI void vgGetResolution(int* w, int* h)
{
	*w = _resW;
	*h = _resH;
}

VAPI void vgSetWindowTitle(const char* title)
{
	glfwSetWindowTitle(_window, title);
}

VAPI void vgGetScreenSize(int* width, int* height)
{
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* dims = glfwGetVideoMode(monitor);
	*width = dims->width;
	*height = dims->height;
}

/* CLEAR AND SWAP FUNCTIONS */

VAPI void vgClear(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
	glViewport(0, 0, _resW, _resH);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

VAPI void vgFill(int r, int g, int b)
{
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
	glViewport(0, 0, _resW, _resH);
	glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

VAPI void vgSwap(void)
{
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

	glfwSwapBuffers(_window);
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
	psetup();

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
	psetup();

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
	psetup();

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
	psetup();

	glBegin(GL_QUADS);
	glVertex2f(x, y);
	glVertex2f(x, y + h);
	glVertex2f(x + w, y + h);
	glVertex2f(x + w, y);
	glEnd();
}

VAPI void vgLinef(float x1, float y1, float x2, float y2)
{
	psetup();

	glLineWidth(_lineW);

	glBegin(GL_LINES);
	glVertex2f(x1, y1);
	glVertex2f(x2, y2);
	glEnd();
}

VAPI void vgPointf(float x, float y)
{
	psetup();

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
	psetup();

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
	psetup();

	glBindTexture(GL_TEXTURE_2D, _texBuffer[_useTex]);
	glColor4ub(_tcolR, _tcolG, _tcolB, _tcolA);

	glEnable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);
	glTexCoord2f(0 + s, 0 + t); glVertex2i(x, y);
	glTexCoord2f(0 + s, 1 + t); glVertex2i(x, y + h);
	glTexCoord2f(1 + s, 1 + t); glVertex2i(x + w, y + h);
	glTexCoord2f(1 + s, 0 + t); glVertex2i(x + w, y);
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
	psetup();

	glTranslatef(x, y, 0); /* lastly, transalate */
	glRotatef(r, 0, 0, 1); /* second, rotate */
	glScalef(s, s, 1); /* first, scale */

	glCallList(_shapeBuffer[shape]);
}

VAPI void vgDrawShapeTextured(vgShape shape, float x, float y, float r,
	float s)
{
	psetup();

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
	
}

VAPI void vgRenderLayer(unsigned char layer)
{
	_layer = (float)layer / 255.0f;
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
	double mx = 0;
	double my = 0;

	glfwGetCursorPos(_window, &mx, &my);

	my -= _windowHeight;

	int cx = (int)mx;
	int cy = (int)-my;
	*x = cx;
	*y = cy;
}

VAPI void vgGetCursorPosScaled(int* x, int* y)
{
	/* get mouse coordinates */
	int mx, my;
	float fx, fy;
	vgGetCursorPos(&mx, &my);
	fx = (float)mx;
	fy = (float)my;

	/* get viewport dimensions */
	const float resScaleX = (float)_resW * (1.0f / _rScale); /* rightmost */
	const float resScaleY = (float)_resH * (1.0f / _rScale); /* topmost */
	const float resOffsetW = -(resScaleX - (float)_resW); /* leftmost */
	const float resOffsetH = -(resScaleY - (float)_resH); /* bottommost */
	const float ratioX = _windowWidth / _resW;
	const float ratioY = _windowHeight / _resH;

	/* apply some scaling */
	fx *= 1.0f / _rScale; /* scale by rScale */
	fy *= 1.0f / _rScale;
	fx *= 1.0f - (1.0f * (_rScale / ratioX)); /* scale to vp + offset */
	fy *= 1.0f - (1.0f * (_rScale / ratioY));
	fx += resOffsetW; /* offset by vp offset */
	fy += resOffsetH;
	
	/* apply render offset */
	if (_useROffset)
	{
		fx += _rOffsetX;
		fy += _rOffsetY;
	}

	*x = (int)fx;
	*y = (int)fy;
}

VAPI void vgGetCursorPosScaledT(int* rx, int* ry, int x, int y, int w, int h,
	int sub_w, int sub_h)
{
	int mx, my;
	float fx, fy;
	vgGetCursorPosScaled(&mx, &my);
	fx = (float)mx; fy = (float)my;

	fx -= x;
	fy -= y;
	fx /= (float)w;
	fy /= (float)h;
	fx *= sub_w;
	fy *= sub_h;

	*rx = (int)fx;
	*ry = (int)fy;
}

VAPI int vgOnLeftClick(void)
{
	return -(GetKeyState(VK_LBUTTON) >> 15);
}

VAPI int vgOnRightClick(void)
{
	return -(GetKeyState(VK_RBUTTON) >> 15);
}

VAPI int vgCursorOverlap(int x, int y, int w, int h)
{
	int mx, my;
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

