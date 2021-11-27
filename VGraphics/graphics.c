/******************************************************************************
* <graphics.c>
* Bailey Jia-Tao Brown
* 2021
*
*	Source file for primitive graphics library
*	Contents:
*		- Preprocessor defs
*		- Includes
*		- Internal resources
*		- Internal helper functions
*		- Module init and terminate functions
*		- Module update functions
*		- Misc rendering functions
*		- Clear, swap and fill functions
*		- Basic draw functions
*		- Advanced draw functions
*		- ITex functions
*		- Texture editing functions
*
******************************************************************************/

/* PREPROCESSOR DEFS */
#define GLEW_STATIC
#define WIN32_LEAN_AND_MEAN

/* INCLUDES */
#include <stdio.h> /* I/O */
#include <stdlib.h> /* Memory allocation */

#include <Windows.h> /* OpenGL dependency */

#include <glew.h> /* OpenGL extension library */
#include <gl/GL.h> /* Graphics library */
#include <gl/GLU.h> /* Extened graphics library */
#include <glfw3.h> /* Window handler library */

#include "graphics.h" /* Header */

/* ========INTERNAL RESOURCES======== */

/* window and rendering data */
static GLFWwindow* _window;
static GLuint _framebuffer;
static GLuint _texture;
static int _vpx, _vpy, _vpw, _vph;
static int _windowWidth;
static int _windowHeight;
static int _resW;
static int _resH;

/* buffer data */
static GLuint _texBuffer[VG_TEXTURES_MAX] = { 0 };
static GLuint _shapeBuffer[VG_SHAPES_MAX] = { 0 };

/* update data */
static unsigned long long _updates = 0;

/* color and size related data */
static int _colR, _colG, _colB, _colA = 0;
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
	gluOrtho2D(0, _resW, 0, _resH);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glViewport(_vpx, _vpy, _vpw, _vph);

	glColor4ub(_colR, _colG, _colB, _colA);
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

	glfwInit();
	glfwWindowHint(GLFW_DECORATED, decorated);
	glfwWindowHint(GLFW_RESIZABLE, resizeable);

	_window = glfwCreateWindow(window_w, window_h, " ", NULL, NULL);
	glfwSetWindowSizeLimits(_window, VG_WINDOW_SIZE_MIN, VG_WINDOW_SIZE_MIN,
		-1, -1);
	glfwMakeContextCurrent(_window);
	glfwSwapInterval(1);

	glewInit();

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

	/* connect framebuffer with texture */
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		_texture, NULL);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	/* setup projection */
	_windowWidth = window_w;
	_windowHeight = window_h;
	_resW = resolution_w;
	_resH = resolution_h;

	_vpx = 0;
	_vpy = 0;
	_vpw = resolution_w;
	_vph = resolution_h;

	/* init texture editing data */
	glGenFramebuffers(1, &_eFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, &_eFrameBuffer);

	/* init texture reading framebuffer */
	glGenFramebuffers(1, &_rFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, &_rFrameBuffer);

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

VAPI void vgUpdate(void)
{
	glfwPollEvents();
	_updates++;
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

/* CLEAR AND SWAP FUNCTIONS */

VAPI void vgClear(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

VAPI void vgFill(int r, int g, int b)
{
	psetup();

	glColor3ub(r, g, b);

	glBegin(GL_QUADS);
	glVertex2f(0.0, 0.0);
	glVertex2f(0.0, _resH);
	glVertex2f(_resW, _resH);
	glVertex2f(_resW, 0.0);
	glEnd();
}

VAPI void vgSwap(void)
{
	rsetup();

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, _texture);

	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(0.0, 0.0);
	glTexCoord2f(0, 1); glVertex2f(0.0, _windowHeight);
	glTexCoord2f(1, 1); glVertex2f(_windowWidth, _windowHeight);
	glTexCoord2f(1, 0); glVertex2f(_windowWidth, 0.0);
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
	glVertex2f(x, y);
	glVertex2f(x, y + h);
	glVertex2f(x + w, y + h);
	glVertex2f(x + w, y);
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
	glVertex2f(x1, y1);
	glVertex2f(x2, y2);
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

VAPI void vgRectTexture(int x, int y, int w, int h)
{
	psetup();

	glBindTexture(GL_TEXTURE_2D, _texBuffer[_useTex]);
	glColor4ub(255, 255, 255, 255);

	glEnable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);
	glTexCoord2d(0, 0); glVertex2f(x, y);
	glTexCoord2d(0, 1); glVertex2f(x, y + h);
	glTexCoord2d(1, 1); glVertex2f(x + w, y + h);
	glTexCoord2d(1, 0); glVertex2f(x + w, y);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

VAPI void vgRectTextureOffset(int x, int y, int w, int h, float s, float t)
{
	psetup();

	glBindTexture(GL_TEXTURE_2D, _texBuffer[_useTex]);
	glColor4ub(255, 255, 255, 255);

	glEnable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);
	glTexCoord2f(0 + s, 0 + t); glVertex2f(x, y);
	glTexCoord2f(0 + s, 1 + t); glVertex2f(x, y + h);
	glTexCoord2f(1 + s, 1 + t); glVertex2f(x + w, y + h);
	glTexCoord2f(1 + s, 0 + t); glVertex2f(x + w, y);
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

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(x, y, 0); /* lastly, transalate */
	glRotatef(r, 0, 0, 1); /* second, rotate */
	glScalef(s, s, 1); /* first, scale */

	glCallList(_shapeBuffer[shape]);
}

VAPI void vgDrawShapeTextured(vgShape shape, float x, float y, float r,
	float s)
{
	psetup();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(x, y, 0); /* lastly, transalate */
	glRotatef(r, 0, 0, 1); /* second, rotate */
	glScalef(s, s, 1); /* first, scale */

	glBindTexture(GL_TEXTURE_2D, _texBuffer[_useTex]);
	glColor4ub(255, 255, 255, 255);

	glEnable(GL_TEXTURE_2D);
	glCallList(_shapeBuffer[shape]);
	glDisable(GL_TEXTURE_2D);
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
	colorBuffer = malloc(width * height * 4);
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

	glEnable(GL_TEXTURE_2D);
	glCallList(_shapeBuffer[shape]);
	glDisable(GL_TEXTURE_2D);
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

	if (data == NULL) printf("FAILED!");

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);

	return data;
}

