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

// FingerPrint

#define QWIKSCAN_MAGIC 'SKWQ'		// 'QWKS'

#define QWIKSCANVER 3

#define QWIKSCAN_RGB 'BGR '			// ' RGB"
#define QWIKSCAN_YUV 'VUY '			// ' YUV"
#define QWIKSCAN_Y8  '8Y  '			// '  Y8"

#define FINGERTYPEID 5				// 2 = float, 5 = double

typedef double FINGERTYPE;			// float or double (as for above FINGERTYPEID)					

#define FINGERELS	16				// Number of Elements in Fingerprint
#define FPFAILMAX   8				// Minimum number of Finger print component failures LumaTol failures that will still match.

#define LUMATOL_MAX_RANGE 20
#define LUMATOL_MAX double(LUMATOL_MAX_RANGE)

struct myfinger_stc{
	FINGERTYPE	lolim[FINGERELS];	
	FINGERTYPE	hilim[FINGERELS];
};

typedef struct myfinger_stc FINGER;

// FingerPrint Component Histogram Ranges

static const int rng_map[FINGERELS][2]={   
	{0,47},
	{48,207},
	{208,255},

	{0,63},
	{64,191},
	{192,255},

	{0,79},
	{80,175},
	{176,255},

	{0,95},
	{96,159},
	{160,255},

	{0,111},
	{144,255},

	{32, 127},
	{128,223},
};

extern int __cdecl	RT_Fingerprint_Lo(const AVSValue &std,const AVSValue &xtra,FINGER &finger,unsigned int *hist,IScriptEnvironment* env,double LTol=0.0);

int __cdecl RT_Fingerprint_Lo(const AVSValue &std,const AVSValue &xtra,FINGER &finger,unsigned int *hist,IScriptEnvironment* env,double LTol) {
// NOTE, LTol optional default 0.0
	char *myName="RT_Fingerprint_Lo: ";
	LTol  = std::max(std::min(LTol,LUMATOL_MAX),0.0);
	MYLO mylo;
	RT_MYstats_Lo(RTHIST_F,std,xtra,mylo,myName,env,hist);				// Get luma histogram
	// FingerPrint Component Histogram Ranges

	const int Pixels = mylo.pixels;

	double frac,whole;
	frac=modf(LTol,&whole);
	int Tol = int(whole);
	int i,j;
	if(frac == 0.0) {									// Integer LumaTol, no fractional part
		for(i=FINGERELS;--i>=0;) {
			unsigned int sm=0;
			const int s = rng_map[i][0];				// Start of range
			const int e = rng_map[i][1];				// End of range
			int s2 = s+Tol;								// contract lolim range by LumaTol Int
			int e2 = e-Tol;
			for(j=s2;j<=e2;++j)
				sm += hist[j];
			const double  smd = sm * 255.0 / Pixels;	// cannot be bigger than 255.0
			finger.lolim[i] = smd;
			if(Tol==0) {
				finger.hilim[i]=smd;					// EXACT FINGERPRINT components, search identical frames OR ARR FINGER
			} else {
				sm=0;
				s2 = std::max(s-Tol,0);						// expand hilim range by LumaTol Int
				e2 = std::min(e+Tol,255);
				for(j=s2;j<=e2;++j)
					sm += hist[j];
				finger.hilim[i] = sm * 255.0 / Pixels;	// cannot be bigger than 255.0
			}
			dprintf("%s %3d] lo=%7.3f hi=%7.3f",myName,i,finger.lolim[i],finger.hilim[i]);
		}
	} else {
		const double AvePixPerLevel = Pixels / 256.0;
		for(i=FINGERELS;--i>=0;) {
			const int s = rng_map[i][0];
			const int e = rng_map[i][1];
			int s2 = s+Tol;										// contract lolim range by LumaTol Int
			int e2 = e-Tol;
			unsigned int sm=0;
			for(j=s2;j<=e2;++j)
				sm += hist[j];
			// Slower but safer more stable method, take biggest possible interpretation of fraction. Process separately
			double frac_cnt_s = hist[s2];
			if(frac_cnt_s > AvePixPerLevel)			frac_cnt_s *= frac; 
			else									frac_cnt_s = AvePixPerLevel * frac;
			double frac_cnt_e = hist[e2];
			if(frac_cnt_e > AvePixPerLevel)			frac_cnt_e *= frac; 
			else									frac_cnt_e = AvePixPerLevel * frac;
			double smd = sm - (frac_cnt_s + frac_cnt_e);		// Fractional reduction of extremities
			smd -= 2.0;											// Round down pixel count by 1 pixel for each extremity
			smd = smd * 255.0 / Pixels;							// Pixel count range 0.0 -> 255.0
			smd = std::max(std::min(255.0,smd),0.0);						// Ensure valid range limit
			finger.lolim[i] = smd;
			s2 = std::max(s-Tol,0);									// expand hilim range by LumaTol Int
			e2 = std::min(e+Tol,255);
			sm=0;
			for(j=s2;j<=e2;++j)
				sm += hist[j];
			frac_cnt_s = 0.0;
			if(s2 > 0) {										// fractional increase possible ?
				frac_cnt_s = hist[s2-1];
				if(frac_cnt_s > AvePixPerLevel)			frac_cnt_s *= frac; 
				else									frac_cnt_s = AvePixPerLevel * frac;
			}
			frac_cnt_e = 0.0;
			if(e2 < 255) {										// fractional increase possible ?
				frac_cnt_e = hist[e2+1];
				if(frac_cnt_e > AvePixPerLevel)			frac_cnt_e *= frac; 
				else									frac_cnt_e = AvePixPerLevel * frac;
			}
			smd = sm + (frac_cnt_s + frac_cnt_e);				// Fractional increase of extremities
			smd += 2.0;											// Round up pixel count by 1 pixel for each extremity
			smd = smd * 255.0 / Pixels;							// Pixel count range 0.0 -> 255.0
			smd = std::max(std::min(255.0,smd),0.0);						// Ensure valid range limit
			finger.hilim[i] = smd;
		}
	}
	return Pixels;
}



AVSValue __cdecl RT_QwikScanEstimateLumaTol(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *myName="RT_QwikScanEstimateLumaTol: ";

	PClip child			= args[0].AsClip();
	PClip child2		= args[1].AsClip();
	int n,n2;
	if(args[2].IsInt()) {n  = args[2].AsInt(); }
	else {
		AVSValue cn       =	GetVar(env,"current_frame");
		if (!cn.IsInt())	env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
		n                 =	cn.AsInt();										// current_frame
	}
	if(args[3].IsInt()) {n2  = args[3].AsInt(); }
	else {
		AVSValue cn       =	GetVar(env,"current_frame");
		if (!cn.IsInt())	env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
		n2                =	cn.AsInt();										// current_frame
	}

    const VideoInfo &vi = child->GetVideoInfo();
	// width=0 means no video, most plugins dont bother with below check (I dont either usually, but probably should)
	if(vi.num_frames <= 0 || vi.width==0) env->ThrowError("%sClips must have video",myName);
	n = (n < 0) ? 0 : (n >= vi.num_frames) ? vi.num_frames - 1 : n;		// range limit n to valid frames

    const VideoInfo &vi2 = child2->GetVideoInfo();
	// width=0 means no video, most plugins dont bother with below check (I dont either usually, but probably should)
	if(vi2.num_frames <= 0 || vi2.width==0) env->ThrowError("%sClips must have video",myName);
	n2 = (n2 < 0) ? 0 : (n2 >= vi2.num_frames) ? vi2.num_frames - 1 : n2;		// range limit n to valid frames

	if(!vi.IsSameColorspace(vi2))                          env->ThrowError("%sDissimilar ColorSpace\n",myName);
	if(vi.width != vi2.width || vi.height != vi2.height)   env->ThrowError("%sDissimilar dimensions\n",myName); 

	if(vi.width==0 || vi.num_frames==0 || vi2.width==0 || vi2.num_frames==0)
		env->ThrowError("%sBoth clips must have video",myName);

	const int matrix	= args[4].AsInt(vi.width>1100 || vi.height>600?3:2) & 0x03;
	// Matrix: REC601 : 1=REC709 : 2 = PC601 : 3 = PC709

	unsigned int *hist1	= NULL;
	unsigned int *hist2	= NULL;

	if((hist1=new unsigned int[256])==NULL) {
		env->ThrowError("%sError, Allocating Histogram1 buffer",myName);
	}
	if((hist2=new unsigned int[256])==NULL) {
		delete [] hist1;
		env->ThrowError("%sError, Allocating Histogram2 buffer",myName);
	}

	AVSValue std1[STD_SIZE]    =	{child,0,0,0,0,0,0,false};
	AVSValue xtra1[XTRA_SIZE];
	xtra1[XTRA_RGBIX]          =	matrix;
	std1[STD_FRAME]	           =	n;

	AVSValue std2[STD_SIZE]	   =	{child2,0,0,0,0,0,0,false};
	AVSValue xtra2[XTRA_SIZE];
	xtra2[XTRA_RGBIX]          =	matrix;
	std2[STD_FRAME]	           =	n2;

	MYLO mylo;

	RT_MYstats_Lo(RTHIST_F,AVSValue(std1,STD_SIZE),AVSValue(xtra1,XTRA_SIZE),mylo,myName,env,hist1);
	RT_MYstats_Lo(RTHIST_F,AVSValue(std2,STD_SIZE),AVSValue(xtra2,XTRA_SIZE),mylo,myName,env,hist2);
	const int Pixels = mylo.pixels;
	const double AvePixPerLevel = Pixels / 256.0;

	int i;

	int cnt1[FINGERELS];
	int cnt2[FINGERELS];
	int j;

	// count ranges
	for(i=FINGERELS;--i>=0;) {
		unsigned int sm1=0,sm2=0;
		const int s = rng_map[i][0];					// Start of range
		const int e = rng_map[i][1];					// End of range
		for(j=s;j<=e;++j) {
			sm1 += hist1[j];
			sm2 += hist2[j];
		}
		cnt1[i] = sm1;
		cnt2[i] = sm2;
	}

	double result=0.0;
	// contract ranges
	for(i=FINGERELS;--i>=0;) {
		double fraction=0.0;
		int subi=0,contract=cnt2[i];
		if(contract > cnt1[i]) {						 // do we need to contract ?
			for(int tol=0;tol<LUMATOL_MAX_RANGE;tol++) { // LUMATOL_RANGE steps, 0 -> LUMATOL_RANGE-1 for contract (inclusive of existing range)
				int s = rng_map[i][0]+tol;				 // Start of range
				int e = rng_map[i][1]-tol;				 // End of range
				int sm= hist2[s] + hist2[e];			 // We cannot stray over 0 and 255 boundaries
				if(contract - sm >= cnt1[i]) {
					contract -= sm;
					++subi;					
				} else {
					int dif	= contract-cnt1[i];		// difference in pixel counts 
					dif += 2;						// pixel count round up for each extremity
					// Do NOT base off sm to keep comparable across different frames
					// QwikScan will use greatest possible interpretation for fraction
					fraction = double(dif) / (AvePixPerLevel*2.0);
					fraction = std::min(std::max(0.0,fraction),1.0);
					break;
				}
			}
		}
		const double d = subi + fraction;
		if(result<d)	result = d;
	}

	// expand ranges
	for(i=FINGERELS;--i>=0;) {
		double fraction = 0.0;
		int addi=0,expand=cnt2[i];
		if(expand < cnt1[i]) {							  // do we need to expand ?
			for(int tol=1;tol<=LUMATOL_MAX_RANGE;tol++) { // LUMATOL_RANGE steps, 1 -> LUMATOL_RANGE (external to existing range)
				int s = rng_map[i][0]-tol;				  // Start of range
				int e = rng_map[i][1]+tol;				  // End of range
				int smf = 0;
				if(s >= 0)		smf += hist2[s];
				if(e <= 255)	smf += hist2[e];
				if(expand+smf <= cnt1[i]) {
					expand += smf;
					++addi;					
				} else {
					int dif	= cnt1[i]-expand;		// difference in pixel counts
					if(s>=0 && e<=255) {
						dif += 2;					// pixel count round up for each extremity
						// Do NOT base on sm to keep comparable across different frames
						// QwikScan will use greatest possible interpretation for fraction
						fraction = double(dif) / (AvePixPerLevel*2.0);
					} else {
						dif ++;						// pixel count round up for one extremity
						// Do NOT base on sm to keep comparable across different frames
						// QwikScan will use greatest possible interpretation for fraction
						fraction = double(dif) / AvePixPerLevel;
					}
					fraction = std::min(std::max(0.0,fraction),1.0);
					break;
				}
			}
		}
		const double d = addi + fraction;
		if(result<d)	result = d;
	}

	delete [] hist1;
	delete [] hist2;
	
	if(result > 0.0) {
		int exponent;
		double mantissa = frexp(result,&exponent);
		double add = pow(10.0,-double(FLT_DIG));
		mantissa = mantissa + add; 
		result = ldexp(mantissa,exponent);
	}
	return  std::min(result,LUMATOL_MAX);
}






