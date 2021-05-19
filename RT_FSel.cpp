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
#include <wx/wx.h>

/*
	typedef struct tagOFN { 
	  DWORD         lStructSize; 
	  HWND          hwndOwner;					// NULL no owner 
	  HINSTANCE     hInstance; 
	  LPCTSTR       lpstrFilter;				// the filter
	  LPTSTR        lpstrCustomFilter; 
	  DWORD         nMaxCustFilter; 
	  DWORD         nFilterIndex;				// we default ALWAYS 1 (ie first one)
	  LPTSTR        lpstrFile;					// initial filename, also for return filename
	  DWORD         nMaxFile;					// size of filename buffer 
	  LPTSTR        lpstrFileTitle; 
	  DWORD         nMaxFileTitle; 
	  LPCTSTR       lpstrInitialDir;			// the initial directory or current if null 
	  LPCTSTR       lpstrTitle;					// Title Bar text, eg 'Open file' 
	  DWORD         Flags; 
	  WORD          nFileOffset; 
	  WORD          nFileExtension; 
	  LPCTSTR       lpstrDefExt; 
	  LPARAM        lCustData; 
	  LPOFNHOOKPROC lpfnHook; 
	  LPCTSTR       lpTemplateName; 
	#if (_WIN32_WINNT >= 0x0500)
	  void *        pvReserved;
	  DWORD         dwReserved;
	  DWORD         FlagsEx;
	#endif // (_WIN32_WINNT >= 0x0500)
	} OPENFILENAME, *LPOPENFILENAME;
*/

