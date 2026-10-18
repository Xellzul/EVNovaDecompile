// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -g -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libpng 1.2.7's one line, compiler and headers, as every libpng TU (Research/runtime-compiler-ceiling.md)
// LINK: after pngtrans; data EVNOVA 0x005816a8
/* pngwrite.c -- libpng 1.2.7 pngwrite.c, Copyright (c) 1998-2004 Glenn Randers-Pehrson, (c) 1996-1997 Andreas Dilger, (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.; libpng licence in png.h; not linked: dead-stripped in the exe; kept for the TU's data; altered: comments and the #if branches this build does not take removed. Notes: Research/notes/png.md */

#define PNG_INTERNAL
#include "png.h"

void PNGAPI
png_write_info_before_PLTE(png_structp png_ptr, png_infop info_ptr)
{
   png_debug(1, "in png_write_info_before_PLTE\n");
   if (!(png_ptr->mode & PNG_WROTE_INFO_BEFORE_PLTE))
   {
   png_write_sig(png_ptr);
   if((png_ptr->mode&PNG_HAVE_PNG_SIGNATURE)&&(png_ptr->mng_features_permitted))
   {
      png_warning(png_ptr,"MNG features are not allowed in a PNG datastream\n");
      png_ptr->mng_features_permitted=0;
   }

   png_write_IHDR(png_ptr, info_ptr->width, info_ptr->height,
      info_ptr->bit_depth, info_ptr->color_type, info_ptr->compression_type,
      info_ptr->filter_type,
      info_ptr->interlace_type);

   if (info_ptr->valid & PNG_INFO_gAMA)
   {
      png_write_gAMA(png_ptr, info_ptr->gamma);
   }
   if (info_ptr->valid & PNG_INFO_sRGB)
      png_write_sRGB(png_ptr, (int)info_ptr->srgb_intent);
   if (info_ptr->valid & PNG_INFO_iCCP)
      png_write_iCCP(png_ptr, info_ptr->iccp_name, PNG_COMPRESSION_TYPE_BASE,
                     info_ptr->iccp_profile, (int)info_ptr->iccp_proflen);
   if (info_ptr->valid & PNG_INFO_sBIT)
      png_write_sBIT(png_ptr, &(info_ptr->sig_bit), info_ptr->color_type);
   if (info_ptr->valid & PNG_INFO_cHRM)
   {
      png_write_cHRM(png_ptr,
         info_ptr->x_white, info_ptr->y_white,
         info_ptr->x_red, info_ptr->y_red,
         info_ptr->x_green, info_ptr->y_green,
         info_ptr->x_blue, info_ptr->y_blue);
   }
   if (info_ptr->unknown_chunks_num)
   {
       png_unknown_chunk *up;

       png_debug(5, "writing extra chunks\n");

       for (up = info_ptr->unknown_chunks;
            up < info_ptr->unknown_chunks + info_ptr->unknown_chunks_num;
            up++)
       {
         int keep=png_handle_as_unknown(png_ptr, up->name);
         if (keep != PNG_HANDLE_CHUNK_NEVER &&
            up->location && !(up->location & PNG_HAVE_PLTE) &&
            !(up->location & PNG_HAVE_IDAT) &&
            ((up->name[3] & 0x20) || keep == PNG_HANDLE_CHUNK_ALWAYS ||
            (png_ptr->flags & PNG_FLAG_KEEP_UNSAFE_CHUNKS)))
         {
            png_write_chunk(png_ptr, up->name, up->data, up->size);
         }
       }
   }
      png_ptr->mode |= PNG_WROTE_INFO_BEFORE_PLTE;
   }
}

