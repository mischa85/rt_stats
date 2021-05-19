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

#include "DDigit.h"												// Can only include in one place, (not in RT_Stats.h)

class RT_Stats : public GenericVideoFilter {   
private:
	void __stdcall DrawStr(PVideoFrame &dst,int x,int y,const char *s) {
		x *= 10;  y *= 20;		// conv char to pixel coords
		DDigitS(vi,dst,x,y,-1,true,false,s);
	}
public:
	RT_Stats(PClip _child,IScriptEnvironment* env) : GenericVideoFilter(_child) {
		# ifdef AVISYNTH_PLUGIN_25
			if(vi.IsPlanar() && vi.pixel_type != 0xA0000008) {	// Planar but NOT YV12, ie Avisynth v2.6+ ColorSpace
				// Here Planar but NOT YV12, If v2.5 Plugin Does NOT support ANY v2.6+ ColorSpaces
				env->ThrowError("RT_Stats: show version unsupported for colorSpace in v2.5 plugin");
			}
		# endif
	};
	~RT_Stats(){};
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {
		n  = (n<0)	? 0 : (n>= vi.num_frames) ?vi.num_frames -1:n;		// Range limit frame n
		PVideoFrame dst = child->GetFrame(n, env);						// get read only source frame n
		env->MakeWritable(&dst);
		char bf[64];
		char beta[16];
		beta[0]='\0';
		if(VERSION_BETA > 0) {
			sprintf(beta,"Beta%02d",VERSION_BETA);
		}
		sprintf(bf,"%d ] \a!RT_Stats\a-: v%.2f%s - " VERSION_DATE " - By StainlessS",n, VERSION_NUMBER,beta);
		DrawStr(dst,0,0,bf);
		return dst;
	};
};

AVSValue __cdecl Create_RT_Stats(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new RT_Stats(args[0].AsClip(),env);
}

