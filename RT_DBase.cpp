
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

	enum {
		DB_FIELDS		= 1024,
		DB_ATTRIB		= 1024,
		DB_USERSTRINGS  = 10,
		DB_USERSTRLEN   = 1024,
		DB_OFFSET		= 32*1024,
		DB_INFOFFSET	= sizeof(MYDB),
		DB_ATTROFFSET	= DB_INFOFFSET  + sizeof(MYDBINF) * DB_FIELDS,
		DB_USTROFFSET	= DB_ATTROFFSET + sizeof(MYATEL)  * DB_ATTRIB,
		DB_SPARESTART	= DB_USTROFFSET + DB_USERSTRLEN   * DB_USERSTRINGS,
		DB_SPARE		= DB_OFFSET - DB_SPARESTART
	};

	#define RTDB	0x42445452			// 'BDTR', 'RTDB' in memory (silly little endian i86)
	#define RTDBVER	0x0000000B

extern FILE * __cdecl DB_Read_Header(const char *myName,const char *fn,const char *omd,MYDB &db,IScriptEnvironment* env,char *retbf=NULL);

const char * __cdecl DB_fn(const char *fn) {
	const char *s=fn;
	int c;
	for(;c=*fn;++fn) { 
		if((c=='\\' || c=='/') && (fn[1]!='\0'))
			s=fn+1;
	}
	return s;
}

// WARNING, Default retbuf = NULL
FILE * __cdecl DB_Read_Header(const char *myName,const char *fn,const char *omd,MYDB &db,IScriptEnvironment* env,char *retbf) {
	char *eptr=NULL;
	char msg[1024]="";
	FILE * fp=NULL;
	if(*fn=='\0')								eptr="Empty filename";
	else if((fp=fopen(fn, omd ))==NULL)			eptr="Cannot open DBase file";
	else if(fread(&db,sizeof(db),1,fp)!=1)		eptr="Reading DBase file header";
	else if(db.name!=RTDB)						eptr="Not an RT_DBase file";
	else if(db.ver!=RTDBVER)					eptr="RT_DBase wrong version";
	if(eptr!=NULL) {
		if(fp!=NULL) {fclose(fp); fp=NULL;}
		char *p = (retbf==NULL) ? msg : retbf;
		sprintf(p,"%sError, %s\n%s\n%s",myName,eptr,DB_fn(fn),fn);
		if(retbf==NULL)
			env->ThrowError("%s",p);
	}
	return fp;
}


AVSValue __cdecl RT_DBaseAlloc(AVSValue args, void* user_data, IScriptEnvironment* env) {	
	const char * myName	= "RT_DBaseAlloc: ";
	const char * fn		= args[0].AsString();
	const int records	= args[1].AsInt();
	const char *typestr	= args[2].AsString();
	const int StringlenMax = args[3].AsInt(256);
	if(*fn=='\0')	env->ThrowError("%sEmpty Filename",myName);

	int64_t MaxFileSz = 0xFFFFF000000LL ;												// 1TB - 1MB

	int fatvol = QueryFatVolume(fn);
	if(fatvol < 0)	env->ThrowError("%sCannot query Filesystem",myName);

	int64_t dfs = QueryDiskFreeSpace(fn) - 0x100000;									// minus 1MB
	if(dfs < 0)	env->ThrowError("%sCannot query DiskFreeSpace",myName);
	// dprintf("DiskFreeSpace=$%0LLX",dfs);
	int64_t maxfs = (fatvol==1)? 0xFFF000000LL : MaxFileSz; 	// limit 4GB on FAT32
	int64_t maxcurdfs=std::min(maxfs,dfs);							// Max current space, Limit to free space available to user.

	int maxfieldsz = 0;
	MYDBINF *dbinf = NULL;
	char ebf[256]="";
	FILE * fp=NULL;
	size_t wr=0;
	MYDB db;
	memset(&db,0,sizeof(db));
	if(records < 0)
		sprintf(ebf,"Illegal -ve Records(%d)",records);
	else if(StringlenMax<=0 || StringlenMax>(256*1024))
		sprintf(ebf,"Illegal StringLenMax=%d(1 -> 256K[%d])",StringlenMax,256*1024);
	else if((dbinf=new MYDBINF[DB_FIELDS])==NULL)
		strcpy(ebf,"allocating DBInf");
	else {
		memset(dbinf,0,DB_FIELDS * sizeof(dbinf[0]));
		int fld,fsz,off,ix,c;
		for(ix=off=fld=0;fld<=DB_FIELDS;++fld) {
			if(fld == DB_FIELDS || (c = typestr[ix]) == 0) {
				wr=1;						// Parsed TypeString
				break;
			}
			++ix;
			dbinf[fld].fieldoff=off;
			fsz=0;
			if(c=='i' || c=='I')		{dbinf[fld].fieldtype=DB_INT;	fsz=4;}
			else if(c=='f' || c=='F')	{dbinf[fld].fieldtype=DB_FLOAT; fsz=4;}
			else if(c=='e' || c=='E')	{dbinf[fld].fieldtype=DB_DOUBLE;fsz=8;}
			else if(c=='b' || c=='B')	{dbinf[fld].fieldtype=DB_BOOL;	fsz=1;}
			else if(c=='n' || c=='N')	{dbinf[fld].fieldtype=DB_BIN;	fsz=1;}
			else if(c=='s' || c=='S')	{dbinf[fld].fieldtype=DB_STRING;
				if(typestr[ix] >= '0' && typestr[ix] <='9') {
					int ix_s=ix;
					while(typestr[ix] >= '0' && typestr[ix] <='9') {
						fsz = fsz * 10 + typestr[ix] - '0';
						++ix;
					}
					if(fsz <= 0 || fsz > (256*1024)) {
						sprintf(ebf,"Illegal String size %d @ TypeString(%d)(1 -> 256K[%d])",fsz,ix_s,256*1024);
						break;
					}
				} else {
					fsz = StringlenMax;
				}
			} else {
				sprintf(ebf,"Unknown type '%c' @ TypeString(%d)",c,ix);
				break;
			}
			dbinf[fld].fieldsize=fsz;
			off+=fsz;
			maxfieldsz=std::max(fsz,maxfieldsz);
		}
		if(wr == 1) {
			wr = 0;
			if(fld==0)						strcpy(ebf,"TypeString zero fields");
			else if(typestr[ix] != '\0')	sprintf(ebf,"TypeString Too many fields (1 -> %d)",DB_FIELDS);				
			else {
				int64_t maxrecords=std::min<long long int>((MaxFileSz - DB_OFFSET) / off,0x7FFFFFFE0LL); // max possible records on any filesystem
				int64_t recmaxfs = std::min<long long int>((maxfs  - DB_OFFSET) / off,0x7FFFFFFE0LL);
				int64_t recmaxdfs= std::min<long long int>((maxcurdfs - DB_OFFSET) / off,0x7FFFFFFE0LL);
				db.name		= RTDB;
				db.ver		= RTDBVER;
				db.offset	= DB_OFFSET;
				db.infoffset= DB_INFOFFSET;
				db.attriboffset=DB_ATTROFFSET;
				db.stroffset= DB_USTROFFSET;
				db.records	= records;
				db.recordsz = off;
				db.recordmax= int(maxrecords);				// maximum possible records (any filesystem/disk size)
				db.maxfieldsz=maxfieldsz;
				db.fields	= fld;
				db.attribs  = DB_ATTRIB;
				db.ustrings = DB_USERSTRINGS;
				db.ustrlen  = DB_USERSTRLEN;
				if(records > maxrecords)				sprintf(ebf,"Records(%d) Exceeds recordmax(%d)",records,maxrecords);
				else if(records > recmaxfs)				strcpy(ebf,"Records * record size Too Big for FileSystem");					
				else if(records > recmaxdfs)			strcpy(ebf,"Not Enough Disk Space Available");					
				else {
					int bfsz = 1024*4;
					BYTE *bf = new BYTE[bfsz];
					if(bf == NULL) {
						strcpy(ebf,"Cannot allocate memory buffer");
					} else {
						memset(bf,0,bfsz);
						if((fp=fopen(fn, "wb" ))==NULL)		strcpy(ebf,"Cannot create DBase file");
						else {
							int64_t clrbytes = DB_OFFSET + int64_t(db.records) * db.recordsz;
							int sods = int(clrbytes / bfsz);
							int odds = int(clrbytes % bfsz);
							wr=1;
							for(int i=sods; wr==1 && --i >= 0; )
								wr=fwrite(bf,bfsz,1,fp);	
							if(wr == 1 && odds > 0)
								wr=fwrite(bf,odds,1,fp);
							if(wr != 1)											strcpy(ebf,"Writing DBase Init data");
							else {
								rewind(fp);
								if((wr=fwrite(&db,sizeof(db),1,fp))!=1) 		strcpy(ebf,"Writing DBase file header");
								else {
									if(fseek(fp,db.infoffset,SEEK_SET))			strcpy(ebf,"Seeking DBase InfoOffset");
									else {
										if((wr=fwrite(dbinf,sizeof(dbinf[0])*db.fields,1,fp))!=1)
																				strcpy(ebf,"Writing DBase Inf");
									}
								}
							}
						}
						delete [] bf;
					}
				}
			}
		}
	}
	if(dbinf != NULL)	delete [] dbinf;
	if (fp != NULL)		fclose(fp);
	if(wr != 1) {
		env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,DB_fn(fn),fn);
	}
	return db.records;
}

AVSValue __cdecl RT_DBaseRecords(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseRecords: ";
	const char *fn		= args[0].AsString();
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb",db,env);
	fclose(fp);
	return db.records;
}

AVSValue __cdecl RT_DBaseFields(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseFields: ";
	const char *fn		= args[0].AsString();
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb",db,env);
	fclose(fp);
	return db.fields;
}

AVSValue __cdecl RT_DBaseRecordSize(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseRecordSize: ";
	const char *fn		= args[0].AsString();
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb",db,env);
	fclose(fp);
	return db.recordsz;
}

AVSValue __cdecl RT_DBaseRecordsMax(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseRecordsMax: ";
	const char *fn		= args[0].AsString();
	const bool current	= args[1].AsBool(true);
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb",db,env);
	fclose(fp);
	int recmx=db.recordmax;
	if(current) {
		int64_t dfs = QueryMaxFileSize(fn);
		if(dfs < 0)	env->ThrowError("%sCannot query MaxFileSize",myName);
		recmx = int(std::min<long long int>(dfs / db.recordsz + db.records,0x7FFFFFFE0LL));
	}
	return  recmx;
}


AVSValue __cdecl RT_DBaseFieldType(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseFieldType: ";
	const char *fn		= args[0].AsString();
	const int field	= args[1].AsInt();
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb",db,env);
	if(field < 0 || field >= db.fields) {
		fclose(fp);
		env->ThrowError("%sError, Bad field number %d(0 -> %d)\n%s\n%s",myName,field, db.fields-1,DB_fn(fn),fn);
	}
	MYDBINF dbinf;
	if((fseek(fp, db.infoffset+field*sizeof(dbinf),SEEK_SET)) || (fread(&dbinf,sizeof(dbinf),1,fp)!=1)) {
		fclose(fp);
		env->ThrowError("%sError, Cannot read DBInf \n%s\n%s",myName,DB_fn(fn),fn);
	}
	fclose(fp);
	int typ = dbinf.fieldtype;
	return typ;
}

AVSValue __cdecl RT_DBaseTypeName(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseTypeName: ";
	const int	type	= args[0].AsInt();
	if(type < 0 || type > 5)
		env->ThrowError("%sError, Bad type number %d(0 -> 5)",myName,type);
	static const char *names[] = {"Bool","Int","Float","String","Bin","Double"};
	return names[type];
}