AVSValue __cdecl  RT_FSelOpen(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_FSelOpen: ";
	const char * myFilter=
		"All files (*.*)|*.*|"
		"Avi Files (*.avi;*.XVid;*.DivX)|*.avi;*.XVid;*.DivX|"
		"Mpg files (*.vob;*.mpg;*.mpeg;*.m1v;*.m2v;*.mpv;*.tp;*.ts;*.trp;*.m2t;*.m2ts;*.pva;*.vro)|*.vob;*.mpg;*.mpeg;*.m1v;"
		"*.m2v;*.mpv;*.tp;*.ts;*.trp;*.m2t;*.m2ts;*.pva;*.vro|"
		"Other Vid (*.mkv;*.Wmv;*.mp4;*.flv;*.ram;*.rm)|*.mkv;*.Wmv;*.mp4;*.flv;*.ram;*.rm|"
		"Audio Files (*.mp3;*.mpa;*mp1;*.mp2;*.wav)|*.mp3;*.mpa;*mp1;*.mp2;*.wav|"
		"Avs files (*.avs;*.avsi)|*.avs;*.avsi|"
		"Text files (*.txt;*.log;*.asc)|*.txt;*.log;*.asc|"
		"Image files (*.bmp;*.jpg;*.jpe;*.jpeg;*.png;*.tga;*.tif;*.gif;*.tiff)|*.bmp;*.jpg;*.jpe;*.jpeg;*.png;*.tga;*.tif;*.gif;*.tiff|"
		"Bat files (*.bat;*.cmd)|*.bat;*.cmd|"
		"Exe files (*.exe)|*.exe";

	const char * title	=args[0].AsString(NULL);
	const char * dir	=args[1].AsString(NULL);			// Default current dir
	const char * filt	=args[2].AsString(myFilter);
	const char * fn		=args[3].AsString("");
	const bool multi	=args[4].AsBool(false);
	const bool debug	=args[5].AsBool(false);
	if(strlen(filt)>4096-3)					env->ThrowError("%sFilter too long",myName);
	char filt2[4096];
	const char *p=filt;
	int ix=0;
	while(*p) {										// convert from PIPE '|' separated to nul separated filter strings
		if(*p=='|') {
			filt2[ix++]='\0';
			++p;
		} else {
			filt2[ix++]=*p++;
		}
	}
	filt2[ix++]='\0';	filt2[ix]='\0';			// Double nul term
	int size = (multi) ? 65536 : (PATH_MAX * 2);
	int len=int(strlen(fn)) + 1;
	if(len>size)	size=len;
	char *szFile = new char [size];					// buffer for file name
	if(szFile==NULL)						env->ThrowError("%sCannot allocate memory",myName);
	strcpy(szFile,fn);								// set initial filename

	wxFileDialog * openFileDialog = new wxFileDialog();

	if (openFileDialog->ShowModal() == wxID_OK) {
    	wxString szFile = openFileDialog->GetPath();
	}

	/*
	int flgs= \
		OFN_PATHMUSTEXIST | 
		OFN_FILEMUSTEXIST |
		OFN_HIDEREADONLY  |		// hide readonly check box
		OFN_LONGNAMES     |
		OFN_NOCHANGEDIR;		// restore original current directory if user changed. Does NOT work for GetOpenFileName.
	    	tc->LoadFile(fileName);   
	if(multi) flgs |=(OFN_ALLOWMULTISELECT|OFN_EXPLORER) ;

	OPENFILENAME ofn;								// common dialog box structure
	ZeroMemory(&ofn, sizeof(ofn));					// Initialize OPENFILENAME
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;							// owner window, we dont have one
	ofn.lpstrFile = szFile;							// filename buff
	ofn.nMaxFile  = size;							// size of filename buff
	ofn.lpstrFilter = filt2;						// Converted filt string
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = dir;
	ofn.lpstrTitle=title;							// Title shown in Title Bar
	ofn.Flags = flgs;
	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)!=TRUE)  {
		delete [] szFile;
		DWORD ret = CommDlgExtendedError();
		if(ret==0) {
			if(debug) dprintf("%sUser CANCELLED",myName);
			return 0;
		}
		if(debug) {
			switch(ret) {
			case FNERR_BUFFERTOOSMALL:	dprintf("%sFilename buffer Too Small",myName); break;
			case FNERR_INVALIDFILENAME:	dprintf("%sInvalid filename",myName); break;
			case FNERR_SUBCLASSFAILURE:	dprintf("%sSubClass failure (low memory)",myName); break;
			case CDERR_DIALOGFAILURE:	dprintf("%sDialog box creation failure",myName); break;
			case CDERR_FINDRESFAILURE:	dprintf("%sFind resource failure",myName); break;
			case CDERR_INITIALIZATION:	dprintf("%sDialog box initialization failure (usually memory)",myName); break;
			case CDERR_LOADRESFAILURE:	dprintf("%sLoad resource failure",myName); break;
			case CDERR_LOADSTRFAILURE:	dprintf("%sLoad string failure",myName); break;
			case CDERR_LOCKRESFAILURE:	dprintf("%sLock resource failure",myName); break;
			case CDERR_MEMALLOCFAILURE:	dprintf("%sUnable to allocate memory structures",myName); break;
			case CDERR_MEMLOCKFAILURE:	dprintf("%sUnable to lock memory associated with handle",myName); break;
			case CDERR_NOHINSTANCE:		dprintf("%sNo instance handle",myName); break;
			case CDERR_NOHOOK:			dprintf("%sNo hook",myName); break;
			case CDERR_NOTEMPLATE:		dprintf("%sNo template",myName); break;
			case CDERR_REGISTERMSGFAIL:	dprintf("%sRegisterWindowMessage returned an error",myName); break;
			case CDERR_STRUCTSIZE:		dprintf("%sInvalid lStructSize member",myName); break;
			default:					dprintf("%sUnknown error",myName); break;
			}
		}
		return (int)ret;
	}
	*/

	int off=0; // ofn.nFileOffset;
	if(!multi || (off>0 && szFile[off-1]=='\\')) {		// single filename selected
		AVSValue retstr = env->SaveString(szFile);
		delete [] szFile;
		return retstr;
	}

	p=szFile;
	int nstr=0;
	while(*p) {			// Count gotten strings, 1st is dir, then filenames. Ends in double nul.
		while(*p) {
			++p;
		}
		++p;
		++nstr;
	}
	int dirlen=off;									// including nul (will convert to '\\').
	int fullen = int((p - szFile - off) + ((nstr - 1) * dirlen)) + 1;		// +1 for final nul term.
	char *mfiles = new char [fullen];				// buffer for multiple expanded file names, '\n' separated
	if(mfiles==NULL) {
		delete [] szFile;
		env->ThrowError("%sCannot allocate multiselect files buffer",myName);
	}
	char *d=mfiles;									// Dest multiselect files buffer
	char *s=&szFile[off];							// point at 1st filename node
	for(int i=nstr-1;--i>=0;) {						// Convert to filenames with full path
		char *r=szFile;								// point at path
		while(*r)
			*d++=*r++;								// Copy path
		*d++='\\';									// backslash separator
		while(*s)
			*d++=*s++;								// append filename
		++s;										// skip nul
		*d++='\n';									// '\n' multiline separator
	}
	*d='\0';										// nul term
	delete [] szFile;
	AVSValue retstr = env->SaveString(mfiles);
	delete [] mfiles;
	return retstr;
}


