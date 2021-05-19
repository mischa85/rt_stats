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

//   DAR = FAR * SAR   :::   FAR = DAR / SAR   :::   SAR = DAR / FAR

AVSValue __cdecl RT_GetDAR(AVSValue args, void*, IScriptEnvironment* env) {
	PClip child  = args[0].AsClip();											// Clip
	double sar = args[1].AsFloat();
    const VideoInfo &vi = child->GetVideoInfo();
	return sar * vi.width / vi.height;
}


AVSValue __cdecl RT_GetSAR(AVSValue args, void*, IScriptEnvironment* env) {
	PClip child  = args[0].AsClip();											// Clip
	double dar = args[1].AsFloat();
    const VideoInfo &vi = child->GetVideoInfo();
	return dar * vi.height / vi.width;
}

AVSValue __cdecl RT_SignalDAR(AVSValue args, void*, IScriptEnvironment* env) {
	double dar = args[0].AsFloat();
	if(dar<=0.0)
		env->ThrowError("RT_SignalDAR: Error, Dar must be greater than zero");
	int darx = (int)(1000.0 * dar + 0.5);
	int dary = 1000;
	const char *p1="MeGUI_DarX";
	AVSValue var1 = GetVar(env,p1);
	env->SetGlobalVar(var1.Defined() ? p1 : env->SaveString(p1),darx);
	const char *p2="MeGUI_DarY";
	AVSValue var2 = GetVar(env,p2);
	env->SetGlobalVar(var2.Defined() ? p2 : env->SaveString(p2),dary);
	return 0;
}

AVSValue __cdecl RT_SignalDAR2(AVSValue args, void*, IScriptEnvironment* env) {
	int darx = args[0].AsInt();
	int dary = args[1].AsInt();
	if(darx<=0 || dary<=0)
		env->ThrowError("RT_SignalDAR2: Error, DarX and DarY must be greater than zero");
	const char *p1="MeGUI_DarX";
	AVSValue var1 = GetVar(env,p1);
	env->SetGlobalVar(var1.Defined() ? p1 : env->SaveString(p1),darx);
	const char *p2="MeGUI_DarY";
	AVSValue var2 = GetVar(env,p2);
	env->SetGlobalVar(var2.Defined() ? p2 : env->SaveString(p2),dary);
	return 0;
}

AVSValue __cdecl RT_GetCropDAR(AVSValue args, void*, IScriptEnvironment* env) {
	const char *myName="RT_GetCropDAR: ";
	PClip child  = args[0].AsClip();											// Clip
	double dar = args[1].AsFloat();
	double x = args[2].AsFloat(0.0);
	double y = args[3].AsFloat(0.0);
	double w = args[4].AsFloat(0.0);
	double h = args[5].AsFloat(0.0);
    const VideoInfo &vi = child->GetVideoInfo();
	w=(w<=0.0)?vi.width +w-x:w;
	h=(h<=0.0)?vi.height+h-y:h;
	if(x<0.0 || x>vi.width)						env->ThrowError("%sInvalid X",myName);
	if(y<0.0 || y>vi.height)					env->ThrowError("%sInvalid Y",myName);
	if(w<=0.0 || (x+w) > vi.width)				env->ThrowError("%sInvalid W",myName);
	if(h<=0.0 || (y+h) > vi.height)				env->ThrowError("%sInvalid H",myName);
	return dar * vi.height / vi.width * w / h;
}

