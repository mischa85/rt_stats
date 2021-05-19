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

int MatchPat(const char *s,const char *p,int insig) {
	/*
		len = MatchPat(s,p,insignif);	String pattern match (anchored)
		int				length;			Length of matching string.
		const char	*	s;				Search string.
		const char	*	p;				Pattern string.
		int				insig;			0 = case significant, 1 = insignificant.

		Returns the length of the matching string or 0 if full match failed.
		
		Matches DOS * and ? chars in pat
	*/
	const char	*str=s;
	char	c,c2;
	while(c=*(p++)) {	// Outer, step through pattern string, inner by search string
		switch(c) {
			case '*':for(;((*s!=0)&&(MatchPat(s,p,insig)==0));s++);break;
			case '?':if(*(s++)==0)return(0);break;
			default: // Not wildcard char
				c2=*s++;
				if(insig) {				// Case insignificant
					if(c>='a'  && c  <= 'z')	c  = c  - ('a'-'A');
					if(c2>='a' && c2 <= 'z')	c2 = c2 - ('a'-'A');
				}
				if(c != c2)return(0);	// Not full match
				break;
		}
	}
	/* remove next line for extent of match */
	/* else return len only if full str matches pattern */
	if(*s)	return(0);				/* Not full match */
	else	return(int)(s-str);		/* len of match, WARNING, RETURN int NOT size_t */
}

int MatchMultipat(const char *s,const char *p) {
	// Match s with multiple wildcard patterns (* or ?) in p, pipe '|' separated.
	char patbuf[PATH_MAX];
	while(*p) {
		char *d = patbuf;
		while(*p && *p!='|')
			*d++ = *p++;										// Copy pattern to temp buf
		*d='\0';												// Nul term
		if(*p) ++ p;											// Skip pipe separator
		d=patbuf;
		if(*d) {												// not lone pipe
			if(MatchPat(s,d,1))									// case insignificant matching
				return 1;										// Full match
		}
	}
	return 0;													// No match
}

AVSValue __cdecl RT_WriteFileList(AVSValue args, void* user_data, IScriptEnvironment* env) {
	env->ThrowError("NOT IMPLEMENTED YET");
	/*
	const char *infilename	= args[0].AsString();
	const char *ofn			= args[1].AsString();
	const bool append		= args[2].AsBool(false);
	char *omode=(append)?"a+t":"wt";
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	char Path[PATH_MAX];										// Path up to and incl last slash
	char Name[PATH_MAX];										// Name up to and incl last '.'
	char Extension[PATH_MAX];									// User supplied extension with pipe separated wildcards
	char ifn[PATH_MAX];											// filename used in system call to FindFirstFile
	char GotName[PATH_MAX];
	const char *s;
	char *d,*Ext,c;

	SplitFn(infilename,Path,Name,Extension);

	int pipe = 0;
	for(d=Extension ; c=*d++ ;) {
		if(c == '|') {
			pipe=1;												// Flag pipe in entension
			break;
		}
	}
	// Now make filename from parts, to be used in FindFirstFile
	for(s=Path, d=ifn;*d++=*s++;);								// strcpy path
	--d;														// back 1, point at nul	
	for(s=Name;*d++=*s++;);										// strcat name node
	--d;														// back 1, point at nul

	if(pipe) {
		*d++ = '.';												// If looking for multipats match all extensions
		*d++ = '*';												// If looking for multipats match all extensions
		*d++ = '\0';
	} else {
		for(s=Extension;*d++=*s++;);							// strcat extension, ifn same as infilename.
	}
	hFind = FindFirstFile(ifn, & FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 	{
		FindClose(hFind);
		return 0;												// no files found
	} 
	FILE * fp;
	// we use write in text mode, let C insert '\r'.
	if((fp=fopen(ofn, omode ))==NULL) {
		FindClose(hFind);
		return -1;												// Cannot output file
	}
	Ext=Extension;
	if(*Ext=='.')
		++Ext;
	int n=0;
	do {
		if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0) { // ignore directories
			if(pipe) {											// match file extension with multipat extension
				s=FindFileData.cFileName;
				const char *ex=s;
				for(; c=*s++ ;) {								// point 1 after last dot, ie extension
					if(c == '.')
						ex = s;									// Point 1 after last '.'
				}
				if(!MatchMultipat(ex,Ext))
					continue;									// no match with piped wildcards
			}
			s=Path; d=GotName;
			while(*d++=*s++);									// strcpy
			--d;												// back 1 to point a nul
			s=FindFileData.cFileName;
			while(*d++=*s++);									// strcpy == strcat name node
			--d;												// back 1 to point a nul
			*d++	= '\n';										// EOL
			*d		= '\0';
			++n;
			if(fputs( GotName, fp ) == EOF) {					// write gotten filename to o/p file
				fclose(fp);
				FindClose(hFind);
				return -1;										// Problem writing file.
			}
		}
	} while(FindNextFile(hFind, &FindFileData));
	fclose(fp);
	FindClose(hFind);
	*/
	int n=0;
	return n;													// return filenames in output file
}



