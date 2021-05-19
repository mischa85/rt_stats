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

//	extern double __cdecl PVF_FrameMovement_Planar(const PVideoFrame &src,const PVideoFrame &src2,const double CW,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,const bool altscan,const bool ChromaI,const int blkw,const int blkh,const int olapx, const int olapy,const double bTh,int *coordsP,double *PercAboveTh,double *BlkAveDf,int *nAboveTh,int *TotalBlks);
//	extern double __cdecl PVF_FrameMovement_YUY2(const PVideoFrame &src,const PVideoFrame &src2,const double CW,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,const bool altscan,const int blkw,const int blkh,const int olapx, const int olapy,const double bTh,int *coordsP,double *PercAboveTh,double *BlkAveDf,int *nAboveTh,int *TotalBlks);
//	extern double __cdecl PVF_FrameMovement_RGB(const PVideoFrame &src,const PVideoFrame &src2,const bool doColor,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,const bool altscan,const int Matrix,const bool IsRGB32,const int blkw,const int blkh,const int olapx, const int olapy,const double bTh,int *coordsP,double *PercAboveTh,double *BlkAveDf,int *nAboveTh,int *TotalBlks);

double __cdecl PVF_FrameMovement_Planar(const PVideoFrame &src,const PVideoFrame &src2,
		const double CW,							   
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,
		const bool altscan,const bool ChromaI,
		const int blkw,const int blkh,const int olapx, const int olapy,
		const double bTh,
		int *coordsP,double *PercAboveTh,double *BlkAveDf,int *nAboveTh,int *TotalBlks
		) {
	const bool DoColor = (CW>0.0);
	double perc=0.0,sum=0.0;
	double fmx,mx=0.0;
	int coords[2]={0,0};
	int TotalBlocks=0,nBlksDif=0;
	int gx,gy;
	const int ystep=blkh-olapy;
	const int xstep=blkw-olapx;
	for(gy=0;gy<hh;gy+=ystep) {
		int yoff = (gy+blkh <=hh) ? gy : hh-blkh;
		for(gx=0;gx<ww;gx+=xstep) {
			int xoff = (gx+blkw <=ww) ? gx : ww-blkw;
			fmx = (DoColor) 
				? PVF_PixelDifference_Planar(src,src2,CW,xx+xoff,yy+yoff,blkw,blkh,xx2+xoff,yy2+yoff,altscan,ChromaI)
				: PVF_LumaDifference_Planar(src,src2,xx+xoff,yy+yoff,blkw,blkh,xx2+xoff,yy2+yoff,altscan);
			++TotalBlocks;
			if(fmx>mx) {
				mx=fmx;
				coords[0]=xoff;
				coords[1]=yoff;
			}
			if(fmx>bTh) {
				++nBlksDif;
			}
			sum += fmx;
		}
	}
	if(coordsP!=NULL) {
		coordsP[0] = coords[0];
		coordsP[1] = coords[1];
	}
	if(PercAboveTh!=NULL)	*PercAboveTh = (nBlksDif * 100.0) / TotalBlocks;
	if(BlkAveDf!=NULL)		*BlkAveDf  = sum / TotalBlocks;
	if(nAboveTh!=NULL)		*nAboveTh  = nBlksDif;
	if(TotalBlks!=NULL)		*TotalBlks = TotalBlocks;
	return mx;
}

double __cdecl PVF_FrameMovement_YUY2(const PVideoFrame &src,const PVideoFrame &src2,
		const double CW,							   
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,
		const bool altscan,
		const int blkw,const int blkh,const int olapx, const int olapy,
		const double bTh,
		int *coordsP,double *PercAboveTh,double *BlkAveDf,int *nAboveTh,int *TotalBlks
		) {
	const bool DoColor = (CW>0.0);
	double perc=0.0,sum=0.0;
	double fmx,mx=0.0;
	int coords[2]={0,0};
	int TotalBlocks=0,nBlksDif=0;
	int gx,gy;
	const int ystep=blkh-olapy;
	const int xstep=blkw-olapx;
	for(gy=0;gy<hh;gy+=ystep) {
		int yoff = (gy+blkh <=hh) ? gy : hh-blkh;
		for(gx=0;gx<ww;gx+=xstep) {
			int xoff = (gx+blkw <=ww) ? gx : ww-blkw;
			fmx = (DoColor) 
				? PVF_PixelDifference_YUY2(src,src2,CW,xx+xoff,yy+yoff,blkw,blkh,xx2+xoff,yy2+yoff,altscan)
				: PVF_LumaDifference_YUY2(src,src2,xx+xoff,yy+yoff,blkw,blkh,xx2+xoff,yy2+yoff,altscan);
			++TotalBlocks;
			if(fmx>mx) {
				mx=fmx;
				coords[0]=xoff;
				coords[1]=yoff;
			}
			if(fmx>bTh) {
				++nBlksDif;
			}
			sum += fmx;
		}
	}
	if(coordsP!=NULL) {
		coordsP[0] = coords[0];
		coordsP[1] = coords[1];
	}
	if(PercAboveTh!=NULL)	*PercAboveTh = (nBlksDif * 100.0) / TotalBlocks;
	if(BlkAveDf!=NULL)		*BlkAveDf  = sum / TotalBlocks;
	if(nAboveTh!=NULL)		*nAboveTh  = nBlksDif;
	if(TotalBlks!=NULL)		*TotalBlks = TotalBlocks;
	return mx;
}

