/* burgergl.h -- the OpenGL entry points and the 2D / texture state BurgerLib's GrOpenGL and SSScreenShape both use. Types and declarations only. */

#ifndef BURGERGL_H
#define BURGERGL_H

#include "burgertypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GL_QUADS                0x0007
#define GL_MAX_TEXTURE_SIZE     0x0d33
#define GL_TEXTURE_2D           0x0de1
#define GL_MODELVIEW            0x1700
#define GL_PROJECTION           0x1701
#define GL_UNSIGNED_BYTE        0x1401
#define GL_RGB                  0x1907
#define GL_RGBA                 0x1908
#define GL_MODULATE             0x2100
#define GL_TEXTURE_ENV_MODE     0x2200
#define GL_TEXTURE_ENV          0x2300
#define GL_NEAREST              0x2600
#define GL_LINEAR               0x2601
#define GL_TEXTURE_MAG_FILTER   0x2800
#define GL_TEXTURE_MIN_FILTER   0x2801
#define GL_TEXTURE_WRAP_S       0x2802
#define GL_TEXTURE_WRAP_T       0x2803
#define GL_REPEAT               0x2901

extern void __stdcall glBegin(unsigned mode);
extern void __stdcall glEnd(void);
extern void __stdcall glViewport(int x, int y, int width, int height);
extern void __stdcall glColor4ub(unsigned char red, unsigned char green, unsigned char blue,
                                 unsigned char alpha);
extern void __stdcall glVertex2i(int x, int y);
extern void __stdcall glTexCoord2f(float s, float t);
extern void __stdcall glGetIntegerv(unsigned pname, int *params);
extern void __stdcall glGenTextures(int n, unsigned *textures);
extern void __stdcall glBindTexture(unsigned target, unsigned texture);
extern void __stdcall glTexImage2D(unsigned target, int level, int components, int width,
                                   int height, int border, unsigned format, unsigned type,
                                   const void *pixels);
extern void __stdcall glTexParameterf(unsigned target, unsigned pname, float param);
extern void __stdcall glTexEnvf(unsigned target, unsigned pname, float param);
extern void __stdcall glMatrixMode(unsigned mode);
extern void __stdcall glPushMatrix(void);
extern void __stdcall glPopMatrix(void);
extern void __stdcall glLoadIdentity(void);
extern void __stdcall glOrtho(double left, double right, double bottom, double top, double zNear,
                              double zFar);

extern Word ScreenUsing2DCoords;
extern Word32 ScreenCurrentTexture;
extern void ScreenForceTexture(Word32 TexNum);
extern void ScreenForceUse2DCoords(Word use2d);

#ifdef __cplusplus
}
#endif

#endif
