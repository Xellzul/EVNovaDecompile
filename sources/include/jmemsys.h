/* jmemsys.h -- IJG libjpeg 6b jmemsys.h, Copyright (C) 1991-1998, Thomas G. Lane; licence in jpeglib.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/jpeg.md */
EXTERN(void *) jpeg_get_small JPP((j_common_ptr cinfo, size_t sizeofobject));
EXTERN(void) jpeg_free_small JPP((j_common_ptr cinfo, void * object,
				  size_t sizeofobject));

EXTERN(void FAR *) jpeg_get_large JPP((j_common_ptr cinfo,
				       size_t sizeofobject));
EXTERN(void) jpeg_free_large JPP((j_common_ptr cinfo, void FAR * object,
				  size_t sizeofobject));

#define MAX_ALLOC_CHUNK  1000000000L

EXTERN(long) jpeg_mem_available JPP((j_common_ptr cinfo,
				     long min_bytes_needed,
				     long max_bytes_needed,
				     long already_allocated));

#define TEMP_NAME_LENGTH   64

typedef struct backing_store_struct * backing_store_ptr;

typedef struct backing_store_struct {

  JMETHOD(void, read_backing_store, (j_common_ptr cinfo,
				     backing_store_ptr info,
				     void FAR * buffer_address,
				     long file_offset, long byte_count));
  JMETHOD(void, write_backing_store, (j_common_ptr cinfo,
				      backing_store_ptr info,
				      void FAR * buffer_address,
				      long file_offset, long byte_count));
  JMETHOD(void, close_backing_store, (j_common_ptr cinfo,
				      backing_store_ptr info));

  FILE * temp_file;
  char temp_name[TEMP_NAME_LENGTH];
} backing_store_info;

EXTERN(void) jpeg_open_backing_store JPP((j_common_ptr cinfo,
					  backing_store_ptr info,
					  long total_bytes_needed));

EXTERN(long) jpeg_mem_init JPP((j_common_ptr cinfo));
EXTERN(void) jpeg_mem_term JPP((j_common_ptr cinfo));
