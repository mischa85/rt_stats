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
#include <limits.h>

int strcmpfn(char *s1,char *s2) {
	// string compare on filename node but comparing digits by value, then strlen of digits.
	char n1[PATH_MAX];
	char n2[PATH_MAX];
	char *p1,*p2;
	for(p1=n1;*p1++=*s1++;);
	Ucase(n1);
	for(p2=n2;*p2++=*s2++;);
	Ucase(n2);
	char *e1,*e2,c1,c2;
	p1=n1;
	p2=n2;
	while(*p1 && *p2) {
		for(e1=p1;c1=*e1, c1 && (c1<'0' || c1>'9');++e1);			// skip over plain text, stop on 1st digit
		for(e2=p2;c2=*e2, c2 && (c2<'0' || c2>'9');++e2);
		if(e1-p1 != e2-p2 || *e1=='\0' || *e2=='\0')
			return strcmp(p1,p2);									// plain text different, just use strcmp
		for(;*p1==*p2 && p1<e1 && p2<e2;++p1,++p2);					// scan through plain text matching chars
		if(p1 < e1) {												//
			if(*p1>*p2) {
				return 1;
			} else if(*p1<*p2) {
				return -1;
			}
			// Should never get here.
			return 0;
		}
		// full match for plain text, now check digits,
		int n1,n2;
		for(n1=0;c1=*e1, c1 && (c1>='0' && c1<='9');++e1) {
			n1 = n1 * 10 + (c1 - '0');
		}
		for(n2=0;c2=*e2, c2 && (c2>='0' && c2<='9');++e2) {
			n2 = n2 * 10 + (c2 - '0');
		}
		// now check on value of the digits
		if(n1>n2) {
			return 1;
		} else if(n1<n2) {
			return -1;
		}
		// value of digits is same, check on strlen of digits
		if(e1-p1>e2-p2) {
			return 1;
		} else if(e1-p1<e2-p2) {
			return -1;
		}
		p1=e1;
		p2=e2;
	}
	if(*p1>*p2) {
		return 1;
	} else if(*p1<*p2) {
		return -1;
	}
	return 0;
}


int Srt_FNinc2(const char *s1,const char *s2) {
	char Path1[PATH_MAX];										// Path up to and incl last slash
	char Path2[PATH_MAX];										// Path up to and incl last slash
	char Name1[PATH_MAX];										// Name up to and incl last '.'
	char Name2[PATH_MAX];										// Name up to and incl last '.'
	char Ext1[PATH_MAX];										// User supplied extension with pipe separated wildcards	
	char Ext2[PATH_MAX];										// User supplied extension with pipe separated wildcards	
	SplitFn(s1,Path1,Name1,Ext1);
	SplitFn(s2,Path2,Name2,Ext2);
	int ret;
	if((ret=_stricmp(Path1,Path2)) != 0)	return ret;				// Primarily by path
	if((ret=_stricmp(Ext1,Ext2))   != 0)	return ret;				// Then by Extension
	if((ret=_stricmp(Name1,Name2)) != 0)	return ret;				// then by Name
	return 0;
}

int Srt_FNinc(const char *s1,const char *s2) {
	char Path1[PATH_MAX];										// Path up to and incl last slash
	char Path2[PATH_MAX];										// Path up to and incl last slash
	char Name1[PATH_MAX];										// Name up to and incl last '.'
	char Name2[PATH_MAX];										// Name up to and incl last '.'
	char Ext1[PATH_MAX];										// User supplied extension with pipe separated wildcards	
	char Ext2[PATH_MAX];										// User supplied extension with pipe separated wildcards	
	SplitFn(s1,Path1,Name1,Ext1);
	SplitFn(s2,Path2,Name2,Ext2);
	int ret;
	if((ret=_stricmp(Path1,Path2)) != 0)	return ret;				// Primarily by path
	if((ret=_stricmp(Ext1,Ext2)) != 0)	return ret;				// Then by Extension
	return strcmpfn(Name1,Name2);
}

int Srt_FNdec2(const char *s1,const char *s2) {
	char Path1[PATH_MAX];										// Path up to and incl last slash
	char Path2[PATH_MAX];										// Path up to and incl last slash
	char Name1[PATH_MAX];										// Name up to and incl last '.'
	char Name2[PATH_MAX];										// Name up to and incl last '.'
	char Ext1[PATH_MAX];										// User supplied extension with pipe separated wildcards	
	char Ext2[PATH_MAX];										// User supplied extension with pipe separated wildcards	
	SplitFn(s2,Path1,Name1,Ext1);
	SplitFn(s1,Path2,Name2,Ext2);
	int ret;
	if((ret=_stricmp(Path1,Path2)) != 0)	return ret;				// Primarily by path
	if((ret=_stricmp(Ext1,Ext2)) != 0)	return ret;				// Then by Extension
	if((ret=_stricmp(Name1,Name2)) != 0)	return ret;				// then by Name
	return 0;
}