AVSValue __cdecl RT_DBaseFieldSize(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseFieldSize: ";
	const char *fn		= args[0].AsString();
	const int	field	= args[1].AsInt();
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb",db,env);
	if(field < 0 || field >= db.fields) {
		fclose(fp);
		env->ThrowError("%sError, Bad field number %d(0 -> %d)\n%s\n%s",myName,field, db.fields-1,DB_fn(fn),fn);
	}
	MYDBINF dbinf;
	if((fseek(fp, db.infoffset+field*sizeof(dbinf),SEEK_SET)) || (fread(&dbinf,sizeof(dbinf),1,fp)!=1)) {
		fclose(fp);
		env->ThrowError("%sError, Cannot read DBInf \n%s\n%s",myName,DB_fn(fn),fn);
	}
	fclose(fp);
	int sz = dbinf.fieldsize;
	return sz;
}


AVSValue DBaseGetID_Lo(MYDB *db,int idix) {
	AVSValue ret;
	idix = std::min(std::max(0,idix),127);
	int idoff   = idix / 32;
	int idbitix = idix & 0x1F;
	unsigned int idflgs = db->idtype[idoff];
	if((idflgs & (1<<idbitix)) == 0) {
		ret = db->id[idix].i;
	} else {
		ret = db->id[idix].f;
	}
	return ret;
}

AVSValue __cdecl RT_DBaseGetID(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseGetID: ";
	const char *fn		= args[0].AsString();
	const int idix		= args[1].AsInt();
	if(idix < -1 || idix > 127)
		env->ThrowError("%sError, Invalid ID index %d(-1 -> 127)\n%s\n%s",myName,idix,DB_fn(fn),fn);
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb",db,env);
	fclose(fp);
	AVSValue ret;
	if(idix == -1) {
		ret = 128;
	} else {
		ret = DBaseGetID_Lo(&db,idix);
	}
	return ret;
}

void DBaseSetID_Lo(MYDB *db,int idix,AVSValue avs) {
	bool isI = avs.IsInt();
	idix = std::min(std::max(0,idix),127);
	int idoff   = idix / 32;
	int idbitix = idix & 0x1F;
	unsigned int idflgs = db->idtype[idoff];
	if(isI) {
		db->id[idix].i = avs.AsInt();
		idflgs &= ~(1 << idbitix);
	} else {
		db->id[idix].f = (float)avs.AsFloat();
		idflgs |= (1 << idbitix);
	}
	db->idtype[idoff] = idflgs;
}


AVSValue __cdecl RT_DBaseSetID(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseSetID: ";
	const char *fn		= args[0].AsString();
	const int idStart	= args[1].AsInt();
	const int arsz		= args[2].ArraySize();
	if(idStart < 0 || idStart > 127) 
		env->ThrowError("%sError, Invalid ID Start index %d(0 -> 127)\n%s\n%s",myName,idStart,DB_fn(fn),fn);
	if(arsz ==0)
		env->ThrowError("%sError, MUST provide list of ID's\n%s\n%s",myName,DB_fn(fn),fn);
	int idEnd = idStart + arsz - 1;
	if(idEnd > 127)			
		env->ThrowError("%sError, Too Many ID's(Ends @%d, max idix=127)\n%s\n%s",myName,idEnd,DB_fn(fn),fn);
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb+",db,env);
	for(int idix=idStart;idix<=idEnd;++idix) {
		int argix = idix-idStart;
		AVSValue at	= args[2][argix];
		int argtype = (at.IsInt()) ? 0 : (at.IsFloat()) ? 1 : -1;
		if(argtype < 0)	{
			fclose(fp);
			env->ThrowError("%sError, @ArgArr(%d) Invalid ID Type, Int or Float only\n%s\n%s",myName,argix+1,DB_fn(fn),fn);
		}
		DBaseSetID_Lo(&db,idix,at);
	}
	rewind(fp);
	size_t wr = fwrite(&db,sizeof(db),1,fp);
	fclose(fp);
	if (wr!=1)
		env->ThrowError("%sError, Cannot update DB header.\n%s\n%s",myName,DB_fn(fn),fn);
	return idEnd-idStart+1;
}



AVSValue __cdecl RT_DBaseCheckID(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseCheckID: ";
	const char *fn		= args[0].AsString();
	const int idStart	= args[1].AsInt();
	const int arsz		= args[2].ArraySize();
	if(idStart < 0 || idStart > 127) 
			env->ThrowError("%sError, Invalid ID Start index %d(0 -> 127)\n%s\n%s",myName,idStart,DB_fn(fn),fn);
	if(arsz ==0)				env->ThrowError("%sError, MUST provide list of ID's\n%s\n%s",myName,DB_fn(fn),fn);
	int idEnd = idStart + arsz - 1;
	if(idEnd > 127)		
		env->ThrowError("%sError, Too Many ID's(Ends @%d, max idix=127)\n%s\n%s",myName,idEnd,DB_fn(fn),fn);
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb",db,env);
	fclose(fp);
	int ret = -1;

	for(int idix=idStart;idix<=idEnd;++idix) {
		int argix = idix-idStart;
		AVSValue at	= args[2][argix];
		int argtype = (at.IsInt()) ? 0 : (at.IsFloat()) ? 1 : -1;
		if(argtype < 0)	env->ThrowError("%sError,  @ArgArr(%d) Invalid ID Type, Int or Float only\n%s\n%s",myName,argix+1,DB_fn(fn),fn);
		AVSValue id = DBaseGetID_Lo(&db,idix);
		if(argtype == 0) {
			int intArg = at.AsInt();
			if(!id.IsInt() || id.AsInt()!=intArg) {
				ret = idix;
				break;
			}
		}
		else {
			float floatArg = (float)at.AsFloat();
			if(!id.IsFloat() || id.AsFloat()!=floatArg) {
				ret = idix;
				break;
			}
		}
	}
	return ret;
}


AVSValue __cdecl RT_DBaseGetAttrib(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseGetAttrib: ";
	const char *fn		= args[0].AsString();
	const int atix		= args[1].AsInt();
	if(atix < -1 || atix >= DB_ATTRIB)
		env->ThrowError("%sError, Invalid Attrib index %d(-1 -> %d)\n%s\n%s",myName,atix,DB_ATTRIB-1,DB_fn(fn),fn);
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb",db,env);
	AVSValue ret;
	if(atix == -1) {
		fclose(fp);
		ret = db.attribs;
	} else {
		MYATEL atel;
		if(fseek(fp, db.attriboffset + atix * sizeof(atel),SEEK_SET)) {
			fclose(fp);
			env->ThrowError("%sError, Seeking DBase attribute(%d)\n%s\n%s",myName,atix,DB_fn(fn),fn);
		}
		size_t rd=fread(&atel,sizeof(atel),1,fp);	
		fclose(fp);
		if(rd != 1)		env->ThrowError("%sError, Reading DBase attribute(%d)\n%s\n%s",myName,atix,DB_fn(fn),fn);
		// *** WARNING ***, Dont try to use ternary conditional (?:) below, compiler always sets ret to float.
		// Presumably, type assignment to ret done at compile time.
		if(atel.attype == ATTR_INT) {
			ret = atel.attrib.i;
		} else {
			ret = atel.attrib.f;
		}
	}
	return ret;
}



AVSValue __cdecl RT_DBaseSetAttrib(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseSetAttrib: ";
	const char *fn		= args[0].AsString();
	const int atixStart	= args[1].AsInt();
	const int arsz		= args[2].ArraySize();
	if(atixStart < 0 || atixStart >= DB_ATTRIB) 
		env->ThrowError("%sError, Invalid ID Start index %d(0 -> %d)\n%s\n%s",myName,atixStart,DB_ATTRIB-1,DB_fn(fn),fn);
	if(arsz ==0)
		env->ThrowError("%sError, MUST provide list of Attrib's\n%s\n%s",myName,DB_fn(fn),fn);
	int atixEnd = atixStart + arsz - 1;
	if(atixEnd >= DB_ATTRIB)			
		env->ThrowError("%sError, Too Many Attrib's(Ends @%d, max idix=%d)\n%s\n%s",myName,atixEnd,DB_ATTRIB-1,DB_fn(fn),fn);
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb+",db,env);
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
			if(fseek(fp, db.attriboffset + atixStart * sizeof(*atel),SEEK_SET))	sprintf(ebf,"Seeking DBase attribute(%d)",atixStart);
			else {if((fwrite(atel,sizeof(*atel)*arsz,1,fp))!=1)					strcpy(ebf,"Updating DBase attributes");}
		}
	}
	fclose(fp);
	if(atel != NULL)	delete [] atel;
	if(ebf[0]!='\0')	env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,DB_fn(fn),fn);
	return atixEnd-atixStart+1;
}


AVSValue __cdecl RT_DBaseGetTypeString(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseGetTypeString: ";
	const char *fn		= args[0].AsString();
	char ebf[256]="";
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb",db,env);
	const int fields  = db.fields;
	MYDBINF *dbinf=NULL;
	char *bf=NULL;
	int bfsz=fields*7+1;	// Allow all to be type string with 6 chars for size (1024*256) = 1 + 6 chars = 7 (+ nul).
	if((dbinf= new MYDBINF[fields])==NULL)			strcpy(ebf,"cannot allocate DBInf");
	else if((bf= new char[bfsz])==NULL)				strcpy(ebf,"cannot allocate string buffer");
	else if(fseek(fp, db.infoffset,SEEK_SET) || fread(dbinf,fields*sizeof(dbinf[0]),1,fp)!=1)
		sprintf(ebf,"cannot read DBInf");
	else {
		memset(bf,0,bfsz);
		char *bfp=bf;
		static const char *ts[]={"b","i","f","s","n","e"};
		for(int fld=0;fld<fields;++fld) {
			const int type		= dbinf[fld].fieldtype;
			const char *p=ts[type];
			*bfp++=*p;
			if(type==3) {
				const int fieldsize	= dbinf[fld].fieldsize;
				sprintf(bfp,"%d",fieldsize);
				while(*bfp)
					++bfp;
			}
		}
	}
	fclose(fp);
	if(dbinf)			delete [] dbinf;
	AVSValue ret;
	if(bf != NULL)		{
		ret=env->SaveString(bf);
		delete [] bf;
	}
	if(ebf[0]!='\0')	env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,DB_fn(fn),fn);
	return ret;
}


AVSValue __cdecl RT_DBaseGetStrAttrib(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseGetStrAttrib: ";
	const char *fn		= args[0].AsString();
	const int atix		= args[1].AsInt();
	if(atix < -2 || atix >= DB_USERSTRINGS)
		env->ThrowError("%sError, Invalid String Attrib index %d(-2 -> %d)\n%s\n%s",myName,atix,DB_USERSTRINGS-1,DB_fn(fn),fn);
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb",db,env);
	AVSValue ret;
	if(atix == -1) {
		fclose(fp);
		ret = db.ustrings;
	} else 	if(atix == -2) {
		fclose(fp);
		ret = db.ustrlen;
	} else {
		char bf[DB_USERSTRLEN+1];
		bf[DB_USERSTRLEN] = '\0';
		if(fseek(fp, db.stroffset + atix * DB_USERSTRLEN,SEEK_SET)) {
			fclose(fp);
			env->ThrowError("%sError, Seeking DBase String attribute(%d)\n%s\n%s",myName,atix,DB_fn(fn),fn);
		}
		size_t rd=fread(bf,DB_USERSTRLEN,1,fp);	
		fclose(fp);
		if(rd != 1)		env->ThrowError("%sError, Reading DBase String attribute(%d)\n%s\n%s",myName,atix,DB_fn(fn),fn);
		ret = env->SaveString(bf);
	}
	return ret;
}

