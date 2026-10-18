// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics (inert in this file: it has no constant-size memset); -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itjpeg.c -- the interface tool's JPEG reader over libjpeg 6b: a jdatasrc.c-style source manager on the loaders' memory stream, the JFIF signature test and the loader. Notes: Research/notes/itjpeg.md */

#include <setjmp.h>
#include "burgertypes.h"
#include "jinclude.h"
#include "jpeglib.h"
#include "jversion.h"
#include "jerror.h"
#include "jpegerrmgr.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"

#define JMESSAGE(code,string)	string ,

const char * const jpeg_std_message_table[] = {
#include "jerror.h"
  NULL
};

#define kJfifMinSize        11
#define kFormatJpeg         0x4a504547    /* 'JPEG' */
#define kImageDepth         32

/* jdatasrc.c's source manager, reading the loaders' memory stream instead of a FILE */
typedef struct {
  struct jpeg_source_mgr pub;
  ITStream * stream;
  JOCTET * buffer;
  boolean start_of_file;
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

#define INPUT_BUF_SIZE  4096

/* init_source (jdatasrc.c). STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051435a
METHODDEF(void)
init_source (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;
  long err;

  err = 0;
  ((my_src_ptr) cinfo->src)->start_of_file = TRUE;
}

/* fill_input_buffer (jdatasrc.c on the stream; a stream error is reported as
   JERR_INPUT_EMPTY, the buffer left as it was). STATE: MATCH 100%, exe 64 / obj 64,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051436e
METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;
  long err = 0;
  long e;
  size_t nbytes;

  nbytes = src->stream->size - src->stream->pos;
  if (nbytes > INPUT_BUF_SIZE)
    nbytes = INPUT_BUF_SIZE;
  if (nbytes != 0) {
    e = DT_BufferRead(src->stream, src->buffer, nbytes);
    if (e != 0) {
      err = e;
      goto done;
    }
  } else {
    if (src->start_of_file)	/* Treat empty input file as fatal error */
      ERREXIT(cinfo, JERR_INPUT_EMPTY);
    WARNMS(cinfo, JWRN_JPEG_EOF);
    /* Insert a fake EOI marker */
    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG_EOI;
    nbytes = 2;
  }
  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = nbytes;
  src->start_of_file = FALSE;
done:
  if (err != 0)
    ERREXIT(cinfo, JERR_INPUT_EMPTY);
  return TRUE;
}

/* skip_input_data (jdatasrc.c). STATE: MATCH 100%, exe 25 / obj 25, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0051440b
METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;
  long err;

  err = 0;

  if (num_bytes > 0) {
    while (num_bytes > (long) src->pub.bytes_in_buffer) {
      num_bytes -= (long) src->pub.bytes_in_buffer;
      (void) fill_input_buffer(cinfo);
    }
    src->pub.next_input_byte += (size_t) num_bytes;
    src->pub.bytes_in_buffer -= (size_t) num_bytes;
  }
}

/* term_source (jdatasrc.c). STATE: MATCH 100%, exe 5 / obj 5, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051443e
METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;
  long err;

  err = 0;
}

/* error_exit. STATE: MATCH 100.0, exe 26 vs obj 26 instructions, byte-identical yes.
   OPEN: none. `long err = 0` is the dead zero pair the exe opens with. */
// FUNCTION: EVNOVA 0x0051444a
METHODDEF(void)
error_exit (j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr) cinfo->err;
  long err = 0;

  if (myerr->pub.output_message != NULL)
    (*myerr->pub.output_message) (cinfo);
  longjmp(myerr->setjmp_buffer, myerr->pub.msg_code ? myerr->pub.msg_code : 1);
}

/* output_message. STATE: MATCH 100.0, exe 22 vs obj 22 instructions, byte-identical yes.
   OPEN: none. The message is formatted and dropped; whatever printed it is compiled out. */
// FUNCTION: EVNOVA 0x0051447d
METHODDEF(void)
output_message (j_common_ptr cinfo)
{
  struct jpeg_error_mgr * err = cinfo->err;
  char buffer[JMSG_LENGTH_MAX] = "";

  if (err->format_message != NULL)
    (*err->format_message) (cinfo, buffer);
}

