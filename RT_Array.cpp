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

// --------------------------------------

	enum {
		ARRAY_ATTRIB	    = 1024,
		ARRAY_USERSTRINGS   = 10,
		ARRAY_USERSTRLEN    = 1024,
		ARRAY_OFFSET        = 32*1024,					
		ARRAY_ATTROFFSET	= sizeof(MYARR),
		ARRAY_USTROFFSET	= ARRAY_ATTROFFSET + sizeof(MYATEL)  * ARRAY_ATTRIB,
		ARRAY_SPARESTART	= ARRAY_USTROFFSET + ARRAY_USERSTRLEN   * ARRAY_USERSTRINGS,
		ARRAY_SPARE			= ARRAY_OFFSET - ARRAY_SPARESTART
	};

	#define RTAR	0x52415452					// 'RATR', 'RTAR' in memory (silly little endian i86)
	#define RTAVER	0x0000000B


static const char * __cdecl Arr_fn(const char *fn) {
	const char *s=fn;
	int c;
	for(;c=*fn;++fn) { 
		if((c=='\\' || c=='/') && (fn[1]!='\0'))
			s=fn+1;
	}
	return s;
}

// 	extern FILE * __cdecl ARR_Read_Header(const char *myName,const char *fn,const char *omd,MYARR &arr,IScriptEnvironment* env,char *retbf=NULL);

// WARNING, Default retbuf = NULL
FILE * __cdecl ARR_Read_Header(const char *myName,const char *fn,const char *omd,MYARR &arr,IScriptEnvironment* env,char *retbf) {
	char *eptr=NULL;
	char msg[1024]="";
	FILE * fp=NULL;
	if(*fn=='\0')								eptr="Empty filename";
	else if((fp=fopen(fn, omd ))==NULL)			eptr="Cannot open Array file";
	else if(fread(&arr,sizeof(arr),1,fp)!=1)	eptr="Reading Array file header";
	else if(arr.name!=RTAR)						eptr="Not an RT_Array file";
	else if(arr.ver!=RTAVER)					eptr="RT_Array wrong version";
	if(eptr!=NULL) {
		if(fp!=NULL) {fclose(fp); fp=NULL;}
		char *p = (retbf==NULL) ? msg : retbf;
		sprintf(p,"%sError, %s\n%s\n%s",myName,eptr,Arr_fn(fn),fn);
		if(retbf==NULL)
			env->ThrowError("%s",p);
	}
	return fp;
}


static char * __cdecl Arr_IxStr(char *bf,int dims,int ix1,int ix2,int ix3) {
	if(dims==1)			sprintf(bf,"%d",		ix1);
	else if(dims==2)	sprintf(bf,"%d,%d",		ix1,ix2);
	else if(dims==3)	sprintf(bf,"%d,%d,%d",	ix1,ix2,ix3);
	else *bf='\0';
	return bf;
}

// --------------------------------------

