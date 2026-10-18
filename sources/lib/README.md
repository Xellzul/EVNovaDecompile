# sources/lib — statically linked third-party code

Reserved, deliberately empty.

`EV Nova.exe` statically links BurgerLib, SpriteWorld and the Metrowerks
Standard Library. CLAUDE.md's standing instruction is to **compile those from
`vendor/` sources rather than decompile them**, so nothing belongs here yet.

Put a file here only when a third-party function has to be hand-transcribed
because no usable upstream source exists — and say why in the file header.

Two functions already in the tree belong here on that day: `SWSetPortToWindow`
@0x00477c70, currently in `../port/qdport.c` (SpriteWorld has a real body in
the mac 1.1.1 build, so it is not port code), and the BurgerLib memory-manager
family around 0x004d8xxx, which is not transcribed at all.
