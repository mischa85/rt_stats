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
#include <string>

AVSValue __cdecl RT_Undefined(AVSValue args, void* user_data, IScriptEnvironment* env) {
	AVSValue Und;
	return Und;
}
	
AVSValue __cdecl RT_VarExist(AVSValue args, void* user_data, IScriptEnvironment* env) {
    try {env->GetVar(args[0].AsString());
	} catch (IScriptEnvironment::NotFound) {return false;} 
	return true;
}

AVSValue __cdecl RT_VarType(AVSValue args, void* user_data, IScriptEnvironment* env) {
	AVSValue ret,a = args[0];
	int type = -1;
	if(a.Defined())
		type = (a.IsInt())?1:(a.IsFloat())?2:(a.IsBool())?0:(a.IsString())?3:(a.IsClip())?4:-1;
	return type;
}


AVSValue __cdecl RT_VarIsSame(AVSValue args, void* user_data, IScriptEnvironment* env) {
	AVSValue a = args[0],b=args[1];
	const int bn = b.ArraySize();
	bool sig = args[2].AsBool(true);
	int i = 0;
	if(bn >= 1) {
		const int at=(!a.Defined())?-1:(a.IsInt())?1:(a.IsFloat())?2:(a.IsBool())?0:(a.IsString())?3:(a.IsClip())?4:-1;
		if (at==0)	{			// BOOL	
			bool v = a.AsBool();			for(i=0;i<bn && (b[i].IsBool()	&& (b[i].AsBool()==v)); ++i);
		} else if(at==1) {		// INT
			int v = a.AsInt();				for(i=0;i<bn && (b[i].IsInt()	&& (b[i].AsInt()==v)); ++i);
		} else if(at==2)	{	// FLOAT
			float v = (float)a.AsFloat();	for(i=0;i<bn && (b[i].IsFloat() && ((float)b[i].AsFloat()==v)); ++i);
		} else if (at==3)	{	// STRING
			const char * v = a.AsString();
			if(sig) {
				for(i=0;i<bn && b[i].IsString();++i) {
					const char *s1 = v;
					const char *s2 = b[i].AsString();
					while(*s1!='\0' && *s1==*s2) {
						++s1;
						++s2;
					}
					if(*s1 != *s2)
						break;
				}
			} else {
				int c1,c2;
				for(i=0;i<bn && b[i].IsString();++i) {
					const char *s1 = v;
					const char *s2 = b[i].AsString();
					while(c1=*s1,c2=*s2,c1!='\0') {
						if(c1 >= 'a' && c1 <= 'z')
							c1 = c1 - ('a' - 'A');
						if(c2 >= 'a' && c2 <= 'z')
							c2 = c2 - ('a' - 'A');
						++s1;
						++s2;
					}
					if(c1 != c2)
						break;
				}
			}
		} else if(at==4)	{	// CLIP
			PClip v  = a.AsClip();
			const VideoInfo &vi = v->GetVideoInfo();
			bool hv =vi.HasVideo();
			bool ha =vi.HasAudio();
			for(i=0;i<bn && b[i].IsClip();++i) {
				PClip v2  = b[i].AsClip();
				const VideoInfo &vi2 = v2->GetVideoInfo();
				if(hv) {
					if(vi.width!=vi2.width || vi.height!=vi2.height || !vi.IsSameColorspace(vi2))
						break;
				} else if(vi2.HasVideo())
					break;
				if(sig) {
					if(ha) {
						if(vi.AudioChannels()!=vi2.AudioChannels()	||
							vi.SampleType()!=vi2.SampleType()		||
							vi.SamplesPerSecond()!=vi2.SamplesPerSecond())
							break;
					} else if(vi2.HasAudio())
						break;
				}
			}
		} else {				// UNDEFINED
			for(i=0;i<bn && !b[i].Defined(); ++i);
		}
	}
	return (i == bn);
}


AVSValue __cdecl RT_FunctionExist(AVSValue args, void* user_data, IScriptEnvironment* env) {
    return env->FunctionExists(args[0].AsString());
}


