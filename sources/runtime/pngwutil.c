// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -g -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libpng 1.2.7's one line, compiler and headers, as every libpng TU (Research/runtime-compiler-ceiling.md)
// LINK: after pngrutil; data EVNOVA 0x00586e68
/* pngwutil.c -- libpng 1.2.7 pngwutil.c, Copyright (c) 1998-2004 Glenn Randers-Pehrson, (c) 1996-1997 Andreas Dilger, (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.; libpng licence in png.h; not linked: dead-stripped in the exe; kept for the TU's data; altered: comments and the #if branches this build does not take removed. Notes: Research/notes/png.md */

#define PNG_INTERNAL
#include "png.h"

void
png_save_uint_32(png_bytep buf, png_uint_32 i)
{
   buf[0] = (png_byte)((i >> 24) & 0xff);
   buf[1] = (png_byte)((i >> 16) & 0xff);
   buf[2] = (png_byte)((i >> 8) & 0xff);
   buf[3] = (png_byte)(i & 0xff);
}

void
png_save_int_32(png_bytep buf, png_int_32 i)
{
   buf[0] = (png_byte)((i >> 24) & 0xff);
   buf[1] = (png_byte)((i >> 16) & 0xff);
   buf[2] = (png_byte)((i >> 8) & 0xff);
   buf[3] = (png_byte)(i & 0xff);
}

void
png_save_uint_16(png_bytep buf, unsigned int i)
{
   buf[0] = (png_byte)((i >> 8) & 0xff);
   buf[1] = (png_byte)(i & 0xff);
}

void PNGAPI
png_write_chunk(png_structp png_ptr, png_bytep chunk_name,
   png_bytep data, png_size_t length)
{
   png_write_chunk_start(png_ptr, chunk_name, (png_uint_32)length);
   png_write_chunk_data(png_ptr, data, length);
   png_write_chunk_end(png_ptr);
}

void PNGAPI
png_write_chunk_start(png_structp png_ptr, png_bytep chunk_name,
   png_uint_32 length)
{
   png_byte buf[4];
   png_debug2(0, "Writing %s chunk (%lu bytes)\n", chunk_name, length);

   png_save_uint_32(buf, length);
   png_write_data(png_ptr, buf, (png_size_t)4);

   png_write_data(png_ptr, chunk_name, (png_size_t)4);

   png_reset_crc(png_ptr);
   png_calculate_crc(png_ptr, chunk_name, (png_size_t)4);
}

void PNGAPI
png_write_chunk_data(png_structp png_ptr, png_bytep data, png_size_t length)
{

   if (data != NULL && length > 0)
   {
      png_calculate_crc(png_ptr, data, length);
      png_write_data(png_ptr, data, length);
   }
}

void PNGAPI
png_write_chunk_end(png_structp png_ptr)
{
   png_byte buf[4];

   png_save_uint_32(buf, png_ptr->crc);

   png_write_data(png_ptr, buf, (png_size_t)4);
}

void
png_write_sig(png_structp png_ptr)
{
   png_byte png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};

   png_write_data(png_ptr, &png_signature[png_ptr->sig_bytes],
      (png_size_t)8 - png_ptr->sig_bytes);
   if(png_ptr->sig_bytes < 3)
      png_ptr->mode |= PNG_HAVE_PNG_SIGNATURE;
}

typedef struct
{
    char *input;
    int input_len;
    int num_output_ptr;
    int max_output_ptr;
    png_charpp output_ptr;
} compression_state;

static int
png_text_compress(png_structp png_ptr,
        png_charp text, png_size_t text_len, int compression,
        compression_state *comp)
{
   int ret;

   comp->num_output_ptr = comp->max_output_ptr = 0;
   comp->output_ptr = NULL;
   comp->input = NULL;

   if (compression == PNG_TEXT_COMPRESSION_NONE)
   {
       comp->input = text;
       comp->input_len = text_len;
       return((int)text_len);
   }

   if (compression >= PNG_TEXT_COMPRESSION_LAST)
   {
      png_warning(png_ptr, "Unknown compression type");
   }

   png_ptr->zstream.avail_in = (uInt)text_len;
   png_ptr->zstream.next_in = (Bytef *)text;
   png_ptr->zstream.avail_out = (uInt)png_ptr->zbuf_size;
   png_ptr->zstream.next_out = (Bytef *)png_ptr->zbuf;

   do
   {

      ret = deflate(&png_ptr->zstream, Z_NO_FLUSH);
      if (ret != Z_OK)
      {

         if (png_ptr->zstream.msg != NULL)
            png_error(png_ptr, png_ptr->zstream.msg);
         else
            png_error(png_ptr, "zlib error");
      }

      if (!png_ptr->zstream.avail_out && png_ptr->zstream.avail_in)
      {

         if (comp->num_output_ptr >= comp->max_output_ptr)
         {
            int old_max;

            old_max = comp->max_output_ptr;
            comp->max_output_ptr = comp->num_output_ptr + 4;
            if (comp->output_ptr != NULL)
            {
               png_charpp old_ptr;

               old_ptr = comp->output_ptr;
               comp->output_ptr = (png_charpp)png_malloc(png_ptr,
                  (png_uint_32)(comp->max_output_ptr *
                  png_sizeof (png_charpp)));
               png_memcpy(comp->output_ptr, old_ptr, old_max
                  * png_sizeof (png_charp));
               png_free(png_ptr, old_ptr);
            }
            else
               comp->output_ptr = (png_charpp)png_malloc(png_ptr,
                  (png_uint_32)(comp->max_output_ptr *
                  png_sizeof (png_charp)));
         }

         comp->output_ptr[comp->num_output_ptr] = (png_charp)png_malloc(png_ptr,
            (png_uint_32)png_ptr->zbuf_size);
         png_memcpy(comp->output_ptr[comp->num_output_ptr], png_ptr->zbuf,
            png_ptr->zbuf_size);
         comp->num_output_ptr++;

         png_ptr->zstream.avail_out = (uInt)png_ptr->zbuf_size;
         png_ptr->zstream.next_out = png_ptr->zbuf;
      }

   } while (png_ptr->zstream.avail_in);

   do
   {

      ret = deflate(&png_ptr->zstream, Z_FINISH);

      if (ret == Z_OK)
      {

         if (!(png_ptr->zstream.avail_out))
         {

            if (comp->num_output_ptr >= comp->max_output_ptr)
            {
               int old_max;

               old_max = comp->max_output_ptr;
               comp->max_output_ptr = comp->num_output_ptr + 4;
               if (comp->output_ptr != NULL)
               {
                  png_charpp old_ptr;

                  old_ptr = comp->output_ptr;

                  comp->output_ptr = (png_charpp)png_malloc(png_ptr,
                     (png_uint_32)(comp->max_output_ptr *
                     png_sizeof (png_charpp)));
                  png_memcpy(comp->output_ptr, old_ptr,
                     old_max * png_sizeof (png_charp));
                  png_free(png_ptr, old_ptr);
               }
               else
                  comp->output_ptr = (png_charpp)png_malloc(png_ptr,
                     (png_uint_32)(comp->max_output_ptr *
                     png_sizeof (png_charp)));
            }

            comp->output_ptr[comp->num_output_ptr] =
               (png_charp)png_malloc(png_ptr, (png_uint_32)png_ptr->zbuf_size);
            png_memcpy(comp->output_ptr[comp->num_output_ptr], png_ptr->zbuf,
               png_ptr->zbuf_size);
            comp->num_output_ptr++;

            png_ptr->zstream.avail_out = (uInt)png_ptr->zbuf_size;
            png_ptr->zstream.next_out = png_ptr->zbuf;
         }
      }
      else if (ret != Z_STREAM_END)
      {

         if (png_ptr->zstream.msg != NULL)
            png_error(png_ptr, png_ptr->zstream.msg);
         else
            png_error(png_ptr, "zlib error");
      }
   } while (ret != Z_STREAM_END);

   text_len = png_ptr->zbuf_size * comp->num_output_ptr;
   if (png_ptr->zstream.avail_out < png_ptr->zbuf_size)
      text_len += png_ptr->zbuf_size - (png_size_t)png_ptr->zstream.avail_out;

   return((int)text_len);
}

