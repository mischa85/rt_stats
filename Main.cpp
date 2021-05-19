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

/*
	Some parts may be liberated from Avisynth source code.
	http://avisynth2.cvs.sourceforge.net/avisynth2/

	http://forum.doom9.org/showthread.php?p=1584313#post1584313

	v1.00, Implemented RGB with internal RGB -> YUV-Y conversion.
	v1.01, Added RT_Debug function.
	v1.02, Added several more functions.
	v1.03, Added several more functions.
	v1.04, Added RT_Call.
	v1.05, Added RT_YInRange. Ucase bug fixed.
	v1.07beta, 11 Dec 2012. Added Lots of stuff, Testing 1-2-3.
	v1.08  Add some funcs
	v1.09  Fixed RT_RgbChanAve w==1 bug (always blue channel).
	v1.10  Added funcs. Fixed RT_TxtSort null string bug, Matrix default now all (width<=720?2:3)
	v1.11  Fixed broken RT_LumaDifference & RT_LumaCorrelation, "must have two clips" bug.
           Added temporary RT_LumaSNSSD()
	v1.12  Added RT_LumaSSSD(), left RT_LumaSNSSD() in situ, probably removed in next version.
	v1.13  RT_QueryBorderCrop CropLess/More modified. Experimental RT_LumaSSSD removed.
	v1.14, Added RT_FindStr. Mods:- RT_Debug, StrReplace fns, RT_NumberValue and RT_HexValue.
	v1.15, RT_YInRangeLocate, internally, Baffle separately limited to dimensions on both axis.
	v1.16, RT_LumaSceneChange added.
	v1.17, Added RT_LumaMovement. RT_Ord mod.
	v1.18, Fixed RT_QueryBorderCrop RGB "Matrix NOT @ PC levels - Check!" bug.
	       Added RT_ColorSpaceXMod/YMod. Added RT_RgbChan family.
	v1.19, RT_LumaSceneChange() moved cap @ 255.0 to just before return instead of on Lut, better metrics.
              Originally like:- mt_lutxy(mt_polish("((x-y)/4)^2")).AverageLuma()
	       Changed RT_QueryBorderCrop() Thresh Massaging.
    v1.20, Changed the way RT_LumaSceneChange works. Added Bias->Pord args. Added RT_Subtitle().
	v1.21, RT_Subtitle FIX: single backslash @ end of '\n' terminated string let '\n' pass without conversion to '\r'.
		       Mod, Strictly eg "\n" and not "\N" interpreted as linefeed etc.
		   Added FSEL functions.
	v1.22, Fixed RT_RGBChanXXXX channels. Changed RT_YStdev() to Standard Deviation from Sample Standard Deviation.
	v1.23, Mod RT_Debug, added Repeat arg.
           Mod RT_YInRangeLocate, added Baffle_W and Baffle_H args.
           Added RT_RgbInRange func.
		   Added RT_RgbInRangeLocate func.

	v1.24, 28/9/13, Fixed Interlaced Y stepping. 
	       Added Mask args to Y/RGB stats, Added RT_YPNorm, RT_RgbChanPNorm.
		   Added RT_TxtFindStr and RT_FileFindStr and RT_String.
	v1.25, 28/9/13, Afterthough, changed RT_string(bool Esc) to int.
	v1.26, 15/10/13, Fixed typo, XTRA_MASKMIN should have been XTRA_MASKMAX.
	       Convert all default func matrix args to switch rec709 if height >= 600
	v1.27, Added RT_FunctionExist, RT_DebugF. Changed all returned Global vars to Local vars except for RT_SignalDAR.
	v1.28, Added Array Funcs + lots of others. Added Esc arg to RT_Subtitle.
	v1.30, 19 Dec 2013, Added DBase Functions, Increased Array attributes to 256, improved error messages.
	v1.40, 9 Sept 2014, Fixed, RT_DBaseGetAttrib returned float instead of Int (for int attrib).
	       Added args Start and End to RT_QueryBorderCrop and RT_QueryLumaMinMax. Added RT_WriteFile + other funcs.
		   Now 1024 Array & DBase Attributes. Array and DBase now supports type double as private type for RT_Stats
		   internal routines, set or get as type Float from Avisynth script.
		   DBase max number of fields increased to 1024. Added DBase/Array String Attributes.
		   Added RT_QwikScan + other stuff.
    v1.41, 15 Sept 2014, Fixed borked v1.40, (released wrong source + binary), RT_QwikScan PC, PD related stuff.
		   Changed RT_QwikScan LumaTol arg to float.
    v1.42, 19 Sept 2014. Modified Float LumaTol implementation. Added additional XP mode to RT_QwikScan, and affects
		   BEST MATCH results. Added RT_AvgLumaDif.
	v1.43, 07 Oct 2014. Added Dbase/Array ID and RT_QwikScanEstimateLumaTol().

	v2.00, 11 Oct 2017.
		   Switch to VS2008 compiler, v2.6 avs+ header.
		   String replace fn's no longer return error on empty source string, return "".
		   Fix, bug in RT_DebugF(). Arg string containing '\r\n' did not always remove '\r'.
		   Fix, Nul termed printString for dprintf().
		   Fix, RT_Subtitle, fixed error in ThrowError, missing %s insertion point, on error.
           Fix, RT_ColorSpaceXMod and RT_ColorSpaceYMod, Added missing ": " at end of myName.
		   Fix, RT_DBaseGetID, not closing file.
		   Fix, RT_FselOpen file extension filter *.exe was *.cmd.
		   Fix, RT_Subtitle, RT_DebugF, RT_Writefile, RT_string, IsInt() to AsInt().


  
	v2.00Beta1, 06 July 2016. Fixed bug in RT_DebugF(). Arg string containing '\r\n' did not always remove '\r'.
	       Added, RT_ForceProcess.
		     RT_QueryBorderCrop, changed defaults samples=40, Thresh=-40.0, Ignore=0.4.
		 	   wmod,hmod limited max 16. Fixed crop centering.
		     RT_LumaDifference, RT_LumaCorrelation, RT_LumaPixelsDifferent, RT_LumaPixelsDifferentCount, 
			    clips do not have to be same dimensions.
			Lots of other changes.
	v2.00Beta2, 2nd Oct 2016, Added args Thresh_w, Thresh_h and Rescan to RT_YInRangeLocate, RT_RGBInRangeLocate.
		Added Debug arg to RT_ForceProcess.
		Replaced RT_DBaseCockTailSort() with RT_DBaseInsertSort(), spotted an obvious improvement to the InsertSort
		  algo that I had used (was initially no better than CockTailSort).
	    Modified RT_DBaseQuickSort() algo, added Ins and RandPivot args.
		Avoided what I believe is a bug in the supposedly Optimal Sedgewick three-way-partitioning quicksort algo.
	v2.0 Beta 4. Bug fix in ARR_Read_Header(), (kicks oneself).
		RT_QwiScan() added FM FrameMovement mode.
	v2.0 Beta 5. Added RT_DBaseReadCSV().
	v2.0 Beta 6. 28 Feb 2017.
		RT_DBaseReadCSV(), relaxed Separator requirement if SPACE/TAB separated. Added StrDelimiter string Arg.
	v2.0 Beta 7. 02 Mar 2017.
		RT_DBaseReadCSV(), Added StartField and EndField args. Max CSV line length increased to 64*1024 bytes.
	v2.0 Beta 8. 04 Mar 2017.
		Added Functions:
			RT_DBaseWriteCSV(), RT_DBaseFindSeq() and RT_DBaseRgbColorCount().	                         
	v2.0 Beta 9. 04 Mar 2017.
			Added args to RT_DBaseRgbColorCount(), x,y,w,h, Mask, MaskMin, MaskMax, msb.
    v2.0. beta 11, 10 Sept 2017.
		Bugfix, RT_DBaseReadCSV write fail on zero length CSV string.
    v2.0. beta 12, 02 Aug 2018.
		Recompile VS2008, require vs2008 runtimes, compile for x64, add Version Resource.
	v2.13,
		StrReplace(s,"",replace) # if find="", then return original strng.

*/