void PNGAPI
png_write_info(png_structp png_ptr, png_infop info_ptr)
{
   int i;

   png_debug(1, "in png_write_info\n");

   png_write_info_before_PLTE(png_ptr, info_ptr);

   if (info_ptr->valid & PNG_INFO_PLTE)
      png_write_PLTE(png_ptr, info_ptr->palette,
         (png_uint_32)info_ptr->num_palette);
   else if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      png_error(png_ptr, "Valid palette required for paletted images\n");

   if (info_ptr->valid & PNG_INFO_tRNS)
      {

         if ((png_ptr->transformations & PNG_INVERT_ALPHA) &&
            info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
         {
            int j;
            for (j=0; j<(int)info_ptr->num_trans; j++)
               info_ptr->trans[j] = (png_byte)(255 - info_ptr->trans[j]);
         }
      png_write_tRNS(png_ptr, info_ptr->trans, &(info_ptr->trans_values),
         info_ptr->num_trans, info_ptr->color_type);
      }
   if (info_ptr->valid & PNG_INFO_bKGD)
      png_write_bKGD(png_ptr, &(info_ptr->background), info_ptr->color_type);
   if (info_ptr->valid & PNG_INFO_hIST)
      png_write_hIST(png_ptr, info_ptr->hist, info_ptr->num_palette);
   if (info_ptr->valid & PNG_INFO_oFFs)
      png_write_oFFs(png_ptr, info_ptr->x_offset, info_ptr->y_offset,
         info_ptr->offset_unit_type);
   if (info_ptr->valid & PNG_INFO_pCAL)
      png_write_pCAL(png_ptr, info_ptr->pcal_purpose, info_ptr->pcal_X0,
         info_ptr->pcal_X1, info_ptr->pcal_type, info_ptr->pcal_nparams,
         info_ptr->pcal_units, info_ptr->pcal_params);
   if (info_ptr->valid & PNG_INFO_sCAL)
      png_write_sCAL_s(png_ptr, (int)info_ptr->scal_unit,
          info_ptr->scal_s_width, info_ptr->scal_s_height);
   if (info_ptr->valid & PNG_INFO_pHYs)
      png_write_pHYs(png_ptr, info_ptr->x_pixels_per_unit,
         info_ptr->y_pixels_per_unit, info_ptr->phys_unit_type);
   if (info_ptr->valid & PNG_INFO_tIME)
   {
      png_write_tIME(png_ptr, &(info_ptr->mod_time));
      png_ptr->mode |= PNG_WROTE_tIME;
   }
   if (info_ptr->valid & PNG_INFO_sPLT)
     for (i = 0; i < (int)info_ptr->splt_palettes_num; i++)
       png_write_sPLT(png_ptr, info_ptr->splt_palettes + i);

   for (i = 0; i < info_ptr->num_text; i++)
   {
      png_debug2(2, "Writing header text chunk %d, type %d\n", i,
         info_ptr->text[i].compression);

      if (info_ptr->text[i].compression > 0)
      {
          png_warning(png_ptr, "Unable to write international text\n");

          info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
      }

      else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_zTXt)
      {

         png_write_zTXt(png_ptr, info_ptr->text[i].key,
            info_ptr->text[i].text, 0,
            info_ptr->text[i].compression);

         info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_zTXt_WR;
      }
      else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_NONE)
      {

         png_write_tEXt(png_ptr, info_ptr->text[i].key,
                         info_ptr->text[i].text,
                         0);

         info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
      }
   }
   if (info_ptr->unknown_chunks_num)
   {
       png_unknown_chunk *up;

       png_debug(5, "writing extra chunks\n");

       for (up = info_ptr->unknown_chunks;
            up < info_ptr->unknown_chunks + info_ptr->unknown_chunks_num;
            up++)
       {
         int keep=png_handle_as_unknown(png_ptr, up->name);
         if (keep != PNG_HANDLE_CHUNK_NEVER &&
            up->location && (up->location & PNG_HAVE_PLTE) &&
            !(up->location & PNG_HAVE_IDAT) &&
            ((up->name[3] & 0x20) || keep == PNG_HANDLE_CHUNK_ALWAYS ||
            (png_ptr->flags & PNG_FLAG_KEEP_UNSAFE_CHUNKS)))
         {
            png_write_chunk(png_ptr, up->name, up->data, up->size);
         }
       }
   }
}

