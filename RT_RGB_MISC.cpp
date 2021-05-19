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




AVSValue __cdecl RT_RGB32AsCommon(AVSValue args, int mode, IScriptEnvironment* env) {
static const char *Names[2]={"RT_RGB32AsInt: ","RT_RGB32AsFloat: "};
const char *myName=Names[mode];
	if(!args[0].IsClip())
		env->ThrowError("%sMust have a clip",myName);
PClip child  = args[0].AsClip();											// Clip
int n;
    if(args[1].IsInt()) {n  = args[1].AsInt(); }							// Frame n
    else {
        AVSValue cn       =	GetVar(env,"current_frame");
        if (!cn.IsInt())	env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
        n                 =	cn.AsInt();										// current_frame
    }
    n += args[2].AsInt(0);													// Delta, default 0
    const VideoInfo &vi     = child->GetVideoInfo();
    n = (n<0) ? 0 : (n>=vi.num_frames)?vi.num_frames-1:n;					// Range limit frame n


	if(!vi.IsRGB32()) {
		env->ThrowError("%sRGB32 Only",myName);
	}

	const int xx=args[3].AsInt(0);
	const int yy=args[4].AsInt(0);

	if(xx <  0 || xx >=vi.width)				env->ThrowError("%sInvalid X coord",myName);
	if(yy <  0 || yy >=vi.height)				env->ThrowError("%sInvalid Y coord",myName);

    PVideoFrame src		= child->GetFrame(n,env);
    const int   pitch	= src->GetPitch();
    const BYTE  *srcp	= src->GetReadPtr() + ((vi.height-1 - yy) * pitch) + (xx * 4);
    int sum	= *((int*)srcp);
	AVSValue result;
	if(mode==0) {
		result = sum;
	} else {
		float *f=(float*)&sum;
	    result = *f;
	}
	return result;
}

AVSValue __cdecl RT_RGB32AsFloat(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return RT_RGB32AsCommon(args, 1,env);
}
	
AVSValue __cdecl RT_RGB32AsInt(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return RT_RGB32AsCommon(args, 0, env);
}

AVSValue __cdecl RT_FloatAsRGBA(AVSValue args, void* user_data, IScriptEnvironment* env) {
	float f=(float)(args[0].AsFloat());
	int *p= (int*)&f;
    return *p;
}