AVSValue __cdecl RT_ArrayAlloc(AVSValue args, void* user_data, IScriptEnvironment* env) {	
	const char * myName="RT_ArrayAlloc: ";
	const char * fn= args[0].AsString();
	if(*fn=='\0')	env->ThrowError("%sEmpty Filename",myName);
	char ebf[256]="";
	FILE * fp=NULL;
	size_t wr=0;

	int64_t MaxFileSz = 0xFFFFF000000LL ;												// 1TB - 1MB
	int fatvol = QueryFatVolume(fn);
	if(fatvol < 0)	env->ThrowError("%sCannot query Filesystem",myName);

	int64_t dfs = QueryDiskFreeSpace(fn) - 0x100000;									// minus 1MB
	if(dfs < 0)	env->ThrowError("%sCannot query DiskFreeSpace",myName);

	int64_t maxfs = (fatvol==1)? 0xFFF000000LL : MaxFileSz; 	// limit 4GB on FAT32
	int64_t maxcurdfs=std::min(maxfs,dfs);							// Max current space, Limit to free space available to user.

	MYARR arr;
	memset(&arr,0,sizeof(arr));
	const int type = args[1].AsInt(1);
	if(type < ARRAY_BOOL || type > ARRAY_DOUBLE)
		env->ThrowError("%sIllegal Type %d(0 -> 5)\n%s\n%s",myName,type,Arr_fn(fn),fn);
	static const int isize[]={1,4,4,256,1,8};
	const int elsz = (type==ARRAY_STRING)?args[5].AsInt(256):isize[type];
	if(elsz<=0 || elsz > (256 *1024))
		env->ThrowError("%sIllegal StringLenMax %d(1->256KB)\n%s\n%s",myName,elsz,Arr_fn(fn),fn);
	int i,d,err=0;
	for(i=3;--i>=0;) {
		arr.dim[i+1] = d = args[i+2].AsInt(0);						// Dimn
		if(d < 0)						err=i+1;
		if(d!=0 && arr.dim[0]==0)		arr.dim[0] = i + 1;			// max set dimension
	}
	if(arr.dim[0]==0)
		arr.dim[0]=1;					// at least 1 dimension, even if dims = 0,0,0 (non-allocated)
	char sbf[64]="";
	if(err)
		env->ThrowError("%sIllegal -ve Dim%d=%d : ARR(%s)\n%s\n%s",myName,
			err,arr.dim[err],
			Arr_IxStr(sbf,arr.dim[0],arr.dim[1],arr.dim[2],arr.dim[3]),
			Arr_fn(fn),fn);
	if(arr.dim[0]==3 && (arr.dim[2]==0))
		env->ThrowError("%sIllegal, 3 dimensional Dim2=0 : ARR(%s)\n%s\n%s",myName,
			Arr_IxStr(sbf,arr.dim[0],arr.dim[1],arr.dim[2],arr.dim[3]),
			Arr_fn(fn),fn);
	i=arr.dim[0] - 1;					// number of dimensions - 1 (mul[] only 0 -> 2)
	arr.mul[0]=arr.mul[1]=arr.mul[2]=1;
	int64_t m = arr.dim[i+1];			// Dimension of least sig valid index
	int64_t Dim1Bytes=1;
	for(;--i>=0;) {
		Dim1Bytes=m;
		arr.mul[i] =int(m);				// more sig index mul product of lesser sig dimensions
		m *= arr.dim[i+1];				// *= next most sig dim
	}
	Dim1Bytes*=elsz;

	int maxPossDim1= int(std::min<long long int>((MaxFileSz - ARRAY_OFFSET) / Dim1Bytes,0x7FFFFFFE0LL));	// max possible dim1 on any filesystem
	int maxfsDim1  = int(std::min<long long int>((maxfs  - ARRAY_OFFSET)	/ Dim1Bytes,0x7FFFFFFE0LL));	// max current filesystem
	int maxdfsDim1 = int(std::min<long long int>((maxcurdfs - ARRAY_OFFSET) / Dim1Bytes,0x7FFFFFFE0LL));	// max free space
	int maxDim1 = std::min(maxfsDim1,maxdfsDim1);

/*
	dprintf("dim[0]=%d",arr.dim[0]);
	dprintf("dim[1]=%d",arr.dim[1]);
	dprintf("dim[2]=%d",arr.dim[2]);
	dprintf("dim[3]=%d",arr.dim[3]);

	dprintf("mul[0]=%d",arr.mul[0]);
	dprintf("mul[1]=%d",arr.mul[1]);
	dprintf("mul[2]=%d",arr.mul[2]);
	dprintf("Dim1Bytes=%0LLd",Dim1Bytes);
	dprintf("maxPossDim1=%d ($%X)",maxPossDim1,maxPossDim1);
	dprintf("maxfsDim1  =%d ($%X)",maxfsDim1,maxfsDim1);
	dprintf("maxdfsDim1 =%d ($%X)",maxdfsDim1,maxdfsDim1);
	dprintf("maxDim1    =%d ($%X)",maxDim1,maxDim1);
*/

	if(Dim1Bytes > int64_t(256*1024*1024)) {
		if(arr.dim[0]==2)	strcpy(ebf,"Dim2 * elsz too big, exceeds 256MB");
		else				strcpy(ebf,"Dim2 * Dim3 * elsz too big, exceeds 256MB");
	} else if(arr.dim[1] > maxPossDim1)	sprintf(ebf,"Dim1(%d) Exceeds Dim1max(%d)",arr.dim[1],maxPossDim1);
	else if(arr.dim[1] > maxfsDim1)		sprintf(ebf,"Dim1(%d) Too Big for FileSystem",arr.dim[1]);					
	else if(arr.dim[1] > maxdfsDim1)	sprintf(ebf,"Dim1(%d) Not Enough Disk Space Available",arr.dim[1]);
	else {
		arr.name		= RTAR;
		arr.ver			= RTAVER;
		arr.offset		= ARRAY_OFFSET;
		arr.attriboffset= ARRAY_ATTROFFSET;
		arr.stroffset	= ARRAY_USTROFFSET;
		arr.type		= type;
		arr.elsz		= elsz;
		arr.dim1Bytes	= int(Dim1Bytes);
		arr.dim1max		= int(maxPossDim1);
		arr.attribs		= ARRAY_ATTRIB;
		arr.ustrings	= ARRAY_USERSTRINGS;
		arr.ustrlen		= ARRAY_USERSTRLEN;
		int bfsz = int(std::min<long long int>(1024*4,(ARRAY_OFFSET + arr.dim[1]*Dim1Bytes)));
		BYTE *bf = new BYTE[bfsz];
		if(bf == NULL) {
			strcpy(ebf,"Allocating memory buffer");
		} else {
			memset(bf,0,bfsz);
			if((fp=fopen(fn, "wb" ))==NULL)									strcpy(ebf,"Cannot create Array file");
			else {
				const int sods = int((ARRAY_OFFSET+Dim1Bytes*arr.dim[1]) / bfsz);
				const int odds = int((ARRAY_OFFSET+Dim1Bytes*arr.dim[1]) % bfsz);
				wr = 1;
				for(i=sods; wr==1 && --i >= 0; )
					wr=fwrite(bf,bfsz,1,fp);
				if(wr==1 && odds > 0)
					wr=fwrite(bf,odds,1,fp);
				if(wr!=1)													strcpy(ebf,"writing Array Init data");
				else {
					rewind(fp);
					if((wr=fwrite(&arr,sizeof(arr),1,fp))!=1)				strcpy(ebf,"writing Array file header");
				}
			}
			delete [] bf;
		}
	}
	if(fp != NULL)		fclose(fp);
	if(wr!=1)			env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,Arr_fn(fn),fn);
	return maxDim1;
}