class RT_Subtitle : public GenericVideoFilter {
	char *string;
	const int align;
	const int x;
	const int y;
	const bool expx;
	const int lines;
	const int ytop;
public:
	RT_Subtitle(PClip _child,char *_s,int _align,int _x,int _y,int _lines,int _yt,bool _expx,IScriptEnvironment* env);
	~RT_Subtitle();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

RT_Subtitle::RT_Subtitle(PClip _child,char *_s,int _align,int _x,int _y,int _lines,int _yt,bool _expx,IScriptEnvironment* env) :
	GenericVideoFilter(_child),string(_s),align(_align),x(_x),y(_y),lines(_lines),ytop(_yt),expx(_expx)	{}

RT_Subtitle::~RT_Subtitle() {
	if(string != NULL) {delete [] string;string = NULL;}
}

PVideoFrame __stdcall RT_Subtitle::GetFrame(int n, IScriptEnvironment* env) {
	const int num_frames = vi.num_frames;
	n = (n<0) ? 0 : (n>=num_frames) ? num_frames - 1 : n;
	PVideoFrame dst = child->GetFrame(n, env);
	env->MakeWritable(&dst);
	const int WW=vi.width;
	const int HH=vi.height;
	const char *s=string;
	const unsigned char *se=(unsigned char*)s;					// unsigned
	int yt=ytop;


	bool rightAligned=(align==9||align==6||align==3);
	bool midAligned=(align==8||align==5||align==2);
	int ccode=-1;
	int nextccode=-1;
	while(*se) {												// while, some to do
		int c;
		int sub=0;
		for(s=(char*)se; c = *se,c!=0 && c!='\r'; ++se) {	// Find end+1 of printable
			if(c=='\a') {
				int cc=se[1];
				if(cc >= 'a' && cc <= 'v')
					cc -= ('a' - 'A');				// Upper Case
				char *colstr="0123456789ABCDEFGHIJKLMNOPQRSTUV-!*";
				char * ccp=colstr;
				while(*ccp && *ccp != cc)
					++ccp;
				if(*ccp) {
					++se; // Color control code parsed with valid arg, skip ctrl code.('\a')
					nextccode=int(ccp - colstr);
					if(nextccode>31) {
						if(cc==33)
							nextccode=-2;			// '!' = ddigit hilite
						else
							nextccode=-1;			// '-' = ddigit default color, or original starting color
					}
					sub+=2;										// exclude from count when positioning
				}
			}
		}
		int llen = int((char*)se - s);							// Len of string
		int viewlen=llen-sub;									// visible chars
		if (viewlen && yt<HH && yt+20>=0) {						// Some to print
			int xt;
			if(!expx) {
				xt=(x==WW)? WW-(viewlen*10):(x== -1)?(WW/2)-(viewlen*10/2):x;
			} else {
				xt=(rightAligned)? (WW)+x-(viewlen*10) \
				   :(midAligned)   ? (WW/2)+x-(viewlen*10/2):x;
			}
			if(xt<WW && xt+viewlen*10>=0 )	{				// Clip non on-screen
				DDigitS(vi,dst,xt,yt,ccode,true,false,s,true,llen);
			}
		}
		yt+=20;							// Newline, down 1
		if(c!=0)
			++se;						// skip /r (outer loop detects nul sentinel)
		ccode=nextccode;				// color code, continued from previous line
	}
	return dst;
}

AVSValue __cdecl Create_RT_Subtitle(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_Subtitle: ";
	PClip child	=	args[0].AsClip();	
	const VideoInfo vi = child->GetVideoInfo();
	# ifdef AVISYNTH_PLUGIN_25
		if(vi.IsPlanar() && vi.pixel_type != 0xA0000008) {
			// Here Planar but NOT YV12, If v2.5 Plugin Does NOT support ANY v2.6+ ColorSpaces
			env->ThrowError("%sUnsupported for colorSpace in v2.5 plugin",myName);
		}
	# endif

	// esc, 0 NO esc codes processed.
	//		1=format string only,
	//		2=both format & data strings.
	int esc	= args[9].AsInt(1);
	if(esc < 0 || esc > 2)
		env->ThrowError("%sEsc range 0 -> 2 only",myName);

	const char *s	=	args[1].AsString();				// text
	int arrsz		=	args[2].ArraySize();

	enum {
		CHICKEN=64
	};


	// what size buffer we need ?
	int i,mem=int(strlen(s) + 1 + CHICKEN);
	for(i=0;i<arrsz;++i) {	
		if(args[2][i].IsString()) {
			const char *st=args[2][i].AsString();
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
	while(*r) {
		if(*r=='%') {
			++r;
			if(*r=='\0') {
				*p++ ='%';
			} else if(*r=='%') {
				*p++=*r++;				// replace escaped double % with single
			} else {
				if(ix>=arrsz) {
					delete [] pbuf;
					env->ThrowError("%s: Expecting data arg (%d)",myName,ix+1);
				}
				char *tp=ptem;
				*tp++='%';
				if(*r=='-' || *r=='+' || *r=='0' || *r==' ' || *r=='#')		// flags
					*tp++=*r++;
				if(*r=='*') {										// int holds length
					t=args[2][ix].IsBool()		    ?1: \
							args[2][ix].IsString()	?2: \
							args[2][ix].IsInt()		?3: \
							args[2][ix].IsFloat()	?4: \
							0;
					if(t!=3) {
						delete [] pbuf;
						env->ThrowError("%sUnsupported data type, Expecting Width as Int (%d)",myName,ix+1);
					}
					tp+=sprintf(tp,"%d",args[2][ix].AsInt());
					++r;											// skip '*'
					++ix;											// next data
				} else {
					while(*r>='0' && *r<='9') {
						*tp++ = *r++;
					}
				}
				if(*r=='.') {
					*tp++ =  *r++;				// precision prefix
					if(*r=='*') {										// int holds length
						t=args[2][ix].IsBool()			?1: \
								args[2][ix].IsString()	?2: \
								args[2][ix].IsInt()		?3: \
								args[2][ix].IsFloat()	?4: \
								0;
						if(t!=3) {
							delete [] pbuf;
							env->ThrowError("%sUnsupported data type, Expecting Precision as Int (%d)",myName,ix+1);
						}
						tp+=sprintf(tp,"%d",args[2][ix].AsInt());
						++r;											// skip '*'
						++ix;											// next data
					} else {
						while(*r>='0' && *r<='9') {
							*tp++ = *r++;
						}
					}
				}
				t=args[2][ix].IsBool()		    ?1: \
						args[2][ix].IsString()	?2: \
						args[2][ix].IsInt()		?3: \
						args[2][ix].IsFloat()	?4: \
						0;
				// type
				if(	(*r=='c' ) || (*r=='C' ) ||											// char as int
					(*r=='d' || *r=='i') ||												// int
					(*r=='o' || *r=='u' || *r=='x' || *r=='X'))  {						// unsigned int
					if(t!=3) {
						int tmpc=*r;
						delete [] pbuf;
						env->ThrowError("%sType='%c', Expecting Int data (%d)",myName,tmpc,ix+1);
					}
					*tp++=*r++;
					*tp='\0';
					p+=sprintf(p,ptem,args[2][ix].AsInt());
					++ix;																// next data
				} else if(*r=='e' || *r=='E' || *r=='f' || *r=='g' || *r=='G') {		// double
					if(t!=4&&t!=3)	{
						int tmpc=*r;
						delete [] pbuf;
						env->ThrowError("%sType='%c', Expecting Float (%d)",myName,tmpc,ix+1);
					}
					*tp++=*r++;
					*tp='\0';
					p+=sprintf(p,ptem,args[2][ix].AsFloat());
					++ix;																// next data
				} else if((*r=='s')||(*r=='S')) {													// string
					if(t!=2&&t!=1) {
						delete [] pbuf;
						env->ThrowError("%sType='s', Expecting String (%d)",myName,ix+1);
					}
					*tp++=*r++;
					*tp='\0';
					if(t==1) {	// Bool
						p+=sprintf(p,ptem,args[2][ix].AsBool()?"True":"False");
					} else {    // String
						p+=sprintf(p,ptem,args[2][ix].AsString());
					}
					++ix;																// next data
				} else {
					int tmpc=*r;
					delete [] pbuf;
					env->ThrowError("%sUnknown format type '%c' (%d)",myName,tmpc,ix+1);
				}
			}			
		} else if(*r == '\\' && esc == 1 && r[1] > ' ') {
			++r;
			c=*r;
			// afnrt
			switch (c) {
			case '\\' :	*p++=*r++;				break;		// replace double backslash with single backslash
			case 'n' :	++r;	*p++='\n';		break;		// \n, dont convert to \r just yet
			case 'r' :	++r;	*p++='\r';		break;		// \r 
			case 't' :	++r;		*p++=' ';	break;		// TAB to single SPACE
			case 'f' :	++r;		*p++='\f';	break;		// Formfeed to Forward 1 char
			case 'a' :
				++r;										// past 'a'
				c=*r;										// get possible color number
				if(c>='A' && c<='Z') c+= ('a'-'A');
				if ((c>='0' && c<='9')||(c>='a' && c<='v')||(c=='-'||c=='*'||c=='!')) {
					*p++='\a';								// convert alarm '\a' to Color code
					*p++=*r++;								// and copy color number
				} else {
					// Check if FOLLOWING programmed color code '%c' or "%s", will be inserted
					if(c == '%' && (r[1]=='c' || r[1]=='C' || r[1]=='s' || r[1]=='S')) {
						*p++='\a';	// insert alarm color control code, programmed color code coming next interation
					} else {
						*p++='\\';
						*p++='a';
					}
				}
				break;
			default :	*p++='\\';	*p++=*r++;	break;		// anything else we copy backslash and whatever follows
			}
		} else {
			*p++=*r++;
		}
	}
	*p=0;											// nul term

	if(ix<arrsz) {
		delete [] pbuf;
		env->ThrowError("%Unexpected data arg (%d)",myName,ix+1);
	}

	// Rescan to eg convert "\r" and "\n" to '\r'. Must do AFTER above, convert \n\r pair HERE ONLY
	p=pbuf;
	r=(const unsigned char*)p;
	int lines = 0;
	while(*r) {
		c=*r;
		if(c=='\\' && esc == 2 && r[1] > ' ') {
			// afnrt
			++r;
			c=*r;
			if(c=='\\') {
				*p++=*r++;						// replace double slash with single
			} else {
				if(c=='n'||c=='r') {
					if(r[1]=='\\') {
						int c2=r[2];
						if((c=='r' && c2=='n') || (c=='n' && c2=='r')) 
							r+=2;				// skip pair "\r\n"
					}
					*p++='\r';					// convert '\n' to carriage return
					++r;
					++lines;
				} else if(c=='t') {
					*p++=' ';					// convert TAB '\t' to single SPACE
					++r;
				} else if(c=='f') {
					*p++='\f';					// convert formfeed '\f' to forward SPACE
					++r;
				} else if(c=='a') {
					c=r[1];
					if(c>='A' && c<='Z') c+= ('a'-'A');
					if ((c>='0' && c<='9')||(c>='a' && c<='v')||(c=='-'||c=='*'||c=='!')) {
						*p++='\a';				// convert alarm '\a' to Color code
						++r;					// skip color code
						*p++=*r++;				// and copy color number
					} else {
						*p++='\\';
						*p++=*r++;						
					}
				} else {
					*p++='\\';
					*p++=*r++;
				}
			}
		} else if(c < ' ') {
			switch(c) {
			case '\n':
				++r;									// Skip '\n'
				if(*r=='\r')	++r;					// Skip pair
				*p++='\r';								// convert NL to carriage return
				++lines;
				break;
			case '\r':
				++r;
				if(*r=='\n')	++r;					// Skip pair
				*p++='\r';
				++lines;
				break;
			case '\t': *p++=' '; ++r;					// convert TAB to single SPACE
				break;
			case '\b': ++r;								// delete BACKSPACE
				break;
			case '\a': 
			case '\f': 
				*p++ = *r++;							// Copy Forward space and color control code
				break;
			default:
				++r;
				*p++ = ' ';				
			}
		} else if(c >= 224) {
			++r;
			*p++ = ' ';
		} else {
			*p++=*r++;
		}
	}
	*p=0;											// nul term

	int align	=	args[3].AsInt(0x80000000);		// align
	int	x		=	args[4].AsInt(0x80000000);		// x
	int	y		=	args[5].AsInt(0x80000000);		// y
	bool vcent	=	args[6].AsBool(false);			// vcent
	bool expx	=	args[7].AsBool(false);			// expx
	bool expy	=	args[8].AsBool(false);			// expy
	int height = vi.height;
	int width  = vi.width;

	if(!expx && align==0x80000000)			align=(x==-1) ? 8 : 7;
	if(align < 1 || align > 9)	align = 7;

	if(x==0x80000000) {
		switch (align) {
		case 2: case 5: case 8: x = -1; break;
		case 3: case 6: case 9: x = width; break;
		default: x = 0; break; }
	}
	if(y==0x80000000) {
		switch (align) {
		case 1: case 2: case 3: y = height; break;
		case 4: case 5: case 6: y = -1; break;
		default: y = 0; break; }
	}

	if(p>pbuf && p[-1] !='\r')		++lines;		// non '\n' terminated line still counts
	int yt;
	if(expy==false) {
		yt= \
		(y==height) ? height-(lines*20) : \
		(y== -1) ? (vcent?height/2-(lines*20/2):height/2-14) : \
		y;
	} else {
		yt=y;
	}
	char *str = new char[p-pbuf + 1];
	if(str==NULL) {
		delete [] pbuf;
		env->ThrowError("RT_Subtitle: Cannot allocate memory");
	}
	strcpy(str,pbuf);
	delete [] pbuf;
	return new RT_Subtitle(child,str,align,x,y,lines,yt,expx,env);
}

class RT_GraphLink : public GenericVideoFilter {
	PClip			*	rClips;
	int					numClips;
public:
	RT_GraphLink(PClip _child,AVSValue _Rx,AVSValue _Bx,IScriptEnvironment* env) : GenericVideoFilter(_child) {
		rClips	= NULL;
		numClips= 0;
		int cn = _Rx.ArraySize();
		int bn = _Bx.ArraySize();
		if(bn>cn)
			bn=cn;											// limit bool count to clip count (at most)
		if(cn>0) {
			int i,cnt = 0;
			for(i=0;i<cn;++i) {								// count how many to do
				if(i>=bn || _Bx[i].AsBool())
					++cnt;
			}
			if(cnt>0) {										// Some to do ?
				if(!(rClips=new PClip[cnt])) {
					env->ThrowError("RT_GraphLink: Cannot allocate memory");
				}
				int ix=0;
				for(i=0;i<cn;++i) {
					if(i>=bn || _Bx[i].AsBool()) {
						rClips[ix++] = _Rx[i].AsClip();
					}
				}
				numClips = cnt;								// For GetFrame()
			}
		}
	}
	~RT_GraphLink(){if(rClips)	{delete[] rClips;	rClips	=NULL;}}
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {
		int n_unlim = n;													// remember original n
		n  = (n<0)	? 0 : (n>= vi.num_frames) ?vi.num_frames -1:n;			// Range limit frame n
		PVideoFrame frm;
		for(int i=0;i<numClips;++i) {
		    const VideoInfo &r_vi = rClips[i]->GetVideoInfo();
			if (n_unlim>=0 && n_unlim<r_vi.num_frames) {	// Only access totally valid frames for the forced clip.
				frm = rClips[i]->GetFrame(n_unlim, env);	// Get Forced clips frame n
			}
		}
		frm = child->GetFrame(n, env);							// Get Real source frame n
		return frm;
	};
};

AVSValue __cdecl Create_RT_GraphLink(AVSValue args, void* user_data, IScriptEnvironment* env) {

	return new RT_GraphLink(args[0].AsClip(),args[1],args[2],env);
}