void PNGAPI
png_write_end(png_structp png_ptr, png_infop info_ptr)
{
   png_debug(1, "in png_write_end\n");
   if (!(png_ptr->mode & PNG_HAVE_IDAT))
      png_error(png_ptr, "No IDATs written into file");

   if (info_ptr != NULL)
   {
      int i;

      if ((info_ptr->valid & PNG_INFO_tIME) &&
         !(png_ptr->mode & PNG_WROTE_tIME))
         png_write_tIME(png_ptr, &(info_ptr->mod_time));

      for (i = 0; i < info_ptr->num_text; i++)
      {
         png_debug2(2, "Writing trailer text chunk %d, type %d\n", i,
            info_ptr->text[i].compression);

         if (info_ptr->text[i].compression > 0)
         {
             png_warning(png_ptr, "Unable to write international text\n");

             info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
         }
         else if (info_ptr->text[i].compression >= PNG_TEXT_COMPRESSION_zTXt)
         {

            png_write_zTXt(png_ptr, info_ptr->text[i].key,
               info_ptr->text[i].text, 0,
               info_ptr->text[i].compression);

            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_zTXt_WR;
         }
         else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_NONE)
         {

            png_write_tEXt(png_ptr, info_ptr->text[i].key,
               info_ptr->text[i].text, 0);

            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
         }
      }
   if (info_ptr->unknown_chunks_num)
   {
       png_unknown_chunk *up;

       png_debug(5, "writing extra chunks\n");

       for (up = info_ptr->unknown_chunks;
            up < info_ptr->unknown_chunks + info_ptr->unknown_chunks_num;
            up++)
       {
         int keep=png_handle_as_unknown(png_ptr, up->name);
         if (keep != PNG_HANDLE_CHUNK_NEVER &&
            up->location && (up->location & PNG_AFTER_IDAT) &&
            ((up->name[3] & 0x20) || keep == PNG_HANDLE_CHUNK_ALWAYS ||
            (png_ptr->flags & PNG_FLAG_KEEP_UNSAFE_CHUNKS)))
         {
            png_write_chunk(png_ptr, up->name, up->data, up->size);
         }
       }
   }
   }

   png_ptr->mode |= PNG_AFTER_IDAT;

   png_write_IEND(png_ptr);
}

void PNGAPI
png_convert_from_struct_tm(png_timep ptime, struct tm FAR * ttime)
{
   png_debug(1, "in png_convert_from_struct_tm\n");
   ptime->year = (png_uint_16)(1900 + ttime->tm_year);
   ptime->month = (png_byte)(ttime->tm_mon + 1);
   ptime->day = (png_byte)ttime->tm_mday;
   ptime->hour = (png_byte)ttime->tm_hour;
   ptime->minute = (png_byte)ttime->tm_min;
   ptime->second = (png_byte)ttime->tm_sec;
}

void PNGAPI
png_convert_from_time_t(png_timep ptime, time_t ttime)
{
   struct tm *tbuf;

   png_debug(1, "in png_convert_from_time_t\n");
   tbuf = gmtime(&ttime);
   png_convert_from_struct_tm(ptime, tbuf);
}

png_structp PNGAPI
png_create_write_struct(png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn)
{
   return (png_create_write_struct_2(user_png_ver, error_ptr, error_fn,
      warn_fn, png_voidp_NULL, png_malloc_ptr_NULL, png_free_ptr_NULL));
}

png_structp PNGAPI
png_create_write_struct_2(png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
   png_malloc_ptr malloc_fn, png_free_ptr free_fn)
{
   png_structp png_ptr;
   int i;
   png_debug(1, "in png_create_write_struct\n");
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
      png_destroy_struct(png_ptr);
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

   png_set_write_fn(png_ptr, png_voidp_NULL, png_rw_ptr_NULL,
      png_flush_ptr_NULL);

   png_set_filter_heuristics(png_ptr, PNG_FILTER_HEURISTIC_DEFAULT,
      1, png_doublep_NULL, png_doublep_NULL);

   if (setjmp(png_ptr->jmpbuf))
      PNG_ABORT();
   return (png_ptr);
}

#undef png_write_init
void PNGAPI
png_write_init(png_structp png_ptr)
{

   png_write_init_2(png_ptr, "1.0.6 or earlier", 0, 0);
}

void PNGAPI
png_write_init_2(png_structp png_ptr, png_const_charp user_png_ver,
   png_size_t png_struct_size, png_size_t png_info_size)
{

   if(png_sizeof(png_struct) > png_struct_size)
     {
       png_ptr->error_fn=NULL;
       png_ptr->flags=0;
       png_error(png_ptr,
       "The png struct allocated by the application for writing is too small.");
     }
   if(png_sizeof(png_info) > png_info_size)
     {
       png_ptr->error_fn=NULL;
       png_ptr->flags=0;
       png_error(png_ptr,
       "The info struct allocated by the application for writing is too small.");
     }
   png_write_init_3(&png_ptr, user_png_ver, png_struct_size);
}

