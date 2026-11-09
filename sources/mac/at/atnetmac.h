/* atnetmac.h -- 1.0.10: the network layer's stacks on Mac, Open Transport, MacTCP and AppleTalk. Notes: Research/notes/maclibs.md */
#ifndef ATNETMAC_H
#define ATNETMAC_H

#define kStackInet      0
#define kStackAppleTalk 1

extern unsigned char gInetUsesOT;
extern unsigned char gAppleTalkUsesOT;

extern void AddressComposeAppleTalk(NetAddress *a, unsigned short net, unsigned char node, unsigned char socket, unsigned char type);
extern void AddressDecomposeAppleTalk(const NetAddress *a, unsigned short *net, unsigned char *node, unsigned char *socket, unsigned char *type);
extern void _SystemUnpatchTraps(void);
extern long StackOpenOT(ULong wanted);
extern void StackCloseOT(void);
extern void StackTickleOT(void);
extern unsigned char StackGetAvailOT(unsigned short mode);
extern ULong StackGetAddressListOT(unsigned short kind, NetAddress *out, ULong max);
extern unsigned char InetAvailableOT(void);
extern unsigned char InetInitOT(void);
extern unsigned char InetActiveOT(void);
extern unsigned char AppleTalkAvailableOT(void);
extern unsigned char AppleTalkInitOT(void);
extern unsigned char AppleTalkActiveOT(void);
extern short gMacTCPRefNum;
extern long StackOpenMacTCP(void);
extern void StackCloseMacTCP(void);
extern void StackTickleMacTCP(void);
extern unsigned char StackGetAvailMacTCP(unsigned short mode);
extern ULong StackGetAddressListMacTCP(unsigned short kind, NetAddress *out, ULong max);
extern long StackOpenAppleTalk(void);
extern void StackCloseAppleTalk(void);
extern void StackTickleAppleTalk(void);
extern unsigned char StackGetAvailAppleTalk(unsigned short mode);
extern ULong StackGetAddressListAppleTalk(unsigned short kind, NetAddress *out, ULong max);

extern NetResolver *ResolverCreateOT(NetStackID stack, void *ref, NetProc proc, MutexList *doneList, MutexList *freeList);
extern NetResolver *ResolverCreateMacTCP(NetStackID stack, void *ref, NetProc proc, MutexList *doneList, MutexList *freeList);
extern NetResolver *ResolverCreateNBP(NetStackID stack, void *ref, NetProc proc, MutexList *doneList, MutexList *freeList);
extern void ResolverTickleOT(NetSocket *s);
extern void ResolverTickleMacTCP(struct DNRResolver *r);
extern void ResolverTickleNBP(struct NBPResolver *r);
extern void ResolverDisposeOT(struct OTResolver *r);
extern void ResolverDisposeMacTCP(struct DNRResolver *r);
extern void ResolverDisposeNBP(struct NBPResolver *r);
extern long ResolverOpenOT(void *ref);
extern long ResolverOpenMacTCP(struct DNRResolver *r);
extern long ResolverOpenNBP(struct NBPResolver *r);
extern long ResolverControlOT(void *ref, NetLookup *lookup);
extern long ResolverControlMacTCP(void *ref, NetLookup *lookup);
extern long ResolverControlNBP(void *ref, NetLookup *lookup);
extern void ResolverCloseOT(void *ref);
extern void ResolverCloseMacTCP(struct DNRResolver *r);
extern void ResolverCloseNBP(struct NBPResolver *r);
extern NetSocket *DatagramCreateOT(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2);
extern NetSocket *DatagramCreateMacTCP(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2);
extern NetSocket *DatagramCreateDDP(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2);
extern void DatagramTickleOT(NetSocket *ref);
extern void DatagramTickleMacTCP(NetSocket *ref);
extern void DatagramTickleDDP(NetSocket *ref);
extern void DatagramDisposeOT(struct OTSocket *s);
extern void DatagramDisposeMacTCP(struct UDPMacTCP *s);
extern void DatagramDisposeDDP(struct DDPSocket *s);
extern long DatagramControlOT(void *ref, void *out, ULong option, ULong value);
extern long DatagramControlMacTCP(struct UDPMacTCP *s, void *out, ULong option, ULong value);
extern long DatagramControlDDP(struct DDPSocket *s, void *out, ULong option, ULong value);
extern long DatagramOpenOT(void *ref, NetAddress *address);
extern long DatagramOpenMacTCP(void *ref, NetAddress *address);
extern long DatagramOpenDDP(void *ref, NetAddress *address);
extern long DatagramSendOT(struct OTSocket *s, NetPacket *packet);
extern long DatagramSendMacTCP(void *ref, NetPacket *packet);
extern long DatagramSendDDP(void *ref, NetPacket *packet);
extern void DatagramCloseOT(void *ref, long graceful);
extern void DatagramCloseMacTCP(struct UDPMacTCP *s, long graceful);
extern void DatagramCloseDDP(struct DDPSocket *s, long graceful);
extern long DatagramGetAddressOT(struct OTSocket *s, NetAddress *out);
extern long DatagramGetAddressMacTCP(void *ref, NetAddress *out);
extern long DatagramGetAddressDDP(struct DDPSocket *s, NetAddress *out);
extern NetStream *StreamCreateOT(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2);
extern NetStream *StreamCreateMacTCP(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2);
extern NetStream *StreamCreateADSP(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2);
extern void StreamTickleOT(struct OTStream *s);
extern void StreamTickleMacTCP(struct TCPMacTCP *s);
extern void StreamTickleADSP(NetStream *s);
extern void StreamDisposeOT(NetSocket *sock);
extern void StreamDisposeMacTCP(NetSocket *sock);
extern void StreamDisposeADSP(NetSocket *sock);
extern long StreamAcceptServerOT(struct OTStream *s, void *other);
extern long StreamAcceptServerMacTCP(void *ref, void *other);
extern long StreamAcceptServerADSP(void *ref, void *other);
extern long StreamOpenClientOT(void *ref, NetAddress *local, const NetAddress *remote, ULong timeout);
extern long StreamOpenClientMacTCP(struct TCPMacTCP *s, NetAddress *local, const NetAddress *remote, ULong timeout);
extern long StreamOpenClientADSP(void *ref, NetAddress *local, const NetAddress *remote, ULong timeout);
extern long StreamSendOT(struct OTStream *s, NetChunk *chunk);
extern long StreamSendMacTCP(void *ref, NetChunk *chunk);
extern long StreamSendADSP(void *ref, NetChunk *chunk);
extern void StreamCloseOT(NetSocket *sock, unsigned char graceful);
extern void StreamCloseMacTCP(NetSocket *sock, unsigned char graceful);
extern void StreamCloseADSP(NetSocket *sock, unsigned char graceful);
extern long StreamGetAddressOT(struct OTStream *s, NetAddress *local, NetAddress *remote);
extern long StreamGetAddressMacTCP(struct TCPMacTCP *s, NetAddress *local, NetAddress *remote);
extern long StreamGetAddressADSP(void *ref, NetAddress *local, NetAddress *remote);

#endif
