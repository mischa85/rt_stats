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
#include <algorithm>

AVSValue __cdecl RT_QueryBorderCrop(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_QueryBorderCrop: ";
	double start_time =  RT_TimerHP_Lo();
	if(!args[0].IsClip())
		env->ThrowError("%sMust have a clip",myName);
	PClip child			=	args[0].AsClip();
    const VideoInfo &vi =	child->GetVideoInfo();	
	int samples			=	args[1].AsInt(QBC_SAMPLES);
	bool laced			=	args[3].AsBool(true);

	if(vi.width==0 || vi.num_frames==0)			env->ThrowError("%sClip has no video",myName);
	# ifdef AVISYNTH_PLUGIN_25
		if(vi.IsPlanar() && vi.pixel_type != 0xA0000008) { 				// Planar but NOT YV12, ie Avisynth v2.6+ ColorSpace
			// Here Planar but NOT YV12, If v2.5 Plugin Does NOT support ANY v2.6+ ColorSpaces
			env->ThrowError("%sColorSpace unsupported in v2.5 plugin",myName);
		}
	# endif

	int xSubS=1,ySubS=1;

	if(vi.IsYUV()) {
		if(vi.IsPlanar()) {
			PVideoFrame	src	= child->GetFrame(0, env);					// get frame 0
			int	rowsizeUV=src->GetRowSize(PLANAR_U);
			if(rowsizeUV!=0) {								// Not Y8
				xSubS=src->GetRowSize(PLANAR_Y)/rowsizeUV;
				ySubS=src->GetHeight(PLANAR_Y)/src->GetHeight(PLANAR_U);
			}
		} else {
			xSubS=2;
		}
	}

	const int xmod		=	args[4].AsInt(xSubS);
	const int ymod		=	args[5].AsInt((laced)?ySubS*2:ySubS);
	const int wmod		=	args[6].AsInt(std::max(xmod,4));
	const int hmod		=	args[7].AsInt(ymod);
	const bool relative	=	args[8].AsBool(false);
	const char *prefix	=	args[9].AsString("QBCrop");
	const int rlbt		=	args[10].AsInt(0x0F);
	const bool debug	=	args[11].AsBool(false);
	double ignore		=	args[12].AsFloat(0.4f);
	const int matrix	=	args[13].AsInt(vi.width>1100 || vi.height>600?3:2);
	double scanperc		=	args[14].AsFloat(49.0);
	const int baffle	=	args[15].AsInt(4);
	const bool scalergb	=	args[16].AsBool(true);
	const bool scaleyuv	=	args[17].AsBool(false);
	double atm			=	args[18].AsFloat(4.0);

	// Start and End = args[19 to 20]

	const int LeftAdd	=	args[21].AsInt(0);
	const int TopAdd	=	args[22].AsInt(0);
	const int RightAdd	=	args[23].AsInt(0);
	const int BotAdd	=	args[24].AsInt(0);
	const int LeftSkip	=	args[25].AsInt(0);
	const int TopSkip	=	args[26].AsInt(0);
	const int RightSkip	=	args[27].AsInt(0);
	const int BotSkip	=	args[28].AsInt(0);


	const int w=vi.width;
	const int h=vi.height;
	const int frames=vi.num_frames;

	if(debug){
		dprintf("%s",myName);
		char beta[16];
		beta[0]='\0';
		if(VERSION_BETA > 0) {
			sprintf(beta,"Beta%d",VERSION_BETA);
		}
		dprintf("%s%s v%.2f%s - By StainlessS",myName,"RT_QueryBorderCrop",VERSION_NUMBER,beta);
		dprintf("%s",myName);
		dprintf("%sInput: Width=%d Height=%d FrameCount=%d",myName,w,h,frames);
	}



	if(samples==0) {
		samples=frames;
		if(debug)	dprintf("%sSamples=0 Converted to FrameCount(%d)",myName,samples);
	} else if(samples>frames) {
		if(debug)	dprintf("%sSamples (%d) limited to FrameCount(%d)",myName,samples,frames);
		samples=frames;
	}

	bool IsDefaultAutoThresh=false;

	double thresh;
	if(args[2].Defined()) {
		thresh	=	args[2].AsFloat();
		IsDefaultAutoThresh=(thresh== -QBC_THRESH || thresh== -32.0f);
		if(debug)
			dprintf("%sThresh set to user supplied %.2f",myName,thresh);
	} else {
		thresh		=	-QBC_THRESH;
		IsDefaultAutoThresh=true;
		if(debug)
			dprintf("%sThresh defaulting to DEFAULT AUTOTHRESH %.2f",myName,thresh);
	}

	if(debug && IsDefaultAutoThresh) {
		dprintf("%sThresh %.2f recognised as DEFAULT AUTOTHRESH",myName,thresh);
	}

	scanperc=(scanperc<1.0) ? 1.0 : (scanperc>99.0) ? 99.0 : scanperc;	// Silent limit
	int rgt=int(w*(100.0-scanperc)/100.0);
	int bot=int(h*(100.0-scanperc)/100.0);
	int lft=int(w*scanperc/100.0);
	int top=int(h*scanperc/100.0);

    if(vi.IsRGB() && (matrix & 0xFFFFFFFC))		env->ThrowError("%sRGB matrix 0 to 3 Only",myName);
	if(samples<1)								env->ThrowError("%sRequires Samples>=1",myName);

	if(xmod<xSubS || wmod<xSubS)				env->ThrowError("%sXMod and WMod, Must be at least %d",myName,xSubS);
	if(ymod<ySubS || hmod<ySubS)				env->ThrowError("%sYMod and HMod, Must be at least %d",myName,ySubS);
	if(xmod%xSubS)								env->ThrowError("%sXMod must be a multiple of %d",myName,xSubS);
	if(ymod%ySubS)								env->ThrowError("%sYMod must be a multiple of %d",myName,ySubS);
	if(wmod%xmod)								env->ThrowError("%sWMod must be a multiple of XMod(%d)",myName,xmod);
	if(hmod%ymod)								env->ThrowError("%sHMod must be a multiple of YMod(%d)",myName,ymod);

	if(wmod>16)									env->ThrowError("%sWMod, %d -> 16 (%d)",myName,xmod,wmod);
	if(hmod>16)									env->ThrowError("%sHMod, %d -> 16 (%d)",myName,ymod,hmod);

	if(rlbt<1 || rlbt>0xF)						env->ThrowError("%sRLBT 1->15 Only",myName);
	if(baffle<=0)								env->ThrowError("%sBaffle must be greater than zero",myName);


	if(TopAdd   < 0)							env->ThrowError("%sTopAdd cannot be -ve",myName);
	if(BotAdd   < 0)							env->ThrowError("%sBotAdd cannot be -ve",myName);
	if(LeftAdd  < 0)							env->ThrowError("%sLeftAdd cannot be -ve",myName);
	if(RightAdd < 0)							env->ThrowError("%sRightAdd cannot be -ve",myName);

	if(TopSkip  < 0 || TopSkip   > h/4)			env->ThrowError("%sTopSkip range 0 -> h/4(%d)",myName,h/4);
	if(BotSkip  < 0 || BotSkip   > h/4)			env->ThrowError("%sBotSkip range 0 -> h/4(%d)",myName,h/4);
	if(LeftSkip < 0 || LeftSkip  > w/4)			env->ThrowError("%sLeftSkip range 0 -> w/4(%d)",myName,w/4);
	if(RightSkip< 0 || RightSkip > w/4)			env->ThrowError("%sRightSkip range 0 -> w/4(%d)",myName,w/4);

	int x1= LeftSkip, y1=TopSkip, x2=w-1-RightSkip, y2=h-1-BotSkip;

	if(x1+LeftAdd+baffle >= lft)				env->ThrowError("%sCheck LeftSkip : LeftAdd : Baffle",myName);
	if(y1+TopAdd+baffle >= top)					env->ThrowError("%sCheck TopSkip : TopAdd : Baffle",myName);
	if(x2-RightAdd-baffle<= rgt)				env->ThrowError("%sCheck RightSkip : RightAdd : Baffle",myName);
	if(y2-BotAdd-baffle  <= bot)				env->ThrowError("%sCheck BotSkip : BotAdd : Baffle",myName);

	if(atm < 0.0)		atm = -atm;				// in case supplied as -ve
	if(atm<0.5)			atm=0.5;				// silently limit
	else if(IsDefaultAutoThresh && atm>fabs(thresh))	atm=fabs(thresh);

	ignore = (ignore < 0.0) ? 0.0 : ignore > 100.0 ? 100.0 : ignore;

	int SampStart=0,SampEnd=frames-1;
	
	const int skipfs	= int(frames * 0.05 + 0.5); 					// Skip 5%  BLACK intro framenum
	const int skipfe	= int(frames * 0.90 + 0.5) - 1;					// Skip 10% BLACK End Credits framenum

	char *smp=NULL;
	if(args[19].Defined()) {											// Start arg
		if((SampStart = args[19].AsInt()) < 0)	SampStart=0;
		if(args[20].Defined()) {										// End
			if(((SampEnd = args[20].AsInt()) <= 0) || SampEnd >= frames)		SampEnd=frames-1;
			smp="%sUser set Start(%d) and End(%d) : Range=%d";
		} else {
			// Try not to use artificial black END credits in auto thresh and crop sampling
			if(skipfe-SampStart+1 >= 250 && skipfe-SampStart+1 >= samples) {
				SampEnd	= skipfe;	// Skip 10% BLACK End Credits
				smp="%sUser set Start(%d), Skip End(%d) : Range=%d";
			} else	smp="%sUser Set Start(%d), No Skip End(%d) : Range=%d";
		}
	} else if(args[20].Defined()) {											// End arg
		if(((SampEnd = args[20].AsInt()) <= 0) || SampEnd >= frames)		SampEnd=frames-1;
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


	if(IsDefaultAutoThresh && atm < fabs(thresh)) {
		double samples_massage 	=(samples>=20)				? 1.0 : (samples-1)		* (1.0/(20-1));
		double range_massage	=(SampRange >= (250*20))	? 1.0 : (SampRange-1)	* (1.0/((250*20)-1));
		double inthresh = thresh;
		thresh = -(((samples_massage * range_massage) * (fabs(thresh) - atm)) + atm);
		if(fabs(thresh - inthresh) > 0.0005) {
			if(debug) {
				if(samples_massage<1.0)
					dprintf("%sLow Samples count(%d), Massaging Auto Thresh(samples scaler=%.6f)",myName,samples,samples_massage);
				if(range_massage<1.0)
					dprintf("%sLow Frames Range(%d), Massaging Auto Thresh(range scaler=%.6f)",myName,SampRange,range_massage);
				if(samples_massage<1.0 && range_massage<1.0)
					dprintf("%sCombined Thresh Massage Scaler=%.6f",myName,samples_massage * range_massage);
				dprintf("%sAuto Thresh reduced from DEFAULT %.6f to %.6f",myName,inthresh,thresh);
			}
		}		
	}

	if(thresh<0.0) {											// no point in scaling Thresh==0.0
		if(vi.IsYUV() && scaleyuv) {
			thresh=thresh *(255.0/(235.0-16.0));				// Auto Thresh is scaled relative to YplaneMin @ PCLevels
			if(debug)
				dprintf("%sAutoThresh: YUV, TV Thresh Scaled to PC %.3f",myName,thresh);
		}else if(vi.IsRGB() && scalergb) {
			if(matrix<2)				env->ThrowError("%sRGB ScaleAutoThreshRGB, Matrix NOT @ PC levels - Check!",myName);
			thresh=thresh *(255.0/(235.0-16.0));				// Auto Thresh is scaled relative to YplaneMin @ PCLevels
			if(debug)
				dprintf("%sAutoThresh: RGB, TV Thresh Scaled to PC %.3f",myName,thresh);
		}
	}

	if(debug){
		dprintf("%s",myName);
		dprintf("%sSamples=%d Thresh=%.3f Laced=%s Matrix=%d",myName,samples,thresh,(laced)?"True":"False",matrix);
		dprintf("%sWMod=%d HMod=%d {Colorspace/Laced Restricted XMod=%d YMod=%d}",myName,wmod,hmod,xmod,ymod);
		dprintf("%sRelative=%s Prefix=%s",myName,(relative)?"True":"False",prefix);
		dprintf("%sRLBT=%d Ignore=%.3f Baffle=%d",myName,rlbt,ignore,baffle);
		dprintf("%sScaleAutoThreshRGB=%s ScaleAutoThreshYUV=%s ",myName,(scalergb)?"Yes":"No",(scaleyuv)?"Yes":"No");
		dprintf("%sAtm=%.3f SampStart=%d SampEnd=%d SampRange=%d",myName,atm,SampStart,SampEnd,SampRange);
		dprintf("%sLeftAdd=%d TopAdd=%d RightAdd=%d BotAdd=%d",myName,LeftAdd,TopAdd,RightAdd,BotAdd);
		dprintf("%sLeftSkip=%d TopSkip=%d RightSkip=%d BotSkip=%d",myName,LeftSkip,TopSkip,RightSkip,BotSkip);
		dprintf("ScanPerc=%f",scanperc);
		dprintf("%s",myName);
	}

	// Find MINLUMA (also for Thresh==0.0)
	int samp;

	AVSValue std[STD_SIZE]	=	{child,0,0,0,0,0,0,false};

	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_THRESH]	=	ignore;
	xtra[XTRA_RGBIX]	=	matrix;

	MYLO mylo;

	double SampMul = (samples <= 1) ? 0.0 : ((SampRange-1) / (samples-1.0));
	if(thresh <= 0.0) {
		int thmin = 255;
		std[STD_XX] = x1;
		std[STD_YY] = y1;
		std[STD_WW] = x2-x1+1;
		std[STD_HH] = y2-y1+1;
		for(samp = 1;samp<=samples;++samp) {
			const int frm = int((samp - 1)	* SampMul + 0.5) + SampStart;
			std[STD_FRAME]	=	frm;	// n
			RT_MYstats_Lo(RTMIN_F,AVSValue(std,STD_SIZE),AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
			const int th = mylo.dat.idat[RTMIN];
			// th=RT_yPlaneMin(frm,threshold=ignore,matrix=matrix)
			if (thmin>th)	thmin=th;
			if(debug)		dprintf("%sAutoThresh %3d) [%6d] YPlaneMin =%3d (%d)",myName,samp,frm,th,thmin);
			if(thmin==0)	break;		// No smaller possible
		}
		thresh = thmin + (-thresh);
		if(debug) {
			dprintf("%s",myName);
			dprintf("%sYPlaneMin=%d : Automatic set Thresh=%.3f",myName,thmin,thresh);
			dprintf("%s",myName);
		}
	}


	// Border detect
	int gotf=-1,gotn=0;										// Track Number of frames where coords 1st found
	for(samp = 1;samp<=samples;++samp) {
		const int frm = int((samp - 1)	* SampMul + 0.5) + SampStart;	// Sample frame
		double threm=0.0;
		std[STD_FRAME]	=	frm;	// n
		int SCur,MCur,ECur,i;
		if((rlbt&0x01) && top>=(TopSkip+baffle-1)) {						// Top Edge
			std[STD_XX]	=	x1;			// x
			std[STD_WW]	=	x2-x1+1;	// w
			std[STD_HH]	=	1;			// h
			SCur=TopSkip; MCur=SCur; ECur=SCur+(baffle-1);
			while(ECur<=top) {
				i=ECur;
				while(i>=MCur) {
					std[STD_YY]	=	i;		// y
					RT_MYstats_Lo(RTAVE_F,AVSValue(std,STD_SIZE),AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
					const double th = mylo.dat.ddat[RTAVE-RTAVE];
				//	th=RT_AverageLuma(frm,x=x1,y=i,w=x2-x1+1,h=1,matrix=matrix);
					if(th>thresh) {
						if(SCur==MCur)	
							threm=th; // Only store most recent threm if 1st part of potentially 2 sweep scan (2nd part, SCur!=MCur)
						--i;
					} else 
						break;
				}
				if(i<MCur)	{ // successfully scanned full single sweep OR 2nd of 2 sweep scan
					y1=SCur;
					top=(y1==TopSkip)?TopSkip:ECur-1;
					if(bot<SCur) {
						bot = SCur;
					}
					if(frm!=gotf) {
						gotf=frm;
						++gotn;			
					}
					if(debug)	dprintf("%sTop %3d) [%6d]  AveY=%6.2f  Y1=%3d",myName,samp,frm,threm,y1);
					break;
				} else {SCur=i+1; MCur=ECur+1; ECur=i+baffle;} // New scan if failed on ECur(i==ECur), else 2nd sweep scan.
			}
		}

		if((rlbt&0x02) && bot<=(h-BotSkip-baffle)) {						// Bottom Edge
			std[STD_XX]	=	x1;			// x
			std[STD_WW]	=	x2-x1+1;	// w
			std[STD_HH]	=	1;			// h
			SCur=(h-1)-BotSkip; MCur=SCur; ECur=SCur-(baffle-1);
			while(ECur>=bot) {
				i=ECur;
				while(i<=MCur) {
					std[STD_YY]	=	i;		// y
					RT_MYstats_Lo(RTAVE_F,AVSValue(std,STD_SIZE),AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
					const double th = mylo.dat.ddat[RTAVE-RTAVE];
				//	th=RT_AverageLuma(frm,x=x1,y=i,w=x2-x1+1,h=1,matrix=matrix);
					if(th>thresh) {
						if(SCur==MCur)		threm=th;
						++i;
					} else
						break;
				}
				if(i>MCur)	{
					y2 = SCur;
					bot=(y2==(h-1)-BotSkip)?(h-1)-BotSkip:ECur+1;
					if(top>SCur) {
						top = SCur;
					}
					if(frm!=gotf) {
						gotf=frm;
						++gotn;
					}
					if(debug)	dprintf("%sBot %3d) [%6d]  AveY=%6.2f  Y2=%3d",myName,samp,frm,threm,y2);
					break;
				}else 	{SCur=i-1; MCur=ECur-1; ECur=i-baffle;}
			}
		}

		if((rlbt&0x04) && lft>=(LeftSkip+baffle-1)) {					// Left Edge
			std[STD_YY]	=	y1;			// y
			std[STD_WW]	=	1;			// w
			std[STD_HH]	=	y2-y1+1;	// h
			SCur=LeftSkip; MCur=SCur; ECur=SCur+(baffle-1);
			while(ECur<=lft) {
				i=ECur;
				while(i>=MCur) {
					std[STD_XX]	=	i;		// x
					RT_MYstats_Lo(RTAVE_F,AVSValue(std,STD_SIZE),AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
					const double th = mylo.dat.ddat[RTAVE-RTAVE];				
				//	th=RT_AverageLuma(frm,x=i,y=y1,w=1,h=y2-y1+1,matrix=matrix);
					if(th>thresh) {
						if(SCur==MCur)
							threm=th;
						--i;
					} else
						break;
				}
				if(i<MCur)	{
					x1=SCur;
					lft=(x1==LeftSkip)?LeftSkip:ECur-1;
					if(rgt<SCur) {
						rgt = SCur;
					}
					if(frm!=gotf) {
						gotf=frm;
						++gotn;
					}
					if(debug)	dprintf("%sLft %3d) [%6d]  AveY=%6.2f  X1=%3d",myName,samp,frm,threm,x1);
					break;
				}else 	{SCur=i+1; MCur=ECur+1; ECur=i+baffle;}
			}
		}

		if((rlbt&0x08) && rgt<=(w-RightSkip-baffle)) {							// Right Edge
			std[STD_YY]	=	y1;			// y
			std[STD_WW]	=	1;			// w
			std[STD_HH]	=	y2-y1+1;	// h
			SCur=(w-1)-RightSkip; MCur=SCur; ECur=SCur-(baffle-1);
			while(ECur>=rgt) {
				i=ECur;
				while(i<=MCur) {
					std[STD_XX]	=	i;		// y
					RT_MYstats_Lo(RTAVE_F,AVSValue(std,STD_SIZE),AVSValue(xtra,XTRA_SIZE),mylo,myName,env);
					const double th = mylo.dat.ddat[RTAVE-RTAVE];								
				//	th=RT_AverageLuma(frm,x=i,y=y1,w=1,h=y2-y1+1,matrix=matrix);
					if(th>thresh) {
						if(SCur==MCur)
							threm=th;
						++i;
					} else
						break;
				}
				if(i>MCur)	{
					x2 = SCur;
					rgt=(x2==(w-1)-RightSkip)?(w-1)-RightSkip:ECur+1;
					if(lft>SCur) {
						lft = SCur;
					}
					if(frm!=gotf) {
						gotf=frm;
						++gotn;
					}
					if(debug) dprintf("%sRgt %3d) [%6d]  AveY=%6.2f  X2=%3d",myName,samp,frm,threm,x2);
					break;
				}else	{SCur=i-1; MCur=ECur-1; ECur=i-baffle;}
			}
		}

		if(
			(top==TopSkip		|| !(rlbt&0x01)) && 
			(bot==h-1-BotSkip	|| !(rlbt&0x02)) &&
			(lft==LeftSkip		|| !(rlbt&0x04)) && 
			(rgt==w-1-RightSkip	|| !(rlbt&0x08))
		)
			break;											// No further cropping possible, Break.
	}

	if(debug){
		dprintf("%s",myName);
		dprintf("%sActive Frames (where image coords first found) = %d",myName,gotn);
		dprintf("%sSampled ImageEdge:  X1=%d  Y1=%d  X2=%d(W=%d,%d)  Y2=%d(H=%d,%d)",
				myName,x1,y1,x2,x2-x1+1,x2-w+1,y2,y2-y1+1,y2-h+1);
		dprintf("%s",myName);
	}

	if(LeftAdd!=0 || TopAdd !=0 || RightAdd !=0 || BotAdd!=0) {
		x1 += LeftAdd;	y1 += TopAdd;	x2 -= RightAdd;	y2 -= BotAdd;
		if(debug) {
			dprintf("%sUser Adjust:   X1=%d  Y1=%d  X2=%d(W=%d,%d)  Y2=%d(H=%d,%d)",
				myName,x1,y1,x2,x2-x1+1,x2-w+1,y2,y2-y1+1,y2-h+1);
			dprintf("%s",myName);
		}
		if(x2 - x1 < wmod*2 || y2 - y1 < hmod*2)	env->ThrowError("%sIllegal user added offsets",myName);
	}

	// Crop Exact						// Make Exact found Coords: Default, QBCropX, QBCropY, QBCropW, QBCropH
	int cx0 = x1;
	int cy0 = y1;
	int cw0 = x2-x1+1;
	int ch0 = y2-y1+1;

	// CropLess
	int wrem1 = (cw0 % wmod == 0) ? 0 : wmod - (cw0 % wmod);
	int cx1 = std::max(int(cx0 - 0.5*wrem1),0) / xmod * xmod;
	int cw1 = (cw0 + (cx0-cx1) + wmod-1) / wmod * wmod;
    while(cx1+cw1>vi.width) {cw1 -= wmod;}								// Too wide, reduce mod WMOD
	//
	int hrem1 = (ch0 % hmod == 0) ? 0 : hmod - (ch0 % hmod);
	int cy1 = std::max(int(cy0 - 0.5*hrem1),0) / ymod * ymod;
	int ch1 = (ch0 + (cy0-cy1) + hmod-1) / hmod * hmod;
    while(cy1+ch1>vi.height) {ch1 -= hmod;}								// Too wide, reduce mod WMOD
	// CropMore
    int cx2 = int(cx0 + 0.5*(cw0 % wmod) + xmod-1) / xmod * xmod;
	int cw2 = (cw0 - (cx2-cx0)) / wmod * wmod;
	//
	int cy2 = int(cy0 + 0.5*(ch0 % hmod) + ymod-1) / ymod * ymod;
	int ch2 = (ch0 - (cy2-cy0)) / hmod * hmod;
	// CropPlus
    int cx3 = int(cx0 + 0.5*(cw0 % wmod) + 2 + xmod-1) / xmod * xmod;
	int cw3 = (cw0 - (cx3-cx0+2)) / wmod * wmod;
	//
	int cy3 = int(cy0 + 0.5*(ch0 % hmod) + 2 + ymod-1) / ymod * ymod;
	int ch3 = (ch0 - (cy3-cy0+2)) / hmod * hmod;
	//

	if(debug){
		dprintf("%sCropExact:  X =%3d   Y =%3d   W =%3d(%3d)   H =%3d(%3d)",myName,cx0,cy0,cw0,(cx0+cw0-w),ch0,(cy0+ch0-h));
		dprintf("%sCropLess:   XL=%3d   YL=%3d   WL=%3d(%3d)   HL=%3d(%3d)",myName,cx1,cy1,cw1,(cx1+cw1-w),ch1,(cy1+ch1-h));
		dprintf("%sCropMore:   XM=%3d   YM=%3d   WM=%3d(%3d)   HM=%3d(%3d)",myName,cx2,cy2,cw2,(cx2+cw2-w),ch2,(cy2+ch2-h));
		dprintf("%sCropPlus:   XP=%3d   YP=%3d   WP=%3d(%3d)   HP=%3d(%3d)",myName,cx3,cy3,cw3,(cx3+cw3-w),ch3,(cy3+ch3-h));
	}

	if(relative) {
		cw0 = cx0 + cw0 - w;	ch0 = cy0 + ch0 - h;
		cw1 = cx1 + cw1 - w;	ch1 = cy1 + ch1 - h;
		cw2 = cx2 + cw2 - w;	ch2 = cy2 + ch2 - h;
		cw3 = cx3 + cw3 - w;	ch3 = cy3 + ch3 - h;
	}
	char bf[1024];
	sprintf(bf,
		"%sX =%d %sY =%d %sW =%d %sH =%d\n"
		"%sXL=%d %sYL=%d %sWL=%d %sHL=%d\n"
		"%sXM=%d %sYM=%d %sWM=%d %sHM=%d\n"
		"%sXP=%d %sYP=%d %sWP=%d %sHP=%d\n"
		"%sTHRESH=%f\n",
		prefix,cx0,prefix,cy0,prefix,cw0,prefix,ch0,
		prefix,cx1,prefix,cy1,prefix,cw1,prefix,ch1,
		prefix,cx2,prefix,cy2,prefix,cw2,prefix,ch2,
		prefix,cx3,prefix,cy3,prefix,cw3,prefix,ch3,
		prefix,thresh
	);
	if(debug){
		dprintf("%sReturn:-\n",myName);
		dprintf("%s   %sX =%d %sY =%d %sW =%d %sH =%d\n",myName,prefix,cx0,prefix,cy0,prefix,cw0,prefix,ch0);
		dprintf("%s   %sXL=%d %sYL=%d %sWL=%d %sHL=%d\n",myName,prefix,cx1,prefix,cy1,prefix,cw1,prefix,ch1);
		dprintf("%s   %sXM=%d %sYM=%d %sWM=%d %sHM=%d\n",myName,prefix,cx2,prefix,cy2,prefix,cw2,prefix,ch2);
		dprintf("%s   %sXP=%d %sYP=%d %sWP=%d %sHP=%d\n",myName,prefix,cx3,prefix,cy3,prefix,cw3,prefix,ch3);
		dprintf("%s   %sTHRESH=%f\n",myName,prefix,thresh);
	}
	if(debug) {
		if(vi.IsPlanar() && wmod!=4)
			dprintf("%s*** WARNING *** Some players and VDubMod may require WMod=4",myName);
		double duration =  RT_TimerHP_Lo() - start_time;
		dprintf("%sScan Total Time=%.3f secs\n",myName,duration);
		dprintf("%s",myName);
	}
	return env->SaveString(bf);
}
