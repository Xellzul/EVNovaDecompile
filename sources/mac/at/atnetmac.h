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
extern int32_t StackOpenOT(ULong wanted);
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
extern int32_t StackOpenMacTCP(void);
extern void StackCloseMacTCP(void);
extern void StackTickleMacTCP(void);
extern unsigned char StackGetAvailMacTCP(unsigned short mode);
extern ULong StackGetAddressListMacTCP(unsigned short kind, NetAddress *out, ULong max);
extern int32_t StackOpenAppleTalk(void);
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
extern int32_t ResolverOpenOT(void *ref);
extern int32_t ResolverOpenMacTCP(struct DNRResolver *r);
extern int32_t ResolverOpenNBP(struct NBPResolver *r);
extern int32_t ResolverControlOT(void *ref, NetLookup *lookup);
extern int32_t ResolverControlMacTCP(void *ref, NetLookup *lookup);
extern int32_t ResolverControlNBP(void *ref, NetLookup *lookup);
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
extern int32_t DatagramControlOT(void *ref, void *out, ULong option, ULong value);
extern int32_t DatagramControlMacTCP(struct UDPMacTCP *s, void *out, ULong option, ULong value);
extern int32_t DatagramControlDDP(struct DDPSocket *s, void *out, ULong option, ULong value);
extern int32_t DatagramOpenOT(void *ref, NetAddress *address);
extern int32_t DatagramOpenMacTCP(void *ref, NetAddress *address);
extern int32_t DatagramOpenDDP(void *ref, NetAddress *address);
extern int32_t DatagramSendOT(struct OTSocket *s, NetPacket *packet);
extern int32_t DatagramSendMacTCP(void *ref, NetPacket *packet);
extern int32_t DatagramSendDDP(void *ref, NetPacket *packet);
extern void DatagramCloseOT(void *ref, int32_t graceful);
extern void DatagramCloseMacTCP(struct UDPMacTCP *s, int32_t graceful);
extern void DatagramCloseDDP(struct DDPSocket *s, int32_t graceful);
extern int32_t DatagramGetAddressOT(struct OTSocket *s, NetAddress *out);
extern int32_t DatagramGetAddressMacTCP(void *ref, NetAddress *out);
extern int32_t DatagramGetAddressDDP(struct DDPSocket *s, NetAddress *out);
extern NetStream *StreamCreateOT(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2);
extern NetStream *StreamCreateMacTCP(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2);
extern NetStream *StreamCreateADSP(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2);
extern void StreamTickleOT(struct OTStream *s);
extern void StreamTickleMacTCP(struct TCPMacTCP *s);
extern void StreamTickleADSP(NetStream *s);
extern void StreamDisposeOT(NetSocket *sock);
extern void StreamDisposeMacTCP(NetSocket *sock);
extern void StreamDisposeADSP(NetSocket *sock);
extern int32_t StreamAcceptServerOT(struct OTStream *s, void *other);
extern int32_t StreamAcceptServerMacTCP(void *ref, void *other);
extern int32_t StreamAcceptServerADSP(void *ref, void *other);
extern int32_t StreamOpenClientOT(void *ref, NetAddress *local, const NetAddress *remote, ULong timeout);
extern int32_t StreamOpenClientMacTCP(struct TCPMacTCP *s, NetAddress *local, const NetAddress *remote, ULong timeout);
extern int32_t StreamOpenClientADSP(void *ref, NetAddress *local, const NetAddress *remote, ULong timeout);
extern int32_t StreamSendOT(struct OTStream *s, NetChunk *chunk);
extern int32_t StreamSendMacTCP(void *ref, NetChunk *chunk);
extern int32_t StreamSendADSP(void *ref, NetChunk *chunk);
extern void StreamCloseOT(NetSocket *sock, unsigned char graceful);
extern void StreamCloseMacTCP(NetSocket *sock, unsigned char graceful);
extern void StreamCloseADSP(NetSocket *sock, unsigned char graceful);
extern int32_t StreamGetAddressOT(struct OTStream *s, NetAddress *local, NetAddress *remote);
extern int32_t StreamGetAddressMacTCP(struct TCPMacTCP *s, NetAddress *local, NetAddress *remote);
extern int32_t StreamGetAddressADSP(void *ref, NetAddress *local, NetAddress *remote);

#endif
