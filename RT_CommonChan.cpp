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

int __cdecl RT_ChanAve_Lo(const AVSValue &clip,const AVSValue &frame,double ret[3],const char*Name,IScriptEnvironment* env) {
	const char *myName=(Name==NULL)?"RT_ChanAve_Lo: ":Name;
	if(!clip.IsClip())
		env->ThrowError("%sMust have a source clip",myName);
	PClip child  = clip.AsClip();											// Clip
    const VideoInfo &vi     = child->GetVideoInfo();
	if(vi.width==0 || vi.num_frames==0)			env->ThrowError("%sClip has no video",myName);
	# ifdef AVISYNTH_PLUGIN_25
		if(vi.IsPlanar() && vi.pixel_type != 0xA0000008) {
			// Here Planar but NOT YV12, Plugin Does NOT support v2.6+ ColorSpace
			env->ThrowError("%s unsupported for colorSpace in v2.5 plugin",myName);
		}
	# endif
	int n;
    if(frame.IsInt()) {n  = frame.AsInt(); }								// Frame n
    else {
        AVSValue cn       =	GetVar(env,"current_frame");
        if (!cn.IsInt())	env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
        n                 =	cn.AsInt();										// current_frame
    }
	n = (n < 0) ? 0 : (n >= vi.num_frames) ? vi.num_frames-1 : n;
	AVSValue std[STD_SIZE] = {clip,n,0,0,0,0,0,false};
	AVSValue und;
	AVSValue xtra[XTRA_SIZE];
	PVideoFrame	src;
	int channels = 3;
	if(vi.IsPlanar()) {
		MYLO ylo;
		RT_MYstats_Lo(RTAVE_F,AVSValue(std,STD_SIZE),AVSValue(xtra,XTRA_SIZE),ylo,myName,env);
		ret[0]=ylo.dat.ddat[RTAVE-RTAVE];
		int xSubS=1,ySubS=1;
		src	= child->GetFrame(n, env);
		int	rowsizeUV=src->GetRowSize(PLANAR_U);
		if(rowsizeUV!=0) {								// Not Y8
			xSubS=src->GetRowSize(PLANAR_Y)/rowsizeUV;
			ySubS=src->GetHeight(PLANAR_Y)/src->GetHeight(PLANAR_U);
			int tSubS = xSubS * ySubS;
			int Pixels = (vi.width * vi.height) / tSubS;
			const int pitchUV = src->GetPitch(PLANAR_U);
			for(int plane=1;plane<=2;++plane) {
				int pln = (plane==1) ? PLANAR_U : PLANAR_V;
				const BYTE  *srcp = src->GetReadPtr(pln);
				int x,y;
				unsigned int sum = 0;
				int64_t	 acc = 0;
				for(y=src->GetHeight(pln);--y>=0;) {
					for(x=rowsizeUV;--x>=0;) {
						sum += srcp[x];
					}
					if(sum & 0x80000000) {acc += sum;sum=0;}					// avoid possiblilty of overflow
					srcp += pitchUV;
				}
				acc += sum;
			    ret[plane]= ((double)acc / Pixels);
			}
		} else {
			ret[1] = 128.0;
			ret[2] = 128.0;
			channels = 1;
		}
	} else if(vi.IsYUY2()) {
		MYLO ylo;
		RT_MYstats_Lo(RTAVE_F,AVSValue(std,STD_SIZE),AVSValue(xtra,XTRA_SIZE),ylo,myName,env);
		ret[0]=ylo.dat.ddat[RTAVE-RTAVE];
		src	= child->GetFrame(n, env);
		int	rowsize=src->GetRowSize();
		int tSubS = 2;
		int Pixels = (vi.width * vi.height) / tSubS;
		const int   pitch = src->GetPitch();
		const BYTE  *srcp = src->GetReadPtr();
		int x,y;
		unsigned int sumU = 0,sumV=0;
		int64_t	 accU = 0, accV=0;
		for(y=src->GetHeight();--y>=0;) {
			for(x=rowsize;(x-=4)>=0;) {
				sumU += srcp[x+1];						
				sumV += srcp[x+3];						
			}
			if(sumU & 0x80000000) {accU += sumU;sumU=0;}					// avoid possiblilty of overflow
			if(sumV & 0x80000000) {accV += sumV;sumV=0;}					// avoid possiblilty of overflow
			srcp += pitch;
		}
		accU += sumU;
		accV += sumV;
		ret[1]= ((double)accU / Pixels);
		ret[2]= ((double)accV / Pixels);
	} else if (vi.IsRGB()) {
		xtra[1] = -1;			// chan = -1 (all R,G,B)	
		MRGBALO rgb;
		RT_MRGBChanstats_Lo(RTAVE_F,AVSValue(std,STD_SIZE),AVSValue(xtra,XTRA_SIZE),rgb,myName,env);
		ret[0]=rgb.chans[0].ddat[RTAVE-RTAVE];
		ret[1]=rgb.chans[1].ddat[RTAVE-RTAVE];
		ret[2]=rgb.chans[2].ddat[RTAVE-RTAVE];
	} else {
		env->ThrowError("%sPlanar or YUY2 or RGB32 or RGB24 ONLY",myName);
	}
	return channels;
}



