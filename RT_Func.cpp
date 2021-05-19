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

AVSValue __cdecl RT_Hex(AVSValue args, void* user_data, IScriptEnvironment* env) {
	int n		= args[0].AsInt();
	int wid		= args[1].AsInt(0);
	wid=(wid<0) ? 0 : (wid > 8) ? 8 : wid;
	char buf[8+1];
	sprintf(buf,"%0*X",wid,n);
	return	env->SaveString(buf);						// Get pointer to Avisynth saved string
}


AVSValue RT_HexValue(AVSValue args, void*, IScriptEnvironment* env) {
	const char *str	= args[0].AsString();
	int	pos	= args[1].AsInt(1) - 1;
	int sz=int(strlen(str));
	if(pos<0 || pos>=sz)
		return 0;
	str+=pos;
	char *stopstring;
	return (int)(strtoul(str,&stopstring,16));
}


AVSValue __cdecl RT_NumberString(AVSValue args, void* user_data, IScriptEnvironment* env) {
	unsigned int n		= (unsigned int)args[0].AsInt();
	unsigned int base	= (unsigned int)args[1].AsInt(10);
	int wid				= args[2].AsInt(0);
	if(base<2 || base > 36) {
		env->ThrowError("RT_NumberString: Illegal base (2->36)");
	}
	wid=(wid<0) ? 0 : (wid > 32) ? 32 : wid;
	char buf[32+32+1],*d=buf;
	if(base==10) {
		sprintf(buf,"%0*d",wid,n);							// do sign for -ve denary
	} else {												// unsigned for other radix
		const char *s="00000000000000000000000000000000";
		while(*d++=*s++);
		--d;
		sprintf(d,"%d",n);
		Ucase(d);
		while(*d)++d;
		int sz=int(d-&buf[32]);
		sz = (sz > wid) ? sz : wid;
		d = d-sz;
	}
	return	env->SaveString(d);						// Get pointer to Avisynth saved string
}


AVSValue RT_NumberValue(AVSValue args, void*, IScriptEnvironment* env) {
	const char * str= args[0].AsString();
	int base		= args[1].AsInt(10);
	if(base<2 || base > 36)
		env->ThrowError("RT_NumberValue: Illegal base (2->36)");
	int	pos	= args[2].AsInt(1) - 1;
	int sz=int(strlen(str));
	if(pos<0 || pos>=sz)
		return 0;
	str+=pos;
	char *stopstring;
	return (int)((base==10) ? strtol(str,&stopstring,base) : strtoul(str,&stopstring,base));
}
