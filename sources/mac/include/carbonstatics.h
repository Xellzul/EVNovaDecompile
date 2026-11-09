/* carbonstatics.h -- the Carbon event statics that Main Screen.o and EVEventLoop.o both define, in the order the image keeps them in __bss (gcc emits uninitialised statics in declaration order). Notes: Research/notes/eveventloop.md */
#ifndef CARBONSTATICS_H
#define CARBONSTATICS_H

#if V111
static IBNibRef sNibRef;
static AEEventHandlerUPP gOpenAppAEHandler;
static AEEventHandlerUPP gQuitAppAEHandler;
static EventHandlerUPP gAppEventHandler;
static EventHandlerUPP gPrefsWindowEventHandler;
static EventHandlerUPP gMainWindowEventHandler;
static EventHandlerUPP gKeyCaptureEventHandler;
static Boolean majorPrefsChange;
#endif

#endif
