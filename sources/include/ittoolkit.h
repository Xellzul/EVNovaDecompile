/* ittoolkit.h -- the interface tool's window part (ittoolkit.c) and control-list part
   (ittoolkit3.c): the calls each makes into the other; also the calls the markup and border
   parts (itmarkup*.c, itborder.c) make into atcharset.c, itmarkup.c and itgfx.c.
   Notes: Research/notes/ittoolkit.md */
#ifndef ITTOOLKIT_H
#define ITTOOLKIT_H

#include "burgertypes.h"
#include "burgerplat.h"
#include "ambrosia.h"

extern long FT_PathCreate(struct FolderSpec **where, const char *name,
                         struct FolderSpec ***result);                   /* atfilespec.c */
extern long PT_ParseTreeLoad(void **text, const char *rawTags, const char *emptyTags,
                         void **errors, ITElement **out);                /* itmarkup.c */
extern ULong _PT_ParseAdvance(const unsigned char *s, ULong *ch, ULong *code);  /* itmarkup.c */
extern unsigned char *U2CStringCopySafe(unsigned char *dst, const unsigned short *src,
                                   ULong dstBytes, ULong maxChars);      /* atcharset.c */
extern unsigned short *UTF32_2UStringCopySafe(unsigned short *dst, const ULong *src, ULong dstBytes,
                                    long maxChars);                      /* atcharset.c */
extern long DT_DrawTextAt(ITImage *image, long x, long y, const char *text,
                         long length);                                   /* itgfx.c */

long IT_ViewGoToPage(WindowNode *window, const char *name);
unsigned char _IT_ViewValidateFocus(WindowNode *window, long self);

ITControlList *_IT_ItemListCreate(WindowNode *window, ITElement *first);
void _IT_ItemListDispose(WindowNode *window, ITControlList *list);
long _IT_GetItemIndex(ITControlList *list, ITControl *control);
long _IT_GetObjectItem(WindowNode *window, ITControlList *list, long item, WindowNode **ownerOut,
                  ITControl **controlOut);
long _IT_ItemListQueryFocus(WindowNode *window, ITControlList *list, void **items);
long _IT_ItemListHandleIdle(WindowNode *window, ITControlList *list);
long _IT_ItemListHandleKey(WindowNode *window, ITControlList *list, const ITKeyEvent *key,
                  ITClick *click);
long _IT_ItemListHandleEdit(WindowNode *window, ITControlList *list, short event, ITClick *click);
long _IT_ItemListHandleClick(WindowNode *window, ITControlList *list, ITInputEvent *event,
                  ITClick *click);
long _IT_ItemListHandleTest(WindowNode *window, ITControlList *list, ULong item, unsigned char *hit);
long _IT_ItemListHandleDraw(WindowNode *window, ITControlList *list, const ITRect *area);
long _IT_ItemListHandleActivate(WindowNode *window, ITControlList *list, unsigned char active);
long _IT_ItemListHandleCursor(WindowNode *window, ITControlList *list, ITInputEvent *event,
                  unsigned short *modifiers);
long _IT_ItemExecute(WindowNode *window, ITControl *control, const char *link);
long _IT_GetItemColor(WindowNode *window, ITControlList *list, ULong item, void *a, void *b,
                  unsigned char *hit);
long IT_ViewGetItemCount(void *windowRef);

#endif
