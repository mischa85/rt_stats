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

int __cdecl PVF_MaskCountLuma_Planar(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,
			const bool altscan,unsigned int *cnt,const PVideoFrame &msrc,const int maskmin,const int maskmax) {
	const int ystep	   = (altscan) ? 2:1;
    const int pitch    = src->GetPitch(PLANAR_Y);
	const int ystride  = pitch*ystep;
    const BYTE  *srcp  = src->GetReadPtr(PLANAR_Y)  + (yy * pitch) + xx;
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
	const int   mpitch      = msrc->GetPitch(PLANAR_Y);
	const BYTE  *msrcp      = msrc->GetReadPtr(PLANAR_Y) + (yy * mpitch) + xx;
	const int   mystride	= mpitch*ystep;
	for(int i=256;--i>=0;cnt[i]=0);
    unsigned int Pixels = 0;
	if(ww == 1) {														// Special case for single pixel width
		for(int y=yhit ; --y>=0; ) {
			if(msrcp[0]>=maskmin && msrcp[0]<=maskmax) {
				++cnt[srcp[0]];
				++Pixels;
			}
			srcp  += ystride;
			msrcp += mystride;
		}
	} else {
		const int eodd = (ww & 0x07);
		const int wm8 = ww - eodd;
		for(int y=yhit; --y>=0 ;) {
			switch(eodd) {
			case 7:	if(msrcp[wm8+6]>=maskmin && msrcp[wm8+6]<=maskmax) {++cnt[srcp[wm8+6]];++Pixels;}
			case 6:	if(msrcp[wm8+5]>=maskmin && msrcp[wm8+5]<=maskmax) {++cnt[srcp[wm8+5]];++Pixels;}
			case 5:	if(msrcp[wm8+4]>=maskmin && msrcp[wm8+4]<=maskmax) {++cnt[srcp[wm8+4]];++Pixels;}
			case 4:	if(msrcp[wm8+3]>=maskmin && msrcp[wm8+3]<=maskmax) {++cnt[srcp[wm8+3]];++Pixels;}
			case 3:	if(msrcp[wm8+2]>=maskmin && msrcp[wm8+2]<=maskmax) {++cnt[srcp[wm8+2]];++Pixels;}
			case 2:	if(msrcp[wm8+1]>=maskmin && msrcp[wm8+1]<=maskmax) {++cnt[srcp[wm8+1]];++Pixels;}
			case 1:	if(msrcp[wm8+0]>=maskmin && msrcp[wm8+0]<=maskmax) {++cnt[srcp[wm8+0]];++Pixels;}
			case 0:	;
			}
			for(int x=wm8; (x-=8)>=0 ; ) {
				if(msrcp[x+7]>=maskmin && msrcp[x+7]<=maskmax) {++cnt[srcp[x+7]];++Pixels;}
				if(msrcp[x+6]>=maskmin && msrcp[x+6]<=maskmax) {++cnt[srcp[x+6]];++Pixels;}
				if(msrcp[x+5]>=maskmin && msrcp[x+5]<=maskmax) {++cnt[srcp[x+5]];++Pixels;}
				if(msrcp[x+4]>=maskmin && msrcp[x+4]<=maskmax) {++cnt[srcp[x+4]];++Pixels;}
				if(msrcp[x+3]>=maskmin && msrcp[x+3]<=maskmax) {++cnt[srcp[x+3]];++Pixels;}
				if(msrcp[x+2]>=maskmin && msrcp[x+2]<=maskmax) {++cnt[srcp[x+2]];++Pixels;}
				if(msrcp[x+1]>=maskmin && msrcp[x+1]<=maskmax) {++cnt[srcp[x+1]];++Pixels;}
				if(msrcp[x+0]>=maskmin && msrcp[x+0]<=maskmax) {++cnt[srcp[x+0]];++Pixels;}
			}
			srcp  += ystride;
			msrcp += mystride;
		}
	}
    return Pixels;
}

