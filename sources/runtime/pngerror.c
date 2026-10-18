// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -g -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libpng 1.2.7's one line, compiler and headers (zlib's compiler, built with debug information on, against the 2.4-era MSL). -g moves jump threading and register choice; the 2.4-era math_x87.h makes fabs an asm inline; build 335 has no -proc generic, its default is generic x86; Research/runtime-compiler-ceiling.md
/* pngerror.c -- libpng 1.2.7 pngerror.c, Copyright (c) 1998-2004 Glenn Randers-Pehrson, (c) 1996-1997 Andreas Dilger, (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.; libpng licence in png.h; altered: comments and the #if branches this build does not take removed (the functions the exe does not link stay, unmarked). Notes: Research/notes/png.md */

#define PNG_INTERNAL
#include "png.h"

static void
png_default_error PNGARG((png_structp png_ptr,
  png_const_charp error_message));
static void
png_default_warning PNGARG((png_structp png_ptr,
  png_const_charp warning_message));

/* png_error. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00536f3f
void PNGAPI
png_error(png_structp png_ptr, png_const_charp error_message)
{
   char msg[16];
   if (png_ptr->flags&(PNG_FLAG_STRIP_ERROR_NUMBERS|PNG_FLAG_STRIP_ERROR_TEXT))
   {
     if (*error_message == '#')
     {
         int offset;
         for (offset=1; offset<15; offset++)
            if (*(error_message+offset) == ' ')
                break;
         if (png_ptr->flags&PNG_FLAG_STRIP_ERROR_TEXT)
         {
            int i;
            for (i=0; i<offset-1; i++)
               msg[i]=error_message[i+1];
            msg[i]='\0';
            error_message=msg;
         }
         else
            error_message+=offset;
     }
     else
     {
         if (png_ptr->flags&PNG_FLAG_STRIP_ERROR_TEXT)
         {
            msg[0]='0';
            msg[1]='\0';
            error_message=msg;
         }
     }
   }
   if (png_ptr != NULL && png_ptr->error_fn != NULL)
      (*(png_ptr->error_fn))(png_ptr, error_message);

   png_default_error(png_ptr, error_message);
}

/* png_warning. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00536efe
void PNGAPI
png_warning(png_structp png_ptr, png_const_charp warning_message)
{
   int offset = 0;
   if (png_ptr->flags&(PNG_FLAG_STRIP_ERROR_NUMBERS|PNG_FLAG_STRIP_ERROR_TEXT))
   {
     if (*warning_message == '#')
     {
         for (offset=1; offset<15; offset++)
            if (*(warning_message+offset) == ' ')
                break;
     }
   }
   if (png_ptr != NULL && png_ptr->warning_fn != NULL)
      (*(png_ptr->warning_fn))(png_ptr, warning_message+offset);
   else
      png_default_warning(png_ptr, warning_message+offset);
}

#define isnonalpha(c) ((c) < 65 || (c) > 122 || ((c) > 90 && (c) < 97))
static PNG_CONST char png_digit[16] = {
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   'A', 'B', 'C', 'D', 'E', 'F'
};

/* png_format_buffer. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00536e56
static void
png_format_buffer(png_structp png_ptr, png_charp buffer, png_const_charp
   error_message)
{
   int iout = 0, iin = 0;

   while (iin < 4)
   {
      int c = png_ptr->chunk_name[iin++];
      if (isnonalpha(c))
      {
         buffer[iout++] = '[';
         buffer[iout++] = png_digit[(c & 0xf0) >> 4];
         buffer[iout++] = png_digit[c & 0x0f];
         buffer[iout++] = ']';
      }
      else
      {
         buffer[iout++] = (png_byte)c;
      }
   }

   if (error_message == NULL)
      buffer[iout] = 0;
   else
   {
      buffer[iout++] = ':';
      buffer[iout++] = ' ';
      png_strncpy(buffer+iout, error_message, 63);
      buffer[iout+63] = 0;
   }
}

/* png_chunk_error. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00536e2a
void PNGAPI
png_chunk_error(png_structp png_ptr, png_const_charp error_message)
{
   char msg[18+64];
   png_format_buffer(png_ptr, msg, error_message);
   png_error(png_ptr, msg);
}

/* png_chunk_warning. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00536dc6
void PNGAPI
png_chunk_warning(png_structp png_ptr, png_const_charp warning_message)
{
   char msg[18+64];
   png_format_buffer(png_ptr, msg, warning_message);
   png_warning(png_ptr, msg);
}

static void
png_default_error(png_structp png_ptr, png_const_charp error_message)
{

   longjmp(png_ptr->jmpbuf, 1);
     ;
   if (&error_message != NULL)
      return;
}

static void
png_default_warning(png_structp png_ptr, png_const_charp warning_message)
{
     ;
   if (warning_message)
     return;
     ;
   if (png_ptr)
      return;
}

/* png_set_error_fn. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00536dac
void PNGAPI
png_set_error_fn(png_structp png_ptr, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warning_fn)
{
   png_ptr->error_ptr = error_ptr;
   png_ptr->error_fn = error_fn;
   png_ptr->warning_fn = warning_fn;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_voidp PNGAPI
png_get_error_ptr(png_structp png_ptr)
{
   return ((png_voidp)png_ptr->error_ptr);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
void PNGAPI
png_set_strip_error_numbers(png_structp png_ptr, png_uint_32 strip_mode)
{
   if(png_ptr != NULL)
   {
     png_ptr->flags &=
       ((~(PNG_FLAG_STRIP_ERROR_NUMBERS|PNG_FLAG_STRIP_ERROR_TEXT))&strip_mode);
   }
}