AVSValue __cdecl RT_FileQueryLines(AVSValue args, void* user_data, IScriptEnvironment* env) {
	enum{BFSZ=1024};
	char bf[BFSZ+2];														// with a little spare
	const char *filename=args[0].AsString();
	FILE *fp;	
	long flen,chk,rd;
	// we use read in binary mode so C does not mess with file size / seek etc.
	if(!(fp = fopen(filename, "rb"))) {env->ThrowError("RT_FileQueryLines: Cannot Open file %s",filename);}	
	if((fseek(fp, 0, SEEK_END)!=0) || ((flen=ftell(fp)) == -1L)) {
		fclose(fp);
		env->ThrowError("RT_FileQueryLines: Cannot seek in file %s",filename);
	}
	rewind(fp);
	if ((chk=3)>flen)	chk=flen;
	if(chk==0) { fclose(fp); return 0;}										// Return 0, empty file
	rd = int(fread(bf, 1, chk, fp));
	if(rd != chk) {fclose(fp);env->ThrowError("RT_FileQueryLines: Cannot read from file %s",filename);}
	if(chk==3 && bf[0]==0xEF && bf[1]==0xBB && bf[2]==0xBF) {
		fclose(fp); env->ThrowError("RT_FileQueryLines: UTF-8 text files are not supported, "
		"re-save with ANSI encoding! : '%s'", filename);}
	if((chk>=2 && ((bf[0]==0xFF && bf[1]==0xFE) || (bf[0]==0xFE && bf[1]==0xFF)))) {
		fclose(fp); env->ThrowError("RT_FileQueryLines: Unicode text files are not supported, "
		"re-save with ANSI encoding! : '%s'", filename);}
	fclose(fp);
	// Now we use read in Text mode, will halt at 1st nul byte if binary file.
	if(!(fp = fopen(filename, "rt"))) {env->ThrowError("RT_FileQueryLines: Cannot reopen file %s",filename);}		
	int line = 0;
	while(fgets(bf, BFSZ,fp ) != NULL) {	// will return non n/l term last line : will halt at first Nul byte if bin file
		++line;
	}
	fclose(fp);
	return line;
}