AVSValue __cdecl RT_Ord(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * s = args[0].AsString();
	size_t pos = args[1].AsInt(1);
	size_t ln = strlen(s);
	return (int)((pos==0 || pos>=(ln+1)) ? 0 : ((unsigned char *)s)[pos-1]);
}

AVSValue __cdecl RT_Timer(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return  double(clock()) / double(CLOCKS_PER_SEC);
}


double __cdecl RT_TimerHP_Lo(void) {
	/*
	typedef uint8_t BYTE;
	typedef uint32_t DWORD;
	typedef int32_t LONG;
	typedef int64_t LONGLONG;

	typedef union _LARGE_INTEGER {
  		struct {
    	DWORD LowPart;
    	LONG  HighPart;
  	};
  	struct {
    	DWORD LowPart;
  	  	LONG  HighPart;
  	} u;
  	LONGLONG QuadPart;
	} LARGE_INTEGER, *PLARGE_INTEGER;

    LARGE_INTEGER liPerfCounter = {0,0};
    LARGE_INTEGER liPerfFreq    = {0,0};
    bool bStat              = true;
    
	uintptr_t dwpOldMask    = SetThreadAffinityMask(GetCurrentThread(), 0x01);
	
    Sleep(0);
    if ((QueryPerformanceFrequency(&liPerfFreq) == 0) || (QueryPerformanceCounter(&liPerfCounter) == 0))
        bStat = false;
    double tim;
    if(!bStat) { // High precision NOT available.
        static clock_t first_time32=0;
        static bool done32=false;
        const clock_t t = clock();
        if(!done32) {
            first_time32 = t;
            done32=true;
        }
        tim = double(t-first_time32) / double(CLOCKS_PER_SEC);     SetThreadAffinityMask
        if(!done64) {
            first_time64 = liPerfCounter.QuadPart;
            done64=true;
        }
        tim = (double)(liPerfCounter.QuadPart-first_time64) / (double)liPerfFreq.QuadPart;
    }
    SetThreadAffinityMask(GetCurrentThread(), dwpOldMask);
    Sleep(0);
    */
   	double tim;
	return tim;                                                 // return double for C client
}


AVSValue __cdecl RT_TimerHP(AVSValue args, void* user_data, IScriptEnvironment* env) {
    return RT_TimerHP_Lo();                                                  // Implicit type conversion to AVSValue float
}



AVSValue __cdecl RT_IncrGlobal(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *s=args[0].AsString();
	int val;
	try {
		AVSValue var  =	env->GetVar(s);
		if(!var.IsInt())
			env->ThrowError("RT_IncrGlobal: Global Var %s is not an int",s);
		val = var.AsInt() + 1;
		env->SetGlobalVar(s,val);
	} catch (IScriptEnvironment::NotFound) {
		val = args[1].AsInt(1);
		env->SetGlobalVar(env->SaveString(s),val);
	} 
	return val;
}

AVSValue __cdecl RT_Version(AVSValue args, void* user_data, IScriptEnvironment* env) {
	double v = VERSION_NUMBER;
	if(VERSION_BETA > 0) {
		v = v - 0.001 + (VERSION_BETA / 100000.0);
	}
	return  v;
}

AVSValue __cdecl RT_VersionString(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char bf[64],beta[16];
	beta[0]='\0';
	if(VERSION_BETA > 0) {
		sprintf(beta,"Beta%02d",VERSION_BETA);
	}
	sprintf(bf,"%.2f%s",VERSION_NUMBER,beta);
	return env->SaveString(bf);
}

AVSValue __cdecl RT_VersionDll(AVSValue args, void* user_data, IScriptEnvironment* env) {
	# ifdef AVISYNTH_PLUGIN_25
		return 2.5;
	# else
		return 2.6;
	# endif
}


AVSValue __cdecl RT_PluginDir(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_PluginDir: ";
	char * varname="$PluginDir$";
	char *ps;
	try {
		AVSValue var  =	env->GetVar(varname);
		if(!var.IsString()) {
			env->ThrowError("%s Var '%s' is not a string",myName,varname);
		}
		ps = (char*)var.AsString();
	} catch (IScriptEnvironment::NotFound) {
		ps="";
	} 
    return env->SaveString(ps);
}

