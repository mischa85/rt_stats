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
#include <string>

AVSValue __cdecl RT_GetWorkingDir(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_GetWorkingDir: ";
	char Path[PATH_MAX];
	if(getcwd(Path,PATH_MAX)==NULL) {
		env->ThrowError("%sCannot get cwd",myName);
	}
	char *p;
	for(p=Path;*p++;);
	--p;
	if(Path<p && (p[-1]!='\\' && p[-1]!='/'))
		*p++ ='\\';
	return env->SaveString(Path,int(p-Path));
}



AVSValue __cdecl RT_FilenameSplit(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="RT_FilenameSplit: ";
	const char *relname	= args[0].AsString();
	// env->ThrowError("Whatever: %s",relname);
	const int get	= args[1].AsInt(15);
	if(get<=0 || get>15)									{env->ThrowError("%sget 1 -> 15 Only",myName);}
	char FullPath[PATH_MAX];
	// if(realpath(relname, FullPath) == NULL )	{env->ThrowError("%sCannot get full path: %s",myName,strerror(errno));}
	realpath(relname,FullPath);
	// env->ThrowError("Whatever: %s",FullPath);
	// int sz = 0;
	// char *pbf;
	// if((pbf= new char [sz+1])==NULL)						{env->ThrowError("%sCannot allocate memory",myName);}
	AVSValue ret = env->SaveString(FullPath);
	// delete [] pbf;
	return ret;
}

AVSValue __cdecl RT_GetFullPathName(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *relname	= args[0].AsString();
	AVSValue newargs[2]	=	{relname,15};
	// env->ThrowError("Whatever: %s",relname);
	return RT_FilenameSplit(AVSValue(newargs,2),NULL,env);
}

AVSValue __cdecl RT_GetFileExtension(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char* name	= args[0].AsString();
	char Ext[PATH_MAX];

	const char *dot = strrchr(name, '.');
	if(!dot || dot == name) {
		strcpy(Ext,"");
	} else {
		strcpy(Ext,dot + 1);
	}

	return env->SaveString(Ext);
}