/* emit_message. STATE: MATCH 100.0, exe 19 vs obj 19 instructions, byte-identical yes.
   OPEN: none. Only the first warning is shown; num_warnings is never counted. */
// FUNCTION: EVNOVA 0x005144b6
METHODDEF(void)
emit_message (j_common_ptr cinfo, int msg_level)
{
  struct jpeg_error_mgr * err = cinfo->err;
  boolean show = FALSE;

  if (msg_level < 0 && err->num_warnings == 0)
    show = TRUE;
  if (show && err->output_message != NULL)
    (*err->output_message) (cinfo);
}

/* format_message. STATE: MATCH 100.0, exe 67 vs obj 67 instructions, byte-identical yes.
   OPEN: none. 6b's lookup; the %s scan and sprintf are BurgerLib's strstr and bounded
   sprintf; `long e = 0` is the dead zero pair. */
// FUNCTION: EVNOVA 0x005144df
METHODDEF(void)
format_message (j_common_ptr cinfo, char * buffer)
{
  struct jpeg_error_mgr * err = cinfo->err;
  long e = 0;
  const char * msgtext = NULL;
  int msg_code = err->msg_code;

  if (msg_code > 0 && msg_code <= err->last_jpeg_message) {
    msgtext = err->jpeg_message_table[msg_code];
  } else if (err->addon_message_table != NULL &&
	     msg_code >= err->first_addon_message &&
	     msg_code <= err->last_addon_message) {
    msgtext = err->addon_message_table[msg_code - err->first_addon_message];
  }

  if (msgtext == NULL) {
    err->msg_parm.i[0] = msg_code;
    msgtext = err->jpeg_message_table[0];
  }

  if (StringFindString(msgtext, "%s") != NULL)
    StringFormatSafe(buffer, JMSG_LENGTH_MAX, msgtext, err->msg_parm.s);
  else
    StringFormatSafe(buffer, JMSG_LENGTH_MAX, msgtext,
	    err->msg_parm.i[0], err->msg_parm.i[1],
	    err->msg_parm.i[2], err->msg_parm.i[3],
	    err->msg_parm.i[4], err->msg_parm.i[5],
	    err->msg_parm.i[6], err->msg_parm.i[7]);
}

/* reset_error_mgr. STATE: MATCH 100.0, exe 7 vs obj 7 instructions, byte-identical yes.
   OPEN: none. msg_code is cleared before num_warnings, through a local err. */
// FUNCTION: EVNOVA 0x00514582
METHODDEF(void)
reset_error_mgr (j_common_ptr cinfo)
{
  struct jpeg_error_mgr * err = cinfo->err;

  err->msg_code = 0;
  err->num_warnings = 0;
}

