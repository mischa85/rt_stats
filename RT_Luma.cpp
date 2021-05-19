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

AVSValue __cdecl RT_YDifference(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_YDifference: ";
	if(!args[0].IsClip())
		env->ThrowError("%sMust have a clip",myName);
	PClip child                 = args[0].AsClip();						// Clip
	int n,n2;
    if(args[1].IsInt()) {n  = args[1].AsInt(); }					// Frame n
    else {
        AVSValue cn         = GetVar(env,"current_frame");
        if (!cn.IsInt())    env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
        n                   = cn.AsInt();							// current_frame
    }
    n2  =  args[2].AsInt(1);										// Delta: default 1 as YDifferenceFromNext
	n2	+= n;														// n2 relative n --> frame number
    const VideoInfo &vi     = child->GetVideoInfo();

    n	= (n<0)		? 0 : (n>=vi.num_frames) ?vi.num_frames-1:n;	// Range limit frame n
    n2	= (n2<0)	? 0 : (n2>=vi.num_frames)?vi.num_frames-1:n2;   // Range limit frame n2

	if(!vi.IsPlanar() && !vi.IsYUY2() && !vi.IsRGB24() && !vi.IsRGB32()) {
		env->ThrowError("%sPlanar, YUY2, RGB24 and RGB32 Only",myName);
	}

	if(vi.width==0 || vi.num_frames==0)		env->ThrowError("%sclip must have video",myName);

	int xx=args[3].AsInt(0);
	int yy=args[4].AsInt(0);
	int ww=args[5].AsInt(0);
	int hh=args[6].AsInt(0);
	int xx2=args[7].AsInt(xx);
	int yy2=args[8].AsInt(yy);
	bool altscan=args[9].AsBool(false);
    const int matrix = args[10].AsInt(vi.width>1100 || vi.height>600?3:2);     // Matrix: REC601 : 1=REC709 : 2 = PC601 : 3 = PC709

	if(ww <= 0) ww += vi.width  - xx;
	if(hh <= 0) hh += vi.height - yy;
	if(altscan && ((hh & 0x01) == 0)) --hh;		// If altscan then ensure ODD number of lines, last line other field does not count.
	altscan=(altscan && hh!=1);

	if(xx <  0 || xx >=vi.width)				env->ThrowError("%sInvalid X coord",myName);
	if(yy <  0 || yy >=vi.height)				env->ThrowError("%sInvalid Y coord",myName);
	if(ww <= 0 || xx + ww > vi.width)			env->ThrowError("%sInvalid W coord for X",myName);
	if(hh <= 0 || yy + hh > vi.height)			env->ThrowError("%sInvalid H coord for Y",myName);
	if(xx2 < 0 || xx2 >=vi.width)				env->ThrowError("%sInvalid X2 coord",myName);
	if(yy2 < 0 || yy2 >=vi.height)				env->ThrowError("%sInvalid Y2 coord",myName);
	if(xx2 + ww >vi.width)						env->ThrowError("%sInvalid W coord for X2",myName);
	if(yy2 + hh >vi.height)						env->ThrowError("%sInvalid H coord for Y2",myName);
	
    PVideoFrame src;
    PVideoFrame src2;

	// fetch frames in lo->hi order
	if(n <= n2) {src  = child->GetFrame(n,env);		src2 = child->GetFrame(n2,env);
	} else		{src2 = child->GetFrame(n2,env);	src  = child->GetFrame(n,env);}

	double result = 0.0;
	if(vi.IsYUY2()) {
		result = PVF_LumaDifference_YUY2(src,src2,xx,yy,ww,hh,xx2,yy2,altscan);
	} else if(vi.IsPlanar()) {
		result = PVF_LumaDifference_Planar(src,src2,xx,yy,ww,hh,xx2,yy2,altscan);
	} else if(vi.IsRGB()) {
		if(matrix<0 || matrix > 3)		env->ThrowError("%Matrix 0 -> 3",myName);
		result = PVF_LumaDifference_RGB(src,src2,xx,yy,ww,hh,xx2,yy2,altscan,matrix,vi.IsRGB32());
	}
	return result;
}

