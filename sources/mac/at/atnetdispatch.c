/* atnetdispatch.c -- 1.0.10: each socket call goes to its stack, Open Transport or else MacTCP for TCP/IP, Open Transport or else NBP, DDP and ADSP for AppleTalk. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "atnetmac.h"

#define kStateClosing   8
#define kNotOpen        -7109

#if !TARGET_API_MAC_CARBON

NetResolver *ResolverCreate(NetStackID stack, void *ref, NetProc proc, MutexList *doneList, MutexList *freeList)
{
    NetResolver *s;

    s = 0;
    switch (stack) {
    case kStackInet:
        s = gInetUsesOT ? ResolverCreateOT(stack, ref, proc, doneList, freeList) : ResolverCreateMacTCP(stack, ref, proc, doneList, freeList);
        break;
    case kStackAppleTalk:
        s = gAppleTalkUsesOT ? ResolverCreateOT(stack, ref, proc, doneList, freeList) : ResolverCreateNBP(stack, ref, proc, doneList, freeList);
        break;
    }
    return s;
}

void ResolverTickle(NetSocket *s)
{
    switch (s->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            ResolverTickleOT(s);
        else
            ResolverTickleMacTCP((struct DNRResolver *)s);
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            ResolverTickleOT(s);
        else
            ResolverTickleNBP((struct NBPResolver *)s);
        break;
    }
}

void ResolverDispose(void *ref)
{
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            ResolverDisposeOT(ref);
        else
            ResolverDisposeMacTCP(ref);
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            ResolverDisposeOT(ref);
        else
            ResolverDisposeNBP(ref);
        break;
    }
}

long ResolverOpen(void *ref)
{
    long err, e;

    err = 0;
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            e = ResolverOpenOT(ref);
        else
            e = ResolverOpenMacTCP(ref);
        if (e != 0)
            err = e;
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            e = ResolverOpenOT(ref);
        else
            e = ResolverOpenNBP(ref);
        if (e != 0)
            err = e;
        break;
    }
    return err;
}

long ResolverControl(void *ref, NetLookup *lookup)
{
    long err, e;

    err = 0;
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            e = ResolverControlOT(ref, lookup);
        else
            e = ResolverControlMacTCP(ref, lookup);
        if (e != 0)
            err = e;
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            e = ResolverControlOT(ref, lookup);
        else
            e = ResolverControlNBP(ref, lookup);
        if (e != 0)
            err = e;
        break;
    }
    return err;
}

void ResolverClose(void *ref)
{
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            ResolverCloseOT(ref);
        else
            ResolverCloseMacTCP(ref);
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            ResolverCloseOT(ref);
        else
            ResolverCloseNBP(ref);
        break;
    }
}

NetSocket *DatagramCreate(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2)
{
    NetSocket *s;

    s = 0;
    switch (stack) {
    case kStackInet:
        s = gInetUsesOT ? DatagramCreateOT(stack, ref, proc, sentList, freeList, recvList, recvList2) : DatagramCreateMacTCP(stack, ref, proc, sentList, freeList, recvList, recvList2);
        break;
    case kStackAppleTalk:
        s = gAppleTalkUsesOT ? DatagramCreateOT(stack, ref, proc, sentList, freeList, recvList, recvList2) : DatagramCreateDDP(stack, ref, proc, sentList, freeList, recvList, recvList2);
        break;
    }
    return s;
}

void DatagramTickle(NetSocket *s)
{
    switch (s->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            DatagramTickleOT(s);
        else
            DatagramTickleMacTCP(s);
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            DatagramTickleOT(s);
        else
            DatagramTickleDDP(s);
        break;
    }
}

void DatagramDispose(void *ref)
{
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            DatagramDisposeOT(ref);
        else
            DatagramDisposeMacTCP(ref);
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            DatagramDisposeOT(ref);
        else
            DatagramDisposeDDP(ref);
        break;
    }
}

long DatagramControl(void *ref, void *out, ULong option, ULong value)
{
    long err, e;

    err = 0;
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            e = DatagramControlOT(ref, out, option, value);
        else
            e = DatagramControlMacTCP(ref, out, option, value);
        if (e != 0)
            err = e;
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            e = DatagramControlOT(ref, out, option, value);
        else
            e = DatagramControlDDP(ref, out, option, value);
        if (e != 0)
            err = e;
        break;
    }
    return err;
}

long DatagramOpen(void *ref, NetAddress *address)
{
    long err, e;

    err = 0;
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            e = DatagramOpenOT(ref, address);
        else
            e = DatagramOpenMacTCP(ref, address);
        if (e != 0)
            err = e;
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            e = DatagramOpenOT(ref, address);
        else
            e = DatagramOpenDDP(ref, address);
        if (e != 0)
            err = e;
        break;
    }
    return err;
}

long DatagramSend(void *ref, NetPacket *packet)
{
    long err, e;

    err = 0;
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            e = DatagramSendOT(ref, packet);
        else
            e = DatagramSendMacTCP(ref, packet);
        if (e != 0)
            err = e;
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            e = DatagramSendOT(ref, packet);
        else
            e = DatagramSendDDP(ref, packet);
        if (e != 0)
            err = e;
        break;
    }
    return err;
}

void DatagramClose(void *ref, long unused)
{
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            DatagramCloseOT(ref, unused);
        else
            DatagramCloseMacTCP(ref, unused);
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            DatagramCloseOT(ref, unused);
        else
            DatagramCloseDDP(ref, unused);
        break;
    }
}

long DatagramGetAddress(void *ref, NetAddress *out)
{
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            DatagramGetAddressOT(ref, out);
        else
            DatagramGetAddressMacTCP(ref, out);
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            DatagramGetAddressOT(ref, out);
        else
            DatagramGetAddressDDP(ref, out);
        break;
    }
}

NetStream *StreamCreate(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2)
{
    NetStream *s;

    s = 0;
    switch (stack) {
    case kStackInet:
        s = gInetUsesOT ? StreamCreateOT(stack, ref, proc, sentList, freeList, recvList, recvList2) : StreamCreateMacTCP(stack, ref, proc, sentList, freeList, recvList, recvList2);
        break;
    case kStackAppleTalk:
        s = gAppleTalkUsesOT ? StreamCreateOT(stack, ref, proc, sentList, freeList, recvList, recvList2) : StreamCreateADSP(stack, ref, proc, sentList, freeList, recvList, recvList2);
        break;
    }
    return s;
}

void StreamTickle(NetStream *s)
{
    switch (s->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            StreamTickleOT((struct OTStream *)s);
        else
            StreamTickleMacTCP((struct TCPMacTCP *)s);
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            StreamTickleOT((struct OTStream *)s);
        else
            StreamTickleADSP(s);
        break;
    }
}

void StreamDispose(NetSocket *sock)
{
    switch ((sock)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            StreamDisposeOT(sock);
        else
            StreamDisposeMacTCP(sock);
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            StreamDisposeOT(sock);
        else
            StreamDisposeADSP(sock);
        break;
    }
}

long StreamAcceptServer(void *ref, void *other)
{
    long err, e;

    err = 0;
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            e = StreamAcceptServerOT(ref, other);
        else
            e = StreamAcceptServerMacTCP(ref, other);
        if (e != 0)
            err = e;
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            e = StreamAcceptServerOT(ref, other);
        else
            e = StreamAcceptServerADSP(ref, other);
        if (e != 0)
            err = e;
        break;
    }
    return err;
}

long StreamOpenClient(void *ref, NetAddress *local, const NetAddress *remote, ULong timeout)
{
    long err, e;

    err = 0;
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            e = StreamOpenClientOT(ref, local, remote, timeout);
        else
            e = StreamOpenClientMacTCP(ref, local, remote, timeout);
        if (e != 0)
            err = e;
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            e = StreamOpenClientOT(ref, local, remote, timeout);
        else
            e = StreamOpenClientADSP(ref, local, remote, timeout);
        if (e != 0)
            err = e;
        break;
    }
    return err;
}

long StreamSend(void *ref, NetChunk *chunk)
{
    long err, e;
    long state;

    err = 0;
    state = ((NetStream *)ref)->state;
    if (state == 0 || state == kStateClosing) {
        err = kNotOpen;
        goto done;
    }
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            e = StreamSendOT(ref, chunk);
        else
            e = StreamSendMacTCP(ref, chunk);
        if (e != 0)
            err = e;
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            e = StreamSendOT(ref, chunk);
        else
            e = StreamSendADSP(ref, chunk);
        if (e != 0)
            err = e;
        break;
    }
done:
    return err;
}

void StreamClose(NetSocket *sock, unsigned char graceful)
{
    switch ((sock)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            StreamCloseOT(sock, graceful);
        else
            StreamCloseMacTCP(sock, graceful);
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            StreamCloseOT(sock, graceful);
        else
            StreamCloseADSP(sock, graceful);
        break;
    }
}

long StreamGetAddress(void *ref, NetAddress *local, NetAddress *remote)
{
    switch (((NetSocket *)ref)->unk6) {
    case kStackInet:
        if (gInetUsesOT)
            StreamGetAddressOT(ref, local, remote);
        else
            StreamGetAddressMacTCP(ref, local, remote);
        break;
    case kStackAppleTalk:
        if (gAppleTalkUsesOT)
            StreamGetAddressOT(ref, local, remote);
        else
            StreamGetAddressADSP(ref, local, remote);
        break;
    }
}


#else /* Carbon */

