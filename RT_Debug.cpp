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

AVSValue __cdecl RT_Debug(AVSValue args, void* user_data, IScriptEnvironment* env) {
	enum{BFSZ=1024};
	char bf[BFSZ+16];														// with a little spare
	int n,c,narr		= args[0].ArraySize();
	bool name=args[1].AsBool(true);
	bool repeat=args[2].AsBool(true);
	const unsigned char *s;
	int o=0;
	int nstart=0;
	if(name) {																// not omitting name
		s=(const unsigned char *)"RT_Debug:";
		for(; *s ;bf[o++]=*s++);
	} else if(repeat) {
		s = (const unsigned char *)args[0][0].AsString();					// 1st string arg
		for(; *s ;bf[o++]=*s++);
		nstart=1;
	}
	const int off = o;														// 1st char after 'RT_Debug:' 
	int lines = 0;
	for(o = off, n = nstart; n < narr; ++n) {
		s = (const unsigned char *)args[0][n].AsString();					// Next string arg
		if(o < BFSZ && *s && o > 0 && bf[o-1] != ' ')
			bf[o++]=' ';													// Space Separator
		while((c=*s) && o < BFSZ) {											// while not end of string and not buffer oflow
			if(c=='\n' || c=='\r') {
				c=*(++s);													// Skip EOL (CR or NL)
				if((c=='\n' && s[-1]=='\r') || (c=='\r' &&  s[-1]=='\n'))	// Allow skip of matched pairs CR/LF or LF/CR
					c=*(++s);												// skip 2nd of match pair EOL
				bf[o++]	='\n';
				bf[o]	='\0';
				std::cerr << bf;												// send to DebugView with n/l
				++lines;
				o=off;														// Back to beginning (after "RT_Debug:")
				if(c && o > 0 && bf[o-1] != ' ')
					bf[o++]=' ';											// Space Separator after "RT_Debug:", more coming.
				continue;													// Next line after EOL
			}
			if(o < BFSZ)  {
				if(c==2)
					c = '\1';												// Convert odd effect '\2' to '\1'
				bf[o++] = c;												// Store it
			}
			++s;															// Next char
		}
	}
	if(o > off || lines==0) {												// allow single blank line separators
		bf[o++]	='\n';
		bf[o]	='\0';
		std::cerr << bf;												// Flush remaining text.
	}
    return 0;
}