AVSValue __cdecl RT_ArrayGetType(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArrayGetType: ";
	const char *fn		= args[0].AsString();
	MYARR arr;
	FILE * fp=ARR_Read_Header(myName,fn,"rb",arr,env);
	fclose(fp);
	return arr.type;
}


AVSValue __cdecl RT_ArrayTypeName(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArrayTypeName: ";
	const int	type	= args[0].AsInt();
	if(type < 0 || type > 5)
		env->ThrowError("%sError, Bad type number %d(0 -> 5)",myName,type);
	static char *names[] = {"Bool","Int","Float","String","Bin","Double"};
	return names[type];
}


AVSValue __cdecl RT_ArrayGetElSize(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArrayGetElSize: ";
	const char *fn		= args[0].AsString();
	MYARR arr;
	FILE * fp=ARR_Read_Header(myName,fn,"rb",arr,env);
	fclose(fp);
	return arr.elsz;
}

AVSValue __cdecl RT_ArrayGetDim1Max(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArrayGetDim1Max: ";
	const char *fn		= args[0].AsString();
	const bool current	= args[1].AsBool(true);
	MYARR arr;
	FILE * fp=ARR_Read_Header(myName,fn,"rb",arr,env);
	fclose(fp);
	int dim1max = arr.dim1max;
	if(current) {
		int64_t dfs = QueryMaxFileSize(fn);
		if(dfs < 0)	env->ThrowError("%sCannot query MaxFileSize",myName);
		dim1max=int(std::min<long long int>(((dfs / arr.dim1Bytes) + arr.dim[1]),0x7FFFFFFE0LL));
	}
	return  dim1max;
}


AVSValue __cdecl RT_ArrayGetDim(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArrayGetDim: ";
	const char *fn		= args[0].AsString();
	const int dim		= args[1].AsInt(0);
	if(dim > 3 || (dim < 0)) env->ThrowError("%sIllegal dimension %d (0 -> 3)\n%s\n%s",myName,dim,Arr_fn(fn),fn);
	MYARR arr;
	FILE * fp=ARR_Read_Header(myName,fn,"rb",arr,env);
	fclose(fp);
	return arr.dim[dim];
}


AVSValue __cdecl RT_ArrayGetAttrib(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArrayGetAttrib: ";
	const char *fn		= args[0].AsString();
	const int atix		= args[1].AsInt();
	if(atix < -1 || atix >= ARRAY_ATTRIB)
		env->ThrowError("%sInvalid Attrib index %d(-1 -> %d)\n%s\n%s",myName,atix,ARRAY_ATTRIB-1,Arr_fn(fn),fn);
	MYARR arr;
	FILE   * fp=ARR_Read_Header(myName,fn,"rb",arr,env);
	AVSValue ret;
	if(atix == -1) {
		fclose(fp);
		ret = arr.attribs;
	} else {
		MYATEL atel;
		if(fseek(fp, arr.attriboffset + atix * sizeof(atel),SEEK_SET)) {
			fclose(fp);
			env->ThrowError("%sError Seekinging Array attribute(%d)\n%s\n%s",myName,atix,Arr_fn(fn),fn);
		}
		size_t rd=fread(&atel,sizeof(atel),1,fp);
		fclose(fp);
		if(rd!=1)		env->ThrowError("%sError reading Array attribute(%d)\n%s\n%s",myName,atix,Arr_fn(fn),fn);
		// *** WARNING ***, Dont try to use ternary conditional (?:) below, compiler seems to always set ret to float.
		// Presumably, type assignment to ret done at compile time.
		if(atel.attype == ATTR_INT) {
			ret = atel.attrib.i;
		} else {
			ret = atel.attrib.f;
		}
	}
	return ret;
}


AVSValue __cdecl RT_ArraySetAttrib(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArraySetAttrib: ";
	const char *fn		= args[0].AsString();
	const int atixStart	= args[1].AsInt();
	const int arsz		= args[2].ArraySize();
	if(atixStart < 0 || atixStart >= ARRAY_ATTRIB) 
		env->ThrowError("%sError, Invalid Attrib Start index %d(0 -> %d)\n%s\n%s",
				myName,atixStart,ARRAY_ATTRIB-1,Arr_fn(fn),fn);
	if(arsz ==0)
		env->ThrowError("%sError, MUST provide list of Attrib's\n%s\n%s",myName,Arr_fn(fn),fn);
	int atixEnd = atixStart + arsz - 1;
	if(atixEnd >= ARRAY_ATTRIB)			
		env->ThrowError("%sError, Too Many Attrib's(Ends @%d, max idix=%d)\n%s\n%s",
			myName,atixEnd,ARRAY_ATTRIB-1,Arr_fn(fn),fn);
	MYARR arr;
	FILE * fp=ARR_Read_Header(myName,fn,"rb+",arr,env);
	char ebf[256]="";
	MYATEL *atel= new MYATEL[arsz];
	if(atel==NULL)			strcpy(ebf,"Cannot allocate mem buffer");
	if(ebf[0]=='\0') {
		for(int atix=atixStart;atix<=atixEnd;++atix) {
			int argix = atix-atixStart;
			AVSValue at	= args[2][argix];
			int argtype = (at.IsInt()) ? ATTR_INT : (at.IsFloat()) ? ATTR_FLOAT : -1;
			if(argtype < 0)	{
				sprintf(ebf,"%sError, @ArgArr(%d) Invalid Attrib Type, Int or Float only",myName,argix+1);
				break;
			}
			atel[argix].attype = argtype;
			if(argtype == ATTR_INT) {atel[argix].attrib.i=at.AsInt();} 
			else					{atel[argix].attrib.f=(float)at.AsFloat();}
		}
		if(ebf[0]=='\0') {
			if(fseek(fp, arr.attriboffset + atixStart * sizeof(*atel),SEEK_SET))sprintf(ebf,"Seeking Array attribute(%d)",atixStart);
			else {if((fwrite(atel,sizeof(*atel)*arsz,1,fp))!=1)					strcpy(ebf,"Updating Array attributes");}
		}
	}
	fclose(fp);
	if(atel != NULL)	delete [] atel;
	if(ebf[0]!='\0')	env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,Arr_fn(fn),fn);
	return arsz;
}

