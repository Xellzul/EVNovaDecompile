/* jpegerrmgr.h -- the error manager the interface tool hands libjpeg 6b (example.c's
   my_error_mgr): jerror.c's error_exit longjmps to it, the JPEG loader in itjpeg.c sets it.
   Types only. Notes: Research/notes/jpeg.md */
#ifndef JPEGERRMGR_H
#define JPEGERRMGR_H

#include <setjmp.h>

struct my_error_mgr {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr * my_error_ptr;

#endif