AVSValue __cdecl RT_DBaseSetStrAttrib(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseSetStrAttrib: ";
	const char * fn		= args[0].AsString();
	const int atixStart	= args[1].AsInt();
	const int arsz		= args[2].ArraySize();
	if(atixStart < 0 || atixStart >= DB_USERSTRINGS) 
		env->ThrowError("%sError, Invalid String Start index %d(0 -> %d)\n%s\n%s",
				myName,atixStart,DB_USERSTRINGS-1,DB_fn(fn),fn);
	if(arsz ==0)
		env->ThrowError("%sError, MUST provide list of Attrib's\n%s\n%s",myName,DB_fn(fn),fn);
	int atixEnd = atixStart + arsz - 1;
	if(atixEnd >= DB_USERSTRINGS)			
		env->ThrowError("%sError, Too Many Attrib's(Ends @%d, max idix=%d)\n%s\n%s",
				myName,atixEnd,DB_USERSTRINGS-1,DB_fn(fn),fn);
	char ebf[256]="";
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb+",db,env);
	int bfsz = db.ustrlen*arsz;
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
				if(slen > db.ustrlen) sprintf(ebf,"%sError, String Attrib(%d) Too Long(%d)",myName,argix+1,slen);
				else {
					char *p= bf + (argix*db.ustrlen);
					memcpy(p,s,slen);
					memset(p+slen,0,db.ustrlen-slen);
				}
			}
			if(*ebf!='\0')
				break;
		}
		if(ebf[0]=='\0') {
			if(fseek(fp, db.stroffset + atixStart * db.ustrlen,SEEK_SET))	sprintf(ebf,"Seeking DBase attribute(%d)",atixStart);
			else {if((fwrite(bf,bfsz,1,fp))!=1)				strcpy(ebf,"Updating DBase String attributes");}
		}
		delete [] bf;
	}
	fclose(fp);
	if(ebf[0]!='\0')	env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,DB_fn(fn),fn);
	return atixEnd-atixStart+1;
}



AVSValue __cdecl RT_DBaseGetField(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName="RT_DBaseGetField: ";
	const char * fn	= args[0].AsString();
	const int record= args[1].AsInt();
	const int field	= args[2].AsInt();
	char ebf[256]="";
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb",db,env);
	AVSValue var;
	const int records= db.records;
	const int fields = db.fields;
	int rd=0;
	if(record < 0 || record >= records)
		sprintf(ebf,"Invalid record %d(0 -> %d) @DB(%d:%d)",record,records-1,record,field);
	else if(fields < 1 || fields > DB_FIELDS)
		sprintf(ebf,"DB(%d) : Internal Error, Invalid number of fields %d(1 -> %d)",record,fields,DB_FIELDS);
	else if(field < 0 || field >= fields)
		sprintf(ebf,"Invalid field %d(0 -> %d) @DB(%d:%d)",field,fields-1,record,field);
	else {
		MYDBINF dbinf;
		if((fseek(fp, db.infoffset+field*sizeof(dbinf),SEEK_SET)) || (fread(&dbinf,sizeof(dbinf),1,fp)!=1)) {
			sprintf(ebf,"cannot read DBInf @DB(%d:%d)",record,field);
		} else {
			const int type		= dbinf.fieldtype;
			const int fieldsize	= dbinf.fieldsize;
			const int fieldoff	= dbinf.fieldoff;
			char bf[512+1],*bfp=bf;
			if(type<DB_BOOL || type > DB_DOUBLE)
				sprintf(ebf,"Internal Error, Illegal DBase Field type=%d",type);
			else if(fieldsize >= sizeof(bf) && ((bfp = new char[fieldsize + 1])==NULL))	// nul term needed for string
				strcpy(ebf,"Cannot Allocate Field buffer");
			else if(fseeko(fp,db.offset + int64_t(record) * db.recordsz + fieldoff,SEEK_SET))
				sprintf(ebf,"Cannot seek to DB(%d:%d)",record,field);
			else if(fread(bfp,fieldsize,1,fp)!=1)
				sprintf(ebf,"Cannot read field DB(%d:%d)",record,field);
			else {
				if(type==DB_INT)		{int  * p = (int*)  bfp;	var = *p;}
				else if(type==DB_FLOAT)	{float* p = (float*)bfp;	var = *p;}
				else if(type==DB_DOUBLE){double*p = (double*)bfp;	var = *p;}
				else if(type==DB_BOOL)	{char * p = (char*) bfp;	var = (*p !=0);}
				else if(type==DB_BIN)	{BYTE * p = (BYTE*) bfp;	var = (int)*p;}
				else					{bfp[fieldsize] = '\0'; var = env->SaveString(bfp);}
				rd=1;	// success
			}
			if(bfp != bf && bfp != NULL)	delete [] bfp;
		}
	}
	fclose(fp);
	if(rd!=1)			env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,DB_fn(fn),fn);
	return var;
}





AVSValue __cdecl RT_DBaseSetField(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseSetField: ";
	const char *fn		= args[0].AsString();
	const int record	= args[1].AsInt();
	const int fieldStart= args[2].AsInt();
	char ebf[256]="";
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb+",db,env);
	AVSValue at=args[3];
	const int arsz = at.ArraySize();
	static const char *ts[]={"Bool(0)","Int(1)","Float(2)","String(3)","Bin[as Int(1)]","Double[as Float(5)]"};
	int type=0;
	size_t wr=0;
	const int records = db.records;
	const int fields  = db.fields;
	if(record < 0 || record >= records)
		sprintf(ebf,"DB(%d:%d) : Invalid record %d(0 -> %d)",record,fieldStart,record,records-1);
	else if(fields < 1 || fields > DB_FIELDS)
		sprintf(ebf,"DB(%d) : Internal Error, Invalid number of fields %d(1 -> %d)",record,fields,DB_FIELDS);
	else if(fieldStart < 0 || fieldStart+arsz > fields)
		sprintf(ebf,"DB(%d:%d) : Invalid field %d(0 -> %d)",record,fieldStart,fieldStart+arsz-1,fields-1);
	else {
		int fld;
		wr=1;
		for(fld=0;fld<arsz && wr==1;++fld) {
			wr=0;
			int fldNo=fld+fieldStart;
			const int atyp = (at[fld].IsInt()) ? 1 : (at[fld].IsFloat()) ? 2 : (at[fld].IsBool()) ? 0 : (at[fld].IsString()) ? 3 : -1;
			if(atyp < 0) {
				sprintf(ebf,"DB(%d:%d) : Unknown Data Arg Type, expecting Type=%s",record,fldNo,ts[type]);
				break;
			}
			MYDBINF dbinf;
			if((fseek(fp, db.infoffset+fldNo*sizeof(dbinf),SEEK_SET)) || (fread(&dbinf,sizeof(dbinf),1,fp)!=1)) {
				sprintf(ebf,"cannot read DBInf @DB(%d:%d)",record,fldNo);
			} else {
				const int type		= dbinf.fieldtype;
				const int fieldsize	= dbinf.fieldsize;
				const int fieldoff	= dbinf.fieldoff;

				if(type< DB_BOOL || type > DB_DOUBLE)
					sprintf(ebf,"DB(%d:%d) : Internal Error, Illegal DBase Field type=%d",record,fldNo,type);
				else if(atyp!=type && !((type==DB_BIN && atyp==1) || (type==DB_DOUBLE && atyp==2)))
					sprintf(ebf,"DB(%d:%d) : Incorrect Data Type=%s, expecting %s",record,fldNo,ts[atyp],ts[type]);
				else if(fseeko(fp,db.offset + int64_t(record) * db.recordsz + fieldoff,SEEK_SET))
					sprintf(ebf,"DB(%d:%d) : Cannot seek to Field",record,fldNo);
				else {
					// *** MUST be in scope for the file write ***
					char Bool_t;
					int Int_t;
					float Float_t;
					double Double_t;
					BYTE Bin_t;
					char bf[512],*p=bf;
					//
					if(type == DB_STRING) {
						char * s = (char*)at[fld].AsString();
						const int len = int(strlen(s));
						if(len > fieldsize)
							sprintf(ebf,"DB(%d:%d) : String Too Long(%d, max=%d)",record,fldNo,len,fieldsize);
						else if(fieldsize > sizeof(bf) && ((p = new char[fieldsize])==NULL))	// nul term not needed
							sprintf(ebf,"DB(%d:%d) : Cannot Allocate String buffer",record,fldNo);
						else {
							memcpy(p,s,len);
							memset(p+len,0,fieldsize-len);
							wr=1;
						}
					} else {
						if		(type == DB_INT  )	{Int_t	 =at[fld].AsInt();				p = (char*) &Int_t;} 
						else if	(type == DB_FLOAT)	{Float_t =(float)at[fld].AsFloat();		p = (char*) &Float_t;} 
						else if	(type == DB_DOUBLE)	{Double_t=(double)at[fld].AsFloat();	p = (char*) &Double_t;} 
						else if	(type == DB_BOOL )	{Bool_t	 =at[fld].AsBool();				p = (char*) &Bool_t;}
						else						{Bin_t	 =(BYTE) (at[fld].AsInt());		p = (char*) &Bin_t; }
						wr=1;
					}
					if(wr == 1) {
						if((wr=fwrite(p,fieldsize,1,fp))!=1)
							sprintf(ebf,"DB(%d:%d) : Writing DBase Field",record,fldNo);
						if(type==DB_STRING && p!=bf)	delete [] p;
					}
				}
			}
		}
	}
	fclose(fp);
	if(wr !=1)			env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,DB_fn(fn),fn);
	return arsz;
}