int __cdecl PVF_MaskCountLuma_YUY2(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,
			const bool altscan,unsigned int *cnt,const PVideoFrame &msrc,const int maskmin,const int maskmax) {
	const int ystep	   = (altscan) ? 2:1;
    const int pitch    = src->GetPitch();
	const int ystride  = pitch*ystep;
    const BYTE  *srcp  = src->GetReadPtr()  + (yy * pitch)	 + (xx *2);
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
	const int   mpitch      = msrc->GetPitch(PLANAR_Y);
	const BYTE  *msrcp      = msrc->GetReadPtr(PLANAR_Y) + (yy * mpitch) + xx;
	const int   mystride	= mpitch*ystep;
	for(int i=256;--i>=0;cnt[i]=0);
    unsigned int Pixels = 0;
	if(ww == 1) {														// Special case for single pixel width
		for(int y=yhit ; --y>=0 ; ) {
			if(msrcp[0]>=maskmin && msrcp[0]<=maskmax) {++cnt[srcp[0]]; ++Pixels;}
			srcp  += ystride;
			msrcp += mystride;
		}
	} else {
		const int eodd = (ww & 0x07);
		const int wm8 = (ww - eodd) * 2;
		for(int y=yhit; --y>=0 ;) {
			switch(eodd) {
			case 7:	if(msrcp[wm8+6]>=maskmin && msrcp[wm8+6]<=maskmax) {++cnt[srcp[wm8+(6*2)]]; ++Pixels;}
			case 6:	if(msrcp[wm8+5]>=maskmin && msrcp[wm8+5]<=maskmax) {++cnt[srcp[wm8+(5*2)]]; ++Pixels;}
			case 5:	if(msrcp[wm8+4]>=maskmin && msrcp[wm8+4]<=maskmax) {++cnt[srcp[wm8+(4*2)]]; ++Pixels;}
			case 4:	if(msrcp[wm8+3]>=maskmin && msrcp[wm8+3]<=maskmax) {++cnt[srcp[wm8+(3*2)]]; ++Pixels;}
			case 3:	if(msrcp[wm8+2]>=maskmin && msrcp[wm8+2]<=maskmax) {++cnt[srcp[wm8+(2*2)]]; ++Pixels;}
			case 2:	if(msrcp[wm8+1]>=maskmin && msrcp[wm8+1]<=maskmax) {++cnt[srcp[wm8+(1*2)]]; ++Pixels;}
			case 1:	if(msrcp[wm8+0]>=maskmin && msrcp[wm8+0]<=maskmax) {++cnt[srcp[wm8+(0*2)]]; ++Pixels;}
			case 0:	;
			}
			for(int x=wm8; (x-=(8*2))>=0 ; ) {
				if(msrcp[x+7]>=maskmin && msrcp[x+7]<=maskmax) {++cnt[srcp[x+(7*2)]]; ++Pixels;}
				if(msrcp[x+6]>=maskmin && msrcp[x+6]<=maskmax) {++cnt[srcp[x+(6*2)]]; ++Pixels;}
				if(msrcp[x+5]>=maskmin && msrcp[x+5]<=maskmax) {++cnt[srcp[x+(5*2)]]; ++Pixels;}
				if(msrcp[x+4]>=maskmin && msrcp[x+4]<=maskmax) {++cnt[srcp[x+(4*2)]]; ++Pixels;}
				if(msrcp[x+3]>=maskmin && msrcp[x+3]<=maskmax) {++cnt[srcp[x+(3*2)]]; ++Pixels;}
				if(msrcp[x+2]>=maskmin && msrcp[x+2]<=maskmax) {++cnt[srcp[x+(2*2)]]; ++Pixels;}
				if(msrcp[x+1]>=maskmin && msrcp[x+1]<=maskmax) {++cnt[srcp[x+(1*2)]]; ++Pixels;}
				if(msrcp[x+0]>=maskmin && msrcp[x+0]<=maskmax) {++cnt[srcp[x+(0*2)]]; ++Pixels;}
			}
			srcp  += ystride;
			msrcp += mystride;
		}
	}
    return Pixels;
}

