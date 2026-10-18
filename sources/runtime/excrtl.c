// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
// COMPILE-WHY: MSL library code (excrtl.obj of MSL_All_x86.lib): MSL's own line (Research/msl-version.md) is this one plus -lang c99 and the avoid_ebp prefix, both object-inert here
/* excrtl.c -- MSL's x86 exception runtime: the structured-exception unwind handler. Notes: Research/notes/excrtl.md */

#define kUnwindFlags            6                /* EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND */
#define kContinueSearch         1                /* ExceptionContinueSearch */
#define kCollidedUnwind         3                /* ExceptionCollidedUnwind */

typedef struct ExceptionRecord {
    unsigned long code;                          /* +0x00 */
    unsigned long flags;                         /* +0x04 */
} ExceptionRecord;

/* __SEHUnwindHandler (MSL excrtl, `?__SEHUnwindHandler@@YA..' there). STATE: MATCH, exe 10 /
   obj 10. OPEN: none */
// FUNCTION: EVNOVA 0x00534550
long SEHUnwindHandler(ExceptionRecord *record, void *frame, void *context, void **dispatcher)
{
    if ((record->flags & kUnwindFlags) == 0)
        return kContinueSearch;
    *dispatcher = frame;
    return kCollidedUnwind;
}
