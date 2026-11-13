/* ittoolkit.h -- the interface tool's window part (ittoolkit.c) and control-list part
   (ittoolkit3.c): the calls each makes into the other; also the calls the markup and border
   parts (itmarkup*.c, itborder.c) make into atcharset.c, itmarkup.c and itgfx.c.
   Notes: Research/notes/ittoolkit.md */
#ifndef ITTOOLKIT_H
#define ITTOOLKIT_H

#include "burgertypes.h"
#include "burgerplat.h"
#include "ambrosia.h"

extern int32_t FT_PathCreate(struct FolderSpec **where, const char *name,
                         struct FolderSpec ***result);                   /* atfilespec.c */
extern int32_t PT_ParseTreeLoad(void **text, const char *rawTags, const char *emptyTags,
                         void **errors, ITElement **out);                /* itmarkup.c */
extern unsigned char *U2CStringCopySafe(unsigned char *dst, const unsigned short *src,
                                   ULong dstBytes, ULong maxChars);      /* atcharset.c */
extern unsigned short *UTF32_2UStringCopySafe(unsigned short *dst, const ULong *src, ULong dstBytes,
                                    int32_t maxChars);                      /* atcharset.c */
extern int32_t DT_DrawTextAt(ITImage *image, int32_t x, int32_t y, const char *text,
                         int32_t length);                                   /* itgfx.c */

int32_t IT_ViewGoToPage(void *windowRef, const char *name);
unsigned char _IT_ViewValidateFocus(WindowNode *window, int32_t self);

ITControlList *_IT_ItemListCreate(WindowNode *window, ITElement *first);
void _IT_ItemListDispose(WindowNode *window, ITControlList *list);
int32_t _IT_GetItemIndex(ITControlList *list, ITControl *control);
int32_t _IT_GetObjectItem(WindowNode *window, ITControlList *list, ULong item, WindowNode **ownerOut,
                  ITControl **controlOut);
int32_t _IT_ItemListQueryFocus(WindowNode *window, ITControlList *list, void **items);
int32_t _IT_ItemListHandleIdle(WindowNode *window, ITControlList *list);
int32_t _IT_ItemListHandleKey(WindowNode *window, ITControlList *list, const ITKeyEvent *key,
                  ITClick *click);
int32_t _IT_ItemListHandleEdit(WindowNode *window, ITControlList *list, short event, ITClick *click);
int32_t _IT_ItemListHandleClick(WindowNode *window, ITControlList *list, ULong event,
                  ITClick *click);
int32_t _IT_ItemListHandleTest(WindowNode *window, ITControlList *list, ULong item, unsigned char *hit);
int32_t _IT_ItemListHandleDraw(WindowNode *window, ITControlList *list, const ITRect *area);
int32_t _IT_ItemListHandleActivate(WindowNode *window, ITControlList *list, unsigned char active);
int32_t _IT_ItemListHandleCursor(WindowNode *window, ITControlList *list, ITInputEvent *event,
                  unsigned short *modifiers);
int32_t _IT_ItemExecute(WindowNode *window, ITControl *control, const char *link);
int32_t _IT_GetItemColor(WindowNode *window, ITControlList *list, ULong item, void *a, void *b,
                  unsigned char *hit);
int32_t IT_ViewGetItemCount(void *windowRef);

#endif
