// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -g -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libpng 1.2.7's one line, compiler and headers (zlib's compiler, built with debug information on, against the 2.4-era MSL). -g moves jump threading and register choice; the 2.4-era math_x87.h makes fabs an asm inline; build 335 has no -proc generic, its default is generic x86; Research/runtime-compiler-ceiling.md
/* png.c -- libpng 1.2.7 png.c, Copyright (c) 1998-2004 Glenn Randers-Pehrson, (c) 1996-1997 Andreas Dilger, (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.; libpng licence in png.h; altered: comments and the #if branches this build does not take removed (the functions the exe does not link stay, unmarked). Notes: Research/notes/png.md */

#define PNG_INTERNAL
#define PNG_NO_EXTERN
#include "png.h"

typedef version_1_2_7 Your_png_h_is_not_version_1_2_7;

const char png_libpng_ver[18] = PNG_LIBPNG_VER_STRING;

const png_byte FARDATA png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};

PNG_IHDR;
PNG_IDAT;
PNG_IEND;
PNG_PLTE;
PNG_bKGD;
PNG_cHRM;
PNG_gAMA;
PNG_hIST;
PNG_iCCP;
PNG_iTXt;
PNG_oFFs;
PNG_pCAL;
PNG_sCAL;
PNG_pHYs;
PNG_sBIT;
PNG_sPLT;
PNG_sRGB;
PNG_tEXt;
PNG_tIME;
PNG_tRNS;
PNG_zTXt;

const int FARDATA png_pass_start[] = {0, 4, 0, 2, 0, 1, 0};

const int FARDATA png_pass_inc[] = {8, 8, 4, 4, 2, 2, 1};

const int FARDATA png_pass_ystart[] = {0, 0, 4, 0, 2, 0, 1};

const int FARDATA png_pass_yinc[] = {8, 8, 8, 4, 4, 2, 2};

const int FARDATA png_pass_mask[] = {0x80, 0x08, 0x88, 0x22, 0xaa, 0x55, 0xff};

const int FARDATA png_pass_dsp_mask[]
   = {0xff, 0x0f, 0xff, 0x33, 0xff, 0x55, 0xff};

/* not linked: dead-stripped in the exe; kept for the TU's data */
void PNGAPI
png_set_sig_bytes(png_structp png_ptr, int num_bytes)
{
   png_debug(1, "in png_set_sig_bytes\n");
   if (num_bytes > 8)
      png_error(png_ptr, "Too many bytes for PNG signature.");

   png_ptr->sig_bytes = (png_byte)(num_bytes < 0 ? 0 : num_bytes);
}