AVSValue __cdecl RT_DebugF(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_DebugF: ";
	const char *s		=	args[0].AsString();				// text
	int arrsz			=	args[1].ArraySize();
	const char *name	=	args[2].AsString("RT_DebugF:");
	int tabsz			=	args[3].AsInt(8);

	if(tabsz<1 || tabsz>32)
		env->ThrowError("%sIllegal TabSz (1->32)",myName);

	enum {
		CHICKEN=64
	};


	const char * sp;
	int ntabs;
	// count tabs (will be expanded) 
	for(ntabs=0,sp=s;*sp;++sp) {
		if((*sp=='\\' && sp[1]=='t') || (*sp=='\t'))
			++ntabs;
	}

	// what size buffer we need ?
	int i,mem=int(strlen(name) + 1 + strlen(s) + 1 + (ntabs * (tabsz-1)) + CHICKEN);

	for(i=0;i<arrsz;++i) {	
		if(args[1][i].IsString()) {
			const char *st=args[1][i].AsString();
			mem += int(strlen(st) + 1 + CHICKEN);
		} else {
			mem += 8 + CHICKEN;		// no particular reason why so big, just chicken factor.
		}
	}

	int memsz = (mem+1)*2;

	char *pbuf = new char[memsz];

	if(pbuf==NULL)
		env->ThrowError("%sCannot allocate memory",myName);

	char *ptem=pbuf+(mem+1);			// temp buffer

	const unsigned char * r= (const unsigned char*)s;
	char *p=pbuf;
	int  c,ix=0;
	int t=0;

	while(*name)
		*p++ = *name++;
	if(p>pbuf && p[-1] != ' ')
		*p++=' ';
	int offset=int(p-pbuf);
	int lines = 0;
	// Parse text and insert variables
	while(c=*r) {
		if(c=='%') {
			++r;
			if(*r=='\0') {
				*p++ ='%';
			} else if(*r=='%') {
				*p++=*r++;				// replace escaped double % with single
			} else {
				if(ix>=arrsz) {
					delete [] pbuf;
					env->ThrowError("%sExpecting data arg (%d)",myName,ix+1);
				}
				char *tp=ptem;
				*tp++='%';
				if(*r=='-' || *r=='+' || *r=='0' || *r==' ' || *r=='#')		// flags
					*tp++=*r++;
				if(*r=='*') {										// int holds length
					t=args[1][ix].IsBool()		    ?1: \
							args[1][ix].IsString()	?2: \
							args[1][ix].IsInt()		?3: \
							args[1][ix].IsFloat()	?4: \
							0;
					if(t!=3)	{
						delete [] pbuf;
						env->ThrowError("%sUnsupported data type, Expecting Width as Int (%d)",myName,ix+1);
					}
					tp+=sprintf(tp,"%d",args[1][ix].AsInt());
					++r;											// skip '*'
					++ix;											// next data
				} else {
					while(*r>='0' && *r<='9') {
						*tp++ = *r++;
					}
				}
				if(*r=='.') {
					*tp++ =  *r++;										// precision prefix
					if(*r=='*') {										// int holds length
						t=args[1][ix].IsBool()			?1: \
								args[1][ix].IsString()	?2: \
								args[1][ix].IsInt()		?3: \
								args[1][ix].IsFloat()	?4: \
								0;
						if(t!=3) {
							delete [] pbuf;
							env->ThrowError("%sUnsupported data type, Expecting Precision as Int (%d)",myName,ix+1);
						}
						tp+=sprintf(tp,"%d",args[1][ix].AsInt());
						++r;											// skip '*'
						++ix;											// next data
					} else {
						while(*r>='0' && *r<='9') {
							*tp++ = *r++;
						}
					}
				}
				t=args[1][ix].IsBool()		    ?1: \
						args[1][ix].IsString()	?2: \
						args[1][ix].IsInt()		?3: \
						args[1][ix].IsFloat()	?4: \
						0;
				// type
				if(	(*r=='c' ) || (*r=='C' ) ||											// char as int
					(*r=='d' || *r=='i') ||												// int
					(*r=='o' || *r=='u' || *r=='x' || *r=='X'))  {						// unsigned int
					if(t!=3)	{
						int tmpc=*r;
						delete [] pbuf;
						env->ThrowError("%sType='%c', Expecting Int data (%d)",myName,tmpc,ix+1);
					}
					*tp++=*r++;
					*tp='\0';
					p+=sprintf(p,ptem,args[1][ix].AsInt());
					++ix;																// next data
				} else if(*r=='e' || *r=='E' || *r=='f' || *r=='g' || *r=='G') {		// double
					if(t!=4&&t!=3)	{
						int tmpc=*r;
						delete [] pbuf;
						env->ThrowError("%sType='%c', Expecting Float (%d)",myName,tmpc,ix+1);
					}
					*tp++=*r++;
					*tp='\0';
					p+=sprintf(p,ptem,args[1][ix].AsFloat());
					++ix;																// next data
				} else if((*r=='s')||(*r=='S')) {													// string
					if(t!=2&&t!=1)	{
						delete [] pbuf;
						env->ThrowError("%sType='s', Expecting String (%d)",myName,ix+1);
					}
					*tp++=*r++;
					*tp='\0';
					if(t==1) {	// Bool
						p+=sprintf(p,ptem,args[1][ix].AsBool()?"True":"False");
					} else {    // String
						char *run= p;
						p+=sprintf(p,ptem,args[1][ix].AsString());
						char *end= p;
						while(run<end && (*run!='\n' && *run!='\r'))
							++run;
						if(run != end) {								// Damn, we got embedded '\n' or '\r'.
							char remc1 = run[0];						// rem overwritten chars
							char remc2 = run[1];
							run[0]='\n';
							run[1]='\0';
							std::cerr << pbuf;					// send to DebugView with name and n/l
							run[0]=remc1;								// restore
							run[1]=remc2;
							if((run[0]=='\r' && run[1]=='\n') || (run[0]=='\n' && run[1]=='\r'))
								++run;									// skip matched \r\n pairs
							++run;
							p = pbuf + offset;							// we have to move remaining text down & rescan
							while(run<end) {
								if(*run!='\n' && *run!='\r') {
									*p++ = *run++;						// to beginning of buffer after name
								} else {
									*p++='\n';
									*p='\0';
									std::cerr << pbuf;  			// send to DebugView with n/l
									if((run[0]=='\r' && run[1]=='\n') || (run[0]=='\n' && run[1]=='\r'))
										++run;							// skip matched \r\n pairs
									++run;
									p = pbuf + offset;
								}
							}
							*p='\0';
						}
					}
					++ix;																// next data
				} else {
					int tmpc=*r;
					delete [] pbuf;
					env->ThrowError("%sUnknown format type '%c' (%d)",myName,tmpc,ix+1);
				}
			}			
		} else if(c == '\\') {
			++r;
			c=*r;
			// nrt
			switch (c) {
			case '\0' :	*p++='\\';				break;		// copy single backslash at end of string
			case '\\' :	*p++=*r++;				break;		// replace double backslash with single backslash
			case 'n' :
			case 'r' :
				++r;

				*p++='\n';
				*p='\0';
				std::cerr << pbuf;					// send to DebugView with n/l
				p=pbuf + offset;	;
				break;
			case 't' :
				++r;
				{
					int i = tabsz - ((p-pbuf) % tabsz);
					for(;--i >= 0; )
						*p++ = ' ';
				}
				break;
			default :	*p++='\\';	*p++=*r++;	break;		// anything else we copy backslash and whatever follows
			}
		} else if(c < 32 || c >= 127) {
			++r;
			// nrt
			switch (c) {
			case '\n' :
			case '\r' :
				*p++='\n';
				*p='\0';
				std::cerr << pbuf;					// send to DebugView with n/l
				p=pbuf + offset;	;
				break;
			case '\t' :
				{
					int i = tabsz - ((p-pbuf-offset) % tabsz);
					for(;--i >= 0; )
						*p++ = ' ';
				}
				break;
			case 2:
				*p++ = '\1';								// convert ODD effect 2 to 1
				break;
			default :	*p++=c;
				break;										// anything else we copy
			}
		} else {
			*p++=*r++;
		}
	}
	*p=0;											// nul term

	if(ix<arrsz) {
		delete [] pbuf;
		env->ThrowError("%sUnexpected data arg (%d)",myName,ix+1);
	}

	if(p > pbuf+offset || lines==0) {										// allow single blank line separators
		*p++	='\n';
		*p		='\0';
		std::cerr << pbuf;											// Flush remaining text.
	}

	delete [] pbuf;
    return  0;
}