double __cdecl RT_LumaDifference_Lo(const AVSValue &args,const char *Name,IScriptEnvironment* env) {
	const char * myName = (Name==NULL) ? "RT_LumaDifference: " : Name;
	PClip child,child2;
	int n,n2;
	bool	gotcur=false;
    if(args[2].IsInt())			{n  = args[2].AsInt(); }			// Frame n
    else {
        AVSValue cn			=	GetVar(env,"current_frame");
        if (!cn.IsInt())		env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
        n                   =	cn.AsInt();							// current_frame
		gotcur=true;
    }
    if(args[8].IsInt())			{n2  = args[8].AsInt(); }			// Frame n2
	else {
		if(gotcur) {
			n2=n;
		} else {
			AVSValue cn			=	GetVar(env,"current_frame");
			if (!cn.IsInt())		env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
			n2                  =	cn.AsInt();							// current_frame
		}
	}
	n	+= args[3].AsInt(0);		// n  += delta
	n2	+= args[9].AsInt(0);		// n2 += delta2

	if(!args[0].IsClip() || !args[1].IsClip())
		env->ThrowError("%sMust have two clips",myName);

	child	= args[0].AsClip();
	child2	= args[1].AsClip();

    const VideoInfo &vi     = child->GetVideoInfo();
    const VideoInfo &vi2    = child2->GetVideoInfo();

	if(!vi.IsSameColorspace(vi2))
		env->ThrowError("%sClips Dissimilar ColorSpace",myName);

	if(!vi.IsPlanar() && !vi.IsYUY2() && !vi.IsRGB24() && !vi.IsRGB32()) {
		env->ThrowError("%sPlanar, YUY2, RGB24 and RGB32 Only",myName);
	}

	if(vi.width==0 || vi.num_frames==0 || vi2.width==0 || vi2.num_frames==0)
		env->ThrowError("%sBoth clips must have video",myName);

	int		xx=args[4].AsInt(0);
	int		yy=args[5].AsInt(0);
	int		ww=args[6].AsInt(0);
	int		hh=args[7].AsInt(0);
	int		xx2=args[10].AsInt(xx);
	int		yy2=args[11].AsInt(yy);
	bool	altscan=args[12].AsBool(false);
	const int	matrix = args[13].AsInt(vi.width>1100 || vi.height>600 || vi2.width>1100 || vi2.height>600?3:2);
		// REC601 : 1=REC709 : 2 = PC601 : 3 = PC709

	if(ww <= 0) ww += vi.width  - xx;
	if(hh <= 0) hh += vi.height - yy;
	if(altscan && ((hh & 0x01) == 0)) --hh;		// If altscan then ensure ODD number of lines, last line other field does not count.
	altscan=(altscan && hh!=1);

	if(xx <  0 || xx >=vi.width)				env->ThrowError("%sInvalid X coord",myName);
	if(yy <  0 || yy >=vi.height)				env->ThrowError("%sInvalid Y coord",myName);
	if(ww <= 0 || xx + ww > vi.width)			env->ThrowError("%sInvalid W coord for X",myName);
	if(hh <= 0 || yy + hh > vi.height)			env->ThrowError("%sInvalid H coord for Y",myName);
	if(xx2 < 0 || xx2 >=vi2.width)				env->ThrowError("%sInvalid X2 coord",myName);
	if(yy2 < 0 || yy2 >=vi2.height)				env->ThrowError("%sInvalid Y2 coord",myName);
	if(xx2 + ww >vi2.width)						env->ThrowError("%sInvalid W coord for X2",myName);
	if(yy2 + hh >vi2.height)					env->ThrowError("%sInvalid H coord for Y2",myName);

	n  = (n<0)	? 0 : (n>= vi.num_frames) ?vi.num_frames -1:n;		// Range limit frame n
    n2 = (n2<0) ? 0 : (n2>=vi2.num_frames)?vi2.num_frames-1:n2;		// Range limit frame n2

    PVideoFrame src;
    PVideoFrame src2;

	// fetch frames in lo->hi order
	if(n <= n2) {src = child->GetFrame(n,env);	src2 = child2->GetFrame(n2,env);
	} else		{src2= child2->GetFrame(n2,env);src = child->GetFrame(n,env);}

	double result = 0.0;
	if(vi.IsYUY2()) {
		result = PVF_LumaDifference_YUY2(src,src2,xx,yy,ww,hh,xx2,yy2,altscan);
	} else if(vi.IsPlanar()) {
		result = PVF_LumaDifference_Planar(src,src2,xx,yy,ww,hh,xx2,yy2,altscan);
	} else if(vi.IsRGB()) {
		if(matrix<0 || matrix > 3)		env->ThrowError("%Matrix 0 -> 3",myName);
		result = PVF_LumaDifference_RGB(src,src2,xx,yy,ww,hh,xx2,yy2,altscan,matrix,vi.IsRGB32());
	}
	return result;
}

