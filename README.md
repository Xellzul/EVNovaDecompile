# EvNovaDecompile

A matching decompilation of 
**EV Nova 1.0.10 for Windows x86**
**EV Nova 1.0.10 for Windows x86 Community Edition** (Not complete, not tested)
**EV Nova 1.0.10 for Mac**
**EV Nova 1.1.1 for Mac**

You need:
Provide files from input-files/README.md
Docker

Prepare build enviroment (takes ages):

`docker build --platform linux/amd64 --build-context inputs=input-files -t evnova-buildkit buildkit`

Run build:

`docker run --rm --platform linux/amd64 --security-opt seccomp=unconfined -v "$PWD":/repo evnova-buildkit   # = build.py all`

Outputs file to: buildkit/out/

Or targeted runs:

`docker run --rm --platform linux/amd64 --security-opt seccomp=unconfined -v "$PWD":/repo evnova-buildkit python3 /repo/buildkit/build.py {TARGET} {{--OPTIONS}}`

Target can be: all, win, ce, mac1010, mac111, check
Options can be: --twice, --no-pin-date, --unsigned, --config FILE

## Credits:
andrews05 for CE patches - https://github.com/andrews05/EV-Nova-CE 

## AI Use

Claude
- Opus 5
- Opus 5.5
- Sonnet 5.5
- Fable 5.2
- Fable 5.1
- Fable 5
- Sonnet 5
Helped by:
Claude Haiku 5.5, Claude Haiku 4.5, Alibaba Qwen 3.8 27B, Google Gemini 3.8 Flash, Google Gemini 3.7 Flash, Google Gemini 3.6 Flash and  Google Gemini 3.1 Pro