AVSValue __cdecl  RT_FSelSaveAs(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_FSelSaveAs: ";
	const char * myFilter=
		"All files (*.*)|*.*|"
		"Avi Files (*.avi;*.XVid;*.DivX)|*.avi;*.XVid;*.DivX|"
		"Mpg files (*.vob;*.mpg;*.mpeg;*.m1v;*.m2v;*.mpv;*.tp;*.ts;*.trp;*.m2t;*.m2ts;*.pva;*.vro)|*.vob;*.mpg;*.mpeg;*.m1v;"
		"*.m2v;*.mpv;*.tp;*.ts;*.trp;*.m2t;*.m2ts;*.pva;*.vro|"
		"Other Vid (*.mkv;*.Wmv;*.mp4;*.flv;*.ram;*.rm)|*.mkv;*.Wmv;*.mp4;*.flv;*.ram;*.rm|"
		"Audio Files (*.mp3;*.mpa;*mp1;*.mp2;*.wav)|*.mp3;*.mpa;*mp1;*.mp2;*.wav|"
		"Avs files (*.avs;*.avsi)|*.avs;*.avsi|"
		"Text files (*.txt;*.log;*.asc)|*.txt;*.log;*.asc|"
		"Image files (*.bmp;*.jpg;*.jpe;*.jpeg;*.png;*.tga;*.tif;*.gif;*.tiff)|*.bmp;*.jpg;*.jpe;*.jpeg;*.png;*.tga;*.tif;*.gif;*.tiff|"
		"Bat files (*.bat;*.cmd)|*.bat;*.cmd";

	const char * title	=args[0].AsString(NULL);
	const char * dir	=args[1].AsString(NULL);			// Default current dir
	const char * filt	=args[2].AsString(myFilter);
	const char * fn		=args[3].AsString("");
	const bool debug	=args[4].AsBool(false);
	if(strlen(filt)>4096-3)					env->ThrowError("%sFilter too long",myName);
	char filt2[4096];
	const char *p=filt;
	int ix=0;
	while(*p) {										// convert from PIPE '|' separated to nul separated filter strings
		if(*p=='|') {
			filt2[ix++]='\0';
			++p;
		} else {
			filt2[ix++]=*p++;
		}
	}
	filt2[ix++]='\0';	filt2[ix]='\0';				// Double nul term
	int size = PATH_MAX * 2;
	int len=int(strlen(fn)) + 1;
	if(len>size)	size=len;
	char *szFile = new char [size];					// buffer for file name
	if(szFile==NULL)						env->ThrowError("%sCannot allocate memory",myName);
	strcpy(szFile,fn);								// set initial filename
	
	wxFileDialog * saveFileDialog = new wxFileDialog();

	if (saveFileDialog->ShowModal() == wxID_OK) {
    	wxString szFile = saveFileDialog->GetPath();
	}

	/*
	int flgs= \
		OFN_OVERWRITEPROMPT | 
		OFN_LONGNAMES       |
		OFN_NOCHANGEDIR;		// restore original current directory if user changed. Does NOT work for GetOpenFileName.
 
	OPENFILENAME ofn;								// common dialog box structure
	ZeroMemory(&ofn, sizeof(ofn));					// Initialize OPENFILENAME
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;							// owner window, we dont have one
	ofn.lpstrFile = szFile;							// filename buff
	ofn.nMaxFile  = size;							// size of filename buff
	ofn.lpstrFilter = filt2;						// Converted filt string
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = dir;
	ofn.lpstrTitle=title;							// Title shown in Title Bar
	ofn.Flags = flgs;

	// Display the Open dialog box. 
	if (GetSaveFileName(&ofn)!=TRUE)  {
		delete [] szFile;
		DWORD ret = CommDlgExtendedError();
		if(ret==0) {
			if(debug) dprintf("%sUser CANCELLED",myName);
			return 0;
		}
		if(debug) {
			switch(ret) {
			case FNERR_BUFFERTOOSMALL:	dprintf("%sFilename buffer Too Small",myName); break;
			case FNERR_INVALIDFILENAME:	dprintf("%sInvalid filename",myName); break;
			case FNERR_SUBCLASSFAILURE:	dprintf("%sSubClass failure (low memory)",myName); break;
			case CDERR_DIALOGFAILURE:	dprintf("%sDialog box creation failure",myName); break;
			case CDERR_FINDRESFAILURE:	dprintf("%sFind resource failure",myName); break;
			case CDERR_INITIALIZATION:	dprintf("%sDialog box initialization failure (usually memory)",myName); break;
			case CDERR_LOADRESFAILURE:	dprintf("%sLoad resource failure",myName); break;
			case CDERR_LOADSTRFAILURE:	dprintf("%sLoad string failure",myName); break;
			case CDERR_LOCKRESFAILURE:	dprintf("%sLock resource failure",myName); break;
			case CDERR_MEMALLOCFAILURE:	dprintf("%sUnable to allocate memory structures",myName); break;
			case CDERR_MEMLOCKFAILURE:	dprintf("%sUnable to lock memory associated with handle",myName); break;
			case CDERR_NOHINSTANCE:		dprintf("%sNo instance handle",myName); break;
			case CDERR_NOHOOK:			dprintf("%sNo hook",myName); break;
			case CDERR_NOTEMPLATE:		dprintf("%sNo template",myName); break;
			case CDERR_REGISTERMSGFAIL:	dprintf("%sRegisterWindowMessage returned an error",myName); break;
			case CDERR_STRUCTSIZE:		dprintf("%sInvalid lStructSize member",myName); break;
			default:					dprintf("%sUnknown error",myName); break;
			}
		}
		return (int)ret;
	}
	AVSValue retstr = env->SaveString(szFile);
	delete [] szFile;
	return retstr;
}




/*
typedef struct _browseinfo {
  HWND              hwndOwner;			// A handle to the owner window for the dialog box.
  PCIDLIST_ABSOLUTE pidlRoot;			// Root folder to browse from PIDL (NULL = DeskTop)
  LPTSTR            pszDisplayName;		// returned name, Presumed PATH_MAX characters
  LPCTSTR           lpszTitle;			// Title Bar String
  UINT              ulFlags;			
  BFFCALLBACK       lpfn;				// Calback functionm, Can be NULL.
  LPARAM            lParam;
  int               iImage;
} BROWSEINFO, *PBROWSEINFO, *LPBROWSEINFO;
*/

/*
int CALLBACK FSelFolderCallback(HWND hwnd,UINT uMsg,LPARAM lp, LPARAM pData) {
	// The callback function required to init desired path to folder, rather than root (Desktop)
	// meaning of lp depends on uMsg type
	// pData is application defined data for the callback function.
	char szPath[PATH_MAX*2];
	switch(uMsg) {
	case BFFM_INITIALIZED:											// Selects the specified folder Path
		szPath[0]='\0';
		{	// Need Brace to init below s without error.
			char*s=(char*)pData;
			if(s!=NULL) {
				if(s[0]=='.'&&s[1]=='\0') {
					if(!GetCurrentDirectory(sizeof(szPath), szPath))
						szPath[0]='\0';
				} else {
					strcpy(szPath,s);
				}
			}
		}
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE,(LPARAM) szPath);
		break;
	case BFFM_SELCHANGED: 											// Indicates the selection has changed. 
		if (SHGetPathFromIDList((LPITEMIDLIST) lp ,szPath)) {		// convert pidl to path
			SendMessage(hwnd, BFFM_SETSTATUSTEXT,0,(LPARAM)szPath);	// Send path
		}
		break;
	}
	return 0;	// Always returns 0
*/
}