AVSValue __cdecl RT_LumaDifference(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return RT_LumaDifference_Lo(args,NULL,env);
}

int __cdecl RT_LumaPixelsDifferentCount_Lo(const AVSValue &args,IScriptEnvironment* env,double *dp) {
	// NOTE, dp is optional, default NULL
	char * myName="RT_LumaPixelsDifferentCount: ";
	PClip child,child2;
	int n,n2;
	bool	gotcur=false;
    if(args[2].IsInt())			{n  = args[2].AsInt(); }			// Frame n
    else {
        AVSValue cn			=	GetVar(env,"current_frame");
        if (!cn.IsInt())		env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
        n                   =	cn.AsInt();							// current_frame
		gotcur=true;
    }
    if(args[8].IsInt())			{n2  = args[8].AsInt(); }			// Frame n2
	else {
		if(gotcur) {
			n2=n;
		} else {
			AVSValue cn			=	GetVar(env,"current_frame");
			if (!cn.IsInt())		env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
			n2                  =	cn.AsInt();							// current_frame
		}
	}
	n	+= args[3].AsInt(0);		// n  += delta
	n2	+= args[9].AsInt(0);		// n2 += delta2

	if(!args[0].IsClip() || !args[1].IsClip())
		env->ThrowError("%sMust have two clips",myName);

	child                 = args[0].AsClip();
	child2                = args[1].AsClip();

    const VideoInfo &vi     = child->GetVideoInfo();
    const VideoInfo &vi2    = child2->GetVideoInfo();

	if(!vi.IsSameColorspace(vi2))
		env->ThrowError("%sClips Dissimilar ColorSpace",myName);

	if(!vi.IsPlanar() && !vi.IsYUY2() && !vi.IsRGB24() && !vi.IsRGB32()) {
		env->ThrowError("%sPlanar, YUY2, RGB24 and RGB32 Only",myName);
	}

	if(vi.width==0 || vi.num_frames==0 || vi2.width==0 || vi2.num_frames==0)
		env->ThrowError("%sBoth clips must have video",myName);

	int		xx=args[4].AsInt(0);
	int		yy=args[5].AsInt(0);
	int		ww=args[6].AsInt(0);
	int		hh=args[7].AsInt(0);
	int		xx2=args[10].AsInt(xx);
	int		yy2=args[11].AsInt(yy);
	bool	altscan=args[12].AsBool(false);
	const int	matrix = args[13].AsInt(vi.width>1100 || vi.height>600 || vi2.width>1100 || vi2.height>600?3:2);
		// REC601 : 1=REC709 : 2 = PC601 : 3 = PC709
	const int thresh = args[14].AsInt(0);
	if(thresh <0 || thresh > 255) {
		env->ThrowError("%sThresh Range 0 -> 255",myName);
	}

	if(ww <= 0) ww += vi.width  - xx;
	if(hh <= 0) hh += vi.height - yy;
	if(altscan && ((hh & 0x01) == 0)) --hh;		// If altscan then ensure ODD number of lines, last line other field does not count.
	altscan=(altscan && hh!=1);

	if(xx <  0 || xx >=vi.width)				env->ThrowError("%sInvalid X coord(%d)",myName,xx);
	if(yy <  0 || yy >=vi.height)				env->ThrowError("%sInvalid Y coord(%d)",myName,yy);
	if(ww <= 0 || xx + ww > vi.width)			env->ThrowError("%sInvalid W(%d) coord for X",myName,ww);
	if(hh <= 0 || yy + hh > vi.height)			env->ThrowError("%sInvalid H(%d) coord for Y",myName,hh);
	if(xx2 < 0 || xx2 >=vi2.width)				env->ThrowError("%sInvalid X2(%d) coord",myName,xx2);
	if(yy2 < 0 || yy2 >=vi2.height)				env->ThrowError("%sInvalid Y2(%d) coord",myName,yy2);
	if(xx2 + ww >vi2.width)						env->ThrowError("%sInvalid W(%d) coord for X2(%d)",myName,ww,xx2);
	if(yy2 + hh >vi2.height)					env->ThrowError("%sInvalid H(%d) coord for Y2(%d)",myName,hh,yy2);

	n  = (n<0)	? 0 : (n>= vi.num_frames) ?vi.num_frames -1:n;		// Range limit frame n
    n2 = (n2<0) ? 0 : (n2>=vi2.num_frames)?vi2.num_frames-1:n2;		// Range limit frame n2

    const unsigned int Pixels = (ww * ((altscan)?(hh+1)>>1:hh));

    PVideoFrame src;
    PVideoFrame src2;

	// fetch frames in lo->hi order
	if(n <= n2) {src = child->GetFrame(n,env);  src2 = child2->GetFrame(n2,env);
	} else		{src2= child2->GetFrame(n2,env);src = child->GetFrame(n,env);}
	
    int sum       = 0;
	double result = 0.0;

	if(vi.IsYUY2()) {
		sum = PVF_LumaPixelsDifferentCount_YUY2(src,src2,xx,yy,ww,hh,xx2,yy2,altscan,thresh,&result);
	} else if(vi.IsPlanar()) {
		sum = PVF_LumaPixelsDifferentCount_Planar(src,src2,xx,yy,ww,hh,xx2,yy2,altscan,thresh,&result);
	} else if(vi.IsRGB()) {
		if(matrix<0 || matrix > 3)		env->ThrowError("%Matrix 0 -> 3",myName);
		sum = PVF_LumaPixelsDifferentCount_RGB(src,src2,xx,yy,ww,hh,xx2,yy2,altscan,thresh,&result,matrix,vi.IsRGB32());
	}

	if(dp != NULL) {
		*dp = (sum * 255.0) / Pixels;
	}
    return sum;
}