double __cdecl PVF_FrameMovement_RGB(const PVideoFrame &src,const PVideoFrame &src2,
		const bool doColor,
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,
		const bool altscan,const int Matrix,const bool IsRGB32,
		const int blkw,const int blkh,const int olapx, const int olapy,
		const double bTh,
		int *coordsP,double *PercAboveTh,double *BlkAveDf,int *nAboveTh,int *TotalBlks
		) {
	double perc=0.0,sum=0.0;
	double fmx,mx=0.0;
	int coords[2]={0,0};
	int TotalBlocks=0,nBlksDif=0;
	int gx,gy;
	const int ystep=blkh-olapy;
	const int xstep=blkw-olapx;
	for(gy=0;gy<hh;gy+=ystep) {
		int yoff = (gy+blkh <=hh) ? gy : hh-blkh;
		for(gx=0;gx<ww;gx+=xstep) {
			int xoff = (gx+blkw <=ww) ? gx : ww-blkw;
			fmx = (doColor) 
				? PVF_PixelDifference_RGB(src,src2,xx+xoff,yy+yoff,blkw,blkh,xx2+xoff,yy2+yoff,altscan,IsRGB32)
				: PVF_LumaDifference_RGB (src,src2,xx+xoff,yy+yoff,blkw,blkh,xx2+xoff,yy2+yoff,altscan,Matrix,IsRGB32);
			++TotalBlocks;
			if(fmx>mx) {
				mx=fmx;
				coords[0]=xoff;
				coords[1]=yoff;
			}
			if(fmx>bTh) {
				++nBlksDif;
			}
			sum += fmx;
		}
	}
	if(coordsP!=NULL) {
		coordsP[0] = coords[0];
		coordsP[1] = coords[1];
	}
	if(PercAboveTh!=NULL)	*PercAboveTh = (nBlksDif * 100.0) / TotalBlocks;
	if(BlkAveDf!=NULL)		*BlkAveDf  = sum / TotalBlocks;
	if(nAboveTh!=NULL)		*nAboveTh  = nBlksDif;
	if(TotalBlks!=NULL)		*TotalBlks = TotalBlocks;
	return mx;
}



