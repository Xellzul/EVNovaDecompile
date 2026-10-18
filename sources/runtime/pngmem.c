// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -g -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libpng 1.2.7's one line, compiler and headers (zlib's compiler, built with debug information on, against the 2.4-era MSL). -g moves jump threading and register choice; the 2.4-era math_x87.h makes fabs an asm inline; build 335 has no -proc generic, its default is generic x86; Research/runtime-compiler-ceiling.md
/* pngmem.c -- libpng 1.2.7 pngmem.c, Copyright (c) 1998-2004 Glenn Randers-Pehrson, (c) 1996-1997 Andreas Dilger, (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.; libpng licence in png.h; altered: comments and the #if branches this build does not take removed (the functions the exe does not link stay, unmarked). Notes: Research/notes/png.md */

#define PNG_INTERNAL
#include "png.h"

png_voidp
png_create_struct(int type)
{
   return (png_create_struct_2(type, png_malloc_ptr_NULL, png_voidp_NULL));
}

/* png_create_struct_2. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005400df
png_voidp
png_create_struct_2(int type, png_malloc_ptr malloc_fn, png_voidp mem_ptr)
{
   png_size_t size;
   png_voidp struct_ptr;

   if (type == PNG_STRUCT_INFO)
      size = png_sizeof(png_info);
   else if (type == PNG_STRUCT_PNG)
      size = png_sizeof(png_struct);
   else
      return (NULL);

   if(malloc_fn != NULL)
   {
      png_struct dummy_struct;
      png_structp png_ptr = &dummy_struct;
      png_ptr->mem_ptr=mem_ptr;
      struct_ptr = (*(malloc_fn))(png_ptr, size);
      if (struct_ptr != NULL)
         png_memset(struct_ptr, 0, size);
      return (struct_ptr);
   }

   struct_ptr = (png_voidp)malloc(size);
   if (struct_ptr != NULL)
      png_memset(struct_ptr, 0, size);

   return (struct_ptr);
}

void
png_destroy_struct(png_voidp struct_ptr)
{
   png_destroy_struct_2(struct_ptr, png_free_ptr_NULL, png_voidp_NULL);
}

/* png_destroy_struct_2. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00540094
void
png_destroy_struct_2(png_voidp struct_ptr, png_free_ptr free_fn,
    png_voidp mem_ptr)
{
   if (struct_ptr != NULL)
   {
      if(free_fn != NULL)
      {
         png_struct dummy_struct;
         png_structp png_ptr = &dummy_struct;
         png_ptr->mem_ptr=mem_ptr;
         (*(free_fn))(png_ptr, struct_ptr);
         return;
      }
      free(struct_ptr);
   }
}

/* png_malloc. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00540037
png_voidp PNGAPI
png_malloc(png_structp png_ptr, png_uint_32 size)
{
   png_voidp ret;

   if (png_ptr == NULL || size == 0)
      return (NULL);

   if(png_ptr->malloc_fn != NULL)
       ret = ((png_voidp)(*(png_ptr->malloc_fn))(png_ptr, (png_size_t)size));
   else
       ret = (png_malloc_default(png_ptr, size));
   if (ret == NULL && (png_ptr->flags&PNG_FLAG_MALLOC_NULL_MEM_OK) == 0)
       png_error(png_ptr, "Out of Memory!");
   return (ret);
}

png_voidp PNGAPI
png_malloc_default(png_structp png_ptr, png_uint_32 size)
{
   png_voidp ret;

   if (png_ptr == NULL || size == 0)
      return (NULL);

 if (size != (size_t)size)
   ret = NULL;
 else
   ret = malloc((size_t)size);

   return (ret);
}

/* png_free. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00540001
void PNGAPI
png_free(png_structp png_ptr, png_voidp ptr)
{
   if (png_ptr == NULL || ptr == NULL)
      return;

   if (png_ptr->free_fn != NULL)
   {
      (*(png_ptr->free_fn))(png_ptr, ptr);
      return;
   }
   else png_free_default(png_ptr, ptr);
}
void PNGAPI
png_free_default(png_structp png_ptr, png_voidp ptr)
{
   if (png_ptr == NULL || ptr == NULL)
      return;

   free(ptr);
}

/* png_malloc_warn. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0053ff95
png_voidp PNGAPI
png_malloc_warn(png_structp png_ptr, png_uint_32 size)
{
   png_voidp ptr;
   png_uint_32 save_flags=png_ptr->flags;

   png_ptr->flags|=PNG_FLAG_MALLOC_NULL_MEM_OK;
   ptr = (png_voidp)png_malloc((png_structp)png_ptr, size);
   png_ptr->flags=save_flags;
   return(ptr);
}

/* png_memcpy_check. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0053ff67
png_voidp PNGAPI
png_memcpy_check (png_structp png_ptr, png_voidp s1, png_voidp s2,
   png_uint_32 length)
{
   png_size_t size;

   size = (png_size_t)length;
   if ((png_uint_32)size != length)
      png_error(png_ptr,"Overflow in png_memcpy_check.");

   return(png_memcpy (s1, s2, size));
}

/* png_memset_check. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0053ff39
png_voidp PNGAPI
png_memset_check (png_structp png_ptr, png_voidp s1, int value,
   png_uint_32 length)
{
   png_size_t size;

   size = (png_size_t)length;
   if ((png_uint_32)size != length)
      png_error(png_ptr,"Overflow in png_memset_check.");

   return (png_memset (s1, value, size));

}

/* png_set_mem_fn. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0053ff16
void PNGAPI
png_set_mem_fn(png_structp png_ptr, png_voidp mem_ptr, png_malloc_ptr
  malloc_fn, png_free_ptr free_fn)
{
   png_ptr->mem_ptr = mem_ptr;
   png_ptr->malloc_fn = malloc_fn;
   png_ptr->free_fn = free_fn;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_voidp PNGAPI
png_get_mem_ptr(png_structp png_ptr)
{
   return ((png_voidp)png_ptr->mem_ptr);
}