AVSValue __cdecl RT_DBaseSet(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseSet: ";
	const char *fn		= args[0].AsString();
	const int record	= args[1].AsInt();
	char ebf[256]="";
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb+",db,env);
	static const char *ts[]={"Bool(0)","Int(1)","Float(2)","String(3)","Bin[as Int(1)]","Double[as Float(5)]"};
	const int arsz    = args[2].ArraySize();
	const int records = db.records;
	const int fields  = db.fields;
	const int recordsz= db.recordsz;
	const int maxfieldsz=db.maxfieldsz;
	size_t wr=0;
	MYDBINF *dbinf=NULL;
	char bf[512],*bfp=bf;
	int bfsz=sizeof(bf);
	if(recordsz>bfsz) {
		bfsz = recordsz;
		bfp  = new char[bfsz];
		if(bfp==NULL) {
			bfsz = maxfieldsz;
			if(sizeof(bf) >= maxfieldsz)	{bfp  = bf;}
			else							{bfp  = new char[bfsz];}
		}
	}

	if(bfp==NULL)
		sprintf(ebf,"DB(%d) : Cannot Allocate record/field buffer",record);	
	else if(record < 0 || record >= records)
		sprintf(ebf,"DB(%d) : Invalid record %d(0 -> %d)",record,record,records-1);
	else if(fields < 1 || fields > DB_FIELDS)
		sprintf(ebf,"DB(%d) : Internal Error, Invalid number of fields %d(1 -> %d)",record,fields,DB_FIELDS);
	else if(arsz != fields)
		sprintf(ebf,"DB(%d) : Incorrect number of field variables(%d) Expecting %d",record,arsz,fields);
	else if((dbinf= new MYDBINF[fields])==NULL)
		sprintf(ebf,"cannot allocate DBInf @DB(%d)",record);	
	else if(fseek(fp, db.infoffset,SEEK_SET) || fread(dbinf,fields*sizeof(dbinf[0]),1,fp)!=1)
		sprintf(ebf,"cannot read DBInf @DB(%d)",record);
	else if(fseeko(fp,db.offset + int64_t(record) * db.recordsz,SEEK_SET))
		sprintf(ebf,"DB(%d) : Cannot seek to Record",record);
	else {
		bool fieldsOnly = (bfsz < recordsz);
		size_t fwr=1;
		char * wrp=bfp;
		for(int fld=0; fwr == 1 && fld<fields;++fld) {
			fwr = 0;
			const int type		= dbinf[fld].fieldtype;
			const int fieldsize	= dbinf[fld].fieldsize;
			const int fieldoff	= dbinf[fld].fieldoff;
			AVSValue at	=args[2][fld];
			const int atyp	= (at.IsInt()) ? 1 : (at.IsFloat()) ? 2 : (at.IsBool()) ? 0 : (at.IsString()) ? 3 : -1;
			if(type< DB_BOOL || type > DB_DOUBLE)
				sprintf(ebf,"DB(%d:%d) : Internal Error, Illegal DBase Field Type=%d",record,fld,type);
			else if(atyp < 0)
				sprintf(ebf,"DB(%d:%d) : Unknown data arg Type, expecting Type=%s ",record,fld,ts[type]);
			else if(atyp!=type && !((type==DB_BIN && atyp==1) || (type==DB_DOUBLE && atyp==2)))
				sprintf(ebf,"DB(%d:%d) : Incorrect Data Type=%s, expecting %s",record,fld,ts[atyp],ts[type]);
			else {
				char * p;
				if(type == DB_STRING) {
					p = (char*)at.AsString();
					const int len = int(strlen(p));
					if(len > fieldsize)
						sprintf(ebf,"DB(%d:%d) : String Too Long for field(%d, max=%d)",record,fld,len,fieldsize);
					else {
						memcpy(wrp,p,len);
						memset(wrp+len,0,fieldsize-len);
						fwr = 1;
					}
				} else {
					int Bool_t,Int_t;
					float Float_t;
					double Double_t;
					BYTE Bin_t;
					if(type == DB_INT)			{Int_t	 =at.AsInt();			p=(char*)&Int_t;  }
					else if(type==DB_FLOAT)		{Float_t =(float)at.AsFloat();	p=(char*)&Float_t;}
					else if(type==DB_DOUBLE)	{Double_t=(double)at.AsFloat();	p=(char*)&Double_t;}
					else if(type == DB_BOOL)	{Bool_t	 =at.AsBool();			p=(char*)&Bool_t; }
					else						{Bin_t	 =(BYTE)at.AsInt();		p=(char*)&Bin_t;  }
					memcpy(wrp,p,fieldsize);
					fwr = 1;
				}
				if(fieldsOnly) {
					if(fwr == 1) {						
						if((fwr=fwrite(bfp,fieldsize,1,fp))!=1)	sprintf(ebf,"DB(%d:%d) : Writing field",record,fld);
						wr = (fld=fields-1) ? fwr : wr;
					}
				} else {
					wrp += fieldsize;
				}
			}
		}
		if(!fieldsOnly && fwr == 1) {
			if(wrp != bfp+recordsz)	sprintf(ebf,"DB(%d) : Internal Error, Record Buffer write mismatch",record);
			else if((wr=fwrite(bfp,recordsz,1,fp))!=1)	sprintf(ebf,"DB(%d) : Writing record",record);
		}
	}
	if(dbinf)						delete [] dbinf;
	if(bfp != bf && bfp != NULL)	delete [] bfp;
	fclose(fp);
	if(wr !=1)			env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,DB_fn(fn),fn);
	return db.records;
}

AVSValue __cdecl RT_DBaseExtend(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseExtend: ";
	const char *fn		= args[0].AsString();
	const int add		= args[1].AsInt(1);
	if(add < 0)	env->ThrowError("%sError, Cannot make smaller Add=%d\n%s\n%s",myName,add,DB_fn(fn),fn);
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb+",db,env);
	char ebf[256]="";
	size_t wr = 0;
	if(add == 0) wr=1;
	else {
		int64_t dfs = QueryMaxFileSize(fn);
		if(dfs < 0)	sprintf(ebf,"%sCannot query MaxFileSize",myName);
		else {
			int lim = int(std::min<long long int>(dfs / db.recordsz + db.records,0x7FFFFFFE0LL));
			int extmax = lim - db.records;
			if(add > extmax) {
				sprintf(ebf,"Add %d, Overflows available space(%d extend available)",add,extmax);
			} else {				
				int64_t clrbytes = int64_t(add) * db.recordsz;
				int bfsz = (clrbytes>(64*10240LL)) ? 64*1024 : int(clrbytes);
				BYTE *bf = new BYTE[bfsz];
				if(bf == NULL) {
					strcpy(ebf,"Allocating memory buffer");
				} else {
					int sods = int(clrbytes  / bfsz);
					int odds = int(clrbytes  % bfsz);
					memset(bf,0,bfsz);
					if(fseeko(fp,db.offset + int64_t(db.records) * db.recordsz,SEEK_SET))
						sprintf(ebf,"DB(%d) : Cannot seek to END Record",db.records);
					else {
						int i;
						for(wr=1,i = sods; wr == 1 && --i>=0; )
							wr=fwrite(bf,bfsz,1,fp);
						if(wr==1 && odds>0)		wr = fwrite(bf,odds,1,fp);
						if(wr != 1)				strcpy(ebf,"Writing DBase Extend data");
						else {
							rewind(fp);
							db.records += add;
							wr = fwrite(&db,sizeof(db),1,fp);
							if(wr != 1)	strcpy(ebf,"Updating DBase header");
						}
					}
					delete [] bf;
				}
			}
		}
	}
	fclose(fp);
	if(wr!=1)			env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,DB_fn(fn),fn);
	return db.records;
}



AVSValue __cdecl RT_DBaseAppend(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName	= "RT_DBaseAppend: ";
	const char *fn		= args[0].AsString();
	MYDB db;
	FILE * fp=DB_Read_Header(myName,fn,"rb+",db,env);
	char ebf[256]="";
	static const char *ts[]={"Bool(0)","Int(1)","Float(2)","String(3)","Bin[as Int(1)]","Double[as Float(5)]"};
	const int arsz    = args[1].ArraySize();
	const int records = db.records;
	const int fields  = db.fields;
	const int recordsz= db.recordsz;
	const int record  = db.records;				// OLD last record + 1 (record is zero relative, records is 1 relative)
	const int maxfieldsz=db.maxfieldsz;
	MYDBINF *dbinf= NULL;
	size_t wr=0;
	char bf[512],*bfp=bf;
	int bfsz=sizeof(bf);

	if(recordsz>bfsz) {
		bfsz = recordsz;
		bfp  = new char[bfsz];
		if(bfp==NULL) {
			bfsz = maxfieldsz;
			if(sizeof(bf) >= maxfieldsz)	{bfp  = bf;}
			else							{bfp  = new char[bfsz];}
		}
	}

	int64_t dfs = QueryMaxFileSize(fn);

	if(dfs < 0)	sprintf(ebf,"%sCannot query MaxFileSize",myName);
	else if(bfp==NULL) sprintf(ebf,"DB(%d) : Cannot Allocate record/field buffer",record);	
	else {
		int recmax = int(std::min<long long int>(dfs / db.recordsz + db.records,0x7FFFFFFE0LL));
		if(record >= recmax)			sprintf(ebf,"DB(%d) : Reached DBase available Limit %d",record,recmax);
		else if(fields < 1 || fields > DB_FIELDS)
			sprintf(ebf,"DB(%d) : Internal Error, Invalid number of fields %d(1 -> %d)",record,fields,DB_FIELDS);
		else if(arsz != fields)
			sprintf(ebf,"DB(%d) : Incorrect number of field variables(%d) Expecting %d",record,arsz,fields);
		else if((dbinf= new MYDBINF[fields])==NULL)
			sprintf(ebf,"cannot allocate DBInf @DB(%d)",record);	
		else if(fseek(fp, db.infoffset,SEEK_SET) || fread(dbinf,fields*sizeof(dbinf[0]),1,fp)!=1)
			sprintf(ebf,"cannot read DBInf @DB(%d)",record);
		else if(fseeko(fp,db.offset + int64_t(db.records) * db.recordsz,SEEK_SET))
			sprintf(ebf,"DB(%d) : Cannot seek to End Record",db.records);
		else {
			++db.records;				// New number of records
			int fwr=1;
			char * wrp=bfp;
			bool fieldsOnly = (bfsz < recordsz);
			for(int fld=0; fwr == 1 && fld<fields;++fld) {
				fwr = 0;
				const int type		= dbinf[fld].fieldtype;
				const int fieldsize	= dbinf[fld].fieldsize;
				const int fieldoff	= dbinf[fld].fieldoff;
				AVSValue at	=args[1][fld];
				const int atyp	= (at.IsInt()) ? 1 : (at.IsFloat()) ? 2 : (at.IsBool()) ? 0 : (at.IsString()) ? 3 : -1;
				if(type< DB_BOOL || type > DB_DOUBLE)
					sprintf(ebf,"DB(%d:%d) : Internal Error, Illegal DBase Field Type=%d",record,fld,type);
				else if(atyp < 0)
					sprintf(ebf,"DB(%d:%d) : Unknown data arg Type, expecting Type=%s ",record,fld,ts[type]);
				else if(atyp!=type && !((type==DB_BIN && atyp==1) || (type==DB_DOUBLE && atyp==2)))
					sprintf(ebf,"DB(%d:%d) : Incorrect Data Type=%s, expecting %s",record,fld,ts[atyp],ts[type]);
				else {
					char *p;
					if(type == DB_STRING) {
						p = (char*)at.AsString();
						const int len = int(strlen(p));
						if(len > fieldsize)
							sprintf(ebf,"DB(%d:%d) : String Too Long for field(%d, max=%d)",record,fld,len,fieldsize);
						else {
							memcpy(wrp,p,len);
							memset(wrp+len,0,fieldsize-len);
							fwr = 1;
						}
					} else {
						int Bool_t,Int_t;
						float Float_t;
						double Double_t;
						BYTE Bin_t;
						if(type == DB_INT)			{Int_t	 =at.AsInt();			p=(char*)&Int_t;  }
						else if(type==DB_FLOAT)		{Float_t =(float)at.AsFloat();	p=(char*)&Float_t;}
						else if(type==DB_DOUBLE)	{Double_t=(double)at.AsFloat();	p=(char*)&Double_t;}
						else if(type == DB_BOOL)	{Bool_t	 =at.AsBool();			p=(char*)&Bool_t; }
						else						{Bin_t	 =(BYTE)at.AsInt();		p=(char*)&Bin_t;  }
						memcpy(wrp,p,fieldsize);
						fwr = 1;
					}
					if(fieldsOnly) {
						if(fwr == 1) {						
							if((wr=fwrite(bfp,fieldsize,1,fp))!=1)	sprintf(ebf,"DB(%d:%d) : Writing field",record,fld);
						}
					} else {
						wrp += fieldsize;
					}
				}
			}
			if(fwr == 1) {
				if(fieldsOnly) {
					rewind(fp);
					if((wr = fwrite(&db,sizeof(db),1,fp)) != 1)		strcpy(ebf,"Updating DBase header");
				} else {
					if(wrp != bfp+recordsz)	sprintf(ebf,"DB(%d) : Internal Error, Record Buffer write mismatch",record);
					else if((wr=fwrite(bfp,recordsz,1,fp))!=1)	sprintf(ebf,"DB(%d) : Writing record",record);
					else {
						rewind(fp);
						if((wr = fwrite(&db,sizeof(db),1,fp)) != 1)		strcpy(ebf,"Updating DBase header");
					}
				}
			}
		}
	}
	if(dbinf)						delete [] dbinf;
	if(bfp != bf && bfp != NULL)	delete [] bfp;
	fclose(fp);
	if(wr !=1)			env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,DB_fn(fn),fn);
	return db.records;
}