static void
png_write_compressed_data_out(png_structp png_ptr, compression_state *comp)
{
   int i;

   if (comp->input)
   {
       png_write_chunk_data(png_ptr, (png_bytep)comp->input,
                            (png_size_t)comp->input_len);
       return;
   }

   for (i = 0; i < comp->num_output_ptr; i++)
   {
      png_write_chunk_data(png_ptr,(png_bytep)comp->output_ptr[i],
         png_ptr->zbuf_size);
      png_free(png_ptr, comp->output_ptr[i]);
      comp->output_ptr[i]=NULL;
   }
   if (comp->max_output_ptr != 0)
      png_free(png_ptr, comp->output_ptr);
      comp->output_ptr=NULL;

   if (png_ptr->zstream.avail_out < (png_uint_32)png_ptr->zbuf_size)
      png_write_chunk_data(png_ptr, png_ptr->zbuf,
         png_ptr->zbuf_size - png_ptr->zstream.avail_out);

   deflateReset(&png_ptr->zstream);

}

void
png_write_IHDR(png_structp png_ptr, png_uint_32 width, png_uint_32 height,
   int bit_depth, int color_type, int compression_type, int filter_type,
   int interlace_type)
{
   png_byte buf[13];

   png_debug(1, "in png_write_IHDR\n");

   switch (color_type)
   {
      case PNG_COLOR_TYPE_GRAY:
         switch (bit_depth)
         {
            case 1:
            case 2:
            case 4:
            case 8:
            case 16: png_ptr->channels = 1; break;
            default: png_error(png_ptr,"Invalid bit depth for grayscale image");
         }
         break;
      case PNG_COLOR_TYPE_RGB:
         if (bit_depth != 8 && bit_depth != 16)
            png_error(png_ptr, "Invalid bit depth for RGB image");
         png_ptr->channels = 3;
         break;
      case PNG_COLOR_TYPE_PALETTE:
         switch (bit_depth)
         {
            case 1:
            case 2:
            case 4:
            case 8: png_ptr->channels = 1; break;
            default: png_error(png_ptr, "Invalid bit depth for paletted image");
         }
         break;
      case PNG_COLOR_TYPE_GRAY_ALPHA:
         if (bit_depth != 8 && bit_depth != 16)
            png_error(png_ptr, "Invalid bit depth for grayscale+alpha image");
         png_ptr->channels = 2;
         break;
      case PNG_COLOR_TYPE_RGB_ALPHA:
         if (bit_depth != 8 && bit_depth != 16)
            png_error(png_ptr, "Invalid bit depth for RGBA image");
         png_ptr->channels = 4;
         break;
      default:
         png_error(png_ptr, "Invalid image color type specified");
   }

   if (compression_type != PNG_COMPRESSION_TYPE_BASE)
   {
      png_warning(png_ptr, "Invalid compression type specified");
      compression_type = PNG_COMPRESSION_TYPE_BASE;
   }

   if (
      !((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
      ((png_ptr->mode&PNG_HAVE_PNG_SIGNATURE) == 0) &&
      (color_type == PNG_COLOR_TYPE_RGB ||
       color_type == PNG_COLOR_TYPE_RGB_ALPHA) &&
      (filter_type == PNG_INTRAPIXEL_DIFFERENCING)) &&
      filter_type != PNG_FILTER_TYPE_BASE)
   {
      png_warning(png_ptr, "Invalid filter type specified");
      filter_type = PNG_FILTER_TYPE_BASE;
   }

   if (interlace_type != PNG_INTERLACE_NONE &&
      interlace_type != PNG_INTERLACE_ADAM7)
   {
      png_warning(png_ptr, "Invalid interlace type specified");
      interlace_type = PNG_INTERLACE_ADAM7;
   }

   png_ptr->bit_depth = (png_byte)bit_depth;
   png_ptr->color_type = (png_byte)color_type;
   png_ptr->interlaced = (png_byte)interlace_type;
   png_ptr->filter_type = (png_byte)filter_type;
   png_ptr->compression_type = (png_byte)compression_type;
   png_ptr->width = width;
   png_ptr->height = height;

   png_ptr->pixel_depth = (png_byte)(bit_depth * png_ptr->channels);
   png_ptr->rowbytes = PNG_ROWBYTES(png_ptr->pixel_depth, width);

   png_ptr->usr_width = png_ptr->width;
   png_ptr->usr_bit_depth = png_ptr->bit_depth;
   png_ptr->usr_channels = png_ptr->channels;

   png_save_uint_32(buf, width);
   png_save_uint_32(buf + 4, height);
   buf[8] = (png_byte)bit_depth;
   buf[9] = (png_byte)color_type;
   buf[10] = (png_byte)compression_type;
   buf[11] = (png_byte)filter_type;
   buf[12] = (png_byte)interlace_type;

   png_write_chunk(png_ptr, (png_bytep)png_IHDR, buf, (png_size_t)13);

   png_ptr->zstream.zalloc = png_zalloc;
   png_ptr->zstream.zfree = png_zfree;
   png_ptr->zstream.opaque = (voidpf)png_ptr;
   if (!(png_ptr->do_filter))
   {
      if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE ||
         png_ptr->bit_depth < 8)
         png_ptr->do_filter = PNG_FILTER_NONE;
      else
         png_ptr->do_filter = PNG_ALL_FILTERS;
   }
   if (!(png_ptr->flags & PNG_FLAG_ZLIB_CUSTOM_STRATEGY))
   {
      if (png_ptr->do_filter != PNG_FILTER_NONE)
         png_ptr->zlib_strategy = Z_FILTERED;
      else
         png_ptr->zlib_strategy = Z_DEFAULT_STRATEGY;
   }
   if (!(png_ptr->flags & PNG_FLAG_ZLIB_CUSTOM_LEVEL))
      png_ptr->zlib_level = Z_DEFAULT_COMPRESSION;
   if (!(png_ptr->flags & PNG_FLAG_ZLIB_CUSTOM_MEM_LEVEL))
      png_ptr->zlib_mem_level = 8;
   if (!(png_ptr->flags & PNG_FLAG_ZLIB_CUSTOM_WINDOW_BITS))
      png_ptr->zlib_window_bits = 15;
   if (!(png_ptr->flags & PNG_FLAG_ZLIB_CUSTOM_METHOD))
      png_ptr->zlib_method = 8;
   deflateInit2(&png_ptr->zstream, png_ptr->zlib_level,
      png_ptr->zlib_method, png_ptr->zlib_window_bits,
      png_ptr->zlib_mem_level, png_ptr->zlib_strategy);
   png_ptr->zstream.next_out = png_ptr->zbuf;
   png_ptr->zstream.avail_out = (uInt)png_ptr->zbuf_size;

   png_ptr->mode = PNG_HAVE_IHDR;
}

void
png_write_PLTE(png_structp png_ptr, png_colorp palette, png_uint_32 num_pal)
{
   png_uint_32 i;
   png_colorp pal_ptr;
   png_byte buf[3];

   png_debug(1, "in png_write_PLTE\n");
   if ((
        !(png_ptr->mng_features_permitted & PNG_FLAG_MNG_EMPTY_PLTE) &&
        num_pal == 0) || num_pal > 256)
   {
     if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
     {
        png_error(png_ptr, "Invalid number of colors in palette");
     }
     else
     {
        png_warning(png_ptr, "Invalid number of colors in palette");
        return;
     }
   }

   if (!(png_ptr->color_type&PNG_COLOR_MASK_COLOR))
   {
      png_warning(png_ptr,
        "Ignoring request to write a PLTE chunk in grayscale PNG");
      return;
   }

   png_ptr->num_palette = (png_uint_16)num_pal;
   png_debug1(3, "num_palette = %d\n", png_ptr->num_palette);

   png_write_chunk_start(png_ptr, (png_bytep)png_PLTE, num_pal * 3);
   for (i = 0, pal_ptr = palette; i < num_pal; i++, pal_ptr++)
   {
      buf[0] = pal_ptr->red;
      buf[1] = pal_ptr->green;
      buf[2] = pal_ptr->blue;
      png_write_chunk_data(png_ptr, buf, (png_size_t)3);
   }
   png_write_chunk_end(png_ptr);
   png_ptr->mode |= PNG_HAVE_PLTE;
}