int Srt_FNdec(const char *s1,const char *s2) {
	char Path1[PATH_MAX];										// Path up to and incl last slash
	char Path2[PATH_MAX];										// Path up to and incl last slash
	char Name1[PATH_MAX];										// Name up to and incl last '.'
	char Name2[PATH_MAX];										// Name up to and incl last '.'
	char Ext1[PATH_MAX];										// User supplied extension with pipe separated wildcards	
	char Ext2[PATH_MAX];										// User supplied extension with pipe separated wildcards	
	SplitFn(s2,Path1,Name1,Ext1);
	SplitFn(s1,Path2,Name2,Ext2);
	int ret;
	if((ret=_stricmp(Path1,Path2)) != 0)	return ret;				// Primarily by path
	if((ret=_stricmp(Ext1,Ext2)) != 0)	return ret;				// Then by Extension
	return strcmpfn(Name1,Name2);
}


int Srt_Sdeci(const char *s1,const char *s2)	{return _stricmp(s2,s1);}

int Srt_Sdec(const char *s1,const char *s2)	{return strcmp(s2,s1);}

int Srt_Sinci(const char *s1,const char *s2)	{return _stricmp(s1,s2);}

int Srt_Sinc(const char *s1,const char *s2)	{return strcmp(s1,s2);}


int Srt_Iinc(const char *s1,const char *s2)	{
	int i1=0,i2=0;
	sscanf(s1,"%d",&i1);
	sscanf(s2,"%d",&i2);
	return (i1==i2) ? 0 : (i1>i2) ? 1 : -1;	
}

int Srt_Finc(const char *s1,const char *s2)	{
	float f1=0.0,f2=0.0;
	sscanf(s1,"%f",&f1);
	sscanf(s2,"%f",&f2);
	return (f1==f2) ? 0 : (f1>f2) ? 1 : -1;
}

int Srt_Idec(const char *s1,const char *s2)	{
	int i1=0,i2=0;
	sscanf(s2,"%d",&i1);
	sscanf(s1,"%d",&i2);
	return (i1==i2) ? 0 : (i1>i2) ? 1 : -1;	
}

int Srt_Fdec(const char *s1,const char *s2)	{
	float f1=0.0,f2=0.0;
	sscanf(s2,"%f",&f1);
	sscanf(s1,"%f",&f2);
	return (f1==f2) ? 0 : (f1>f2) ? 1 : -1;
}



void Insert(char **arr,char *p,int cnt,int (*cmp)(const char*, const char*)) {
	// cnt is the nels currently occupied before call
	int i;
	for(i=cnt; i > 0 && cmp(arr[i-1],p) > 0 ; --i) {
		arr[i]=arr[i-1];				// Make a space
	}
	arr[i]=p;
}

AVSValue __cdecl RT_TxtSort(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *s1		= args[0].AsString();						// 1st string
	const int srtmode	= args[1].AsInt(0);
	static	int (*routs[])(const char *,const char *)= {
		Srt_Sinci,	Srt_Sinc,	Srt_Sdeci,	Srt_Sdec,
		Srt_FNinc,	Srt_FNinc2,	Srt_FNdec,	Srt_FNdec2,
		Srt_Iinc,	Srt_Finc,	Srt_Idec,	Srt_Fdec
	};
	
	if((srtmode<0)||(srtmode>=12))
		env->ThrowError("RT_TxtSort: Illegal sort mode.");
	int (*cmp)(const char *,const char *) = routs[srtmode];

	const char *s;
	int tot,c;
	int lines=0;
	for(s=s1 ; c=*s++ ;) {											// Count lines and find end
		if(c== '\n' || c == '\r') {
			if(c == '\n') {
				if(*s == '\r')
					++s;
			} else if(c == '\r') {
				if(*s == '\n')
					++s;
			}
			++lines;
		}
	}
	--s;															// back up to nul
	tot=int(s-s1+1);														// len of string, INCL nul
	if(s>s1 && s[-1] != '\n' && s[-1] != '\r') {					// if not eol term add 1 for n/l incr lines
		++tot;
		++lines;
	}

	if(lines<=1)
		return	s1;													// Nothing to sort, return source string

	char *p,*pbf = new char[tot];									
	if(pbf==NULL) {
		env->ThrowError("RT_TxtSort: Cannot allocate memory 1");
	}
	char *o,*obf = new char[tot];									
	if(obf==NULL) {
		delete [] pbf;
		env->ThrowError("RT_TxtSort: Cannot allocate memory 2");
	}
	char *a;
	char **arr = new char*[lines];									
	if(arr==NULL) {
		delete [] obf;
		delete [] pbf;
		env->ThrowError("RT_TxtSort: Cannot allocate memory 3");
	}
	int i;
	for(i=0,a=p=pbf,s=(char*)s1 ; ;) {
		c=*s;
		if(c== '\n' || c == '\r' || c == '\0') {
			*p++ = '\0';
			Insert(arr,a,i,cmp);
			if(c == '\n') {
				if(*(++s) == '\r')
					++s;
			} else if(c == '\r') {
				if(*(++s) == '\n')
					++s;
			}
			if(*s == '\0')
				break;
			++i;
			a=p;
		} else {
			*p++ = c;
			++s;
		}
	}
	for(o=obf,i=0;i<lines;++i) {
		a=arr[i];
		for(;*o++=*a++;);
		o[-1]='\n';		
	}
	AVSValue ret =	env->SaveString(obf,int(o-obf));						// Get pointer to Avisynth
	delete [] obf;													
	delete [] arr;
	delete [] pbf;
	return ret;	
}

