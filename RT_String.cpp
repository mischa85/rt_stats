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

// REMark: env->SaveString(s,sz)
// env->SaveString(s)		// s must be null term string, auto sets size.
// env->SaveString(s,sz)	// sz is len of text excluding nul term which rout adds. (uses memcpy not strcpy)


AVSValue __cdecl RT_StrAddStr(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *s1		= args[0].AsString();						// 1st string
	AVSValue sarr		= args[1];									// string array as AVSValues
	int narr			= sarr.ArraySize();							// Number of strings in array, 1 or more
	const char *s,*sn;
	int n,tot;
	for(s=s1 ; *s++ ;);												// scan till 1 past nul
	--s;															// back up 1 to point at nul term
	tot=int(s-s1);														// len of 1st string, excl nul
	for(n = 0; n < narr; ++n) {
		sn = sarr[n].AsString();
		for(s=sn ; *s++ ;);
		--s;
		tot += int(s-sn);												// count total text length of strings
	}
	char *p,*pbf = new char[tot+1];									// alloc buf total size (+1 overwrite last with nul)
	if(pbf==NULL)
		env->ThrowError("RT_StrAddStr: Cannot allocate memory");
	for(s=s1,p=pbf ; *p++=*s++ ;);									// strcpy 1st string to pbf
	--p;															// back up 1 to point at nul term
	for(n = 0; n < narr; ++n) {										// strcat additional strings to pbf
		sn = sarr[n].AsString();
		for(s=sn ; *p++=*s++  ;);		
		--p;
	}
	AVSValue ret =	env->SaveString(pbf,int(p-pbf));						// p-pbf is text len only (excl nul)
	delete [] pbf;													// delete our temp buffer
	return ret;	
}


AVSValue __cdecl RT_TxtAddStr(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *s1		= args[0].AsString();						// 1st string
	AVSValue sarr		= args[1];									// string array as AVSValues
	int narr			= sarr.ArraySize();							// Number of strings in array, 1 or more
	const char *s,*sn;
	int n,tot,c;
	for(s=s1 ; *s++ ;);												// scan till 1 past nul
	--s;															// back up 1 to point at nul term
	tot=int(s-s1);														// len of 1st string, excl nul
	for(n = 0; n < narr; ++n) {
		sn = sarr[n].AsString();
		for(s=sn ; *s++ ;);
		--s;
		tot += int(s-sn);												// count total text length (incl n/l) of strings
	}
	char *p,*pbf = new char[tot+narr+1];							// alloc buf tot size + (narr + 1) for '\n' (not nul)
	if(pbf==NULL)
		env->ThrowError("RT_TxtAddStr: Cannot allocate memory");
	for(s=s1,p=pbf ; c = *s++ ;) {									// strcpy 1st string to pbf
		if(c== '\n' || c == '\r') {
			if(c == '\n') {
				if(*s == '\r')
					++s;
			} else if(c == '\r') {
				if(*s == '\n')
					++s;
			}
			*p++='\n';
		} else {
			*p++ = c;
		}
	}
	// ONLY NON empty strings have n/l appended on 1st string
	if(p > pbf && p[-1] != '\n')
		*p++ = '\n';												// n/l separator
	for(n = 0; n < narr; ++n) {										// strcat additional strings to pbf
		sn = sarr[n].AsString();
		char * ps=p;
		for(s=sn ; c = *s++ ;) {									// strcpy 1st string to pbf
			if(c== '\n' || c == '\r') {
				if(c == '\n') {
					if(*s == '\r')
						++s;
				} else if(c == '\r') {
					if(*s == '\n')
						++s;
				}
				*p++='\n';
			} else {
				*p++ = c;
			}
		}
		// On subsequent strings, even empty strings have n/l appended
		if(p == ps || p[-1] != '\n')
			*p++ = '\n';											// n/l separator
	}
	AVSValue ret =	env->SaveString(pbf,int(p-pbf));						// Get pointer to Avisynth
	delete [] pbf;													// delete our temp buffer
	return ret;	
}