int __cdecl PVF_MaskCountLuma_RGB(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,const bool altscan,
			unsigned int *cnt,const PVideoFrame &msrc,const int maskmin, const int maskmax,const int matrix,const bool IsRGB32) {
	int xstep = (IsRGB32) ? 4 : 3;
	const int ystep	   = (altscan) ? 2:1;
    const int pitch    = src->GetPitch();
    const int height   = src->GetHeight();
	const int ystride  = pitch*ystep;
	const BYTE  *srcp  = src->GetReadPtr() + ((height-1 - yy) * pitch) + (xx * xstep);
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
	const int   mpitch      = msrc->GetPitch(PLANAR_Y);
	const BYTE  *msrcp      = msrc->GetReadPtr(PLANAR_Y) + (yy * mpitch) + xx;
	const int   mystride	= mpitch*ystep;
	const int mat = matrix & 0x03;
	for(int i=256;--i>=0;cnt[i]=0);
    unsigned int Pixels = 0;
    // RGB to YUV-Y Conversion
	// Matrix: Default=0=REC601 : 1=REC709 : 2 = PC601 : 3 = PC709
	double				Kr,Kb;
	int					S_y,offset_y;
	if(mat & 0x01)		{Kr = 0.2126; Kb        = 0.0722;}			// 709  1 or 3
	else                {Kr = 0.2990; Kb        = 0.1140;}			// 601  0 or 2
	if(mat & 0x02)		{S_y = 255   ; offset_y  = 0;}				// PC   2 or 3
	else                {S_y = 219   ; offset_y  = 16;}				// TV   0 or 1
	const int			shift	=   15;
	const int           half    =   1 << (shift - 1);
	const double        mulfac  =   double(1<<shift);
	double              Kg      =   1.0 - Kr - Kb;
	const int           Srgb    =   255;
	const int			Yb = int(S_y  * Kb        * mulfac / Srgb + 0.5); //B
	const int			Yg = int(S_y  * Kg        * mulfac / Srgb + 0.5); //G
	const int			Yr = int(S_y  * Kr        * mulfac / Srgb + 0.5); //R
	const int			OffyPlusHalf = (offset_y<<shift) + half;
	//
	if(ww == 1) {														// Special case for single pixel width
		for(int y=yhit ; --y>=0; ) {
			if(msrcp[0]>=maskmin && msrcp[0]<=maskmax) {
				++cnt[(srcp[0] * Yb + srcp[1] * Yg + srcp[2] * Yr + OffyPlusHalf) >> shift];
				++Pixels;
			}
			srcp  -= ystride;
			msrcp += mystride;
		}
	} else {
		const int eodd = (ww & 0x07);
		const int wm = (ww - eodd);
		const int wm8 = wm * xstep;
		if(IsRGB32) {
			for(int y=yhit; --y>=0 ; ) {
				switch(eodd) {
				case 7:	
					if(msrcp[wm+6]>=maskmin && msrcp[wm+6]<=maskmax) {
						++cnt[(srcp[wm8+(6*4+0)]*Yb + srcp[wm8+(6*4+1)]*Yg + srcp[wm8+(6*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 6:	
					if(msrcp[wm+5]>=maskmin && msrcp[wm+5]<=maskmax) {
						++cnt[(srcp[wm8+(5*4+0)]*Yb + srcp[wm8+(5*4+1)]*Yg + srcp[wm8+(5*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 5:	
					if(msrcp[wm+4]>=maskmin && msrcp[wm+4]<=maskmax) {
						++cnt[(srcp[wm8+(4*4+0)]*Yb + srcp[wm8+(4*4+1)]*Yg + srcp[wm8+(4*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 4:	
					if(msrcp[wm+3]>=maskmin && msrcp[wm+3]<=maskmax) {
						++cnt[(srcp[wm8+(3*4+0)]*Yb + srcp[wm8+(3*4+1)]*Yg + srcp[wm8+(3*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 3:	
					if(msrcp[wm+2]>=maskmin && msrcp[wm+2]<=maskmax) {
						++cnt[(srcp[wm8+(2*4+0)]*Yb + srcp[wm8+(2*4+1)]*Yg + srcp[wm8+(2*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 2:	
					if(msrcp[wm+1]>=maskmin && msrcp[wm+1]<=maskmax) {
						++cnt[(srcp[wm8+(1*4+0)]*Yb + srcp[wm8+(1*4+1)]*Yg + srcp[wm8+(1*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 1:	
					if(msrcp[wm+0]>=maskmin && msrcp[wm+0]<=maskmax) {
						++cnt[(srcp[wm8+(0*4+0)]*Yb + srcp[wm8+(0*4+1)]*Yg + srcp[wm8+(0*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 0:	;
				}
				for(int x=wm; (x-=8)>=0 ; ) {
					const int xm = x*4;
					if(msrcp[x+7]>=maskmin && msrcp[x+7]<=maskmax) {
						++cnt[(srcp[xm+(7*4+0)]*Yb + srcp[xm+(7*4+1)]*Yg + srcp[xm+(7*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+6]>=maskmin && msrcp[x+6]<=maskmax) {
						++cnt[(srcp[xm+(6*4+0)]*Yb + srcp[xm+(6*4+1)]*Yg + srcp[xm+(6*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+5]>=maskmin && msrcp[x+5]<=maskmax) {
						++cnt[(srcp[xm+(5*4+0)]*Yb + srcp[xm+(5*4+1)]*Yg + srcp[xm+(5*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+4]>=maskmin && msrcp[x+4]<=maskmax) {
						++cnt[(srcp[xm+(4*4+0)]*Yb + srcp[xm+(4*4+1)]*Yg + srcp[xm+(4*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+3]>=maskmin && msrcp[x+3]<=maskmax) {
						++cnt[(srcp[xm+(3*4+0)]*Yb + srcp[xm+(3*4+1)]*Yg + srcp[xm+(3*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+2]>=maskmin && msrcp[x+2]<=maskmax) {
						++cnt[(srcp[xm+(2*4+0)]*Yb + srcp[xm+(2*4+1)]*Yg + srcp[xm+(2*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+1]>=maskmin && msrcp[x+1]<=maskmax) {
						++cnt[(srcp[xm+(1*4+0)]*Yb + srcp[xm+(1*4+1)]*Yg + srcp[xm+(1*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+0]>=maskmin && msrcp[x+0]<=maskmax) {
						++cnt[(srcp[xm+(0*4+0)]*Yb + srcp[xm+(0*4+1)]*Yg + srcp[xm+(0*4+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				}
				srcp  -= ystride;
				msrcp += mystride;
			}
		} else {
			for(int y=yhit; --y>=0 ; ) {
				switch(eodd) {
				case 7:	
					if(msrcp[wm+6]>=maskmin && msrcp[wm+6]<=maskmax) {
						++cnt[(srcp[wm8+(6*3+0)]*Yb + srcp[wm8+(6*3+1)]*Yg + srcp[wm8+(6*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 6:	
					if(msrcp[wm+5]>=maskmin && msrcp[wm+5]<=maskmax) {
						++cnt[(srcp[wm8+(5*3+0)]*Yb + srcp[wm8+(5*3+1)]*Yg + srcp[wm8+(5*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 5:	
					if(msrcp[wm+4]>=maskmin && msrcp[wm+4]<=maskmax) {
						++cnt[(srcp[wm8+(4*3+0)]*Yb + srcp[wm8+(4*3+1)]*Yg + srcp[wm8+(4*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 4:	
					if(msrcp[wm+3]>=maskmin && msrcp[wm+3]<=maskmax) {
						++cnt[(srcp[wm8+(3*3+0)]*Yb + srcp[wm8+(3*3+1)]*Yg + srcp[wm8+(3*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 3:	
					if(msrcp[wm+2]>=maskmin && msrcp[wm+2]<=maskmax) {
						++cnt[(srcp[wm8+(2*3+0)]*Yb + srcp[wm8+(2*3+1)]*Yg + srcp[wm8+(2*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 2:	
					if(msrcp[wm+1]>=maskmin && msrcp[wm+1]<=maskmax) {
						++cnt[(srcp[wm8+(1*3+0)]*Yb + srcp[wm8+(1*3+1)]*Yg + srcp[wm8+(1*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 1:	
					if(msrcp[wm+0]>=maskmin && msrcp[wm+0]<=maskmax) {
						++cnt[(srcp[wm8+(0*3+0)]*Yb + srcp[wm8+(0*3+1)]*Yg + srcp[wm8+(0*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				case 0:	;
				}
				for(int x=wm; (x-=8)>=0 ; ) {
					const int xm = x*3;
					if(msrcp[x+7]>=maskmin && msrcp[x+7]<=maskmax) {
						++cnt[(srcp[xm+(7*3+0)]*Yb + srcp[xm+(7*3+1)]*Yg + srcp[xm+(7*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+6]>=maskmin && msrcp[x+6]<=maskmax) {
						++cnt[(srcp[xm+(6*3+0)]*Yb + srcp[xm+(6*3+1)]*Yg + srcp[xm+(6*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+5]>=maskmin && msrcp[x+5]<=maskmax) {
						++cnt[(srcp[xm+(5*3+0)]*Yb + srcp[xm+(5*3+1)]*Yg + srcp[xm+(5*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+4]>=maskmin && msrcp[x+4]<=maskmax) {
						++cnt[(srcp[xm+(4*3+0)]*Yb + srcp[xm+(4*3+1)]*Yg + srcp[xm+(4*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+3]>=maskmin && msrcp[x+3]<=maskmax) {
						++cnt[(srcp[xm+(3*3+0)]*Yb + srcp[xm+(3*3+1)]*Yg + srcp[xm+(3*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+2]>=maskmin && msrcp[x+2]<=maskmax) {
						++cnt[(srcp[xm+(2*3+0)]*Yb + srcp[xm+(2*3+1)]*Yg + srcp[xm+(2*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+1]>=maskmin && msrcp[x+1]<=maskmax) {
						++cnt[(srcp[xm+(1*3+0)]*Yb + srcp[xm+(1*3+1)]*Yg + srcp[xm+(1*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
					if(msrcp[x+0]>=maskmin && msrcp[x+0]<=maskmax) {
						++cnt[(srcp[xm+(0*3+0)]*Yb + srcp[xm+(0*3+1)]*Yg + srcp[xm+(0*3+2)]*Yr + OffyPlusHalf) >> shift];
						++Pixels;
					}
				}
				srcp  -= ystride;
				msrcp += mystride;
			}
		}
	}
    return Pixels;
}

int __cdecl RT_MYstats_Lo(int flgs,const AVSValue &std,const AVSValue &xtra,MYLO &ylo,const char*Name,IScriptEnvironment* env,unsigned int*histp) {

	const char *myName=(Name==NULL)?"RT_MYstatLo: ":Name;

	flgs = flgs & (RTMIN_F|RTMAX_F|RTMINMAX_F|RTMEDIAN_F|RTAVE_F|RTSTDEV_F|RTYINRNG_F|RTPNORM_F|RTHIST_F);
	if(histp==NULL) flgs &= (~RTHIST_F);

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

	if(!vi.IsPlanar() && !vi.IsYUY2() && !vi.IsRGB32() && !vi.IsRGB24()) {
		env->ThrowError("%sPlanar or YUY2 or RGB32 or RGB24 ONLY",myName);
	}

	if(vi.width==0 || vi.num_frames==0)
	env->ThrowError("%sClip must have video",myName);

	int xx=			std[STD_XX].AsInt(0);
	int yy=			std[STD_YY].AsInt(0);
	int	ww=			std[STD_WW].AsInt(0);
	int	hh=			std[STD_HH].AsInt(0);
	bool altscan=	std[STD_LACED].AsBool(false);

	double thresh=	xtra[XTRA_THRESH].AsFloat(0.0);
	thresh = (thresh<0.0) ? 0.0 : (thresh>100.0) ? 100.0 : thresh;
	thresh = thresh / 100.0;

	int lo=			xtra[XTRA_LO].AsInt(128);
	lo = (lo < 0) ? 0 : (lo > 255) ? 255 : lo;
	int hi=			xtra[XTRA_HI].AsInt(lo);
	hi = (hi < 0) ? 0 : (hi > 255) ? 255 : hi;

	double mu	=xtra[XTRA_MU].AsFloat(0.0);
	int div		=xtra[XTRA_D].AsInt(1);
	int power	=xtra[XTRA_P].AsInt(1);
	int up		=xtra[XTRA_U].AsInt(1);


	if(mu <0.0	 || mu>255.0)					env->ThrowError("%sInvalid mu (0.0->255.0)",myName);
	if(div <1	 || div>255)					env->ThrowError("%sInvalid d (1->255)",myName);
	if(power < 1 || power>16)					env->ThrowError("%sInvalid p (1->16)",myName);
	if(up <1     || up>255)						env->ThrowError("%sInvalid u (1->255)",myName);

	const int matrix=xtra[XTRA_RGBIX].AsInt(vi.width>1100 || vi.height>600?3:2);
		// Matrix: Default=0=REC601 : 1=REC709 : 2 = PC601 : 3 = PC709
    if((matrix & 0xFFFFFFFC) && vi.IsRGB())		env->ThrowError("%sRGB matrix 0 to 3 Only",myName);
	
	if(ww <= 0) ww += vi.width  - xx;
	if(hh <= 0) hh += vi.height - yy;
	if(altscan && ((hh & 0x01) == 0)) --hh;	// If altscan then ensure ODD number of lines, last line other field does not count.
	altscan=(altscan && hh!=1);
		
	if(xx <  0 || xx >=vi.width)				env->ThrowError("%sInvalid X coord",myName);
	if(yy <  0 || yy >=vi.height)				env->ThrowError("%sInvalid Y coord",myName);
	if(ww <= 0 || xx + ww > vi.width)			env->ThrowError("%sInvalid W coord",myName);
	if(hh <= 0 || yy + hh > vi.height)			env->ThrowError("%sInvalid H coord",myName);

	int Pixels = (ww * ((altscan)?(hh+1)>>1:hh));
	double mean=0.0;

	unsigned int cntbuf[256],*cnt;

	cnt= (flgs & RTHIST_F) ? histp : cntbuf;

    PVideoFrame src     = child->GetFrame(n,env);
	PClip mask=NULL;
	const int maskmin=xtra[XTRA_MASKMIN].AsInt(128);
	const int maskmax=xtra[XTRA_MASKMAX].AsInt(255);
	// If full range maskmin/max, then dont bother with mask
	if((maskmin!=0 || maskmax!=255) && xtra[XTRA_MASK].IsClip()) {
		mask  = xtra[XTRA_MASK].AsClip();												// Mask

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

		PVideoFrame msrc  = mask->GetFrame(n,env);

		if(vi.IsPlanar())		
			Pixels = PVF_MaskCountLuma_Planar(src,xx,yy,ww,hh,altscan,cnt,msrc,maskmin,maskmax);
		else if(vi.IsYUY2())	
			Pixels = PVF_MaskCountLuma_YUY2(src,xx,yy,ww,hh,altscan,cnt,msrc,maskmin,maskmax);
		else					
			Pixels = PVF_MaskCountLuma_RGB(src,xx,yy,ww,hh,altscan,cnt,msrc,maskmin,maskmax,matrix,vi.IsRGB32());
	} else {
		if(flgs==RTAVE_F) {
			if(vi.IsPlanar())		
				mean = PVF_AverageLuma_Planar(src,xx,yy,ww,hh,altscan);
			else if(vi.IsYUY2())	
				mean = PVF_AverageLuma_YUY2(src,xx,yy,ww,hh,altscan);
			else					
				mean = PVF_AverageLuma_RGB(src,xx,yy,ww,hh,altscan,matrix,vi.IsRGB32());
		} else {
			if(vi.IsPlanar())		
				Pixels = PVF_CountLuma_Planar(src,xx,yy,ww,hh,altscan,cnt);
			else if(vi.IsYUY2())	
				Pixels = PVF_CountLuma_YUY2(src,xx,yy,ww,hh,altscan,cnt);
			else					
				Pixels = PVF_CountLuma_RGB(src,xx,yy,ww,hh,altscan,cnt,matrix,vi.IsRGB32());
		}
	}

	int i;
	for(i=RTMIN;i<=RTMEDIAN;ylo.dat.idat[i++]=0);
	for(i=0;i<=(RTPNORM-RTAVE);ylo.dat.ddat[i++]=0.0);
	ylo.pixels = Pixels;

	if(Pixels==0)
		return 0;		// Got Nothing, No valid pixels

	if(mask == NULL && flgs==RTAVE_F) {
	    ylo.dat.ddat[RTAVE-RTAVE] = mean;
	} else {
		int rmin;

		if(flgs & (RTAVE_F | RTSTDEV_F)) {
			if((flgs & RTAVE_F) || (Pixels>1)) {
				int i;
				int64_t acc		 = 0;
				for(acc=0,i=256;--i>=0;) {
					acc += cnt[i] * i;
				}
				mean = (double)acc / Pixels;
				if(flgs & RTAVE_F) {
					ylo.dat.ddat[RTAVE-RTAVE]=mean;
				}
			}
		}
		if(flgs & (RTMIN_F | RTMINMAX_F)) {
			rmin=255;
			const unsigned int lim = (unsigned int)(Pixels * thresh);		// making smaller than pixels, no overflow
			unsigned int sm=0;
			for(i=0;i<256;++i) {
				sm += cnt[i];
				if(sm> lim) {
					rmin=i;
					break;
				}
			}
			if(flgs & RTMIN_F) {
				ylo.dat.idat[RTMIN]=rmin;
			}
		}
		if(flgs & (RTMAX_F | RTMINMAX_F)) {
			int rmax=0;
			const unsigned int lim = (unsigned int)(Pixels * thresh);		// making smaller than pixels, no overflow
			unsigned int sm=0;
			for(i=256;--i>=0;) {
				sm += cnt[i];
				if(sm > lim) {
					rmax=i;
					break;
				}
			}
			if(flgs & RTMAX_F) {
				ylo.dat.idat[RTMAX]=rmax;
			}
			if(flgs & RTMINMAX_F) {
				ylo.dat.idat[RTMINMAX]=rmax-rmin;
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
			ylo.dat.idat[RTMEDIAN]=rmed; 
		}
		if(flgs & RTYINRNG_F) {
			unsigned int sm=0;
			for(i=lo;i<=hi;++i) {
				sm += cnt[i];
			}
			ylo.dat.ddat[RTYINRNG-RTAVE]=double(sm) / Pixels;
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
			ylo.dat.ddat[RTSTDEV-RTAVE]=std;
		}

		if(flgs & RTPNORM_F) {
			const double mu_d	=double(mu);
			const double div_d	=double(div);
			const double power_d=double(power);
			double Sum_DiffPowered = 0.0;
			// At this point Pixels known to be > 0, otherwise function already returned 0
			for (i=256;--i>=0;) {
				const unsigned int count=cnt[i];					// Count of pixels with value i
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
				dprintf("RT_MYPNorm: ***ERROR*** E=%X mu=%f div=%d power=%d\n",e,mu_d,div,power);
				dprintf("RT_MYPNorm:             DiffPowered=%f 1/p=%f pn=%f\n",DiffPowered, 1.0 / power_d,pn);
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
			ylo.dat.ddat[RTPNORM-RTAVE] = pn;
		}
	}
	return flgs;
}

// ----------------------------------------------------------------------------
//                     RT Standard
// ----------------------------------------------------------------------------

AVSValue __cdecl RT_YPlaneMin(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_YPlaneMin: ";
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_THRESH]	= args[8];
	xtra[XTRA_RGBIX]	= args[9];	// RGBIX = matrix
	xtra[XTRA_MASK]		= args[10];
	xtra[XTRA_MASKMIN]	= args[11];
	xtra[XTRA_MASKMAX]	= args[12];
	MYLO mylo;
	RT_MYstats_Lo(RTMIN_F,args,AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
	return mylo.dat.idat[RTMIN];
}

AVSValue __cdecl RT_YPlaneMax(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_YPlaneMax: ";
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_THRESH]	= args[8];
	xtra[XTRA_RGBIX]	= args[9];	// RGBIX = matrix
	xtra[XTRA_MASK]		= args[10];
	xtra[XTRA_MASKMIN]	= args[11];
	xtra[XTRA_MASKMAX]	= args[12];
	MYLO mylo;
	RT_MYstats_Lo(RTMAX_F,args,AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
	return mylo.dat.idat[RTMAX];
}

AVSValue __cdecl RT_YPlaneMinMaxDifference(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_YPlaneMinMaxDifference: ";
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_THRESH]	= args[8];
	xtra[XTRA_RGBIX]	= args[9];	// RGBIX = matrix
	xtra[XTRA_MASK]		= args[10];
	xtra[XTRA_MASKMIN]	= args[11];
	xtra[XTRA_MASKMAX]	= args[12];
	MYLO mylo;
	RT_MYstats_Lo(RTMINMAX_F,args,AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
	return mylo.dat.idat[RTMINMAX];
}

AVSValue __cdecl RT_YPlaneMedian(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_YPlaneMedian: ";
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_RGBIX]	= args[8];	// RGBIX = matrix
	xtra[XTRA_MASK]		= args[9];
	xtra[XTRA_MASKMIN]	= args[10];
	xtra[XTRA_MASKMAX]	= args[11];
	MYLO mylo;
	RT_MYstats_Lo(RTMEDIAN_F,args,AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
	return mylo.dat.idat[RTMEDIAN];
}

AVSValue __cdecl RT_AverageLuma(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_AverageLuma: ";
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_RGBIX]	= args[8];	// RGBIX = matrix
	xtra[XTRA_MASK]		= args[9];
	xtra[XTRA_MASKMIN]	= args[10];
	xtra[XTRA_MASKMAX]	= args[11];
	MYLO mylo;
	RT_MYstats_Lo(RTAVE_F,args,AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
	return mylo.dat.ddat[RTAVE-RTAVE];
}

AVSValue __cdecl RT_YPlaneStdev(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_YPlaneStdev: ";
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_RGBIX]	= args[8];	// RGBIX = matrix
	xtra[XTRA_MASK]		= args[9];
	xtra[XTRA_MASKMIN]	= args[10];
	xtra[XTRA_MASKMAX]	= args[11];
	MYLO mylo;
	RT_MYstats_Lo(RTSTDEV_F,args,AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
	return mylo.dat.ddat[RTSTDEV-RTAVE];
}

AVSValue __cdecl RT_YInRange(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_YInRange: ";
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_RGBIX]	= args[8];	// RGBIX = matrix
	xtra[XTRA_LO]		= args[9];
	xtra[XTRA_HI]		= args[10];
	xtra[XTRA_MASK]		= args[11];
	xtra[XTRA_MASKMIN]	= args[12];
	xtra[XTRA_MASKMAX]	= args[13];
	MYLO mylo;
	RT_MYstats_Lo(RTYINRNG_F,args,AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
	return  mylo.dat.ddat[RTYINRNG-RTAVE];
}

AVSValue __cdecl RT_YPNorm(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_YPNorm: ";
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_RGBIX]	= args[8];	// RGBIX = matrix
	xtra[XTRA_MU]		= args[9];
	xtra[XTRA_D]		= args[10];
	xtra[XTRA_P]		= args[11];
	xtra[XTRA_U]		= args[12];
	xtra[XTRA_MASK]		= args[13];
	xtra[XTRA_MASKMIN]	= args[14];
	xtra[XTRA_MASKMAX]	= args[15];
	MYLO mylo;
	RT_MYstats_Lo(RTPNORM_F,args,AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
	return  mylo.dat.ddat[RTPNORM-RTAVE];
}

AVSValue __cdecl RT_YStats(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_YStats: ";
	int RTALLY=(RTMIN_F|RTMAX_F|RTMINMAX_F|RTMEDIAN_F|RTAVE_F|RTSTDEV_F|RTYINRNG_F|RTPNORM_F);
	int flgs=(args[12].AsInt(RTALLY)) & RTALLY;
	if(flgs) {
		const char *prefix=args[13].AsString("YS_");
		const char *p,*nam[]={"yMin","yMax","yMinMaxDiff","yMed", "yAve","yStdev","yInRng","yPNorm"};
		AVSValue xtra[XTRA_SIZE];
		xtra[XTRA_THRESH]	= args[8];
		xtra[XTRA_RGBIX]	= args[9];	// RGBIX = matrix
		xtra[XTRA_LO]		= args[10];
		xtra[XTRA_HI]		= args[11];
		xtra[XTRA_MU]		= args[14];
		xtra[XTRA_D]		= args[15];
		xtra[XTRA_P]		= args[16];
		xtra[XTRA_U]		= args[17];
		xtra[XTRA_MASK]		= args[18];
		xtra[XTRA_MASKMIN]	= args[19];
		xtra[XTRA_MASKMAX]	= args[20];

		MYLO mylo;
		flgs=RT_MYstats_Lo(flgs,args,AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
		int pfixlen=int(strlen(prefix));
		if(pfixlen>128)
			pfixlen=128;
		char bf[128+32];
		memcpy(bf,prefix,pfixlen);
		char *d=bf+pfixlen;
		const char *pix="PixelCount";
		for(p=pix;*d++=*p++;);			// strcat variable name part
		AVSValue n,var;
		var = GetVar(env,bf);
		n = mylo.pixels;
		env->SetVar(var.Defined()?bf:env->SaveString(bf),n);
		if(flgs==0)
			return 0;					// Set only YS_PixelCount
		int i;
		for(i=RTMIN;i<=RTPNORM;++i) {
			if(flgs & (1 << i)) {
				char *d=bf+pfixlen;
				for(p=nam[i];*d++=*p++;);			// strcat variable name part
				AVSValue n,var     =	GetVar(env,bf);
				if(i<RTAVE)
					n = mylo.dat.idat[i];
				else
					n = mylo.dat.ddat[i-RTAVE];
				env->SetVar(var.Defined()?bf:env->SaveString(bf),n);
			}
		}
	}
	return  flgs;
}


