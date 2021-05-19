/*
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "RT_Stats.h"

int __cdecl RT_MRGBChanstats_Lo(int flgs,const AVSValue &std,const AVSValue &xtra,MRGBALO &rgb,const char*Name,IScriptEnvironment* env) {

	const char *myName=(Name==NULL)?"RT_MRGBChanstatLo: ":Name;

	flgs=flgs & (RTMIN_F|RTMAX_F|RTMINMAX_F|RTMEDIAN_F|RTAVE_F|RTSTDEV_F|RTYINRNG_F|RTPNORM_F);

	if(!std[STD_CLIP].IsClip())
		env->ThrowError("%sMust have a source clip",myName);

	PClip child  = std[STD_CLIP].AsClip();											// Clip

	int n;
    if(std[STD_FRAME].IsInt()) {n  = std[STD_FRAME].AsInt(); }						// Frame n
    else {
        AVSValue cn       =	GetVar(env,"current_frame");
        if (!cn.IsInt())	env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
        n                 =	cn.AsInt();										// current_frame
    }
    n += std[STD_DELTA].AsInt(0);											// Delta, default 0

    const VideoInfo &vi     = child->GetVideoInfo();
    n = (n<0) ? 0 : (n>=vi.num_frames)?vi.num_frames-1:n;					// Range limit frame n

	if(!vi.IsRGB()) {
		env->ThrowError("%sRGB ONLY",myName);
	}

	int xx=			std[STD_XX].AsInt(0);
	int yy=			std[STD_YY].AsInt(0);
	int	ww=			std[STD_WW].AsInt(0);
	int	hh=			std[STD_HH].AsInt(0);
	bool altscan=		std[STD_LACED].AsBool(false);

	int maxchan = (vi.IsRGB24()) ? 2 : 3;

	int chan = xtra[XTRA_RGBIX].AsInt(0);				// 0=R, 1=G, 2=B, 3=A

	int chmin,chmax;

	if(chan == -1) {
		chmin=0;
		chmax=2;										// chan == -1 ONLY PROCESSES R,G,B
	} else if(chan == -2) {
		if(maxchan!=3)
			env->ThrowError("%sInvalid Chan(-2). RGB24 has no ALPHA Channel",myName);
		chmin=0;
		chmax=3;									// chan == -2 PROCESSES R,G,B, and ALPHA if RGB32
	} else {
		if(chan < 0 || chan > maxchan) {
			env->ThrowError("%sInvalid Channel number",myName);
		}
		chmin=chan;
		chmax=chan;
	}


	double thresh=	xtra[XTRA_THRESH].AsFloat(0.0);
	thresh = (thresh<0.0) ? 0.0 : (thresh>100.0) ? 100.0 : thresh;
	thresh = thresh / 100.0;

	int lo=			xtra[XTRA_LO].AsInt(128);
	lo = (lo < 0) ? 0 : (lo > 255) ? 255 : lo;
	int hi=			xtra[XTRA_HI].AsInt(lo);
	hi = (hi < 0) ? 0 : (hi > 255) ? 255 : hi;


	double mu=xtra[XTRA_MU].AsFloat(0.0);
	int div=xtra[XTRA_D].AsInt(1);
	int power=xtra[XTRA_P].AsInt(1);
	int up=xtra[XTRA_U].AsInt(1);
	
	if(ww <= 0) ww += vi.width  - xx;
	if(hh <= 0) hh += vi.height - yy;
	if(altscan && ((hh & 0x01) == 0)) --hh;	// If altscan then ensure ODD number of lines, last line other field does not count.
	altscan=(altscan && hh!=1 && vi.height!=1);

	if(xx <  0 || xx >=vi.width)				env->ThrowError("%sInvalid X coord",myName);
	if(yy <  0 || yy >=vi.height)				env->ThrowError("%sInvalid Y coord",myName);
	if(ww <= 0 || xx + ww > vi.width)			env->ThrowError("%sInvalid W coord",myName);
	if(hh <= 0 || yy + hh > vi.height)			env->ThrowError("%sInvalid H coord",myName);


	if(mu <0.0 || mu>255.0)						env->ThrowError("%sInvalid mu (0.0->255.0)",myName);
	if(div <1 || div>255)						env->ThrowError("%sInvalid d (1->255)",myName);
	if(power < 1 || power>16)					env->ThrowError("%sInvalid p (1->16)",myName);
	if(up <1 || up>255)							env->ThrowError("%sInvalid u (1->255)",myName);


	PClip mask=NULL;
	const int maskmin=xtra[XTRA_MASKMIN].AsInt(128);
	const int maskmax=xtra[XTRA_MASKMAX].AsInt(255);

	if((maskmin!=0 || maskmax!=255) && xtra[XTRA_MASK].IsClip()) {
		mask  = xtra[XTRA_MASK].AsClip();								// Mask

		if(!mask->GetVideoInfo().IsPlanar()) {
			env->ThrowError("%sPlanar Mask ONLY",myName);
		}
		if((vi.width != (mask->GetVideoInfo().width)) || (vi.height != (mask->GetVideoInfo().height))) {
			env->ThrowError("%sMask clip dissimilar dimensions\n",myName);
		}
		if(vi.num_frames > mask->GetVideoInfo().num_frames) {
			env->ThrowError("%sMask clip frame count smaller than clip.\n",myName);
		}
		if(maskmin<0 || maskmin>255)
			env->ThrowError("%sMaskMin Range 0->255 (%d)\n",myName,maskmin);

		if(maskmax<maskmin || maskmax>255)
			env->ThrowError("%sMaskMax Range MaskMin->255 (min=%d max=%d)\n",myName,maskmin,maskmax);
	}

	unsigned int Pixels = (ww * ((altscan)?(hh+1)>>1:hh));			// all selected pixels (not masked)

    PVideoFrame src         = child->GetFrame(n,env);
    const int   pitch       = src->GetPitch(PLANAR_Y);					// PLANAR_Y no effect on non-Planar (equates to 0)
    const BYTE  *srcp       = src->GetReadPtr(PLANAR_Y);
	const int ystep			= (altscan) ? 2:1;
	const int ystride		= pitch*ystep;

    const int				xstep = (vi.IsRGB24()) ? 3 : 4;
	int rgbxx,rgbww;
	rgbxx = xx * xstep;		rgbww = ww * xstep;

	srcp += ((vi.height-1 - yy) * pitch) + rgbxx;			// Upside down RGB, height-1 is top line, -yy is top line of yy coord

	int64_t accA[4];
    unsigned int sumA[4];
	unsigned int cntA[4][256];
    int x, y,i;

	int ww2=ww * xstep;
	int x2;
	if(chmin==chmax) { // Single Channel
		int64_t acc=0;
		unsigned int sum=0;
		unsigned int *cnt=&cntA[chmin][0];
		srcp += (chmin==3) ? 3 : 2 - chmin;										// RGB to BGR offset
		if(flgs != RTAVE_F) {
			memset(cnt,NULL,sizeof(cntA[0]));
		}
		if (mask !=NULL) { // MASK
			PVideoFrame msrc        = mask->GetFrame(n,env);
			const int   mpitch      = msrc->GetPitch(PLANAR_Y);					// PLANAR_Y no effect on non-Planar (equates to 0)
			const BYTE  *msrcp      = msrc->GetReadPtr(PLANAR_Y);
			const int mystride		= mpitch*ystep;

			msrcp += (yy * mpitch) + xx;

			Pixels = 0;		// We are gonna count from mask
			if(ww == 1) {														// Special case for single pixel width
				if(flgs==RTAVE_F) {
					for(y=0 ; y < hh; y += ystep) {
						if(*msrcp>=maskmin && *msrcp<=maskmax) {
							++Pixels;
							sum += *srcp;
						}
						srcp -= ystride;
						msrcp+=mystride;
					}
				} else {
					for(y=0 ; y < hh  ; y += ystep) {                       
						if(*msrcp>=maskmin && *msrcp<=maskmax) {
							++Pixels;
							++cnt[*srcp];
						}
						srcp -= ystride;
						msrcp+= mystride;
					}
				}
			} else {				
				if(flgs==RTAVE_F) {
					if(Pixels > 0x808080) {
						for(y=0 ; y < hh; y += ystep) {                       
							for (x2=ww2, x=ww ;x2-=xstep, --x>=0 ;) {
							if(msrcp[x]>=maskmin && msrcp[x]<=maskmax) {
									++Pixels;
									sum += srcp[x2];
								}
							}
							if(sum & 0x80000000) {acc += sum;sum=0;}					// avoid possiblilty of overflow
							srcp -= ystride;
							msrcp+= mystride;
						}
					} else {
						for(y=0 ; y < hh; y += ystep) {                       
							for (x2=ww2, x=ww ;x2-=xstep, --x>=0 ;) {
							if(msrcp[x]>=maskmin && msrcp[x]<=maskmax) {
									++Pixels;
									sum += srcp[x2];
								}
							}
							srcp -= ystride;
							msrcp+= mystride;
						}
					}
				} else {
					for(y=0 ; y < hh  ; y += ystep) {                       
						for (x2=ww2, x=ww ;x2-=xstep, --x>=0 ;) {
							if(msrcp[x]>=maskmin && msrcp[x]<=maskmax) {
								++Pixels;
								++cnt[srcp[x2]];
							}
						}
						srcp -= ystride;
						msrcp+= mystride;
					}
				}
			}
		} else { // NO MASK
			if(ww == 1) {															// Special case for single pixel width
				if(flgs==RTAVE_F) {
					for(y=0 ; y < hh; y += ystep) {
						sum += *srcp;
						srcp-= ystride;
					}
				} else {
					for(y=0 ; y < hh  ; y += ystep) {                       
						++cnt[*srcp];
						srcp -= ystride;
					}
				}
			} else {
				if(flgs==RTAVE_F) {
					if(Pixels > 0x808080) {
						for(y=0 ; y < hh; y += ystep) {                       
							for (x=ww2 ;x-=xstep, x>=0 ;) {
								sum += srcp[x];
							}
							if(sum & 0x80000000) {acc += sum;sum=0;}					// avoid possiblilty of overflow
							srcp -= ystride;
						}
					} else {
						for(y=0 ; y < hh; y += ystep) {                       
							for (x=ww2 ;x-=xstep, x>=0 ;) {
								sum += srcp[x];
							}
							srcp -= ystride;
						}
					}
				} else {
					for(y=0 ; y < hh  ; y += ystep) {                       
						for (x=ww2;x-=xstep, x>=0 ;) {
							++cnt[srcp[x]];
						}
						srcp -= ystride;
					}
				}
			}
		}
		accA[chmin]=acc;														// update arrays
		sumA[chmin]=sum;
	} else {	// multi-channel
		if(flgs != RTAVE_F) {
			memset(cntA,NULL,sizeof(cntA));
		} else {
			for(i=4;--i>=0;) {
				sumA[i]=0;
				accA[i]=0;
			}
		}
		const bool DoAlpha = chmax == 3;
		if (mask !=NULL) { // MASK
			PVideoFrame msrc        = mask->GetFrame(n,env);
			const int   mpitch      = msrc->GetPitch(PLANAR_Y);					// PLANAR_Y no effect on non-Planar (equates to 0)
			const BYTE  *msrcp      = msrc->GetReadPtr(PLANAR_Y);
			const int mystride		= mpitch*ystep;

			msrcp += (yy * mpitch) + xx;

			Pixels = 0;		// We are gonna count from mask
			if(ww == 1) {														// Special case for single pixel width
				if(flgs==RTAVE_F) {
					if(DoAlpha) {										// RGB Plus ALPHA
						for(y=0 ; y < hh; y += ystep) {
							if(*msrcp>=maskmin && *msrcp<=maskmax) {
								++Pixels;
								sumA[0] += srcp[2];						// RED
								sumA[1] += srcp[1];						// GREEN
								sumA[2] += srcp[0];						// BLUE
								sumA[3] += srcp[3];						// ALPHA
							}
							srcp -= ystride;
							msrcp+=mystride;
						}
					} else {											// RGB Only
						for(y=0 ; y < hh; y += ystep) {
							if(*msrcp>=maskmin && *msrcp<=maskmax) {
								++Pixels;
								sumA[0] += srcp[2];						// RED
								sumA[1] += srcp[1];						// GREEN
								sumA[2] += srcp[0];						// BLUE
							}
							srcp -= ystride;
							msrcp+=mystride;
						}
					}
				} else {
					if(DoAlpha) {										// RGB Plus ALPHA
						for(y=0 ; y < hh  ; y += ystep) {                       
							if(*msrcp>=maskmin && *msrcp<=maskmax) {
								++Pixels;
								++cntA[0][srcp[2]];						// RED
								++cntA[1][srcp[1]];						// GREEN
								++cntA[2][srcp[0]];						// BLUE
								++cntA[3][srcp[3]];						// ALPHA
							}
							srcp -= ystride;
							msrcp+= mystride;
						}
					} else {											// RGB Only
						for(y=0 ; y < hh  ; y += ystep) {                       
							if(*msrcp>=maskmin && *msrcp<=maskmax) {
								++Pixels;
								++cntA[0][srcp[2]];						// RED
								++cntA[1][srcp[1]];						// GREEN
								++cntA[2][srcp[0]];						// BLUE
							}
							srcp -= ystride;
							msrcp+= mystride;
						}
					}
				}
			} else { // ww != 1
				if(flgs==RTAVE_F) {
					if(Pixels > 0x808080) {
						if(DoAlpha) {										// RGB Plus ALPHA
							for(y=0 ; y < hh; y += ystep) {                       
								for (x2=ww2, x=ww ; x2-=xstep, --x>=0 ; ) {
									if(msrcp[x]>=maskmin && msrcp[x]<=maskmax) {
											++Pixels;
											sumA[0] += srcp[x2+2];			// RED
											sumA[1] += srcp[x2+1];			// GREEN
											sumA[2] += srcp[x2+0];			// BLUE
											sumA[3] += srcp[x2+3];			// ALPHA
									}
								}
								if(sumA[0] & 0x80000000) {accA[0] += sumA[0];sumA[0]=0;}		// avoid possiblilty of overflow
								if(sumA[1] & 0x80000000) {accA[1] += sumA[1];sumA[1]=0;}
								if(sumA[2] & 0x80000000) {accA[2] += sumA[2];sumA[2]=0;}
								if(sumA[3] & 0x80000000) {accA[3] += sumA[3];sumA[3]=0;}
								srcp -= ystride;
								msrcp+= mystride;
							}
						} else {											// RGB Only									
							for(y=0 ; y < hh; y += ystep) {                       
								for (x2=ww2, x=ww ; x2-=xstep, --x>=0 ; ) {
									if(msrcp[x]>=maskmin && msrcp[x]<=maskmax) {
											++Pixels;
											sumA[0] += srcp[x2+2];			// RED
											sumA[1] += srcp[x2+1];			// GREEN
											sumA[2] += srcp[x2+0];			// BLUE
									}	
								}
								if(sumA[0] & 0x80000000) {accA[0] += sumA[0];sumA[0]=0;}		// avoid possiblilty of overflow
								if(sumA[1] & 0x80000000) {accA[1] += sumA[1];sumA[1]=0;}
								if(sumA[2] & 0x80000000) {accA[2] += sumA[2];sumA[2]=0;}
								srcp -= ystride;
								msrcp+= mystride;
							}					
						}
					} else {
						if(DoAlpha) {										// RGB Plus ALPHA
							for(y=0 ; y < hh; y += ystep) {                       
								for (x2=ww2, x=ww ; x2-=xstep, --x>=0 ; ) {
									if(msrcp[x]>=maskmin && msrcp[x]<=maskmax) {
											++Pixels;
											sumA[0] += srcp[x2+2];			// RED
											sumA[1] += srcp[x2+1];			// GREEN
											sumA[2] += srcp[x2+0];			// BLUE
											sumA[3] += srcp[x2+3];			// ALPHA
									}
								}
								srcp -= ystride;
								msrcp+= mystride;
							}
						} else {											// RGB Only									
							for(y=0 ; y < hh; y += ystep) {                       
								for (x2=ww2, x=ww ; x2-=xstep, --x>=0 ; ) {
									if(msrcp[x]>=maskmin && msrcp[x]<=maskmax) {
											++Pixels;
											sumA[0] += srcp[x2+2];			// RED
											sumA[1] += srcp[x2+1];			// GREEN
											sumA[2] += srcp[x2+0];			// BLUE
									}	
								}
								srcp -= ystride;
								msrcp+= mystride;
							}					
						}
					}
				} else {
					if(DoAlpha) {										// RGB Plus ALPHA
						for(y=0 ; y < hh  ; y += ystep) {                       
							for (x2=ww2, x=ww; x2-=xstep, --x>=0 ; ) {
								if(msrcp[x]>=maskmin && msrcp[x]<=maskmax) {
									++Pixels;
									++cntA[0][srcp[x2+2]];					// RED
									++cntA[1][srcp[x2+1]];					// GREEN
									++cntA[2][srcp[x2+0]];					// BLUE
									++cntA[3][srcp[x2+3]];					// ALPHA
								}
							}
							srcp -= ystride;
							msrcp+= mystride;
						}
					} else {											// RGB Only
						for(y=0 ; y < hh  ; y += ystep) {                       
							for (x2=ww2, x=ww; x2-=xstep, --x>=0 ; ) {
								if(msrcp[x]>=maskmin && msrcp[x]<=maskmax) {
									++Pixels;
									++cntA[0][srcp[x2+2]];					// RED
									++cntA[1][srcp[x2+1]];					// GREEN
									++cntA[2][srcp[x2+0]];					// BLUE
								}
							}
							srcp -= ystride;
							msrcp+= mystride;
						}
					}
				}
			}
		} else { // NO MASK
			if(ww == 1) {															// Special case for single pixel width
				if(flgs==RTAVE_F) {
					if(DoAlpha) {									// RGB Plus ALPHA
						for(y=0 ; y < hh; y += ystep) {
							sumA[0] += srcp[2];						// RED
							sumA[1] += srcp[1];						// GREEN
							sumA[2] += srcp[0];						// BLUE
							sumA[3] += srcp[3];						// ALPHA
							srcp-= ystride;
						}
					} else {										// RGB Only
						for(y=0 ; y < hh; y += ystep) {
							sumA[0] += srcp[2];						// RED
							sumA[1] += srcp[1];						// GREEN
							sumA[2] += srcp[0];						// BLUE
							srcp-= ystride;
						}
					}
				} else {
					if(DoAlpha) {									// RGB Plus ALPHA
						for(y=0 ; y < hh  ; y += ystep) {                       
							++cntA[0][srcp[2]];						// RED
							++cntA[1][srcp[1]];						// GREEN
							++cntA[2][srcp[0]];						// BLUE
							++cntA[3][srcp[3]];						// ALPHA
							srcp -= ystride;
						}
					} else {										// RGB Only
						for(y=0 ; y < hh  ; y += ystep) {                       
							++cntA[0][srcp[2]];						// RED
							++cntA[1][srcp[1]];						// GREEN
							++cntA[2][srcp[0]];						// BLUE
							srcp -= ystride;
						}
					}
				}
			} else {
				if(flgs==RTAVE_F) {
					if(Pixels > 0x808080) {
						if(DoAlpha) {										// RGB Plus ALPHA
							for(y=0 ; y < hh; y += ystep) {                       
								for (x=ww2 ; x-=xstep, x>=0 ;) {
									sumA[0] += srcp[x+2];				// RED
									sumA[1] += srcp[x+1];				// GREEN
									sumA[2] += srcp[x+0];				// BLUE
									sumA[3] += srcp[x+3];				// ALPHA
								}
								if(sumA[0] & 0x80000000) {accA[0] += sumA[0];sumA[0]=0;}		// avoid possiblilty of overflow
								if(sumA[1] & 0x80000000) {accA[1] += sumA[1];sumA[1]=0;}
								if(sumA[2] & 0x80000000) {accA[2] += sumA[2];sumA[2]=0;}
								if(sumA[3] & 0x80000000) {accA[3] += sumA[3];sumA[3]=0;}
								srcp -= ystride;
							}
						} else {									// RGB Only
							for(y=0 ; y < hh; y += ystep) {                       
								for (x=ww2 ; x-=xstep, x>=0 ;) {
									sumA[0] += srcp[x+2];				// RED
									sumA[1] += srcp[x+1];				// GREEN
									sumA[2] += srcp[x+0];				// BLUE
								}
								if(sumA[0] & 0x80000000) {accA[0] += sumA[0];sumA[0]=0;}		// avoid possiblilty of overflow
								if(sumA[1] & 0x80000000) {accA[1] += sumA[1];sumA[1]=0;}
								if(sumA[2] & 0x80000000) {accA[2] += sumA[2];sumA[2]=0;}
								srcp -= ystride;
							}
						}
					} else {
						if(DoAlpha) {										// RGB Plus ALPHA
							for(y=0 ; y < hh; y += ystep) {                       
								for (x=ww2 ; x-=xstep, x>=0 ;) {
									sumA[0] += srcp[x+2];				// RED
									sumA[1] += srcp[x+1];				// GREEN
									sumA[2] += srcp[x+0];				// BLUE
									sumA[3] += srcp[x+3];				// ALPHA
								}
								srcp -= ystride;
							}
						} else {									// RGB Only
							for(y=0 ; y < hh; y += ystep) {                       
								for (x=ww2 ; x-=xstep, x>=0 ;) {
									sumA[0] += srcp[x+2];				// RED
									sumA[1] += srcp[x+1];				// GREEN
									sumA[2] += srcp[x+0];				// BLUE
								}
								srcp -= ystride;
							}
						}
					}
				} else {
					if(DoAlpha) {										// RGB Plus ALPHA
						for(y=0 ; y < hh  ; y += ystep) {                       
							for (x=ww2 ; x-=xstep, x>=0 ;) {
								++cntA[0][srcp[x+2]];					// RED
								++cntA[1][srcp[x+1]];					// GREEN
								++cntA[2][srcp[x+0]];					// BLUE
								++cntA[3][srcp[x+3]];					// ALPHA
							}
							srcp -= ystride;
						}
					} else {									// RGB Only
						for(y=0 ; y < hh  ; y += ystep) {                       
							for (x=ww2 ; x-=xstep, x>=0 ;) {
								++cntA[0][srcp[x+2]];					// RED
								++cntA[1][srcp[x+1]];					// GREEN
								++cntA[2][srcp[x+0]];					// BLUE
							}
							srcp -= ystride;
						}
					}
				}
			}
		}

	} // END 	// multi-channel

	memset(&rgb,NULL,sizeof(rgb));

	rgb.chmin=chmin;
	rgb.chmax=chmax;
	rgb.pixels=Pixels;

	if(Pixels==0)
		return 0;		// Got Nothing, No valid pixels

	for(chan=chmin;chan<=chmax;++chan) {
		int64_t acc;
		unsigned long sum;
		if(flgs==RTAVE_F) {
			acc=accA[chan];
			sum=sumA[chan];
			acc += sum;
			rgb.chans[chan].ddat[RTAVE-RTAVE] = ((double)acc / Pixels);
		} else {
			unsigned int *cnt=&cntA[chan][0];
			int rmin;
			double mean=0.0;

			if(flgs & (RTAVE_F | RTSTDEV_F)) {
				if((flgs & RTAVE_F) || (Pixels>1)) {
					for(acc=0,i=256;--i>=0;) {
						acc += cnt[i] * i;
					}
					mean = (double)acc / Pixels;
					if(flgs & RTAVE_F) {
						rgb.chans[chan].ddat[RTAVE-RTAVE]=mean;
					}
				}
			}
			if(flgs & (RTMIN_F | RTMINMAX_F)) {
				rmin=255;
				const unsigned int lim = (unsigned int)(Pixels * thresh);
				unsigned int sm=0;
				for(i=0;i<256;++i) {
					sm += cnt[i];
					if(sm > lim) {
						rmin=i;
						break;
					}
				}
				if(flgs & RTMIN_F) {
					rgb.chans[chan].idat[RTMIN]=rmin;
				}
			}			
			if(flgs & (RTMAX_F | RTMINMAX_F)) {
				int rmax=0;
				const unsigned int lim = (unsigned int)(Pixels * thresh);
				unsigned int sm=0;
				for(i=256;--i>=0;) {
					sm += cnt[i];
					if(sm > lim) {
						rmax=i;
						break;
					}
				}
				if(flgs & RTMAX_F) {
					rgb.chans[chan].idat[RTMAX]=rmax;
				}
				if(flgs & RTMINMAX_F) {
					rgb.chans[chan].idat[RTMINMAX]=rmax-rmin;
				}
			}
			if(flgs & RTMEDIAN_F) {
				int rmed=0;
				const unsigned int lim = Pixels / 2;		// making smaller than pixels, no overflow
				unsigned int sm=0;
				for(i=0;i<256;++i) {
					sm += cnt[i];
					if(sm > lim) {
						rmed=i;
						break;
					}
				}
				rgb.chans[chan].idat[RTMEDIAN]=rmed; 
			}
			if(flgs & RTYINRNG_F) {
				unsigned int sm=0;
				for(i=lo;i<=hi;++i) {
					sm += cnt[i];
				}
				rgb.chans[chan].ddat[RTYINRNG-RTAVE]=((double)sm / Pixels);
			}			
			if(flgs & RTSTDEV_F) {											// Sample Std Deviation
				double std=0.0;
				if(Pixels > 1) {
					double Sum_DiffSquared = 0.0;
					for (i=256;--i>=0;) {
						const unsigned int count=cnt[i];
						if(count) {
							const double diff = (double)i - mean;				// x - xbar
							Sum_DiffSquared += (diff * diff * double(count));	// Sigma((x-xbar)^2) 
						}
					}
					std=(Sum_DiffSquared<=0.0)?0.0:sqrt(Sum_DiffSquared / Pixels);
				}
				rgb.chans[chan].ddat[RTSTDEV-RTAVE] = std;
			}			
			if(flgs & RTPNORM_F) {
				const double mu_d	=double(mu);
				const double div_d	=double(div);
				const double power_d=double(power);
				double Sum_DiffPowered = 0.0;
				// At this point Pixels known to be > 0, otherwise function already returned
				for (i=256;--i>=0;) {
					const int count=cnt[i];					// Count of pixels with value i
					if(count) {
						double diff = i - mu_d;				// -255.0 -> 255.0
						if (div>1)
							diff /= div_d;
						if (power>1)
							diff = pow(diff, power_d);		// (-255.0 -> 255.0) ^ power
						// power may be even or odd, but integer. So diff can be positive or negative.
						Sum_DiffPowered += diff * (double)count;
					}
				}
				double DiffPowered = Sum_DiffPowered / Pixels;
				double pn;
				if(DiffPowered >= 0.0) {
					pn = pow(DiffPowered, 1.0 / power_d);
				} else {
					pn = - (pow(- DiffPowered, 1.0 / power_d));
				}
#ifdef _DEBUG
				// floating point error flags (eg 1.0/3.0 gives _EM_INEXACT, we dont care)
				unsigned int e = (_clearfp() & ~_EM_INEXACT);
				if(e) {
					dprintf("MRGBChanPNorm: ***ERROR*** E=%X mu=%f div=%d power=%d chan=%d\n",e,mu_d,div,power,chan);
					dprintf("MRGBChanPNorm:             DiffPowered=%f 1/p=%f pn=%f\n",DiffPowered, 1.0 / power_d,pn);
				}
#endif
				pn = pn * double(up);
				// ensure fits in float for client
				if(pn>0.0) {
					if(pn > FLT_MAX)		pn = FLT_MAX;
					else if(pn < FLT_MIN)	pn = FLT_MIN;
				} else if(pn<0.0) {
					if(pn < -FLT_MAX)		pn = -FLT_MAX;
					else if(pn > -FLT_MIN)	pn = -FLT_MIN;
				}
				rgb.chans[chan].ddat[RTPNORM-RTAVE] = pn;
			}
		}
	}
	return flgs;
}


// -----------------------------------------------------------------------------------
//								RT Standard Maskless
// -----------------------------------------------------------------------------------


AVSValue __cdecl RT_RgbChanMin(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_RGBChanMin: ";
	int chan=args[9].AsInt(0);
	if(chan < 0)	env->ThrowError("%sInvalid Channel number",myName);
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_THRESH]	= args[8];
	xtra[XTRA_RGBIX]	= chan;		// RGBIX = chan
	MRGBALO rgb;
	if (RT_MRGBChanstats_Lo(RTMIN_F,args,AVSValue(xtra,XTRA_SIZE),rgb,myName,env)==0)
		return -1;
	return  rgb.chans[rgb.chmin].idat[RTMIN];
}

AVSValue __cdecl RT_RgbChanMax(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_RGBChanMax: ";
	int chan=args[9].AsInt(0);
	if(chan < 0)	env->ThrowError("%sInvalid Channel number",myName);
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_THRESH]	= args[8];
	xtra[XTRA_RGBIX]	= chan;		// RGBIX = chan
	MRGBALO rgb;
	if (RT_MRGBChanstats_Lo(RTMAX_F,args,AVSValue(xtra,XTRA_SIZE),rgb,myName,env)==0)
		return -1;
	return  rgb.chans[rgb.chmin].idat[RTMAX];
}

AVSValue __cdecl RT_RgbChanMinMaxDifference(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_RgbChanMinMaxDifference: ";
	int chan=args[9].AsInt(0);
	if(chan < 0)	env->ThrowError("%sInvalid Channel number",myName);
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_THRESH]	= args[8];
	xtra[XTRA_RGBIX]	= chan;		// RGBIX = chan
	MRGBALO rgb;
	if (RT_MRGBChanstats_Lo(RTMINMAX_F,args,AVSValue(xtra,XTRA_SIZE),rgb,myName,env)==0)
		return -1;
	return  rgb.chans[rgb.chmin].idat[RTMINMAX];
}

AVSValue __cdecl RT_RgbChanMedian(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_RgbChanMedian: ";
	int chan=args[8].AsInt(0);
	if(chan < 0)	env->ThrowError("%sInvalid Channel number",myName);
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_RGBIX]	= chan;		// RGBIX = chan
	MRGBALO rgb;
	if (RT_MRGBChanstats_Lo(RTMEDIAN_F,args,AVSValue(xtra,XTRA_SIZE),rgb,myName,env)==0)
		return -1;
	return  rgb.chans[rgb.chmin].idat[RTMEDIAN];
}

AVSValue __cdecl RT_RgbChanAve(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_RgbChanAve: ";
	int chan=args[8].AsInt(0);
	if(chan < 0)	env->ThrowError("%sInvalid Channel number",myName);
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_RGBIX]	= chan;		// RGBIX = chan
	MRGBALO rgb;
	if (RT_MRGBChanstats_Lo(RTAVE_F,args,AVSValue(xtra,XTRA_SIZE),rgb,myName,env)==0)
		return -1;
	return  rgb.chans[rgb.chmin].ddat[RTAVE-RTAVE];
}

AVSValue __cdecl RT_RgbChanStdev(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_RgbChanStdev: ";
	int chan=args[8].AsInt(0);
	if(chan < 0)	env->ThrowError("%sInvalid Channel number",myName);
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_RGBIX]	= chan;		// RGBIX = chan
	MRGBALO rgb;
	if (RT_MRGBChanstats_Lo(RTSTDEV_F,args,AVSValue(xtra,XTRA_SIZE),rgb,myName,env)==0)
		return -1;
	return  rgb.chans[rgb.chmin].ddat[RTSTDEV-RTAVE];
}


AVSValue __cdecl RT_RgbChanInRange(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_RgbChanInRange: ";
	int chan=args[8].AsInt(0);
	if(chan < 0)	env->ThrowError("%sInvalid Channel number",myName);
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_RGBIX]	= chan;		// RGBIX = chan
	xtra[XTRA_LO]		= args[9];
	xtra[XTRA_HI]		= args[10];
	MRGBALO rgb;
	if (RT_MRGBChanstats_Lo(RTYINRNG_F,args,AVSValue(xtra,XTRA_SIZE),rgb,myName,env)==0)
		return -1;
	return  rgb.chans[rgb.chmin].ddat[RTYINRNG-RTAVE];
}

AVSValue __cdecl RT_RgbChanPNorm(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_RgbChanPNorm: ";
	int chan=args[8].AsInt(0);
	if(chan < 0)	env->ThrowError("%sInvalid Channel number",myName);
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_RGBIX]	= chan;		// RGBIX = chan
	xtra[XTRA_MU]		= args[9];
	xtra[XTRA_D]		= args[10];
	xtra[XTRA_P]		= args[11];
	xtra[XTRA_U]		= args[12];
	MRGBALO rgb;
	if (RT_MRGBChanstats_Lo(RTPNORM_F,args,AVSValue(xtra,XTRA_SIZE),rgb,myName,env)==0)
		return -1;
	return  rgb.chans[rgb.chmin].ddat[RTPNORM-RTAVE];
}


AVSValue __cdecl RT_RgbChanStats(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_RgbChanStats: ";
	int RTALLY=(RTMIN_F|RTMAX_F|RTMINMAX_F|RTMEDIAN_F|RTAVE_F|RTSTDEV_F|RTYINRNG_F|RTPNORM_F);
	int flgs=(args[12].AsInt(RTALLY)) & RTALLY;
	if(flgs) {
		int i;
		const char *prefix=args[13].AsString("RCS_");
		int chan=args[9].AsInt(0);
		AVSValue xtra[XTRA_SIZE];
		xtra[XTRA_THRESH]	= args[8];
		xtra[XTRA_RGBIX]	= chan;		// RGBIX = chan
		xtra[XTRA_LO]		= args[10];
		xtra[XTRA_HI]		= args[11];
		xtra[XTRA_MU]		= args[14];
		xtra[XTRA_D]		= args[15];
		xtra[XTRA_P]		= args[16];
		xtra[XTRA_U]		= args[17];
		xtra[XTRA_MASK]		= args[18];
		xtra[XTRA_MASKMIN]	= args[19];
		xtra[XTRA_MASKMAX]	= args[20];
		MRGBALO rgb;
		flgs=RT_MRGBChanstats_Lo(flgs,args,AVSValue(xtra,XTRA_SIZE),rgb,myName,env);
		int pfixlen=int(strlen(prefix));
		if(pfixlen>128)
			pfixlen=128;
		char bf[128+32];
		const char *p,*nam[]={"Min_","Max_","MinMaxDiff_","Med_", "Ave_","Stdev_","InRng_","PNorm_"};
		memcpy(bf,prefix,pfixlen);
		char *d=bf+pfixlen;
		const char *pix="PixelCount_";
		for(p=pix;*d++=*p++;);			// strcat variable name part
		d[0]='\0';						// nul term (d[-1] is currently nul)
		int chmin=rgb.chmin;
		int chmax=rgb.chmax;
		AVSValue n,var;
		for(chan=chmin;chan<=chmax;++chan) {
			d[-1]=chan+'0';					// Append channel number at string nul term - 1
			var = GetVar(env,bf);
			n = rgb.pixels;
			env->SetVar(var.Defined()?bf:env->SaveString(bf),n);
		}
		if(flgs==0) {
			return 0;					// Set only PixelCount_x
		}
		for(i=RTMIN;i<=RTPNORM;++i) {
			if(flgs & (1 << i)) {
				d=bf+pfixlen;
				for(p=nam[i];*d++=*p++;);			// strcat variable name part
				d[0]='\0';							// nul term (d[-1] is currently nul)
				for(chan=chmin;chan<=chmax;++chan) {
					d[-1]=chan+'0';					// Append channel number at string nul term - 1
					var     =	GetVar(env,bf);
					if(i<RTAVE) {
						n = rgb.chans[chan].idat[i];
					} else {
						n = rgb.chans[chan].ddat[i-RTAVE];
					}
					env->SetVar(var.Defined()?bf:env->SaveString(bf),n);
				}
			}
		}
	}
	return  flgs;
}