AVSValue ArrayGetID_Lo(MYARR *arr,int idix) {
	AVSValue ret;
	idix = std::min(std::max(0,idix),127);
	int idoff   = idix / 32;
	int idbitix = idix & 0x1F;
	unsigned int idflgs = arr->idtype[idoff];
	if((idflgs & (1<<idbitix)) == 0)	{ret = arr->id[idix].i;} 
	else								{ret = arr->id[idix].f;}
	return ret;
}


AVSValue __cdecl RT_ArrayGetID(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArrayGetID: ";
	const char *fn		= args[0].AsString();
	const int idix		= args[1].AsInt();
	if(idix < -1 || idix >= 128)
		env->ThrowError("%sError, Invalid ID index %d(-1 -> 127)\n%s\n%s",myName,idix,Arr_fn(fn),fn);
	MYARR arr;
	FILE * fp=ARR_Read_Header(myName,fn,"rb",arr,env);
	fclose(fp);
	AVSValue ret;
	if(idix == -1)	{ret = 128;}
	else			{ret = ArrayGetID_Lo(&arr,idix);}
	return ret;
}

void ArraySetID_Lo(MYARR *arr,int idix,AVSValue avs) {
	bool isI = avs.IsInt();
	idix = std::min(std::max(0,idix),127);
	int idoff   = idix / 32;
	int idbitix = idix & 0x1F;
	unsigned int idflgs = arr->idtype[idoff];
	if(isI) {arr->id[idix].i = avs.AsInt();				idflgs &= ~(1 << idbitix);}
	else	{arr->id[idix].f = (float)avs.AsFloat();	idflgs |= (1 << idbitix);}
	arr->idtype[idoff] = idflgs;
}


AVSValue __cdecl RT_ArraySetID(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArraySetID: ";
	const char *fn		= args[0].AsString();
	const int idStart	= args[1].AsInt();
	const int arsz		= args[2].ArraySize();
	if(idStart < 0 || idStart > 127) 
		env->ThrowError("%sError, Invalid ID Start index %d(0 -> 127)\n%s\n%s",myName,idStart,Arr_fn(fn),fn);
	if(arsz ==0)
		env->ThrowError("%sError, MUST provide list of ID's\n%s\n%s",myName,Arr_fn(fn),fn);
	int idEnd = idStart + arsz - 1;
	if(idEnd > 127)			
		env->ThrowError("%sError, Too Many ID's(Ends @%d, max idix=127)\n%s\n%s",myName,idEnd,Arr_fn(fn),fn);
	MYARR arr;
	FILE * fp=ARR_Read_Header(myName,fn,"rb+",arr,env);
	AVSValue ret;
	for(int idix=idStart;idix<=idEnd;++idix) {
		int argix = idix-idStart;
		AVSValue at	= args[2][argix];
		int argtype = (at.IsInt()) ? 0 : (at.IsFloat()) ? 1 : -1;
		if(argtype < 0)	{
			fclose(fp);
			env->ThrowError("%sError, @ArgArr(%d) Invalid ID Type, Int or Float only\n%s\n%s",myName,argix+1,Arr_fn(fn),fn);
		}
		ArraySetID_Lo(&arr,idix,at);
	}
	rewind(fp);
	size_t wr = fwrite(&arr,sizeof(arr),1,fp);
	fclose(fp);
	if (wr!=1)
		env->ThrowError("%sError, Cannot update Array header.\n%s\n%s",myName,Arr_fn(fn),fn);
	return arsz;
}