void
png_write_IDAT(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_debug(1, "in png_write_IDAT\n");

   if (!(png_ptr->mode & PNG_HAVE_IDAT) &&
       png_ptr->compression_type == PNG_COMPRESSION_TYPE_BASE)
   {
      unsigned int z_cmf = data[0];
      if ((z_cmf & 0x0f) == 8 && (z_cmf & 0xf0) <= 0x70)
      {

         if (length >= 2 &&
             png_ptr->height < 16384 && png_ptr->width < 16384)
         {
            png_uint_32 uncompressed_idat_size = png_ptr->height *
               ((png_ptr->width *
               png_ptr->channels * png_ptr->bit_depth + 15) >> 3);
            unsigned int z_cinfo = z_cmf >> 4;
            unsigned int half_z_window_size = 1 << (z_cinfo + 7);
            while (uncompressed_idat_size <= half_z_window_size &&
                   half_z_window_size >= 256)
            {
               z_cinfo--;
               half_z_window_size >>= 1;
            }
            z_cmf = (z_cmf & 0x0f) | (z_cinfo << 4);
            if (data[0] != (png_byte)z_cmf)
            {
               data[0] = (png_byte)z_cmf;
               data[1] &= 0xe0;
               data[1] += (png_byte)(0x1f - ((z_cmf << 8) + data[1]) % 0x1f);
            }
         }
      }
      else
         png_error(png_ptr,
            "Invalid zlib compression method or flags in IDAT");
   }

   png_write_chunk(png_ptr, (png_bytep)png_IDAT, data, length);
   png_ptr->mode |= PNG_HAVE_IDAT;
}

void
png_write_IEND(png_structp png_ptr)
{
   png_debug(1, "in png_write_IEND\n");
   png_write_chunk(png_ptr, (png_bytep)png_IEND, png_bytep_NULL,
     (png_size_t)0);
   png_ptr->mode |= PNG_HAVE_IEND;
}

void
png_write_gAMA(png_structp png_ptr, double file_gamma)
{
   png_uint_32 igamma;
   png_byte buf[4];

   png_debug(1, "in png_write_gAMA\n");

   igamma = (png_uint_32)(file_gamma * 100000.0 + 0.5);
   png_save_uint_32(buf, igamma);
   png_write_chunk(png_ptr, (png_bytep)png_gAMA, buf, (png_size_t)4);
}
void
png_write_gAMA_fixed(png_structp png_ptr, png_fixed_point file_gamma)
{
   png_byte buf[4];

   png_debug(1, "in png_write_gAMA\n");

   png_save_uint_32(buf, (png_uint_32)file_gamma);
   png_write_chunk(png_ptr, (png_bytep)png_gAMA, buf, (png_size_t)4);
}

void
png_write_sRGB(png_structp png_ptr, int srgb_intent)
{
   png_byte buf[1];

   png_debug(1, "in png_write_sRGB\n");
   if(srgb_intent >= PNG_sRGB_INTENT_LAST)
         png_warning(png_ptr,
            "Invalid sRGB rendering intent specified");
   buf[0]=(png_byte)srgb_intent;
   png_write_chunk(png_ptr, (png_bytep)png_sRGB, buf, (png_size_t)1);
}

void
png_write_iCCP(png_structp png_ptr, png_charp name, int compression_type,
   png_charp profile, int profile_len)
{
   png_size_t name_len;
   png_charp new_name;
   compression_state comp;

   png_debug(1, "in png_write_iCCP\n");
   if (name == NULL || (name_len = png_check_keyword(png_ptr, name,
      &new_name)) == 0)
   {
      png_warning(png_ptr, "Empty keyword in iCCP chunk");
      return;
   }

   if (compression_type != PNG_COMPRESSION_TYPE_BASE)
      png_warning(png_ptr, "Unknown compression type in iCCP chunk");

   if (profile == NULL)
      profile_len = 0;

   if (profile_len)
       profile_len = png_text_compress(png_ptr, profile, (png_size_t)profile_len,
          PNG_COMPRESSION_TYPE_BASE, &comp);

   png_write_chunk_start(png_ptr, (png_bytep)png_iCCP,
          (png_uint_32)name_len+profile_len+2);
   new_name[name_len+1]=0x00;
   png_write_chunk_data(png_ptr, (png_bytep)new_name, name_len + 2);

   if (profile_len)
      png_write_compressed_data_out(png_ptr, &comp);

   png_write_chunk_end(png_ptr);
   png_free(png_ptr, new_name);
}

void
png_write_sPLT(png_structp png_ptr, png_sPLT_tp spalette)
{
   png_size_t name_len;
   png_charp new_name;
   png_byte entrybuf[10];
   int entry_size = (spalette->depth == 8 ? 6 : 10);
   int palette_size = entry_size * spalette->nentries;
   png_sPLT_entryp ep;

   png_debug(1, "in png_write_sPLT\n");
   if (spalette->name == NULL || (name_len = png_check_keyword(png_ptr,
      spalette->name, &new_name))==0)
   {
      png_warning(png_ptr, "Empty keyword in sPLT chunk");
      return;
   }

   png_write_chunk_start(png_ptr, (png_bytep)png_sPLT,
          (png_uint_32)(name_len + 2 + palette_size));
   png_write_chunk_data(png_ptr, (png_bytep)new_name, name_len + 1);
   png_write_chunk_data(png_ptr, (png_bytep)&spalette->depth, 1);

   for (ep = spalette->entries; ep<spalette->entries+spalette->nentries; ep++)
   {
       if (spalette->depth == 8)
       {
           entrybuf[0] = (png_byte)ep->red;
           entrybuf[1] = (png_byte)ep->green;
           entrybuf[2] = (png_byte)ep->blue;
           entrybuf[3] = (png_byte)ep->alpha;
           png_save_uint_16(entrybuf + 4, ep->frequency);
       }
       else
       {
           png_save_uint_16(entrybuf + 0, ep->red);
           png_save_uint_16(entrybuf + 2, ep->green);
           png_save_uint_16(entrybuf + 4, ep->blue);
           png_save_uint_16(entrybuf + 6, ep->alpha);
           png_save_uint_16(entrybuf + 8, ep->frequency);
       }
       png_write_chunk_data(png_ptr, entrybuf, (png_size_t)entry_size);
   }

   png_write_chunk_end(png_ptr);
   png_free(png_ptr, new_name);
}

void
png_write_sBIT(png_structp png_ptr, png_color_8p sbit, int color_type)
{
   png_byte buf[4];
   png_size_t size;

   png_debug(1, "in png_write_sBIT\n");

   if (color_type & PNG_COLOR_MASK_COLOR)
   {
      png_byte maxbits;

      maxbits = (png_byte)(color_type==PNG_COLOR_TYPE_PALETTE ? 8 :
                png_ptr->usr_bit_depth);
      if (sbit->red == 0 || sbit->red > maxbits ||
          sbit->green == 0 || sbit->green > maxbits ||
          sbit->blue == 0 || sbit->blue > maxbits)
      {
         png_warning(png_ptr, "Invalid sBIT depth specified");
         return;
      }
      buf[0] = sbit->red;
      buf[1] = sbit->green;
      buf[2] = sbit->blue;
      size = 3;
   }
   else
   {
      if (sbit->gray == 0 || sbit->gray > png_ptr->usr_bit_depth)
      {
         png_warning(png_ptr, "Invalid sBIT depth specified");
         return;
      }
      buf[0] = sbit->gray;
      size = 1;
   }

   if (color_type & PNG_COLOR_MASK_ALPHA)
   {
      if (sbit->alpha == 0 || sbit->alpha > png_ptr->usr_bit_depth)
      {
         png_warning(png_ptr, "Invalid sBIT depth specified");
         return;
      }
      buf[size++] = sbit->alpha;
   }

   png_write_chunk(png_ptr, (png_bytep)png_sBIT, buf, size);
}

