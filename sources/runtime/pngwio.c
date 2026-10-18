// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -g -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libpng 1.2.7's one line, compiler and headers, as every libpng TU (Research/runtime-compiler-ceiling.md)
// LINK: after pngtrans; data EVNOVA 0x00581a78
/* pngwio.c -- libpng 1.2.7 pngwio.c, Copyright (c) 1998-2004 Glenn Randers-Pehrson, (c) 1996-1997 Andreas Dilger, (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.; libpng licence in png.h; not linked: dead-stripped in the exe; kept for the TU's data; altered: comments and the #if branches this build does not take removed. Notes: Research/notes/png.md */

#define PNG_INTERNAL
#include "png.h"

void
png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   if (png_ptr->write_data_fn != NULL )
      (*(png_ptr->write_data_fn))(png_ptr, data, length);
   else
      png_error(png_ptr, "Call to NULL write function");
}

void
png_flush(png_structp png_ptr)
{
   if (png_ptr->output_flush_fn != NULL)
      (*(png_ptr->output_flush_fn))(png_ptr);
}

void PNGAPI
png_set_write_fn(png_structp png_ptr, png_voidp io_ptr,
   png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn)
{
   png_ptr->io_ptr = io_ptr;

   png_ptr->write_data_fn = write_data_fn;

   png_ptr->output_flush_fn = output_flush_fn;

   if (png_ptr->read_data_fn != NULL)
   {
      png_ptr->read_data_fn = NULL;
      png_warning(png_ptr,
         "Attempted to set both read_data_fn and write_data_fn in");
      png_warning(png_ptr,
         "the same structure.  Resetting read_data_fn to NULL.");
   }
}