AVSValue __cdecl RT_ArrayCheckID(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArrayCheckID: ";
	const char *fn		= args[0].AsString();
	const int idStart	= args[1].AsInt();
	const int arsz		= args[2].ArraySize();
	if(idStart < 0 || idStart > 127) 
			env->ThrowError("%sError, Invalid ID Start index %d(0 -> 127)\n%s\n%s",myName,idStart,Arr_fn(fn),fn);
	if(arsz ==0)				env->ThrowError("%sError, MUST provide list of ID's\n%s\n%s",myName,Arr_fn(fn),fn);
	int idEnd = idStart + arsz - 1;
	if(idEnd > 127)		
		env->ThrowError("%sError, Too Many ID's(Ends @%d, max idix=127)\n%s\n%s",myName,idEnd,Arr_fn(fn),fn);
	MYARR arr;
	FILE * fp=ARR_Read_Header(myName,fn,"rb",arr,env);
	fclose(fp);
	int ret = -1;
	for(int idix=idStart;idix<=idEnd;++idix) {
		int argix = idix-idStart;
		AVSValue at	= args[2][argix];
		int argtype = (at.IsInt()) ? 0 : (at.IsFloat()) ? 1 : -1;
		if(argtype < 0)	env->ThrowError("%sError,  @ArgArr(%d) Invalid ID Type, Int or Float only\n%s\n%s",myName,argix+1,Arr_fn(fn),fn);
		AVSValue id = ArrayGetID_Lo(&arr,idix);
		if(argtype == 0) {
			int intArg = at.AsInt();
			if(!id.IsInt() || id.AsInt()!=intArg) {
				ret = idix;
				break;
			}
		} else {
			float floatArg = (float)at.AsFloat();
			if(!id.IsFloat() || id.AsFloat()!=floatArg) {
				ret = idix;
				break;
			}
		}
	}
	return ret;
}




AVSValue __cdecl RT_ArrayGetStrAttrib(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArrayGetStrAttrib: ";
	const char *fn		= args[0].AsString();
	const int atix		= args[1].AsInt();
	if(atix < -2 || atix >= ARRAY_USERSTRINGS)
		env->ThrowError("%sError, Invalid String Attrib index %d(-2 -> %d)\n%s\n%s",myName,atix,ARRAY_USERSTRINGS-1,Arr_fn(fn),fn);
	MYARR arr;
	FILE * fp=ARR_Read_Header(myName,fn,"rb",arr,env);
	AVSValue ret;
	if(atix == -1) {
		fclose(fp);
		ret = arr.ustrings;
	} else if(atix == -2) {
		fclose(fp);
		ret = arr.ustrlen;
	} else {
		char bf[ARRAY_USERSTRLEN+1];
		bf[ARRAY_USERSTRLEN] = '\0';
		if(fseek(fp,arr.stroffset + atix * ARRAY_USERSTRLEN,SEEK_SET)) {
			fclose(fp);
			env->ThrowError("%sError, Seeking Array String attribute(%d)\n%s\n%s",myName,atix,Arr_fn(fn),fn);
		}
		size_t rd=fread(bf,ARRAY_USERSTRLEN,1,fp);	
		fclose(fp);
		if(rd != 1)		env->ThrowError("%sError, Reading Array String attribute(%d)\n%s\n%s",myName,atix,Arr_fn(fn),fn);
		ret = env->SaveString(bf);
	}
	return ret;
}



AVSValue __cdecl RT_ArraySetStrAttrib(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArraySetStrAttrib: ";
	const char *fn		= args[0].AsString();
	const int atixStart	= args[1].AsInt();
	const int arsz		= args[2].ArraySize();
	if(atixStart < 0 || atixStart >= ARRAY_USERSTRINGS) 
		env->ThrowError("%sError, Invalid String Start index %d(0 -> %d)\n%s\n%s",
			myName,atixStart,ARRAY_USERSTRINGS-1,Arr_fn(fn),fn);
	if(arsz==0)
		env->ThrowError("%sError, MUST provide list of Attrib's\n%s\n%s",myName,Arr_fn(fn),fn);
	int atixEnd = atixStart + arsz - 1;
	if(atixEnd >= ARRAY_USERSTRINGS)			
		env->ThrowError("%sError, Too Many Attrib's(Ends @%d, max idix=%d)\n%s\n%s",
				myName,atixEnd,ARRAY_USERSTRINGS-1,Arr_fn(fn),fn);
	char ebf[256]="";
	MYARR arr;
	FILE * fp=ARR_Read_Header(myName,fn,"rb+",arr,env);
	int bfsz = arr.ustrlen*arsz;
	char *bf = new char[bfsz+1];
	if(bf==NULL) {
		strcpy(ebf,"Cannot allocate mem buffer");
	} else {
		for(int atix=atixStart;atix<=atixEnd;++atix) {
			int argix = atix-atixStart;
			AVSValue at	= args[2][argix];
			if(!at.IsString())	sprintf(ebf,"%sError, @StrArr(%d) Invalid Type, String only",myName,argix+1);
			else {
				const char * s=at.AsString();
				int slen = int(strlen(s));
				if(slen > arr.ustrlen) sprintf(ebf,"%sError, String Attrib(%d) Too Long(%d)",myName,argix+1,slen);
				else {
					char *p= bf + (argix*arr.ustrlen);
					memcpy(p,s,slen);
					memset(p+slen,0,arr.ustrlen-slen);
				}
			}
			if(*ebf!='\0')
				break;
		}
		if(ebf[0]=='\0') {
			if(fseek(fp, arr.stroffset + atixStart * arr.ustrlen,SEEK_SET))	sprintf(ebf,"Seeking Array attribute(%d)",atixStart);
			else {if((fwrite(bf,bfsz,1,fp))!=1)				strcpy(ebf,"Updating Array String attributes");}
		}
		delete [] bf;
	}
	fclose(fp);
	if(ebf[0]!='\0')	env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,Arr_fn(fn),fn);
	return atixEnd-atixStart+1;
}