AVSValue __cdecl RT_DBaseFindSeq(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName="RT_DBaseFindSeq: ";
	char ebf[256]="";
	MYDB db;
	MYDBINF * dbinf=NULL;
	const char * dbfn	  = args[0].AsString();
	FILE * dbfp=DB_Read_Header(myName,dbfn,"rb+",db,env);
	const int records= db.records;
	const int fields = db.fields;
	const int RecordSz = db.recordsz;
	const int StartField=args[1].AsInt();
	const int EndField=args[2].AsInt();
	const int Frame=args[3].AsInt();
	int low=args[4].AsInt(0);
	int high=args[5].AsInt(records-1);
	int Result=-1;
	if(StartField==EndField)						sprintf(ebf,"StartField(%d) Cannot be same as EndField",StartField);
	else if(low<0)									sprintf(ebf,"low(%d) record does not exist",low);
	else if(high >= records)						sprintf(ebf,"high(%d) record does not exist",high);
	else if((dbinf=new MYDBINF[fields])==NULL)		strcpy(ebf,"Cannot Allocate DBINF");
	else if((fseek(dbfp, db.infoffset,SEEK_SET)) || (fread(dbinf,fields*sizeof(*dbinf),1,dbfp)!=1))	strcpy(ebf,"Cannot read DBInf");
	else if (dbinf[StartField].fieldtype!=DB_INT)	strcpy(ebf,"StartField is not type Int");
	else if (dbinf[EndField].fieldtype!=DB_INT)		strcpy(ebf,"EndField is not type Int");
	else {
		const int StartFieldOff	= dbinf[StartField].fieldoff;
		const int EndFieldOff	= dbinf[EndField].fieldoff;
		while(low <= high && ebf[0]=='\0') {
			int t;
			int mid = (low + high) / 2;
			if(fseeko(dbfp,db.offset + int64_t(mid) * RecordSz + EndFieldOff ,SEEK_SET))
				{sprintf(ebf,"Cannot seek to DB(%d,%d)",mid,EndField); break;}
			size_t rd = fread(&t,sizeof(t),1,dbfp);
			if(rd != 1)
				{sprintf(ebf,"Cannot Read file @ DB(%d,%d)",mid,EndField); break;}
			if(t < Frame) {
				low = mid + 1;
			} else {
				if(fseeko(dbfp,db.offset + int64_t(mid) * RecordSz + StartFieldOff ,SEEK_SET))
					{sprintf(ebf,"Cannot seek to DB(%d,%d)",mid,StartField); break;}
				rd = fread(&t,sizeof(t),1,dbfp);
				if(rd != 1)
					{sprintf(ebf,"Cannot Read file @ DB(%d,%d)",mid,StartField); break;}
				if(t > Frame) {
					high = mid - 1;
				} else {
					Result = mid;									// found
					break;
				}
			}
		}
	}
	if(dbinf!=NULL)		delete [] dbinf;
	if(dbfp!=NULL)		fclose(dbfp);
	if(ebf[0])			env->ThrowError("%sError, %s",myName,ebf);
	return Result;
}
	

static int dbaseCompareFields_Lo(FILE *fp,char *ebf,const MYDB &db,const int &fcnt,const int f[3],const MYDBINF dbinf[3],
		char *fbf[2],
		const int a,const int b,const int field,const int field2,const int field3,const bool sig,
		char *pivotbf=NULL,int pivotrec=0) {
	int k,x = 0;
	if(ebf[0]==0 && a != b) {
		int offset = db.offset;
		int recordsz=db.recordsz;
		for(k=0;ebf[0]=='\0' && x==0 && k<fcnt;++k) {
			const int fld=f[k];
			const int type		= dbinf[k].fieldtype;
			const int fieldsize	= dbinf[k].fieldsize;
			const int fieldoff	= dbinf[k].fieldoff;
			if(pivotbf==NULL || pivotrec!=0) {
				if((fseeko(fp,offset + int64_t(a) * recordsz + fieldoff,SEEK_SET)) ||
					(fread(fbf[0],fieldsize,1,fp)!=1))		sprintf(ebf,"Cannot read Record1 field DB(%d:%d)",a,fld);
			} else {
				memcpy(fbf[0],pivotbf+fieldoff,fieldsize);	// Get field from pivotbf
			}
			if(ebf[0]!=0)	break;
			if(pivotbf==NULL || pivotrec!=1) {
				if((fseeko(fp,offset + int64_t(b) * recordsz + fieldoff,SEEK_SET)) ||
					(fread(fbf[1],fieldsize,1,fp)!=1))		sprintf(ebf,"Cannot read Record2 field DB(%d:%d)",b,fld);
			} else {
				memcpy(fbf[1],pivotbf+fieldoff,fieldsize);	// Get field from pivotbf
			}
			if(ebf[0]==0) {
				if(type==DB_INT)		{
					int  * p1 = (int*)  fbf[0];
					int  * p2 = (int*)  fbf[1];
					x = (*p1<*p2) ? -1 : (*p1>*p2) ? 1 : 0;
				} else if(type==DB_FLOAT)	{
					float* p1 = (float*)fbf[0];
					float* p2 = (float*)fbf[1];
					x = (*p1<*p2) ? -1 : (*p1>*p2) ? 1 : 0;
				} else if(type==DB_DOUBLE){
					double*p1 = (double*)fbf[0];
					double*p2 = (double*)fbf[1];
					x = (*p1<*p2) ? -1 : (*p1>*p2) ? 1 : 0;
				} else if(type==DB_BOOL)	{
					char * p1 = (char*) fbf[0];
					char * p2 = (char*) fbf[1];
					int b1=(*p1!=0) ? 1 : 0;
					int b2=(*p2!=0) ? 1 : 0;
					x = (*p1<*p2) ? -1 : (*p1>*p2) ? 1 : 0;
				} else if(type==DB_BIN)	{
					BYTE * p1 = (BYTE*) fbf[0];
					BYTE * p2 = (BYTE*) fbf[1];
					x = (*p1<*p2) ? -1 : (*p1>*p2) ? 1 : 0;
				} else					{
					fbf[0][fieldsize] = '\0';
					fbf[1][fieldsize] = '\0';
					x = (sig)
						? strcmp(fbf[0],fbf[1])
						: strcasecmp(fbf[0],fbf[1]);
					x = (x<0) ? -1 : (x>0) ? 1 : 0;
				}
			}
		}
	}
    return x;
}

static int dbaseSwapRecords_Lo(FILE *fp,char *ebf,const MYDB &db,char *recbf[2],const int bfsz,const int a,const int b) {
	int swapped = 0;
	if(ebf[0]==0 && a != b) {
		int64_t off1 = db.offset + int64_t(a)*db.recordsz;
		int64_t off2 = db.offset + int64_t(b)*db.recordsz;
		const int sods = db.recordsz / bfsz;
		const int odds = db.recordsz % bfsz;
		for(int s=0; s<sods && ebf[0]==0 ;++s) {
			if((fseeko(fp,off1,SEEK_SET))||(fread(recbf[0],bfsz,1,fp)!=1))		sprintf(ebf,"Cannot read  Record1 DB(%d)",a);
			else if((fseeko(fp,off2,SEEK_SET))||(fread(recbf[1],bfsz,1,fp)!=1))	sprintf(ebf,"Cannot read  Record2 DB(%d)",b);
			else if((fseeko(fp,off1,SEEK_SET))||(fwrite(recbf[1],bfsz,1,fp)!=1))	sprintf(ebf,"Cannot write Record1 DB(%d)",a);
			else if((fseeko(fp,off2,SEEK_SET))||(fwrite(recbf[0],bfsz,1,fp)!=1))	sprintf(ebf,"Cannot write Record2 DB(%d)",b);
			off1 += bfsz;
			off2 += bfsz;
		}
		if(ebf[0]==0 && odds > 0) {
			if((fseeko(fp,off1,SEEK_SET))||(fread(recbf[0],odds,1,fp)!=1))		sprintf(ebf,"Cannot read  Record1 DB(%d)",a);
			else if((fseeko(fp,off2,SEEK_SET))||(fread(recbf[1],odds,1,fp)!=1))	sprintf(ebf,"Cannot read  Record2 DB(%d)",b);
			else if((fseeko(fp,off1,SEEK_SET))||(fwrite(recbf[1],odds,1,fp)!=1))	sprintf(ebf,"Cannot write Record1 DB(%d)",a);
			else if((fseeko(fp,off2,SEEK_SET))||(fwrite(recbf[0],odds,1,fp)!=1))	sprintf(ebf,"Cannot write Record2 DB(%d)",b);
		}
		swapped = 1;
	}
	return swapped;
}


AVSValue __cdecl RT_DBaseInsertSort(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName     = "RT_InsertSort: ";
	char ebf[256]="";
	MYDB db;
	const char * fn			= args[0].AsString();
	FILE * fp=DB_Read_Header(myName,fn,"rb+",db,env);
	const int recS			= args[1].AsInt(0);
	const int recE			= args[2].AsInt(db.records-1);
	const int field			= args[3].AsInt(0);
	int field2_t			= args[4].AsInt(field);
	int field3_t			= args[5].AsInt(field2_t);
	const bool sig			= args[6].AsBool(true);
	const bool ascend		= args[7].AsBool(true);
	const bool debug		= args[8].AsBool(false);
	const int field3 = (field3_t==field2_t) ? -1 : field3_t;
	const int field2 = (field2_t==field)    ? -1 : field2_t;
	const int records = db.records;
	const int fields  = db.fields;
	const int recordsz= db.recordsz;
	if (recE<0 || recE>=records)							sprintf(ebf,"Invalid RecE(%d)",recE);
	else if (recS<0 || recS>recE)							sprintf(ebf,"Invalid RecS(%d)",recS);
	else if (field<0 || field>=fields)						sprintf(ebf,"Invalid Field(%d)",field);
	else if (field2<-1 || field2>=fields)					sprintf(ebf,"Invalid Field2(%d)",field2);
	else if (field3==field || field3<-1 || field3>=fields)	sprintf(ebf,"Invalid Field3(%d)",field3);
	else {
		int f[3]={field,field2,field3};
		MYDBINF dbinf[3];
		char *fbf[2]={NULL,NULL};
		double start_time =  RT_TimerHP_Lo();
		int fcnt=(field2<0)?1:(field3<0)?2:3;
		char *recbf[2]={NULL,NULL};
		int i;
		int maxfldsz=0;
		for(i=0;i<fcnt&&ebf[0]==0;++i) {
			if(f[i] >= 0) {
				if((fseek(fp, db.infoffset+f[i]*sizeof(dbinf[0]),SEEK_SET)) || (fread(&dbinf[i],sizeof(dbinf[0]),1,fp)!=1))
					sprintf(ebf,"cannot read DBInf[%d]",f[i]);
				else {
					maxfldsz=std::max(dbinf[i].fieldsize,maxfldsz);
				}
			}
		}
		const int bfsz = std::min(recordsz,16*1024);
		for(i=0;i<2&&ebf[0]==0;++i) {if((fbf[i] = new char[maxfldsz+1]) == NULL)	sprintf(ebf,"cannot alloc field buffer");}
		for(i=0;i<2&&ebf[0]==0;++i) {if((recbf[i] = new char[bfsz]) == NULL)		sprintf(ebf,"cannot alloc record buffer");}
		int incr = (ascend) ? 1 : -1;
		int low =recS,high=recE;
		if(debug)	dprintf("%sCommencing Sort",myName);
		for(i=low+1 ; ebf[0]==0 && i<=high ; ++i) {
			int j=i, x=incr;
            while(ebf[0]==0 && j > low && x==incr) {
				x = dbaseCompareFields_Lo(fp,ebf,db,fcnt,f,dbinf,fbf,j-1,j,field,field2,field3,sig);
				if(ebf[0]!=0)	break;
				if(x == incr) {
					dbaseSwapRecords_Lo(fp,ebf,db,recbf,bfsz,j - 1, j);
					if(ebf[0]!=0)	break;
					--j;
				} else {
					j = low;		// Swap not needed, Abort inner pass, everything before is already sorted.
				}
			}
		}
		for(i=0;i<2;++i)	{if(recbf[i]!=NULL) {delete [] recbf[i]; recbf[i]=NULL;}}
		for(i=0;i<2;++i)	{if(fbf[i]!=NULL) {delete [] fbf[i]; fbf[i]=NULL;}}
		if(ebf[0]==0 && debug) {
			double duration =  RT_TimerHP_Lo() - start_time;
			dprintf("%sScan Total Time=%.3f secs (%.3fmins)\n",myName,duration,duration/60.0);
		}
	}
	fclose(fp);
	if(ebf[0])			env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,DB_fn(fn),fn);
	return 0;
}


