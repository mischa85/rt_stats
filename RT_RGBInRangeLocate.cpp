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

double __cdecl RT_RgbInRange_Lo(const AVSValue &std,const AVSValue &rgbminmax,IScriptEnvironment* env) {

	const char *myName="RT_RgbInRangeLo: ";
	if(!std[STD_CLIP].IsClip())
		env->ThrowError("%sMust have a clip",myName);
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
		env->ThrowError("%sRGB24 and RGB32 Only",myName);
	}

	int xx=std[STD_XX].AsInt(0);
	const int yy=std[STD_YY].AsInt(0);
	int	ww=			std[STD_WW].AsInt(0);
	int	hh=			std[STD_HH].AsInt(0);
	bool laced=std[STD_LACED].AsBool(false);

	if(ww <= 0) ww += vi.width  - xx;
	if(hh <= 0) hh += vi.height - yy;
	if(laced && ((hh & 0x01) == 0)) --hh;	// If laced then ensure ODD number of lines, last line other field does not count.
	laced=(laced && hh!=1 && vi.height!=1);

	if(xx <  0 || xx >=vi.width)				env->ThrowError("%sInvalid X coord",myName);
	if(yy <  0 || yy >=vi.height)				env->ThrowError("%sInvalid Y coord",myName);
	if(ww <= 0 || xx + ww > vi.width)			env->ThrowError("%sInvalid W coord",myName);
	if(hh <= 0 || yy + hh > vi.height)			env->ThrowError("%sInvalid H coord",myName);

	const int Rlo=		rgbminmax[RED_MIN].AsInt(128);
	const int Rhi=		rgbminmax[RED_MAX].AsInt(Rlo);

	const int Glo=		rgbminmax[GRN_MIN].AsInt(128);
	const int Ghi=		rgbminmax[GRN_MAX].AsInt(Glo);

	const int Blo=		rgbminmax[BLU_MIN].AsInt(128);
	const int Bhi=		rgbminmax[BLU_MAX].AsInt(Blo);

    const unsigned int Pixels = (ww * ((laced)?(hh+1)>>1:hh));

    PVideoFrame src         = child->GetFrame(n,env);
    const int   pitch       = src->GetPitch();
    const BYTE  *srcp       = src->GetReadPtr();
	const int ystep			= (laced) ? 2:1;
	const int ystride		= pitch*ystep;

	int64_t acc				= 0;
    unsigned long sum       = 0;
    int x, y;

    const int			xstep = (vi.IsRGB24()) ? 3 : 4;
	xx *= xstep;		ww *= xstep;
	srcp += ((vi.height-1 - yy) * pitch) + xx;	// Upside down RGB, height-1 is top line, -yy is top line of yy coord
	
	if(ww == xstep) {														// Special case for single pixel width
		for(y=0 ; y < hh; ++y) {
			if(
				srcp[0]>= Blo && 
				srcp[0]<= Bhi && 
				srcp[1]>= Glo && 
				srcp[1]<= Ghi && 
				srcp[2]>= Rlo && 
				srcp[2]<= Rhi
				) {
				++sum;
			}
			srcp -= ystride;
		}
	} else {
		for(y=0 ; y < hh; ++y) {                       
			for (x=0 ; x < ww; x += xstep) {								// x scans left to right
				if(
					srcp[x+0]>= Blo && 
					srcp[x+0]<= Bhi && 
					srcp[x+1]>= Glo && 
					srcp[x+1]<= Ghi && 
					srcp[x+2]>= Rlo && 
					srcp[x+2]<= Rhi
					) {
					++sum;
				}
				if(sum & 0x80000000) {acc += sum; sum=0;}					// avoid possiblilty of overflow
			}
			srcp -= ystride;
		}
	}
	acc += sum;	
	return 	 (double)acc / Pixels;
}


AVSValue __cdecl RT_RgbInRange(AVSValue args, void* user_data, IScriptEnvironment* env) {
	AVSValue mnmx[RGBMINMAX_SIZE] =	{args[8],args[9],args[10],args[11],args[12],args[13]};
	return (float)RT_RgbInRange_Lo(args,AVSValue(mnmx,RGBMINMAX_SIZE),env);
}