AVSValue __cdecl RT_ArrayGet(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName="RT_ArrayGet: ";
	const char *fn = args[0].AsString();
	char ebf[256]="",sbf[64]="";
	MYARR arr;
	FILE   * fp=ARR_Read_Header(myName,fn,"rb",arr,env);
	AVSValue var;
	const int type=arr.type;
	const int elsz=arr.elsz;
	const int dim=arr.dim[0];
	int i,darg[3],ix,err,got;
	int rd=0;
	int64_t nel    = 00LL;
	for(got=err=i=0;i<3;++i) {
		if(!args[i+1].Defined()) {
			ix = -1;
			if(i < dim)
				err |= 0x02;					// missing arg
		} else {
			got = i + 1;
			ix=args[got].AsInt();
			if(i >= dim) {
				err |= 0x01;					// too many args
			} else {
				if(ix<0 || ix >= arr.dim[got])
					err |= 0x04;				// subscript out of bounds
				else
					nel += int64_t(ix) * arr.mul[i];
			}
		}
		darg[i]=ix;
	}
	int64_t nelLim = int64_t(arr.dim[1]) * arr.mul[0];
	if(nel < 0 || nel >= nelLim)						err |= 0x08;
	else if(type<ARRAY_BOOL || type > ARRAY_DOUBLE)		err |= 0x10;
	if(err) {
		char mbf[64]="";
		Arr_IxStr(sbf,got,darg[0],darg[1],darg[2]);
		Arr_IxStr(mbf,dim,arr.dim[1]-1,arr.dim[2]-1,arr.dim[3]-1);
		if(err & 0x01)			sprintf(ebf,"ARR(%s) : Too many subscripts, got %d expecting %d",sbf,got,dim);
		else if(err & 0x02)		sprintf(ebf,"ARR(%s) : Missing subscript, got %d expecting %d",sbf,got,dim);
		else if(err & 0x04)		sprintf(ebf,"ARR(%s) : Subscript out of bounds MAX(%s)",sbf,mbf);
		else if(err & 0x08)		sprintf(ebf,"ARR(%s) : Internal Error, Element does not exist(%0LLd)",sbf,nel);
		else					sprintf(ebf,"Internal Error, Illegal Array type=%d",type);
	} else {
		char bf[1024+1],*bfp=bf;
		if(elsz >= sizeof(bf) && ((bfp=new char[elsz+1])==NULL))			// nul term needed for string
			strcpy(ebf,"Cannot Allocate temp read buffer");
		else if(fseeko(fp,arr.offset + nel * elsz,SEEK_SET))			
			sprintf(ebf,"Cannot seek to Element ARR(%s) : NEL(%0LLd)",Arr_IxStr(sbf,dim,darg[0],darg[1],darg[2]),nel);
		else if(fread(bfp,elsz,1,fp)!=1)
			sprintf(ebf,"Cannot read Element ARR(%s) : NEL(%0LLd)",Arr_IxStr(sbf,dim,darg[0],darg[1],darg[2]),nel);
		else {
			switch(type) {
			case ARRAY_BOOL:	{char  * p = (char*)  bfp;	var = (*p !=0);}	break;
			case ARRAY_INT:		{int   * p = (int*)   bfp;	var = *p;}			break;
			case ARRAY_FLOAT:	{float * p = (float*) bfp;	var = *p;}			break;
			case ARRAY_STRING:	bfp[elsz] = '\0';var = env->SaveString(bfp);	break;
			case ARRAY_BIN:		{BYTE  * p = (BYTE*)   bfp; var = (int)*p;}		break;
			case ARRAY_DOUBLE:	{double* p = (double*) bfp;	var = *p;}			break;
			}
			rd=1;	// success
		}
		if(bfp != bf && bfp != NULL)	delete [] bfp;
	}
	fclose(fp);
	if(rd!=1)			env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,Arr_fn(fn),fn);
	return var;
}



