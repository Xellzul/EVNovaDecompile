// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -g -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libpng 1.2.7's one line, compiler and headers (zlib's compiler, built with debug information on, against the 2.4-era MSL). -g moves jump threading and register choice; the 2.4-era math_x87.h makes fabs an asm inline; build 335 has no -proc generic, its default is generic x86; Research/runtime-compiler-ceiling.md
/* pngget.c -- libpng 1.2.7 pngget.c, Copyright (c) 1998-2004 Glenn Randers-Pehrson, (c) 1996-1997 Andreas Dilger, (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.; libpng licence in png.h; altered: comments and the #if branches this build does not take removed (the functions the exe does not link stay, unmarked). Notes: Research/notes/png.md */

#define PNG_INTERNAL
#include "png.h"

/* png_get_valid. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538bfd
png_uint_32 PNGAPI
png_get_valid(png_structp png_ptr, png_infop info_ptr, png_uint_32 flag)
{
   if (png_ptr != NULL && info_ptr != NULL)
      return(info_ptr->valid & flag);
   else
      return(0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_rowbytes(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
      return(info_ptr->rowbytes);
   else
      return(0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_bytepp PNGAPI
png_get_rows(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
      return(info_ptr->row_pointers);
   else
      return(0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_image_width(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->width;
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_image_height(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->height;
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_byte PNGAPI
png_get_bit_depth(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->bit_depth;
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_byte PNGAPI
png_get_color_type(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->color_type;
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_byte PNGAPI
png_get_filter_type(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->filter_type;
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_byte PNGAPI
png_get_interlace_type(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->interlace_type;
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_byte PNGAPI
png_get_compression_type(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->compression_type;
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_x_pixels_per_meter(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   if (info_ptr->valid & PNG_INFO_pHYs)
   {
      png_debug1(1, "in %s retrieval function\n", "png_get_x_pixels_per_meter");
      if(info_ptr->phys_unit_type != PNG_RESOLUTION_METER)
          return (0);
      else return (info_ptr->x_pixels_per_unit);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_y_pixels_per_meter(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   if (info_ptr->valid & PNG_INFO_pHYs)
   {
      png_debug1(1, "in %s retrieval function\n", "png_get_y_pixels_per_meter");
      if(info_ptr->phys_unit_type != PNG_RESOLUTION_METER)
          return (0);
      else return (info_ptr->y_pixels_per_unit);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_pixels_per_meter(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   if (info_ptr->valid & PNG_INFO_pHYs)
   {
      png_debug1(1, "in %s retrieval function\n", "png_get_pixels_per_meter");
      if(info_ptr->phys_unit_type != PNG_RESOLUTION_METER ||
         info_ptr->x_pixels_per_unit != info_ptr->y_pixels_per_unit)
          return (0);
      else return (info_ptr->x_pixels_per_unit);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
float PNGAPI
png_get_pixel_aspect_ratio(png_structp png_ptr, png_infop info_ptr)
   {
   if (png_ptr != NULL && info_ptr != NULL)
   if (info_ptr->valid & PNG_INFO_pHYs)
   {
      png_debug1(1, "in %s retrieval function\n", "png_get_aspect_ratio");
      if (info_ptr->x_pixels_per_unit == 0)
         return ((float)0.0);
      else
         return ((float)((float)info_ptr->y_pixels_per_unit
            /(float)info_ptr->x_pixels_per_unit));
   }
   return ((float)0.0);
}

/* png_get_x_offset_microns. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538bd4
png_int_32 PNGAPI
png_get_x_offset_microns(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   if (info_ptr->valid & PNG_INFO_oFFs)
   {
      png_debug1(1, "in %s retrieval function\n", "png_get_x_offset_microns");
      if(info_ptr->offset_unit_type != PNG_OFFSET_MICROMETER)
          return (0);
      else return (info_ptr->x_offset);
   }
   return (0);
}

/* png_get_y_offset_microns. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538bab
png_int_32 PNGAPI
png_get_y_offset_microns(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   if (info_ptr->valid & PNG_INFO_oFFs)
   {
      png_debug1(1, "in %s retrieval function\n", "png_get_y_offset_microns");
      if(info_ptr->offset_unit_type != PNG_OFFSET_MICROMETER)
          return (0);
      else return (info_ptr->y_offset);
   }
   return (0);
}

/* png_get_x_offset_pixels. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538b82
png_int_32 PNGAPI
png_get_x_offset_pixels(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   if (info_ptr->valid & PNG_INFO_oFFs)
   {
      png_debug1(1, "in %s retrieval function\n", "png_get_x_offset_microns");
      if(info_ptr->offset_unit_type != PNG_OFFSET_PIXEL)
          return (0);
      else return (info_ptr->x_offset);
   }
   return (0);
}

/* png_get_y_offset_pixels. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538b59
png_int_32 PNGAPI
png_get_y_offset_pixels(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   if (info_ptr->valid & PNG_INFO_oFFs)
   {
      png_debug1(1, "in %s retrieval function\n", "png_get_y_offset_microns");
      if(info_ptr->offset_unit_type != PNG_OFFSET_PIXEL)
          return (0);
      else return (info_ptr->y_offset);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_byte PNGAPI
png_get_channels(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
      return(info_ptr->channels);
   else
      return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_bytep PNGAPI
png_get_signature(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
      return(info_ptr->signature);
   else
      return (NULL);
}

/* png_get_bKGD. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538b2c
png_uint_32 PNGAPI
png_get_bKGD(png_structp png_ptr, png_infop info_ptr,
   png_color_16p *background)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_bKGD)
      && background != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "bKGD");
      *background = &(info_ptr->background);
      return (PNG_INFO_bKGD);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_cHRM(png_structp png_ptr, png_infop info_ptr,
   double *white_x, double *white_y, double *red_x, double *red_y,
   double *green_x, double *green_y, double *blue_x, double *blue_y)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_cHRM))
   {
      png_debug1(1, "in %s retrieval function\n", "cHRM");
      if (white_x != NULL)
         *white_x = (double)info_ptr->x_white;
      if (white_y != NULL)
         *white_y = (double)info_ptr->y_white;
      if (red_x != NULL)
         *red_x = (double)info_ptr->x_red;
      if (red_y != NULL)
         *red_y = (double)info_ptr->y_red;
      if (green_x != NULL)
         *green_x = (double)info_ptr->x_green;
      if (green_y != NULL)
         *green_y = (double)info_ptr->y_green;
      if (blue_x != NULL)
         *blue_x = (double)info_ptr->x_blue;
      if (blue_y != NULL)
         *blue_y = (double)info_ptr->y_blue;
      return (PNG_INFO_cHRM);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_cHRM_fixed(png_structp png_ptr, png_infop info_ptr,
   png_fixed_point *white_x, png_fixed_point *white_y, png_fixed_point *red_x,
   png_fixed_point *red_y, png_fixed_point *green_x, png_fixed_point *green_y,
   png_fixed_point *blue_x, png_fixed_point *blue_y)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_cHRM))
   {
      png_debug1(1, "in %s retrieval function\n", "cHRM");
      if (white_x != NULL)
         *white_x = info_ptr->int_x_white;
      if (white_y != NULL)
         *white_y = info_ptr->int_y_white;
      if (red_x != NULL)
         *red_x = info_ptr->int_x_red;
      if (red_y != NULL)
         *red_y = info_ptr->int_y_red;
      if (green_x != NULL)
         *green_x = info_ptr->int_x_green;
      if (green_y != NULL)
         *green_y = info_ptr->int_y_green;
      if (blue_x != NULL)
         *blue_x = info_ptr->int_x_blue;
      if (blue_y != NULL)
         *blue_y = info_ptr->int_y_blue;
      return (PNG_INFO_cHRM);
   }
   return (0);
}

/* png_get_gAMA. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538b01
png_uint_32 PNGAPI
png_get_gAMA(png_structp png_ptr, png_infop info_ptr, double *file_gamma)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_gAMA)
      && file_gamma != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "gAMA");
      *file_gamma = (double)info_ptr->gamma;
      return (PNG_INFO_gAMA);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_gAMA_fixed(png_structp png_ptr, png_infop info_ptr,
    png_fixed_point *int_file_gamma)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_gAMA)
      && int_file_gamma != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "gAMA");
      *int_file_gamma = info_ptr->int_gamma;
      return (PNG_INFO_gAMA);
   }
   return (0);
}

/* png_get_sRGB. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538ad1
png_uint_32 PNGAPI
png_get_sRGB(png_structp png_ptr, png_infop info_ptr, int *file_srgb_intent)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_sRGB)
      && file_srgb_intent != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "sRGB");
      *file_srgb_intent = (int)info_ptr->srgb_intent;
      return (PNG_INFO_sRGB);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_iCCP(png_structp png_ptr, png_infop info_ptr,
             png_charpp name, int *compression_type,
             png_charpp profile, png_uint_32 *proflen)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_iCCP)
      && name != NULL && profile != NULL && proflen != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "iCCP");
      *name = info_ptr->iccp_name;
      *profile = info_ptr->iccp_profile;

      *proflen = (int)info_ptr->iccp_proflen;
      *compression_type = (int)info_ptr->iccp_compression;
      return (PNG_INFO_iCCP);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_sPLT(png_structp png_ptr, png_infop info_ptr,
             png_sPLT_tpp spalettes)
{
   if (png_ptr != NULL && info_ptr != NULL && spalettes != NULL)
     *spalettes = info_ptr->splt_palettes;
   return ((png_uint_32)info_ptr->splt_palettes_num);
}

/* png_get_hIST. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538aa4
png_uint_32 PNGAPI
png_get_hIST(png_structp png_ptr, png_infop info_ptr, png_uint_16p *hist)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_hIST)
      && hist != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "hIST");
      *hist = info_ptr->hist;
      return (PNG_INFO_hIST);
   }
   return (0);
}

/* png_get_IHDR. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538982
png_uint_32 PNGAPI
png_get_IHDR(png_structp png_ptr, png_infop info_ptr,
   png_uint_32 *width, png_uint_32 *height, int *bit_depth,
   int *color_type, int *interlace_type, int *compression_type,
   int *filter_type)

{
   if (png_ptr != NULL && info_ptr != NULL && width != NULL && height != NULL &&
      bit_depth != NULL && color_type != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "IHDR");
      *width = info_ptr->width;
      *height = info_ptr->height;
      *bit_depth = info_ptr->bit_depth;
      if (info_ptr->bit_depth < 1 || info_ptr->bit_depth > 16)
        png_error(png_ptr, "Invalid bit depth");
      *color_type = info_ptr->color_type;
      if (info_ptr->color_type > 6)
        png_error(png_ptr, "Invalid color type");
      if (compression_type != NULL)
         *compression_type = info_ptr->compression_type;
      if (filter_type != NULL)
         *filter_type = info_ptr->filter_type;
      if (interlace_type != NULL)
         *interlace_type = info_ptr->interlace_type;

      if (width == 0 || *width > PNG_UINT_31_MAX)
        png_error(png_ptr, "Invalid image width");
      if (height == 0 || *height > PNG_UINT_31_MAX)
        png_error(png_ptr, "Invalid image height");
      if (info_ptr->width > (PNG_UINT_32_MAX
                 >> 3)
                 - 64
                 - 1
                 - 7*8
                 - 8)
      {
         png_warning(png_ptr,
            "Width too large for libpng to process image data.");
      }
      return (1);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_oFFs(png_structp png_ptr, png_infop info_ptr,
   png_int_32 *offset_x, png_int_32 *offset_y, int *unit_type)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_oFFs)
      && offset_x != NULL && offset_y != NULL && unit_type != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "oFFs");
      *offset_x = info_ptr->x_offset;
      *offset_y = info_ptr->y_offset;
      *unit_type = (int)info_ptr->offset_unit_type;
      return (PNG_INFO_oFFs);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_pCAL(png_structp png_ptr, png_infop info_ptr,
   png_charp *purpose, png_int_32 *X0, png_int_32 *X1, int *type, int *nparams,
   png_charp *units, png_charpp *params)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_pCAL)
      && purpose != NULL && X0 != NULL && X1 != NULL && type != NULL &&
      nparams != NULL && units != NULL && params != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "pCAL");
      *purpose = info_ptr->pcal_purpose;
      *X0 = info_ptr->pcal_X0;
      *X1 = info_ptr->pcal_X1;
      *type = (int)info_ptr->pcal_type;
      *nparams = (int)info_ptr->pcal_nparams;
      *units = info_ptr->pcal_units;
      *params = info_ptr->pcal_params;
      return (PNG_INFO_pCAL);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_sCAL(png_structp png_ptr, png_infop info_ptr,
             int *unit, double *width, double *height)
{
    if (png_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & PNG_INFO_sCAL))
    {
        *unit = info_ptr->scal_unit;
        *width = info_ptr->scal_pixel_width;
        *height = info_ptr->scal_pixel_height;
        return (PNG_INFO_sCAL);
    }
    return(0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_pHYs(png_structp png_ptr, png_infop info_ptr,
   png_uint_32 *res_x, png_uint_32 *res_y, int *unit_type)
{
   png_uint_32 retval = 0;

   if (png_ptr != NULL && info_ptr != NULL &&
      (info_ptr->valid & PNG_INFO_pHYs))
   {
      png_debug1(1, "in %s retrieval function\n", "pHYs");
      if (res_x != NULL)
      {
         *res_x = info_ptr->x_pixels_per_unit;
         retval |= PNG_INFO_pHYs;
      }
      if (res_y != NULL)
      {
         *res_y = info_ptr->y_pixels_per_unit;
         retval |= PNG_INFO_pHYs;
      }
      if (unit_type != NULL)
      {
         *unit_type = (int)info_ptr->phys_unit_type;
         retval |= PNG_INFO_pHYs;
      }
   }
   return (retval);
}

/* png_get_PLTE. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00538948
png_uint_32 PNGAPI
png_get_PLTE(png_structp png_ptr, png_infop info_ptr, png_colorp *palette,
   int *num_palette)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_PLTE)
       && palette != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "PLTE");
      *palette = info_ptr->palette;
      *num_palette = info_ptr->num_palette;
      png_debug1(3, "num_palette = %d\n", *num_palette);
      return (PNG_INFO_PLTE);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_sBIT(png_structp png_ptr, png_infop info_ptr, png_color_8p *sig_bit)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_sBIT)
      && sig_bit != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "sBIT");
      *sig_bit = &(info_ptr->sig_bit);
      return (PNG_INFO_sBIT);
   }
   return (0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_text(png_structp png_ptr, png_infop info_ptr, png_textp *text_ptr,
   int *num_text)
{
   if (png_ptr != NULL && info_ptr != NULL && info_ptr->num_text > 0)
   {
      png_debug1(1, "in %s retrieval function\n",
         (png_ptr->chunk_name[0] == '\0' ? "text"
             : (png_const_charp)png_ptr->chunk_name));
      if (text_ptr != NULL)
         *text_ptr = info_ptr->text;
      if (num_text != NULL)
         *num_text = info_ptr->num_text;
      return ((png_uint_32)info_ptr->num_text);
   }
   if (num_text != NULL)
     *num_text = 0;
   return(0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_tIME(png_structp png_ptr, png_infop info_ptr, png_timep *mod_time)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_tIME)
       && mod_time != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "tIME");
      *mod_time = &(info_ptr->mod_time);
      return (PNG_INFO_tIME);
   }
   return (0);
}

/* png_get_tRNS. STATE: genuine libpng 1.2.7; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005388de
png_uint_32 PNGAPI
png_get_tRNS(png_structp png_ptr, png_infop info_ptr,
   png_bytep *trans, int *num_trans, png_color_16p *trans_values)
{
   png_uint_32 retval = 0;
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_tRNS))
   {
      png_debug1(1, "in %s retrieval function\n", "tRNS");
      if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
          if (trans != NULL)
          {
             *trans = info_ptr->trans;
             retval |= PNG_INFO_tRNS;
          }
          if (trans_values != NULL)
             *trans_values = &(info_ptr->trans_values);
      }
      else
      {
          if (trans_values != NULL)
          {
             *trans_values = &(info_ptr->trans_values);
             retval |= PNG_INFO_tRNS;
          }
          if(trans != NULL)
             *trans = NULL;
      }
      if(num_trans != NULL)
      {
         *num_trans = info_ptr->num_trans;
         retval |= PNG_INFO_tRNS;
      }
   }
   return (retval);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_unknown_chunks(png_structp png_ptr, png_infop info_ptr,
             png_unknown_chunkpp unknowns)
{
   if (png_ptr != NULL && info_ptr != NULL && unknowns != NULL)
     *unknowns = info_ptr->unknown_chunks;
   return ((png_uint_32)info_ptr->unknown_chunks_num);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_byte PNGAPI
png_get_rgb_to_gray_status (png_structp png_ptr)
{
   return (png_byte)(png_ptr? png_ptr->rgb_to_gray_status : 0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_voidp PNGAPI
png_get_user_chunk_ptr(png_structp png_ptr)
{
   return (png_ptr? png_ptr->user_chunk_ptr : NULL);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_compression_buffer_size(png_structp png_ptr)
{
   return (png_uint_32)(png_ptr? png_ptr->zbuf_size : 0L);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_asm_flags (png_structp png_ptr)
{
    return (png_uint_32)(png_ptr? png_ptr->asm_flags : 0L);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_asm_flagmask (int flag_select)
{
    png_uint_32 settable_asm_flags = 0;

    if (flag_select & PNG_SELECT_READ)
        settable_asm_flags |=
          PNG_ASM_FLAG_MMX_READ_COMBINE_ROW  |
          PNG_ASM_FLAG_MMX_READ_INTERLACE    |
          PNG_ASM_FLAG_MMX_READ_FILTER_SUB   |
          PNG_ASM_FLAG_MMX_READ_FILTER_UP    |
          PNG_ASM_FLAG_MMX_READ_FILTER_AVG   |
          PNG_ASM_FLAG_MMX_READ_FILTER_PAETH ;


    return settable_asm_flags;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_mmx_flagmask (int flag_select, int *compilerID)
{
    png_uint_32 settable_mmx_flags = 0;

    if (flag_select & PNG_SELECT_READ)
        settable_mmx_flags |=
          PNG_ASM_FLAG_MMX_READ_COMBINE_ROW  |
          PNG_ASM_FLAG_MMX_READ_INTERLACE    |
          PNG_ASM_FLAG_MMX_READ_FILTER_SUB   |
          PNG_ASM_FLAG_MMX_READ_FILTER_UP    |
          PNG_ASM_FLAG_MMX_READ_FILTER_AVG   |
          PNG_ASM_FLAG_MMX_READ_FILTER_PAETH ;

    if (compilerID != NULL) {
        *compilerID = -1;
    }

    return settable_mmx_flags;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_byte PNGAPI
png_get_mmx_bitdepth_threshold (png_structp png_ptr)
{
    return (png_byte)(png_ptr? png_ptr->mmx_bitdepth_threshold : 0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_mmx_rowbytes_threshold (png_structp png_ptr)
{
    return (png_uint_32)(png_ptr? png_ptr->mmx_rowbytes_threshold : 0L);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_user_width_max (png_structp png_ptr)
{
    return (png_ptr? png_ptr->user_width_max : 0);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
png_uint_32 PNGAPI
png_get_user_height_max (png_structp png_ptr)
{
    return (png_ptr? png_ptr->user_height_max : 0);
}