void
png_write_cHRM(png_structp png_ptr, double white_x, double white_y,
   double red_x, double red_y, double green_x, double green_y,
   double blue_x, double blue_y)
{
   png_byte buf[32];
   png_uint_32 itemp;

   png_debug(1, "in png_write_cHRM\n");

   if (white_x < 0 || white_x > 0.8 || white_y < 0 || white_y > 0.8 ||
       white_x + white_y > 1.0)
   {
      png_warning(png_ptr, "Invalid cHRM white point specified");
      return;
   }
   itemp = (png_uint_32)(white_x * 100000.0 + 0.5);
   png_save_uint_32(buf, itemp);
   itemp = (png_uint_32)(white_y * 100000.0 + 0.5);
   png_save_uint_32(buf + 4, itemp);

   if (red_x < 0 || red_x > 0.8 || red_y < 0 || red_y > 0.8 ||
       red_x + red_y > 1.0)
   {
      png_warning(png_ptr, "Invalid cHRM red point specified");
      return;
   }
   itemp = (png_uint_32)(red_x * 100000.0 + 0.5);
   png_save_uint_32(buf + 8, itemp);
   itemp = (png_uint_32)(red_y * 100000.0 + 0.5);
   png_save_uint_32(buf + 12, itemp);

   if (green_x < 0 || green_x > 0.8 || green_y < 0 || green_y > 0.8 ||
       green_x + green_y > 1.0)
   {
      png_warning(png_ptr, "Invalid cHRM green point specified");
      return;
   }
   itemp = (png_uint_32)(green_x * 100000.0 + 0.5);
   png_save_uint_32(buf + 16, itemp);
   itemp = (png_uint_32)(green_y * 100000.0 + 0.5);
   png_save_uint_32(buf + 20, itemp);

   if (blue_x < 0 || blue_x > 0.8 || blue_y < 0 || blue_y > 0.8 ||
       blue_x + blue_y > 1.0)
   {
      png_warning(png_ptr, "Invalid cHRM blue point specified");
      return;
   }
   itemp = (png_uint_32)(blue_x * 100000.0 + 0.5);
   png_save_uint_32(buf + 24, itemp);
   itemp = (png_uint_32)(blue_y * 100000.0 + 0.5);
   png_save_uint_32(buf + 28, itemp);

   png_write_chunk(png_ptr, (png_bytep)png_cHRM, buf, (png_size_t)32);
}
void
png_write_cHRM_fixed(png_structp png_ptr, png_fixed_point white_x,
   png_fixed_point white_y, png_fixed_point red_x, png_fixed_point red_y,
   png_fixed_point green_x, png_fixed_point green_y, png_fixed_point blue_x,
   png_fixed_point blue_y)
{
   png_byte buf[32];

   png_debug(1, "in png_write_cHRM\n");

   if (white_x > 80000L || white_y > 80000L || white_x + white_y > 100000L)
   {
      png_warning(png_ptr, "Invalid fixed cHRM white point specified");
      return;
   }
   png_save_uint_32(buf, (png_uint_32)white_x);
   png_save_uint_32(buf + 4, (png_uint_32)white_y);

   if (red_x > 80000L || red_y > 80000L || red_x + red_y > 100000L)
   {
      png_warning(png_ptr, "Invalid cHRM fixed red point specified");
      return;
   }
   png_save_uint_32(buf + 8, (png_uint_32)red_x);
   png_save_uint_32(buf + 12, (png_uint_32)red_y);

   if (green_x > 80000L || green_y > 80000L || green_x + green_y > 100000L)
   {
      png_warning(png_ptr, "Invalid fixed cHRM green point specified");
      return;
   }
   png_save_uint_32(buf + 16, (png_uint_32)green_x);
   png_save_uint_32(buf + 20, (png_uint_32)green_y);

   if (blue_x > 80000L || blue_y > 80000L || blue_x + blue_y > 100000L)
   {
      png_warning(png_ptr, "Invalid fixed cHRM blue point specified");
      return;
   }
   png_save_uint_32(buf + 24, (png_uint_32)blue_x);
   png_save_uint_32(buf + 28, (png_uint_32)blue_y);

   png_write_chunk(png_ptr, (png_bytep)png_cHRM, buf, (png_size_t)32);
}

void
png_write_tRNS(png_structp png_ptr, png_bytep trans, png_color_16p tran,
   int num_trans, int color_type)
{
   png_byte buf[6];

   png_debug(1, "in png_write_tRNS\n");
   if (color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (num_trans <= 0 || num_trans > (int)png_ptr->num_palette)
      {
         png_warning(png_ptr,"Invalid number of transparent colors specified");
         return;
      }

      png_write_chunk(png_ptr, (png_bytep)png_tRNS, trans, (png_size_t)num_trans);
   }
   else if (color_type == PNG_COLOR_TYPE_GRAY)
   {

      if(tran->gray >= (1 << png_ptr->bit_depth))
      {
         png_warning(png_ptr,
           "Ignoring attempt to write tRNS chunk out-of-range for bit_depth");
         return;
      }
      png_save_uint_16(buf, tran->gray);
      png_write_chunk(png_ptr, (png_bytep)png_tRNS, buf, (png_size_t)2);
   }
   else if (color_type == PNG_COLOR_TYPE_RGB)
   {

      png_save_uint_16(buf, tran->red);
      png_save_uint_16(buf + 2, tran->green);
      png_save_uint_16(buf + 4, tran->blue);
      if(png_ptr->bit_depth == 8 && (buf[0] | buf[2] | buf[4]))
         {
            png_warning(png_ptr,
              "Ignoring attempt to write 16-bit tRNS chunk when bit_depth is 8");
            return;
         }
      png_write_chunk(png_ptr, (png_bytep)png_tRNS, buf, (png_size_t)6);
   }
   else
   {
      png_warning(png_ptr, "Can't write tRNS with an alpha channel");
   }
}

void
png_write_bKGD(png_structp png_ptr, png_color_16p back, int color_type)
{
   png_byte buf[6];

   png_debug(1, "in png_write_bKGD\n");
   if (color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (
          (png_ptr->num_palette ||
          (!(png_ptr->mng_features_permitted & PNG_FLAG_MNG_EMPTY_PLTE))) &&
         back->index > png_ptr->num_palette)
      {
         png_warning(png_ptr, "Invalid background palette index");
         return;
      }
      buf[0] = back->index;
      png_write_chunk(png_ptr, (png_bytep)png_bKGD, buf, (png_size_t)1);
   }
   else if (color_type & PNG_COLOR_MASK_COLOR)
   {
      png_save_uint_16(buf, back->red);
      png_save_uint_16(buf + 2, back->green);
      png_save_uint_16(buf + 4, back->blue);
      if(png_ptr->bit_depth == 8 && (buf[0] | buf[2] | buf[4]))
         {
            png_warning(png_ptr,
              "Ignoring attempt to write 16-bit bKGD chunk when bit_depth is 8");
            return;
         }
      png_write_chunk(png_ptr, (png_bytep)png_bKGD, buf, (png_size_t)6);
   }
   else
   {
      if(back->gray >= (1 << png_ptr->bit_depth))
      {
         png_warning(png_ptr,
           "Ignoring attempt to write bKGD chunk out-of-range for bit_depth");
         return;
      }
      png_save_uint_16(buf, back->gray);
      png_write_chunk(png_ptr, (png_bytep)png_bKGD, buf, (png_size_t)2);
   }
}

void
png_write_hIST(png_structp png_ptr, png_uint_16p hist, int num_hist)
{
   int i;
   png_byte buf[3];

   png_debug(1, "in png_write_hIST\n");
   if (num_hist > (int)png_ptr->num_palette)
   {
      png_debug2(3, "num_hist = %d, num_palette = %d\n", num_hist,
         png_ptr->num_palette);
      png_warning(png_ptr, "Invalid number of histogram entries specified");
      return;
   }

   png_write_chunk_start(png_ptr, (png_bytep)png_hIST, (png_uint_32)(num_hist * 2));
   for (i = 0; i < num_hist; i++)
   {
      png_save_uint_16(buf, hist[i]);
      png_write_chunk_data(png_ptr, buf, (png_size_t)2);
   }
   png_write_chunk_end(png_ptr);
}

