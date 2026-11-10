"""The EV Nova build kit's build path: buildkit/build.py is the entry point.

Modules, in the order a build uses them:
  host       build.ini, paths, running a Windows exe under Wine, hashes
  toolchain  the expected size, CRC32 and SHA-256 of every tool, and `build.py check`
  coff, pe   the small object, archive and PE edits the Windows link needs
  win        EV Nova.exe 1.0.10 and the CE build: compile, archives, resources, link, post-steps
  mac1010    the mac 1.0.10 data fork: the Mac compiler plugin under Unicorn, MWLinkPPC, the fork
  mwppcemu   the plugin host
  mac111     the mac 1.1.1 Universal binary: Apple gcc 4.0.1, ld64-97.17, lipo
  umbrellas  which image object each sources/mac/tu umbrella builds
"""