AVSValue __cdecl RT_TxtQueryLines(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *Str=args[0].AsString();
	const char *s=Str;
	int c,Lines=0;
	while(c=*s) {
		if(c=='\n' || c == '\r') {
			if((c=='\n' && s[1]=='\r') || (c=='\r' && s[1]=='\n'))
				++s;
			++Lines;
		}
		++s;
	}
	if(Str < s && s[-1] != '\n' && s[-1] != '\r')
		++Lines;															// Last line not n/l terminated, but still counts
	return Lines;
}

AVSValue __cdecl RT_TxtGetLine(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *Str	=args[0].AsString();
	const int Line	=args[1].AsInt(0);								// Default 0 = 1st line (lines 0 relative)
	const char *s=Str;
	int c,Lines=0;
	while((Line > Lines) && (c=*s)) {								// Halt at START of required line
		if(c=='\n' || c == '\r') {
			if((c=='\n' && s[1]=='\r') || (c=='\r' && s[1]=='\n'))
				++s;
			++Lines;
		}
		++s;
	}
	if(Line != Lines) {env->ThrowError("RT_TxtGetLine: Invalid Line Number");}
	const char *e = s;
	while((c=*e) && c != '\n' && c != '\r')
		++e;
	int sz = int(e - s);													// strlen excluding nul term or \r or \n
	AVSValue ret =	env->SaveString(s,sz);							// Get pointer to Avisynth saved string
	return ret;	
}





AVSValue __cdecl RT_QuoteStr(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *s		= args[0].AsString();						// 1st string
	int len = int(strlen(s));
	bool hasquot=false;
	int i;
	for(i=0;i<len;++i) {
		if(hasquot=(s[i]=='"'))	
			break;
	}
	int qcnt = (hasquot) ? 3 : 1;
	int sz = len + (qcnt*2) + 1;
	char *pbf = new char[sz];
	if(pbf==NULL)
		env->ThrowError("RT_QuoteStr: Cannot allocate memory");
	char *p=pbf;
	for(i=0;i<qcnt;++i)
		*p++='"';			// pre quote
	for(;*p++=*s++;);
	--p;					// at nul
	for(i=0;i<qcnt;++i)
		*p++='"';			// post quote
	*p='\0';				// nul term
	AVSValue ret =	env->SaveString(pbf);						// Get pointer to Avisynth
	delete [] pbf;												// delete our temp buffer
	return ret;	
}


char * StrReplace_Lo(const char*string,const char*find,const char*replace,const bool sig) {
	//	Recursive String Replacement.
	//	Returns a string allocated with 'new', ownership is passed to the client who MUST delete [] when done.
	const char *found;	
	char *pbf=NULL;	
	int sn,fn;
	if((sn=int(strlen(string)))==0 || (fn=int(strlen(find)))==0 || (found=StrStrC(string,find,sig))==NULL) {
		char *pbf=	new char[sn+1];	
		if(pbf != NULL)
			strcpy(pbf,string);
		return pbf;													// Return strdup() or NULL for OMEM Error
	} else {
		char * s_new=StrReplace_Lo(found+fn,find,replace,sig);		// Search remainder, Result belongs to us.
		if(s_new==NULL) {
			return NULL;											// OMEM Error
		} else {
			int left_n = int(found-string);
			int replace_n=int(strlen(replace));
			int sz = left_n + replace_n + int(strlen(s_new));
			char *pbf=	new char[sz+1];
			if(pbf != NULL) {
				memcpy(pbf,string,left_n);
				memcpy(pbf+left_n,replace,replace_n);
				strcpy(pbf+left_n+replace_n,s_new);					// and nul term
			}
			delete [] s_new;										// It belongs to us, We MUST delete,
			return pbf;												// pbf belongs to client.
		}
	}
}

