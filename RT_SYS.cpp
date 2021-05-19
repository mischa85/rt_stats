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

#define _CRT_RAND_S		// for rand_s, and requires stdlib.h (before includes)

#include "RT_Stats.h"
#include <limits.h>
#include <algorithm>

int __cdecl RandInt_Lo(int randmax) {
	bool sig = (randmax<0);
	randmax  = abs(randmax);
	unsigned int number;
	number = rand();
	int i = int(((double)number / ((double) UINT_MAX + 1)) * (double)randmax);
	if(sig)	i = -i;
	return i;
}

AVSValue __cdecl RT_RandInt(AVSValue args, void* user_data, IScriptEnvironment* env) {
	int randmax = args[0].AsInt(0x7FFFFFFF);
	if(randmax ==0)				env->ThrowError("RT_RandInt: RandMax Cannot be 0");
	if(randmax == 0x80000000)	env->ThrowError("RT_RandInt: 1 <= Abs(RandMax) <= $7FFFFFFF");
	return RandInt_Lo(randmax);
}

AVSValue __cdecl RT_Sleep(AVSValue args, void* user_data, IScriptEnvironment* env) {
	double tim = std::max(std::min(args[0].AsFloat(),60.0),0.0) * 1000.0;
	unsigned int t = tim;
    sleep(t);
    return 0;
}

AVSValue __cdecl RT_GetPid(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return getpid();
}

AVSValue __cdecl RT_FileDuplicate(AVSValue args, void* user_data, IScriptEnvironment* env) {
	/*
	const char * myName     = "RT_FileDuplicate: ";
	char ebf[256]="";
	FILE * fp1=NULL;
	FILE * fp2=NULL;
	char *bf=NULL;
	int64_t flen=0;
	int bfsz=8*1024;
	char full1[PATH_MAX],full2[PATH_MAX];
	const char*fn1	= args[0].AsString();
	const char*fn2	= args[1].AsString();
	const bool ovwr	= args[2].AsBool(false);
	if(*fn1=='\0')															strcpy(ebf,"Empty source filename1");
	else if(*fn2=='\0')														strcpy(ebf,"Empty dest filename2");
	else if(realpath(fn1,full1))											strcpy(ebf,"Cannot get fullpath filename1");
	else if(realpath(fn2,full2))											strcpy(ebf,"Cannot get fullpath filename2");
	else if(strcasecmp(full1,full2)==0)										strcpy(ebf,"filename1 == filename2");
	else if(!ovwr && GetFileAttributes(full2)!=INVALID_FILE_ATTRIBUTES)		strcpy(ebf,"Filename2 already exists");
	else if((fp1=fopen(full1,"rb"))==NULL)									strcpy(ebf,"Cannot open source file");
	else if((fp2=fopen(full2,"wb"))==NULL)									strcpy(ebf,"Cannot open dest file");
	else if((fseeko(fp1,0,SEEK_END)!=0)||((flen=_ftell0LL(fp1))==-1L))	strcpy(ebf,"Cannot seek in source file");
	else if((bfsz=intstd::minflen,bfsz)))==0)									strcpy(ebf,"Zero len source file");
	else if((bf=new char[bfsz])==NULL)										strcpy(ebf,"Cannot allocate buffer");
	else {
		rewind(fp1);
		int sods = int(flen / bfsz);
		int odds = int(flen % bfsz);
		for(int i=0; i<sods && ebf[0]==0 ;++i) {
			if(fread(bf,  bfsz,1,fp1)!=1)			strcpy(ebf,"Cannot read source file");
			else if(fwrite(bf,bfsz,1,fp2)!=1)		strcpy(ebf,"Cannot write dest file");
		}
		if(ebf[0]==0 && odds > 0) {
			if(fread(bf,odds,1,fp1)!=1)				strcpy(ebf,"Cannot read source file");
			else if(fwrite(bf,odds,1,fp2)!=1)		strcpy(ebf,"Cannot write dest file");
		}
	}
	if(bf != NULL)	delete [] bf;
	if(fp2!= NULL)	fclose(fp2);
	if(fp1!= NULL)	fclose(fp1);
	if(ebf[0]!=0)	env->ThrowError("%sError, %s",myName,ebf);
	*/
	return 0;
}

AVSValue __cdecl RT_ForceProcess(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// Force read frames.
	char *myName="RT_ForceProcess: ";
	if(!args[0].IsClip())		env->ThrowError("%sMust have a clip",myName);
	PClip child  = args[0].AsClip();											// Clip
    const VideoInfo &vi = child->GetVideoInfo();
	const int frames=vi.num_frames;
	const bool video = (args[1].AsBool(true)  && vi.HasVideo() && frames > 0);
	const bool audio = (args[2].AsBool(false) && vi.HasAudio() && vi.num_audio_samples > 0);
	const bool debug=args[3].AsBool(true);
	const int64_t nsamples = (!audio) ? 0 : (video) ? vi.AudioSamplesFromFrames(1) :  vi.audio_samples_per_second;
	BYTE *bf = NULL;
	if(audio) {
		int bfsz = int(vi.BytesFromAudioSamples(nsamples));
		bf = new BYTE[bfsz];
		if(bf==NULL)		env->ThrowError("%sCannot allocate audio buffer",myName);
	}
	if(video) {
		double start_T =  double(clock()) / double(CLOCKS_PER_SEC);
		double sT = start_T;
		int parts = std::min(20,frames);
		int part=1;
		int sf=0;                       // this time start frameno
		int stopf = frames * part / parts;
		if(debug)	dprintf("%sCommencing Forced process",myName);
		for(int n=0;n<frames;++n) {
			if(bf!=NULL) {
				int64_t s = vi.AudioSamplesFromFrames(n);
				int64_t e = vi.AudioSamplesFromFrames(n+1);
				child->GetAudio((void*)bf, s, e-s, env);	// Force 1 video frame worth of audio
			}
			PVideoFrame	src = child->GetFrame(n,env);		// Force decoding. MUST assign to PVideoFrame
	        if(debug && n+1 >= stopf) {
				double now = double(clock()) / double(CLOCKS_PER_SEC);
				double Tim   = std::max(now - sT,0.0001); // time taken for this part
				double FPS = (n+1-sf)/Tim;
				double SPF = 1.0/FPS;
				dprintf("%s%6d] %6.2f%%  nFrms=%d  T=%.3fsec  :  %.2fFpS  %.6fSpF",
					myName,n,(n+1)*100.0/frames,n-sf+1,Tim,FPS,SPF);
				sf=n+1;                    // next time start frame
				sT=now;                    // next time start time
				++part;                    // next time part number
				stopf = frames * part / parts; // next time stop frame
		    }
			if((n & 0x7F)==0) {
				sleep(0);
			}
		}
		if(debug) {
			double Tim = std::max(double(clock()) / double(CLOCKS_PER_SEC) - start_T,0.0001);
			double FPS = frames/Tim;
			double SPF = 1.0/FPS;
			dprintf("%sTime=%.3fsecs (%.3fmins) : Avg %.3fFpS %.6fSpF",myName,Tim,Tim/60.0,FPS,SPF);
		}
	} else if(bf!=NULL) {
		int64_t s,e;
		for(s=0; s < vi.num_audio_samples; s+= nsamples) {
			e = s + nsamples;
			if(e > vi.num_audio_samples) 
				e = vi.num_audio_samples;
			child->GetAudio((void*)bf, s, e-s, env);
		}
	}
	if(bf!=NULL) {
		delete [] bf;
	}
    return 0;
}