AVSValue __cdecl RT_PluginFunctions(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_PluginFunctions: ";
	char * varname="$PluginFunctions$";
	char *ps;
	try {
		AVSValue var  =	env->GetVar(varname);
		if(!var.IsString()) {
			env->ThrowError("%s Var '%s' is not a string",myName,varname);
		}
		ps = (char*)var.AsString();
	} catch (IScriptEnvironment::NotFound) {
		ps="";
	} 
    return env->SaveString(ps);
}

AVSValue __cdecl RT_InternalFunctions(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_InternalFunctions: ";
	char * varname="$InternalFunctions$";
	char *ps;
	try {
		AVSValue var  =	env->GetVar(varname);
		if(!var.IsString()) {
			env->ThrowError("%s Var '%s' is not a string",myName,varname);
		}
		ps = (char*)var.AsString();		
	} catch (IScriptEnvironment::NotFound) {
		ps="";
	} 
    return env->SaveString(ps);
}


AVSValue __cdecl RT_ScriptName(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_ScriptName: ";
	char * varname="$ScriptName$";
	char *ps;
	try {
		AVSValue var  =	env->GetVar(varname);
		if(!var.IsString()) {
			env->ThrowError("%s Var '%s' is not a string",myName,varname);
		}
		ps = (char*)var.AsString();		
	} catch (IScriptEnvironment::NotFound) {
		ps="";
	} 
    return env->SaveString(ps);
}

AVSValue __cdecl RT_ScriptFile(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_ScriptFile: ";
	char * varname="$ScriptFile$";
	char *ps;
	try {
		AVSValue var  =	env->GetVar(varname);
		if(!var.IsString()) {
			env->ThrowError("%s Var '%s' is not a string",myName,varname);
		}
		ps = (char*)var.AsString();
	} catch (IScriptEnvironment::NotFound) {
		ps="";
	} 
    return env->SaveString(ps);
}

AVSValue __cdecl RT_ScriptDir(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_ScriptDir: ";
	char * varname="$ScriptDir$";
	char *ps;
	try {
		AVSValue var  =	env->GetVar(varname);
		if(!var.IsString()) {
			env->ThrowError("%s Var '%s' is not a string",myName,varname);
		}
		ps = (char*)var.AsString();
	} catch (IScriptEnvironment::NotFound) {
		ps="";
	} 
    return env->SaveString(ps);
}


AVSValue __cdecl RT_PluginParam(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_PluginParam: ";
	const char *s=args[0].AsString();
	int len = int(strlen(s));
	char *bf = new char[len + 16];
	if(bf==NULL)	env->ThrowError("%sCannot allocate memory",myName);
	strcpy(bf,"$Plugin!");
	strcat(bf,s);
	strcat(bf,"!Param$");
	char *ps;
	try {
		AVSValue var  =	env->GetVar(bf);
		if(!var.IsString()) {
			delete [] bf;
			env->ThrowError("%s Var '$Plugin!%s!Param$' is not a string",myName,s);
		}
		ps = (char*)var.AsString();
	} catch (IScriptEnvironment::NotFound) {
		ps="";
	} 
	delete [] bf;
	return env->SaveString(ps);
}


AVSValue __cdecl RT_GetSystemEnv(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// char * myName="RT_GetSystemEnv: ";
	const char *s=args[0].AsString();
	char * sysenv = getenv(s);
	if(sysenv==NULL)
		sysenv="";
	return env->SaveString(sysenv);
}