AVSValue __cdecl RT_StrReplace(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *string	= args[0].AsString();
	const char *find	= args[1].AsString();
	const char *replace	= args[2].AsString();
	const bool sig		= args[3].AsBool(true);
	AVSValue ret;
	if(*string=='\0' || *find=='\0') {
		ret=string;													// if string=="" or find=="" then return original string
	} else {
		char *new_s = StrReplace_Lo(string,find,replace,sig);			// We MUST delete
		if(new_s==NULL)			env->ThrowError("RT_StrReplace: Cannot allocate memory");
		ret =	env->SaveString(new_s);									// let SaveString calc length
		delete [] new_s;												// delete temp buffer (it belongs to us)
	}
	return ret;	
}

char * StrReplaceDeep_Lo(const char*string,const char*find,const char*replace,const bool sig) {
	//	Recursive Deep String Replacement.
	//	Returns a string allocated with 'new', ownership is passed to the client who MUST delete [] when done.
	const char *found;	
	char *pbf=NULL;	
	int sn,fn;
	if((sn=int(strlen(string)))==0 || (fn=int(strlen(find)))==0 || (found=StrStrC(string,find,sig))==NULL) {
		pbf = new char[sn+1];	
		if(pbf != NULL) {
			strcpy(pbf,string);
		}
	} else {
		const int i=int(found-string);
		const int rn=int(strlen(replace));
		const int k = rn - std::min(rn,fn-1);

		pbf = new char[(rn-k) + (sn - (i + fn)) + 1];
		if(pbf!=NULL) {
			memcpy(pbf,replace+k,rn-k);
			strcpy(pbf+rn-k,string+i+fn);
			char * s_new=StrReplaceDeep_Lo(pbf,find,replace,sig);	// Deep re-search and replace
			delete [] pbf;											// done with it
			pbf=NULL;
			if(s_new!=NULL) {
				pbf=new char[i + k + strlen(s_new) + 1];
				if(pbf!=NULL) {
					memcpy(pbf,string,i);
					memcpy(pbf+i,replace,k);
					strcpy(pbf+i+k,s_new);
				}
				delete [] s_new;
			}
		}
	}
	return pbf;													// Return strdup() or NULL for OMEM Error
}

AVSValue __cdecl RT_StrReplaceDeep(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *string	= args[0].AsString();
	const char *find	= args[1].AsString();
	const char *replace	= args[2].AsString();
	const bool sig		= args[3].AsBool(true);
	AVSValue ret;
	if(*string=='\0' || *find=='\0') {
		ret=string;														// if string=="" or find=="" then return original string
	} else {
		char *new_s;
		new_s = StrReplaceDeep_Lo(string,find,replace,sig);				// Deep replace
		if(new_s==NULL)			env->ThrowError("RT_StrReplaceDeep: Cannot allocate memory");
		ret =	env->SaveString(new_s);									// let SaveString calc length
		delete [] new_s;												// delete temp buffer (it belongs to us)
	}
	return ret;	
}



