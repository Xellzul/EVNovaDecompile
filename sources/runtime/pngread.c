// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -g -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libpng 1.2.7's one line, compiler and headers (zlib's compiler, built with debug information on, against the 2.4-era MSL). -g moves jump threading and register choice; the 2.4-era math_x87.h makes fabs an asm inline; build 335 has no -proc generic, its default is generic x86; Research/runtime-compiler-ceiling.md
/* pngread.c -- libpng 1.2.7 pngread.c, Copyright (c) 1998-2004 Glenn Randers-Pehrson, (c) 1996-1997 Andreas Dilger, (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.; libpng licence in png.h; altered: comments and the #if branches this build does not take removed (the functions the exe does not link stay, unmarked). Notes: Research/notes/png.md */

#define PNG_INTERNAL
#include "png.h"

/* png_create_read_struct. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538852
png_structp PNGAPI
png_create_read_struct(png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn)
{

   return (png_create_read_struct_2(user_png_ver, error_ptr, error_fn,
      warn_fn, png_voidp_NULL, png_malloc_ptr_NULL, png_free_ptr_NULL));
}

/* png_create_read_struct_2. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005386a0
png_structp PNGAPI
png_create_read_struct_2(png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
   png_malloc_ptr malloc_fn, png_free_ptr free_fn)
{

   png_structp png_ptr;

   int i;

   png_debug(1, "in png_create_read_struct\n");
   png_ptr = (png_structp)png_create_struct_2(PNG_STRUCT_PNG,
      (png_malloc_ptr)malloc_fn, (png_voidp)mem_ptr);
   if (png_ptr == NULL)
      return (NULL);

   png_init_mmx_flags(png_ptr);

   png_ptr->user_width_max=PNG_USER_WIDTH_MAX;
   png_ptr->user_height_max=PNG_USER_HEIGHT_MAX;

   if (setjmp(png_ptr->jmpbuf))
   {
      png_free(png_ptr, png_ptr->zbuf);
      png_ptr->zbuf=NULL;
      png_destroy_struct_2((png_voidp)png_ptr,
         (png_free_ptr)free_fn, (png_voidp)mem_ptr);
      return (NULL);
   }

   png_set_mem_fn(png_ptr, mem_ptr, malloc_fn, free_fn);

   png_set_error_fn(png_ptr, error_ptr, error_fn, warn_fn);

   i=0;
   do
   {
     if(user_png_ver[i] != png_libpng_ver[i])
        png_ptr->flags |= PNG_FLAG_LIBRARY_MISMATCH;
   } while (png_libpng_ver[i++]);

   if (png_ptr->flags & PNG_FLAG_LIBRARY_MISMATCH)
   {

     if (user_png_ver == NULL || user_png_ver[0] != png_libpng_ver[0] ||
         (user_png_ver[0] == '1' && user_png_ver[2] != png_libpng_ver[2]) ||
         (user_png_ver[0] == '0' && user_png_ver[2] < '9'))
     {
        png_ptr->flags=0;
        png_error(png_ptr,
           "Incompatible libpng version in application and library");
     }
   }

   png_ptr->zbuf_size = PNG_ZBUF_SIZE;
   png_ptr->zbuf = (png_bytep)png_malloc(png_ptr,
     (png_uint_32)png_ptr->zbuf_size);
   png_ptr->zstream.zalloc = png_zalloc;
   png_ptr->zstream.zfree = png_zfree;
   png_ptr->zstream.opaque = (voidpf)png_ptr;

   switch (inflateInit(&png_ptr->zstream))
   {
     case Z_OK:   break;
     case Z_MEM_ERROR:
     case Z_STREAM_ERROR: png_error(png_ptr, "zlib memory error"); break;
     case Z_VERSION_ERROR: png_error(png_ptr, "zlib version error"); break;
     default: png_error(png_ptr, "Unknown zlib error");
   }

   png_ptr->zstream.next_out = png_ptr->zbuf;
   png_ptr->zstream.avail_out = (uInt)png_ptr->zbuf_size;

   png_set_read_fn(png_ptr, png_voidp_NULL, png_rw_ptr_NULL);

   if (setjmp(png_ptr->jmpbuf))
      PNG_ABORT();
   return (png_ptr);
}

#undef png_read_init

/* not linked: dead-stripped in the exe; kept for the TU's data */
void PNGAPI
png_read_init(png_structp png_ptr)
{

   png_read_init_2(png_ptr, "1.0.6 or earlier", 0, 0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
void PNGAPI
png_read_init_2(png_structp png_ptr, png_const_charp user_png_ver,
   png_size_t png_struct_size, png_size_t png_info_size)
{

   if(png_sizeof(png_struct) > png_struct_size)
     {
       png_ptr->error_fn=NULL;
       png_ptr->flags=0;
       png_error(png_ptr,
       "The png struct allocated by the application for reading is too small.");
     }
   if(png_sizeof(png_info) > png_info_size)
     {
       png_ptr->error_fn=NULL;
       png_ptr->flags=0;
       png_error(png_ptr,
         "The info struct allocated by application for reading is too small.");
     }
   png_read_init_3(&png_ptr, user_png_ver, png_struct_size);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
void PNGAPI
png_read_init_3(png_structpp ptr_ptr, png_const_charp user_png_ver,
   png_size_t png_struct_size)
{
   jmp_buf tmp_jmp;

   int i=0;

   png_structp png_ptr=*ptr_ptr;

   do
   {
     if(user_png_ver[i] != png_libpng_ver[i])
     {
       png_ptr->warning_fn=NULL;
       png_warning(png_ptr,
        "Application uses deprecated png_read_init() and should be recompiled.");
       break;
     }
   } while (png_libpng_ver[i++]);

   png_debug(1, "in png_read_init_3\n");


   png_memcpy(tmp_jmp, png_ptr->jmpbuf, png_sizeof (jmp_buf));

   if(png_sizeof(png_struct) > png_struct_size)
     {
       png_destroy_struct(png_ptr);
       *ptr_ptr = (png_structp)png_create_struct(PNG_STRUCT_PNG);
       png_ptr = *ptr_ptr;
     }

   png_memset(png_ptr, 0, png_sizeof (png_struct));


   png_memcpy(png_ptr->jmpbuf, tmp_jmp, png_sizeof (jmp_buf));

   png_ptr->user_width_max=PNG_USER_WIDTH_MAX;
   png_ptr->user_height_max=PNG_USER_HEIGHT_MAX;

   png_ptr->zbuf_size = PNG_ZBUF_SIZE;
   png_ptr->zbuf = (png_bytep)png_malloc(png_ptr,
     (png_uint_32)png_ptr->zbuf_size);
   png_ptr->zstream.zalloc = png_zalloc;
   png_ptr->zstream.zfree = png_zfree;
   png_ptr->zstream.opaque = (voidpf)png_ptr;

   switch (inflateInit(&png_ptr->zstream))
   {
     case Z_OK:   break;
     case Z_MEM_ERROR:
     case Z_STREAM_ERROR: png_error(png_ptr, "zlib memory"); break;
     case Z_VERSION_ERROR: png_error(png_ptr, "zlib version"); break;
     default: png_error(png_ptr, "Unknown zlib error");
   }

   png_ptr->zstream.next_out = png_ptr->zbuf;
   png_ptr->zstream.avail_out = (uInt)png_ptr->zbuf_size;

   png_set_read_fn(png_ptr, png_voidp_NULL, png_rw_ptr_NULL);
}

/* png_read_info. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005381d3
void PNGAPI
png_read_info(png_structp png_ptr, png_infop info_ptr)
{
   png_debug(1, "in png_read_info\n");

   if (png_ptr->sig_bytes < 8)
   {
      png_size_t num_checked = png_ptr->sig_bytes,
                 num_to_check = 8 - num_checked;

      png_read_data(png_ptr, &(info_ptr->signature[num_checked]), num_to_check);
      png_ptr->sig_bytes = 8;

      if (png_sig_cmp(info_ptr->signature, num_checked, num_to_check))
      {
         if (num_checked < 4 &&
             png_sig_cmp(info_ptr->signature, num_checked, num_to_check - 4))
            png_error(png_ptr, "Not a PNG file");
         else
            png_error(png_ptr, "PNG file corrupted by ASCII conversion");
      }
      if (num_checked < 3)
         png_ptr->mode |= PNG_HAVE_PNG_SIGNATURE;
   }

   for(;;)
   {
      png_byte chunk_length[4];
      png_uint_32 length;

      png_read_data(png_ptr, chunk_length, 4);
      length = png_get_uint_31(png_ptr,chunk_length);

      png_reset_crc(png_ptr);
      png_crc_read(png_ptr, png_ptr->chunk_name, 4);

      png_debug2(0, "Reading %s chunk, length=%lu.\n", png_ptr->chunk_name,
         length);

      if (!png_memcmp(png_ptr->chunk_name, png_IHDR, 4))
         png_handle_IHDR(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_IEND, 4))
         png_handle_IEND(png_ptr, info_ptr, length);
      else if (png_handle_as_unknown(png_ptr, png_ptr->chunk_name))
      {
         if (!png_memcmp(png_ptr->chunk_name, png_IDAT, 4))
            png_ptr->mode |= PNG_HAVE_IDAT;
         png_handle_unknown(png_ptr, info_ptr, length);
         if (!png_memcmp(png_ptr->chunk_name, png_PLTE, 4))
            png_ptr->mode |= PNG_HAVE_PLTE;
         else if (!png_memcmp(png_ptr->chunk_name, png_IDAT, 4))
         {
            if (!(png_ptr->mode & PNG_HAVE_IHDR))
               png_error(png_ptr, "Missing IHDR before IDAT");
            else if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
                     !(png_ptr->mode & PNG_HAVE_PLTE))
               png_error(png_ptr, "Missing PLTE before IDAT");
            break;
         }
      }
      else if (!png_memcmp(png_ptr->chunk_name, png_PLTE, 4))
         png_handle_PLTE(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_IDAT, 4))
      {
         if (!(png_ptr->mode & PNG_HAVE_IHDR))
            png_error(png_ptr, "Missing IHDR before IDAT");
         else if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
                  !(png_ptr->mode & PNG_HAVE_PLTE))
            png_error(png_ptr, "Missing PLTE before IDAT");

         png_ptr->idat_size = length;
         png_ptr->mode |= PNG_HAVE_IDAT;
         break;
      }
      else if (!png_memcmp(png_ptr->chunk_name, png_bKGD, 4))
         png_handle_bKGD(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_cHRM, 4))
         png_handle_cHRM(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_gAMA, 4))
         png_handle_gAMA(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_hIST, 4))
         png_handle_hIST(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_oFFs, 4))
         png_handle_oFFs(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_pCAL, 4))
         png_handle_pCAL(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_sCAL, 4))
         png_handle_sCAL(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_pHYs, 4))
         png_handle_pHYs(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_sBIT, 4))
         png_handle_sBIT(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_sRGB, 4))
         png_handle_sRGB(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_iCCP, 4))
         png_handle_iCCP(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_sPLT, 4))
         png_handle_sPLT(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_tEXt, 4))
         png_handle_tEXt(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_tIME, 4))
         png_handle_tIME(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_tRNS, 4))
         png_handle_tRNS(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_zTXt, 4))
         png_handle_zTXt(png_ptr, info_ptr, length);
      else
         png_handle_unknown(png_ptr, info_ptr, length);
   }
}

/* png_read_update_info. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005381a3
void PNGAPI
png_read_update_info(png_structp png_ptr, png_infop info_ptr)
{
   png_debug(1, "in png_read_update_info\n");
   if (!(png_ptr->flags & PNG_FLAG_ROW_INIT))
      png_read_start_row(png_ptr);
   else
      png_warning(png_ptr,
      "Ignoring extra png_read_update_info() call; row buffer not reallocated");
   png_read_transform_info(png_ptr, info_ptr);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
void PNGAPI
png_start_read_image(png_structp png_ptr)
{
   png_debug(1, "in png_start_read_image\n");
   if (!(png_ptr->flags & PNG_FLAG_ROW_INIT))
      png_read_start_row(png_ptr);
}

/* png_read_row. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00537d25
void PNGAPI
png_read_row(png_structp png_ptr, png_bytep row, png_bytep dsp_row)
{
   int ret;
   png_debug2(1, "in png_read_row (row %lu, pass %d)\n",
      png_ptr->row_number, png_ptr->pass);
   if (!(png_ptr->flags & PNG_FLAG_ROW_INIT))
      png_read_start_row(png_ptr);
   if (png_ptr->row_number == 0 && png_ptr->pass == 0)
   {

   }

   if (png_ptr->interlaced && (png_ptr->transformations & PNG_INTERLACE))
   {
      switch (png_ptr->pass)
      {
         case 0:
            if (png_ptr->row_number & 0x07)
            {
               if (dsp_row != NULL)
                  png_combine_row(png_ptr, dsp_row,
                     png_pass_dsp_mask[png_ptr->pass]);
               png_read_finish_row(png_ptr);
               return;
            }
            break;
         case 1:
            if ((png_ptr->row_number & 0x07) || png_ptr->width < 5)
            {
               if (dsp_row != NULL)
                  png_combine_row(png_ptr, dsp_row,
                     png_pass_dsp_mask[png_ptr->pass]);
               png_read_finish_row(png_ptr);
               return;
            }
            break;
         case 2:
            if ((png_ptr->row_number & 0x07) != 4)
            {
               if (dsp_row != NULL && (png_ptr->row_number & 4))
                  png_combine_row(png_ptr, dsp_row,
                     png_pass_dsp_mask[png_ptr->pass]);
               png_read_finish_row(png_ptr);
               return;
            }
            break;
         case 3:
            if ((png_ptr->row_number & 3) || png_ptr->width < 3)
            {
               if (dsp_row != NULL)
                  png_combine_row(png_ptr, dsp_row,
                     png_pass_dsp_mask[png_ptr->pass]);
               png_read_finish_row(png_ptr);
               return;
            }
            break;
         case 4:
            if ((png_ptr->row_number & 3) != 2)
            {
               if (dsp_row != NULL && (png_ptr->row_number & 2))
                  png_combine_row(png_ptr, dsp_row,
                     png_pass_dsp_mask[png_ptr->pass]);
               png_read_finish_row(png_ptr);
               return;
            }
            break;
         case 5:
            if ((png_ptr->row_number & 1) || png_ptr->width < 2)
            {
               if (dsp_row != NULL)
                  png_combine_row(png_ptr, dsp_row,
                     png_pass_dsp_mask[png_ptr->pass]);
               png_read_finish_row(png_ptr);
               return;
            }
            break;
         case 6:
            if (!(png_ptr->row_number & 1))
            {
               png_read_finish_row(png_ptr);
               return;
            }
            break;
      }
   }

   if (!(png_ptr->mode & PNG_HAVE_IDAT))
      png_error(png_ptr, "Invalid attempt to read row data");

   png_ptr->zstream.next_out = png_ptr->row_buf;
   png_ptr->zstream.avail_out = (uInt)png_ptr->irowbytes;
   do
   {
      if (!(png_ptr->zstream.avail_in))
      {
         while (!png_ptr->idat_size)
         {
            png_byte chunk_length[4];

            png_crc_finish(png_ptr, 0);

            png_read_data(png_ptr, chunk_length, 4);
            png_ptr->idat_size = png_get_uint_31(png_ptr,chunk_length);

            png_reset_crc(png_ptr);
            png_crc_read(png_ptr, png_ptr->chunk_name, 4);
            if (png_memcmp(png_ptr->chunk_name, png_IDAT, 4))
               png_error(png_ptr, "Not enough image data");
         }
         png_ptr->zstream.avail_in = (uInt)png_ptr->zbuf_size;
         png_ptr->zstream.next_in = png_ptr->zbuf;
         if (png_ptr->zbuf_size > png_ptr->idat_size)
            png_ptr->zstream.avail_in = (uInt)png_ptr->idat_size;
         png_crc_read(png_ptr, png_ptr->zbuf,
            (png_size_t)png_ptr->zstream.avail_in);
         png_ptr->idat_size -= png_ptr->zstream.avail_in;
      }
      ret = inflate(&png_ptr->zstream, Z_PARTIAL_FLUSH);
      if (ret == Z_STREAM_END)
      {
         if (png_ptr->zstream.avail_out || png_ptr->zstream.avail_in ||
            png_ptr->idat_size)
            png_error(png_ptr, "Extra compressed data");
         png_ptr->mode |= PNG_AFTER_IDAT;
         png_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;
         break;
      }
      if (ret != Z_OK)
         png_error(png_ptr, png_ptr->zstream.msg ? png_ptr->zstream.msg :
                   "Decompression error");

   } while (png_ptr->zstream.avail_out);

   png_ptr->row_info.color_type = png_ptr->color_type;
   png_ptr->row_info.width = png_ptr->iwidth;
   png_ptr->row_info.channels = png_ptr->channels;
   png_ptr->row_info.bit_depth = png_ptr->bit_depth;
   png_ptr->row_info.pixel_depth = png_ptr->pixel_depth;
   png_ptr->row_info.rowbytes = PNG_ROWBYTES(png_ptr->row_info.pixel_depth,
       png_ptr->row_info.width);

   if(png_ptr->row_buf[0])
   png_read_filter_row(png_ptr, &(png_ptr->row_info),
      png_ptr->row_buf + 1, png_ptr->prev_row + 1,
      (int)(png_ptr->row_buf[0]));

   png_memcpy_check(png_ptr, png_ptr->prev_row, png_ptr->row_buf,
      png_ptr->rowbytes + 1);

   if((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
      (png_ptr->filter_type == PNG_INTRAPIXEL_DIFFERENCING))
   {

      png_do_read_intrapixel(&(png_ptr->row_info), png_ptr->row_buf + 1);
   }

   if (png_ptr->transformations)
      png_do_read_transformations(png_ptr);

   if (png_ptr->interlaced &&
      (png_ptr->transformations & PNG_INTERLACE))
   {
      if (png_ptr->pass < 6)

         png_do_read_interlace(png_ptr);

      if (dsp_row != NULL)
         png_combine_row(png_ptr, dsp_row,
            png_pass_dsp_mask[png_ptr->pass]);
      if (row != NULL)
         png_combine_row(png_ptr, row,
            png_pass_mask[png_ptr->pass]);
   }
   else
   {
      if (row != NULL)
         png_combine_row(png_ptr, row, 0xff);
      if (dsp_row != NULL)
         png_combine_row(png_ptr, dsp_row, 0xff);
   }
   png_read_finish_row(png_ptr);

   if (png_ptr->read_row_fn != NULL)
      (*(png_ptr->read_row_fn))(png_ptr, png_ptr->row_number, png_ptr->pass);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
void PNGAPI
png_read_rows(png_structp png_ptr, png_bytepp row,
   png_bytepp display_row, png_uint_32 num_rows)
{
   png_uint_32 i;
   png_bytepp rp;
   png_bytepp dp;

   png_debug(1, "in png_read_rows\n");
   rp = row;
   dp = display_row;
   if (rp != NULL && dp != NULL)
      for (i = 0; i < num_rows; i++)
      {
         png_bytep rptr = *rp++;
         png_bytep dptr = *dp++;

         png_read_row(png_ptr, rptr, dptr);
      }
   else if(rp != NULL)
      for (i = 0; i < num_rows; i++)
      {
         png_bytep rptr = *rp;
         png_read_row(png_ptr, rptr, png_bytep_NULL);
         rp++;
      }
   else if(dp != NULL)
      for (i = 0; i < num_rows; i++)
      {
         png_bytep dptr = *dp;
         png_read_row(png_ptr, png_bytep_NULL, dptr);
         dp++;
      }
}

/* png_read_image. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00537ccf
void PNGAPI
png_read_image(png_structp png_ptr, png_bytepp image)
{
   png_uint_32 i,image_height;
   int pass, j;
   png_bytepp rp;

   png_debug(1, "in png_read_image\n");

   pass = png_set_interlace_handling(png_ptr);

   image_height=png_ptr->height;
   png_ptr->num_rows = image_height;

   for (j = 0; j < pass; j++)
   {
      rp = image;
      for (i = 0; i < image_height; i++)
      {
         png_read_row(png_ptr, *rp, png_bytep_NULL);
         rp++;
      }
   }
}

/* png_read_end. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005378c6
void PNGAPI
png_read_end(png_structp png_ptr, png_infop info_ptr)
{
   png_byte chunk_length[4];
   png_uint_32 length;

   png_debug(1, "in png_read_end\n");
   png_crc_finish(png_ptr, 0);

   do
   {

      png_read_data(png_ptr, chunk_length, 4);
      length = png_get_uint_31(png_ptr,chunk_length);

      png_reset_crc(png_ptr);
      png_crc_read(png_ptr, png_ptr->chunk_name, 4);

      png_debug1(0, "Reading %s chunk.\n", png_ptr->chunk_name);

      if (!png_memcmp(png_ptr->chunk_name, png_IHDR, 4))
         png_handle_IHDR(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_IEND, 4))
         png_handle_IEND(png_ptr, info_ptr, length);
      else if (png_handle_as_unknown(png_ptr, png_ptr->chunk_name))
      {
         if (!png_memcmp(png_ptr->chunk_name, png_IDAT, 4))
         {
            if (length > 0 || png_ptr->mode & PNG_AFTER_IDAT)
               png_error(png_ptr, "Too many IDAT's found");
         }
         else
            png_ptr->mode |= PNG_AFTER_IDAT;
         png_handle_unknown(png_ptr, info_ptr, length);
         if (!png_memcmp(png_ptr->chunk_name, png_PLTE, 4))
            png_ptr->mode |= PNG_HAVE_PLTE;
      }
      else if (!png_memcmp(png_ptr->chunk_name, png_IDAT, 4))
      {

         if (length > 0 || png_ptr->mode & PNG_AFTER_IDAT)
            png_error(png_ptr, "Too many IDAT's found");
         png_crc_finish(png_ptr, length);
      }
      else if (!png_memcmp(png_ptr->chunk_name, png_PLTE, 4))
         png_handle_PLTE(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_bKGD, 4))
         png_handle_bKGD(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_cHRM, 4))
         png_handle_cHRM(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_gAMA, 4))
         png_handle_gAMA(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_hIST, 4))
         png_handle_hIST(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_oFFs, 4))
         png_handle_oFFs(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_pCAL, 4))
         png_handle_pCAL(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_sCAL, 4))
         png_handle_sCAL(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_pHYs, 4))
         png_handle_pHYs(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_sBIT, 4))
         png_handle_sBIT(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_sRGB, 4))
         png_handle_sRGB(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_iCCP, 4))
         png_handle_iCCP(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_sPLT, 4))
         png_handle_sPLT(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_tEXt, 4))
         png_handle_tEXt(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_tIME, 4))
         png_handle_tIME(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_tRNS, 4))
         png_handle_tRNS(png_ptr, info_ptr, length);
      else if (!png_memcmp(png_ptr->chunk_name, png_zTXt, 4))
         png_handle_zTXt(png_ptr, info_ptr, length);
      else
         png_handle_unknown(png_ptr, info_ptr, length);
   } while (!(png_ptr->mode & PNG_HAVE_IEND));
}

/* png_destroy_read_struct. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005377ea
void PNGAPI
png_destroy_read_struct(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr,
   png_infopp end_info_ptr_ptr)
{
   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL, end_info_ptr = NULL;
   png_free_ptr free_fn;
   png_voidp mem_ptr;

   png_debug(1, "in png_destroy_read_struct\n");
   if (png_ptr_ptr != NULL)
      png_ptr = *png_ptr_ptr;

   if (info_ptr_ptr != NULL)
      info_ptr = *info_ptr_ptr;

   if (end_info_ptr_ptr != NULL)
      end_info_ptr = *end_info_ptr_ptr;

   free_fn = png_ptr->free_fn;
   mem_ptr = png_ptr->mem_ptr;

   png_read_destroy(png_ptr, info_ptr, end_info_ptr);

   if (info_ptr != NULL)
   {
      png_free_data(png_ptr, info_ptr, PNG_FREE_TEXT, -1);

      png_destroy_struct_2((png_voidp)info_ptr, (png_free_ptr)free_fn,
          (png_voidp)mem_ptr);
      *info_ptr_ptr = NULL;
   }

   if (end_info_ptr != NULL)
   {
      png_free_data(png_ptr, end_info_ptr, PNG_FREE_TEXT, -1);
      png_destroy_struct_2((png_voidp)end_info_ptr, (png_free_ptr)free_fn,
         (png_voidp)mem_ptr);
      *end_info_ptr_ptr = NULL;
   }

   if (png_ptr != NULL)
   {
      png_destroy_struct_2((png_voidp)png_ptr, (png_free_ptr)free_fn,
          (png_voidp)mem_ptr);
      *png_ptr_ptr = NULL;
   }
}

/* png_read_destroy. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0053759c
void
png_read_destroy(png_structp png_ptr, png_infop info_ptr, png_infop end_info_ptr)
{
   jmp_buf tmp_jmp;
   png_error_ptr error_fn;
   png_error_ptr warning_fn;
   png_voidp error_ptr;
   png_free_ptr free_fn;

   png_debug(1, "in png_read_destroy\n");
   if (info_ptr != NULL)
      png_info_destroy(png_ptr, info_ptr);

   if (end_info_ptr != NULL)
      png_info_destroy(png_ptr, end_info_ptr);

   png_free(png_ptr, png_ptr->zbuf);
   png_free(png_ptr, png_ptr->big_row_buf);
   png_free(png_ptr, png_ptr->prev_row);
   png_free(png_ptr, png_ptr->palette_lookup);
   png_free(png_ptr, png_ptr->dither_index);
   png_free(png_ptr, png_ptr->gamma_table);
   png_free(png_ptr, png_ptr->gamma_from_1);
   png_free(png_ptr, png_ptr->gamma_to_1);
   if (png_ptr->free_me & PNG_FREE_PLTE)
      png_zfree(png_ptr, png_ptr->palette);
   png_ptr->free_me &= ~PNG_FREE_PLTE;
   if (png_ptr->free_me & PNG_FREE_TRNS)
      png_free(png_ptr, png_ptr->trans);
   png_ptr->free_me &= ~PNG_FREE_TRNS;
   if (png_ptr->free_me & PNG_FREE_HIST)
      png_free(png_ptr, png_ptr->hist);
   png_ptr->free_me &= ~PNG_FREE_HIST;
   if (png_ptr->gamma_16_table != NULL)
   {
      int i;
      int istop = (1 << (8 - png_ptr->gamma_shift));
      for (i = 0; i < istop; i++)
      {
         png_free(png_ptr, png_ptr->gamma_16_table[i]);
      }
   png_free(png_ptr, png_ptr->gamma_16_table);
   }
   if (png_ptr->gamma_16_from_1 != NULL)
   {
      int i;
      int istop = (1 << (8 - png_ptr->gamma_shift));
      for (i = 0; i < istop; i++)
      {
         png_free(png_ptr, png_ptr->gamma_16_from_1[i]);
      }
   png_free(png_ptr, png_ptr->gamma_16_from_1);
   }
   if (png_ptr->gamma_16_to_1 != NULL)
   {
      int i;
      int istop = (1 << (8 - png_ptr->gamma_shift));
      for (i = 0; i < istop; i++)
      {
         png_free(png_ptr, png_ptr->gamma_16_to_1[i]);
      }
   png_free(png_ptr, png_ptr->gamma_16_to_1);
   }

   inflateEnd(&png_ptr->zstream);
   png_free(png_ptr, png_ptr->save_buffer);

   png_free(png_ptr, png_ptr->current_text);

   png_memcpy(tmp_jmp, png_ptr->jmpbuf, png_sizeof (jmp_buf));

   error_fn = png_ptr->error_fn;
   warning_fn = png_ptr->warning_fn;
   error_ptr = png_ptr->error_ptr;
   free_fn = png_ptr->free_fn;

   png_memset(png_ptr, 0, png_sizeof (png_struct));

   png_ptr->error_fn = error_fn;
   png_ptr->warning_fn = warning_fn;
   png_ptr->error_ptr = error_ptr;
   png_ptr->free_fn = free_fn;

   png_memcpy(png_ptr->jmpbuf, tmp_jmp, png_sizeof (jmp_buf));

}

/* not linked: dead-stripped in the exe; kept for the TU's data */
void PNGAPI
png_set_read_status_fn(png_structp png_ptr, png_read_status_ptr read_row_fn)
{
   png_ptr->read_row_fn = read_row_fn;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
void PNGAPI
png_read_png(png_structp png_ptr, png_infop info_ptr,
                           int transforms,
                           voidp params)
{
   int row;


   if (transforms & PNG_TRANSFORM_INVERT_ALPHA)
       png_set_invert_alpha(png_ptr);

   png_read_info(png_ptr, info_ptr);
   if (info_ptr->height > PNG_UINT_32_MAX/png_sizeof(png_bytep))
      png_error(png_ptr,"Image is too high to process with png_read_png()");


   if (transforms & PNG_TRANSFORM_STRIP_16)
       png_set_strip_16(png_ptr);


   if (transforms & PNG_TRANSFORM_STRIP_ALPHA)
       png_set_strip_alpha(png_ptr);



   if (transforms & PNG_TRANSFORM_PACKSWAP)
       png_set_packswap(png_ptr);


   if (transforms & PNG_TRANSFORM_EXPAND)
       if ((png_ptr->bit_depth < 8) ||
           (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE) ||
           (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)))
         png_set_expand(png_ptr);


   if (transforms & PNG_TRANSFORM_INVERT_MONO)
       png_set_invert_mono(png_ptr);


   if ((transforms & PNG_TRANSFORM_SHIFT)
       && png_get_valid(png_ptr, info_ptr, PNG_INFO_sBIT))
   {
      png_color_8p sig_bit;

      png_get_sBIT(png_ptr, info_ptr, &sig_bit);
      png_set_shift(png_ptr, sig_bit);
   }


   if (transforms & PNG_TRANSFORM_BGR)
       png_set_bgr(png_ptr);


   if (transforms & PNG_TRANSFORM_SWAP_ALPHA)
       png_set_swap_alpha(png_ptr);


   if (transforms & PNG_TRANSFORM_SWAP_ENDIAN)
       png_set_swap(png_ptr);

   png_read_update_info(png_ptr, info_ptr);

   png_free_data(png_ptr, info_ptr, PNG_FREE_ROWS, 0);
   if(info_ptr->row_pointers == NULL)
   {
      info_ptr->row_pointers = (png_bytepp)png_malloc(png_ptr,
         info_ptr->height * png_sizeof(png_bytep));
      info_ptr->free_me |= PNG_FREE_ROWS;
      for (row = 0; row < (int)info_ptr->height; row++)
      {
         info_ptr->row_pointers[row] = (png_bytep)png_malloc(png_ptr,
            png_get_rowbytes(png_ptr, info_ptr));
      }
   }

   png_read_image(png_ptr, info_ptr->row_pointers);
   info_ptr->valid |= PNG_INFO_IDAT;

   png_read_end(png_ptr, info_ptr);

   if(transforms == 0 || params == NULL)
        return;

}