static void dbaseQuicksort_Lo(FILE *fp,char *ebf,const MYDB &db,const int &fcnt,const int f[3],const MYDBINF dbinf[3],
		char *fbf[2],char *recbf[2],const int bfsz,
		int low,int high,const int field,const int field2,const int field3,const bool sig,int incr,char *pivotbf=NULL,
		const int ins=-1,const bool randPivot=false) {
	bool escape=false;
	while(ebf[0]==0 && high > low) {	// While two or more records
		int nel = high - low + 1;
		if(nel <= ins) { // less than ins elements, insert sort.
            for(int i=low+1;ebf[0]==0 && i<=high;++i) {
                int j = i;
				int x = incr;
                while(ebf[0]==0 && j > low && x==incr) {
					x = dbaseCompareFields_Lo(fp,ebf,db,fcnt,f,dbinf,fbf,j-1,j,field,field2,field3,sig);
					if(ebf[0]!=0) break;
                    if(x==incr) {
						dbaseSwapRecords_Lo(fp,ebf,db,recbf,bfsz,j - 1, j);
						if(ebf[0]!=0) break;
						--j;
					} else {                 // Swap not needed, Abort inner pass, everything before is already sorted.
						j=low;
					}
                }
            }
            high=low;												// terminate while(high > low)
		} else {
			int pivot = (randPivot) ? low+RandInt_Lo(nel) : low + (high-low)/2;	// pivot element that we will use
			dbaseSwapRecords_Lo(fp,ebf,db,recbf,bfsz,low,pivot);	// Swap pivot 'out of the way'
            pivot=low;												// We swapped pivot to low
			if(pivotbf!=NULL) {
				// buffer pivot, for use in comparisons
				int64_t off1 = db.offset + int64_t(pivot)*db.recordsz;
				if((fseeko(fp,off1,SEEK_SET))||(fread(pivotbf,bfsz,1,fp)!=1)) {
					sprintf(ebf,"Cannot read  Record DB(%d)",pivot);
					pivotbf=NULL;
				}
			}
            int i=low+1;											// Start at Pivot + 1
            int j=high;
            int p=i-1;
            int q=j+1;
			// Separate into 4 parts (when ascend), pivot : less than : greater than : pivot.
            while(ebf[0]==0 && i <= j) {
				int istat=incr;
                while(ebf[0]==0 && istat==incr && i <= high) {		// Until DB[i] >= DB[pivot]
					istat = dbaseCompareFields_Lo(fp,ebf,db,fcnt,f,dbinf,fbf,pivot,i,field,field2,field3,sig,pivotbf,0);
                    if(ebf[0]==0 && istat==incr)	++i;
					else						break;
                }
				int jstat = incr;
                while(ebf[0]==0 && jstat==incr && j>pivot) {		// Until DB[j] <= DB[Pivot]
					jstat = dbaseCompareFields_Lo(fp,ebf,db,fcnt,f,dbinf,fbf,j,pivot,field,field2,field3,sig,pivotbf,1);
                    if(ebf[0]==0 && jstat==incr)	--j;
					else						break;
                }
				if(ebf[0]!=0) break;
                if(i < j) {
					if(istat==0 && jstat==0) {		// db[i] == db[pivot] and db[j] == db[pivot] (both correct side)
						++p;
						dbaseSwapRecords_Lo(fp,ebf,db,recbf,bfsz,p,i);  // swap db[i] with less than pivot group
						if(ebf[0] != 0)	break;
						--q;
						dbaseSwapRecords_Lo(fp,ebf,db,recbf,bfsz,j,q);  // swap db[j] with greater than pivot group
						if(ebf[0] != 0)	break;
					} else {											// One or both DV[i],DB[j] NOT equal to DB[Pivot]
						dbaseSwapRecords_Lo(fp,ebf,db,recbf,bfsz,i,j);  // Swap DB[i] & DB[j] (move the not equal one/both to correct side)
						if(ebf[0] != 0)	break;
                        if(istat==0 && jstat!=0) {
							--q;
							dbaseSwapRecords_Lo(fp,ebf,db,recbf,bfsz,j,q);  // swap original db[i], now db[j]<->db[q]
							if(ebf[0] != 0)	break;
                        } else if(istat!=0 && jstat==0) {
                            p=p+1;
							dbaseSwapRecords_Lo(fp,ebf,db,recbf,bfsz,p,i);  // swap original db[j], now db[i]<->db[p]
							if(ebf[0] != 0)	break;
                        } // Else if both DB[i]!=DB[Pivot] and DB[j]!=DB{Pivot] then have already swapped i,j to correct sides.
					}
                    ++i;
                    --j;
                } else if(i == j) {
                    ++i;
                    --j;
                    // If i WAS equal to j, then possible that ALL items are same as pivot,
                    // we check for number above and below pivot to see if we can avoid Partition by 4 to Partition by 3 moves.
                    // If all items are same as pivot, then we are done and can escape.
					if(j<p+1 && q-1<i) {
						escape=true;
						high=low; // DONE
					}
                    break;
                }
            } // End while(ebf[0]==0 && i <= j)
			if(!escape) {
				// Here if ascend: DB[low]->DB[p]==Pivot : DB[p+1]->DB[j]<Pivot : DB[i]->DB[q-1]>Pivot : DB[q]->DB[high]==Pivot
				// Swap high same as pivot group into center, swapping with greater than pivot middle group.
				for(int k=low;ebf[0]==0 && k<=p;++k)	{
					dbaseSwapRecords_Lo(fp,ebf,db,recbf,bfsz,k,j);
					--j;
				}
				// Swap high same as pivot group into center, swapping with greater than pivot middle group.
				for(int k=high;ebf[0]==0 && k>=q;--k)	{
					dbaseSwapRecords_Lo(fp,ebf,db,recbf,bfsz,i,k);
					++i;
				}
				if(ebf[0] != 0)	break;
				//  Here if ascend, DB[low]->DB[j] < pivot : DB[j+1]->DB[i-1] same as pivot : DB[i]->DB[high] > pivot
				int nelLo = j-low + 1;
				int nelHi = high-i+ 1;
				// Recurse smaller partition, iterate larger (center, 'same as pivot' partition already in final position).
				if(nelHi>nelLo) {
					if(nelLo>=2) {
						dbaseQuicksort_Lo(fp,ebf,db,fcnt,f,dbinf,fbf,recbf,bfsz,low,j,field,field2,field3,sig,incr,pivotbf,ins,randPivot);
					}
					low=i;		// iterate greater than pivot partition
				} else {
					if(nelHi>=2) {
						dbaseQuicksort_Lo(fp,ebf,db,fcnt,f,dbinf,fbf,recbf,bfsz,i,high,field,field2,field3,sig,incr,pivotbf,ins,randPivot);
					}
					high=j;		// iterate less than pivot partition
				}
			}
		} // End if(nel <= ins) // less than ins elements, insert sort.
	} // End While(low < high)
}

AVSValue __cdecl RT_DBaseQuickSort(AVSValue args, void* user_data, IScriptEnvironment* env) {
/*
QuickSort Three-Way Partitioning.
  Modification of Bentley-McIlroy partitioning which was modified by Robert Sedgewick, and further modified.
   Chosen pivot is low + (high-low)/2, which is swapped with low record and comparisons done on that and not
   on a copy of pivot record in memory (possibly up to max 262,144KB, 1024 fields of 256KB each).

   Any comments should be taken as if sort ascending = true.

   x or less records use insert sort, otherwise,

   At first four partitions are created:
    some of the elements that are equal to the pivot;
    the element that are "less" than the pivot;
    the elements that are "greater" than the pivot;
    the rest of the elements that are equal to the pivot.

	later, outer elements equal to (and including) pivot are swapped into centre (final position) and then the smaller
	sized greater or lesser partition is recursively sorted, and the greater sized iteratively sorted.

*/
	const char * myName     = "RT_DBaseQuickSort: ";
	char ebf[256]="";
	MYDB db;
	const char * fn			= args[0].AsString();
	FILE * fp=DB_Read_Header(myName,fn,"rb+",db,env);
	const int recS			= args[1].AsInt(0);
	const int recE			= args[2].AsInt(db.records-1);
	const int field			= args[3].AsInt(0);
	int field2_t			= args[4].AsInt(field);
	int field3_t			= args[5].AsInt(field2_t);
	const bool sig			= args[6].AsBool(true);
	const bool ascend		= args[7].AsBool(true);
	const bool debug		= args[8].AsBool(false);
	const int ins			= args[9].AsInt(-1);
	const bool randPivot	= args[10].AsBool(false);
	const int field3 = (field3_t==field2_t) ? -1 : field3_t;
	const int field2 = (field2_t==field)    ? -1 : field2_t;
	const int records = db.records;
	const int fields  = db.fields;
	const int recordsz= db.recordsz;
	if (recE<0 || recE>=records)							sprintf(ebf,"Invalid RecE(%d)",recE);
	else if (recS<0 || recS>recE)							sprintf(ebf,"Invalid RecS(%d)",recS);
	else if (field<0 || field>=fields)						sprintf(ebf,"Invalid Field(%d)",field);
	else if (field2<-1 || field2>=fields)					sprintf(ebf,"Invalid Field2(%d)",field2);
	else if (field3==field || field3<-1 || field3>=fields)	sprintf(ebf,"Invalid Field3(%d)",field3);
	else {
		int f[3]={field,field2,field3};
		MYDBINF dbinf[3];
		char *fbf[2]={NULL,NULL};
		char *pivotbf=NULL;
		double start_time =  RT_TimerHP_Lo();
		int fcnt=(field2<0)?1:(field3<0)?2:3;
		char *recbf[2]={NULL,NULL};
		int i;
		int maxfldsz=0;
		for(i=0;i<fcnt&&ebf[0]==0;++i) {
			if(f[i] >= 0) {
				if((fseek(fp, db.infoffset+f[i]*sizeof(dbinf[0]),SEEK_SET)) || (fread(&dbinf[i],sizeof(dbinf[0]),1,fp)!=1))
					sprintf(ebf,"cannot read DBInf[%d]",f[i]);
				else {
					maxfldsz=std::max(dbinf[i].fieldsize,maxfldsz);
				}
			}
		}
		const int bfsz = std::min(recordsz,16*1024);
		for(i=0;i<2&&ebf[0]==0;++i) {if((fbf[i] = new char[maxfldsz+1]) == NULL)	sprintf(ebf,"cannot alloc field buffer");}
		for(i=0;i<2&&ebf[0]==0;++i) {if((recbf[i] = new char[bfsz]) == NULL)		sprintf(ebf,"cannot alloc record buffer");}
		if(bfsz==recordsz) {
			pivotbf = new char[bfsz];
		}
        int incr = (ascend)  ? 1 : -1;
		if(debug)	dprintf("%sCommencing Sort (ins=%d : RandPivot=%s)",myName,ins,randPivot?"T":"F");
		dbaseQuicksort_Lo(fp,ebf,db,fcnt,f,dbinf,fbf,recbf,bfsz,recS,recE,field,field2,field3,sig,incr,pivotbf,ins,randPivot);
		if(pivotbf!=NULL)	{delete [] pivotbf; pivotbf=NULL;}
		for(i=0;i<2;++i)	{if(recbf[i]!=NULL) {delete [] recbf[i]; recbf[i]=NULL;}}
		for(i=0;i<2;++i)	{if(fbf[i]!=NULL) {delete [] fbf[i]; fbf[i]=NULL;}}
		if(ebf[0]==0 && debug) {
			double duration =  RT_TimerHP_Lo() - start_time;
			dprintf("%sScan Total Time=%.3f secs (%.3fmins)\n",myName,duration,duration/60.0);
		}
	}
	fclose(fp);
	if(ebf[0])			env->ThrowError("%sError, %s\n%s\n%s",myName,ebf,DB_fn(fn),fn);
	return 0;
}