AVSValue __cdecl RT_RGBInRangeLocate(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_RgbInRangeLocate: ";
	if(!args[0].IsClip())
		env->ThrowError("%sMust have a clip",myName);
	PClip child			=	args[0].AsClip();
    const VideoInfo &vi =	child->GetVideoInfo();
	int n;
    if(args[1].IsInt())		{n  = args[1].AsInt(); }						// Frame n
    else {
        AVSValue cn       =	GetVar(env,"current_frame");
        if (!cn.IsInt())	env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
        n                 =	cn.AsInt();										// current_frame
    }
    n += args[2].AsInt(0);													// Delta, default 0
    n = (n<0) ? 0 : (n>=vi.num_frames)?vi.num_frames-1:n;					// Range limit frame n
	const int xx	=	args[3].AsInt(0);
	const int yy	=	args[4].AsInt(0);
	int ww	=	args[5].AsInt(0);
	int hh	=	args[6].AsInt(0);
	if(ww <= 0) ww += vi.width  - xx;
	if(hh <= 0) hh += vi.height - yy;
	if(xx <  0 || xx >=vi.width)				env->ThrowError("%sInvalid X coord",myName);
	if(yy <  0 || yy >=vi.height)				env->ThrowError("%sInvalid Y coord",myName);
	if(ww <= 0 || xx + ww > vi.width)			env->ThrowError("%sInvalid W coord",myName);
	if(hh <= 0 || yy + hh > vi.height)			env->ThrowError("%sInvalid H coord",myName);	
	const int baffle		=	args[7].AsInt(8);
	double thresh	=	args[8].AsFloat(0.0);
	const int Rlo	=	args[9].AsInt(128);
	const int Rhi	=	args[10].AsInt(255);
	const int Glo	=	args[11].AsInt(128);
	const int Ghi	=	args[12].AsInt(255);
	const int Blo	=	args[13].AsInt(128);
	const int Bhi	=	args[14].AsInt(255);
	const char*prefix=	args[15].AsString("RGBIRL_");
	const bool debug=	args[16].AsBool(false);
	int baff_w		=	args[17].AsInt(baffle);
	int baff_h		=	args[18].AsInt(baffle);
	double thresh_w =	args[19].AsFloat((float)thresh);
	double thresh_h =	args[20].AsFloat((float)thresh);
	bool rescan     =	args[21].AsBool(false);

	if(xx<0||yy<0||ww<=0||hh<=0)				env->ThrowError("%sillegal coordinates",myName);
	if(baff_w<1)								env->ThrowError("%sillegal baffle_w",myName);
	if(baff_h<1)								env->ThrowError("%sillegal baffle_h",myName);

	if(Rlo<0 || Rlo>255)						env->ThrowError("%sillegal Rlo (0.0->255)",myName);
	if(Rhi<Rlo || Rhi>255)						env->ThrowError("%sillegal Rhi (Rlo->255)",myName);

	if(Glo<0 || Glo>255)						env->ThrowError("%sillegal Glo (0.0->255)",myName);
	if(Ghi<Glo || Ghi>255)						env->ThrowError("%sillegal Ghi (Glo->255)",myName);

	if(Blo<0 || Blo>255)						env->ThrowError("%sillegal Blo (0.0->255)",myName);
	if(Bhi<Blo || Bhi>255)						env->ThrowError("%sillegal Bhi (Blo->255)",myName);

	baff_w = (baff_w>ww) ? ww : baff_w;
	baff_h = (baff_h>hh) ? hh : baff_h;

	// thresh > 0.0 is percent thresh, < 0.0 is pixel width,height thresh, 0.0 is any in range pixel at all.
	if(thresh_w > 0.0) {
		thresh_w=std::min(thresh_w,100.0);			// silent error
		thresh_w /= 100.0;						// range 0.0 -> 1.0 for RT_YInRange()
		// At least baffle minus a little bit (Note these are back-to-front, eg thresh_w is for top/bot edges]
		// th has to be GREATER THAN, hence the 'minus a little bit'.
		thresh_w = thresh_w * ww - (1.0/ww);
	} else if (thresh_w < 0.0) {
		thresh_w = std::min<int>(-thresh_w,ww);			// negate, limit max width, silent err
		thresh_w -= (1.0/ww);
	}

	if(thresh_h > 0.0) {
		thresh_h=std::min(thresh_h,100.0);			// silent error
		thresh_h /= 100.0;						// range 0.0 -> 1.0 for RT_YInRange()
		thresh_h = thresh_h * hh - (1.0/hh);
	} else if (thresh_h < 0.0) {
		thresh_h = std::min<int>(-thresh_h,hh);			// negate, limit max height, silent err
		thresh_h -= (1.0/hh);
	}

	AVSValue std[STD_SIZE]= {child,n,0,xx,yy,ww,1,false};				// child,n,delta,x,y,w,1,laced
	AVSValue mnmx[RGBMINMAX_SIZE] =	{Rlo,Rhi,Glo,Ghi,Blo,Bhi};

	
	int x1,x2,y1,y2;
	x1=xx;
	y1=yy;
	x2=xx+ww-1;
	y2=yy+hh-1;

	const double ThPix_w = thresh_w;
	const double ThPix_h = thresh_h;
	double Th_w,Th_h;
	int SCur,MCur,ECur;
	double threm;
	
	int ret[4]={-1,-1,-1,-1};
	
	bool HorizontalEdge = true;				// Start with Top and Bot edges (horizontal edge)
	bool vChanged=true,hChanged=false;		// Force horizontal

	while((HorizontalEdge&&vChanged)||(!HorizontalEdge&&hChanged)) {
		int x,y;
		if(HorizontalEdge) {
			int Org_y1=ret[1],Org_y2=ret[3];
			ret[1]=ret[3]= -1;
			std[STD_XX]=x1;			// x
			std[STD_WW]=x2-x1+1;	// w
			std[STD_HH]=1;			// h

			Th_w = ThPix_w / (x2-x1+1);			// recalc thresh for current horizontal length
			if(Th_w>=1.0) {
				if(debug) {
					dprintf("%s %d] Width too narrow for Thresh_w, cannot succeed, breaking search",myName,n);
				}
				break;
			}

			SCur=y1; MCur=SCur; ECur=SCur+baff_h-1;
			while(ECur<=y2) {											// top edge
				y=ECur;
				while(y>=MCur) {
					std[STD_YY]=y;		// y
					const double th=RT_RgbInRange_Lo(AVSValue(std,STD_SIZE),AVSValue(mnmx,RGBMINMAX_SIZE),env);
					if(th>Th_w) {									// !!! Th_w for horizontal edge (T,B) !!!
						if(SCur==MCur)	threm=th;
						--y;
					} else
						break;
				}
				if(y<MCur) {
					ret[1] = y1 =SCur;
					if(debug) {
						dprintf("%s %d] Top %4d : x1=%-4d y1=%-4d x2=%-4d y2=%-4d %4dx%-4d (outer edge passing Th : %7.3f%% %.1f pixels)",
								myName,n,ret[1],x1,y1,x2,y2,x2-x1+1,y2-y1+1,threm*100.0,threm*(x2-x1+1));
					}
					break;
				}
				else {SCur=y+1; MCur=ECur+1; ECur=y+baff_h;}
			}
			if(ret[1] < 0)	{
				break;
			}

			SCur=y2; MCur=SCur; ECur=SCur-baff_h+1;						// bottom edge
			while(ECur>=y1) {
				y=ECur; 
				while(y<=MCur) {
					std[STD_YY]=y;		// y
					const double th=RT_RgbInRange_Lo(AVSValue(std,STD_SIZE),AVSValue(mnmx,RGBMINMAX_SIZE),env);
					if(th>Th_w) {								// !!! Th_w for Horizotnal edge (T,B) !!!
						if(SCur==MCur)		threm=th;			// remember outermost passing threshold
						++y;
					} else 
						break;
				}
				if(y>MCur) 			{
					ret[3] = y2 = SCur;
					if(debug) {
						dprintf("%s %d] Bot %4d : x1=%-4d y1=%-4d x2=%-4d y2=%-4d %4dx%-4d (outer edge passing Th : %7.3f%% %.1f pixels)",
							myName,n,ret[3],x1,y1,x2,y2,x2-x1+1,y2-y1+1,threm*100.0,threm*(x2-x1+1));
					}
					break;
				}
				else  {SCur=y-1; MCur=ECur-1; ECur=y-baff_h;}
			}
			if(ret[3] < 0)	{
				break;
			}
			vChanged=false;
			hChanged=(Org_y1!=y1 || Org_y2 != y2);
		} else { // Vertical edge (L,R)
			int Org_x1=ret[0],Org_x2=ret[2];
			ret[0]=ret[2]=-1;
			std[STD_YY]=y1;			// y
			std[STD_HH]=y2-y1+1;	// h
			std[STD_WW]=1;			// w

			Th_h = ThPix_h / (y2-y1+1);			// recalc thresh for current vertical length

			if(Th_h>=1.0) {
				if(debug) {
					dprintf("%s%d] Height too short for Thresh_h, cannot succeed, breaking search",myName,n);
				}
				break;
			}
			

			SCur=x1; MCur=SCur; ECur=SCur+baff_w-1;						// left edge
			while(ECur<=x2) {	
				x=ECur;
				while(x>=MCur) {
					std[STD_XX]=x;		// x
					const double th=RT_RgbInRange_Lo(AVSValue(std,STD_SIZE),AVSValue(mnmx,RGBMINMAX_SIZE),env);
					if(th>Th_h) {								// !!! Th_h for vertical edge (L,R) !!!
						if(SCur==MCur)		threm=th;
						--x;
					} else 
						break;

				}
				if(x<MCur) 			{
					ret[0] = x1 = SCur;
					if(debug) {
						dprintf("%s %d] Lft %4d : x1=%-4d y1=%-4d x2=%-4d y2=%-4d %4dx%-4d (outer edge passing Th : %7.3f%% %.1f pixels)",
								myName,n,ret[0],x1,y1,x2,y2,x2-x1+1,y2-y1+1,threm*100.0,threm*(y2-y1+1));
					}
					break;
				}
				else	{SCur=x+1; MCur=ECur+1; ECur=x+baff_w;}
			}
			if(ret[0] < 0)	{
				break;
			}

			SCur=x2; MCur=SCur; ECur=SCur-baff_w+1;						// right edge
			while(ECur>=x1) {
				x=ECur;
				while(x<=MCur) {
					std[STD_XX]=x;		// x
					const double th=RT_RgbInRange_Lo(AVSValue(std,STD_SIZE),AVSValue(mnmx,RGBMINMAX_SIZE),env);
					if(th>Th_h) {								// !!! Th_h for vertical edge (L,R) !!!
						if(SCur==MCur)		threm=th;
						++x;
					} else 
						break;
				}	
				if(x>MCur) 			{
					ret[2] = x2 = SCur;
					if(debug) {
						dprintf("%s %d] Rgt %4d : x1=%-4d y1=%-4d x2=%-4d y2=%-4d %4dx%-4d (outer edge passing Th : %7.3f%% %.1f pixels)",
								myName,n,ret[2],x1,y1,x2,y2,x2-x1+1,y2-y1+1,threm*100.0,threm*(y2-y1+1));
					}
					break;
				}
				else 	{SCur=x-1; MCur=ECur-1; ECur=x-baff_w;}
			}				
			if(ret[2] < 0)	{
				break;
			}
			hChanged=false;
			vChanged=(Org_x1!=x1 || Org_x2 != x2);
		}
		HorizontalEdge=!HorizontalEdge;		// Toggle edge mode
		if(!rescan && HorizontalEdge) {
			if(debug) {
				dprintf("%s %d] Perimeter no recan detect @ x1=%d y1=%d x2=%d y2=%d (%dx%d)",
					myName,n,ret[0],ret[1],ret[2],ret[3],ret[2]-ret[0]+1,ret[3]-ret[1]+1);
			}
			break;							// done 2 passes, no rescan exit
		}
	} // end while

	bool result=false;
	if(ret[2]>=0) {
		if(debug) {
			dprintf("%s %d] Successful Detection @ x1=%d y1=%d x2=%d y2=%d (%dx%d)",
				myName,n,ret[0],ret[1],ret[2],ret[3],ret[2]-ret[0]+1,ret[3]-ret[1]+1);
			dprintf("%s",myName);
		}
		ret[2]=ret[2]-ret[0]+1;
		ret[3]=ret[3]-ret[1]+1;
		if(debug) {
			dprintf("%s %d] Returned Locals %sX=%d %sY=%d %sW=%d %sH=%d",
				myName,n,prefix,ret[0],prefix,ret[1],prefix,ret[2],prefix,ret[3]);
			dprintf("%s",myName);
		}
		const char *p,*nam[]={"X","Y","W","H"};
		int pfixlen=int(strlen(prefix));
		if(pfixlen>128)
			pfixlen=128;
		char bf[128+32];
		memcpy(bf,prefix,pfixlen);
		for(int i=0;i<4;++i) {
			char *d=bf+pfixlen;
			for(p=nam[i];*d++=*p++;);			// strcat variable name part
			AVSValue var  =	GetVar(env,bf);
			env->SetVar(var.Defined()?bf:env->SaveString(bf),ret[i]);
		}
		result = true;
	} else if(debug) {
		dprintf("%s %d] NOT FOUND",myName,n);
		dprintf("%s",myName);
	}

	return result;
}