png_size_t
png_check_keyword(png_structp png_ptr, png_charp key, png_charpp new_key)
{
   png_size_t key_len;
   png_charp kp, dp;
   int kflag;
   int kwarn=0;

   png_debug(1, "in png_check_keyword\n");
   *new_key = NULL;

   if (key == NULL || (key_len = png_strlen(key)) == 0)
   {
      png_warning(png_ptr, "zero length keyword");
      return ((png_size_t)0);
   }

   png_debug1(2, "Keyword to be checked is '%s'\n", key);

   *new_key = (png_charp)png_malloc_warn(png_ptr, (png_uint_32)(key_len + 2));
   if (*new_key == NULL)
   {
      png_warning(png_ptr, "Out of memory while procesing keyword");
      return ((png_size_t)0);
   }

   for (kp = key, dp = *new_key; *kp != '\0'; kp++, dp++)
   {
      if (*kp < 0x20 || (*kp > 0x7E && (png_byte)*kp < 0xA1))
      {
         png_warning(png_ptr, "invalid character in keyword");
         *dp = ' ';
      }
      else
      {
         *dp = *kp;
      }
   }
   *dp = '\0';

   kp = *new_key + key_len - 1;
   if (*kp == ' ')
   {
      png_warning(png_ptr, "trailing spaces removed from keyword");

      while (*kp == ' ')
      {
        *(kp--) = '\0';
        key_len--;
      }
   }

   kp = *new_key;
   if (*kp == ' ')
   {
      png_warning(png_ptr, "leading spaces removed from keyword");

      while (*kp == ' ')
      {
        kp++;
        key_len--;
      }
   }

   png_debug1(2, "Checking for multiple internal spaces in '%s'\n", kp);

   for (kflag = 0, dp = *new_key; *kp != '\0'; kp++)
   {
      if (*kp == ' ' && kflag == 0)
      {
         *(dp++) = *kp;
         kflag = 1;
      }
      else if (*kp == ' ')
      {
         key_len--;
         kwarn=1;
      }
      else
      {
         *(dp++) = *kp;
         kflag = 0;
      }
   }
   *dp = '\0';
   if(kwarn)
      png_warning(png_ptr, "extra interior spaces removed from keyword");

   if (key_len == 0)
   {
      png_free(png_ptr, *new_key);
      *new_key=NULL;
      png_warning(png_ptr, "Zero length keyword");
   }

   if (key_len > 79)
   {
      png_warning(png_ptr, "keyword length must be 1 - 79 characters");
      new_key[79] = '\0';
      key_len = 79;
   }

   return (key_len);
}

void
png_write_tEXt(png_structp png_ptr, png_charp key, png_charp text,
   png_size_t text_len)
{
   png_size_t key_len;
   png_charp new_key;

   png_debug(1, "in png_write_tEXt\n");
   if (key == NULL || (key_len = png_check_keyword(png_ptr, key, &new_key))==0)
   {
      png_warning(png_ptr, "Empty keyword in tEXt chunk");
      return;
   }

   if (text == NULL || *text == '\0')
      text_len = 0;
   else
      text_len = png_strlen(text);

   png_write_chunk_start(png_ptr, (png_bytep)png_tEXt, (png_uint_32)key_len+text_len+1);

   png_write_chunk_data(png_ptr, (png_bytep)new_key, key_len + 1);
   if (text_len)
      png_write_chunk_data(png_ptr, (png_bytep)text, text_len);

   png_write_chunk_end(png_ptr);
   png_free(png_ptr, new_key);
}

void
png_write_zTXt(png_structp png_ptr, png_charp key, png_charp text,
   png_size_t text_len, int compression)
{
   png_size_t key_len;
   char buf[1];
   png_charp new_key;
   compression_state comp;

   png_debug(1, "in png_write_zTXt\n");

   if (key == NULL || (key_len = png_check_keyword(png_ptr, key, &new_key))==0)
   {
      png_warning(png_ptr, "Empty keyword in zTXt chunk");
      return;
   }

   if (text == NULL || *text == '\0' || compression==PNG_TEXT_COMPRESSION_NONE)
   {
      png_write_tEXt(png_ptr, new_key, text, (png_size_t)0);
      png_free(png_ptr, new_key);
      return;
   }

   text_len = png_strlen(text);

   png_free(png_ptr, new_key);

   text_len = png_text_compress(png_ptr, text, text_len, compression,
       &comp);

   png_write_chunk_start(png_ptr, (png_bytep)png_zTXt, (png_uint_32)
      (key_len+text_len+2));

   png_write_chunk_data(png_ptr, (png_bytep)key, key_len + 1);
   buf[0] = (png_byte)compression;

   png_write_chunk_data(png_ptr, (png_bytep)buf, (png_size_t)1);

   png_write_compressed_data_out(png_ptr, &comp);

   png_write_chunk_end(png_ptr);
}

void
png_write_oFFs(png_structp png_ptr, png_int_32 x_offset, png_int_32 y_offset,
   int unit_type)
{
   png_byte buf[9];

   png_debug(1, "in png_write_oFFs\n");
   if (unit_type >= PNG_OFFSET_LAST)
      png_warning(png_ptr, "Unrecognized unit type for oFFs chunk");

   png_save_int_32(buf, x_offset);
   png_save_int_32(buf + 4, y_offset);
   buf[8] = (png_byte)unit_type;

   png_write_chunk(png_ptr, (png_bytep)png_oFFs, buf, (png_size_t)9);
}

void
png_write_pCAL(png_structp png_ptr, png_charp purpose, png_int_32 X0,
   png_int_32 X1, int type, int nparams, png_charp units, png_charpp params)
{
   png_size_t purpose_len, units_len, total_len;
   png_uint_32p params_len;
   png_byte buf[10];
   png_charp new_purpose;
   int i;

   png_debug1(1, "in png_write_pCAL (%d parameters)\n", nparams);
   if (type >= PNG_EQUATION_LAST)
      png_warning(png_ptr, "Unrecognized equation type for pCAL chunk");

   purpose_len = png_check_keyword(png_ptr, purpose, &new_purpose) + 1;
   png_debug1(3, "pCAL purpose length = %d\n", (int)purpose_len);
   units_len = png_strlen(units) + (nparams == 0 ? 0 : 1);
   png_debug1(3, "pCAL units length = %d\n", (int)units_len);
   total_len = purpose_len + units_len + 10;

   params_len = (png_uint_32p)png_malloc(png_ptr, (png_uint_32)(nparams
      *png_sizeof(png_uint_32)));

   for (i = 0; i < nparams; i++)
   {
      params_len[i] = png_strlen(params[i]) + (i == nparams - 1 ? 0 : 1);
      png_debug2(3, "pCAL parameter %d length = %lu\n", i, params_len[i]);
      total_len += (png_size_t)params_len[i];
   }

   png_debug1(3, "pCAL total length = %d\n", (int)total_len);
   png_write_chunk_start(png_ptr, (png_bytep)png_pCAL, (png_uint_32)total_len);
   png_write_chunk_data(png_ptr, (png_bytep)new_purpose, purpose_len);
   png_save_int_32(buf, X0);
   png_save_int_32(buf + 4, X1);
   buf[8] = (png_byte)type;
   buf[9] = (png_byte)nparams;
   png_write_chunk_data(png_ptr, buf, (png_size_t)10);
   png_write_chunk_data(png_ptr, (png_bytep)units, (png_size_t)units_len);

   png_free(png_ptr, new_purpose);

   for (i = 0; i < nparams; i++)
   {
      png_write_chunk_data(png_ptr, (png_bytep)params[i],
         (png_size_t)params_len[i]);
   }

   png_free(png_ptr, params_len);
   png_write_chunk_end(png_ptr);
}

void
png_write_sCAL_s(png_structp png_ptr, int unit, png_charp width,
   png_charp height)
{
   png_size_t total_len;
   char wbuf[32], hbuf[32];
   png_byte bunit = unit;

   png_debug(1, "in png_write_sCAL_s\n");

   png_strcpy(wbuf,(const char *)width);
   png_strcpy(hbuf,(const char *)height);
   total_len = 1 + png_strlen(wbuf)+1 + png_strlen(hbuf);

   png_debug1(3, "sCAL total length = %d\n", total_len);
   png_write_chunk_start(png_ptr, (png_bytep)png_sCAL, (png_uint_32)total_len);
   png_write_chunk_data(png_ptr, (png_bytep)&bunit, 1);
   png_write_chunk_data(png_ptr, (png_bytep)wbuf, png_strlen(wbuf)+1);
   png_write_chunk_data(png_ptr, (png_bytep)hbuf, png_strlen(hbuf));

   png_write_chunk_end(png_ptr);
}