#include "RT_Stats.h"

/*
	Compiling for both Avisynth v2.58 & v2.6 ProjectName under VS6, where ProjectName is the base name of the plugin.
	Create an additional project ProjectName26 and copy the
	project files into ProjectName folder. Add headers and source files to v2.6 project.
	You should have "avisynth25.h" in the ProjectName Header Files and
	"avisynth26.h" in the ProjectName26 Header Files.
	For the v2.58 project, add preprocessor definition to :-
		Menu/Project/Settings/All Configuration/C/C++, AVISYNTH_PLUGIN_25
*/
#ifdef AVISYNTH_PLUGIN_25
	extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env) {
#else
	// New 2.6 requirement!!! //
	// Declare and initialise server pointers static storage.
	const AVS_Linkage *AVS_linkage = 0;

	// New 2.6 requirement!!! //
	// DLL entry point called from LoadPlugin() to setup a user plugin.
	extern "C" __declspec(dllexport) const char* __stdcall
			AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors) {

	// New 2.6 requirment!!! //
	// Save the server pointers.
	AVS_linkage = vectors;
#endif

 env->AddFunction("RT_ColorSpaceXMod",			"c",RT_ColorSpaceXMod, 0);
 env->AddFunction("RT_ColorSpaceYMod",			"c[Laced]b",RT_ColorSpaceYMod, 0);
 env->AddFunction("RT_Stats",					"c",Create_RT_Stats, 0);
 env->AddFunction("RT_GraphLink",				"cc+b*",Create_RT_GraphLink, 0);
 env->AddFunction("RT_Subtitle",				"cs.*[align]i[x]i[y]i[vcent]b[expx]b[expy]b[esc]i",Create_RT_Subtitle, 0);
 env->AddFunction("RT_BitNOT",					"i" ,RT_BitNOT, 0);
 env->AddFunction("RT_BitAND",					"ii",RT_BitAND, 0);
 env->AddFunction("RT_BitOR" ,					"ii",RT_BitOR,  0);
 env->AddFunction("RT_BitXOR",					"ii",RT_BitXOR, 0);
 env->AddFunction("RT_BitASL",					"ii",RT_BitASL, 0);
 env->AddFunction("RT_BitASR",					"ii",RT_BitASR, 0);
 env->AddFunction("RT_BitLSL",					"ii",RT_BitLSL, 0);
 env->AddFunction("RT_BitLSR",					"ii",RT_BitLSR, 0);
 env->AddFunction("RT_BitTST",					"ii",RT_BitTST, 0);
 env->AddFunction("RT_BitCLR",					"ii",RT_BitCLR, 0);
 env->AddFunction("RT_BitSET",					"ii",RT_BitSET, 0);
 env->AddFunction("RT_BitCHG",					"ii",RT_BitCHG, 0);
 env->AddFunction("RT_BitROR",					"ii",RT_BitROR, 0);
 env->AddFunction("RT_BitROL",					"ii",RT_BitROL, 0);
 env->AddFunction("RT_BitSetCount",				"i",RT_BitSetCount, 0);
 env->AddFunction("RT_Hex",						"i[width]i",RT_Hex, 0);
 env->AddFunction("RT_HexValue",				"s[pos]i",RT_HexValue, 0);
 env->AddFunction("RT_NumberString",			"i[base]i[width]i",RT_NumberString, 0);
 env->AddFunction("RT_NumberValue",				"s[base]i[pos]i",RT_NumberValue, 0);
 env->AddFunction("RT_Call",					"s[Hide]b[Debug]b", RT_Call, 0);
 env->AddFunction("RT_GetLastError",			"", RT_GetLastError, 0);
 env->AddFunction("RT_GetLastErrorString",		"", RT_GetLastErrorString, 0);
 env->AddFunction("RT_Debug",					"s+[name]b[repeat]b", RT_Debug, 0);
 env->AddFunction("RT_DebugF",					"s.*[name]s[tabsz]i", RT_DebugF, 0);
 env->AddFunction("RT_GetProcessName",			"[Parent]b[debug]b", RT_GetProcessName, 0);
 env->AddFunction("RT_Undefined",				"",RT_Undefined, 0);
 env->AddFunction("RT_VarExist",				"s",RT_VarExist, 0);
 env->AddFunction("RT_FunctionExist",			"s",RT_FunctionExist, 0);
 env->AddFunction("RT_Ord",						"s[pos]i",RT_Ord, 0);
 env->AddFunction("RT_Timer",					"",RT_Timer, 0);
 env->AddFunction("RT_IncrGlobal",				"s[Init]i",RT_IncrGlobal, 0);
 env->AddFunction("RT_Version",					"",RT_Version, 0);
 env->AddFunction("RT_VersionString",			"",RT_VersionString, 0);
 env->AddFunction("RT_VersionDll",				"",RT_VersionDll, 0);
 env->AddFunction("RT_PluginDir",				"",RT_PluginDir, 0);
 env->AddFunction("RT_PluginFunctions",			"",RT_PluginFunctions, 0);
 env->AddFunction("RT_InternalFunctions",		"",RT_InternalFunctions, 0);
 env->AddFunction("RT_ScriptName",				"",RT_ScriptName, 0);
 env->AddFunction("RT_ScriptFile",				"",RT_ScriptFile, 0);
 env->AddFunction("RT_ScriptDir",				"",RT_ScriptDir, 0);
 env->AddFunction("RT_PluginParam",				"s",RT_PluginParam, 0);
 env->AddFunction("RT_GetSystemEnv",			"s",RT_GetSystemEnv, 0);
 env->AddFunction("RT_GetFileTime",				"si",RT_GetFileTime, 0);
 env->AddFunction("RT_LocalTimeString",			"[file]b",RT_LocalTimeString, 0);
 env->AddFunction("RT_VarType",					".",RT_VarType, 0);
 env->AddFunction("RT_VarIsSame",				"..+[sig]b",RT_VarIsSame, 0);
 env->AddFunction("RT_TimerHP",					"",RT_TimerHP, 0);
 env->AddFunction("RT_FileQueryLines",			"s",RT_FileQueryLines, 0);
 env->AddFunction("RT_ReadTxtFromFile",			"s[Lines]i[Start]i",RT_ReadTxtFromFile, 0);
 env->AddFunction("RT_WriteFileList",			"ss[append]b",RT_WriteFileList, 0);
 env->AddFunction("RT_TxtWriteFile",			"ss[append]b",RT_TxtWriteFile, 0);
 env->AddFunction("RT_FileDelete",				"s",RT_FileDelete, 0);
 env->AddFunction("RT_FileFindStr",				"ss[sig]b[pos]i[start]i[lines]i",RT_FileFindStr, 0);
 env->AddFunction("RT_WriteFile",				"ss.*[Append]b",RT_WriteFile, 0);
 env->AddFunction("RT_FileRename",				"ss",RT_FileRename, 0);
 env->AddFunction("RT_FSelOpen"  ,				"[title]s[dir]s[filt]s[fn]s[multi]b[debug]b",RT_FSelOpen,0);
 env->AddFunction("RT_FSelSaveAs",				"[title]s[dir]s[filt]s[fn]s[debug]b",RT_FSelSaveAs,0);
 env->AddFunction("RT_FSelFolder",				"[title]s[dir]s[debug]b",RT_FSelFolder,0);
 env->AddFunction("RT_YDifference",				"c[n]i[delta]i[x]i[y]i[w]i[h]i[x2]i[y2]i[Interlaced]b[matrix]i",RT_YDifference, 0);
 env->AddFunction("RT_LumaDifference",			"cc[n]i[delta]i[x]i[y]i[w]i[h]i[n2]i[delta2]i[x2]i[y2]i[Interlaced]b[matrix]i",RT_LumaDifference, 0);
 env->AddFunction("RT_LumaCorrelation",			"cc[n]i[delta]i[x]i[y]i[w]i[h]i[n2]i[delta2]i[x2]i[y2]i[Interlaced]b[matrix]i",RT_LumaCorrelation, 0);
 env->AddFunction("RT_LumaPixelsDifferentCount","cc[n]i[delta]i[x]i[y]i[w]i[h]i[n2]i[delta2]i[x2]i[y2]i[Interlaced]b[matrix]i[Thresh]i",RT_LumaPixelsDifferentCount, 0);
 env->AddFunction("RT_LumaPixelsDifferent",		"cc[n]i[delta]i[x]i[y]i[w]i[h]i[n2]i[delta2]i[x2]i[y2]i[Interlaced]b[matrix]i[Thresh]i",RT_LumaPixelsDifferent, 0);
 env->AddFunction("RT_RGB32AsFloat",			"c[n]i[delta]i[x]i[y]i",RT_RGB32AsFloat, 0);
 env->AddFunction("RT_RGB32AsInt",				"c[n]i[delta]i[x]i[y]i",RT_RGB32AsInt, 0);
 env->AddFunction("RT_FloatAsRGBA",				"f",RT_FloatAsRGBA, 0);
 env->AddFunction("RT_YankChain",				"c[n]i[delta]i",RT_YankChain, 0);
 env->AddFunction("RT_TxtQueryLines",			"s",RT_TxtQueryLines, 0);
 env->AddFunction("RT_TxtGetLine",				"s[Line]i",RT_TxtGetLine, 0);
 env->AddFunction("RT_StrAddStr",				"ss+",RT_StrAddStr, 0);
 env->AddFunction("RT_TxtAddStr",				"ss+",RT_TxtAddStr, 0);
 env->AddFunction("RT_QuoteStr",				"s",RT_QuoteStr, 0);
 env->AddFunction("RT_StrReplace",				"sss[sig]b",RT_StrReplace, 0);
 env->AddFunction("RT_StrReplaceDeep",			"sss[sig]b",RT_StrReplaceDeep, 0);
 env->AddFunction("RT_StrReplaceMulti",			"sss[sig]b",RT_StrReplaceMulti, 0);
 env->AddFunction("RT_StrPad",					"si[c]s",RT_StrPad, 0);
 env->AddFunction("RT_FindStr",					"ss[sig]b[pos]i",RT_FindStr, 0);
 env->AddFunction("RT_TxtFindStr",				"ss[sig]b[pos]i[start]i[lines]i",RT_TxtFindStr, 0);
 env->AddFunction("RT_String",					"s.*[esc]i",RT_String, 0);
 env->AddFunction("RT_TxtSort",					"s[mode]i",RT_TxtSort, 0);
 env->AddFunction("RT_GetWorkingDir",			"",RT_GetWorkingDir, 0);
 env->AddFunction("RT_FilenameSplit",			"s[get]i",RT_FilenameSplit, 0);
 env->AddFunction("RT_GetFullPathName",			"s",RT_GetFullPathName, 0);
 env->AddFunction("RT_GetFileExtension",		"s",RT_GetFileExtension, 0);
 env->AddFunction("RT_QueryLumaMinMax",			"c[Samples]i[Ignore]f[Prefix]s[Debug]b[X]i[Y]i[W]i[H]i[Matrix]i[Start]i[End]i",RT_QueryLumaMinMax,0);
 env->AddFunction("RT_GetSAR",					"cf",RT_GetSAR,0);
 env->AddFunction("RT_GetDAR",					"cf",RT_GetDAR,0);
 env->AddFunction("RT_SignalDAR",				"f" ,RT_SignalDAR,0);
 env->AddFunction("RT_SignalDAR2",				"ii" ,RT_SignalDAR2,0);
 env->AddFunction("RT_GetCropDAR",				"cf[x]f[y]f[w]f[h]f",RT_GetCropDAR,0);
 env->AddFunction("RT_ArrayAlloc",				"s[Type]i[Dim1]i[Dim2]i[Dim3]i[StringLenMax]i",RT_ArrayAlloc,0);
 env->AddFunction("RT_ArrayGetDim",				"s[Dim]i",RT_ArrayGetDim,0);
 env->AddFunction("RT_ArrayGetType",			"s",RT_ArrayGetType,0);
 env->AddFunction("RT_ArrayGetElSize",			"s",RT_ArrayGetElSize,0);
 env->AddFunction("RT_ArrayGet",				"si[ix2]i[ix3]i",RT_ArrayGet,0);
 env->AddFunction("RT_ArraySet",				"s.i[ix2]i[ix3]i",RT_ArraySet,0);
 env->AddFunction("RT_ArrayExtend",				"s[Add]i",RT_ArrayExtend,0);
 env->AddFunction("RT_ArrayAppend",				"s.",RT_ArrayAppend,0);
 env->AddFunction("RT_ArrayGetAttrib",			"si",RT_ArrayGetAttrib,0);
 env->AddFunction("RT_ArrayGetStrAttrib",		"si" ,RT_ArrayGetStrAttrib, 0);
 env->AddFunction("RT_ArrayTypeName",			"i"  ,RT_ArrayTypeName, 0); 
 env->AddFunction("RT_ArrayGetID",				"si", RT_ArrayGetID , 0);
 env->AddFunction("RT_DBaseAlloc",				"sis[StringLenMax]i",RT_DBaseAlloc,0);
 env->AddFunction("RT_DBaseFields",				"s",RT_DBaseFields,0);
 env->AddFunction("RT_DBaseFieldType",			"si",RT_DBaseFieldType,0);
 env->AddFunction("RT_DBaseFieldSize",			"si",RT_DBaseFieldSize,0);
 env->AddFunction("RT_DBaseRecords",			"s",RT_DBaseRecords,0);
 env->AddFunction("RT_DBaseRecordSize",			"s",RT_DBaseRecordSize,0);
 env->AddFunction("RT_DBaseGetField",			"sii",RT_DBaseGetField,0);
 env->AddFunction("RT_DBaseSet",				"si.+",RT_DBaseSet,0);
 env->AddFunction("RT_DBaseExtend",				"s[Add]i",RT_DBaseExtend,0);
 env->AddFunction("RT_DBaseAppend",				"s.+",RT_DBaseAppend,0);
 env->AddFunction("RT_DBaseGetAttrib",			"si",RT_DBaseGetAttrib,0);
 env->AddFunction("RT_DBaseTypeName",			"i"  ,RT_DBaseTypeName, 0);
 env->AddFunction("RT_ChanAve",					"c[n]i[Prefix]s",RT_ChanAve, 0);
 env->AddFunction("RT_RgbChanMin",				"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[threshold]f[Chan]i[mask]c[maskmin]i[maskmax]i",RT_RgbChanMin, 0);
 env->AddFunction("RT_RgbChanMax",				"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[threshold]f[Chan]i[mask]c[maskmin]i[maskmax]i",RT_RgbChanMax, 0);
 env->AddFunction("RT_RgbChanMinMaxDifference",	"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[threshold]f[Chan]i[mask]c[maskmin]i[maskmax]i",RT_RgbChanMinMaxDifference, 0);
 env->AddFunction("RT_RgbChanMedian",			"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[Chan]i[mask]c[maskmin]i[maskmax]i",RT_RgbChanMedian, 0);
 env->AddFunction("RT_RgbChanStdev",			"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[Chan]i[mask]c[maskmin]i[maskmax]i",RT_RgbChanStdev, 0);
 env->AddFunction("RT_RgbChanInRange",     		"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[Chan]i[lo]i[hi]i[mask]c[maskmin]i[maskmax]i",RT_RgbChanInRange, 0);
 env->AddFunction("RT_RgbChanPNorm",     		"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[Chan]i[mu]f[d]i[p]i[u]i[mask]c[maskmin]i[maskmax]i",RT_RgbChanPNorm, 0);
 env->AddFunction("RT_RgbChanStats",			"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[threshold]f[Chan]i[lo]i[hi]i[flgs]i[prefix]s[mu]f[d]i[p]i[u]i[mask]c[maskmin]i[maskmax]i",RT_RgbChanStats, 0);
 env->AddFunction("RT_YPlaneMin",				"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[threshold]f[matrix]i[mask]c[maskmin]i[maskmax]i",RT_YPlaneMin, 0);
 env->AddFunction("RT_YPlaneMax",				"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[threshold]f[matrix]i[mask]c[maskmin]i[maskmax]i",RT_YPlaneMax, 0);
 env->AddFunction("RT_YPlaneMinMaxDifference",	"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[threshold]f[matrix]i[mask]c[maskmin]i[maskmax]i",RT_YPlaneMinMaxDifference, 0);
 env->AddFunction("RT_YPlaneMedian",			"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[matrix]i[mask]c[maskmin]i[maskmax]i",RT_YPlaneMedian, 0);
 env->AddFunction("RT_AverageLuma",				"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[matrix]i[mask]c[maskmin]i[maskmax]i",RT_AverageLuma, 0);
 env->AddFunction("RT_YPlaneStdev",				"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[matrix]i[mask]c[maskmin]i[maskmax]i",RT_YPlaneStdev, 0);
 env->AddFunction("RT_YInRange",        		"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[matrix]i[lo]i[hi]i[mask]c[maskmin]i[maskmax]i",RT_YInRange, 0);
 env->AddFunction("RT_YPNorm",					"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[matrix]i[mu]f[d]i[p]i[u]i[mask]c[maskmin]i[maskmax]i",RT_YPNorm, 0);
 env->AddFunction("RT_YStats",					"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[threshold]f[matrix]i[lo]i[hi]i[flgs]i[prefix]s[mu]f[d]i[p]i[u]i[mask]c[maskmin]i[maskmax]i",RT_YStats, 0);
 env->AddFunction("RT_RgbChanAve",				"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[Chan]i[mask]c[maskmin]i[maskmax]i",RT_RgbChanAve, 0);
 env->AddFunction("RT_QwikScanEstimateLumaTol",	"cc[n]i[n2]i[matrix]i", RT_QwikScanEstimateLumaTol , 0);
 env->AddFunction("RT_AvgLumaDif",				"c[n]i[slices]b[matrix]i", RT_AvgLumaDif, 0);
 env->AddFunction("RT_RgbInRange",				"c[n]i[delta]i[x]i[y]i[w]i[h]i[Interlaced]b[Rlo]i[Rhi]i[Glo]i[Ghi]i[Blo]i[Bhi]i",RT_RgbInRange, 0);
 env->AddFunction("RT_DBaseGetStrAttrib",		"si" ,RT_DBaseGetStrAttrib, 0);
 env->AddFunction("RT_DBaseGetID",				"si", RT_DBaseGetID , 0);

//////////////////////////// CHANGED IN 2 /////////////

 env->AddFunction("RT_ArrayGetDim1Max",			"s[Current]b",RT_ArrayGetDim1Max,0);
 env->AddFunction("RT_ArraySetAttrib",			"si.+",RT_ArraySetAttrib,0);
 env->AddFunction("RT_ArraySetID",				"si.+",RT_ArraySetID , 0);
 env->AddFunction("RT_ArraySetStrAttrib",		"sis+",RT_ArraySetStrAttrib, 0);
 env->AddFunction("RT_DBaseRecordsMax",			"s[Current]b",RT_DBaseRecordsMax,0);
 env->AddFunction("RT_DBaseSetAttrib",			"si.+",RT_DBaseSetAttrib,0);
 env->AddFunction("RT_DBaseSetField",			"sii.+",RT_DBaseSetField,0);
 env->AddFunction("RT_DBaseSetID",				"si.+",RT_DBaseSetID , 0);
 env->AddFunction("RT_DBaseSetStrAttrib",		"sis+",RT_DBaseSetStrAttrib, 0);
 env->AddFunction("RT_FrameDifference",			"cc[n]i[n2]i[ChromaWeight]f[x]i[y]i[w]i[h]i[x2]i[y2]i[altscan]b[ChromaI]b[Matrix]i",RT_FrameDifference, 0);
 env->AddFunction("RT_QueryBorderCrop",			"c[Samples]i[Thresh]f[Laced]b[xMod]i[yMod]i[wMod]i[hMod]i[Relative]b[Prefix]s[RLBT]i[Debug]b[Ignore]f[Matrix]i[ScanPerc]f[Baffle]i[ScaleAutoThreshRGB]b[ScaleAutoThreshYUV]b[ATM]f[Start]i[End]i[LeftAdd]i[TopAdd]i[RightAdd]i[BotAdd]i[LeftSkip]i[TopSkip]i[RightSkip]i[BotSkip]i",RT_QueryBorderCrop,0);
 env->AddFunction("RT_QwikScanCreate",			"cs[Prev]s[Next]s[Matrix]i[Debug]b[x]i[y]i[w]i[h]i",RT_QwikScanCreate, 0);
 env->AddFunction("RT_QwikScan",				"ciciss[LumaTol]f[Flags]i[LC]f[LD]f[FD]f[PD]f[PC]i[XP]i[ChromaWeight]f[PDThresh]i[MaxDistance]i[Inclusive]b[Prefix]s[FM]f[blkW]i[blkH]i[olapx]i[olapy]i[FpFailMax]i",RT_QwikScan, 0);
 env->AddFunction("RT_RGBInRangeLocate",		"c[n]i[delta]i[x]i[y]i[w]i[h]i[baffle]i[thresh]f[Rlo]i[Rhi]i[Glo]i[Ghi]i[Blo]i[Bhi]i[prefix]s[debug]b[baffle_w]i[baffle_h]i[thresh_w]f[thresh_h]f[rescan]b",RT_RGBInRangeLocate,0);
 env->AddFunction("RT_YInRangeLocate",			"c[n]i[delta]i[x]i[y]i[w]i[h]i[baffle]i[thresh]f[lo]i[hi]i[matrix]i[prefix]s[debug]b[baffle_w]i[baffle_h]i[thresh_w]f[thresh_h]f[rescan]b",RT_YInRangeLocate,0);

/////////////////////////////////////
///////// BELOW NEW IN 2 \\\\\\\\\\\\
/////////////////////////////////////

 env->AddFunction("RT_ArrayCheckID",			"si.+",RT_ArrayCheckID, 0);
 env->AddFunction("RT_DBaseCheckID",			"si.+",RT_DBaseCheckID, 0);
 env->AddFunction("RT_DBaseFindSeq",		    "siii[low]i[high]i",RT_DBaseFindSeq, 0);
 env->AddFunction("RT_DBaseGetTypeString",		"s",   RT_DBaseGetTypeString, 0);
 env->AddFunction("RT_FileDuplicate",			"ss[Overwrite]b",RT_FileDuplicate, 0);
 env->AddFunction("RT_ForceProcess",			"c[Video]b[Audio]b[debug]b",   RT_ForceProcess, 0);
 env->AddFunction("RT_FrameMovement",  			"cc[n]i[n2]i[ChromaWeight]f[x]i[y]i[w]i[h]i[x2]i[y2]i[altscan]b[ChromaI]b[Matrix]i[blkw]i[blkh]i[OlapX]i[OLapY]i[BlkTh]f[Prefix]s",RT_FrameMovement, 0);
 env->AddFunction("RT_GetPid",  				"",RT_GetPid, 0);
 env->AddFunction("RT_RandInt",					"[RandMax]i",RT_RandInt, 0);
 env->AddFunction("RT_Sleep",				    "f",RT_Sleep, 0);

 env->AddFunction("RT_DBaseInsertSort",			"s[lo]i[hi]i[Field]i[Field2]i[Field3]i[Sig]b[Ascend]b[Debug]b",RT_DBaseInsertSort, 0);
 env->AddFunction("RT_DBaseQuickSort",			"s[lo]i[hi]i[Field]i[Field2]i[Field3]i[Sig]b[Ascend]b[Debug]b[ins]i[randPivot]b",RT_DBaseQuickSort, 0);

 env->AddFunction("RT_DBaseReadCSV",		    "ss[separator]s[StrDelimiter]s[StartField]i[EndField]i",RT_DBaseReadCSV, 0);
 env->AddFunction("RT_DBaseWriteCSV",		    "ss[separator]s[StrDelimiter]s[StartField]i[EndField]i[low]i[high]i[Append]b[Fmt]s",RT_DBaseWriteCSV, 0);

 return "`RT_Stats' RT_Stats plugin";
}