AVSValue __cdecl RT_QwikScanCreate(AVSValue args, void* user_data, IScriptEnvironment* env) {

	const char * myName	= "RT_QwikScanCreate: ";
	double start_time	= RT_TimerHP_Lo();
	PClip child			= args[0].AsClip();
	const VideoInfo &vi = child->GetVideoInfo();

	const char * arrfn	= args[1].AsString("");
	const char * Prevarrfn= args[2].AsString("");
	const char * Nextarrfn= args[3].AsString("");
	const int	matrix	= args[4].AsInt(vi.width>1100 || vi.height>600?3:2) & 0x03;
	const bool	debug	= args[5].AsBool(false);
	const int xx= args[6].AsInt(0);
	const int yy= args[7].AsInt(0);
	int ww= args[8].AsInt(0);
	int hh= args[9].AsInt(0);

    const int framecount=vi.num_frames;
    const int lastframe=framecount-1;

	if(*arrfn=='\0')
		env->ThrowError("%sARR, Empty Filename",myName);
	if(vi.width==0 || vi.num_frames==0)
		env->ThrowError("%sClip must have video",myName);

	if(*Prevarrfn=='\0' && *Nextarrfn=='\0')
		env->ThrowError("%sEither Prev ARR and/or Next ARR is compulsory",myName);

	if(ww <= 0) ww += vi.width  - xx;
	if(hh <= 0) hh += vi.height - yy;
	if(xx <  0 || xx >=vi.width)				env->ThrowError("%sInvalid X coord(%d)",myName,xx);
	if(yy <  0 || yy >=vi.height)				env->ThrowError("%sInvalid Y coord(%d)",myName,yy);
	if(ww <= 0 || xx + ww > vi.width)			env->ThrowError("%sInvalid W(%d) coord for X",myName,ww);
	if(hh <= 0 || yy + hh > vi.height)			env->ThrowError("%sInvalid H(%d) coord for Y",myName,hh);

	bool rgb	= vi.IsRGB();
	bool planar = vi.IsPlanar();

	int Channels = 3;
	if(planar) {
		PVideoFrame src = child->GetFrame(0,env);
		if(src->GetRowSize(PLANAR_U) == 0) {
			Channels = 1;
		}
	}

	AVSValue std[STD_SIZE]	=	{child,0,0,xx,yy,ww,hh,false};
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_RGBIX]		=	matrix;

	double T_start =  RT_TimerHP_Lo();
	if(debug)	{
		dprintf("\n%sRT_QwikScanCreate() by StainlessS\n",myName);
		dprintf("%sArrayAlloc Finger Array",myName);
	}

	AVSValue arrargs[] = {arrfn,FINGERTYPEID,framecount,FINGERELS};
    RT_ArrayAlloc(AVSValue(arrargs,4),NULL,env);				
	if(debug)	{
		double T_time =  RT_TimerHP_Lo();
		dprintf("%sFinger Array Alloc time = %.2f seconds (%.2f mins)\n",myName,T_time-T_start,(T_time-T_start)/60.0);    
	}
	arrargs[3]=FINGERELS*256;
	if(*Prevarrfn!='\0') {
		arrargs[0]=Prevarrfn;
		arrargs[1]=1;											// type int
		if(debug)	dprintf("%sArrayAlloc Prev Locator Array",myName);
		T_start =  RT_TimerHP_Lo();
		RT_ArrayAlloc(AVSValue(arrargs,4),NULL,env);
		if(debug)	{
			double T_time =  RT_TimerHP_Lo();
			dprintf("%sPrev Locator Array Alloc time = %.2f seconds (%.2f mins)\n",myName,T_time-T_start,(T_time-T_start)/60.0);    
		}
	}
	if(*Nextarrfn!='\0') {
		arrargs[0]=Nextarrfn;
		arrargs[1]=1;											// type int
		if(debug)	dprintf("%sArrayAlloc Locator Next Array",myName);
		T_start =  RT_TimerHP_Lo();
		RT_ArrayAlloc(AVSValue(arrargs,4),NULL,env);
		if(debug)	{
			double T_time =  RT_TimerHP_Lo();
			dprintf("%sNext Locator Array Alloc time = %.2f seconds (%.2f mins)\n",myName,T_time-T_start,(T_time-T_start)/60.0);    
		}
	}

	char msg[1024+1];
	msg[0]='\0';												// Clear for error message

	// At this point all three ARR's created (if required), nothing is open, ie no FILE pointers nor memory allocs

	FINGERTYPE*bf		= NULL;
	int	  *arrt			= NULL;
	unsigned int *hist	= NULL;

	FILE *fp	= NULL;
	FILE *fp2	= NULL;
	FILE *fp3	= NULL;

	MYARR arr;
	MYARR arr2;
	MYARR arr3;

	enum {NBLOCKS=128};

	const int blks   = framecount / NBLOCKS;
	const int odds   = (framecount % NBLOCKS) * FINGERELS;
	const int blknel = FINGERELS*NBLOCKS;

	if((bf=new FINGERTYPE[blknel])==NULL) {
		sprintf(msg,"%sError, Allocating Finger buffer",myName);
	}else if((hist=new unsigned int[256])==NULL) {
		sprintf(msg,"%sError, Allocating Histogram buffer",myName);
	} else if((fp = ARR_Read_Header(myName,arrfn,"rb+",arr,env,msg))!=NULL) {
		if(*Prevarrfn!='\0')
			fp2=ARR_Read_Header(myName,Prevarrfn,"rb+",arr2,env,msg);
		if(msg[0]=='\0' && *Nextarrfn!='\0')
			fp3=ARR_Read_Header(myName,Nextarrfn,"rb+",arr3,env,msg);
		if(msg[0]=='\0' && (arrt = new int[FINGERELS*256])==NULL)
			sprintf(msg,"%sError, Allocating arrt buffer for Prev/Next ARR",myName);
	}

	if(msg[0]) {
		if(bf)	delete [] bf;
		if(arrt)delete [] arrt;
		if(hist)delete [] hist;
		if(fp)	fclose(fp);
		if(fp2) fclose(fp2);
		if(fp3) fclose(fp3);
		env->ThrowError(msg);								// return message set by us, or ARR_Read_Header
	}

	// At this point all required FILE's open, hist, bf, and arrt
	
	arr.id[0].i = QWIKSCAN_MAGIC;
	arr.id[1].i = QWIKSCANVER;
	arr.id[2].i = (rgb) ? QWIKSCAN_RGB : (Channels==1) ? QWIKSCAN_Y8 : QWIKSCAN_YUV;
	arr.id[3].i = Channels;
	arr.id[4].i = vi.width;
	arr.id[5].i = vi.height;
	arr.id[6].i = matrix;
	arr.id[7].i = framecount;
	arr.id[8].i = xx;
	arr.id[9].i = yy;
	arr.id[10].i = ww;
	arr.id[11].i = hh;

	FINGER Finger;

	int el;
	int blk;
	int frame;
	
	if(fseek(fp,arr.offset,SEEK_SET)) {
		strcpy(msg,"seeking ARR file");
	} else {
		if(debug) dprintf("%sFilling ARR with FingerPrint data (Will take some time)\n",myName);
		frame=0;
		int fix = 1;
		int flim = int(framecount / 20.0 * fix);
		for(blk=0;msg[0]=='\0' && blk<blks;++blk) {
			for(el=0;el<blknel;el+=FINGERELS) {
				std[STD_FRAME]	=	frame;	// n
				RT_Fingerprint_Lo(AVSValue(std,STD_SIZE),AVSValue(xtra,XTRA_SIZE),Finger,hist,env);
				for(int j=FINGERELS;--j>=0;) {
					bf[el+j] = Finger.lolim[j];
				}
				++frame;
				if(debug && frame>=flim) {
					dprintf("%sFingerprint[%d] %.1f%%",myName,frame-1,frame*100.0/framecount);
					++fix;
					flim = int(framecount / 20.0 * fix);
				}
			}
			if(fwrite(bf,blknel*sizeof(bf[0]),1,fp) != 1) {
				strcpy(msg,"writing ARR file data");
			}
		}

		if(msg[0]=='\0') {
			if(odds > 0) {
				for(el=0;el<odds;el+=FINGERELS) {
					std[STD_FRAME]	=	frame;	// n
					RT_Fingerprint_Lo(AVSValue(std,STD_SIZE),AVSValue(xtra,XTRA_SIZE),Finger,hist,env);
					for(int j=FINGERELS;--j>=0;) {
						bf[el+j] = Finger.lolim[j];
					}
					++frame;
					if(debug && frame>=flim) {
						dprintf("%sFingerprint[%d] %.1f%%",myName,frame-1,frame*100.0/framecount);
						++fix;
						flim = int(framecount / 20.0 * fix);
					}
				}
				if(fwrite(bf,odds * sizeof(bf[0]),1,fp) != 1) {
					strcpy(msg,"writing ARR file records (odds)");
				}
			}
			if(msg[0]=='\0') {
				rewind(fp);
				if(fwrite(&arr,sizeof(arr),1,fp) != 1)
					strcpy(msg,"Updating Array header");
			}
		}

		int i;

		// PREV
		if(msg[0]=='\0' && *Prevarrfn!='\0') {
			if(debug) dprintf("%sFilling Prev Locator Array with data\n",myName);

			for(i=0;i<12;++i) {
				arr2.id[i] = arr.id[i];
			}
			arr2.id[12].i = -1;												// Flag Prev

			for(i=FINGERELS*256;--i>=0;arrt[i]=-1);							// Init arrt to invalid

			if(fseek(fp,arr.offset,SEEK_SET)) {
				strcpy(msg,"seeking Prev Locator Array data");
			} else if(fseek(fp2,arr2.offset,SEEK_SET)) {
				strcpy(msg,"seeking Prev Locator Array data");
			} else {
				frame=0;
				fix = 1;
				flim = int(framecount / 20.0 * fix);
				for(blk=0;msg[0]=='\0' && blk<blks;++blk) {
					if(fread(bf,blknel*sizeof(bf[0]),1,fp)!=1) {
						strcpy(msg,"reading Prev Locator Array");
					} else {
						for(el=0;msg[0]=='\0' && el<blknel;el+=FINGERELS) {
							if(fwrite(arrt,FINGERELS*256*sizeof(arrt[0]),1,fp2) != 1) {
								strcpy(msg,"writing Prev Locator Array");
							} else {
								for(i=FINGERELS;--i>=0;) {
									arrt[(i*256) + (int)(bf[el+i])] = frame;
								}
								++frame;
								if(msg[0]=='\0' && debug && frame>=flim) {
									dprintf("%sPrev Locator Array[%d] %.1f%%",myName,frame-1,frame*100.0/framecount);
									++fix;
									flim = int(framecount / 20.0 * fix);
								}
							}
						}
					}
				}
				if(msg[0]=='\0' && odds > 0) {
					if(fread(bf,odds * sizeof(bf[0]),1,fp)!=1) {
						strcpy(msg,"reading Prev Locator Array ODDS");
					} else {
						for(el=0;msg[0]=='\0' && el<odds;el+=FINGERELS) {
							if(fwrite(arrt,FINGERELS*256*sizeof(arrt[0]),1,fp2) != 1) {
								strcpy(msg,"writing Prev Locator Array ODDS");
							} else {
								for(i=FINGERELS;--i>=0;) {
									arrt[(i*256) + (int)(bf[el+i])] = frame;
								}
								++frame;
								if(msg[0]=='\0' && debug && frame>=flim) {
									dprintf("%sPrev Locator Array[%d] %.1f%%",myName,frame-1,frame*100.0/framecount);
									++fix;
									flim = int(framecount / 20.0 * fix);
								}
							}
						}
					}
				}
				if(msg[0]=='\0') {
					rewind(fp2);
					if(fwrite(&arr2,sizeof(arr2),1,fp2) != 1)
						strcpy(msg,"Updating Prev Locator Array header");
				}
			}
		} // End Prev


		// NEXT
		if(msg[0]=='\0' && *Nextarrfn!='\0') {
			if(debug) dprintf("%sFilling Next Locator Array with data\n",myName);

			for(i=0;i<12;++i) {
				arr3.id[i] = arr.id[i];
			}
			arr3.id[12].i = 1;												// Flag Next ARR

			for(i=FINGERELS*256;--i>=0;arrt[i]=-1);									// Init arrt to invalid

			frame=framecount;
			fix  = 1;
			flim = framecount - int(framecount / 20.0 * fix);

			if(odds > 0) {
				if(fseeko(fp,arr.offset+int64_t(blks)*blknel*sizeof(bf[0]),SEEK_SET)||fread(bf,odds * sizeof(bf[0]),1,fp)!=1) {
					strcpy(msg,"reading Next Locator Array ODDS");
				} else {
					for(el=odds;msg[0]=='\0' && (el-=FINGERELS)>=0;) {
						--frame;
						if(fseeko(fp3,arr3.offset+frame*int64_t(FINGERELS*256*sizeof(arrt[0])),SEEK_SET)||
								fwrite(arrt,FINGERELS*256*sizeof(arrt[0]),1,fp3)!= 1) {						
							strcpy(msg,"writing Next Locator Array ODDS");
						} else {
							for(i=FINGERELS;--i>=0;) {
								arrt[(i*256) + (int)(bf[el+i])]= frame;
							}
							if(debug && frame<=flim) {
								dprintf("%sNext Locator Array[%d] %.1f%%",myName,frame,(framecount-frame)*100.0/framecount);
								++fix;
								flim = framecount - int(framecount / 20.0 * fix);
							}
						}
					}
				}
			}

			for(blk=blks;msg[0]=='\0' && --blk>=0;) {
				if(fseeko(fp,arr.offset+int64_t(blk)*blknel*sizeof(bf[0]),SEEK_SET)||fread(bf,blknel*sizeof(bf[0]),1,fp)!=1) {
					strcpy(msg,"reading Next Locator Array");
				} else {
					for(el=blknel;msg[0]=='\0' && (el-=FINGERELS)>=0;) {
						--frame;
						if(fseeko(fp3,arr3.offset+frame*int64_t(FINGERELS*256*sizeof(arrt[0])),SEEK_SET)||fwrite(arrt,FINGERELS*256*sizeof(arrt[0]),1,fp3) != 1) {
							strcpy(msg,"writing Next Locator Array");
						} else {
							for(i=FINGERELS;--i>=0;) {
								arrt[(i*256) + (int)(bf[el+i])] = frame;
							}
							if(debug && frame<=flim) {
								dprintf("%sNext Locator Array[%d] %.1f%%",myName,frame,(framecount-frame)*100.0/framecount);
								++fix;
								flim = framecount - int(framecount / 20.0 * fix);
							}
						}
					}
				}
			}
			if(msg[0]=='\0') {
				rewind(fp3);
				if(fwrite(&arr3,sizeof(arr3),1,fp3) != 1)
					strcpy(msg,"Updating Next Locator Array header");
			}
		} // End Next
	}

	if(bf  != NULL)	delete [] bf;
	if(arrt != NULL)delete [] arrt;
	if(hist!= NULL)	delete [] hist;
	if(fp  != NULL)	fclose(fp);
	if(fp2 != NULL) fclose(fp2);
	if(fp3 != NULL) fclose(fp3);
	if(msg[0])
		env->ThrowError("%sError, %s",myName,msg);
	double end_time =  RT_TimerHP_Lo();
	if(debug)
		dprintf("%sTotal time = %.2f seconds (%.2f mins)\n",myName,end_time-start_time,(end_time-start_time)/60.0);    
	return 0;
}


