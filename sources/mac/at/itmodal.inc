/* itmodal.inc -- 1.0.10: the toolkit's text at the end of its object on Mac: the frame hooks, the modal dialog's filter, the sound list's flush; ittoolkit.c includes it where the exe has them. Notes: Research/notes/maclibs.md */
#define kFrameInvert 2
#define kFilterBeep 7
#define kNoCursor -1

#pragma options align=mac68k
typedef struct ITSoundNode {
    struct ITSoundNode *next;
    unsigned char done;
    Handle sound;
    SndChannelPtr channel;
    SndCallBackUPP callback;
} ITSoundNode;
#pragma options align=reset

extern ITSoundNode *gIT_SoundList;
extern int32_t IT_HandleMacEvent(EventRecord *event, short edit, short *cursor);

pascal void IT_FrameRectHook(signed char verb, const Rect *r)
{
    CHECK_WINDOWS(0);
    if (verb != kFrameInvert)
        StdRect(verb, r);
}

pascal void IT_FrameRgnHook(signed char verb, RgnHandle rgn)
{
    CHECK_WINDOWS(0);
    if (verb != kFrameInvert)
        StdRgn(verb, rgn);
}

pascal Boolean IT_ViewModalFilter(DialogPtr dialog, EventRecord *event, short *item)
{
    Boolean handled;
    WindowNode *view;
    WindowPtr window;
    WindowPtr hit;
    short cursor;

    handled = 0;
    hit = 0;
    window = GetDialogWindow(dialog);
    view = IT_WindowFindView(window);
    if (view != 0) {
        if (event->what == mouseDown) {
            FindWindow(event->where, &hit);
            if (hit != window)
                SysBeep(kFilterBeep);
        }
        cursor = kNoCursor;
        IT_HandleMacEvent(event, 0, &cursor);
        if (cursor != kNoCursor)
            IT_SetCursor(cursor);
        if (!IT_ViewExists(view))
            *item = 1;
        handled = 1;
    }
    return handled;
}

void _IT_SoundFlush(unsigned char wait)
{
    ITSoundNode *node;
    ITSoundNode **link;

    link = &gIT_SoundList;
    while ((node = *link) != 0) {
        while (!node->done && wait)
            SystemIdle();
        if (node->done) {
            if (node->channel != 0) {
                SndDisposeChannel(node->channel, true);
                node->channel = 0;
            }
            if (node->callback != 0) {
                DisposeSndCallBackUPP(node->callback);
                node->callback = 0;
            }
            if (node->sound != 0) {
                DisposeHandle(node->sound);
                node->sound = 0;
            }
            *link = (*link)->next;
            MemoryDeallocate(node);
            link = 0;
        }
        if (link != 0)
            link = &(*link)->next;
        else
            link = &gIT_SoundList;
    }
}