AVSValue __cdecl RT_ReadTxtFromFile(AVSValue args, void* user_data, IScriptEnvironment* env) {
	enum{BFSZ=1024};
	char bf[BFSZ+2];														// with a little spare
	const char *filename=args[0].AsString();
	int	Lines=args[1].AsInt(0);												// Allow user set default=0
	int	Start=args[2].AsInt(0);												// Zero relative start
	if(Lines<0)	env->ThrowError("RT_ReadTxtFromFile: Lines Illegal\n");
	if(Start<0)	env->ThrowError("RT_ReadTxtFromFile: Start Illegal\n");
	FILE *fp;	
	unsigned char *p,*s,*pbf;
	long flen,chk,rd,line;
	// we use read in binary mode so C does not mess with file size / seek etc.
	if(!(fp = fopen(filename, "rb"))) {env->ThrowError("RT_ReadTxtFromFile: Cannot Open file %s",filename);}	
	if((fseek(fp, 0, SEEK_END)!=0) || ((flen=ftell(fp)) == -1L)) {
		fclose(fp);
		env->ThrowError("RT_ReadTxtFromFile: Cannot seek in file %s",filename);
	}
	rewind(fp);
	if ((chk=3)>flen)	chk=flen;
	if(chk==0) { fclose(fp); bf[0]=0; return env->SaveString(bf);}			// Return "" to Avisynth
	rd = int(fread(bf, 1, chk, fp));
	if(rd != chk) {fclose(fp);env->ThrowError("RT_ReadTxtFromFile: Cannot read from file %s",filename);}
	if(chk==3 && bf[0]==0xEF && bf[1]==0xBB && bf[2]==0xBF) {
		fclose(fp); env->ThrowError("RT_ReadTxtFromFile: UTF-8 text files are not supported, "
				"re-save with ANSI encoding! : '%s'", filename);}
	if((chk>=2 && ((bf[0]==0xFF && bf[1]==0xFE) || (bf[0]==0xFE && bf[1]==0xFF)))) {
		fclose(fp); env->ThrowError("RT_ReadTxtFromFile: Unicode text files are not supported, "
		"re-save with ANSI encoding! : '%s'", filename);}
	fclose(fp);
	// Now we use read in Text mode, will halt at 1st nul byte if binary file.
	if(!(fp = fopen(filename, "rt"))) {env->ThrowError("RT_ReadTxtFromFile: Cannot reopen file %s",filename);}		
	if((pbf = new unsigned char[flen+1])==NULL)					// +1, we may need to add '\n'
		{fclose(fp); env->ThrowError("RT_ReadTxtFromFile: Cannot allocate memory for %s",filename);}
	p=pbf;
	int End = (Lines <= 0) ? 0x7FFFFFFF : Start + Lines;
	line = 0;
	while(line < End && (fgets(bf, BFSZ,fp ) != NULL)) {					// will halt at first Nul byte if bin file
		if(line >= Start) {
			s = (unsigned char *)bf;										// start of input buffer
			while(*s) {
				*p++=*s++;
			}
		}
		++line;
	}
	fclose(fp);
	if(pbf==p) {													// zero len file or invalid line number
		delete [] pbf;												// Delete original buffer
		env->ThrowError("RT_ReadTxtFromFile: Invalid Line Number");
	}
	if(p[-1]!='\n')
		*p++ = '\n';											// Ensure last line is '\n' terminated, same as other lines
	AVSValue ret =	env->SaveString((const char*)pbf,int(p - pbf));	// Get pointer to Avisynth saved string
	delete [] pbf;												// Delete original buffer
	return ret;													// Return Avisynth's copy to Avisynth
}




AVSValue __cdecl RT_FileDelete(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *fn		= args[0].AsString();
	int e =remove(fn);
	if(e) {
		if(errno == EACCES) {
			e = -1;									// access denied / read only
		} else if(errno == ENOENT) {
			e = -2;									// Path not found OR is Directory
		} else {
			e = -3;									// Unknown error.
		}
	}
	return e;
}




AVSValue __cdecl RT_FileRename(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *oldfn	= args[0].AsString();
	const char *newfn	= args[1].AsString();
	int e =rename(oldfn,newfn);
	if(e) {
		if(errno == EACCES) {
			e = -1;									// access denied / read only
		} else if(errno == ENOENT) {
			e = -2;									// Path not found OR is Directory
		} else if(errno == EINVAL) {
			e = -3;									// Invalid name.
		} else {
			e = -4;									// Unknown error.
		}
	}
	return e;
}