/* png_sig_cmp. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00537535
int PNGAPI
png_sig_cmp(png_bytep sig, png_size_t start, png_size_t num_to_check)
{
   png_byte png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
   if (num_to_check > 8)
      num_to_check = 8;
   else if (num_to_check < 1)
      return (0);

   if (start > 7)
      return (0);

   if (start + num_to_check > 8)
      num_to_check = 8 - start;

   return ((int)(png_memcmp(&sig[start], &png_signature[start], num_to_check)));
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
int PNGAPI
png_check_sig(png_bytep sig, int num)
{
  return ((int)!png_sig_cmp(sig, (png_size_t)0, (png_size_t)num));
}

/* png_zalloc. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005374e5
voidpf
png_zalloc(voidpf png_ptr, uInt items, uInt size)
{
   png_voidp ptr;
   png_structp p=png_ptr;
   png_uint_32 save_flags=p->flags;
   png_uint_32 num_bytes;

   if (items > PNG_UINT_32_MAX/size)
   {
     png_warning (png_ptr, "Potential overflow in png_zalloc()");
     return (NULL);
   }
   num_bytes = (png_uint_32)items * size;

   p->flags|=PNG_FLAG_MALLOC_NULL_MEM_OK;
   ptr = (png_voidp)png_malloc((png_structp)png_ptr, num_bytes);
   p->flags=save_flags;

   return ((voidpf)ptr);
}

/* png_zfree. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005374d5
void
png_zfree(voidpf png_ptr, voidpf ptr)
{
   png_free((png_structp)png_ptr, (png_voidp)ptr);
}

/* png_reset_crc. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005374ba
void
png_reset_crc(png_structp png_ptr)
{
   png_ptr->crc = crc32(0, Z_NULL, 0);
}

/* png_calculate_crc. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00537465
void
png_calculate_crc(png_structp png_ptr, png_bytep ptr, png_size_t length)
{
   int need_crc = 1;

   if (png_ptr->chunk_name[0] & 0x20)
   {
      if ((png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_MASK) ==
          (PNG_FLAG_CRC_ANCILLARY_USE | PNG_FLAG_CRC_ANCILLARY_NOWARN))
         need_crc = 0;
   }
   else
   {
      if (png_ptr->flags & PNG_FLAG_CRC_CRITICAL_IGNORE)
         need_crc = 0;
   }

   if (need_crc)
      png_ptr->crc = crc32(png_ptr->crc, ptr, (uInt)length);
}

/* png_create_info_struct. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00537428
png_infop PNGAPI
png_create_info_struct(png_structp png_ptr)
{
   png_infop info_ptr;

   png_debug(1, "in png_create_info_struct\n");
   if(png_ptr == NULL) return (NULL);
   info_ptr = (png_infop)png_create_struct_2(PNG_STRUCT_INFO,
      png_ptr->malloc_fn, png_ptr->mem_ptr);
   if (info_ptr != NULL)
      png_info_init_3(&info_ptr, png_sizeof(png_info));

   return (info_ptr);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
void PNGAPI
png_destroy_info_struct(png_structp png_ptr, png_infopp info_ptr_ptr)
{
   png_infop info_ptr = NULL;

   png_debug(1, "in png_destroy_info_struct\n");
   if (info_ptr_ptr != NULL)
      info_ptr = *info_ptr_ptr;

   if (info_ptr != NULL)
   {
      png_info_destroy(png_ptr, info_ptr);

      png_destroy_struct_2((png_voidp)info_ptr, png_ptr->free_fn,
          png_ptr->mem_ptr);
      *info_ptr_ptr = NULL;
   }
}

#undef png_info_init

/* not linked: dead-stripped in the exe; kept for the TU's data */
void PNGAPI
png_info_init(png_infop info_ptr)
{

   png_info_init_3(&info_ptr, 0);
}

