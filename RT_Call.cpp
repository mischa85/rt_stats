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
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

AVSValue __cdecl RT_Call(AVSValue args, void*, IScriptEnvironment* env) {
	const char *s,*scmd = args[0].AsString();
	bool debug=args[2].AsBool(false);
	for(s=scmd;*s++;);
	// CreateProcessW can modify the command string and so cannot be const,
	// WE CONSIDER NON CONST ANYWAY even though NOT using CreateProcessW.
	char *d,* pbf = new char[s-scmd];			// len of string + 1 for nul
	if(!pbf) {
		env->ThrowError("RT_Call: Cannot allocate memory");
	}
	for(s=scmd,d=pbf;*d++=*s++;);				// Duplicate the string, SYS may alter our const string.
	
	// STARTUPINFO			stinfo;
	// PROCESS_INFORMATION pinfo;pbf
	// int i;
	// Clear the structures
	// for(d=(char*)&stinfo,i=sizeof(STARTUPINFO); --i>=0; d[i]='\0');
	// for(d=(char*)&pinfo,i=sizeof(PROCESS_INFORMATION); --i>=0; d[i]='\0');
	// stinfo.cb = sizeof(STARTUPINFO);
	
	// unsigned int fdwCreate = (args[1].AsBool(false)) ? CREATE_NO_WINDOW : 0;	// Hide Console Window ?
    
	pid_t child_pid;

	if(debug) {
		dprintf("RT_Call: %s",pbf);
	}

	// SetLastError(ERROR_SUCCESS);

	/*
	int success = CreateProcess(
		NULL,				// ApplicationName, name of executable. NULL, name  is in Commandline instead
		pbf,  				// CommandLine, command line string, Cannot be const.
		NULL,				// Process Attributes, NULL, the handle cannot be inherited
		NULL,				// ThreadAttributes, NULL=the handle cannot be inherited
		FALSE,				// handle inheritance option, Dont inherit
		fdwCreate,			// creation flags
		NULL,				// new environment block
		NULL,				// current directory name, same as ours
		&stinfo,			// startup information
		&pinfo				// process information
		);
	*/
    
	if (child_pid != 0){
		int succes = waitpid(child_pid, NULL, 0);

		if (succes == -1){
        	dprintf("RT_Call: An error occurred in waitpid");
          	return 0;
        }
	}
	else {
		execl (pbf, pbf);
		dprintf("RT_Call: An error occured in execl");
		return 0;
	}

	/*
	if ( success ) {
		// Wait until child process exits.
		DWORD Wret = WaitForSingleObject(pinfo.hThread, INFINITE);
		if(Wret==WAIT_OBJECT_0 && debug) {
			DWORD dwExitCode = 0;
			if(GetExitCodeProcess(pinfo.hProcess , &dwExitCode)) {
				dprintf("RT_Call: Process returned = %d (0x%08X)",dwExitCode,dwExitCode);
			}
		}
	    // Close process and thread handles. 
        CloseHandle( pinfo.hProcess );
        CloseHandle( pinfo.hThread );
		return 0;									// Successful application start
	} else {
		if(debug) {
			char *e=GetErrorString();
			if(e) {
				dprintf("RT_Call: *** Error *** creating process = %s",e);
				delete [] e;
			}
		}
	}
	*/

	delete [] pbf;									// delete temp buffer
	return 1;										// Failed to start
}

AVSValue __cdecl RT_GetLastErrorString(AVSValue args, void*, IScriptEnvironment* env) {
	// char *e=GetErrorString();
	char *e=strerror(errno);
	if(e==NULL)								env->ThrowError("RT_GetLastErrorString: Cannot Allocate Memory");
	AVSValue ret =	env->SaveString(e);
	delete [] e;
	return ret;
}

AVSValue __cdecl RT_GetLastError(AVSValue args, void*, IScriptEnvironment* env) {
	// return (int)(GetLastError());
	return (int)(errno);
}
