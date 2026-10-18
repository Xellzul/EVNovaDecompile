# EvNovaDecompile

A matching decompilation of **EV Nova 1.0.10 for Windows x86** (`EV Nova.exe` - SHA-256: 43e8db386caa0fd32d48e8b5a45b81c894063870a0b52e95ef89a1c6471a248c (1,703,968 bytes, FileVersion 1.0.10.0), Ambrosia Software / ATMOS; Windows port by Contraband Entertainment, 2003).

There is still a lot of Claudism texts, Compiler mangled code (`((*(unsigned long long *)((char *)shipData + 0x4b7a0)`), unnamed variables / data, misnamed variables data. however it is possible to recreate origin exe byte-for-byte.

The C/C++ source in `sources/` recompiles, function by function, to instruction-identical and byte-identical code with the original Metrowerks CodeWarrior toolchain (`mwcc` 3.2.4 build 418 for game code). 

YOU CANNOT PLAY THIS, YOU CANNOT LINK IT. There is NO toolchain provided - yet.

You need: `mwcc.exe` / `mwld.exe` 3.2.4 build 418 for game
build 335 for libraries (zlib, libpng, libjpeg, AmbrosiaTools, REGGIE, RT3)

## AI Use

Claude
- Opus 5
- Opus 5.5
- Sonnet 5.5
- Fable 5.1
- Fable 5
- Sonnet 5
Helped by:
Claude Haiku 5.5, Claude Haiku 4.5, Alibaba Qwen 3.8 27B, Google Gemini 3.8 Flash, Google Gemini 3.7 Flash, Google Gemini 3.6 Flash and  Google Gemini 3.1 Pro