void PNGAPI
png_write_init_3(png_structpp ptr_ptr, png_const_charp user_png_ver,
   png_size_t png_struct_size)
{
   png_structp png_ptr=*ptr_ptr;
   jmp_buf tmp_jmp;
   int i = 0;
   do
   {
     if (user_png_ver[i] != png_libpng_ver[i])
     {
       png_ptr->warning_fn=NULL;
       png_warning(png_ptr,
     "Application uses deprecated png_write_init() and should be recompiled.");
       break;
     }
   } while (png_libpng_ver[i++]);

   png_debug(1, "in png_write_init_3\n");

   png_memcpy(tmp_jmp, png_ptr->jmpbuf, png_sizeof (jmp_buf));

   if (png_sizeof(png_struct) > png_struct_size)
     {
       png_destroy_struct(png_ptr);
       png_ptr = (png_structp)png_create_struct(PNG_STRUCT_PNG);
       *ptr_ptr = png_ptr;
     }

   png_memset(png_ptr, 0, png_sizeof (png_struct));

   png_ptr->user_width_max=PNG_USER_WIDTH_MAX;
   png_ptr->user_height_max=PNG_USER_HEIGHT_MAX;

   png_init_mmx_flags(png_ptr);

   png_memcpy(png_ptr->jmpbuf, tmp_jmp, png_sizeof (jmp_buf));

   png_set_write_fn(png_ptr, png_voidp_NULL, png_rw_ptr_NULL,
      png_flush_ptr_NULL);

   png_ptr->zbuf_size = PNG_ZBUF_SIZE;
   png_ptr->zbuf = (png_bytep)png_malloc(png_ptr,
      (png_uint_32)png_ptr->zbuf_size);

   png_set_filter_heuristics(png_ptr, PNG_FILTER_HEURISTIC_DEFAULT,
      1, png_doublep_NULL, png_doublep_NULL);
}

void PNGAPI
png_write_rows(png_structp png_ptr, png_bytepp row,
   png_uint_32 num_rows)
{
   png_uint_32 i;
   png_bytepp rp;

   png_debug(1, "in png_write_rows\n");

   for (i = 0, rp = row; i < num_rows; i++, rp++)
   {
      png_write_row(png_ptr, *rp);
   }
}

void PNGAPI
png_write_image(png_structp png_ptr, png_bytepp image)
{
   png_uint_32 i;
   int pass, num_pass;
   png_bytepp rp;

   png_debug(1, "in png_write_image\n");

   num_pass = png_set_interlace_handling(png_ptr);

   for (pass = 0; pass < num_pass; pass++)
   {

      for (i = 0, rp = image; i < png_ptr->height; i++, rp++)
      {
         png_write_row(png_ptr, *rp);
      }
   }
}

