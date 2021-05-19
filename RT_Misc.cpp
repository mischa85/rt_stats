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
#include <iostream>

AVSValue __cdecl GetVar(IScriptEnvironment* env, const char* name) {
    try {return env->GetVar(name);} catch (IScriptEnvironment::NotFound) {} return AVSValue();}

int __cdecl dprintf(char* fmt, ...) {
char printString[2048]="";
	char *p=printString;
	va_list argp;
	va_start(argp, fmt);
	vsprintf(p, fmt, argp);
	va_end(argp);
	for(;*p++;);
	--p;										// @ null term
	if(printString == p || p[-1] != '\n') {
		p[0]='\n';							// append n/l if not there already
		p[1]='\0';
	}
	std::cerr << printString;
	return int(p-printString);						// strlen printString
}



void __cdecl Ucase(char *s) {
	int c;
	for(;c=*s++;) {
		if(c >= 'a' && c <= 'z')
			s[-1] = c - ('a' - 'A');
	}
}


void __cdecl SplitFn(const char *fn,char*path,char *name,char *ext) {
	const char *s,*p,*e;
	char c;
	for(p=s=fn;c=*s++ ;) {
		if(c=='\\' || c == '/')
			p = s;												// Point 1 after last slash
	}
	if(path != NULL) {memcpy(path,fn,p-fn);	path[p-fn]='\0';}
	for(e=s=p ;c=*s++ ;) {
		if(c == '.')
			e = s-1;											// Point @ last '.'
	}
	if(name != NULL) {memcpy(name,p,e-p);	name[e-p]='\0';}
	if(ext != NULL) {strcpy(ext,e);}
}



char * __cdecl GetErrorString(unsigned int dwLastError) {
// The arg is optional, from Prototype: extern char * GetErrorString(DWORD dwLastError=GetLastError());
// Ownership of returned string is passed to client and must be deleted by them.
    /*
	DWORD dwFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM ;

    HMODULE hModule		= NULL; // default to system source
    LPSTR MessageBuffer;
    DWORD dwBufferLength;
	char * str = NULL;
    //
    // Call FormatMessage() to allow for message text to be acquired from the system 
    if(dwBufferLength = FormatMessageA(
        dwFormatFlags,
        hModule,											// module to get message from (NULL == system)
        dwLastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),			// default language
        (LPSTR) &MessageBuffer,
        0,
        NULL
        )) {
			int len = int(strlen(MessageBuffer));
			str = new char[len+16];
			if(str!=NULL) {
				char c,*s=MessageBuffer,*d=str;
				while(c=*s) {
					if(c=='\r' || c=='\n')
						break;
					*d++=c;
					++s;
				}
				sprintf(d," (0x%08X)",dwLastError);
			}
			// Free the buffer allocated by the system.
			//
			LocalFree(MessageBuffer);
    } else {
		str=new char [128];
		if(str) {
			sprintf(str,"Unknown Error (0x%08X)",dwLastError);
		}
	}
	*/
	char * str = NULL;
	return str;
}

const char * __cdecl StrStrC(const char *s,const char *sub,const bool sig,int slen,int  sublen) {
// NOTE, both slen and sublen optional, default -1 is do strlen()
	if(sublen<0)		sublen = int(strlen(sub));
	if(sublen==0)		return s;				// empty substring return pointer to string s
	if (slen < 0)		slen=int(strlen(s));
	int end=slen-sublen;
	int i,j;
	for(i=0;i<=end;++i) {
		for(j=0;j<sublen;++j) {
			int c=((unsigned char*)s)[i+j];
			int c2=((unsigned char*)sub)[j];
			if(!sig) {
				if(c>='a' && c<='z')
					c -= ('a' - 'A');				// Upper Case
				if(c2>='a' && c2<='z')
					c2 -= ('a' - 'A');				// Upper Case
			}
			if(c != c2)
				break;
		}
		if(j==sublen)
			return s+i;
	}
	return NULL;		// Not found
}

