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

AVSValue __cdecl RT_YankChain(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// Force read frame only.
	char *myName="RT_YankChain: ";
	if(!args[0].IsClip())
		env->ThrowError("%sMust have a clip",myName);
	PClip child  = args[0].AsClip();										// Clip
	int n;
    if(args[1].IsInt()) {n  = args[1].AsInt(); }							// Frame n
    else {
        AVSValue cn       =	GetVar(env,"current_frame");
        if (!cn.IsInt())	env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
        n                 =	cn.AsInt();										// current_frame
    }
    n += args[2].AsInt(0);								// Delta, default 0
    const VideoInfo &vi     = child->GetVideoInfo();
	if(n>=0 && n < vi.num_frames) {						// Only if exists
  		PVideoFrame	src = child->GetFrame(n,env);		// Force decoding. MUST assign to PVideoFrame
	}
    return 0;
}

AVSValue __cdecl RT_ColorSpaceXMod(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_ColorSpaceXMod: ";
	PClip clip = args[0].AsClip();
    const VideoInfo vi = clip->GetVideoInfo();
	if(vi.width==0 || vi.num_frames==0)			env->ThrowError("%sClip has no video",myName);
	# ifdef AVISYNTH_PLUGIN_25
		if(vi.IsPlanar() && vi.pixel_type != 0xA0000008) { 				// Planar but NOT YV12, ie Avisynth v2.6+ ColorSpace
			// Here Planar but NOT YV12, If v2.5 Plugin Does NOT support ANY v2.6+ ColorSpaces
			env->ThrowError("%sColorSpace unsupported in v2.5 plugin",myName);
		}
	# endif
	int xmod=1;
	if(vi.IsYUV()) {
		if(vi.IsPlanar()) {
			PVideoFrame	src	= clip->GetFrame(0, env);					// get frame 0
			int	rowsizeUV=src->GetRowSize(PLANAR_U);
			if(rowsizeUV!=0)											// Not Y8
				xmod=src->GetRowSize(PLANAR_Y)/rowsizeUV;
		} else
			xmod=2;														// YUY2
	}
	return xmod;
}

AVSValue __cdecl RT_ColorSpaceYMod(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_ColorSpaceYMod: ";
	PClip clip = args[0].AsClip();
    const VideoInfo vi = clip->GetVideoInfo();
	if(vi.width==0 || vi.num_frames==0)			env->ThrowError("%sClip has no video",myName);
	# ifdef AVISYNTH_PLUGIN_25
		if(vi.IsPlanar() && vi.pixel_type != 0xA0000008) { 				// Planar but NOT YV12, ie Avisynth v2.6+ ColorSpace
			// Here Planar but NOT YV12, If v2.5 Plugin Does NOT support ANY v2.6+ ColorSpaces
			env->ThrowError("%sColorSpace unsupported in v2.5 plugin",myName);
		}
	# endif
	int ymod=1;
	if(vi.IsYUV()) {
		if(vi.IsPlanar()) {
			PVideoFrame	src	= clip->GetFrame(0, env);					// get frame 0
			int	rowsizeUV=src->GetRowSize(PLANAR_U);
			if(rowsizeUV!=0)											// Not Y8
				ymod=src->GetHeight(PLANAR_Y)/src->GetHeight(PLANAR_U);
		}
	}
	bool laced=args[1].AsBool(true);
	return (laced)?ymod*2:ymod;
}