void PNGAPI
png_write_row(png_structp png_ptr, png_bytep row)
{
   png_debug2(1, "in png_write_row (row %ld, pass %d)\n",
      png_ptr->row_number, png_ptr->pass);

   if (png_ptr->row_number == 0 && png_ptr->pass == 0)
   {

   if (!(png_ptr->mode & PNG_WROTE_INFO_BEFORE_PLTE))
      png_error(png_ptr,
         "png_write_info was never called before png_write_row.");

      png_write_start_row(png_ptr);
   }

   if (png_ptr->interlaced && (png_ptr->transformations & PNG_INTERLACE))
   {
      switch (png_ptr->pass)
      {
         case 0:
            if (png_ptr->row_number & 0x07)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;
         case 1:
            if ((png_ptr->row_number & 0x07) || png_ptr->width < 5)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;
         case 2:
            if ((png_ptr->row_number & 0x07) != 4)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;
         case 3:
            if ((png_ptr->row_number & 0x03) || png_ptr->width < 3)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;
         case 4:
            if ((png_ptr->row_number & 0x03) != 2)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;
         case 5:
            if ((png_ptr->row_number & 0x01) || png_ptr->width < 2)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;
         case 6:
            if (!(png_ptr->row_number & 0x01))
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;
      }
   }

   png_ptr->row_info.color_type = png_ptr->color_type;
   png_ptr->row_info.width = png_ptr->usr_width;
   png_ptr->row_info.channels = png_ptr->usr_channels;
   png_ptr->row_info.bit_depth = png_ptr->usr_bit_depth;
   png_ptr->row_info.pixel_depth = (png_byte)(png_ptr->row_info.bit_depth *
      png_ptr->row_info.channels);

   png_ptr->row_info.rowbytes = PNG_ROWBYTES(png_ptr->row_info.pixel_depth,
      png_ptr->row_info.width);

   png_debug1(3, "row_info->color_type = %d\n", png_ptr->row_info.color_type);
   png_debug1(3, "row_info->width = %lu\n", png_ptr->row_info.width);
   png_debug1(3, "row_info->channels = %d\n", png_ptr->row_info.channels);
   png_debug1(3, "row_info->bit_depth = %d\n", png_ptr->row_info.bit_depth);
   png_debug1(3, "row_info->pixel_depth = %d\n", png_ptr->row_info.pixel_depth);
   png_debug1(3, "row_info->rowbytes = %lu\n", png_ptr->row_info.rowbytes);

   png_memcpy_check(png_ptr, png_ptr->row_buf + 1, row,
      png_ptr->row_info.rowbytes);

   if (png_ptr->interlaced && png_ptr->pass < 6 &&
      (png_ptr->transformations & PNG_INTERLACE))
   {
      png_do_write_interlace(&(png_ptr->row_info),
         png_ptr->row_buf + 1, png_ptr->pass);

      if (!(png_ptr->row_info.width))
      {
         png_write_finish_row(png_ptr);
         return;
      }
   }

   if (png_ptr->transformations)
      png_do_write_transformations(png_ptr);

   if((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
      (png_ptr->filter_type == PNG_INTRAPIXEL_DIFFERENCING))
   {

      png_do_write_intrapixel(&(png_ptr->row_info), png_ptr->row_buf + 1);
   }

   png_write_find_filter(png_ptr, &(png_ptr->row_info));

   if (png_ptr->write_row_fn != NULL)
      (*(png_ptr->write_row_fn))(png_ptr, png_ptr->row_number, png_ptr->pass);
}

void PNGAPI
png_set_flush(png_structp png_ptr, int nrows)
{
   png_debug(1, "in png_set_flush\n");
   png_ptr->flush_dist = (nrows < 0 ? 0 : nrows);
}

void PNGAPI
png_write_flush(png_structp png_ptr)
{
   int wrote_IDAT;

   png_debug(1, "in png_write_flush\n");

   if (png_ptr->row_number >= png_ptr->num_rows)
     return;

   do
   {
      int ret;

      ret = deflate(&png_ptr->zstream, Z_SYNC_FLUSH);
      wrote_IDAT = 0;

      if (ret != Z_OK)
      {
         if (png_ptr->zstream.msg != NULL)
            png_error(png_ptr, png_ptr->zstream.msg);
         else
            png_error(png_ptr, "zlib error");
      }

      if (!(png_ptr->zstream.avail_out))
      {

         png_write_IDAT(png_ptr, png_ptr->zbuf,
                        png_ptr->zbuf_size);
         png_ptr->zstream.next_out = png_ptr->zbuf;
         png_ptr->zstream.avail_out = (uInt)png_ptr->zbuf_size;
         wrote_IDAT = 1;
      }
   } while(wrote_IDAT == 1);

   if (png_ptr->zbuf_size != png_ptr->zstream.avail_out)
   {

      png_write_IDAT(png_ptr, png_ptr->zbuf,
                     png_ptr->zbuf_size - png_ptr->zstream.avail_out);
      png_ptr->zstream.next_out = png_ptr->zbuf;
      png_ptr->zstream.avail_out = (uInt)png_ptr->zbuf_size;
   }
   png_ptr->flush_rows = 0;
   png_flush(png_ptr);
}

void PNGAPI
png_destroy_write_struct(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr)
{
   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL;
   png_free_ptr free_fn = NULL;
   png_voidp mem_ptr = NULL;

   png_debug(1, "in png_destroy_write_struct\n");
   if (png_ptr_ptr != NULL)
   {
      png_ptr = *png_ptr_ptr;
      free_fn = png_ptr->free_fn;
      mem_ptr = png_ptr->mem_ptr;
   }

   if (info_ptr_ptr != NULL)
      info_ptr = *info_ptr_ptr;

   if (info_ptr != NULL)
   {
      png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);

      if (png_ptr->num_chunk_list)
      {
         png_free(png_ptr, png_ptr->chunk_list);
         png_ptr->chunk_list=NULL;
         png_ptr->num_chunk_list=0;
      }

      png_destroy_struct_2((png_voidp)info_ptr, (png_free_ptr)free_fn,
         (png_voidp)mem_ptr);
      *info_ptr_ptr = NULL;
   }

   if (png_ptr != NULL)
   {
      png_write_destroy(png_ptr);
      png_destroy_struct_2((png_voidp)png_ptr, (png_free_ptr)free_fn,
         (png_voidp)mem_ptr);
      *png_ptr_ptr = NULL;
   }
}