AVSValue __cdecl RT_ArraySet(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArraySet: ";
	const char *fn		= args[0].AsString();
	char ebf[256]="";
	size_t wr=0;
	int64_t nel    = 00LL;
	MYARR arr;
	FILE   * fp=ARR_Read_Header(myName,fn,"rb+",arr,env);
	const int dim=arr.dim[0];
	const int type=arr.type;
	const int elsz=arr.elsz;
	int i,darg[3],ix,err,got;
	for(got=err=i=0;i<3;++i) {
		if(!args[i+2].Defined()) {
			ix = -1;
			if(i < dim)
				err |= 0x02;					// missing arg
		} else {
			got = i + 1;
			ix=args[i+2].AsInt();
			if(i >= dim) {
				err |= 0x01;					// too many args
			} else {
				if(ix < 0 || ix >= arr.dim[got])
					err |= 0x04;				// subscript out of bounds
				else
					nel += int64_t(ix) * arr.mul[i];
			}
		}
		darg[i]=ix;
	}

	AVSValue at=args[1];
	const int atyp = (at.IsInt()) ? 1 : (at.IsFloat()) ? 2 : (at.IsBool()) ? 0 : (at.IsString()) ? 3 : -1;

	int64_t nelLim = int64_t(arr.dim[1]) * arr.mul[0];
	if(nel < 0 || nel >= nelLim)																err |= 0x08;
	else if(type<ARRAY_BOOL || type > ARRAY_DOUBLE)												err |= 0x10;
	else if(atyp < 0)																			err |= 0x20;
	else if(atyp!=type && !((type==ARRAY_BIN && atyp==1) || (type==ARRAY_DOUBLE && atyp==2)))	err |= 0x40;	
	else if(err==0 && fseeko(fp,arr.offset+ nel*elsz,SEEK_SET))								err |= 0x80;
	if(err) {
		char sbf[256]="",mbf[64]="";
		Arr_IxStr(sbf,got,darg[0],darg[1],darg[2]);
		Arr_IxStr(mbf,dim,arr.dim[1]-1,arr.dim[2]-1,arr.dim[3]-1);
		static const char *ts[]={"Bool(0)","Int(1)","Float(2)","String(3)","Bin[as Int(1)]","Double[as Float(5)]"};
		if		(err & 0x01)	sprintf(ebf,"ARR(%s) : Too many subscripts, got %d expecting %d",sbf,got,dim);
		else if	(err & 0x02)	sprintf(ebf,"ARR(%s) : Missing subscript, got %d expecting %d",sbf,got,dim);
		else if	(err & 0x04)	sprintf(ebf,"ARR(%s) : Subscript out of bounds MAX(%s)",sbf,mbf);
		else if	(err & 0x08)	sprintf(ebf,"ARR(%s) : Internal Error, Element does not exist(%0LLd)",sbf,nel);
		else if	(err & 0x10)	sprintf(ebf,"ARR(%s) : Internal Error, Illegal Array Element type=%d",sbf,type);
		else if	(err & 0x20)	sprintf(ebf,"ARR(%s) : Unknown Data arg Type, expecting Type=%d",sbf,type);
		else if	(err & 0x40)	sprintf(ebf,"ARR(%s) : Incorrect Data Type=%s, expecting %s",sbf,ts[atyp],ts[type]);
		else 					sprintf(ebf,"ARR(%s) : NEL(%0LLd) Cannot seek to Element",sbf,nel);
	} else {	
		if(type==ARRAY_STRING) {
			char bf[1024],*bfp=bf;
			char *s = (char*)at.AsString();
			const int len = int(strlen(s));
			if(len > elsz)									sprintf(ebf,"String Too Long %d(max=%d)",len,elsz);
			else if(elsz > sizeof(bf) && (bfp= new char[elsz])==NULL)		// nul term not needed for string
				strcpy(ebf,"Cannot Allocate String buffer");
			else {
				memcpy(bfp,s,len);
				memset(bfp+len,0,arr.elsz-len);
				if((wr=fwrite(bfp,elsz,1,fp))!=1)			strcpy(ebf,"writing Array data");
				if(bfp != bf) delete [] bfp;
			}
		} else {
			// *** MUST be in scope for the file write ***
			int		Int_t;
			float	Float_t;
			double	Double_t;
			BYTE	Bin_t;
			char	Bool_t,*p;
			//
			if(type==ARRAY_INT)			{ Int_t		= at.AsInt();			p = (char*) &Int_t;   }
			else if(type==ARRAY_FLOAT)	{ Float_t	= (float)at.AsFloat();	p = (char*) &Float_t; }
			else if(type==ARRAY_DOUBLE)	{ Double_t	= (double)at.AsFloat();	p = (char*) &Double_t;}
			else if(type==ARRAY_BOOL)	{ Bool_t	= at.AsBool();			p = (char*) &Bool_t;  }
			else						{ Bin_t		= (BYTE) (at.AsInt());	p = (char*) &Bin_t;   }
			if((wr=fwrite(p,elsz,1,fp)) !=1)		strcpy(ebf,"writing Array data");
		}
	}
	fclose(fp);
	if(wr!=1)			env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,Arr_fn(fn),fn);
	return arr.dim[1];	
}



AVSValue __cdecl RT_ArrayExtend(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArrayExtend: ";
	const char * fn		= args[0].AsString();
	const int add		= args[1].AsInt(1);
	if(add < 0)	env->ThrowError("%sError, Cannot make smaller, Add=%d\n%s\n%s",myName,add,Arr_fn(fn),fn);
	MYARR arr;
	FILE   * fp=ARR_Read_Header(myName,fn,"rb+",arr,env);
	char ebf[256]="";
	size_t wr = 0;
	if(add == 0) wr=1;
	else {
		int64_t dfs = QueryMaxFileSize(fn);
		if(dfs < 0)						sprintf(ebf,"%sCannot query MaxFileSize",myName);
		else {
			int lim = int(std::min<long long int>(((dfs / arr.dim1Bytes) + arr.dim[1]),0x7FFFFFFE0LL));
			const int extmaxsz = lim - arr.dim[1];			// how many we can grow by
			if(add > extmaxsz)			
				sprintf(ebf,"Add %d, Overflows available space(%d extend available)",add,extmaxsz);
			else {
				char bf[1024],*bfp=bf;
				int64_t clrbytes = int64_t(add) * arr.dim1Bytes;
				int bfsz = (clrbytes>(1024*10240LL)) ? 1024*1024 : int(clrbytes);
				if(bfsz > sizeof(bf)) {
					bfp = new char[bfsz];
					if(bfp==NULL) {
						bfp=bf;
						bfsz=sizeof(bf);
					}
				}
				const int sods = int(clrbytes  /  bfsz);
				const int odds = int(clrbytes  %  bfsz);
				memset(bfp,0,bfsz);
				if(fseeko(fp,arr.offset+int64_t(arr.dim[1])*arr.dim1Bytes,SEEK_SET))		strcpy(ebf,"Cannot seek to END");
				else {
					int i;
					for(wr=1,i = sods; wr == 1 && --i >= 0; )
						wr=fwrite(bfp,bfsz,1,fp);
					if(wr==1 && odds>0)		wr = fwrite(bfp,odds,1,fp);
					if(wr != 1)				strcpy(ebf,"writing Array Extend data");
					else {
						rewind(fp);
						arr.dim[1] += add;
						wr = fwrite(&arr,sizeof(arr),1,fp);
						if(wr != 1)	strcpy(ebf,"updating Array header");
					}
				}
				if(bfp!=NULL && bfp!=bf)	delete [] bfp;
			}
		}
	}
	fclose(fp);
	if(wr!=1)			env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,Arr_fn(fn),fn);
	return arr.dim[1];	// New size of Dim1
}