/* jpeg_std_error. STATE: MATCH 100.0, exe 24 vs obj 24 instructions, byte-identical yes.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00514593
GLOBAL(struct jpeg_error_mgr *)
jpeg_std_error (struct jpeg_error_mgr * err)
{
  err->error_exit = error_exit;
  err->emit_message = emit_message;
  err->output_message = output_message;
  err->format_message = format_message;
  err->reset_error_mgr = reset_error_mgr;

  err->trace_level = 0;
  err->num_warnings = 0;
  err->msg_code = 0;

  err->jpeg_message_table = jpeg_std_message_table;
  err->last_jpeg_message = (int) JMSG_LASTMSGCODE - 1;

  err->addon_message_table = NULL;
  err->first_addon_message = 0;
  err->last_addon_message = 0;

  return err;
}

/* JPEG_CheckFileType (a JFIF file: SOI and APP0 markers, "JFIF\0" at 6). STATE: MATCH 100%, exe 21 /
   obj 21, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005145ed
unsigned char JPEG_CheckFileType(const void *buffer, ULong size)
{
    const unsigned char *data = (const unsigned char *)buffer;

    if (size >= kJfifMinSize && data[0] == 0xff && data[2] == 0xff && data[6] == 'J' &&
        data[7] == 'F' && data[8] == 'I' && data[9] == 'F' && data[10] == 0)
        return 1;
    return 0;
}

/* JPEG_Decode (load a JPEG off a stream into a new 32-bit image, row by row as a r g b).
   STATE: MATCH 100%, exe 249 / obj 249, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00514627
long JPEG_Decode(ITStream *stream, ITImage **imageOut, ITImage **maskOut,
                  ITTransparent *trans, ITBackground *bg)
{
    long err;
    ITImage *spare;
    ITImage *mask;
    long e;
    ULong y;
    unsigned char locked;
    JSAMPROW scan;
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    ITRect bounds;
    my_src_ptr src;
    ITColor *pixels;
    ULong j;
    ULong x;

    err = 0;
    spare = mask = NULL;
    src = NULL;
    pixels = NULL;
    scan = NULL;
    locked = 0;
    *imageOut = NULL;
    if (maskOut != NULL)
        *maskOut = mask;
    if (trans != NULL)
        DT_AlphaInfoDefaults(trans);
    if (bg != NULL)
        DT_FileInfoDefaults(bg);
    /* BUG: clears a decompress struct with the smaller compress struct's size */
    MemoryClear(&cinfo, sizeof(struct jpeg_compress_struct));
    MemoryClear(&jerr, sizeof(jerr));
    e = setjmp(jerr.setjmp_buffer);
    if (e != 0) {
        if (e == JERR_OUT_OF_MEMORY) {
            err = e;
            goto nomem;
        }
        err = e;
        err = kStreamOverrun;
        goto done;
    }
    cinfo.err = jpeg_std_error(&jerr.pub);
    jpeg_create_decompress(&cinfo);

    src = (my_src_ptr)MemoryAllocate(sizeof(my_source_mgr));
    if (src == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(src, sizeof(my_source_mgr));
    src->buffer = (JOCTET *)MemoryAllocate(INPUT_BUF_SIZE);
    if (src->buffer == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    src->pub.init_source = init_source;
    src->pub.fill_input_buffer = fill_input_buffer;
    src->pub.skip_input_data = skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart;
    src->pub.term_source = term_source;
    src->stream = stream;
    src->pub.bytes_in_buffer = 0;
    src->pub.next_input_byte = NULL;
    cinfo.src = &src->pub;

    jpeg_read_header(&cinfo, TRUE);
    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);

    DT_SetRect(&bounds, 0, 0, cinfo.output_width, cinfo.output_height);
    if ((*imageOut = DT_ImageCreate(&bounds, kImageDepth, NULL)) == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    e = DT_ImageLock(*imageOut, NULL, NULL, NULL);
    if (e != 0) {
        err = e;
        goto done;
    }
    locked = 1;
    if (bg != NULL) {
        bg->format = kFormatJpeg;
        bg->gamma = cinfo.output_gamma;
    }
    pixels = (ITColor *)MemoryAllocate(cinfo.output_width * sizeof(ITColor));
    if (pixels == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    scan = (JSAMPROW)MemoryAllocate(cinfo.output_width * cinfo.output_components);
    if (scan == NULL) {
nomem:
        err = kOutOfMemory;
        goto done;
    }

    for (y = 0; y < cinfo.image_height; y++) {
        if (jpeg_read_scanlines(&cinfo, &scan, 1) == 0)
            err = kImageFailed;
        for (x = 0; x < cinfo.image_width; x++) {
            pixels[x].alpha = 0xff;
            j = x * 3;
            pixels[x].red = scan[j];
            pixels[x].green = scan[j + 1];
            pixels[x].blue = scan[j + 2];
        }
        DT_ImageSetRawColors(*imageOut, NULL, y, pixels);
    }
    jpeg_finish_decompress(&cinfo);
done:
    jpeg_destroy_decompress(&cinfo);
    if (locked)
        DT_ImageUnlock(*imageOut, NULL);
    if (scan != NULL)
        MemoryDeallocate(scan);
    if (pixels != NULL)
        MemoryDeallocate(pixels);
    if (src != NULL) {
        if (src->buffer != NULL)
            MemoryDeallocate(src->buffer);
        MemoryDeallocate(src);
    }
    return err;
}