void PNGAPI
png_info_init_3(png_infopp ptr_ptr, png_size_t png_info_struct_size)
{
   png_infop info_ptr = *ptr_ptr;

   png_debug(1, "in png_info_init_3\n");

   if(png_sizeof(png_info) > png_info_struct_size)
     {
       png_destroy_struct(info_ptr);
       info_ptr = (png_infop)png_create_struct(PNG_STRUCT_INFO);
       *ptr_ptr = info_ptr;
     }

   png_memset(info_ptr, 0, png_sizeof (png_info));
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
void PNGAPI
png_data_freer(png_structp png_ptr, png_infop info_ptr,
   int freer, png_uint_32 mask)
{
   png_debug(1, "in png_data_freer\n");
   if (png_ptr == NULL || info_ptr == NULL)
      return;
   if(freer == PNG_DESTROY_WILL_FREE_DATA)
      info_ptr->free_me |= mask;
   else if(freer == PNG_USER_WILL_FREE_DATA)
      info_ptr->free_me &= ~mask;
   else
      png_warning(png_ptr,
         "Unknown freer parameter in png_data_freer.");
}

/* png_free_data. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0053709a
void PNGAPI
png_free_data(png_structp png_ptr, png_infop info_ptr, png_uint_32 mask,
   int num)
{
   png_debug(1, "in png_free_data\n");
   if (png_ptr == NULL || info_ptr == NULL)
      return;

if ((mask & PNG_FREE_TEXT) & info_ptr->free_me)
{
   if (num != -1)
   {
     if (info_ptr->text && info_ptr->text[num].key)
     {
         png_free(png_ptr, info_ptr->text[num].key);
         info_ptr->text[num].key = NULL;
     }
   }
   else
   {
       int i;
       for (i = 0; i < info_ptr->num_text; i++)
           png_free_data(png_ptr, info_ptr, PNG_FREE_TEXT, i);
       png_free(png_ptr, info_ptr->text);
       info_ptr->text = NULL;
       info_ptr->num_text=0;
   }
}

if ((mask & PNG_FREE_TRNS) & info_ptr->free_me)
{
    png_free(png_ptr, info_ptr->trans);
    info_ptr->valid &= ~PNG_INFO_tRNS;
    info_ptr->trans = NULL;
}

if ((mask & PNG_FREE_SCAL) & info_ptr->free_me)
{
    info_ptr->valid &= ~PNG_INFO_sCAL;
}

if ((mask & PNG_FREE_PCAL) & info_ptr->free_me)
{
    png_free(png_ptr, info_ptr->pcal_purpose);
    png_free(png_ptr, info_ptr->pcal_units);
    info_ptr->pcal_purpose = NULL;
    info_ptr->pcal_units = NULL;
    if (info_ptr->pcal_params != NULL)
    {
        int i;
        for (i = 0; i < (int)info_ptr->pcal_nparams; i++)
        {
          png_free(png_ptr, info_ptr->pcal_params[i]);
          info_ptr->pcal_params[i]=NULL;
        }
        png_free(png_ptr, info_ptr->pcal_params);
        info_ptr->pcal_params = NULL;
    }
    info_ptr->valid &= ~PNG_INFO_pCAL;
}

if ((mask & PNG_FREE_ICCP) & info_ptr->free_me)
{
    png_free(png_ptr, info_ptr->iccp_name);
    png_free(png_ptr, info_ptr->iccp_profile);
    info_ptr->iccp_name = NULL;
    info_ptr->iccp_profile = NULL;
    info_ptr->valid &= ~PNG_INFO_iCCP;
}

if ((mask & PNG_FREE_SPLT) & info_ptr->free_me)
{
   if (num != -1)
   {
      if(info_ptr->splt_palettes)
      {
          png_free(png_ptr, info_ptr->splt_palettes[num].name);
          png_free(png_ptr, info_ptr->splt_palettes[num].entries);
          info_ptr->splt_palettes[num].name = NULL;
          info_ptr->splt_palettes[num].entries = NULL;
      }
   }
   else
   {
       if(info_ptr->splt_palettes_num)
       {
         int i;
         for (i = 0; i < (int)info_ptr->splt_palettes_num; i++)
            png_free_data(png_ptr, info_ptr, PNG_FREE_SPLT, i);

         png_free(png_ptr, info_ptr->splt_palettes);
         info_ptr->splt_palettes = NULL;
         info_ptr->splt_palettes_num = 0;
       }
       info_ptr->valid &= ~PNG_INFO_sPLT;
   }
}

if ((mask & PNG_FREE_UNKN) & info_ptr->free_me)
{
   if (num != -1)
   {
       if(info_ptr->unknown_chunks)
       {
          png_free(png_ptr, info_ptr->unknown_chunks[num].data);
          info_ptr->unknown_chunks[num].data = NULL;
       }
   }
   else
   {
       int i;

       if(info_ptr->unknown_chunks_num)
       {
         for (i = 0; i < (int)info_ptr->unknown_chunks_num; i++)
            png_free_data(png_ptr, info_ptr, PNG_FREE_UNKN, i);

         png_free(png_ptr, info_ptr->unknown_chunks);
         info_ptr->unknown_chunks = NULL;
         info_ptr->unknown_chunks_num = 0;
       }
   }
}

if ((mask & PNG_FREE_HIST)  & info_ptr->free_me)
{
    png_free(png_ptr, info_ptr->hist);
    info_ptr->hist = NULL;
    info_ptr->valid &= ~PNG_INFO_hIST;
}

if ((mask & PNG_FREE_PLTE) & info_ptr->free_me)
{
    png_zfree(png_ptr, info_ptr->palette);
    info_ptr->palette = NULL;
    info_ptr->valid &= ~PNG_INFO_PLTE;
    info_ptr->num_palette = 0;
}

if ((mask & PNG_FREE_ROWS) & info_ptr->free_me)
{
    if(info_ptr->row_pointers)
    {
       int row;
       for (row = 0; row < (int)info_ptr->height; row++)
       {
          png_free(png_ptr, info_ptr->row_pointers[row]);
          info_ptr->row_pointers[row]=NULL;
       }
       png_free(png_ptr, info_ptr->row_pointers);
       info_ptr->row_pointers=NULL;
    }
    info_ptr->valid &= ~PNG_INFO_IDAT;
}

   if(num == -1)
     info_ptr->free_me &= ~mask;
   else
     info_ptr->free_me &= ~(mask & ~PNG_FREE_MUL);
}

/* png_info_destroy. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00537043
void
png_info_destroy(png_structp png_ptr, png_infop info_ptr)
{
   png_debug(1, "in png_info_destroy\n");

   png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);

   if (png_ptr->num_chunk_list)
   {
       png_free(png_ptr, png_ptr->chunk_list);
       png_ptr->chunk_list=NULL;
       png_ptr->num_chunk_list=0;
   }

   png_info_init_3(&info_ptr, png_sizeof(png_info));
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_voidp PNGAPI
png_get_io_ptr(png_structp png_ptr)
{
   return (png_ptr->io_ptr);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_charp PNGAPI
png_get_copyright(png_structp png_ptr)
{
   if (&png_ptr != NULL)
   return ((png_charp) "\n libpng version 1.2.7 - September 12, 2004\n\
   Copyright (c) 1998-2004 Glenn Randers-Pehrson\n\
   Copyright (c) 1996-1997 Andreas Dilger\n\
   Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.\n");
   return ((png_charp) "");
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_charp PNGAPI
png_get_libpng_ver(png_structp png_ptr)
{

   if (&png_ptr != NULL)
      return ((png_charp) PNG_LIBPNG_VER_STRING);
   return ((png_charp) "");
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_charp PNGAPI
png_get_header_ver(png_structp png_ptr)
{

   if (&png_ptr != NULL)
      return ((png_charp) PNG_LIBPNG_VER_STRING);
   return ((png_charp) "");
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_charp PNGAPI
png_get_header_version(png_structp png_ptr)
{

   if (&png_ptr != NULL)
      return ((png_charp) PNG_HEADER_VERSION_STRING);
   return ((png_charp) "");
}

/* png_handle_as_unknown. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00536fee
int PNGAPI
png_handle_as_unknown(png_structp png_ptr, png_bytep chunk_name)
{

   int i;
   png_bytep p;
   if((png_ptr == NULL && chunk_name == NULL) || png_ptr->num_chunk_list<=0)
      return 0;
   p=png_ptr->chunk_list+png_ptr->num_chunk_list*5-5;
   for (i = png_ptr->num_chunk_list; i; i--, p-=5)
      if (!png_memcmp(chunk_name, p, 4))
        return ((int)*(p+4));
   return 0;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
int PNGAPI
png_reset_zstream(png_structp png_ptr)
{
   return (inflateReset(&png_ptr->zstream));
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_access_version_number(void)
{

   return((png_uint_32) PNG_LIBPNG_VER);
}

/* png_init_mmx_flags. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00536fcf
void
png_init_mmx_flags (png_structp png_ptr)
{
    png_ptr->mmx_rowbytes_threshold = 0;
    png_ptr->mmx_bitdepth_threshold = 0;

    png_ptr->asm_flags &= ~( PNG_MMX_FLAGS );

}

/* not linked: dead-stripped in the exe; kept for the TU's data */
int PNGAPI
png_mmx_support(void)
{
    return -1;
}