NetResolver *ResolverCreate(NetStackID stack, void *ref, NetProc proc, MutexList *doneList, MutexList *freeList)
{
    return ResolverCreateOT(stack, ref, proc, doneList, freeList);
}

void ResolverTickle(NetSocket *s)
{
    ResolverTickleOT(s);
}

void ResolverDispose(void *ref)
{
    ResolverDisposeOT(ref);
}

long ResolverOpen(void *ref)
{
    long err, e;

    err = 0;
    e = ResolverOpenOT(ref);
    if (e != 0)
        err = e;
    return err;
}

long ResolverControl(void *ref, NetLookup *lookup)
{
    long err, e;

    err = 0;
    e = ResolverControlOT(ref, lookup);
    if (e != 0)
        err = e;
    return err;
}

void ResolverClose(void *ref)
{
    ResolverCloseOT(ref);
}

NetSocket *DatagramCreate(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2)
{
    return DatagramCreateOT(stack, ref, proc, sentList, freeList, recvList, recvList2);
}

void DatagramTickle(NetSocket *s)
{
    DatagramTickleOT(s);
}

void DatagramDispose(void *ref)
{
    DatagramDisposeOT(ref);
}

long DatagramControl(void *ref, void *out, ULong option, ULong value)
{
    long err, e;

    err = 0;
    e = DatagramControlOT(ref, out, option, value);
    if (e != 0)
        err = e;
    return err;
}