AVSValue __cdecl RT_FileFindStr(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char* myName="RT_FileFindStr: ";
	const char  *filename	= args[0].AsString();
	const char  *subs		= args[1].AsString();
	const bool  sig			= args[2].AsBool(true);
	const int   pos			= args[3].AsInt(1) - 1;
	const int   start		= args[4].AsInt(0);
	const int   lines		= args[5].AsInt(0);		// Default is ALL
	if(pos<0)	return -1;	// not in string
	const char *p = subs;
	int end = (lines <= 0) ? 0x7FFFFFFF : start + lines;
	int c;
	while(c=*p, c!='\0' && c!='\n' && c!='\r') ++p;		// find len of sub string, excluding \r or \n
	int subslen=int(p-subs);
	if(subslen==0 || start < 0)
		return -1;					// Empty subs always returns not found (-1)

	enum{BFSZ=1024};
	char bf[BFSZ+2];														// with a little spare
	FILE *fp;	
	long flen,chk,rd;
	// we use read in binary mode so C does not mess with file size / seek etc.
	if(!(fp = fopen(filename, "rb"))) {env->ThrowError("%sCannot Open file %s",myName,filename);}	
	if((fseek(fp, 0, SEEK_END)!=0) || ((flen=ftell(fp)) == -1L)) {
		fclose(fp);
		env->ThrowError("%sCannot seek in file %s",myName,filename);
	}
	rewind(fp);
	if ((chk=3)>flen)	chk=flen;
	if(chk==0) { fclose(fp); return -1;}										// Return -1, empty file, subs not found
	rd = int(fread(bf, 1, chk, fp));
	if(rd != chk) {fclose(fp);env->ThrowError("%sCannot read from file %s",myName,filename);}
	if(chk==3 && bf[0]==0xEF && bf[1]==0xBB && bf[2]==0xBF) {
		fclose(fp); env->ThrowError("%sUTF-8 text files are not supported, "
		"re-save with ANSI encoding! : '%s'",myName, filename);}
	if((chk>=2 && ((bf[0]==0xFF && bf[1]==0xFE) || (bf[0]==0xFE && bf[1]==0xFF)))) {
		fclose(fp); env->ThrowError("%sUnicode text files are not supported, "
		"re-save with ANSI encoding! : '%s'",myName, filename);}
	fclose(fp);
	// Now we use read in Text mode, will halt at 1st nul byte if binary file.
	if(!(fp = fopen(filename, "rt"))) {env->ThrowError("%sCannot reopen file %s",myName,filename);}		

	int n = 0;
	while(n != start && fgets(bf, BFSZ,fp ) != NULL) {	// will return non n/l term last line : will halt at first Nul byte if bin file
		++n;
	}
	if(n != start)	{fclose(fp);return -1;}		// start line not found

	while(fgets(bf, BFSZ,fp ) != NULL && n < end) {
		int slen = int(strlen(bf));
		slen -= pos;											// - start position to search = remaining len
		if(slen > 0) {											// remaining string at least as long as subs
			if (StrStrC(bf+pos,subs,sig, slen, subslen) != NULL) {
				fclose(fp);
				return n;										// GOT IT, return line number, 0 rel
		}	}
		++n;
	}
	fclose(fp);
	return -1;
}


AVSValue __cdecl RT_TxtWriteFile(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *s,*is		= args[0].AsString();
	const char *ofn			= args[1].AsString();
	const bool append		= args[2].AsBool(false);
	const char *omode=(append)?"a+t":"wt";
	FILE * fp;
	// we use write in text mode, let C insert '\r'.
	if((fp=fopen(ofn, omode ))==NULL) {return -1;}					// Cannot output file
	int c,lines = 0;
	s=is;
	do {
		c=*s;
		if(c=='\n' || c=='\r' || c == '\0') {
			if(is<s) {
				if(fwrite(is,s-is,1,fp)!=1) {
					fclose(fp);
					return -1;										// write file error
				}
			}
			if(c=='\n') {
				++s;
				if(*s=='\r')
					++s;
			} else if(c=='\r') {
				++s;
				if(*s=='\n')
					++s;
			}
			if(is<s) {
				if (fputc('\n',fp)==EOF) {
					fclose(fp);
					return -1;											// write file error
				}
				++lines;
			}
			is=s;
		} else {
			++s;
		}
	} while (*is);													// !!! Exit when 1st char in string is end
	fclose(fp);
	return lines;
}





