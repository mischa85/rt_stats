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

int __cdecl QueryFatVolume(const char *relname) {
	// Return:- 1=FAT. 0 = Not FAT. -1 on error;
	
	/*
	int ret = -1;
	char FullPath[PATH_MAX];
	if(_fullpath(FullPath, relname, _PATH_MAX ) != NULL ) {
		TCHAR RootPathName[_PATH_MAX];
		_splitpath(FullPath,RootPathName, NULL,NULL,NULL );
		char *p=RootPathName;
		while(*p++);
		--p;
		if(p>RootPathName && p[-1] != '\\') {*p++='\\';*p='\0';}
		TCHAR FileSystemNameBuffer[PATH_MAX+1];
		BOOL result = GetVolumeInformation(RootPathName,NULL,0,NULL,NULL,NULL,FileSystemNameBuffer,PATH_MAX+1);
		if(result) {
			ret = (_strnicmp(FileSystemNameBuffer,"FAT",3)==0) ?1:0;		// Just the 1st 3 characters (FAT/FAT32)
		}
	}
	*/

	//return ret;
	return 0;
}

int64_t __cdecl QueryDiskFreeSpace(const char *relname) {
	// int64_t ret = -1;
	int64_t ret = 0xFFF000000LL;
	/*
	char FullPath[MAX_PATH];
	if(_fullpath(FullPath, relname, _PATH_MAX ) != NULL ) {
		TCHAR RootPathName[_PATH_MAX];
		_splitpath(FullPath,RootPathName, NULL,NULL,NULL );
		char *p=RootPathName;
		while(*p++);
		--p;
		if(p>RootPathName && p[-1] != '\\') {*p++='\\';*p='\0';}
		ULARGE_INTEGER FreeBytesAvailableToCaller;
		ULARGE_INTEGER TotalNumberOfBytes;
		ULARGE_INTEGER TotalNumberOfFreeBytes;
		BOOL result=GetDiskFreeSpaceEx(RootPathName,&FreeBytesAvailableToCaller,&TotalNumberOfBytes,&TotalNumberOfFreeBytes);
		if(result) {
			ret = int64_t(FreeBytesAvailableToCaller.QuadPart);
		}
	}
	*/
	return ret;
}

int64_t __cdecl QueryMaxFileSize(const char *relname) {
	// int64_t ret = -1;
	int64_t ret = 0xFFF000000LL;

	/*
	char FullPath[_PATH_MAX];
	if(_fullpath(FullPath, relname, _PATH_MAX ) != NULL ) {
		TCHAR RootPathName[_PATH_MAX];
		_splitpath(FullPath,RootPathName, NULL,NULL,NULL );
		char *p=RootPathName;
		while(*p++);
		--p;
		if(p>RootPathName && p[-1] != '\\') {*p++='\\';*p='\0';}
		ULARGE_INTEGER FreeBytesAvailableToCaller;
		ULARGE_INTEGER TotalNumberOfBytes;
		ULARGE_INTEGER TotalNumberOfFreeBytes;
		if(GetDiskFreeSpaceEx(RootPathName,&FreeBytesAvailableToCaller,&TotalNumberOfBytes,&TotalNumberOfFreeBytes)) {
			int64_t dfs = int64_t(FreeBytesAvailableToCaller.QuadPart) - 0x1000000LL;	// minus 1MB
			if(dfs > 0) {
				TCHAR FileSystemNameBuffer[PATH_MAX+1];
				if(GetVolumeInformation(RootPathName,NULL,0,NULL,NULL,NULL,FileSystemNameBuffer,PATH_MAX+1)) {
					if(_strnicmp(FileSystemNameBuffer,"FAT",3)==0) {	// Just the 1st 3 characters (FAT/FAT32)
						if(dfs>0xFFF000000LL) dfs = 0xFFF000000LL; 		// limit 4GB-1MB on FAT32
					}
					ret = dfs;
				}
			}
		}
	}
	*/
	return ret;
}