long DatagramOpen(void *ref, NetAddress *address)
{
    long err, e;

    err = 0;
    e = DatagramOpenOT(ref, address);
    if (e != 0)
        err = e;
    return err;
}

long DatagramSend(void *ref, NetPacket *packet)
{
    long err, e;

    err = 0;
    e = DatagramSendOT(ref, packet);
    if (e != 0)
        err = e;
    return err;
}

void DatagramClose(void *ref, long unused)
{
    DatagramCloseOT(ref, unused);
}

long DatagramGetAddress(void *ref, NetAddress *out)
{
    return DatagramGetAddressOT(ref, out);
}

NetStream *StreamCreate(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2)
{
    return StreamCreateOT(stack, ref, proc, sentList, freeList, recvList, recvList2);
}

void StreamTickle(NetStream *s)
{
    StreamTickleOT((struct OTStream *)s);
}

void StreamDispose(NetSocket *sock)
{
    StreamDisposeOT(sock);
}

long StreamAcceptServer(void *ref, void *other)
{
    long err, e;

    err = 0;
    e = StreamAcceptServerOT(ref, other);
    if (e != 0)
        err = e;
    return err;
}

long StreamOpenClient(void *ref, NetAddress *local, const NetAddress *remote, ULong timeout)
{
    long err, e;

    err = 0;
    e = StreamOpenClientOT(ref, local, remote, timeout);
    if (e != 0)
        err = e;
    return err;
}

long StreamSend(void *ref, NetChunk *chunk)
{
    long err, e;
    long state;

    err = 0;
    state = ((NetStream *)ref)->state;
    if (state == 0 || state == kStateClosing) {
        err = kNotOpen;
        goto done;
    }
    e = StreamSendOT(ref, chunk);
    if (e != 0)
        err = e;
done:
    return err;
}

void StreamClose(NetSocket *sock, unsigned char graceful)
{
    StreamCloseOT(sock, graceful);
}

long StreamGetAddress(void *ref, NetAddress *local, NetAddress *remote)
{
    return StreamGetAddressOT(ref, local, remote);
}

#endif