AVSValue __cdecl RT_QwikScan(AVSValue args, void* user_data, IScriptEnvironment* env) {
	
	const char *myName	= "RT_QwikScan: ";

	enum {
		aSEARCHCLIP,					// ARR Clip, SearchStart clip
		aSEARCHSTART,
		aFINDCLIP,
		aFINDFRAME,
		aARR,
		aPNARR,
		aLUMATOL,
		aFLAGS,
		aLC,
		aLD,
		aFD,
		aPD,
		aPC,
		aXP,
		aFDCHROMAWEIGHT,
		aPDTHRESH,
		aMAXDISTANCE,
		aINCLUSIVE,
		aPREFIX,
		aFM,
		aBlkW,
		aBlkH,
		aOlapX,
		aOlapY,
		aFpFailMax
	};


	PClip			SearchClip	= args[aSEARCHCLIP].AsClip();
	const VideoInfo &vi		    = SearchClip->GetVideoInfo();
	const int framecount	    = vi.num_frames;
	PClip			FindClip	= args[aFINDCLIP].AsClip();
	const VideoInfo &vi2	    = FindClip->GetVideoInfo();
	const char *	ARRfn		= args[aARR].AsString();
	const char *	PNARRfn		= args[aPNARR].AsString();

	if(!vi.IsSameColorspace(vi2))	
		env->ThrowError("%sError, Clips Dissimilar ColorSpace",myName);
	if(vi.width==0 || framecount==0 || vi2.width==0 || vi2.num_frames==0)
		env->ThrowError("%sError, Both clips must have video");
	if(vi.width != vi2.width || vi.height != vi2.height)
		env->ThrowError("%sError, Clips Dissimilar Dimensions");
	if(*ARRfn == '\0' || *PNARRfn == '\0')
		env->ThrowError("%sError, Need both ARR and Prev/Next ARR filenames");

	char msg[1024]="";

	int  clipchannels	= 3;
	int  clipCS=QWIKSCAN_YUV;
	if(vi.IsPlanar()) {
		if(SearchClip->GetFrame(0,env)->GetRowSize(PLANAR_U) == 0) {
			clipchannels = 1;
			clipCS=QWIKSCAN_Y8;
		}
	} else if(vi.IsRGB()) {
		clipCS=QWIKSCAN_RGB;
	}

	int i;
	MYARR arr;
	FILE * fp=ARR_Read_Header(myName,ARRfn,"rb",arr,env);
	MYARR pnarr;
	FILE * pnfp=ARR_Read_Header(myName,PNARRfn,"rb",pnarr,env,msg);				// Msg buf, Optional return
	if(pnfp==NULL) {
		fclose(fp);
		env->ThrowError("%sError, %s",myName,msg);
	}

	const int Magic			= arr.id[0].i;
	const int Version		= arr.id[1].i;
	const int ColorSpace	= arr.id[2].i;
	const int Channels		= arr.id[3].i;
	const int Width			= arr.id[4].i;
	const int Height		= arr.id[5].i;
	const int Matrix		= arr.id[6].i;
	const int Frms			= arr.id[7].i;
	const int xx			= arr.id[8].i;
	const int yy			= arr.id[9].i;
	const int ww			= arr.id[10].i;
	const int hh			= arr.id[11].i;
	const int xx2			= xx;
	const int yy2			= yy;

	const int ArrayFlag	    = arr.id[12].i;
	const int PrvNxtFlag	= pnarr.id[12].i;
	const int pixcnt		= ww*hh;

	char *idnam[]={"Magic","Version","ColorSpace","Channels","Width","Height","Matrix","Framecount","X","Y","W","H","Flag"};

	for(i=0;i<=12;++i) {
		int idoff   = i / 32;
		int idbitix = i & 0x1F;
		if(arr.idtype[idoff] & (1<<idbitix)) {
			sprintf(msg,"QWIKSCAN ID[%d] '%s' type is Float should be Int",i,idnam[i]);
			break;
		} else if(pnarr.idtype[idoff]  & (1<<idbitix)) {
			sprintf(msg,"PNARR ID[%d] '%s'type is Float should be Int",i,idnam[i]);
			break;
		}
	}
	if(msg[0]=='\0') {
		if(ArrayFlag!=0)						strcpy(msg, "QWIKSCAN ID[12], Should be 0(Incorrect ARR)");
		else if(PrvNxtFlag!=1&&PrvNxtFlag!=-1)	strcpy(msg, "PNARR, PrevNextFlag Attribute[12] should be -1(Prev) or 1(Next)");
		else {
			for(i=0;i<12;++i) {
				if(arr.id[i].i != pnarr.id[i].i) {
					sprintf(msg,"QWIKSCAN ID[%d] '%s' Array<->PnArr Mismatch",i,idnam[i]);
					break;
				}
			}
		}
		if(msg[0]=='\0') {
			// Check valid ARR attributes and if mismatch
			if(Magic != QWIKSCAN_MAGIC) {
				strcpy(msg, "QWIKSCAN ID[0], ID != QWIKSCAN_MAGIC");
			}else if(Version != QWIKSCANVER) {
				sprintf(msg,"QWIKSCAN ID[1], VERSION Incorrect");
			}else if(ColorSpace != clipCS) {
				strcpy(msg, "QWIKSCAN ID[2], ColorSpace Mismatch with SearchClip");
			}else if(Channels != clipchannels ) {
				strcpy(msg, "QWIKSCAN ID[3], Channels Mismatch with SearchClip");
			}else if(Width != vi.width) {
				strcpy(msg, "QWIKSCAN ID[4], Width mismatch with SearchClip");
			}else if(Height != vi.height) {
				strcpy(msg, "QWIKSCAN ID[5], Height mismatch with SearchClip");
			}else if(Matrix < 0 || Matrix > 3) {
				strcpy(msg, "QWIKSCAN ID[6], Matrix Not range 0->3");
			}else if(Frms != framecount) {
				strcpy(msg, "QWIKSCAN ID[7], FrameCount mismatch with SearchClip");
			}else if(xx<0) {
				strcpy(msg, "QWIKSCAN ID[8], X coord cannot be -ve");
			}else if(yy<0) {
				strcpy(msg, "QWIKSCAN ID[9], Y coord cannot be -ve");
			}else if(ww<0) {
				strcpy(msg, "QWIKSCAN ID[10], W cannot be -ve");
			}else if(xx+ww > vi.width) {
				strcpy(msg, "QWIKSCAN ID[10], X+W cannot be wider than Width");
			}else if(hh<0) {
				strcpy(msg, "QWIKSCAN ID[11], H cannot be -ve");
			}else if(yy+hh > vi.height) {
				strcpy(msg, "QWIKSCAN ID[11], Y+H cannot be taller than Height");
			}
		}
	}

	if(msg[0]) {
		fclose(fp);
		fclose(pnfp);
		env->ThrowError("%sError, %s",myName,msg);
	}

	const int		SearchStart	= args[aSEARCHSTART].AsInt();
	const int		FindFrame	= args[aFINDFRAME].AsInt();
	const double	LumaTol		 = args[aLUMATOL].AsFloat(1.0);
	int				Flags		 = args[aFLAGS].AsInt(1);			// default LC
	const double	LC			 = args[aLC].AsFloat(1.0);
	const double	LD			 = args[aLD].AsFloat(1.0);
	const double	FD			 = args[aFD].AsFloat(1.0);
	const double	PD			 = args[aPD].AsFloat(1.0);
	const int		PC			 = args[aPC].AsInt((pixcnt+127)/255);
	const int		XP			 = args[aXP].AsInt(1);
	double			ChromaWeight = args[aFDCHROMAWEIGHT].AsFloat(0.3333333f);
	const int		PDThresh	 = args[aPDTHRESH].AsInt(0);
	int	MaxDistance=args[aMAXDISTANCE].AsInt(framecount - 1);
	const bool		Inclusive	 = args[aINCLUSIVE].AsBool(true);
	const char *	Prefix		 = args[aPREFIX].AsString("QWKS_");
	const double	FM			 = args[aFM].AsFloat(1.0);
	int				BlkW		 = args[aBlkW].AsInt(64);
	int				BlkH		 = args[aBlkH].AsInt(BlkW);
	int				oLapX		 = args[aOlapX].AsInt(BlkW/2);
	int				oLapY		 = args[aOlapY].AsInt(BlkH/2);
	const int		FpFailMax	 = args[aFpFailMax].AsInt(0);

	// basic args and clips check

	if(SearchStart < 0 || SearchStart >= framecount) { 
		sprintf(msg,"SearchClip SearchStart=%d does not exist",SearchStart);
	}else if(FindFrame < 0 || FindFrame >= vi2.num_frames) {
		sprintf(msg,"FindClip FindFrame=%d does not exist",FindFrame);
	}else if(LumaTol < 0.0 || LumaTol > LUMATOL_MAX) {
		sprintf(msg,"LumaTol range 0.0 -> %.1f",LUMATOL_MAX);
	}else if(Flags & (~0x3F)) {
		strcpy(msg,"Illegal Flags");
	}else if(LC<0.0 || LC > 255.0) {
		strcpy(msg,"LC range 0.0 -> 255.0");
	}else if(LD<0.0 || LD > 255.0) {
		strcpy(msg,"LD range 0.0 -> 255.0");
	}else if(FD<0.0 || FD > 255.0) {
		strcpy(msg,"FD range 0.0 -> 255.0");
	}else if(PD<0.0 || PD > 255.0) {
		strcpy(msg,"PD range 0.0 -> 255.0");
	}else if(PC<0) {
		strcpy(msg,"PC should be greater or equal to zero");
	}else if(PC>pixcnt) {
		strcpy(msg,"PC is greater than width*height");
	}else if(XP < 0 || XP > 3) {
		strcpy(msg,"XP range 0 -> 3");
	}else if(ChromaWeight< 0.0 || ChromaWeight > 1.0) {
		strcpy(msg,"ChromaWeight 0.0 -> 1.0");
	} else if(PDThresh<0 || PDThresh>255) {
		strcpy(msg,"PDThresh 0 -> 255");
	} else if(FpFailMax<0 || FpFailMax>FPFAILMAX) {
		sprintf(msg,"FpFailMax 0 -> %d",FPFAILMAX);
	} else if(Flags & 0x20) {
		if(BlkW>ww)			BlkW=ww;
		if(BlkH>hh)			BlkH=hh;
		if(oLapX>BlkW/2)	oLapX=BlkH/2;
		if(oLapY>BlkH/2)	oLapY=BlkH/2;
		if(BlkW&0x01)				strcpy(msg,"BlkW must be even");
		else if(BlkH&0x01)			strcpy(msg,"BlkH must be even");
		else if(BlkW<8)				strcpy(msg,"BlkW minimum 8");
		else if(BlkH<8)				strcpy(msg,"BlkH minimum 8");
		else if(oLapX<0)			strcpy(msg,"oLapX minimum 0");
		else if(oLapY<0)			strcpy(msg,"oLapY minimum 0");
		else if(FM<0.0 || FM>255.0)	strcpy(msg,"0.0 <= FM <= 255.0");
	}

	if(msg[0]) {
		fclose(fp);
		fclose(pnfp);
		env->ThrowError("%sError, %s",myName,msg);
	}

	const int records	= arr.dim[1];
	int *ibf			= NULL;
	unsigned int *hist	= NULL;

	if(records != framecount ) {
		sprintf(msg,"ARR Records and SearchClip.FrameCount do not match (%d,%d)",records,framecount);
	}else if(records != pnarr.dim[1] ) {
		sprintf(msg,"ARR/PNARR Records (FrameCounts) do not match (%d,%d)",records,pnarr.dim[1]);
	}else if(SearchStart >= records) {
		sprintf(msg,"SearchClip SearchStart record %d does not exist",SearchStart);
	}else if(arr.dim[2] != FINGERELS) {
		sprintf(msg,"Bad ARR, should have %d fields(%d)",FINGERELS,arr.dim[2]);
	}else if(arr.dim[0] != 2) {
		strcpy(msg,"Bad ARR, should have 2 dimensions");
	} else if(pnarr.dim[2] != (FINGERELS*256)) {
		sprintf(msg,"Bad PNARR, should have %d fields(%d)",FINGERELS*256,pnarr.dim[2]);
	}else if((ibf = new int[FINGERELS*256]) == NULL) {
		strcpy(msg,"allocating ibf buffer");
	}else if((hist = new unsigned int[256]) == NULL) {
		strcpy(msg,"allocating hist buffer");
	}

	if(msg[0]) {
		if(ibf)		delete [] ibf;
		if(hist)	delete [] hist;
		fclose(fp);
		fclose(pnfp);
		env->ThrowError("%sError, %s",myName,msg);
	}

	double arrbf[FINGERELS];
	double tmn[FINGERELS];
	double tmx[FINGERELS];
	int	   mni[FINGERELS];
	int	   mxi[FINGERELS];

    MaxDistance	= std::min(std::max(0,MaxDistance),abs((PrvNxtFlag==1?records-1:0)-SearchStart));	// Silent limit

	AVSValue std[STD_SIZE]	=	{FindClip,FindFrame,0,xx,yy,ww,hh,false};
	AVSValue xtra[XTRA_SIZE];
	xtra[XTRA_RGBIX]		=	Matrix;
	FINGER Finger;

	// Get INEXACT fingerprint of FindFrame (with tolerance range)
	RT_Fingerprint_Lo(AVSValue(std,STD_SIZE),AVSValue(xtra,XTRA_SIZE),Finger,hist,env,LumaTol);
	for(i=FINGERELS;--i>=0;) {
		mni[i] = int(tmn[i] = Finger.lolim[i]);
		mxi[i] = int(tmx[i] = Finger.hilim[i]);
	}

	// lumadif,lumacorr use 14
	// Pixdif uses 15
	// "cc[n]i[delta]i[x]i[y]i[w]i[h]i[n2]i[delta2]i[x2]i[y2]i[Interlaced]b[matrix]i[Thresh]i"
	AVSValue exitargs[15]={SearchClip,FindClip,SearchStart,0,xx,yy,ww,hh,FindFrame,0,xx2,yy2,false,Matrix,PDThresh};



	// framedif uses 14
	// RT_FrameDifference,	"cc[n]i[n2]i[ChromaWeight]f[x]i[y]i[w]i[h]i[x2]i[y2]i[altscan]b[ChromaI]b[Matrix]i"
	AVSValue fdexitargs[14]={SearchClip,FindClip,SearchStart,FindFrame,ChromaWeight,xx,yy,ww,hh,xx2,yy2,false,false,Matrix};

	//RT_FrameMovement,  "cc[n]i[n2]i[ChromaWeight]f[x]i[y]i[w]i[h]i[x2]i[y2]i[altscan]b[ChromaI]b[Matrix]i[blkw]i[blkh]i[OlapX]i[OLapY]i[BlkTh]f[Prefix]s"
	AVSValue fmexitargs[20]={SearchClip,FindClip,SearchStart,FindFrame,ChromaWeight,xx,yy,ww,hh,xx2,yy2,false,false,Matrix,BlkW,BlkH,oLapX,oLapY,""};

	int coords[2]={0,0};
	double PercAboveTh[1]={0.0};
	double BlkAveDf[1]={0.0};
	int nAboveTh[1]={0};
	int TotalBlks[1]={0};

	int		exit_Flags	= 0;

	int		bm_Flags = 0;
	double	bm_LC=256.0;
	double	bm_LD=256.0;
	double	bm_FD=256.0;
	double	bm_PD=256.0;
	int		bm_PC=0x7FFFFFFF;
	double	bm_FM=256.0;

	int		bm_LC_Frm=-1;
	int		bm_LD_Frm=-1;
	int		bm_FD_Frm=-1;
	int		bm_PD_Frm=-1;
	int		bm_PC_Frm=-1;
	int		bm_FM_Frm=-1;

	int		exit_LC_XP=0;
	int		exit_LD_XP=0;
	int		exit_FD_XP=0;
	int		exit_PD_XP=0;
	int		exit_PC_XP=0;
	int		exit_FM_XP=0;

	int	Result	=	-1;												// Not Yet Found

	if(Inclusive) {													//  User wants to also compare with SearchStart.
		if(fseeko(fp,arr.offset + SearchStart*int64_t(FINGERELS*sizeof(arrbf[0])),SEEK_SET) || fread(arrbf,FINGERELS*sizeof(arrbf[0]),1,fp)!=1) {
			delete [] ibf;
			delete [] hist;
			fclose(fp);
			fclose(pnfp);
			env->ThrowError("%sError#1, Reading ARR file(%s)",myName,ARRfn);
		}
		int BinsFailed=0;
		for(i=FINGERELS;--i>=0;) {
			if(arrbf[i]<tmn[i] || arrbf[i]>tmx[i])
				 ++BinsFailed;
		}
		if(BinsFailed<=FpFailMax) {
			dprintf("%sInclusive, BinsFailed(%d) <= FpFailMax(%d)",myName,BinsFailed,FpFailMax);
			if(Flags & 0x3F) {
				if(Flags&(1<<0)) {
					// "cc[n]i[delta]i[x]i[y]i[w]i[h]i[n2]i[delta2]i[x2]i[y2]i[Interlaced]b[matrix]i"
					exitargs[2]=SearchStart;
					double lc = std::min((1.0 - RT_LumaCorrelation_Lo(AVSValue(exitargs,14),env)) * 255.0,255.0);
					if(lc < bm_LC) {			// NO POINT IN BEST MATCH EQUAL @ STARTFRAME
						bm_LC = lc;
						bm_LC_Frm=SearchStart;
						bm_Flags |= (1<<0);
					}
					if(lc <= LC) {
						Result = SearchStart;
						dprintf("%sSearchStart, LumaCorrelation Condition Succeeds @%d lc=%f",myName,Result,lc);
						if(XP) {
							int fnxt = Result;
							while(abs(SearchStart-(fnxt+PrvNxtFlag)) <= MaxDistance) {
								exitargs[2]=fnxt+PrvNxtFlag;
								double lctmp = std::max(std::min((1.0-RT_LumaCorrelation_Lo(AVSValue(exitargs,14),env))*255.0,255.0),0.0);
								if(lctmp > lc || (XP==3 &&  lctmp == lc))
									break;						// not as good OR not Strictly better than
								if(XP == 1 && lctmp == lc) {	// looking for 'better than', check if better AFTER equal
									double lctmp_2 = lc;
									int    fnxt_2  = fnxt;
									while(abs(SearchStart-(fnxt_2+PrvNxtFlag)) <= MaxDistance) {
										exitargs[2]=fnxt_2+PrvNxtFlag;
										double lctmp_2 = std::max(std::min((1.0-RT_LumaCorrelation_Lo(AVSValue(exitargs,14),env))*255.0,255.0),0.0);
										if(lctmp_2 != lc)
											break;						// not equal, break												
										fnxt_2 += PrvNxtFlag;
									}
									// if (lctmp_2 == lc) then Maxdistance reached
									// if (lctmp_2 > lc) then worse match
									if(lctmp_2 >= lc)
										break;						// not better than, break
									lc  = lctmp_2;					// Update bestsofar
									fnxt= fnxt_2 + PrvNxtFlag;		// Update best so far frame
									continue;
								}
								lc=lctmp;
								fnxt += PrvNxtFlag;
							}
							if(fnxt != Result) {
								dprintf("%sSearchStart, LumaCorrelation Condition Extended to %d lc=%f",myName,fnxt,lc);
								exit_LC_XP = abs(Result - fnxt); 
								Result = fnxt;
							}
						}
						bm_LC=lc;
						bm_LC_Frm=Result;
						exit_Flags |= (1<<0);
					}
				}
				if(Flags&(1<<1)) {
					// "cc[n]i[delta]i[x]i[y]i[w]i[h]i[n2]i[delta2]i[x2]i[y2]i[Interlaced]b[matrix]i"
					exitargs[2]=SearchStart;
					double ld = RT_LumaDifference_Lo(AVSValue(exitargs,14),myName,env);
					if(ld < bm_LD) {			// NO POINT IN BEST MATCH EQUAL @ STARTFRAME
						bm_LD = ld;
						bm_LD_Frm=SearchStart;
						bm_Flags |= (1<<1);
					}
					if(ld <= LD) {
						Result = SearchStart;
						dprintf("%sSearchStart, LumaDifference Condition Succeeds @%d ld=%f",myName,Result,ld);
						if(XP>0) {
							int fnxt = Result;
							while(abs(SearchStart-(fnxt+PrvNxtFlag)) <= MaxDistance) {
								exitargs[2]=fnxt+PrvNxtFlag;
								const double ldtmp=RT_LumaDifference_Lo(AVSValue(exitargs,14),myName,env);
								if(ldtmp > ld || (XP==3 &&  ldtmp == ld))
									break;						// not as good OR not Strictly better than
								if(XP == 1 && ldtmp == ld) {	// looking for 'better than', check if better AFTER equal
									double ldtmp_2 = ld;
									int    fnxt_2  = fnxt;
									while(abs(SearchStart-(fnxt_2+PrvNxtFlag)) <= MaxDistance) {
										exitargs[2]=fnxt_2+PrvNxtFlag;
										ldtmp_2=RT_LumaDifference_Lo(AVSValue(exitargs,14),myName,env);
										if(ldtmp_2 != ld)
											break;						// not equal, break												
										fnxt_2 += PrvNxtFlag;
									}
									// if (ldtmp_2 == ld) then Maxdistance reached
									// if (ldtmp_2 > ld) then worse match
									if(ldtmp_2 >= ld)
										break;						// not better than, break
									ld  = ldtmp_2;					// Update bestsofar
									fnxt= fnxt_2 + PrvNxtFlag;		// Update best so far frame
									continue;
								}
								ld=ldtmp;
								fnxt += PrvNxtFlag;
							}							
							if(fnxt != Result) {
								dprintf("%sSearchStart, LumaDifference Condition Extended to %d ld=%f",myName,fnxt,ld);
								exit_LD_XP = abs(Result - fnxt); 
								Result = fnxt;
							}
						}
						bm_LD=ld;
						bm_LD_Frm=Result;
						exit_Flags |= (1<<1);
					}
				}
				if(Flags&(1<<2)) {
					// RT_FrameDifference,	"cc[n]i[n2]i[ChromaWeight]f[x]i[y]i[w]i[h]i[x2]i[y2]i[altscan]b[ChromaI]b[Matrix]i"
					exitargs[2]=SearchStart;
					double fd=RT_FrameDifference_Lo(AVSValue(fdexitargs,14),NULL,env);
					if(fd < bm_FD) {			// NO POINT IN BEST MATCH EQUAL @ STARTFRAME
						bm_FD = fd;
						bm_FD_Frm=SearchStart;
						bm_Flags |= (1<<2);
					}
					if(fd <= FD) {
						Result = SearchStart;
						dprintf("%sSearchStart, FrameDifference Condition Succeeds @%d fd=%f",myName,Result,fd);
						if(XP) {
							int fnxt = Result;
							while(abs(SearchStart-(fnxt+PrvNxtFlag)) <= MaxDistance) {
								fdexitargs[2]=fnxt+PrvNxtFlag;
								const double fdtmp=RT_FrameDifference_Lo(AVSValue(fdexitargs,14),NULL,env);
								if(fdtmp > fd || (XP==3 &&  fdtmp == fd))
									break;						// not as good OR not Strictly better than
								if(XP == 1 && fdtmp == fd) {	// looking for 'better than', check if better AFTER equal
									double fdtmp_2 = fd;
									int    fnxt_2  = fnxt;
									while(abs(SearchStart-(fnxt_2+PrvNxtFlag)) <= MaxDistance) {
										fdexitargs[2]=fnxt_2+PrvNxtFlag;
										fdtmp_2 = RT_FrameDifference_Lo(AVSValue(fdexitargs,14),NULL,env);
										if(fdtmp_2 != fd)
											break;						// not equal, break												
										fnxt_2 += PrvNxtFlag;
									}
									// if (fdtmp_2 == fd) then Maxdistance reached
									// if (fdtmp_2 > fd) then worse match
									if(fdtmp_2 >= fd)
										break;						// not better than, break
									fd  = fdtmp_2;					// Update bestsofar
									fnxt= fnxt_2 + PrvNxtFlag;		// Update best so far frame
									continue;
								}
								fd   =  fdtmp;
								fnxt += PrvNxtFlag;
							}
							if(fnxt != Result) {
								dprintf("%sSearchStart, FrameDifference Condition Extended to %d fd=%f",myName,fnxt,fd);
								exit_FD_XP = abs(Result - fnxt); 
								Result = fnxt;
							}
						}
						bm_FD=fd;
						bm_FD_Frm=Result;
						exit_Flags |= (1<<2);
					}
				}
                // !!! WARNING !!! COMBINED PixelDiff and/or PixelDiffCount, watch out for indentation
				if(Flags&(3<<3)) {  // PixelDiff and/or PixelDiffCount
					// "cc[n]i[delta]i[x]i[y]i[w]i[h]i[n2]i[delta2]i[x2]i[y2]i[Interlaced]b[matrix]i[Thresh]i"
					exitargs[2]=SearchStart;
					double pd=0.0;
					int pc = RT_LumaPixelsDifferentCount_Lo(AVSValue(exitargs,15),env,&pd);
					if(Flags&(1<<3) && pd < bm_PD) {	// NO POINT IN BEST MATCH EQUAL @ STARTFRAME
						bm_PD = pd;
						bm_PD_Frm=SearchStart;
						bm_Flags |= (1<<3);
					}
					if(Flags&(1<<4) && pc < bm_PC) {	// NO POINT IN BEST MATCH EQUAL @ STARTFRAME
						bm_PC = pc;
						bm_PC_Frm=SearchStart;
						bm_Flags |= (1<<4);
					}
					if(Flags&(1<<3) && pd <= PD) { // PD reported if both requested
						Result = SearchStart;
						dprintf("%sSearchStart, LumaPixelsDifferent Condition Succeeds @%d pd=%f",myName,Result,pd);
						if(XP) {
							int fnxt = Result;
							while(abs(SearchStart-(fnxt+PrvNxtFlag)) <= MaxDistance) {
								exitargs[2]=fnxt+PrvNxtFlag;
								double pdtmp=0.0;
								RT_LumaPixelsDifferentCount_Lo(AVSValue(exitargs,15),env,&pdtmp);
								if(pdtmp > pd || (XP==3 &&  pdtmp == pd))
									break;						// not as good OR not Strictly better than
								if(XP == 1 && pdtmp == pd) {	// looking for 'better than', check if better AFTER equal
									double pdtmp_2 = pd;
									int    fnxt_2  = fnxt;
									while(abs(SearchStart-(fnxt_2+PrvNxtFlag)) <= MaxDistance) {
										exitargs[2]=fnxt+PrvNxtFlag;
										RT_LumaPixelsDifferentCount_Lo(AVSValue(exitargs,15),env,&pdtmp_2);
										if(pdtmp_2 != pd)
											break;						// not equal, break												
										fnxt_2 += PrvNxtFlag;
									}
									// if (pdtmp_2 == pd) then Maxdistance reached
									// if (pdtmp_2 > pd) then worse match
									if(pdtmp_2 >= pd)
										break;						// not better than, break
									pd  = pdtmp_2;					// Update bestsofar
									fnxt= fnxt_2 + PrvNxtFlag;		// Update best so far frame
									continue;
								}
								pd=pdtmp;
								fnxt += PrvNxtFlag;
							}
							if(fnxt != Result) {
								dprintf("%sSearchStart, LumaPixelsDifferent Condition Extended to %d pd=%f",myName,fnxt,pd);
								exit_PC_XP = abs(Result - fnxt); 
								Result = fnxt;
							}
						}
						bm_PD=pd;
						bm_PD_Frm=Result;
						exit_Flags |= (1<<3);
					}
					if (Flags&(1<<4) && pc <= PC) {
						Result = SearchStart;
						dprintf("%sSearchStart, LumaPixelsDifferentCount Condition Succeeds @%d pc=%d",myName,Result,pc);
						if(XP) {
							int fnxt = Result;
							while(abs(SearchStart-(fnxt+PrvNxtFlag)) <= MaxDistance) {
								exitargs[2]=fnxt+PrvNxtFlag;
								const int pctmp=RT_LumaPixelsDifferentCount_Lo(AVSValue(exitargs,15),env);
								if(pctmp > pc || (XP==3 &&  pctmp == pc))
									break;						// not as good OR not Strictly better than
								if(XP == 1 && pctmp == pc) {	// looking for 'better than', check if better AFTER equal
									int	   pctmp_2 = pc;
									int    fnxt_2  = fnxt;
									while(abs(SearchStart-(fnxt_2+PrvNxtFlag)) <= MaxDistance) {
										exitargs[2]=fnxt+PrvNxtFlag;
										pctmp_2 = RT_LumaPixelsDifferentCount_Lo(AVSValue(exitargs,15),env);
										if(pctmp_2 != pc)
											break;						// not equal, break												
										fnxt_2 += PrvNxtFlag;
									}
									// if (pctmp_2 == pc) then Maxdistance reached
									// if (pctmp_2 > pc) then worse match
									if(pctmp_2 >= pc)
										break;						// not better than, break
									pc  = pctmp_2;					// Update bestsofar
									fnxt= fnxt_2 + PrvNxtFlag;		// Update best so far frame
									continue;
								}
								pc=pctmp;
								fnxt += PrvNxtFlag;
							}
							if(fnxt != Result) {
								dprintf("%sSearchStart, LumaPixelsDifferentCount Condition Extended to %d pc=%d",myName,fnxt,pc);
								exit_PC_XP = abs(Result - fnxt); 
								Result = fnxt;
							}
						}
						bm_PC=pc;
						bm_PC_Frm=Result;
						exit_Flags |= (1<<4);
					}
                }
				if(Flags&(1<<5)) {
					// "cc[n]i[n2]i[ChromaWeight]f[x]i[y]i[w]i[h]i[x2]i[y2]i[altscan]b[ChromaI]b[Matrix]i[blkw]i[blkh]i[OlapX]i[OLapY]i[BlkTh]f[Prefix]s"

					fmexitargs[2]=SearchStart;
					double fm = RT_FrameMovement_Lo(AVSValue(fmexitargs,20),NULL,coords,PercAboveTh,BlkAveDf,nAboveTh,TotalBlks,env);
					if(fm < bm_FM) {			// NO POINT IN BEST MATCH EQUAL @ STARTFRAME
						bm_FM = fm;
						bm_FM_Frm=SearchStart;
						bm_Flags |= (1<<5);
					}
					if(fm <= FM) {
						Result = SearchStart;
						dprintf("%sSearchStart, FrameMovement Condition Succeeds @%d fm=%f",myName,Result,fm);
						if(XP>0) {
							int fnxt = Result;
							while(abs(SearchStart-(fnxt+PrvNxtFlag)) <= MaxDistance) {
								fmexitargs[2]=fnxt+PrvNxtFlag;
								const double fmtmp=RT_FrameMovement_Lo(AVSValue(fmexitargs,20),
										NULL,coords,PercAboveTh,BlkAveDf,nAboveTh,TotalBlks,env);
								if(fmtmp > fm || (XP==3 &&  fmtmp == fm))
									break;						// not as good OR not Strictly better than
								if(XP == 1 && fmtmp == fm) {	// looking for 'better than', check if better AFTER equal
									double fmtmp_2 = fm;
									int    fnxt_2  = fnxt;
									while(abs(SearchStart-(fnxt_2+PrvNxtFlag)) <= MaxDistance) {
										fmexitargs[2]=fnxt_2+PrvNxtFlag;
										fmtmp_2=RT_FrameMovement_Lo(AVSValue(fmexitargs,20),
											NULL,coords,PercAboveTh,BlkAveDf,nAboveTh,TotalBlks,env);
										if(fmtmp_2 != fm)
											break;						// not equal, break												
										fnxt_2 += PrvNxtFlag;
									}
									// if (fmtmp_2 == fm) then Maxdistance reached
									// if (fmtmp_2 > fm) then worse match
									if(fmtmp_2 >= fm)
										break;						// not better than, break
									fm  = fmtmp_2;					// Update bestsofar
									fnxt= fnxt_2 + PrvNxtFlag;		// Update best so far frame
									continue;
								}
								fm=fmtmp;
								fnxt += PrvNxtFlag;
							}							
							if(fnxt != Result) {
								dprintf("%sSearchStart, FrameMovement Condition Extended to %d fm=%f",myName,fnxt,fm);
								exit_FM_XP = abs(Result - fnxt); 
								Result = fnxt;
							}
						}
						bm_FM=fm;
						bm_FM_Frm=Result;
						exit_Flags |= (1<<5);
					}
				}			
				if(exit_Flags==0) {
					dprintf("%sInclusive Match @ %d, EXIT conditions FAIL",myName,SearchStart);
				} else {
			   		Result = SearchStart;
					dprintf("%sInclusive Match, EXIT condition SUCCEEDS @ %d",myName,SearchStart);
				}
			} else { // if(Flags & 0x3F)
		   		Result = SearchStart;									// All done, closest match possible
				dprintf("%sInclusive Match Returning %d",myName,SearchStart);
			}
		} else {
			dprintf("%sInclusive, BinsFailed(%d) > FpFailMax(%d)",myName,BinsFailed,FpFailMax);
		} // End of if(BinsFailed<=FpFailMax)
	}
	

    if(Result == -1 && MaxDistance > 0) {

		int Cursor=SearchStart;									// PNARR Cursor frame

		while(abs(SearchStart-Cursor) < MaxDistance) { // Cursor distance MUST be at least 1 less than MaxDistance

			if(fseeko(pnfp,pnarr.offset + Cursor*FINGERELS*256*sizeof(int),SEEK_SET) || 
						fread(ibf,FINGERELS*256*sizeof(int),1,pnfp)!=1) {
				delete [] ibf;
				delete [] hist;
				fclose(fp);
				fclose(pnfp);
				env->ThrowError("%sError#3, Reading PNDB file(%s)",myName,PNARRfn);
			}

			int FarDist	=	-1;
			int FarFrame=	-1;
			int BinsFailed=0;
			for(i=0;i<FINGERELS;++i) {
		    	const int si = mni[i];
				const int sz = mxi[i] - si + 1;
				const int off = i*256;
		    	int NearFrame= -1;								// Not found
		    	int NearDist = 0x7FFFFFFF;						// Max possible
				int j;
		    	for(j=sz; --j>=0;) {    	
					const int f = ibf[off+si+j];				// Get Nearest APPROXIMATE to CURSOR.
					if(f >= 0) {
						const int d = abs(SearchStart-f);
		            	if(d < NearDist) {
		            		NearDist = d;
		            		NearFrame = f;
		            	}
					}
				}
				if(NearFrame >= 0) {					
					if(NearDist > FarDist) {
						FarDist	= NearDist;
						FarFrame= NearFrame;
					}
				} else if (++BinsFailed>FpFailMax) {
					break;											// No frames fulfill condition
				}
			}

			if(BinsFailed>FpFailMax) {
				dprintf("%sBinsFailed(%d) > FpFailMax(%d)",myName,BinsFailed,FpFailMax);
				break;													// at least one required stat failed
			}

			dprintf("%sBinsFailed(%d) <= FpFailMax(%d)",myName,BinsFailed,FpFailMax);

			if(FarDist <= MaxDistance) {

				if(fseeko(fp,arr.offset + FarFrame * int64_t(FINGERELS * sizeof(arrbf[0])),SEEK_SET) ||  fread(arrbf,FINGERELS * sizeof(arrbf[0]),1,fp)!=1) {
					delete [] ibf;
					delete [] hist;
					fclose(fp);
					fclose(pnfp);
					env->ThrowError("%sError#4, Reading ARR file(%s)",myName,ARRfn);
				}

				BinsFailed=0;
				for(i=FINGERELS;--i>=0;) {
					if((arrbf[i] < tmn[i] || arrbf[i] > tmx[i]) && ++BinsFailed>FpFailMax)
						break;
				}

				if(BinsFailed<=FpFailMax) {
					if(Flags & 0x3F) {
						if(Flags&(1<<0)) {
							// "cc[n]i[delta]i[x]i[y]i[w]i[h]i[n2]i[delta2]i[x2]i[y2]i[Interlaced]b[matrix]i"
							exitargs[2]=FarFrame;
							double lc = std::max(std::min((1.0-RT_LumaCorrelation_Lo(AVSValue(exitargs,14),env))*255.0,255.0),0.0);
							if(lc < bm_LC || (XP == 2 && lc == bm_LC)) {
								bm_LC = lc;
								bm_LC_Frm=FarFrame;
								bm_Flags |= (1<<0);
							}
							if(lc <= LC) {
								Result = FarFrame;
								dprintf("%sLumaCorrelation Condition Succeeds @%d lc=%f",myName,Result,lc);
								if(XP) {
									int fnxt = Result;
									while(abs(SearchStart-(fnxt+PrvNxtFlag)) <= MaxDistance) {
										exitargs[2]=fnxt+PrvNxtFlag;
										double lctmp = std::max(std::min((1.0-RT_LumaCorrelation_Lo(AVSValue(exitargs,14),env))*255.0,255.0),0.0);
										if(lctmp > lc || (XP==3 &&  lctmp == lc))
											break;						// not as good OR not Strictly better than
										if(XP == 1 && lctmp == lc) {	// looking for 'better than', check if better AFTER equal
											double lctmp_2 = lc;
											int    fnxt_2  = fnxt;
											while(abs(SearchStart-(fnxt_2+PrvNxtFlag)) <= MaxDistance) {
												exitargs[2]=fnxt_2+PrvNxtFlag;
												double lctmp_2 = std::max(std::min((1.0-RT_LumaCorrelation_Lo(AVSValue(exitargs,14),env))*255.0,255.0),0.0);
												if(lctmp_2 != lc)
													break;						// not equal, break												
												fnxt_2 += PrvNxtFlag;
											}
											// if (lctmp_2 == lc) then Maxdistance reached
											// if (lctmp_2 > lc) then worse match
											if(lctmp_2 >= lc)
												break;						// not better than, break
											lc  = lctmp_2;					// Update bestsofar
											fnxt= fnxt_2 + PrvNxtFlag;		// Update best so far frame
											continue;
										}
										lc=lctmp;
										fnxt += PrvNxtFlag;
									}
									if(fnxt != Result) {
										dprintf("%sLumaCorrelation Condition Extended to %d lc=%f",myName,fnxt,lc);
										exit_LC_XP = abs(Result - fnxt); 
										Result = fnxt;
									}
								}
								bm_LC=lc;
								bm_LC_Frm=Result;
								exit_Flags |= (1<<0);
							}
						}
						if(Flags&(1<<1)) {					
							// "cc[n]i[delta]i[x]i[y]i[w]i[h]i[n2]i[delta2]i[x2]i[y2]i[Interlaced]b[matrix]i"
							exitargs[2]=FarFrame;
							double ld = RT_LumaDifference_Lo(AVSValue(exitargs,14),myName,env);
							if(ld < bm_LD || (XP == 2 && ld == bm_LD)) {
								bm_LD = ld;
								bm_LD_Frm=FarFrame;
								bm_Flags |= (1<<1);
							}
							if(ld <= LD) {
								Result = FarFrame;
								dprintf("%sLumaDifference Condition Succeeds @%d ld=%f",myName,Result,ld);
								if(XP) {
									int fnxt = Result;
									while(abs(SearchStart-(fnxt+PrvNxtFlag)) <= MaxDistance) {
										exitargs[2]=fnxt+PrvNxtFlag;
										const double ldtmp=RT_LumaDifference_Lo(AVSValue(exitargs,14),myName,env);
										if(ldtmp > ld || (XP==3 &&  ldtmp == ld))
											break;						// not as good OR not Strictly better than
										if(XP == 1 && ldtmp == ld) {	// looking for 'better than', check if better AFTER equal
											double ldtmp_2 = ld;
											int    fnxt_2  = fnxt;
											while(abs(SearchStart-(fnxt_2+PrvNxtFlag)) <= MaxDistance) {
												exitargs[2]=fnxt_2+PrvNxtFlag;
												ldtmp_2=RT_LumaDifference_Lo(AVSValue(exitargs,14),myName,env);
												if(ldtmp_2 != ld)
													break;						// not equal, break												
												fnxt_2 += PrvNxtFlag;
											}
											// if (ldtmp_2 == ld) then Maxdistance reached
											// if (ldtmp_2 > ld) then worse match
											if(ldtmp_2 >= ld)
												break;						// not better than, break
											ld  = ldtmp_2;					// Update bestsofar
											fnxt= fnxt_2 + PrvNxtFlag;		// Update best so far frame
											continue;
										}
										ld=ldtmp;
										fnxt += PrvNxtFlag;
									}
									if(fnxt != Result) {
										dprintf("%sLumaDifference Condition Extended to %d ld=%f",myName,fnxt,ld);
										exit_LD_XP = abs(Result - fnxt); 
										Result = fnxt;
									}
								}
								bm_LD=ld;
								bm_LD_Frm=Result;
								exit_Flags |= (1<<1);
							}
						}
						if(Flags&(1<<2)) {
							// "cc[n]i[n2]i[ChromaWeight]f"
							fdexitargs[2]=FarFrame;
							double fd=RT_FrameDifference_Lo(AVSValue(fdexitargs,14),NULL,env);
							if(fd < bm_FD || (XP == 2 && fd == bm_FD)) {
								bm_FD = fd;
								bm_FD_Frm=FarFrame;
								bm_Flags |= (1<<2);
							}
							if(fd <= FD) {
								Result = FarFrame;
								dprintf("%sFrameDifference Condition Succeeds @%d fd=%f",myName,Result,fd);
								if(XP) {
									int fnxt = Result;
									while(abs(SearchStart-(fnxt+PrvNxtFlag)) <= MaxDistance) {
										fdexitargs[2]=fnxt+PrvNxtFlag;
										const double fdtmp=RT_FrameDifference_Lo(AVSValue(fdexitargs,14),NULL,env);
										if(fdtmp > fd || (XP==3 &&  fdtmp == fd))
											break;						// not as good OR not Strictly better than
										if(XP == 1 && fdtmp == fd) {	// looking for 'better than', check if better AFTER equal
											double fdtmp_2 = fd;
											int    fnxt_2  = fnxt;
											while(abs(SearchStart-(fnxt_2+PrvNxtFlag)) <= MaxDistance) {
												fdexitargs[2]=fnxt_2+PrvNxtFlag;
												fdtmp_2 = RT_FrameDifference_Lo(AVSValue(fdexitargs,14),NULL,env);
												if(fdtmp_2 != fd)
													break;						// not equal, break												
												fnxt_2 += PrvNxtFlag;
											}
											// if (fdtmp_2 == fd) then Maxdistance reached
											// if (fdtmp_2 > fd) then worse match
											if(fdtmp_2 >= fd)
												break;						// not better than, break
											fd  = fdtmp_2;					// Update bestsofar
											fnxt= fnxt_2 + PrvNxtFlag;		// Update best so far frame
											continue;
										}
										fd   =  fdtmp;
										fnxt += PrvNxtFlag;
									}
									if(fnxt != Result) {
										dprintf("%sFrameDifference Condition Extended to %d fd=%f",myName,fnxt,fd);
										exit_FD_XP = abs(Result - fnxt); 
										Result = fnxt;
									}
								}
								bm_FD=fd;
								bm_FD_Frm=Result;
								exit_Flags |= (1<<2);
							}
						}
                        // !!! WARNING !!! COMBINED PixelDiff and/or PixelDiffCount, watch out for indentation
						if(Flags&(3<<3)) {  // PixelDiff and/or PixelDiffCount
							// "cc[n]i[delta]i[x]i[y]i[w]i[h]i[n2]i[delta2]i[x2]i[y2]i[Interlaced]b[matrix]i[Thresh]i"
							exitargs[2]=FarFrame;
							double pd=0.0;
							int pc = RT_LumaPixelsDifferentCount_Lo(AVSValue(exitargs,15),env,&pd);
							if(Flags&(1<<3) && (pd < bm_PD || (XP == 2 && pd == bm_PD))) {
								bm_PD = pd;
								bm_PD_Frm=FarFrame;
								bm_Flags |= (1<<3);
							}
							if(Flags&(1<<4) && (pc < bm_PC || (XP == 2 && pc == bm_PC))) {
								bm_PC = pc;
								bm_PC_Frm=FarFrame;
								bm_Flags |= (1<<4);
							}
							if(Flags&(1<<3) && pd <= PD) {
								Result = FarFrame;
								dprintf("%sLumaPixelsDifferent Condition Succeeds @%d pd=%f",myName,Result,pd);
								if(XP) {
									int fnxt = Result;
									while(abs(SearchStart-(fnxt+PrvNxtFlag)) <= MaxDistance) {
										exitargs[2]=fnxt+PrvNxtFlag;
										double pdtmp=0.0;
										RT_LumaPixelsDifferentCount_Lo(AVSValue(exitargs,15),env,&pdtmp);
										if(pdtmp > pd || (XP==3 &&  pdtmp == pd))
											break;						// not as good OR not Strictly better than
										if(XP == 1 && pdtmp == pd) {	// looking for 'better than', check if better AFTER equal
											double pdtmp_2 = pd;
											int    fnxt_2  = fnxt;
											while(abs(SearchStart-(fnxt_2+PrvNxtFlag)) <= MaxDistance) {
												exitargs[2]=fnxt+PrvNxtFlag;
												RT_LumaPixelsDifferentCount_Lo(AVSValue(exitargs,15),env,&pdtmp_2);
												if(pdtmp_2 != pd)
													break;						// not equal, break												
												fnxt_2 += PrvNxtFlag;
											}
											// if (pdtmp_2 == pd) then Maxdistance reached
											// if (pdtmp_2 > pd) then worse match
											if(pdtmp_2 >= pd)
												break;						// not better than, break
											pd  = pdtmp_2;					// Update bestsofar
											fnxt= fnxt_2 + PrvNxtFlag;		// Update best so far frame
											continue;
										}
										pd=pdtmp;
										fnxt += PrvNxtFlag;
									}
									if(fnxt != Result) {
										dprintf("%sLumaPixelsDifferent Condition Extended to %d pd=%f",myName,fnxt,pd);
										exit_PD_XP = abs(Result - fnxt); 
										Result = fnxt;
									}
								}
								bm_PD=pd;
								bm_PD_Frm=Result;
								exit_Flags |= (1<<3);
							} else if (Flags&(1<<4) && pc <= PC) {
								Result = FarFrame;
								dprintf("%sLumaPixelsDifferentCount Condition Succeeds @%d pc=%d",myName,Result,pc);
								if(XP) {
									int fnxt = Result;
									while(abs(SearchStart-(fnxt+PrvNxtFlag)) <= MaxDistance) {
										exitargs[2]=fnxt+PrvNxtFlag;
										const int pctmp=RT_LumaPixelsDifferentCount_Lo(AVSValue(exitargs,15),env);
										if(pctmp > pc || (XP==3 &&  pctmp == pc))
											break;						// not as good OR not Strictly better than
										if(XP == 1 && pctmp == pc) {	// looking for 'better than', check if better AFTER equal
											int	   pctmp_2 = pc;
											int    fnxt_2  = fnxt;
											while(abs(SearchStart-(fnxt_2+PrvNxtFlag)) <= MaxDistance) {
												exitargs[2]=fnxt+PrvNxtFlag;
												pctmp_2 = RT_LumaPixelsDifferentCount_Lo(AVSValue(exitargs,15),env);
												if(pctmp_2 != pc)
													break;						// not equal, break												
												fnxt_2 += PrvNxtFlag;
											}
											// if (pctmp_2 == pc) then Maxdistance reached
											// if (pctmp_2 > pc) then worse match
											if(pctmp_2 >= pc)
												break;						// not better than, break
											pc  = pctmp_2;					// Update bestsofar
											fnxt= fnxt_2 + PrvNxtFlag;		// Update best so far frame
											continue;
										}
										pc=pctmp;
										fnxt += PrvNxtFlag;
									}
									if(fnxt != Result) {
										dprintf("%sLumaPixelsDifferentCount Condition Extended to %d pc=%d",myName,fnxt,pc);
										exit_PC_XP = abs(Result - fnxt); 
										Result = fnxt;
									}
								}
								bm_PC=pc;
								bm_PC_Frm=Result;
								exit_Flags |= (1<<4);
							}
						}

						if(Flags&(1<<5)) {					
							fmexitargs[2]=FarFrame;
							double fm = RT_FrameMovement_Lo(AVSValue(fmexitargs,20),
								NULL,coords,PercAboveTh,BlkAveDf,nAboveTh,TotalBlks,env);
							if(fm < bm_FM || (XP == 2 && fm == bm_FM)) {
								bm_FM = fm;
								bm_FM_Frm=FarFrame;
								bm_Flags |= (1<<5);
							}
							if(fm <= FM) {
								Result = FarFrame;
								dprintf("%sFrameMovement Condition Succeeds @%d fm=%f",myName,Result,fm);
								if(XP) {
									int fnxt = Result;
									while(abs(SearchStart-(fnxt+PrvNxtFlag)) <= MaxDistance) {
										fmexitargs[2]=fnxt+PrvNxtFlag;
										const double fmtmp=RT_FrameMovement_Lo(AVSValue(fmexitargs,20),
											NULL,coords,PercAboveTh,BlkAveDf,nAboveTh,TotalBlks,env);
										if(fmtmp > fm || (XP==3 &&  fmtmp == fm))
											break;						// not as good OR not Strictly better than
										if(XP == 1 && fmtmp == fm) {	// looking for 'better than', check if better AFTER equal
											double fmtmp_2 = fm;
											int    fnxt_2  = fnxt;
											while(abs(SearchStart-(fnxt_2+PrvNxtFlag)) <= MaxDistance) {
												fmexitargs[2]=fnxt_2+PrvNxtFlag;
												fmtmp_2=RT_FrameMovement_Lo(AVSValue(fmexitargs,20),
													NULL,coords,PercAboveTh,BlkAveDf,nAboveTh,TotalBlks,env);
												if(fmtmp_2 != fm)
													break;						// not equal, break												
												fnxt_2 += PrvNxtFlag;
											}
											// if (fmtmp_2 == fm) then Maxdistance reached
											// if (fmtmp_2 > fm) then worse match
											if(fmtmp_2 >= fm)
												break;						// not better than, break
											fm  = fmtmp_2;					// Update bestsofar
											fnxt= fnxt_2 + PrvNxtFlag;		// Update best so far frame
											continue;
										}
										fm=fmtmp;
										fnxt += PrvNxtFlag;
									}
									if(fnxt != Result) {
										dprintf("%sFrameMovement Condition Extended to %d fm=%f",myName,fnxt,fm);
										exit_FM_XP = abs(Result - fnxt); 
										Result = fnxt;
									}
								}
								bm_FM=fm;
								bm_FM_Frm=Result;
								exit_Flags |= (1<<5);
							}
						}

						if(exit_Flags==0) {
								dprintf("%sMatch @ %d, EXIT conditions FAIL",myName,FarFrame);
								dprintf("%sNew Cursor %d",myName,FarFrame);
							Cursor=FarFrame;
						} else {
			   				Result = FarFrame;
							dprintf("%sFULL Match, EXIT condition SUCCEEDS @ %d",myName,Result);
							break;
						}
					} else { // End of if(Flags & 0x3F)
		   				Result = FarFrame;
						dprintf("%sFarFrame=%d",myName,FarFrame);
						dprintf("%sMatch (Without Exit Condition) Returning %d",myName,Result);
						break;
					}
				} else {
dprintf("%sMatch failed",myName);
dprintf("%sFarFrame=%d",myName,FarFrame);
dprintf("%sNew Cursor %d",myName,FarFrame);
					Cursor = FarFrame;
				}
			} else {
dprintf("%sMaxDistance reached, exit NOT Found",myName);
				break;													// Neither frame nor next cursor valid
			}
        } // End while(abs(SearchStart-Cursor) < MaxDistance)
	} else if(Result < 0) {
dprintf("%sMaxDistance Reached on first frame (Inclusive Failed)",myName);
	}
	delete [] ibf;
	delete [] hist;
	fclose(fp);
	fclose(pnfp);

	size_t pfixlen=strlen(Prefix);
	if(pfixlen>128)
		pfixlen=128;
	char pfixbf[128+32];
	memcpy(pfixbf,Prefix,pfixlen);
	char *d,*dest=pfixbf+pfixlen;
	const char *p="EXIT_Flags";
	for(d=dest;*d++=*p++;);					// strcat variable name part
	AVSValue var;
	var = GetVar(env,pfixbf);
	env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),exit_Flags);
	int cnt=0;
	if(exit_Flags) {
		int i,f=exit_Flags;
		for(i=6;--i>=0;) {
			if(f&0x01)
				++cnt;
			f>>=1;
		}
	}
	p="EXIT_COUNT";
	for(d=dest;*d++=*p++;);					// strcat variable name part
	var = GetVar(env,pfixbf);
	env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),cnt);
	p="BM_Flags";
	for(d=dest;*d++=*p++;);					// strcat variable name part
	var = GetVar(env,pfixbf);
	env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_Flags);
	cnt=0;
	if(bm_Flags) {
		int i,f=bm_Flags;
		for(i=6;--i>=0;) {
			if(f&0x01)
				++cnt;
			f>>=1;
		}
	}
	p="BM_COUNT";
	for(d=dest;*d++=*p++;);					// strcat variable name part
	var = GetVar(env,pfixbf);
	env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),cnt);
	if(bm_Flags) {
		if(bm_Flags & (1<<0)) {
			p="BM_LC";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_LC);
			p="BM_LC_Frm";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_LC_Frm);
			p="BM_LC_XP";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),exit_LC_XP);
		}
		if(bm_Flags & (1<<1)) {
			p="BM_LD";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_LD);
			p="BM_LD_Frm";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_LD_Frm);
			p="BM_LD_XP";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),exit_LD_XP);
		}
		if(bm_Flags & (1<<2)) {
			p="BM_FD";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_FD);
			p="BM_FD_Frm";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_FD_Frm);
			p="BM_FD_XP";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),exit_FD_XP);
		}
		if(bm_Flags & (1<<3)) {
			p="BM_PD";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_PD);
			p="BM_PD_Frm";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_PD_Frm);
			p="BM_PD_XP";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),exit_PD_XP);
		}
		if(bm_Flags & (1<<4)) {
			p="BM_PC";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_PC);
			p="BM_PC_Frm";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_PC_Frm);
			p="BM_PC_XP";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),exit_PC_XP);
		}
		if(bm_Flags & (1<<5)) {
			p="BM_FM";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_FM);
			p="BM_FM_Frm";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),bm_FM_Frm);
			p="BM_FM_XP";
			for(d=dest;*d++=*p++;);			// strcat
			var = GetVar(env,pfixbf);
			env->SetVar(var.Defined()?pfixbf:env->SaveString(pfixbf),exit_FM_XP);
		}
	}
	return Result;
}




