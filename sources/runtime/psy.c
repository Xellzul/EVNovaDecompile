// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; dropping -inline auto loses MATCHes here; inlined: min_curve, max_curve, attenuate_curve, interp_curve and couple_point); Research/notes/burgervorbis.md
/* psy.c -- libvorbis 1.0rc3 lib/psy.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (seed_chase's and _vp_compute_mask's scratch from the heap instead of alloca, FastMemSet); altered: comments and the functions the exe does not link removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "codec.h"
#include "codec_internal.h"

#include "masking.h"
#include "psy.h"
#include "os.h"
#include "lpc.h"
#include "smallft.h"
#include "scales.h"
#include "misc.h"

#define NEGINF -9999.f

/* _vp_global_free. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005477b0
void _vp_global_free(vorbis_look_psy_global *look){
  if(look){
    FastMemSet(look,0,sizeof(*look));
    _ogg_free(look);
  }
}

/* _vi_psy_free. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005477f0
void _vi_psy_free(vorbis_info_psy *i){
  if(i){
    FastMemSet(i,0,sizeof(*i));
    _ogg_free(i);
  }
}

/* set_curve. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00547830
static void set_curve(float *ref,float *c,int n, float crate){
  int i,j=0;

  for(i=0;i<MAX_BARK-1;i++){
    int endpos=rint(fromBARK((float)(i+1))*2*n/crate);
    float base=ref[i];
    if(j<endpos){
      float delta=(ref[i+1]-base)/(endpos-j);
      for(;j<endpos && j<n;j++){
	c[j]=base;
	base+=delta;
      }
    }
  }
}

static void min_curve(float *c,
		       float *c2){
  int i;
  for(i=0;i<EHMER_MAX;i++)if(c2[i]<c[i])c[i]=c2[i];
}
static void max_curve(float *c,
		       float *c2){
  int i;
  for(i=0;i<EHMER_MAX;i++)if(c2[i]>c[i])c[i]=c2[i];
}

static void attenuate_curve(float *c,float att){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]+=att;
}

static void interp_curve(float *c,float *c1,float *c2,float del){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]=c2[i]*del+c1[i]*(1.f-del);
}

extern int analysis_noisy;
/* setup_curve. STATE: BurgerLib's libvorbis 1.0rc3, MATCH, byte-identical (1896 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00547a10
static void setup_curve(float **c,
			int band,
			float *curveatt_dB){
  int i,j;
  float ath[EHMER_MAX];
  float tempc[P_LEVELS][EHMER_MAX];
  float *ATH=ATH_Bark_dB_lspconservative;

  memcpy(c[0]+2,c[4]+2,sizeof(*c[0])*EHMER_MAX);
  memcpy(c[2]+2,c[4]+2,sizeof(*c[2])*EHMER_MAX);

  for(i=0;i<EHMER_MAX;i++){
    float oc_min=band*.5f+(i-EHMER_OFFSET)*.125f;
    float oc_max=band*.5f+(i-EHMER_OFFSET+1)*.125f;
    float bark=toBARK(fromOC(oc_min));
    int ibark=floor(bark);
    float del=bark-ibark;
    float ath_min,ath_max;

    if(ibark<26)
      ath_min=ATH[ibark]*(1.f-del)+ATH[ibark+1]*del;
    else
      ath_min=ATH[25];

    bark=toBARK(fromOC(oc_max));
    ibark=floor(bark);
    del=bark-ibark;

    if(ibark<26)
      ath_max=ATH[ibark]*(1.f-del)+ATH[ibark+1]*del;
    else
      ath_max=ATH[25];

    if(ath_min>ath_max)ath_min=ath_max;
    ath[i]=ath_min;
  }

  for(i=1;i<P_LEVELS;i+=2){
    interp_curve(c[i]+2,c[i-1]+2,c[i+1]+2,.5);
  }

  for(i=0;i<P_LEVELS;i++){
    attenuate_curve(c[i]+2,curveatt_dB[i]);
    memcpy(tempc[i],ath,EHMER_MAX*sizeof(*tempc[i]));
    attenuate_curve(tempc[i],-i*10.f);
    max_curve(tempc[i],c[i]+2);
  }

  for(j=1;j<P_LEVELS;j++){
    min_curve(tempc[j],tempc[j-1]);
    min_curve(c[j]+2,tempc[j]);
  }

  for(j=0;j<P_LEVELS;j++){

    for(i=0;i<EHMER_OFFSET;i++)
      if(c[j][i+2]>-200.f)break;
    c[j][0]=i;

    for(i=EHMER_MAX-1;i>EHMER_OFFSET+1;i--)
      if(c[j][i+2]>-200.f)
	break;
    c[j][1]=i;
  }
}

/* _vp_psy_init. STATE: BurgerLib's libvorbis 1.0rc3, MATCH, byte-identical (10398 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00548180
void _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,
		  vorbis_info_psy_global *gi,int n,long rate){
  long i,j,k,lo=-99,hi=0;
  long maxoc;
  FastMemSet(p,0,sizeof(*p));

  p->eighth_octave_lines=gi->eighth_octave_lines;
  p->shiftoc=(int)rint(log(gi->eighth_octave_lines*8.f)/log(2.f))-1;

  p->firstoc=toOC(.25f*rate/n)*(1<<(p->shiftoc+1))-gi->eighth_octave_lines;
  maxoc=toOC((n*.5f-.25f)*rate/n)*(1<<(p->shiftoc+1))+.5f;
  p->total_octave_lines=maxoc-p->firstoc+1;

  if(vi->ath)
    p->ath=(float *)_ogg_malloc(n*sizeof(*p->ath));
  p->octave=(long *)_ogg_malloc(n*sizeof(*p->octave));
  p->bark=(long *)_ogg_malloc(n*sizeof(*p->bark));
  p->vi=vi;
  p->n=n;
  p->rate=rate;

  if(vi->ath)
    set_curve(vi->ath, p->ath,n,rate);
  for(i=0;i<n;i++){
    float bark=toBARK(rate/(2*n)*i);

    for(;lo+vi->noisewindowlomin<i &&
	  toBARK(rate/(2*n)*lo)<(bark-vi->noisewindowlo);lo++);

    for(;hi<n && (hi<i+vi->noisewindowhimin ||
	  toBARK(rate/(2*n)*hi)<(bark+vi->noisewindowhi));hi++);

    p->bark[i]=(lo<<16)+hi;
  }

  for(i=0;i<n;i++)
    p->octave[i]=toOC((i*.5f+.25f)*rate/n)*(1<<(p->shiftoc+1))+.5f;

  p->tonecurves=(float ***)_ogg_malloc(P_BANDS*sizeof(*p->tonecurves));
  p->noisethresh=(float *)_ogg_malloc(n*sizeof(*p->noisethresh));
  p->noiseoffset=(float *)_ogg_malloc(n*sizeof(*p->noiseoffset));
  for(i=0;i<P_BANDS;i++)
    p->tonecurves[i]=(float **)_ogg_malloc(P_LEVELS*sizeof(*p->tonecurves[i]));

  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++)
      p->tonecurves[i][j]=(float *)_ogg_malloc((EHMER_MAX+2)*sizeof(*p->tonecurves[i][j]));

  memcpy(p->tonecurves[0][4]+2,tone_125_40dB_SL,sizeof(*p->tonecurves[0][4])*EHMER_MAX);
  memcpy(p->tonecurves[0][6]+2,tone_125_60dB_SL,sizeof(*p->tonecurves[0][6])*EHMER_MAX);
  memcpy(p->tonecurves[0][8]+2,tone_125_80dB_SL,sizeof(*p->tonecurves[0][8])*EHMER_MAX);
  memcpy(p->tonecurves[0][10]+2,tone_125_100dB_SL,sizeof(*p->tonecurves[0][10])*EHMER_MAX);

  memcpy(p->tonecurves[2][4]+2,tone_125_40dB_SL,sizeof(*p->tonecurves[2][4])*EHMER_MAX);
  memcpy(p->tonecurves[2][6]+2,tone_125_60dB_SL,sizeof(*p->tonecurves[2][6])*EHMER_MAX);
  memcpy(p->tonecurves[2][8]+2,tone_125_80dB_SL,sizeof(*p->tonecurves[2][8])*EHMER_MAX);
  memcpy(p->tonecurves[2][10]+2,tone_125_100dB_SL,sizeof(*p->tonecurves[2][10])*EHMER_MAX);

  memcpy(p->tonecurves[4][4]+2,tone_250_40dB_SL,sizeof(*p->tonecurves[4][4])*EHMER_MAX);
  memcpy(p->tonecurves[4][6]+2,tone_250_60dB_SL,sizeof(*p->tonecurves[4][6])*EHMER_MAX);
  memcpy(p->tonecurves[4][8]+2,tone_250_80dB_SL,sizeof(*p->tonecurves[4][8])*EHMER_MAX);
  memcpy(p->tonecurves[4][10]+2,tone_250_100dB_SL,sizeof(*p->tonecurves[4][10])*EHMER_MAX);

  memcpy(p->tonecurves[6][4]+2,tone_500_40dB_SL,sizeof(*p->tonecurves[6][4])*EHMER_MAX);
  memcpy(p->tonecurves[6][6]+2,tone_500_60dB_SL,sizeof(*p->tonecurves[6][6])*EHMER_MAX);
  memcpy(p->tonecurves[6][8]+2,tone_500_80dB_SL,sizeof(*p->tonecurves[6][8])*EHMER_MAX);
  memcpy(p->tonecurves[6][10]+2,tone_500_100dB_SL,sizeof(*p->tonecurves[6][10])*EHMER_MAX);

  memcpy(p->tonecurves[8][4]+2,tone_1000_40dB_SL,sizeof(*p->tonecurves[8][4])*EHMER_MAX);
  memcpy(p->tonecurves[8][6]+2,tone_1000_60dB_SL,sizeof(*p->tonecurves[8][6])*EHMER_MAX);
  memcpy(p->tonecurves[8][8]+2,tone_1000_80dB_SL,sizeof(*p->tonecurves[8][8])*EHMER_MAX);
  memcpy(p->tonecurves[8][10]+2,tone_1000_100dB_SL,sizeof(*p->tonecurves[8][10])*EHMER_MAX);

  memcpy(p->tonecurves[10][4]+2,tone_2000_40dB_SL,sizeof(*p->tonecurves[10][4])*EHMER_MAX);
  memcpy(p->tonecurves[10][6]+2,tone_2000_60dB_SL,sizeof(*p->tonecurves[10][6])*EHMER_MAX);
  memcpy(p->tonecurves[10][8]+2,tone_2000_80dB_SL,sizeof(*p->tonecurves[10][8])*EHMER_MAX);
  memcpy(p->tonecurves[10][10]+2,tone_2000_100dB_SL,sizeof(*p->tonecurves[10][10])*EHMER_MAX);

  memcpy(p->tonecurves[12][4]+2,tone_4000_40dB_SL,sizeof(*p->tonecurves[12][4])*EHMER_MAX);
  memcpy(p->tonecurves[12][6]+2,tone_4000_60dB_SL,sizeof(*p->tonecurves[12][6])*EHMER_MAX);
  memcpy(p->tonecurves[12][8]+2,tone_4000_80dB_SL,sizeof(*p->tonecurves[12][8])*EHMER_MAX);
  memcpy(p->tonecurves[12][10]+2,tone_4000_100dB_SL,sizeof(*p->tonecurves[12][10])*EHMER_MAX);

  memcpy(p->tonecurves[14][4]+2,tone_8000_40dB_SL,sizeof(*p->tonecurves[14][4])*EHMER_MAX);
  memcpy(p->tonecurves[14][6]+2,tone_8000_60dB_SL,sizeof(*p->tonecurves[14][6])*EHMER_MAX);
  memcpy(p->tonecurves[14][8]+2,tone_8000_80dB_SL,sizeof(*p->tonecurves[14][8])*EHMER_MAX);
  memcpy(p->tonecurves[14][10]+2,tone_8000_100dB_SL,sizeof(*p->tonecurves[14][10])*EHMER_MAX);

  memcpy(p->tonecurves[16][4]+2,tone_16000_40dB_SL,sizeof(*p->tonecurves[16][4])*EHMER_MAX);
  memcpy(p->tonecurves[16][6]+2,tone_16000_60dB_SL,sizeof(*p->tonecurves[16][6])*EHMER_MAX);
  memcpy(p->tonecurves[16][8]+2,tone_16000_80dB_SL,sizeof(*p->tonecurves[16][8])*EHMER_MAX);
  memcpy(p->tonecurves[16][10]+2,tone_16000_100dB_SL,sizeof(*p->tonecurves[16][10])*EHMER_MAX);

  for(i=0;i<P_BANDS;i+=2)
    for(j=4;j<P_LEVELS;j+=2)
      for(k=2;k<EHMER_MAX+2;k++)
	p->tonecurves[i][j][k]+=vi->tone_masteratt;

  for(i=1;i<P_BANDS;i+=2)
    for(j=4;j<P_LEVELS;j+=2){
      memcpy(p->tonecurves[i][j]+2,p->tonecurves[i-1][j]+2,EHMER_MAX*sizeof(*p->tonecurves[i][j]));
      min_curve(p->tonecurves[i][j]+2,p->tonecurves[i+1][j]+2);
    }

  for(i=0;i<P_BANDS;i++)
    setup_curve(p->tonecurves[i],i,vi->toneatt.block[i]);

  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_63Hz",i,p->tonecurves[0][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_88Hz",i,p->tonecurves[1][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_125Hz",i,p->tonecurves[2][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_170Hz",i,p->tonecurves[3][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_250Hz",i,p->tonecurves[4][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_350Hz",i,p->tonecurves[5][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_500Hz",i,p->tonecurves[6][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_700Hz",i,p->tonecurves[7][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_1kHz",i,p->tonecurves[8][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_1.4Hz",i,p->tonecurves[9][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_2kHz",i,p->tonecurves[10][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_2.4kHz",i,p->tonecurves[11][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
     _analysis_output("curve_4kHz",i,p->tonecurves[12][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_5.6kHz",i,p->tonecurves[13][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_8kHz",i,p->tonecurves[14][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_11.5kHz",i,p->tonecurves[15][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_16kHz",i,p->tonecurves[16][i]+2,EHMER_MAX,0,0);

  if(vi->curvelimitp){
    for(i=0;i<P_BANDS;i++)
      for(j=0;j<P_LEVELS;j++){
	for(k=2;k<EHMER_OFFSET+2+vi->curvelimitp;k++)
	  if(p->tonecurves[i][j][k]> vi->peakatt.block[i][j])
	    p->tonecurves[i][j][k]=  vi->peakatt.block[i][j];
	  else
	    break;
      }
  }

  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_63Hz",i,p->tonecurves[0][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_88Hz",i,p->tonecurves[1][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_125Hz",i,p->tonecurves[2][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_170Hz",i,p->tonecurves[3][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_250Hz",i,p->tonecurves[4][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_350Hz",i,p->tonecurves[5][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_500Hz",i,p->tonecurves[6][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_700Hz",i,p->tonecurves[7][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_1kHz",i,p->tonecurves[8][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_1.4Hz",i,p->tonecurves[9][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_2kHz",i,p->tonecurves[10][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_2.4kHz",i,p->tonecurves[11][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_4kHz",i,p->tonecurves[12][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_5.6kHz",i,p->tonecurves[13][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_8kHz",i,p->tonecurves[14][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_11.5kHz",i,p->tonecurves[15][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_16kHz",i,p->tonecurves[16][i]+2,EHMER_MAX,0,0);

  if(vi->peakattp)
    for(i=0;i<P_BANDS;i++)
      for(j=0;j<P_LEVELS;j++)
	if(p->tonecurves[i][j][EHMER_OFFSET+2]< vi->peakatt.block[i][j])
	  p->tonecurves[i][j][EHMER_OFFSET+2]=  vi->peakatt.block[i][j];

  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_63Hz",i,p->tonecurves[0][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_88Hz",i,p->tonecurves[1][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_125Hz",i,p->tonecurves[2][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_170Hz",i,p->tonecurves[3][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_250Hz",i,p->tonecurves[4][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_350Hz",i,p->tonecurves[5][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_500Hz",i,p->tonecurves[6][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_700Hz",i,p->tonecurves[7][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_1kHz",i,p->tonecurves[8][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_1.4Hz",i,p->tonecurves[9][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_2kHz",i,p->tonecurves[10][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_2.4kHz",i,p->tonecurves[11][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_4kHz",i,p->tonecurves[12][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_5.6kHz",i,p->tonecurves[13][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_8kHz",i,p->tonecurves[14][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_11.5kHz",i,p->tonecurves[15][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_16kHz",i,p->tonecurves[16][i]+2,EHMER_MAX,0,0);

  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++)
      if(p->tonecurves[i][j][EHMER_OFFSET+2]< vi->tone_guard)
	  p->tonecurves[i][j][EHMER_OFFSET+2]=  vi->tone_guard;

  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_63Hz",i,p->tonecurves[0][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_88Hz",i,p->tonecurves[1][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_125Hz",i,p->tonecurves[2][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_170Hz",i,p->tonecurves[3][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_250Hz",i,p->tonecurves[4][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_350Hz",i,p->tonecurves[5][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_500Hz",i,p->tonecurves[6][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_700Hz",i,p->tonecurves[7][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_1kHz",i,p->tonecurves[8][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_1.4Hz",i,p->tonecurves[9][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_2kHz",i,p->tonecurves[10][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_2.4kHz",i,p->tonecurves[11][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_4kHz",i,p->tonecurves[12][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_5.6kHz",i,p->tonecurves[13][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_8kHz",i,p->tonecurves[14][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_11.5kHz",i,p->tonecurves[15][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_16kHz",i,p->tonecurves[16][i]+2,EHMER_MAX,0,0);

  for(i=0;i<n;i++){
    float halfoc=toOC((i+.5f)*rate/(2.f*n))*2.f;
    int inthalfoc;
    float del;

    if(halfoc<0)halfoc=0;
    if(halfoc>=P_BANDS-1)halfoc=P_BANDS-1;
    inthalfoc=(int)halfoc;
    del=halfoc-inthalfoc;
    p->noiseoffset[i]=
      p->vi->noiseoff[inthalfoc]*(1.-del) +
      p->vi->noiseoff[inthalfoc+1]*del;
  }

  analysis_noisy=1;
  _analysis_output("noiseoff",0,p->noiseoffset,n,1,0);
  _analysis_output("noisethresh",0,p->noisethresh,n,1,0);
  analysis_noisy=1;
}

/* _vp_psy_clear. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054aa20
void _vp_psy_clear(vorbis_look_psy *p){
  int i,j;
  if(p){
    if(p->ath)_ogg_free(p->ath);
    if(p->octave)_ogg_free(p->octave);
    if(p->bark)_ogg_free(p->bark);
    if(p->tonecurves){
      for(i=0;i<P_BANDS;i++){
	for(j=0;j<P_LEVELS;j++){
	  _ogg_free(p->tonecurves[i][j]);
	}
	_ogg_free(p->tonecurves[i]);
      }
      _ogg_free(p->tonecurves);
    }
    _ogg_free(p->noiseoffset);
    _ogg_free(p->noisethresh);
    FastMemSet(p,0,sizeof(*p));
  }
}

/* seed_curve. STATE: BurgerLib's libvorbis 1.0rc3, the choice clamp as two ifs (MATCH,
   byte-identical). OPEN: none. */