void
png_write_destroy(png_structp png_ptr)
{
   jmp_buf tmp_jmp;
   png_error_ptr error_fn;
   png_error_ptr warning_fn;
   png_voidp error_ptr;
   png_free_ptr free_fn;

   png_debug(1, "in png_write_destroy\n");

   deflateEnd(&png_ptr->zstream);

   png_free(png_ptr, png_ptr->zbuf);
   png_free(png_ptr, png_ptr->row_buf);
   png_free(png_ptr, png_ptr->prev_row);
   png_free(png_ptr, png_ptr->sub_row);
   png_free(png_ptr, png_ptr->up_row);
   png_free(png_ptr, png_ptr->avg_row);
   png_free(png_ptr, png_ptr->paeth_row);

   png_free(png_ptr, png_ptr->prev_filters);
   png_free(png_ptr, png_ptr->filter_weights);
   png_free(png_ptr, png_ptr->inv_filter_weights);
   png_free(png_ptr, png_ptr->filter_costs);
   png_free(png_ptr, png_ptr->inv_filter_costs);

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

void PNGAPI
png_set_filter(png_structp png_ptr, int method, int filters)
{
   png_debug(1, "in png_set_filter\n");
   if((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
      (method == PNG_INTRAPIXEL_DIFFERENCING))
         method = PNG_FILTER_TYPE_BASE;
   if (method == PNG_FILTER_TYPE_BASE)
   {
      switch (filters & (PNG_ALL_FILTERS | 0x07))
      {
         case 5:
         case 6:
         case 7: png_warning(png_ptr, "Unknown row filter for method 0");
         case PNG_FILTER_VALUE_NONE:  png_ptr->do_filter=PNG_FILTER_NONE; break;
         case PNG_FILTER_VALUE_SUB:   png_ptr->do_filter=PNG_FILTER_SUB;  break;
         case PNG_FILTER_VALUE_UP:    png_ptr->do_filter=PNG_FILTER_UP;   break;
         case PNG_FILTER_VALUE_AVG:   png_ptr->do_filter=PNG_FILTER_AVG;  break;
         case PNG_FILTER_VALUE_PAETH: png_ptr->do_filter=PNG_FILTER_PAETH;break;
         default: png_ptr->do_filter = (png_byte)filters; break;
      }

      if (png_ptr->row_buf != NULL)
      {
         if ((png_ptr->do_filter & PNG_FILTER_SUB) && png_ptr->sub_row == NULL)
         {
            png_ptr->sub_row = (png_bytep)png_malloc(png_ptr,
              (png_ptr->rowbytes + 1));
            png_ptr->sub_row[0] = PNG_FILTER_VALUE_SUB;
         }

         if ((png_ptr->do_filter & PNG_FILTER_UP) && png_ptr->up_row == NULL)
         {
            if (png_ptr->prev_row == NULL)
            {
               png_warning(png_ptr, "Can't add Up filter after starting");
               png_ptr->do_filter &= ~PNG_FILTER_UP;
            }
            else
            {
               png_ptr->up_row = (png_bytep)png_malloc(png_ptr,
                  (png_ptr->rowbytes + 1));
               png_ptr->up_row[0] = PNG_FILTER_VALUE_UP;
            }
         }

         if ((png_ptr->do_filter & PNG_FILTER_AVG) && png_ptr->avg_row == NULL)
         {
            if (png_ptr->prev_row == NULL)
            {
               png_warning(png_ptr, "Can't add Average filter after starting");
               png_ptr->do_filter &= ~PNG_FILTER_AVG;
            }
            else
            {
               png_ptr->avg_row = (png_bytep)png_malloc(png_ptr,
                  (png_ptr->rowbytes + 1));
               png_ptr->avg_row[0] = PNG_FILTER_VALUE_AVG;
            }
         }

         if ((png_ptr->do_filter & PNG_FILTER_PAETH) &&
             png_ptr->paeth_row == NULL)
         {
            if (png_ptr->prev_row == NULL)
            {
               png_warning(png_ptr, "Can't add Paeth filter after starting");
               png_ptr->do_filter &= (png_byte)(~PNG_FILTER_PAETH);
            }
            else
            {
               png_ptr->paeth_row = (png_bytep)png_malloc(png_ptr,
                  (png_ptr->rowbytes + 1));
               png_ptr->paeth_row[0] = PNG_FILTER_VALUE_PAETH;
            }
         }

         if (png_ptr->do_filter == PNG_NO_FILTERS)
            png_ptr->do_filter = PNG_FILTER_NONE;
      }
   }
   else
      png_error(png_ptr, "Unknown custom filter method");
}

void PNGAPI
png_set_filter_heuristics(png_structp png_ptr, int heuristic_method,
   int num_weights, png_doublep filter_weights,
   png_doublep filter_costs)
{
   int i;

   png_debug(1, "in png_set_filter_heuristics\n");
   if (heuristic_method >= PNG_FILTER_HEURISTIC_LAST)
   {
      png_warning(png_ptr, "Unknown filter heuristic method");
      return;
   }

   if (heuristic_method == PNG_FILTER_HEURISTIC_DEFAULT)
   {
      heuristic_method = PNG_FILTER_HEURISTIC_UNWEIGHTED;
   }

   if (num_weights < 0 || filter_weights == NULL ||
      heuristic_method == PNG_FILTER_HEURISTIC_UNWEIGHTED)
   {
      num_weights = 0;
   }

   png_ptr->num_prev_filters = (png_byte)num_weights;
   png_ptr->heuristic_method = (png_byte)heuristic_method;

   if (num_weights > 0)
   {
      if (png_ptr->prev_filters == NULL)
      {
         png_ptr->prev_filters = (png_bytep)png_malloc(png_ptr,
            (png_uint_32)(png_sizeof(png_byte) * num_weights));

         for (i = 0; i < num_weights; i++)
         {
            png_ptr->prev_filters[i] = 255;
         }
      }

      if (png_ptr->filter_weights == NULL)
      {
         png_ptr->filter_weights = (png_uint_16p)png_malloc(png_ptr,
            (png_uint_32)(png_sizeof(png_uint_16) * num_weights));

         png_ptr->inv_filter_weights = (png_uint_16p)png_malloc(png_ptr,
            (png_uint_32)(png_sizeof(png_uint_16) * num_weights));
         for (i = 0; i < num_weights; i++)
         {
            png_ptr->inv_filter_weights[i] =
            png_ptr->filter_weights[i] = PNG_WEIGHT_FACTOR;
         }
      }

      for (i = 0; i < num_weights; i++)
      {
         if (filter_weights[i] < 0.0)
         {
            png_ptr->inv_filter_weights[i] =
            png_ptr->filter_weights[i] = PNG_WEIGHT_FACTOR;
         }
         else
         {
            png_ptr->inv_filter_weights[i] =
               (png_uint_16)((double)PNG_WEIGHT_FACTOR*filter_weights[i]+0.5);
            png_ptr->filter_weights[i] =
               (png_uint_16)((double)PNG_WEIGHT_FACTOR/filter_weights[i]+0.5);
         }
      }
   }

   if (png_ptr->filter_costs == NULL)
   {
      png_ptr->filter_costs = (png_uint_16p)png_malloc(png_ptr,
         (png_uint_32)(png_sizeof(png_uint_16) * PNG_FILTER_VALUE_LAST));

      png_ptr->inv_filter_costs = (png_uint_16p)png_malloc(png_ptr,
         (png_uint_32)(png_sizeof(png_uint_16) * PNG_FILTER_VALUE_LAST));

      for (i = 0; i < PNG_FILTER_VALUE_LAST; i++)
      {
         png_ptr->inv_filter_costs[i] =
         png_ptr->filter_costs[i] = PNG_COST_FACTOR;
      }
   }

   for (i = 0; i < PNG_FILTER_VALUE_LAST; i++)
   {
      if (filter_costs == NULL || filter_costs[i] < 0.0)
      {
         png_ptr->inv_filter_costs[i] =
         png_ptr->filter_costs[i] = PNG_COST_FACTOR;
      }
      else if (filter_costs[i] >= 1.0)
      {
         png_ptr->inv_filter_costs[i] =
            (png_uint_16)((double)PNG_COST_FACTOR / filter_costs[i] + 0.5);
         png_ptr->filter_costs[i] =
            (png_uint_16)((double)PNG_COST_FACTOR * filter_costs[i] + 0.5);
      }
   }
}

void PNGAPI
png_set_compression_level(png_structp png_ptr, int level)
{
   png_debug(1, "in png_set_compression_level\n");
   png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_LEVEL;
   png_ptr->zlib_level = level;
}

void PNGAPI
png_set_compression_mem_level(png_structp png_ptr, int mem_level)
{
   png_debug(1, "in png_set_compression_mem_level\n");
   png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_MEM_LEVEL;
   png_ptr->zlib_mem_level = mem_level;
}

void PNGAPI
png_set_compression_strategy(png_structp png_ptr, int strategy)
{
   png_debug(1, "in png_set_compression_strategy\n");
   png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_STRATEGY;
   png_ptr->zlib_strategy = strategy;
}

void PNGAPI
png_set_compression_window_bits(png_structp png_ptr, int window_bits)
{
   if (window_bits > 15)
      png_warning(png_ptr, "Only compression windows <= 32k supported by PNG");
   else if (window_bits < 8)
      png_warning(png_ptr, "Only compression windows >= 256 supported by PNG");

   if (window_bits == 8)
     {
       png_warning(png_ptr, "Compression window is being reset to 512");
       window_bits=9;
     }
   png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_WINDOW_BITS;
   png_ptr->zlib_window_bits = window_bits;
}

void PNGAPI
png_set_compression_method(png_structp png_ptr, int method)
{
   png_debug(1, "in png_set_compression_method\n");
   if (method != 8)
      png_warning(png_ptr, "Only compression method 8 is supported by PNG");
   png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_METHOD;
   png_ptr->zlib_method = method;
}

void PNGAPI
png_set_write_status_fn(png_structp png_ptr, png_write_status_ptr write_row_fn)
{
   png_ptr->write_row_fn = write_row_fn;
}

void PNGAPI
png_set_write_user_transform_fn(png_structp png_ptr, png_user_transform_ptr
   write_user_transform_fn)
{
   png_debug(1, "in png_set_write_user_transform_fn\n");
   png_ptr->transformations |= PNG_USER_TRANSFORM;
   png_ptr->write_user_transform_fn = write_user_transform_fn;
}

void PNGAPI
png_write_png(png_structp png_ptr, png_infop info_ptr,
              int transforms, voidp params)
{

   if (transforms & PNG_TRANSFORM_INVERT_ALPHA)
       png_set_invert_alpha(png_ptr);

   png_write_info(png_ptr, info_ptr);

   if (transforms & PNG_TRANSFORM_INVERT_MONO)
       png_set_invert_mono(png_ptr);

   if ((transforms & PNG_TRANSFORM_SHIFT)
               && (info_ptr->valid & PNG_INFO_sBIT))
       png_set_shift(png_ptr, &info_ptr->sig_bit);

   if (transforms & PNG_TRANSFORM_PACKING)
       png_set_packing(png_ptr);

   if (transforms & PNG_TRANSFORM_SWAP_ALPHA)
       png_set_swap_alpha(png_ptr);

   if (transforms & PNG_TRANSFORM_STRIP_FILLER)
       png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);

   if (transforms & PNG_TRANSFORM_BGR)
       png_set_bgr(png_ptr);

   if (transforms & PNG_TRANSFORM_SWAP_ENDIAN)
       png_set_swap(png_ptr);

   if (transforms & PNG_TRANSFORM_PACKSWAP)
       png_set_packswap(png_ptr);

   if (info_ptr->valid & PNG_INFO_IDAT)
       png_write_image(png_ptr, info_ptr->row_pointers);

   png_write_end(png_ptr, info_ptr);

   if(transforms == 0 || params == NULL)
        return;
}