void
png_write_pHYs(png_structp png_ptr, png_uint_32 x_pixels_per_unit,
   png_uint_32 y_pixels_per_unit,
   int unit_type)
{
   png_byte buf[9];

   png_debug(1, "in png_write_pHYs\n");
   if (unit_type >= PNG_RESOLUTION_LAST)
      png_warning(png_ptr, "Unrecognized unit type for pHYs chunk");

   png_save_uint_32(buf, x_pixels_per_unit);
   png_save_uint_32(buf + 4, y_pixels_per_unit);
   buf[8] = (png_byte)unit_type;

   png_write_chunk(png_ptr, (png_bytep)png_pHYs, buf, (png_size_t)9);
}

void
png_write_tIME(png_structp png_ptr, png_timep mod_time)
{
   png_byte buf[7];

   png_debug(1, "in png_write_tIME\n");
   if (mod_time->month  > 12 || mod_time->month  < 1 ||
       mod_time->day    > 31 || mod_time->day    < 1 ||
       mod_time->hour   > 23 || mod_time->second > 60)
   {
      png_warning(png_ptr, "Invalid time specified for tIME chunk");
      return;
   }

   png_save_uint_16(buf, mod_time->year);
   buf[2] = mod_time->month;
   buf[3] = mod_time->day;
   buf[4] = mod_time->hour;
   buf[5] = mod_time->minute;
   buf[6] = mod_time->second;

   png_write_chunk(png_ptr, (png_bytep)png_tIME, buf, (png_size_t)7);
}

void
png_write_start_row(png_structp png_ptr)
{

   png_size_t buf_size;

   png_debug(1, "in png_write_start_row\n");
   buf_size = (png_size_t)(PNG_ROWBYTES(
      png_ptr->usr_channels*png_ptr->usr_bit_depth,png_ptr->width)+1);

   png_ptr->row_buf = (png_bytep)png_malloc(png_ptr, (png_uint_32)buf_size);
   png_ptr->row_buf[0] = PNG_FILTER_VALUE_NONE;

   if (png_ptr->do_filter & PNG_FILTER_SUB)
   {
      png_ptr->sub_row = (png_bytep)png_malloc(png_ptr,
         (png_ptr->rowbytes + 1));
      png_ptr->sub_row[0] = PNG_FILTER_VALUE_SUB;
   }

   if (png_ptr->do_filter & (PNG_FILTER_AVG | PNG_FILTER_UP | PNG_FILTER_PAETH))
   {

      png_ptr->prev_row = (png_bytep)png_malloc(png_ptr, (png_uint_32)buf_size);
      png_memset(png_ptr->prev_row, 0, buf_size);

      if (png_ptr->do_filter & PNG_FILTER_UP)
      {
         png_ptr->up_row = (png_bytep )png_malloc(png_ptr,
            (png_ptr->rowbytes + 1));
         png_ptr->up_row[0] = PNG_FILTER_VALUE_UP;
      }

      if (png_ptr->do_filter & PNG_FILTER_AVG)
      {
         png_ptr->avg_row = (png_bytep)png_malloc(png_ptr,
            (png_ptr->rowbytes + 1));
         png_ptr->avg_row[0] = PNG_FILTER_VALUE_AVG;
      }

      if (png_ptr->do_filter & PNG_FILTER_PAETH)
      {
         png_ptr->paeth_row = (png_bytep )png_malloc(png_ptr,
            (png_ptr->rowbytes + 1));
         png_ptr->paeth_row[0] = PNG_FILTER_VALUE_PAETH;
      }
   }

   if (png_ptr->interlaced)
   {
      if (!(png_ptr->transformations & PNG_INTERLACE))
      {
         png_ptr->num_rows = (png_ptr->height + png_pass_yinc[0] - 1 -
            png_pass_ystart[0]) / png_pass_yinc[0];
         png_ptr->usr_width = (png_ptr->width + png_pass_inc[0] - 1 -
            png_pass_start[0]) / png_pass_inc[0];
      }
      else
      {
         png_ptr->num_rows = png_ptr->height;
         png_ptr->usr_width = png_ptr->width;
      }
   }
   else
   {
      png_ptr->num_rows = png_ptr->height;
      png_ptr->usr_width = png_ptr->width;
   }
   png_ptr->zstream.avail_out = (uInt)png_ptr->zbuf_size;
   png_ptr->zstream.next_out = png_ptr->zbuf;
}

void
png_write_finish_row(png_structp png_ptr)
{

   int ret;

   png_debug(1, "in png_write_finish_row\n");

   png_ptr->row_number++;

   if (png_ptr->row_number < png_ptr->num_rows)
      return;

   if (png_ptr->interlaced)
   {
      png_ptr->row_number = 0;
      if (png_ptr->transformations & PNG_INTERLACE)
      {
         png_ptr->pass++;
      }
      else
      {

         do
         {
            png_ptr->pass++;
            if (png_ptr->pass >= 7)
               break;
            png_ptr->usr_width = (png_ptr->width +
               png_pass_inc[png_ptr->pass] - 1 -
               png_pass_start[png_ptr->pass]) /
               png_pass_inc[png_ptr->pass];
            png_ptr->num_rows = (png_ptr->height +
               png_pass_yinc[png_ptr->pass] - 1 -
               png_pass_ystart[png_ptr->pass]) /
               png_pass_yinc[png_ptr->pass];
            if (png_ptr->transformations & PNG_INTERLACE)
               break;
         } while (png_ptr->usr_width == 0 || png_ptr->num_rows == 0);

      }

      if (png_ptr->pass < 7)
      {
         if (png_ptr->prev_row != NULL)
            png_memset(png_ptr->prev_row, 0,
               (png_size_t)(PNG_ROWBYTES(png_ptr->usr_channels*
               png_ptr->usr_bit_depth,png_ptr->width))+1);
         return;
      }
   }

   do
   {

      ret = deflate(&png_ptr->zstream, Z_FINISH);

      if (ret == Z_OK)
      {

         if (!(png_ptr->zstream.avail_out))
         {
            png_write_IDAT(png_ptr, png_ptr->zbuf, png_ptr->zbuf_size);
            png_ptr->zstream.next_out = png_ptr->zbuf;
            png_ptr->zstream.avail_out = (uInt)png_ptr->zbuf_size;
         }
      }
      else if (ret != Z_STREAM_END)
      {
         if (png_ptr->zstream.msg != NULL)
            png_error(png_ptr, png_ptr->zstream.msg);
         else
            png_error(png_ptr, "zlib error");
      }
   } while (ret != Z_STREAM_END);

   if (png_ptr->zstream.avail_out < png_ptr->zbuf_size)
   {
      png_write_IDAT(png_ptr, png_ptr->zbuf, png_ptr->zbuf_size -
         png_ptr->zstream.avail_out);
   }

   deflateReset(&png_ptr->zstream);
}