AVSValue __cdecl RT_DBaseReadCSV(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName="RT_DBaseReadCSV: ";
	const char * dbfn	  = args[0].AsString();
	const char * txtfn	  = args[1].AsString();
	const unsigned char * commaStr = (unsigned char*)	args[2].AsString(",");
	const char *StrDelimiter=args[3].AsString("\"");
	const int StartField=args[4].AsInt(0);
	int EndField=args[5].AsInt(-1);
	const int    comma	  = (*commaStr==9)?32:*commaStr;
	if(comma==10||comma==13||comma==34||comma==0)	env->ThrowError("%sIllegal Separator[Chr(%d)]",myName,comma);
	const int StrDelimiterLen=(int)strlen(StrDelimiter);
	if(StrDelimiterLen==0)			env->ThrowError("%sIllegal zero length StrDelimiter]",myName);
	int i;
	for(i=0;i<StrDelimiterLen;++i) {
		if(StrDelimiter[i]==32)	    env->ThrowError("%sIllegal character StrDelimiter[%d]=SPACE",myName,i+1);
		if(StrDelimiter[i]==9)	    env->ThrowError("%sIllegal character StrDelimiter[%d]=TAB",myName,i+1);
		if(StrDelimiter[i]==10)		env->ThrowError("%sIllegal character StrDelimiter[%d]=Chr(10)",myName,i+1);
		if(StrDelimiter[i]==13)		env->ThrowError("%sIllegal character StrDelimiter[%d]=Chr(13)",myName,i+1);
		if(StrDelimiter[i]==comma)	env->ThrowError("%sIllegal character StrDelimiter[%d]=Separator",myName,i+1);
	}
	char ebf[256]="";
	FILE *txtfp=NULL;
	enum {TSZ=64*1024,TXTBFSZ};
	char *txtbf=NULL;
	long flen,chk;
	int field=0,line=0,added=0;
	// we use read in binary mode so C does not mess with file size / seek etc.
	if(!(txtfp = fopen(txtfn, "rb")))										strcpy(ebf,"Cannot Open CSV file");
	else if((fseek(txtfp, 0, SEEK_END)!=0) || ((flen=ftell(txtfp)) == -1L))	strcpy(ebf,"Cannot seek in CSV file");
	else if(rewind(txtfp), (chk=std::min<long int>(flen,3))!=0) {
		char tmpbf[16];
		long rd;
		if((rd = (long)fread(tmpbf, 1, chk, txtfp)) != chk)							
			strcpy(ebf,"Cannot read from CSV file");
		else if(chk==3 && tmpbf[0]==0xEF && tmpbf[1]==0xBB && tmpbf[2]==0xBF)	
			strcpy(ebf,"UTF-8 CSV files are not supported, re-save with ANSI encoding!");
		else if((chk>=2 && ((tmpbf[0]==0xFF && tmpbf[1]==0xFE) || (tmpbf[0]==0xFE && tmpbf[1]==0xFF))))
			strcpy(ebf,"Unicode text files are not supported, re-save with ANSI encoding!");
		else {
			fclose(txtfp); txtfp=NULL;
			MYDBINF * dbinf=NULL;
			BYTE *clrbf   = NULL;
			char *fieldbf = NULL;
			MYDB db;
			FILE * dbfp=DB_Read_Header(myName,dbfn,"rb+",db,env);
			const int records= db.records;
			const int fields = db.fields;
			const int recordsz=db.recordsz;
			if(EndField == -1) EndField = fields-1;
			int64_t clrbytes = db.recordsz;
			int clrbfsz = (clrbytes>TSZ) ? TSZ : int(clrbytes);
			const int sods = int(clrbytes  / clrbfsz);
			const int odds = int(clrbytes  % clrbfsz);
			int64_t dfs = QueryMaxFileSize(dbfn);
			int lim = int(std::min<long long int>(dfs / db.recordsz + db.records,0x7FFFFFFE0LL));
			if(StartField<0)	
				strcpy(ebf,"-ve StartField");
			else if(StartField>=fields)	
				strcpy(ebf,"StartField does not exist");
			else if(EndField < -1)	
				strcpy(ebf,"-ve EndField, -1 only");
			else if(EndField < StartField)	
				strcpy(ebf,"EndField < StartField");
			else if(EndField >= fields)	
				strcpy(ebf,"EndField does not exist");
			else if(dfs < 0)									
				strcpy(ebf,"Cannot query MaxFileSize");
			else if((dbinf=new MYDBINF[fields])==NULL)	
				strcpy(ebf,"Cannot Allocate DBINF");
			else if((fseek(dbfp, db.infoffset,SEEK_SET)) || (fread(dbinf,fields*sizeof(*dbinf),1,dbfp)!=1))
				strcpy(ebf,"Cannot read DBInf");
			// Now we use read in Text mode, will halt at 1st nul byte if binary file.
			else if(!(txtfp = fopen(txtfn, "rt")))
				strcpy(ebf,"Cannot reopen CSV file");
			else if((clrbf = new BYTE[clrbfsz]) == NULL)
				strcpy(ebf,"Cannot allocate clear buffer");
			else if((fieldbf=new char[db.maxfieldsz])==NULL)
				strcpy(ebf,"Cannot allocate field buffer");
			else if((txtbf=new char[TXTBFSZ])==NULL)
				strcpy(ebf,"Cannot allocate text buffer");
			else {
				char *ps;
				memset(clrbf,0,clrbfsz);
				memset(txtbf,0,TXTBFSZ);
				memset(fieldbf,0,db.maxfieldsz);
				while(ebf[0]=='\0') {
					field=StartField;
					ps=txtbf;
					// will return non n/l term last line : will halt at first Nul byte if bin file
					char * gotp=fgets(txtbf,TXTBFSZ,txtfp );
					if(gotp==NULL)	break;
					++line;
					while(*ps==32||*ps==9)++ps;								// Eat white
					if(*ps=='\n' || *ps=='\0' || *ps=='#')
						continue;											// empty or comment line only
					if(db.records+1 > lim) {
						strcpy(ebf,"Overflows available disk space");
					} else {				
						if(fseeko(dbfp,db.offset + int64_t(db.records) * db.recordsz,SEEK_SET))
							strcpy(ebf,"Cannot seek to END Record");
						else {
							int i,wr;
							for(wr=1,i = sods; wr == 1 && --i>=0; )
								wr=(int)fwrite(clrbf,clrbfsz,1,dbfp);
							if(wr==1 && odds>0)		wr = (int)fwrite(clrbf,odds,1,dbfp);
							if(wr != 1)				strcpy(ebf,"Writing DBase Extend data");
							else {
								rewind(dbfp);
								++db.records;
								++added;
								wr = (int)fwrite(&db,sizeof(db),1,dbfp);
								if(wr != 1)	strcpy(ebf,"Updating DBase header");
							}
						}
					}
					while(ebf[0]=='\0' && field<=EndField) {
						const int record    = db.records-1;
						const int type      = dbinf[field].fieldtype;
						const int fieldsize	= dbinf[field].fieldsize;
						const int fieldoff	= dbinf[field].fieldoff;
						if(type<DB_BOOL || type > DB_DOUBLE)
							strcpy(ebf,"Internal Error, Illegal DBase Field type");
						else if(fseeko(dbfp,db.offset + int64_t(record) * db.recordsz + fieldoff,SEEK_SET))
							strcpy(ebf,"Cannot seek in DB");
						else {
							char *stopstring;
							while(*ps==32||*ps==9)++ps;						// Eat white
							if(type==DB_INT || type == DB_BIN) {
								bool neg=false;
								if(*ps == '-' || *ps=='+') {
									neg=(*ps == '-');
									++ps;
									while(*ps==32||*ps==9)++ps;						// Eat white
								}
								int base = 10;
								if(*ps=='$') {
									++ps;base=16;
									while(*ps==32||*ps==9)++ps;						// Eat white
								}
								int num;
								if(base==10) {
									if(*ps>='0' && *ps<='9') {
										if (type == DB_BIN) {
											num=strtoul(ps,&stopstring,base);
											num=(neg)?-num:num;
											BYTE bin=BYTE(num);
											ps=stopstring;
											if((fwrite(&bin,sizeof(bin),1,dbfp))!=1)	strcpy(ebf,"Error writing DBase Bin Field");
										} else {
											num=strtol(ps,&stopstring,base);
											num=(neg)?-num:num;
											ps=stopstring;
											if((fwrite(&num,sizeof(num),1,dbfp))!=1)	strcpy(ebf,"Error writing DBase Int Field");
										}
									} else {
										if (type == DB_BIN)	strcpy(ebf,"Expecting CSV Bin Number");
										else				strcpy(ebf,"Expecting CSV Int Number");
									}
								} else {
									if((*ps>='0' && *ps<='9')||(*ps>='a'&&*ps<='f')||(*ps>='A'&&*ps<='F')) {
										if (type == DB_BIN) {
											num=strtoul(ps,&stopstring,base);
											num=(neg)?-num:num;
											BYTE bin=BYTE(num);
											ps=stopstring;
											if((fwrite(&bin,sizeof(bin),1,dbfp))!=1)	strcpy(ebf,"Error writing DBase Bin Field");
										} else {
											num = strtoul(ps,&stopstring,base);
											num=(neg)?-num:num;
											ps=stopstring;
											if((fwrite(&num,sizeof(num),1,dbfp))!=1)	strcpy(ebf,"Error writing DBase Int Field");
										}
									} else {
										if (type == DB_BIN)	strcpy(ebf,"Expecting CSV Hex Bin Number");
										else				strcpy(ebf,"Expecting CSV Hex Int Number");
									}
								}
							} else if (type == DB_FLOAT||type == DB_DOUBLE)	{
								bool neg=false;
								if(*ps == '-' || *ps=='+') {
									neg=(*ps == '-');
									++ps;
									while(*ps==32||*ps==9)++ps;						// Eat white
								}
								if(*ps=='.'||(*ps>='0' && *ps<='9')) {
									double dbl=strtod(ps,&stopstring);
									dbl = (neg) ? -dbl : dbl;
									ps=stopstring;
									if(type == DB_DOUBLE) {
										if((fwrite(&dbl,sizeof(dbl),1,dbfp))!=1)	strcpy(ebf,"Error writing DBase Double Field");
									} else {
										float flt=float(dbl);
										if((fwrite(&flt,sizeof(flt),1,dbfp))!=1)	strcpy(ebf,"Error writing DBase Float Field");
									}
								} else {
									strcpy(ebf,"Expecting CSV Float Number");
								}
							} else if (type == DB_BOOL)	{
								BYTE bol;
								if((ps[0]=='t'||ps[0]=='T')&&(ps[1]=='r'||ps[1]=='R')&&(ps[2]=='u'||ps[2]=='U')&&(ps[3]=='e'||ps[3]=='E')) {
									bol=1;
									ps+=4;
									if((fwrite(&bol,sizeof(bol),1,dbfp))!=1)	strcpy(ebf,"Error writing DBase Bool Field");
								} else if((ps[0]=='f'||ps[0]=='F')&&(ps[1]=='a'||ps[1]=='A')&&(ps[2]=='l'||ps[2]=='L')
									&&(ps[3]=='s'||ps[3]=='S')&&(ps[4]=='e'||ps[4]=='E')) {
									bol=0;
									ps+=5;
									if((fwrite(&bol,sizeof(bol),1,dbfp))!=1)	strcpy(ebf,"Error writing DBase Bool Field");
								} else {
									strcpy(ebf,"Expecting CSV Bool");
								}
							} else if (type == DB_STRING)	{
								char *pfnd=strstr(ps,StrDelimiter);
								if(pfnd==ps) {
									ps+=StrDelimiterLen;
									pfnd=strstr(ps,StrDelimiter);
									if(pfnd!=NULL) {
										int len =int(pfnd-ps);
										if(len<=fieldsize) {
											if(len>0 && ((fwrite(ps,len,1,dbfp))!=1))
												strcpy(ebf,"Error writing DBase String Field");
											else 
												ps=pfnd+StrDelimiterLen;	// skip closing deliliter
										} else {
											strcpy(ebf,"Error CSV String too big for field");
										}
									} else {
										strcpy(ebf,"Expecting CSV String closing string delimiter");
									}
								} else {
									strcpy(ebf,"Expecting CSV String opening string delimiter");
								}
							} 
							if(ebf[0]=='\0') {
								bool hasSpace=(*ps==32 || *ps==9);
								while(*ps==32||*ps==9)++ps;
								++field;
								if(field <= EndField) {
									if(*ps==comma) {
										++ps;									// Eat Comma
										while(*ps==32||*ps==9)++ps;				// Eat white
										if(*ps=='\n' || *ps=='\0' || *ps=='#')	strcpy(ebf,"Expecting CSV data");
									} else if(hasSpace) {						// Accept SPACE and TAB as alternative separator
										if(*ps=='\n' || *ps=='\0' || *ps=='#')	strcpy(ebf,"Expecting CSV data");
									} else {
										strcpy(ebf,"Expecting CSV Separator");
									}
								} else {
									if(*ps=='#') {
										*ps='\0';							// Eat Comment
									} else if(*ps=='\r' || *ps=='\n') {
										*ps='\0';							// Eat n/l
									}
									if(*ps!='\0')							strcpy(ebf,"Unexpected CSV Data");
								}
							}
						} // End, Seek DBInf
					} // End, while(ebf[0]=='\0' && field<fields)
				} // End, fgets text line loop
			} // End, open/allocator errors
			if(fieldbf!=NULL)	delete [] fieldbf;
			if(clrbf!=NULL)		delete [] clrbf;
			if(dbinf!=NULL)		delete [] dbinf;
			if(dbfp!=NULL)		fclose(dbfp);
		}
	} // End, else if(rewind(txtfp), (chk=min(flen,3))!=0)
	if(txtfp!=NULL)		fclose(txtfp);
	if(txtbf!=NULL)		delete [] txtbf;
	if(ebf[0])			{
		if(line > 0)	env->ThrowError("%sError, %s : CSV Line=%d field=%d",myName,ebf,line,field);
		else			env->ThrowError("%sError, %s",myName,ebf);
	}
	return added;
}