// FUNCTION: EVNOVA 0x0054ab20
static void seed_curve(float *seed,
		       const float **curves,
		       float amp,
		       int oc, int n,
		       int linesper,float dBoffset){
  int i,post1;
  int seedptr;
  const float *posts,*curve;

  int choice=(int)((amp+dBoffset)*.1f);
  if(choice<0)choice=0;
  if(choice>P_LEVELS-1)choice=P_LEVELS-1;
  posts=curves[choice];
  curve=posts+2;
  post1=(int)posts[1];
  seedptr=oc+(posts[0]-16)*linesper-(linesper>>1);

  for(i=posts[0];i<post1;i++){
    if(seedptr>0){
      float lin=amp+curve[i];
      if(seed[seedptr]<lin)seed[seedptr]=lin;
    }
    seedptr+=linesper;
    if(seedptr>=n)break;
  }
}

/* seed_loop. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054acc0
static void seed_loop(vorbis_look_psy *p,
		      const float ***curves,
		      const float *f,
		      const float *flr,
		      float *seed,
		      float specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  float dBoffset=vi->max_curve_dB-specmax;

  for(i=0;i<n;i++){
    float max=f[i];
    long oc=p->octave[i];
    while(i+1<n && p->octave[i+1]==oc){
      i++;
      if(f[i]>max)max=f[i];
    }

    if(max+6.f>flr[i]){
      oc=oc>>p->shiftoc;
      if(oc>=P_BANDS)oc=P_BANDS-1;
      if(oc<0)oc=0;
      seed_curve(seed,
		 curves[oc],
		 max,
		 p->octave[i]-p->firstoc,
		 p->total_octave_lines,
		 p->eighth_octave_lines,
		 dBoffset);
    }
  }
}

/* seed_chase. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054adc0
static void seed_chase(float *seeds, int linesper, long n){
  long  *posstack=(long *)_ogg_malloc(n*sizeof(*posstack));
  float *ampstack=(float *)_ogg_malloc(n*sizeof(*ampstack));
  long   stack=0;
  long   pos=0;
  long   i;

  for(i=0;i<n;i++){
    if(stack<2){
      posstack[stack]=i;
      ampstack[stack++]=seeds[i];
    }else{
      while(1){
	if(seeds[i]<ampstack[stack-1]){
	  posstack[stack]=i;
	  ampstack[stack++]=seeds[i];
	  break;
	}else{
	  if(i<posstack[stack-1]+linesper){
	    if(stack>1 && ampstack[stack-1]<=ampstack[stack-2] &&
	       i<posstack[stack-2]+linesper){
	      stack--;
	      continue;
	    }
	  }
	  posstack[stack]=i;
	  ampstack[stack++]=seeds[i];
	  break;
	}
      }
    }
  }

  for(i=0;i<stack;i++){
    long endpos;
    if(i<stack-1 && ampstack[i+1]>ampstack[i]){
      endpos=posstack[i+1];
    }else{
      endpos=posstack[i]+linesper+1;
    }
    if(endpos>n)endpos=n;
    for(;pos<endpos;pos++)
      seeds[pos]=ampstack[i];
  }

  _ogg_free(posstack);
  _ogg_free(ampstack);
}

/* max_seeds. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054af30
static void max_seeds(vorbis_look_psy *p,
		      float *seed,
		      float *flr){
  long   n=p->total_octave_lines;
  int    linesper=p->eighth_octave_lines;
  long   linpos=0;
  long   pos;

  seed_chase(seed,linesper,n);

  pos=p->octave[0]-p->firstoc-(linesper>>1);
  while(linpos+1<p->n){
    float minV=seed[pos];
    long end=((p->octave[linpos]+p->octave[linpos+1])>>1)-p->firstoc;
    if(minV>p->vi->tone_abs_limit)minV=p->vi->tone_abs_limit;
    while(pos+1<=end){
      pos++;
      if((seed[pos]>NEGINF && seed[pos]<minV) || minV==NEGINF)
	minV=seed[pos];
    }

    end=pos+p->firstoc;
    for(;linpos<p->n && p->octave[linpos]<=end;linpos++)
      if(flr[linpos]<minV)flr[linpos]=minV;
  }

  {
    float minV=seed[p->total_octave_lines-1];
    for(;linpos<p->n;linpos++)
      if(flr[linpos]<minV)flr[linpos]=minV;
  }
}

/* bark_noise_hybridmp. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054b0b0
static void bark_noise_hybridmp(int n,const long *b,
				const float *f,
				float *noise,
				const float offset,
				const int fixed){
  long i,hi=b[0]>>16,lo=b[0]>>16,hif=0,lof=0;
  double xa=0,xb=0;
  double ya=0,yb=0;
  double x2a=0,x2b=0;
  double xya=0,xyb=0;
  double na=0,nb=0;

  for(i=0;i<n;i++){
    if(hi<n){
      int bi=b[i]&0xffffL;
      for(;hi<bi;hi++){
	int ii=(hi<0?-hi:hi);
        double bin=(f[ii]<-offset?1.:f[ii]+offset);
	double nn= bin*bin;
	na  += nn;
	xa  += hi*nn;
	ya  += bin*nn;
	x2a += hi*hi*nn;
	xya += hi*bin*nn;
      }
      bi=b[i]>>16;
      for(;lo<bi;lo++){
	int ii=(lo<0?-lo:lo);
        double bin=(f[ii]<-offset?1.:f[ii]+offset);
	double nn= bin*bin;
	na  -= nn;
	xa  -= lo*nn;
	ya  -= bin*nn;
	x2a -= lo*lo*nn;
	xya -= lo*bin*nn;
      }
    }

    if(hif<n && fixed>0){
      int bi=i+fixed/2;
      if(bi>n)bi=n;

      for(;hif<bi;hif++){
	int ii=(hif<0?-hif:hif);
        double bin=(f[ii]<-offset?1.:f[ii]+offset);
	double nn= bin*bin;
	nb  += nn;
	xb  += hif*nn;
	yb  += bin*nn;
	x2b += hif*hif*nn;
	xyb += hif*bin*nn;
      }
      bi=i-(fixed+1)/2;
      for(;lof<bi;lof++){
	int ii=(lof<0?-lof:lof);
        double bin=(f[ii]<-offset?1.:f[ii]+offset);
	double nn= bin*bin;
	nb  -= nn;
	xb  -= lof*nn;
	yb  -= bin*nn;
	x2b -= lof*lof*nn;
	xyb -= lof*bin*nn;
      }
    }

    {
      double va=0.f;

      if(na>2){
        double denom=1./(na*x2a-xa*xa);
        double a=(ya*x2a-xya*xa)*denom;
        double b=(na*xya-xa*ya)*denom;
        va=a+b*i;
      }
      if(va<0.)va=0.;

      if(fixed>0){
        double vb=0.f;

        if(nb>2){
          double denomf=1./(nb*x2b-xb*xb);
          double af=(yb*x2b-xyb*xb)*denomf;
          double bf=(nb*xyb-xb*yb)*denomf;
          vb=af+bf*i;
        }
        if(vb<0.)vb=0.;
        if(va>vb && vb>0.)va=vb;
      }

      noise[i]=va-offset;
    }
  }
}

/* _vp_remove_floor. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054b640
void _vp_remove_floor(vorbis_look_psy *p,
		      float *mdct,
		      float *codedflr,
		      float *residue){
  int i,n=p->n;

  for(i=0;i<n;i++)
    if(mdct[i]!=0.f)
      residue[i]=mdct[i]/codedflr[i];
    else
      residue[i]=0.f;
}

/* _vp_compute_mask. STATE: BurgerLib's libvorbis 1.0rc3, MATCH, byte-identical (2014 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054b6a0
void _vp_compute_mask(vorbis_look_psy *p,
		      float *logfft,
		      float *logmdct,
		      float *logmask,
		      float global_specmax,
		      float local_specmax,
		      float bitrate_noise_offset){
  int i,n=p->n;
  static int seq=0;

  float *seed=(float *)_ogg_malloc(sizeof(*seed)*p->total_octave_lines);
  for(i=0;i<p->total_octave_lines;i++)seed[i]=NEGINF;

  if(p->vi->noisemaskp){
    float *work=(float *)_ogg_malloc(n*sizeof(*work));

    bark_noise_hybridmp(n,p->bark,logmdct,logmask,
			140.,-1);

    for(i=0;i<n;i++)work[i]=logmdct[i]-logmask[i];

    bark_noise_hybridmp(n,p->bark,work,logmask,0.,
			p->vi->noisewindowfixed);

    for(i=0;i<n;i++)work[i]=logmdct[i]-work[i];

    _analysis_output("noisemedian",seq,work,n,1,0);

    for(i=0;i<n;i++)logmask[i]+=work[i];
    _analysis_output("noiseenvelope",seq,logmask,n,1,0);
    for(i=0;i<n;i++)logmask[i]-=work[i];

    for(i=0;i<n;i++){
      int dB=logmask[i]+.5f;
      if(dB>=NOISE_COMPAND_LEVELS)dB=NOISE_COMPAND_LEVELS-1;
      logmask[i]= work[i]+p->vi->noisecompand[dB]+p->noiseoffset[i]+bitrate_noise_offset;
      if(logmask[i]>p->vi->noisemaxsupp)logmask[i]=p->vi->noisemaxsupp;
    }
    _analysis_output("noise",seq,logmask,n,1,0);

    _ogg_free(work);
  }else{
    for(i=0;i<n;i++)logmask[i]=NEGINF;
  }

  if(p->vi->ath){
    float att=local_specmax+p->vi->ath_adjatt;
    if(att<p->vi->ath_maxatt)att=p->vi->ath_maxatt;

    for(i=0;i<n;i++){
      float av=p->ath[i]+att;
      if(av>logmask[i])logmask[i]=av;
    }
  }

  seed_loop(p,(const float ***)p->tonecurves,logfft,logmask,seed,global_specmax);
  max_seeds(p,seed,logmask);

  for(i=0;i<n;i++)if(logmdct[i]>=logmask[i])break;
  if(i==n)
    for(i=0;i<n;i++)logmask[i]=NEGINF;
  else
    for(i=0;i<n;i++)
      if(logfft[i]<logmdct[i])logfft[i]=logmdct[i];

  seq++;
  _ogg_free(seed);
}

/* couple_lossless. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054be80
static void couple_lossless(float A, float B,
			    float granule,float igranule,
			    float *mag, float *ang,
			    int flip_p){

  if(fabs(A)>fabs(B)){
    A=rint(A*igranule)*granule;
    B=rint(B*igranule)*granule;

    *mag=A; *ang=(A>0.f?A-B:B-A);
  }else{
    A=rint(A*igranule)*granule;
    B=rint(B*igranule)*granule;

    *mag=B; *ang=(B>0.f?A-B:B-A);
  }

  if(flip_p && *ang>fabs(*mag)*1.9999f){
    *ang= -fabs(*mag)*2.f;
    *mag= -*mag;
  }
}

static void couple_point(float A, float B, float fA, float fB,
			 float granule,float igranule,
			 float fmag, float *mag, float *ang){

  float origmag=FAST_HYPOT(A*fA,B*fB),corr;

  if(fmag!=0.f){

    if(fabs(A)>fabs(B)){
      *mag=A;
    }else{
      *mag=B;
    }

    corr=origmag/FAST_HYPOT(fmag*fA,fmag*fB);
    *mag=rint(*mag*corr*igranule)*granule;
    *ang=0.f;

  }else{
    *mag=0.f;
    *ang=0.f;
  }
}

/* _vp_quantize_couple (couple_point inlined). STATE: BurgerLib's libvorbis 1.0rc3, MATCH,
   byte-identical (1184 B). OPEN: none. */