AVSValue __cdecl RT_StrReplaceMulti(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *string	= args[0].AsString();
	const char *find	= args[1].AsString();
	const char *replace	= args[2].AsString();
	const bool sig		= args[3].AsBool(true);
	AVSValue ret;
	if(*string=='\0' || *find=='\0') {
		ret=string;														// if string=="" then return original string
	} else {
		const char*fs,*fe=find;
		const char*rs,*re=replace;
		char c,*s,*fn=NULL,*rn=NULL;
		if((s= new char[strlen(string)+1]) != NULL) {
			strcpy(s,string);											// create initial editable string
		}
		int i=0;
		while(s != NULL){
			if(fn != NULL)	{delete [] fn; fn = NULL;}
			if(rn != NULL)	{delete [] rn; rn = NULL;}

			fs=fe;														// start of current find string
			while((c=*fe) && (c!='\n') && (c!='\r'))					// find end of current replace string
				++fe;
			int fsz=int(fe-fs);												// len excl CR/NL
			if(c !='\0') {
				++fe;													// skip CR or NL
				if((c=='\n' && *fe=='\r') || (c=='\r' && *fe=='\n'))
					++fe;												// skip matched CR/NL
			}

			rs=re;														// start of current replace string
			while((c=*re) && (c!='\n') && (c!='\r'))					// find end of current replace string
				++re;
			int rsz=int(re-rs);												// len excl CR/NL
			if(c !='\0') {
				++re;													// skip CR or NL
				if((c=='\n' && *re=='\r') || (c=='\r' && *re=='\n'))
					++re;												// skip matched CR/NL
			}

			if(fsz==0) {												// zero len find
				if(fs==fe) {											// end of find text
					if(rs==re)											// end of replace text
						break;											// Done
					delete [] s;
					env->ThrowError("RT_StrReplaceMulti: Too many replace strings[%d]",i);					
				}
				delete [] s;
				env->ThrowError("RT_StrReplaceMulti: Zero length find string[%d]",i);								
			} else if(rs==re){											// Unmatched replace string
				delete [] s;
				env->ThrowError("RT_StrReplaceMulti: Missing replace string[%d]",i);								
			}

			if((fn= new char[fsz+1])==NULL) {
				delete [] s;
				s = NULL;
				break;
			}
			memcpy(fn,fs,fsz);
			fn[fsz]='\0';

			if((rn= new char[rsz+1])==NULL) {
				delete [] s;
				s = NULL;
				break;
			}
			memcpy(rn,rs,rsz);
			rn[rsz]='\0';

			char *new_s = StrReplace_Lo(s,fn,rn,sig);						// We MUST delete
			delete [] s;
			s=new_s;
			++i;
		}
		if(fn != NULL)	delete [] fn;
		if(rn != NULL)	delete [] rn;
		if(s==NULL)	env->ThrowError("RT_StrReplace: Cannot allocate memory");
		ret =	env->SaveString(s);											// let SaveString calc length
		delete [] s;														// delete temp buffer (it belongs to us)
	}
	return ret;	
}


AVSValue __cdecl RT_StrPad(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *s					= args[0].AsString();
	const int pad					= args[1].AsInt();
	const unsigned char *character	= (unsigned char *)(args[2].AsString(" "));
	const char *p=s;
	for(;*p++;);
	--p;
	int slen=int(p-s);
	if(slen>=pad)
		return s;								// return original string no env->SaveString
	char *pbf = new char[pad];
	if(pbf==NULL)
		env->ThrowError("RT_StrPad: Cannot allocate memory");
	memcpy(pbf,s,slen);
	memset(pbf+slen,int(*character),pad-slen);
	AVSValue ret =	env->SaveString(pbf,pad);
	delete [] pbf;
	return ret;	
}


AVSValue __cdecl RT_FindStr(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *s					= args[0].AsString();
	const char *subs				= args[1].AsString();
	const bool sig					= args[2].AsBool(true);
	const int pos					= args[3].AsInt(1) - 1;
	int subslen=int(strlen(subs));
	int slen=int(strlen(s));
	int plen=slen-pos;
	if(subslen==0 || plen<=0)
		return 0;					// Empty s or subs always returns not found (0)
	const char *p=StrStrC(s+pos,subs,sig);
	return (p==NULL) ? 0 : int(p - s + 1);
}


AVSValue __cdecl RT_TxtFindStr(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *str			= args[0].AsString();
	const char *subs		= args[1].AsString();
	const bool sig			= args[2].AsBool(true);
	const int  pos			= args[3].AsInt(1) - 1;
	const int  start		= args[4].AsInt(0);
	const int  lines		= args[5].AsInt(0);		// Default is ALL
	if(pos<0)	return -1;	// not in string
	const char *p = subs;
	int end = (lines <= 0) ? 0x7FFFFFFF : start + lines;
	int c;
	while(c=*p, c!='\0' && c!='\n' && c!='\r') ++p;		// find len of sub string, excluding \r or \n
	int subslen=int(p-subs);
	if(subslen==0 || start < 0)
		return -1;					// Empty subs always returns not found (-1)
	int n=0;						// line number, zero rel
	const char *s=str;
	while((start > n) && (c=*s)) {								// Halt at required start line
		if(c=='\n' || c == '\r') {
			if((c=='\n' && s[1]=='\r') || (c=='\r' && s[1]=='\n'))
				++s;
			++n;
		}
		++s;
	}
	if(n != start)	return -1;		// start line not found

	while (*s && n < end) {
		const char *e = s;
		while((c=*e) && c != '\n' && c != '\r')	++e;
		int slen = int(e - s);										// length of text string
		slen -= pos;											// - start position to search = remaining len
		if(slen > 0) {											// remaining string at least as long as subs
			if (StrStrC(s+pos,subs,sig, slen, subslen) != NULL)
				return n;										// GOT IT, return line number, 0 rel
		}
		s=e;
		if(*s=='\n') {
			++s;
			if(*s=='\r')
				++s;
		} else if(*s=='\r') {
			++s;
			if(*s=='\n')
				++s;
		}
		++n;
	}
	return -1;	// not found
}


