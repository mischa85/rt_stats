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



AVSValue __cdecl RT_QueryLumaMinMax(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_QueryLumaMinMax: ";
	double start_time =  RT_TimerHP_Lo();
	if(!args[0].IsClip())
		env->ThrowError("%sMust have a clip",myName);
	PClip child			=	args[0].AsClip();
    const VideoInfo &vi =	child->GetVideoInfo();
	if(vi.width==0 || vi.num_frames==0)			env->ThrowError("%sClip has no video",myName);

	int samples			=	args[1].AsInt(QLMM_SAMPLES);
	double ignore		=	args[2].AsFloat(QLMM_IGNORE);
	const char* prefix	=	args[3].AsString("QLMM");
	const bool debug	=	args[4].AsBool(false);
	const int xx		=	args[5].AsInt(0);
	const int yy		=	args[6].AsInt(0);
	int ww				=	args[7].AsInt(0);
	int hh				=	args[8].AsInt(0);
	if(ww <= 0) ww += vi.width  - xx;
	if(hh <= 0) hh += vi.height - yy;
	if(xx <  0 || xx >=vi.width)				env->ThrowError("%sInvalid X coord",myName);
	if(yy <  0 || yy >=vi.height)				env->ThrowError("%sInvalid Y coord",myName);
	if(ww <= 0 || xx + ww > vi.width)			env->ThrowError("%sInvalid W coord",myName);
	if(hh <= 0 || yy + hh > vi.height)			env->ThrowError("%sInvalid H coord",myName);
	const int matrix	=	args[9].AsInt(vi.width>1100 || vi.height>600?3:2);
    if(vi.IsRGB() && matrix & 0xFFFFFFFC)		env->ThrowError("%sRGB matrix 0 to 3 Only",myName);

	const int frames = vi.num_frames;

	if(debug){
		dprintf("%s",myName);
		char beta[16];
		beta[0]='\0';
		if(VERSION_BETA > 0) {
			sprintf(beta,"Beta%d",VERSION_BETA);
		}
		dprintf("%s%s v%.2f%s - By StainlessS",myName,"RT_QueryLumaMinMax",VERSION_NUMBER,beta);
		dprintf("%s",myName);
		dprintf("%sInput: Width=%d Height=%d FrameCount=%d",myName,vi.width,vi.height,frames);
	}

	if(samples==0) {
		samples=frames;
		if(debug)	dprintf("%sSamples=0 Converted to FrameCount(%d)",myName,samples);
	} else if(samples>frames) {
		if(debug)	dprintf("%sSamples (%d) limited to FrameCount(%d)",myName,samples,frames);
		samples=frames;
	}

	int SampStart=0,SampEnd=frames-1;
	
	const int skipfs	= int(frames * 0.05 + 0.5); 					// Skip 5%  BLACK intro framenum
	const int skipfe	= int(frames * 0.90 + 0.5) - 1;					// Skip 10% BLACK End Credits framenum

	char *smp=NULL;
	if(args[10].Defined()) {											// Start arg
		if((SampStart = args[10].AsInt()) < 0)	SampStart=0;
		if(args[11].Defined()) {										// End
			if(((SampEnd = args[11].AsInt()) <= 0) || SampEnd >= frames)		SampEnd=frames-1;
			smp="%sUser set Start(%d) and End(%d) : Range=%d";
		} else {
			// Try not to use artificial black END credits in auto thresh and crop sampling
			if(skipfe-SampStart+1 >= 250 && skipfe-SampStart+1 >= samples) {
				SampEnd	= skipfe;	// Skip 10% BLACK End Credits
				smp="%sUser set Start(%d), Skip End(%d) : Range=%d";
			} else	smp="%sUser Set Start(%d), No Skip End(%d) : Range=%d";
		}
	} else if(args[11].Defined()) {											// End arg
		if(((SampEnd = args[11].AsInt()) <= 0) || SampEnd >= frames)		SampEnd=frames-1;
		// Try not to use artificial black Intro credits in auto thresh and crop sampling
		if(SampEnd-skipfs+1 >= 250 && SampEnd-skipfs+1 >= samples) {
			SampStart	= skipfs;	// Skip 5% BLACK Intro Credits
			smp="%sSkip Start(%d), User set End(%d) : Range=%d";
		} else	smp="%sNo Skip Start(%d), User set End(%d) : Range=%d";
	} else {																// Start,End NOT supplied
		if(skipfe-skipfs+1 >= 250 && skipfe-skipfs+1 >= samples) {// Try not to use artificial black credits in auto thresh & crop sampling
			SampStart	= skipfs; 	// Skip 5%  BLACK intro
			SampEnd		= skipfe;	// Skip 10% BLACK End Credits
			smp="%sSkip Start(%d) and End(%d) : Range=%d";
		} else if(skipfe-skipfs+1 < 250)	smp="%sNo Skip Start(%d), No Skip End(%d) : Range=%d [Skip range < 250]";
		else	smp="%sNo Skip Start(%d), No Skip End(%d) : Range=%d [Not enough for Samples]";
	}

	const int SampRange = SampEnd - SampStart + 1;		// frame range of sampled frames

	if(debug) {
		dprintf("%sPotential Auto Credits Skip Start@5%%=%d End @90%%=%d : Range=%d",myName,skipfs,skipfe,skipfe-skipfs+1);
		if(smp!=NULL)	dprintf(smp,myName,SampStart,SampEnd,SampRange);
	}

	if(samples > SampRange) {
		samples = SampRange;
		if(debug)	dprintf("%sUser set Start or End, Reducing Samples to %d",myName,samples);
	}

	if(SampStart<0 || SampStart >=frames || SampEnd<SampStart || SampEnd>=frames) {
		if(debug) dprintf("%sIllegal frame Start(%d) or End(%d)",myName,SampStart,SampEnd);
		env->ThrowError("%sIllegal frame Start(%d) or End(%d)",myName,SampStart,SampEnd);
	}

	if(debug) {
		dprintf("%s",myName);
		dprintf("%sSamples=%d Ignore=%.2f Prefix=%s",myName,samples,ignore,prefix);
		dprintf("%sX=%d Y=%d W=%d H=%d Matrix=%d",myName,xx,yy,ww,hh,matrix);
		dprintf("%sSampStart=%d SampEnd=%d SampRange=%d",myName,SampStart,SampEnd,SampRange);
		dprintf("%s",myName);
	}

	AVSValue std[STD_SIZE]	=	{child,0,0,xx,yy,ww,hh,false};

	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_THRESH]	=	ignore;
	xtra[XTRA_RGBIX]	=	matrix;

	MYLO mylo;

	int samp,mn=255,mx=0;
	double SampMul = (samples <= 1) ? 0.0 : ((SampRange-1) / (samples-1.0));
	for(samp = 1;samp <= samples; ++samp) {
		const int n = int((samp - 1) * SampMul + 0.5) + SampStart;
		std[STD_FRAME]=n;
		RT_MYstats_Lo((RTMIN_F|RTMAX_F),AVSValue(std,STD_SIZE),AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
		if(mylo.dat.idat[RTMIN] < mn)		mn=mylo.dat.idat[RTMIN];
		if(mylo.dat.idat[RTMAX] > mx)		mx=mylo.dat.idat[RTMAX];
		if(debug) {
			dprintf("%sLumaRange: %-2d) [%-5d] LumaMin = %3d(%3d)  LumaMax = %3d(%3d)",
					myName,samp,n,mylo.dat.idat[RTMIN],mn,mylo.dat.idat[RTMAX],mx);
		}
	}

	char bf[256]="";
	sprintf(bf,"%sMin=%d %sMax=%d",prefix,mn,prefix,mx);
	if(debug){
		double duration =  RT_TimerHP_Lo() - start_time;

		dprintf("%s",myName);
		dprintf("%sReturn: %s",myName,bf);
		dprintf("%sScan Total Time=%.3f secs\n",myName,duration);
		dprintf("%s",myName);
	}
	return env->SaveString(bf);
}

