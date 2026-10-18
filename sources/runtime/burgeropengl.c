// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
// LINK: after itattr; data EVNOVA 0x00582028
/* burgeropengl.c -- BurgerLib GrOpenGL: 2D drawing and texture upload through OpenGL; not linked. Notes: Research/notes/burgeropengl.md */

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "burgergl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern Word ImageInit(Image_t *Output, Word Width, Word Height, ImageTypes_e Depth);
extern Word ImageStore888(Image_t *Output, const Image_t *Input);
extern Word ImageStore8888(Image_t *Output, const Image_t *Input);

#define ScreenUse2DCoords(x) if (ScreenUsing2DCoords != (x)) { ScreenForceUse2DCoords(x); }
#define ScreenSetTexture(x) if (ScreenCurrentTexture != (x)) { ScreenForceTexture(x); }

void OpenGLInit(void)
{
    glViewport(0, 0, ScreenWidth, ScreenHeight);
}

void OpenGLSolidRect(int x, int y, Word Width, Word Height, Word32 Color)
{
    ScreenUse2DCoords(1);
    ScreenSetTexture(0);
    glBegin(GL_QUADS);
    glColor4ub((Word8)(Color >> 16), (Word8)(Color >> 8), (Word8)Color, (Word8)(Color >> 24));
    glVertex2i(x, y);
    glVertex2i(x + Width, y);
    glVertex2i(x + Width, y + Height);
    glVertex2i(x, y + Height);
    glEnd();
}

void OpenGLTextureDraw2D(Word TexNum, int x, int y, Word Width, Word Height)
{
    ScreenUse2DCoords(1);
    ScreenSetTexture(TexNum);
    glBegin(GL_QUADS);
    glColor4ub(255, 255, 255, 255);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2i(x, y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2i(x + Width, y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2i(x + Width, y + Height);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2i(x, y + Height);
    glEnd();
}

void OpenGLTextureDraw2DSubRect(Word TexNum, int x, int y, Word Width, Word Height,
                                const float *UVPtr)
{
    ScreenUse2DCoords(1);
    ScreenSetTexture(TexNum);
    glBegin(GL_QUADS);
    glColor4ub(255, 255, 255, 255);
    glTexCoord2f(UVPtr[0], UVPtr[1]);
    glVertex2i(x, y);
    glTexCoord2f(UVPtr[2], UVPtr[1]);
    glVertex2i(x + Width, y);
    glTexCoord2f(UVPtr[2], UVPtr[3]);
    glVertex2i(x + Width, y + Height);
    glTexCoord2f(UVPtr[0], UVPtr[3]);
    glVertex2i(x, y + Height);
    glEnd();
}

Word OpenGLLoadTexture(Word *TextureNum, const Image_t *ImagePtr)
{
    Image_t Temp;
    int MaxSize;
    Word Width;
    Word Height;
    unsigned TexNum;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxSize);
    Width = PowerOf2(ImagePtr->Width);
    Height = PowerOf2(ImagePtr->Height);
    if (Width > (Word)MaxSize)
        Width = MaxSize;
    if (Height > (Word)MaxSize)
        Height = MaxSize;
    glGenTextures(1, &TexNum);
    *TextureNum = TexNum;
    glBindTexture(GL_TEXTURE_2D, TexNum);
    switch (ImagePtr->DataType) {
    case IMAGE888:
        if (Width == ImagePtr->Width && Height == ImagePtr->Height) {
            glTexImage2D(GL_TEXTURE_2D, 0, 3, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                         ImagePtr->ImagePtr);
            break;
        }
    case IMAGE332:
    case IMAGE8_PAL:
    case IMAGE555:
    case IMAGE565:
        if (ImageInit(&Temp, Width, Height, IMAGE888) != 0)
            return 1;
        ImageStore888(&Temp, ImagePtr);
        glTexImage2D(GL_TEXTURE_2D, 0, 3, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     Temp.ImagePtr);
        ImageDestroy(&Temp);
        break;
    case IMAGE8888:
        if (Width == ImagePtr->Width && Height == ImagePtr->Height) {
            glTexImage2D(GL_TEXTURE_2D, 0, 4, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         ImagePtr->ImagePtr);
            break;
        }
    default:
        if (ImageInit(&Temp, Width, Height, IMAGE8888) != 0)
            return 1;
        ImageStore8888(&Temp, ImagePtr);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     Temp.ImagePtr);
        ImageDestroy(&Temp);
        break;
    }
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    return 0;
}

#ifdef __cplusplus
}
#endif