/*

HANDLE CreateFile(
  LPCTSTR lpFileName,          // pointer to name of the file
  DWORD dwDesiredAccess,       // access (read-write) mode
  DWORD dwShareMode,           // share mode
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                               // pointer to security attributes
  DWORD dwCreationDisposition,  // how to create
  DWORD dwFlagsAndAttributes,  // file attributes
  HANDLE hTemplateFile         // handle to file with attributes to 
                               // copy
);

BOOL CloseHandle(
  HANDLE hObject   // handle to object to close
);

The GetFileTime function retrieves the date and time that a file was created, last accessed, and last modified. 

BOOL GetFileTime(
  HANDLE hFile,                 // handle to the file
  LPFILETIME lpCreationTime,    // address of creation time
  LPFILETIME lpLastAccessTime,  // address of last access time
  LPFILETIME lpLastWriteTime    // address of last write time
);

 
The FILETIME structure is a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601. 

typedef struct _FILETIME { // ft 
    DWORD dwLowDateTime; 
    DWORD dwHighDateTime; 
} FILETIME; 
 
typedef struct _SYSTEMTIME {  // st 
    WORD wYear; 
    WORD wMonth; 
    WORD wDayOfWeek; 
    WORD wDay; 
    WORD wHour; 
    WORD wMinute; 
    WORD wSecond; 
    WORD wMilliseconds; 
} SYSTEMTIME; 

VOID GetLocalTime(
  LPSYSTEMTIME lpSystemTime   // address of system time structure
);

DWORD GetTickCount(VOID)


*/

AVSValue __cdecl RT_GetFileTime(AVSValue args, void* user_data, IScriptEnvironment* env) {
	/*
	char * myName="RT_GetFileTime: ";
	const char *fn=args[0].AsString();
	const int item = args[1].AsInt();
	if(item <0 || item > 2) {
        env->ThrowError("%sError invalid time item int %d(0 -> 2)",myName,item);
	}

	SetLastError(ERROR_SUCCESS);

    HANDLE hFile = CreateFile(
		fn,					// Filename
		0,					// An application can query device attributes without accessing the device. 
		FILE_SHARE_READ,	// Subsequent open operations on the object will succeed only if read access is requested.
		NULL,				// If lpSecurityAttributes is NULL, the handle cannot be inherited.  
		OPEN_EXISTING,		// Opens the file. The function fails if the file does not exist.
		0,					// File attrubtes, Anything
		NULL				// No template file
		);
    if (hFile == INVALID_HANDLE_VALUE) {
        env->ThrowError("%sError cannot open file '%s': %s",myName,fn,GetErrorString());
    }

	FILETIME	ft = { 0 };
    SYSTEMTIME	st = { 0 };

	LPFILETIME ct = (item==0) ? &ft : NULL;
	LPFILETIME wt = (item==1) ? &ft : NULL;
	LPFILETIME at = (item==2) ? &ft : NULL;

    BOOL ok = GetFileTime(hFile, ct, at, wt);

	CloseHandle(hFile);

	if(!ok)
        env->ThrowError("%sError Cannot get file time (%s : %s) ",myName,fn,GetErrorString());
	
    int ret=FileTimeToSystemTime(&ft,	&st);
    if(ret==0)
		env->ThrowError("%sError Cannot convert to SystemTime (%s : %s) ",myName,fn,GetErrorString());

	*/
	char bf[64];
	// sprintf(bf,"%4d-%02d-%02d %02d:%02d:%02d.%03d",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
	
	return env->SaveString(bf);
}

AVSValue __cdecl RT_LocalTimeString(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const bool file = args[0].AsBool(true);

    //     SYSTEMTIME  st = { 0 };

	struct timespec now;
    struct tm tm;

	/*
	if(file) {
		unsigned int tick = clock_gettime(CLOCK_MONOTONIC,&now); // GetTickCount();
		while(clock_gettime(CLOCK_MONOTONIC,&now)==tick)	// Wait until system clock goes TICK (prevent two separate calls returning same time)
		    sleep(0);
	}
	*/
	
	char bf[64];
    const int bufsize = 31;

    int retval = clock_gettime(CLOCK_REALTIME, &now);
	gmtime_r(&now.tv_sec, &tm);
    
	if(file) {
		strftime(bf, bufsize, "%Y-%m-%dT%H:%M:%S.", &tm);
    	sprintf(bf, "%s%09luZ", bf, now.tv_nsec);
	} else {
		strftime(bf, bufsize, "%Y-%m-%dT%H:%M:%S.", &tm);
    	sprintf(bf, "%s%09luZ", bf, now.tv_nsec);
    }

	/*
	if(file) {
        sprintf(bf,"%4d%02d%02d_%02d%02d%02d_%03d",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
    } else {
        sprintf(bf,"%4d-%02d-%02d %02d:%02d:%02d.%03d",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
    }
	*/
	
	return env->SaveString(bf);
}

