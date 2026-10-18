/* lsp.h -- libvorbis 1.0rc3 lib/lsp.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. altered: comments removed. Notes: Research/notes/burgervorbis.md */
#ifndef _V_LSP_H_
#define _V_LSP_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int vorbis_lpc_to_lsp(float *lpc,float *lsp,int m);

extern void vorbis_lsp_to_curve(float *curve,int *map,int n,int ln,
				float *lsp,int m,
				float amp,float ampoffset);

#ifdef __cplusplus
}
#endif

#endif