AVSValue __cdecl RT_WriteFile(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_WriteFile: ";
	const char *ofn		= args[0].AsString();
	const char *fmt		= args[1].AsString();
	AVSValue datn		= args[2];							// data
	const bool append	= args[3].AsBool(false);

	int arrsz			= datn.ArraySize();

	enum {
		CHICKEN=64
	};

	// what size buffer we need ?
	int i,mem=int(strlen(fmt) + 1 + CHICKEN);
	for(i=0;i<arrsz;++i) {	
		if(datn[i].IsString()) {
			const char *st=datn[i].AsString();
			mem += int(strlen(st) + 1 + CHICKEN);
		} else {
			mem += 8 + CHICKEN;		// no particular reason why so big, just chicken factor.
		}
	}

	char *pbuf = new char[(mem+1)*2];
	if(pbuf==NULL)
		env->ThrowError("%sCannot allocate memory",myName);

	char *ptem=pbuf+(mem+1);			// temp buffer
	
	const unsigned char* r= (const unsigned char*)fmt;
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
					t=datn[ix].IsBool()		    ?1: \
							datn[ix].IsString()	?2: \
							datn[ix].IsInt()		?3: \
							datn[ix].IsFloat()	?4: \
							0;
					if(t!=3)	{
						delete [] pbuf;
						env->ThrowError("%sUnsupported data type, Expecting Width as Int (%d)",myName,ix+1);
					}
					tp+=sprintf(tp,"%d",datn[ix].AsInt());
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
						t=datn[ix].IsBool()			?1: \
								datn[ix].IsString()	?2: \
								datn[ix].IsInt()		?3: \
								datn[ix].IsFloat()	?4: \
								0;
						if(t!=3) {
							delete [] pbuf;
							env->ThrowError("%sUnsupported data type, Expecting Precision as Int (%d)",myName,ix+1);
						}
						tp+=sprintf(tp,"%d",datn[ix].AsInt());
						++r;											// skip '*'
						++ix;											// next data
					} else {
						while(*r>='0' && *r<='9') {
							*tp++ = *r++;
						}
					}
				}
				t=datn[ix].IsBool()		    ?1: \
						datn[ix].IsString()	?2: \
						datn[ix].IsInt()		?3: \
						datn[ix].IsFloat()	?4: \
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
					p+=sprintf(p,ptem,datn[ix].AsInt());
					++ix;																// next data
				} else if(*r=='e' || *r=='E' || *r=='f' || *r=='g' || *r=='G') {		// double
					if(t!=4&&t!=3)	{
						int tmpc=*r;
						delete [] pbuf;
						env->ThrowError("%sType='%c', Expecting Float (%d)",myName,tmpc,ix+1);
					}
					*tp++=*r++;
					*tp='\0';
					p+=sprintf(p,ptem,datn[ix].AsFloat());
					++ix;																// next data
				} else if((*r=='s')||(*r=='S')) {													// string
					if(t!=2&&t!=1)	{
						delete [] pbuf;
						env->ThrowError("%sType='s', Expecting String (%d)",myName,ix+1);
					}
					*tp++=*r++;
					*tp='\0';
					if(t==1) {	// Bool
						p+=sprintf(p,ptem,datn[ix].AsBool()?"True":"False");
					} else {    // String
						p+=sprintf(p,ptem,datn[ix].AsString());
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

	const char *omode=(append)?"a+t":"wt";
	FILE * fp;
	// we use write in text mode, let C insert '\r'.
	if((fp=fopen(ofn, omode ))==NULL) {						// Cannot output file
		delete [] pbuf;
		return -1;
	}
	int lines = 0;
	char *s,*is;
	s=is=pbuf;;
	do {
		c=*s;
		if(c=='\n' || c=='\r' || c == '\0') {
			if(is<s) {
				if(fwrite(is,s-is,1,fp)!=1) {
					delete [] pbuf;
					fclose(fp);
					return -1;										// write file error
				}
			}
			if(c=='\n') {
				++s;
				if(*s=='\r')
					++s;
			} else if(c=='\r') {
				++s;
				if(*s=='\n')
					++s;
			}
			if(is<s) {
				if (fputc('\n',fp)==EOF) {
					delete [] pbuf;
					fclose(fp);
					return -1;											// write file error
				}
				++lines;
			}
			is=s;
		} else {
			++s;
		}
	} while (*is);													// !!! Exit when 1st char in string is end
	delete [] pbuf;
	fclose(fp);
	return lines;
}