AVSValue __cdecl RT_DBaseWriteCSV(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char * myName="RT_DBaseWriteCSV: ";
	char ebf[256]="";
	FILE *txtfp=NULL;
	char *FieldBuffer=NULL;
	const char * dbfn	  = args[0].AsString();
	const char * txtfn	  = args[1].AsString();
	const unsigned char * commaStr = (unsigned char*)	args[2].AsString(",");
	const char *StrDelimiter=args[3].AsString("\"");
	const char *Fmt=args[9].AsString("");
	const int    comma	  = (*commaStr==9)?32:*commaStr;
	if(comma==10||comma==13||comma==34||comma==0)	env->ThrowError("%sIllegal Separator[Chr(%d)]",myName,comma);
	const int StrDelimiterLen=(int)strlen(StrDelimiter);
	if(StrDelimiterLen==0)			env->ThrowError("%sIllegal zero length StrDelimiter]",myName);
	int i;
	for(i=0;i<StrDelimiterLen;++i) {
		if(StrDelimiter[i]==32)	    env->ThrowError("%sIllegal character StrDelimiter[%d]=SPACE",myName,i+1);
		if(StrDelimiter[i]==9)	    env->ThrowError("%sIllegal character StrDelimiter[%d]=TAB",myName,i+1);
		if(StrDelimiter[i]==10)		env->ThrowError("%sIllegal character StrDelimiter[%d]=Chr(10)",myName,i+1);
		if(StrDelimiter[i]==13)		env->ThrowError("%sIllegal character StrDelimiter[%d]=Chr(13)",myName,i+1);
		if(StrDelimiter[i]==comma)	env->ThrowError("%sIllegal character StrDelimiter[%d]=Separator",myName,i+1);
	}
	const int FmtLen=(int)strlen(Fmt);
	int record=-1,added=0;
	MYDB db;
	MYDBINF * dbinf=NULL;
	FILE * dbfp=DB_Read_Header(myName,dbfn,"rb+",db,env);
	const int records= db.records;
	const int fields = db.fields;
	const int recordsz=db.recordsz;
	const int maxfieldsz=db.maxfieldsz;
	const int FieldBufferSize=maxfieldsz+2;
	const int StartField=args[4].AsInt(0);
	int EndField=args[5].AsInt(fields-1);
	int low     =args[6].AsInt(0);
	int high    =args[7].AsInt(records-1);
	int validFields=EndField-StartField+1;
	bool txexists = false;
	txtfp = fopen(txtfn, "r+");
	if(txtfp!=NULL) {
		fclose(txtfp);	txtfp=NULL;
		txexists=true;
	}

	bool Append =args[8].AsBool(false);
	const char * txtwmode = (Append && txexists) ? "r+" : "w";
	int field;
	if(low<0)									sprintf(ebf,"low(%d) cannot b -ve",low);
	else if(high<low)							sprintf(ebf,"low(%d) <= high(%d)",low,high);
	else if((dbinf=new MYDBINF[fields])==NULL)	
		strcpy(ebf,"Cannot Allocate DBINF");
	else if((fseek(dbfp, db.infoffset,SEEK_SET)) || (fread(dbinf,fields*sizeof(*dbinf),1,dbfp)!=1))
		strcpy(ebf,"Cannot read DBInf");
	else {
		if(FmtLen>0) {
			if(FmtLen>validFields)							strcpy(ebf,"Fmt string too long");
			else {
				int i;
				for(i=0;i<FmtLen && ebf[0]=='\0';++i) {
					char c=Fmt[i];
					if(c!=' ' && c!='.') {
						const int ix = StartField+i;
						const int type = dbinf[ix].fieldtype;
						if(type==DB_STRING) {
							sprintf(ebf,"Field %d, Type String, allows no formatting",ix);
						} else if(type==DB_BOOL) {
							if(c!='u' && c!='U' && c!='l' &&c!='L')
								sprintf(ebf,"Field %d, Type Bool allows only U,L formatting",ix);
						} else if(type==DB_BIN) {
							if(c !='x' && c !='X' && c !='$')
								sprintf(ebf,"Field %d, Type Bin allows only x,X,$ formatting",ix);
						} else if(type==DB_INT) {
							if(c !='x' && c !='X' && c !='$')
								sprintf(ebf,"Field %d, Type Int allows only x,X,$ formatting",ix);
						} else if(type==DB_FLOAT) {
							sprintf(ebf,"Field %d, Type Float, allows no formatting",ix);
						} else if(type==DB_INT) {
							sprintf(ebf,"Field %d, Type double, allows no formatting",ix);
						}
					}
				}
			}
		}
		if(ebf[0]=='\0') {
			if((FieldBuffer=new char [FieldBufferSize])==NULL)	
				strcpy(ebf,"Cannot Allocate FieldBuffer");
			else if(!(txtfp = fopen(txtfn, txtwmode)))		strcpy(ebf,"Cannot Open CSV file");
			else {
				if(Append) {
					if(fseek(txtfp, 0,SEEK_END))			strcpy(ebf,"CSV Cannot seek to END");
				}
				for(record=low;ebf[0]=='\0' && record<=high;++record) {
					int rd,wr=0;
					for(field=StartField;ebf[0]=='\0' && field<=EndField;++field) {
						int FmtIx = field-StartField;
						char c=(FmtIx<FmtLen) ? Fmt[FmtIx] : ' ';
						memset(FieldBuffer,0,FieldBufferSize);
						const int type      = dbinf[field].fieldtype;
						const int fieldsize	= dbinf[field].fieldsize;
						const int fieldoff	= dbinf[field].fieldoff;
						if(type<DB_BOOL || type > DB_DOUBLE)
							{strcpy(ebf,"Internal Error, Illegal DBase Field type"); break;}
						if(fseeko(dbfp,db.offset + int64_t(record) * db.recordsz + fieldoff,SEEK_SET))
							{strcpy(ebf,"Cannot seek in DB"); break;}
						if((rd = (int)fread(FieldBuffer,fieldsize, 1, dbfp)) != 1)							
							{strcpy(ebf,"Cannot read from DB file"); break;}
						char tbf[128];
						if(type==DB_STRING) {
							char *dat = (char *)FieldBuffer;
							wr = (int)fwrite(StrDelimiter,StrDelimiterLen, 1, txtfp);					
							if(wr==1) wr = (int)fwrite(dat,strlen(dat), 1, txtfp);					
							if(wr==1) wr = (int)fwrite(StrDelimiter,StrDelimiterLen, 1, txtfp);					
						} else if(type==DB_BOOL) {
							bool b =*((BYTE *)FieldBuffer) == 0;
							const char *dat;
							if(b)	dat = (c=='u'||c=='U') ? "TRUE" : (c=='l'||c=='L') ? "true" : "True";
							else	dat = (c=='u'||c=='U') ? "FALSE" : (c=='l'||c=='L') ? "false" : "False";
							wr = (int)fwrite(dat,strlen(dat), 1, txtfp);
						} else if(type==DB_INT) {					
							int dat = *((int *)FieldBuffer);
							const char *format =  (c=='$')?"$%0.8X":(c=='x')?"%0.8x":(c=='X')?"%0.8X":"%d";
							sprintf(tbf,format,dat);
							wr = (int)fwrite(tbf,strlen(tbf), 1, txtfp);					
						} else if(type==DB_BIN) {
							int dat = *((BYTE *)FieldBuffer);
							const char *format =  (c=='$')?"$%0.2X":(c=='x')?"%0.2x":(c=='X')?"%0.2X":"%d";
							sprintf(tbf,format,dat);
							wr = (int)fwrite(tbf,strlen(tbf), 1, txtfp);					
						} else if(type==DB_FLOAT) {
							double dat = *((float *)FieldBuffer);
							sprintf(tbf,"%f",dat);
							wr = (int)fwrite(tbf,strlen(tbf), 1, txtfp);					
						} else if(type==DB_DOUBLE) {
							double dat = *((double *)FieldBuffer);
							sprintf(tbf,"%.12f",dat);
							wr = (int)fwrite(tbf,strlen(tbf), 1, txtfp);					
						}
						if (wr != 1)				{strcpy(ebf,"Cannot write to CSV file"); break;}
						if(field != EndField) {
							wr = (int)fwrite(commaStr,strlen((const char*)commaStr), 1, txtfp);					
							if(wr!=1)		{strcpy(ebf,"Cannot write to CSV file"); break;}
						}
					} // end for field
					if(ebf[0]) break;
					char * lf="\n";
					wr = (int)fwrite(lf,strlen(lf), 1, txtfp);					
					if(wr!=1)		{strcpy(ebf,"Cannot write to CSV file"); break;}
					++added;
				} // end for record
			}
		}
	}
	if(txtfp!=NULL)			fclose(txtfp);
	if(FieldBuffer!=NULL)	delete [] FieldBuffer;
	if(dbinf!=NULL)			delete [] dbinf;
	if(dbfp!=NULL)			fclose(dbfp);
	if(ebf[0])				{
		if(record >= 0 )	env->ThrowError("%sError, %s : DB record=%d field=%d",myName,ebf,record,field);
		else				env->ThrowError("%sError, %s",myName,ebf);
	}
	return added;
}