AVSValue __cdecl RT_String(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_String: ";
	const char *s		=	args[0].AsString();				// text
	int arrsz			=	args[1].ArraySize();
	int esc				=	args[2].AsInt(1);

	if(esc < 0 || esc > 2)
		env->ThrowError("%sEsc range 0 -> 2 only",myName);

	enum {
		CHICKEN=64
	};

	// what size buffer we need ?
	int i,mem=int(strlen(s) + 1 + CHICKEN);
	for(i=0;i<arrsz;++i) {	
		if(args[1][i].IsString()) {
			const char *st=args[1][i].AsString();
			mem += int(strlen(st) + 1 + CHICKEN);
		} else {
			mem += 8 + CHICKEN;		// no particular reason why so big, just chicken factor.
		}
	}

	char *pbuf = new char[(mem+1)*2];
	if(pbuf==NULL)
		env->ThrowError("%sCannot allocate memory",myName);

	char *ptem=pbuf+(mem+1);			// temp buffer
	
	const unsigned char * r= (const unsigned char *)s;
	char *p=pbuf;
	int  c,ix=0;
	int t=0;

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
						p+=sprintf(p,ptem,args[1][ix].AsString());
					}
					++ix;																// next data
				} else {
					int tmpc=*r;
					delete [] pbuf;
					env->ThrowError("%sUnknown format type '%c' (%d)",myName,tmpc,ix+1);
				}
			}			
		} else if(c == '\\' && esc == 1) {
			++r;
			c=*r;
			// abfnrtv
			switch (c) {
			case '\0' :	*p++='\\';				break;		// copy single backslash at end of string
			case '\\' :	*p++=*r++;				break;		// replace double backslash with single backslash
			case 'n' :	++r;		*p++='\n';	break;
			case 'r' :	++r;		*p++='\r';	break;
			case 't' :	++r;		*p++='\t';	break;
			case 'v' :	++r;		*p++='\v';	break;
			case 'f' :	++r;		*p++='\f';	break;
			case 'b' :	++r;		*p++='\b';	break;
			case 'a' :	++r;		*p++='\a';	break;
			default :	*p++='\\';	*p++=*r++;	break;		// anything else we copy backslash and whatever follows
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

//	dprintf("RT_String: MEM Estimate=%d : Actual=%d\n",mem+1,strlen(pbuf));

	if(esc==2) {
		p=pbuf;
		r=(const unsigned char*)p;
		while(c=*r) {
			if(c=='\\') {
				++r;
				c=*r;
				// abfnrtv
				switch (c) {
				case '\0' :	*p++='\\';				break;		// copy single backslash at end of string
				case '\\' :	*p++=*r++;				break;		// replace double backslash with single backslash
				case 'n' :	++r;		*p++='\n';	break;
				case 'r' :	++r;		*p++='\r';	break;
				case 't' :	++r;		*p++='\t';	break;
				case 'v' :	++r;		*p++='\v';	break;
				case 'f' :	++r;		*p++='\f';	break;
				case 'b' :	++r;		*p++='\b';	break;
				case 'a' :	++r;		*p++='\a';	break;
				default :	*p++='\\';	*p++=*r++;	break;		// anything else we copy backslash and whatever follows
				}
			} else {
				*p++=*r++;
			}
		}
		*p=0;											// nul term
	}
	AVSValue ret = env->SaveString(pbuf,int(p-pbuf));
	delete [] pbuf;
    return  ret;
}