AVSValue __cdecl RT_ChanAve(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_ChanAve: ";
	double ret[3];
	int channels = RT_ChanAve_Lo(args[0],args[1],ret,myName,env);
	const char *prefix=args[2].AsString("RCA_");
	int pfixlen=int(strlen(prefix));
	if(pfixlen>128)
		pfixlen=128;
	char bf[128+32];
	memcpy(bf,prefix,pfixlen);
	char *d=bf+pfixlen;
	const char *p,*ave="Ave_";
	for(p=ave;*d++=*p++;);			// strcat variable name part
	d[0]='\0';						// nul term (d[-1] is currently nul)
	for(int chan=0;chan<=2;++chan) {
		d[-1]=chan+'0';					// Append channel number at string nul term - 1
		AVSValue var = GetVar(env,bf);
		env->SetVar(var.Defined()?bf:env->SaveString(bf),ret[chan]);
	}
	return channels;
}


// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------







double __cdecl RT_FrameDifference_Lo(const AVSValue &args,const char*name,IScriptEnvironment* env) {
	const char * myName=(name==NULL)?"RT_FrameDifference: ":name;
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
    const int matrix = args[13].AsInt(vi.width>1100 || vi.height>600 || vi2.width>1100 || vi2.height>600?3:2) & 0x03;
	if(vi.IsRGB() && (matrix<0 || matrix>3))	env->ThrowError("%s 0 <= matrix <= 3",myName);

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

	n  = (n<0)	? 0 : (n>= vi.num_frames) ?vi.num_frames -1:n;		// Range limit frame n
    n2 = (n2<0) ? 0 : (n2>=vi2.num_frames)?vi2.num_frames-1:n2;		// Range limit frame n2

    PVideoFrame src;
    PVideoFrame src2;

	// fetch frames in lo->hi order
	if(n <= n2) {
		src         = child->GetFrame(n,env);
		src2        = child2->GetFrame(n2,env);
	} else {
		src2        = child2->GetFrame(n2,env);
		src         = child->GetFrame(n,env);
	}

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
	
	double result = 0.0;

	if(vi.IsYUY2()) {
		result = (CW>0.0)
			? PVF_PixelDifference_YUY2(src,src2,CW,xx,yy,ww,hh,xx2,yy2,altscan)
			: PVF_LumaDifference_YUY2(src,src2,xx,yy,ww,hh,xx2,yy2,altscan);
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
		result = (CW>0.0)
			? PVF_PixelDifference_Planar(src,src2,CW,xx,yy,ww,hh,xx2,yy2,altscan,ChromaI)
			: PVF_LumaDifference_Planar(src,src2,xx,yy,ww,hh,xx2,yy2,altscan);
	} else {
		result =(CW>0.0)
			? PVF_PixelDifference_RGB(src,src2,xx,yy,ww,hh,xx2,yy2,altscan,vi.IsRGB32())
			: PVF_LumaDifference_RGB(src,src2,xx,yy,ww,hh,xx2,yy2,altscan,matrix,vi.IsRGB32());
	}
    return result;
}


AVSValue __cdecl RT_FrameDifference(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return RT_FrameDifference_Lo(args,NULL,env);
}