AVSValue __cdecl RT_ArrayAppend(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_ArrayAppend: ";
	const char *fn		= args[0].AsString();
	MYARR arr;
	FILE   * fp=ARR_Read_Header(myName,fn,"rb+",arr,env);
	if(arr.dim[0]!=1) {
		fclose(fp);
		env->ThrowError("%sCannot append to multi dimensional Array\n%s\n%s",myName,Arr_fn(fn),fn);
	}
	char ebf[256]="";
	AVSValue at=args[1];

	static const char *ts[]={"Bool(0)","Int(1)","Float(2)","String(3)","Bin[as Int(1)]","Double[as Float(2)]"};
	const int type=arr.type;
	const int elsz=arr.elsz;
	const int atyp = (at.IsInt()) ? 1 : (at.IsFloat()) ? 2 : (at.IsBool()) ? 0 : (at.IsString()) ? 3 : -1;
	size_t wr=0;	
	int64_t dfs = QueryMaxFileSize(fn);
	if(dfs < 0)												sprintf(ebf,"%sCannot query MaxFileSize",myName);
	else {
		int dim1max=int(std::min<long long int>(((dfs / arr.dim1Bytes) + arr.dim[1]),0x7FFFFFFE0LL));
		if(type<ARRAY_BOOL || type > ARRAY_DOUBLE)			sprintf(ebf,"Internal Error, Illegal Array type=%d",type);
		else if(atyp < 0)									sprintf(ebf,"Unknown data arg Type, expecting Type=%s",ts[type]);
		else if(arr.dim[1] >= dim1max)						sprintf(ebf,"Reached Array Limit %d",dim1max);
		else if(fseeko(fp,arr.offset+int64_t(arr.dim[1])*arr.dim1Bytes,SEEK_SET))
			strcpy(ebf,"Cannot seek to END");
		else if(atyp!=type && !((type==ARRAY_BIN&&atyp==1) || (type==ARRAY_DOUBLE&&atyp==2)))	
			sprintf(ebf,"Incorrect Type=%s, expecting %s",ts[atyp],ts[type]);
		else if(type==ARRAY_STRING) {
			char *s = (char*)at.AsString();
			char bf[1024],*bfp=bf;
			const int len = int(strlen(s));
			if(len > elsz)									sprintf(ebf,"String Too Long %d(max=%d)",len,elsz);
			else if(elsz > sizeof(bf) && (bfp= new char[elsz])==NULL)	// nul term not needed for string
				strcpy(ebf,"Allocating String buffer");
			else {
				memcpy(bfp,s,len);
				memset(bfp+len,0,arr.elsz-len);
				if((wr=fwrite(bfp,elsz,1,fp))!=1)			strcpy(ebf,"writing Array Element");
				if(bfp != bf) delete [] bfp;
			}
		} else {
			// *** MUST be in scope for the file write (indirect access via p) ***
			int		Int_t;
			float	Float_t;
			double	Double_t;
			BYTE	Bin_t;
			//
			char	Bool_t,*p;
			if(type==ARRAY_INT)			{ Int_t		= at.AsInt();			p = (char*) &Int_t;   }
			else if(type==ARRAY_FLOAT)	{ Float_t	= (float)at.AsFloat();	p = (char*) &Float_t; }
			else if(type==ARRAY_DOUBLE)	{ Double_t	= (double)at.AsFloat();	p = (char*) &Double_t;}
			else if(type==ARRAY_BOOL)	{ Bool_t	= at.AsBool();			p = (char*) &Bool_t;  }
			else						{ Bin_t		= (BYTE) (at.AsInt());	p = (char*) &Bin_t;   }
			if((wr = fwrite(p,elsz,1,fp))!=1)				strcpy(ebf,"writing Array Element");
		}
		if(wr==1) {
			++arr.dim[1];
			rewind(fp);
			if((wr=fwrite(&arr,sizeof(arr),1,fp))!=1)		strcpy(ebf,"updating array header");
		}
	}
	fclose(fp);
	if(wr != 1) {
		env->ThrowError("%sError, ARR(%d) %s\n%s\n%s",myName,arr.dim[1],ebf,Arr_fn(fn),fn);
	}
	return arr.dim[1];
}