void
png_do_write_interlace(png_row_infop row_info, png_bytep row, int pass)
{

   png_debug(1, "in png_do_write_interlace\n");

   if (pass < 6)
   {

      switch (row_info->pixel_depth)
      {
         case 1:
         {
            png_bytep sp;
            png_bytep dp;
            int shift;
            int d;
            int value;
            png_uint_32 i;
            png_uint_32 row_width = row_info->width;

            dp = row;
            d = 0;
            shift = 7;
            for (i = png_pass_start[pass]; i < row_width;
               i += png_pass_inc[pass])
            {
               sp = row + (png_size_t)(i >> 3);
               value = (int)(*sp >> (7 - (int)(i & 0x07))) & 0x01;
               d |= (value << shift);

               if (shift == 0)
               {
                  shift = 7;
                  *dp++ = (png_byte)d;
                  d = 0;
               }
               else
                  shift--;

            }
            if (shift != 7)
               *dp = (png_byte)d;
            break;
         }
         case 2:
         {
            png_bytep sp;
            png_bytep dp;
            int shift;
            int d;
            int value;
            png_uint_32 i;
            png_uint_32 row_width = row_info->width;

            dp = row;
            shift = 6;
            d = 0;
            for (i = png_pass_start[pass]; i < row_width;
               i += png_pass_inc[pass])
            {
               sp = row + (png_size_t)(i >> 2);
               value = (*sp >> ((3 - (int)(i & 0x03)) << 1)) & 0x03;
               d |= (value << shift);

               if (shift == 0)
               {
                  shift = 6;
                  *dp++ = (png_byte)d;
                  d = 0;
               }
               else
                  shift -= 2;
            }
            if (shift != 6)
                   *dp = (png_byte)d;
            break;
         }
         case 4:
         {
            png_bytep sp;
            png_bytep dp;
            int shift;
            int d;
            int value;
            png_uint_32 i;
            png_uint_32 row_width = row_info->width;

            dp = row;
            shift = 4;
            d = 0;
            for (i = png_pass_start[pass]; i < row_width;
               i += png_pass_inc[pass])
            {
               sp = row + (png_size_t)(i >> 1);
               value = (*sp >> ((1 - (int)(i & 0x01)) << 2)) & 0x0f;
               d |= (value << shift);

               if (shift == 0)
               {
                  shift = 4;
                  *dp++ = (png_byte)d;
                  d = 0;
               }
               else
                  shift -= 4;
            }
            if (shift != 4)
               *dp = (png_byte)d;
            break;
         }
         default:
         {
            png_bytep sp;
            png_bytep dp;
            png_uint_32 i;
            png_uint_32 row_width = row_info->width;
            png_size_t pixel_bytes;

            dp = row;

            pixel_bytes = (row_info->pixel_depth >> 3);

            for (i = png_pass_start[pass]; i < row_width;
               i += png_pass_inc[pass])
            {

               sp = row + (png_size_t)i * pixel_bytes;

               if (dp != sp)
                  png_memcpy(dp, sp, pixel_bytes);

               dp += pixel_bytes;
            }
            break;
         }
      }

      row_info->width = (row_info->width +
         png_pass_inc[pass] - 1 -
         png_pass_start[pass]) /
         png_pass_inc[pass];
         row_info->rowbytes = PNG_ROWBYTES(row_info->pixel_depth,
            row_info->width);
   }
}