AVSValue __cdecl  RT_FSelFolder(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char * myName="RT_FSelFolder: ";
	const char * title	=args[0].AsString("");
	const char * dir	=args[1].AsString(".");			// Default to '.' = current directory
	const bool debug	=args[2].AsBool(false);

	wxDirDialog* dirDialog = new wxDirDialog();

	char szFold[PATH_MAX * 2];

	if (dirDialog->ShowModal() == wxID_OK) {
    	wxString wxFold = dirDialog->GetPath();
		szFold == wxFold.ToStdString();
	}

	/*
	LPMALLOC pMalloc;										// Shell allocator
    if (!SUCCEEDED(SHGetMalloc(&pMalloc))) {				// Did we successfully get the shell mem alloc interaface
		env->ThrowError("%sCannot get Shell Alloc Interface",myName);
	}
	
	char szFold[PATH_MAX * 2];
	strcpy(szFold,dir);
	int flgs= \
		BIF_STATUSTEXT		  |
		BIF_RETURNFSANCESTORS |
		BIF_RETURNONLYFSDIRS  |							// File system objects only (selectable), DONT SEEM TO WORK PROPER.
		BIF_EDITBOX			  |							// EDITBOX (ie 4.0 + Only)
		BIF_NEWDIALOGSTYLE;								// 6.0 + only (xp+)
	BROWSEINFO bi;										// Browse structure
	ZeroMemory(&bi,sizeof(bi));							// clr
	bi.hwndOwner		= NULL;							// No owner window
	bi.pidlRoot			= NULL;							// Desktop
	bi.pszDisplayName	= NULL;							// We are gonna get path ourselves
	bi.lpszTitle		= title;
	bi.ulFlags			= flgs;
	bi.lpfn				= FSelFolderCallback;			// Browse callback function
	bi.lParam			= (LPARAM)szFold;
	//
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);	// return NULL on user CANCEL, else pidl, user must free using shell allocator
	if (pidl==NULL) {
		if(debug) dprintf("%sUser CANCELLED",myName);
		pMalloc->Release();   							// done with shell allocator
		return 0;
	}
	bool b = (SHGetPathFromIDList(pidl,szFold)!=0);	// Converts an item identifier list to a file system path (MUST be filesystem)
	pMalloc->Free(pidl);						// free the pidl
	pMalloc->Release();   						// done with shell allocator
	if(!b) {
		if(debug) dprintf("%sNot a Filesystem Object",myName);
		return -1;
	}
	*/

	return env->SaveString(szFold);
}