double __cdecl RT_FrameMovement_Lo(const AVSValue &args,const char*name,
		int *coordsP,double *PercAboveTh,double *BlkAveDf,int *nAboveTh,int *TotalBlks,
		IScriptEnvironment* env) {
	char * myName="RT_FrameMovement: ";
	PClip child,child2;
	int n,n2;
	bool	gotcur=false;
    if(args[2].IsInt())			{n  = args[2].AsInt(); }					// Frame n
    else {
        AVSValue cn			=	GetVar(env,"current_frame");
        if (!cn.IsInt())		env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
        n                   =	cn.AsInt();									// current_frame
		gotcur=true;
    }
    if(args[3].IsInt())			{n2  = args[3].AsInt(); }					// Frame n2
	else {
		if(gotcur) {
			n2=n;
		} else {
			AVSValue cn			=	GetVar(env,"current_frame");
			if (!cn.IsInt())		env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
			n2                  =	cn.AsInt();								// current_frame
		}
	}

	if(!args[0].IsClip() || !args[1].IsClip())
		env->ThrowError("%sMust have two clips",myName);

	child                 = args[0].AsClip();							// Clip
	child2                = args[1].AsClip();							// Clip2

    const VideoInfo &vi     = child->GetVideoInfo();
    const VideoInfo &vi2    = child2->GetVideoInfo();

	if(!vi.IsSameColorspace(vi2))
		env->ThrowError("%sClips Dissimilar ColorSpace",myName);

	if(!(vi.IsPlanar() && vi.IsYUV()) && !vi.IsYUY2() && !vi.IsRGB24() && !vi.IsRGB32()) {
		env->ThrowError("%sPlanar YUV, YUY2, RGB24 and RGB32 Only",myName);
	}

	if(vi.width==0 || vi.num_frames==0 || vi2.width==0 || vi2.num_frames==0)
	env->ThrowError("%sBoth clips must have video",myName);

	# ifdef AVISYNTH_PLUGIN_25
		if(vi.IsPlanar() &&
			(
				vi.pixel_type != 0xA0000008	// YV12
				&& 
				vi.pixel_type!=0xE0000000   // Y8
			)
		) {
			// Planar but NOT YV12 or Y8, v2.5 Plugin Does NOT support v2.6+ ColorSpace
			env->ThrowError("%s unsupported for colorSpace in v2.5 plugin",myName);			
		}
	# endif

	double CW		= args[4].AsFloat(0.3333333f);

	if(CW< 0.0 || CW > 1.0)
		env->ThrowError("%sChromaWeight 0.0 -> 1.0",myName);

	int		xx=args[5].AsInt(0);
	int		yy=args[6].AsInt(0);
	int		ww=args[7].AsInt(0);
	int		hh=args[8].AsInt(0);
	int		xx2=args[9].AsInt(xx);
	int		yy2=args[10].AsInt(yy);
	bool	altscan=args[11].AsBool(false);
	bool	ChromaI=args[12].AsBool(false);
	ChromaI = (ChromaI && vi.IsYV12());

	const int matrix = args[13].AsInt(vi.width>1100 || vi.height>600 || vi2.width>1100 || vi2.height>600?3:2);

	if(ww <= 0) ww += vi.width  - xx;
	if(hh <= 0) hh += vi.height - yy;
	if(altscan && ((hh & 0x01) == 0)) --hh;		// If altscan then ensure ODD number of lines, last line other field does not count.
	altscan=(altscan && hh!=1);

	if(xx <  0 || xx >=vi.width)				env->ThrowError("%sInvalid X coord(%d)",myName,xx);
	if(yy <  0 || yy >=vi.height)				env->ThrowError("%sInvalid Y coord(%d)",myName,yy);
	if(ww <= 0 || xx + ww > vi.width)			env->ThrowError("%sInvalid W coord for X(%d)",myName,ww);
	if(hh <= 0 || yy + hh > vi.height)			env->ThrowError("%sInvalid H coord for Y(%d)",myName,hh);
	if(xx2 < 0 || xx2 >=vi2.width)				env->ThrowError("%sInvalid X2 coord(%d)",myName,xx2);
	if(yy2 < 0 || yy2 >=vi2.height)				env->ThrowError("%sInvalid Y2 coord(%d)",myName,yy2);
	if(xx2 + ww >vi2.width)						env->ThrowError("%sInvalid W coord for X2",myName);
	if(yy2 + hh >vi2.height)					env->ThrowError("%sInvalid H coord for Y2",myName);
	if(vi.IsRGB() && (matrix<0 || matrix>3))	env->ThrowError("%s 0 <= matrix <= 3",myName);

	int blkw=args[14].AsInt(64);
	int blkh=args[15].AsInt(blkw);

	if(blkw>ww)	blkw=ww;
	if(blkw<8)									env->ThrowError("%sInvalid BlkW, must be at least 8.",myName);
	else if(blkw&0x01)							env->ThrowError("%sBlkW, must be even.",myName); 

	if(blkh>hh)	blkh=hh;
	if(blkh<8)									env->ThrowError("%sInvalid BlkH, must be at least 8.",myName);
	else if(blkh&0x01)							env->ThrowError("%sBlkH, must be even.",myName); 

	const int olapx = args[16].AsInt(blkw/2);
	const int olapy = args[17].AsInt(blkh/2);

	if(olapx<0 || olapx>blkw/2)					env->ThrowError("%s 0 <= OLapX <= blkw/2(%d)",myName,blkw/2);
	if(olapy<0 || olapy>blkh/2)					env->ThrowError("%s 0 <= OLapY <= blkh/2(%d)",myName,blkh/2);

	// If altscan then restore even hh after above checks, below routines will repeat
	if(altscan && ((hh & 0x01))) ++hh;

	const double BlkTh = args[18].AsFloat(1.0);
	const char *Prefix=args[19].AsString("");

	n  = (n<0)	? 0 : (n>= vi.num_frames) ?vi.num_frames -1:n;		// Range limit frame n
    n2 = (n2<0) ? 0 : (n2>=vi2.num_frames)?vi2.num_frames-1:n2;		// Range limit frame n2


    PVideoFrame src;
    PVideoFrame src2;

	if(n <= n2) {
		src         = child->GetFrame(n,env);
		src2        = child2->GetFrame(n2,env);
	} else {
		src2        = child2->GetFrame(n2,env);
		src         = child->GetFrame(n,env);
	}

	double result = 0.0;
	

	if(vi.IsYUY2()) {
		result = PVF_FrameMovement_YUY2(src,src2,CW,xx,yy,ww,hh,xx2,yy2,altscan,blkw,blkh,olapx,olapy,
			BlkTh,coordsP,PercAboveTh,BlkAveDf,nAboveTh,TotalBlks);
	} else if(vi.IsPlanar()){
		// Switch OFF ChromaWeight if Y8 : Switch OFF ChromaI if not YV12 : Check YV12 height if ChromaI
		const int rowsizeUV=src->GetRowSize(PLANAR_U);
		if(rowsizeUV!=0) {								// Not Y8
			int xSubS=src->GetRowSize(PLANAR_Y) / rowsizeUV;
			int ySubS=src->GetHeight(PLANAR_Y)  / src->GetHeight(PLANAR_U);
			if(ySubS!=2 || xSubS!=2)	
				ChromaI=false;
		} else {
			CW = 0.0;
			ChromaI=false;
		}
		if(ChromaI && vi.height&0x03)	env->ThrowError("%s ChromaI=True and YV12 height not multiple of 4",myName);
		result = PVF_FrameMovement_Planar(src,src2,CW,xx,yy,ww,hh,xx2,yy2,altscan,ChromaI,blkw,blkh,olapx,olapy,BlkTh,coordsP,PercAboveTh,BlkAveDf,nAboveTh,TotalBlks);
	} else {
		result = PVF_FrameMovement_RGB(src,src2,(CW>0.0),xx,yy,ww,hh,xx2,yy2,altscan,matrix,vi.IsRGB32(),blkw,blkh,olapx,olapy,BlkTh,coordsP,PercAboveTh,BlkAveDf,nAboveTh,TotalBlks);
	}
	if(Prefix!="") {
		int pfixlen=int(strlen(Prefix));
		if(pfixlen>128)
			pfixlen=128;
		char bf[128+32];
		memcpy(bf,Prefix,pfixlen);
		static const char *nam[]={"XOff","YOff"};
		char *d;
		AVSValue var;
		for(int i=0;i<2;++i) {
			d=bf+pfixlen;
			for(const char *p=nam[i];*d++=*p++;);			// strcat variable name part
			var  =	GetVar(env,bf);
			env->SetVar(var.Defined()?bf:env->SaveString(bf),coordsP[i]);
		}
		d=bf+pfixlen;
		for(const char *p="PercAboveTh";*d++=*p++;);			// strcat variable name part
		var  =	GetVar(env,bf);
		env->SetVar(var.Defined()?bf:env->SaveString(bf),PercAboveTh[0]);
		d=bf+pfixlen;
		for(const char *p="BlkAveDf";*d++=*p++;);			// strcat variable name part
		var  =	GetVar(env,bf);
		env->SetVar(var.Defined()?bf:env->SaveString(bf),BlkAveDf[0]);		
		d=bf+pfixlen;
		for(const char *p="nAboveTh";*d++=*p++;);			// strcat variable name part
		var  =	GetVar(env,bf);
		env->SetVar(var.Defined()?bf:env->SaveString(bf),nAboveTh[0]);		
		d=bf+pfixlen;
		for(const char *p="TotalBlks";*d++=*p++;);			// strcat variable name part
		var  =	GetVar(env,bf);
		env->SetVar(var.Defined()?bf:env->SaveString(bf),TotalBlks[0]);		
	}
    return result;
}

AVSValue __cdecl RT_FrameMovement(AVSValue args, void* user_data, IScriptEnvironment* env) {
	int coords[2]={0,0};
	double PercAboveTh[1]={0.0};
	double BlkAveDf[1]={0.0};
	int nAboveTh[1]={0};
	int TotalBlks[1]={0};
	double result = RT_FrameMovement_Lo(args,NULL,coords,PercAboveTh,BlkAveDf,nAboveTh,TotalBlks,env);
	return result;
}