AVSValue __cdecl RT_LumaPixelsDifferentCount(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return RT_LumaPixelsDifferentCount_Lo(args,env);
}

double __cdecl RT_LumaPixelsDifferent_Lo(const AVSValue &args,IScriptEnvironment* env) {
	double ret=0.0;
	RT_LumaPixelsDifferentCount_Lo(args,env,&ret);
	return ret;
}

AVSValue __cdecl RT_LumaPixelsDifferent(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return RT_LumaPixelsDifferent_Lo(args,env);
}

double __cdecl RT_LumaCorrelation_Lo(const AVSValue &args, IScriptEnvironment* env) {
	char * myName="RT_LumaCorrelation: ";
	PClip child,child2;
	int n,n2;
	bool	gotcur=false;
    if(args[2].IsInt())			{n  = args[2].AsInt(); }			// Frame n
    else {
        AVSValue cn			=	GetVar(env,"current_frame");
        if (!cn.IsInt())		env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
        n                   =	cn.AsInt();							// current_frame
		gotcur=true;
    }
    if(args[8].IsInt())			{n2  = args[8].AsInt(); }			// Frame n2
	else {
		if(gotcur) {
			n2=n;
		} else {
			AVSValue cn			=	GetVar(env,"current_frame");
			if (!cn.IsInt())		env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
			n2                  =	cn.AsInt();							// current_frame
		}
	}
	n	+= args[3].AsInt(0);		// n  += delta
	n2	+= args[9].AsInt(0);		// n2 += delta2

	if(!args[0].IsClip() || !args[1].IsClip())
		env->ThrowError("%sMust have two clips",myName);

	child                 = args[0].AsClip();
	child2                = args[1].AsClip();

    const VideoInfo &vi     = child->GetVideoInfo();
    const VideoInfo &vi2    = child2->GetVideoInfo();

	if(!vi.IsSameColorspace(vi2))
		env->ThrowError("%sClips Dissimilar ColorSpace",myName);

	if(!vi.IsPlanar() && !vi.IsYUY2() && !vi.IsRGB24() && !vi.IsRGB32()) {
		env->ThrowError("%sPlanar, YUY2, RGB24 and RGB32 Only",myName);
	}

	if(vi.width==0 || vi.num_frames==0 || vi2.width==0 || vi2.num_frames==0)
		env->ThrowError("%sBoth clips must have video",myName);

	int		xx=args[4].AsInt(0);
	int		yy=args[5].AsInt(0);
	int		ww=args[6].AsInt(0);
	int		hh=args[7].AsInt(0);
	int		xx2=args[10].AsInt(xx);
	int		yy2=args[11].AsInt(yy);
	bool	altscan=args[12].AsBool(false);
	const int	matrix = args[13].AsInt(vi.width>1100 || vi.height>600 || vi2.width>1100 || vi2.height>600?3:2);
		// REC601 : 1=REC709 : 2 = PC601 : 3 = PC709

	if(ww <= 0) ww += vi.width  - xx;
	if(hh <= 0) hh += vi.height - yy;
	if(altscan && ((hh & 0x01) == 0)) --hh;		// If altscan then ensure ODD number of lines, last line other field does not count.
	altscan=(altscan && hh!=1);

	if(xx <  0 || xx >=vi.width)				env->ThrowError("%sInvalid X coord",myName);
	if(yy <  0 || yy >=vi.height)				env->ThrowError("%sInvalid Y coord",myName);
	if(ww <= 0 || xx + ww > vi.width)			env->ThrowError("%sInvalid W coord for X",myName);
	if(hh <= 0 || yy + hh > vi.height)			env->ThrowError("%sInvalid H coord for Y",myName);
	if(xx2 < 0 || xx2 >=vi2.width)				env->ThrowError("%sInvalid X2 coord",myName);
	if(yy2 < 0 || yy2 >=vi2.height)				env->ThrowError("%sInvalid Y2 coord",myName);
	if(xx2 + ww > vi2.width)					env->ThrowError("%sInvalid W coord for X2",myName);
	if(yy2 + hh > vi2.height)					env->ThrowError("%sInvalid H coord for Y2",myName);

	n  = (n<0)	? 0 : (n>= vi.num_frames) ?vi.num_frames -1:n;		// Range limit frame n
    n2 = (n2<0) ? 0 : (n2>=vi2.num_frames)?vi2.num_frames-1:n2;		// Range limit frame n2

    PVideoFrame src;
    PVideoFrame src2;
	
	// fetch frames in lo->hi order
	if(n <= n2) {
		src   = child->GetFrame(n,env);
		src2  = child2->GetFrame(n2,env);
	} else {
		src2  = child2->GetFrame(n2,env);
		src   = child->GetFrame(n,env);
	}

	double result=0.0;
	if(vi.IsYUY2()) {
		result = PVF_LumaCorrelation_YUY2(src,src2,xx,yy,ww,hh,xx2,yy2,altscan);
	} else if(vi.IsPlanar()) {
		result = PVF_LumaCorrelation_Planar(src,src2,xx,yy,ww,hh,xx2,yy2,altscan);
	} else if(vi.IsRGB()) {
		if(matrix<0 || matrix > 3)		env->ThrowError("%Matrix 0 -> 3",myName);
		result = PVF_LumaCorrelation_RGB(src,src2,xx,yy,ww,hh,xx2,yy2,altscan,matrix,vi.IsRGB32());
	}
	return result;
// http://en.wikipedia.org/wiki/Pearson_product-moment_correlation_coefficient#Mathematical_properties
}

AVSValue __cdecl RT_LumaCorrelation(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return RT_LumaCorrelation_Lo(args,env);
}