// FUNCTION: EVNOVA 0x0054c000
void _vp_quantize_couple(vorbis_look_psy *p,
			 vorbis_info_mapping0 *vi,
			 float **pcm,
			 float **sofar,
			 float **quantized,
			 int   *nonzero,
			 int   passno){

  int i,j,k,n=p->n;
  vorbis_info_psy *info=p->vi;

  for(i=0;i<vi->coupling_steps;i++){
    float granulem=info->couple_pass[passno].granulem;
    float igranulem=info->couple_pass[passno].igranulem;

    if(nonzero[vi->coupling_mag[i]] ||
       nonzero[vi->coupling_ang[i]]){

      float *pcmM=pcm[vi->coupling_mag[i]];
      float *pcmA=pcm[vi->coupling_ang[i]];
      float *floorM=pcm[vi->coupling_mag[i]]+n;
      float *floorA=pcm[vi->coupling_ang[i]]+n;
      float *sofarM=sofar[vi->coupling_mag[i]];
      float *sofarA=sofar[vi->coupling_ang[i]];
      float *qM=quantized[vi->coupling_mag[i]];
      float *qA=quantized[vi->coupling_ang[i]];

      nonzero[vi->coupling_mag[i]]=1;
      nonzero[vi->coupling_ang[i]]=1;

      for(j=0,k=0;j<n;k++){
	vp_couple *part=info->couple_pass[passno].couple_pass+k;
	float rqlimit=part->outofphase_requant_limit;
	int flip_p=part->outofphase_redundant_flip_p;

	for(;j<part->limit && j<p->n;j++){

	  float ang,mag,fmag=fabs(pcmM[j]);
	  mag=fabs(pcmA[j]);
	  if(fmag<mag)fmag=mag;

	  if(fmag<part->amppost_point){
	    couple_point(pcmM[j],pcmA[j],floorM[j],floorA[j],
			 granulem,igranulem,fmag,&mag,&ang);

	  }else{
	    couple_lossless(pcmM[j],pcmA[j],
			    granulem,igranulem,&mag,&ang,flip_p);
	  }

	  qM[j]=mag-sofarM[j];
	  qA[j]=ang-sofarA[j];

	  if(qA[j]<-rqlimit || qA[j]>rqlimit){
	    qM[j]=0.f;
	    qA[j]=0.f;
	  }
	}
      }
    }
  }
}
