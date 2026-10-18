// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -g -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libpng 1.2.7's one line, compiler and headers (zlib's compiler, built with debug information on, against the 2.4-era MSL). -g moves jump threading and register choice; the 2.4-era math_x87.h makes fabs an asm inline; build 335 has no -proc generic, its default is generic x86; Research/runtime-compiler-ceiling.md
/* pngrio.c -- libpng 1.2.7 pngrio.c, Copyright (c) 1998-2004 Glenn Randers-Pehrson, (c) 1996-1997 Andreas Dilger, (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.; libpng licence in png.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/png.md */

#define PNG_INTERNAL
#include "png.h"

/* png_read_data. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005388b3
void
png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_debug1(4,"reading %d bytes\n", (int)length);
   if (png_ptr->read_data_fn != NULL)
      (*(png_ptr->read_data_fn))(png_ptr, data, length);
   else
      png_error(png_ptr, "Call to NULL read function");
}

/* png_set_read_fn. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538871
void PNGAPI
png_set_read_fn(png_structp png_ptr, png_voidp io_ptr,
   png_rw_ptr read_data_fn)
{
   png_ptr->io_ptr = io_ptr;

   png_ptr->read_data_fn = read_data_fn;

   if (png_ptr->write_data_fn != NULL)
   {
      png_ptr->write_data_fn = NULL;
      png_warning(png_ptr,
         "It's an error to set both read_data_fn and write_data_fn in the ");
      png_warning(png_ptr,
         "same structure.  Resetting write_data_fn to NULL.");
   }

   png_ptr->output_flush_fn = NULL;
}