#define PNG_MAXSUM (~((png_uint_32)0) >> 1)
#define PNG_HISHIFT 10
#define PNG_LOMASK ((png_uint_32)0xffffL)
#define PNG_HIMASK ((png_uint_32)(~PNG_LOMASK >> PNG_HISHIFT))
void
png_write_find_filter(png_structp png_ptr, png_row_infop row_info)
{
   png_bytep prev_row, best_row, row_buf;
   png_uint_32 mins, bpp;
   png_byte filter_to_do = png_ptr->do_filter;
   png_uint_32 row_bytes = row_info->rowbytes;
   int num_p_filters = (int)png_ptr->num_prev_filters;

   png_debug(1, "in png_write_find_filter\n");

   bpp = (row_info->pixel_depth + 7) >> 3;

   prev_row = png_ptr->prev_row;
   best_row = row_buf = png_ptr->row_buf;
   mins = PNG_MAXSUM;

   if ((filter_to_do & PNG_FILTER_NONE) &&
       filter_to_do != PNG_FILTER_NONE)
   {
      png_bytep rp;
      png_uint_32 sum = 0;
      png_uint_32 i;
      int v;

      for (i = 0, rp = row_buf + 1; i < row_bytes; i++, rp++)
      {
         v = *rp;
         sum += (v < 128) ? v : 256 - v;
      }

      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         png_uint_32 sumhi, sumlo;
         int j;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_NONE)
            {
               sumlo = (sumlo * png_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               sumhi = (sumhi * png_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * png_ptr->filter_costs[PNG_FILTER_VALUE_NONE]) >>
            PNG_COST_SHIFT;
         sumhi = (sumhi * png_ptr->filter_costs[PNG_FILTER_VALUE_NONE]) >>
            PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;
         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }
      mins = sum;
   }

   if (filter_to_do == PNG_FILTER_SUB)

   {
      png_bytep rp, lp, dp;
      png_uint_32 i;
      for (i = 0, rp = row_buf + 1, dp = png_ptr->sub_row + 1; i < bpp;
           i++, rp++, dp++)
      {
         *dp = *rp;
      }
      for (lp = row_buf + 1; i < row_bytes;
         i++, rp++, lp++, dp++)
      {
         *dp = (png_byte)(((int)*rp - (int)*lp) & 0xff);
      }
      best_row = png_ptr->sub_row;
   }

   else if (filter_to_do & PNG_FILTER_SUB)
   {
      png_bytep rp, dp, lp;
      png_uint_32 sum = 0, lmins = mins;
      png_uint_32 i;
      int v;

      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 lmhi, lmlo;
         lmlo = lmins & PNG_LOMASK;
         lmhi = (lmins >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_SUB)
            {
               lmlo = (lmlo * png_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               lmhi = (lmhi * png_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         lmlo = (lmlo * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_SUB]) >>
            PNG_COST_SHIFT;
         lmhi = (lmhi * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_SUB]) >>
            PNG_COST_SHIFT;

         if (lmhi > PNG_HIMASK)
            lmins = PNG_MAXSUM;
         else
            lmins = (lmhi << PNG_HISHIFT) + lmlo;
      }

      for (i = 0, rp = row_buf + 1, dp = png_ptr->sub_row + 1; i < bpp;
           i++, rp++, dp++)
      {
         v = *dp = *rp;

         sum += (v < 128) ? v : 256 - v;
      }
      for (lp = row_buf + 1; i < row_bytes;
         i++, rp++, lp++, dp++)
      {
         v = *dp = (png_byte)(((int)*rp - (int)*lp) & 0xff);

         sum += (v < 128) ? v : 256 - v;

         if (sum > lmins)
            break;
      }

      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 sumhi, sumlo;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_SUB)
            {
               sumlo = (sumlo * png_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               sumhi = (sumhi * png_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_SUB]) >>
            PNG_COST_SHIFT;
         sumhi = (sumhi * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_SUB]) >>
            PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;
         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }

      if (sum < mins)
      {
         mins = sum;
         best_row = png_ptr->sub_row;
      }
   }

   if (filter_to_do == PNG_FILTER_UP)
   {
      png_bytep rp, dp, pp;
      png_uint_32 i;

      for (i = 0, rp = row_buf + 1, dp = png_ptr->up_row + 1,
           pp = prev_row + 1; i < row_bytes;
           i++, rp++, pp++, dp++)
      {
         *dp = (png_byte)(((int)*rp - (int)*pp) & 0xff);
      }
      best_row = png_ptr->up_row;
   }

   else if (filter_to_do & PNG_FILTER_UP)
   {
      png_bytep rp, dp, pp;
      png_uint_32 sum = 0, lmins = mins;
      png_uint_32 i;
      int v;

      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 lmhi, lmlo;
         lmlo = lmins & PNG_LOMASK;
         lmhi = (lmins >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_UP)
            {
               lmlo = (lmlo * png_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               lmhi = (lmhi * png_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         lmlo = (lmlo * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_UP]) >>
            PNG_COST_SHIFT;
         lmhi = (lmhi * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_UP]) >>
            PNG_COST_SHIFT;

         if (lmhi > PNG_HIMASK)
            lmins = PNG_MAXSUM;
         else
            lmins = (lmhi << PNG_HISHIFT) + lmlo;
      }

      for (i = 0, rp = row_buf + 1, dp = png_ptr->up_row + 1,
           pp = prev_row + 1; i < row_bytes; i++)
      {
         v = *dp++ = (png_byte)(((int)*rp++ - (int)*pp++) & 0xff);

         sum += (v < 128) ? v : 256 - v;

         if (sum > lmins)
            break;
      }

      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 sumhi, sumlo;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_UP)
            {
               sumlo = (sumlo * png_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               sumhi = (sumhi * png_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * png_ptr->filter_costs[PNG_FILTER_VALUE_UP]) >>
            PNG_COST_SHIFT;
         sumhi = (sumhi * png_ptr->filter_costs[PNG_FILTER_VALUE_UP]) >>
            PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;
         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }

      if (sum < mins)
      {
         mins = sum;
         best_row = png_ptr->up_row;
      }
   }

   if (filter_to_do == PNG_FILTER_AVG)
   {
      png_bytep rp, dp, pp, lp;
      png_uint_32 i;
      for (i = 0, rp = row_buf + 1, dp = png_ptr->avg_row + 1,
           pp = prev_row + 1; i < bpp; i++)
      {
         *dp++ = (png_byte)(((int)*rp++ - ((int)*pp++ / 2)) & 0xff);
      }
      for (lp = row_buf + 1; i < row_bytes; i++)
      {
         *dp++ = (png_byte)(((int)*rp++ - (((int)*pp++ + (int)*lp++) / 2))
                 & 0xff);
      }
      best_row = png_ptr->avg_row;
   }

   else if (filter_to_do & PNG_FILTER_AVG)
   {
      png_bytep rp, dp, pp, lp;
      png_uint_32 sum = 0, lmins = mins;
      png_uint_32 i;
      int v;

      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 lmhi, lmlo;
         lmlo = lmins & PNG_LOMASK;
         lmhi = (lmins >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_AVG)
            {
               lmlo = (lmlo * png_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               lmhi = (lmhi * png_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         lmlo = (lmlo * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_AVG]) >>
            PNG_COST_SHIFT;
         lmhi = (lmhi * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_AVG]) >>
            PNG_COST_SHIFT;

         if (lmhi > PNG_HIMASK)
            lmins = PNG_MAXSUM;
         else
            lmins = (lmhi << PNG_HISHIFT) + lmlo;
      }

      for (i = 0, rp = row_buf + 1, dp = png_ptr->avg_row + 1,
           pp = prev_row + 1; i < bpp; i++)
      {
         v = *dp++ = (png_byte)(((int)*rp++ - ((int)*pp++ / 2)) & 0xff);

         sum += (v < 128) ? v : 256 - v;
      }
      for (lp = row_buf + 1; i < row_bytes; i++)
      {
         v = *dp++ =
          (png_byte)(((int)*rp++ - (((int)*pp++ + (int)*lp++) / 2)) & 0xff);

         sum += (v < 128) ? v : 256 - v;

         if (sum > lmins)
            break;
      }

      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 sumhi, sumlo;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_NONE)
            {
               sumlo = (sumlo * png_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               sumhi = (sumhi * png_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * png_ptr->filter_costs[PNG_FILTER_VALUE_AVG]) >>
            PNG_COST_SHIFT;
         sumhi = (sumhi * png_ptr->filter_costs[PNG_FILTER_VALUE_AVG]) >>
            PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;
         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }

      if (sum < mins)
      {
         mins = sum;
         best_row = png_ptr->avg_row;
      }
   }

   if (filter_to_do == PNG_FILTER_PAETH)
   {
      png_bytep rp, dp, pp, cp, lp;
      png_uint_32 i;
      for (i = 0, rp = row_buf + 1, dp = png_ptr->paeth_row + 1,
           pp = prev_row + 1; i < bpp; i++)
      {
         *dp++ = (png_byte)(((int)*rp++ - (int)*pp++) & 0xff);
      }

      for (lp = row_buf + 1, cp = prev_row + 1; i < row_bytes; i++)
      {
         int a, b, c, pa, pb, pc, p;

         b = *pp++;
         c = *cp++;
         a = *lp++;

         p = b - c;
         pc = a - c;

         pa = p < 0 ? -p : p;
         pb = pc < 0 ? -pc : pc;
         pc = (p + pc) < 0 ? -(p + pc) : p + pc;

         p = (pa <= pb && pa <=pc) ? a : (pb <= pc) ? b : c;

         *dp++ = (png_byte)(((int)*rp++ - p) & 0xff);
      }
      best_row = png_ptr->paeth_row;
   }

   else if (filter_to_do & PNG_FILTER_PAETH)
   {
      png_bytep rp, dp, pp, cp, lp;
      png_uint_32 sum = 0, lmins = mins;
      png_uint_32 i;
      int v;

      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 lmhi, lmlo;
         lmlo = lmins & PNG_LOMASK;
         lmhi = (lmins >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_PAETH)
            {
               lmlo = (lmlo * png_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               lmhi = (lmhi * png_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         lmlo = (lmlo * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_PAETH]) >>
            PNG_COST_SHIFT;
         lmhi = (lmhi * png_ptr->inv_filter_costs[PNG_FILTER_VALUE_PAETH]) >>
            PNG_COST_SHIFT;

         if (lmhi > PNG_HIMASK)
            lmins = PNG_MAXSUM;
         else
            lmins = (lmhi << PNG_HISHIFT) + lmlo;
      }

      for (i = 0, rp = row_buf + 1, dp = png_ptr->paeth_row + 1,
           pp = prev_row + 1; i < bpp; i++)
      {
         v = *dp++ = (png_byte)(((int)*rp++ - (int)*pp++) & 0xff);

         sum += (v < 128) ? v : 256 - v;
      }

      for (lp = row_buf + 1, cp = prev_row + 1; i < row_bytes; i++)
      {
         int a, b, c, pa, pb, pc, p;

         b = *pp++;
         c = *cp++;
         a = *lp++;

         p = b - c;
         pc = a - c;
         pa = p < 0 ? -p : p;
         pb = pc < 0 ? -pc : pc;
         pc = (p + pc) < 0 ? -(p + pc) : p + pc;
         p = (pa <= pb && pa <=pc) ? a : (pb <= pc) ? b : c;

         v = *dp++ = (png_byte)(((int)*rp++ - p) & 0xff);

         sum += (v < 128) ? v : 256 - v;

         if (sum > lmins)
            break;
      }

      if (png_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         png_uint_32 sumhi, sumlo;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (png_ptr->prev_filters[j] == PNG_FILTER_VALUE_PAETH)
            {
               sumlo = (sumlo * png_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               sumhi = (sumhi * png_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * png_ptr->filter_costs[PNG_FILTER_VALUE_PAETH]) >>
            PNG_COST_SHIFT;
         sumhi = (sumhi * png_ptr->filter_costs[PNG_FILTER_VALUE_PAETH]) >>
            PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;
         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }

      if (sum < mins)
      {
         best_row = png_ptr->paeth_row;
      }
   }

   png_write_filtered_row(png_ptr, best_row);

   if (png_ptr->num_prev_filters > 0)
   {
      int j;
      for (j = 1; j < num_p_filters; j++)
      {
         png_ptr->prev_filters[j] = png_ptr->prev_filters[j - 1];
      }
      png_ptr->prev_filters[j] = best_row[0];
   }
}

void
png_write_filtered_row(png_structp png_ptr, png_bytep filtered_row)
{
   png_debug(1, "in png_write_filtered_row\n");
   png_debug1(2, "filter = %d\n", filtered_row[0]);

   png_ptr->zstream.next_in = filtered_row;
   png_ptr->zstream.avail_in = (uInt)png_ptr->row_info.rowbytes + 1;

   do
   {
      int ret;

      ret = deflate(&png_ptr->zstream, Z_NO_FLUSH);

      if (ret != Z_OK)
      {
         if (png_ptr->zstream.msg != NULL)
            png_error(png_ptr, png_ptr->zstream.msg);
         else
            png_error(png_ptr, "zlib error");
      }

      if (!(png_ptr->zstream.avail_out))
      {

         png_write_IDAT(png_ptr, png_ptr->zbuf, png_ptr->zbuf_size);
         png_ptr->zstream.next_out = png_ptr->zbuf;
         png_ptr->zstream.avail_out = (uInt)png_ptr->zbuf_size;
      }

   } while (png_ptr->zstream.avail_in);

   if (png_ptr->prev_row != NULL)
   {
      png_bytep tptr;

      tptr = png_ptr->prev_row;
      png_ptr->prev_row = png_ptr->row_buf;
      png_ptr->row_buf = tptr;
   }

   png_write_finish_row(png_ptr);

   png_ptr->flush_rows++;

   if (png_ptr->flush_dist > 0 &&
       png_ptr->flush_rows >= png_ptr->flush_dist)
   {
      png_write_flush(png_ptr);
   }
}
