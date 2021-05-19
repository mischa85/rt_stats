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

#ifndef __RT_EXTDEF_H__
 	extern FILE * __cdecl ARR_Read_Header(const char *myName,const char *fn,const char *omd,MYARR &arr,IScriptEnvironment* env,char *retbf=NULL);


	extern AVSValue __cdecl GetVar(IScriptEnvironment* env, const char* name);
	extern int __cdecl dprintf(char* fmt, ...);
	extern void __cdecl SplitFn(const char *fn,char*path,char *name,char *ext);
	extern void __cdecl Ucase(char *s);
	// extern char * __cdecl GetErrorString(unsigned int dwLastError=GetLastError());
	extern const char * __cdecl StrStrC(const char *s,const char *sub,const bool sig,int slen = -1,int  sublen = -1);
	extern int __cdecl		RT_MYstats_Lo(int flgs,const AVSValue &std,const AVSValue &xtra,MYLO &ylo,const char*Name,IScriptEnvironment* env,unsigned int*histp=NULL);
	extern int __cdecl		RT_MRGBChanstats_Lo(int flgs,const AVSValue &std,const AVSValue &xtra,MRGBALO &rgb,const char*Name,IScriptEnvironment* env);
	extern double __cdecl	RT_LumaCorrelation_Lo(const AVSValue &args, IScriptEnvironment* env);
	extern double __cdecl   RT_LumaDifference_Lo(const AVSValue &args,const char *Name,IScriptEnvironment* env);
	extern int __cdecl		RT_LumaPixelsDifferentCount_Lo(const AVSValue &args,IScriptEnvironment* env,double *dp=NULL);
	extern double __cdecl	RT_LumaPixelsDifferent_Lo(const AVSValue &args,IScriptEnvironment* env);
	extern double __cdecl	RT_RgbInRange_Lo(const AVSValue &std,const AVSValue &rgbminmax,IScriptEnvironment* env);
	extern int __cdecl		RT_ChanAve_Lo(const AVSValue &clip,const AVSValue &frame,double ret[3],const char*Name,IScriptEnvironment* env);
	extern double __cdecl	RT_TimerHP_Lo(void);
//
	extern AVSValue __cdecl RT_ColorSpaceXMod(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ColorSpaceYMod(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl Create_RT_GraphLink(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl Create_RT_Stats(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl Create_RT_Subtitle(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_Call(AVSValue args, void*, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_GetLastError(AVSValue args, void*, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_GetLastErrorString(AVSValue args, void*, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_Debug(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DebugF(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_GetProcessName(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_Undefined(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_VarExist(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FunctionExist(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_Ord(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_Timer(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_TimerHP(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_IncrGlobal(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_Version(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_VersionString(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_VersionDll(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_PluginDir(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_PluginFunctions(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_Plugins(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_InternalFunctions(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ScriptName(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ScriptFile(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ScriptDir(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_PluginParam(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_GetSystemEnv(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_GetFileTime(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_LocalTimeString(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_VarType(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_VarIsSame(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_WriteFileList(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FileQueryLines(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ReadTxtFromFile(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_TxtWriteFile(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FileDelete(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FileFindStr(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_WriteFile(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FileRename(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FSelOpen(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FSelSaveAs(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FSelFolder(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_Hex(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_HexValue(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_NumberString(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_NumberValue(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitNOT(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitAND(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitOR(AVSValue args,  void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitXOR(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitASL(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitASR(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitLSL(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitLSR(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitTST(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitCLR(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitSET(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitCHG(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitROR(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitROL(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_BitSetCount(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_YDifference(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_LumaCorrelation(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_LumaDifference(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RGB32AsFloat(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RGB32AsInt(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FloatAsRGBA(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_YankChain(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_StrAddStr(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_TxtAddStr(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_QuoteStr(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_StrReplace(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_StrReplaceDeep(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_StrReplaceMulti(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_StrPad(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FindStr(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_TxtQueryLines(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_TxtGetLine(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_TxtFindStr(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_String(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_TxtSort(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_GetWorkingDir(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FilenameSplit(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_GetFullPathName(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_GetFileExtension(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_QueryLumaMinMax(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_QueryBorderCrop(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_YInRangeLocate(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RgbInRange(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RGBInRangeLocate(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_GetSAR(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_GetDAR(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_SignalDAR(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_SignalDAR2(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_GetCropDAR(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArrayAlloc(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArrayGetDim(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArrayGetType(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArrayGetElSize(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArrayGetDim1Max(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArrayGet(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArraySet(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArrayExtend(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArrayAppend(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArrayGetAttrib(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArraySetAttrib(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArrayGetStrAttrib(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArraySetStrAttrib(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArrayTypeName(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArrayGetID(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ArraySetID(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseAlloc(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseRecords(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseFields(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseRecordSize(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseRecordsMax(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseFieldType(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseFieldSize(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseExtend(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseGetAttrib(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseSetAttrib(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseGetField(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseSetField(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseSet(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseAppend(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseGetStrAttrib(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseSetStrAttrib(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseTypeName(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseGetID(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseSetID(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RgbChanMin(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RgbChanMax(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RgbChanMinMaxDifference(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RgbChanMedian(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RgbChanAve(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RgbChanStdev(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RgbChanInRange(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RgbChanPNorm(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RgbChanStats(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_YPlaneMin(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_YPlaneMax(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_YPlaneMinMaxDifference(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_YPlaneMedian(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_AverageLuma(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_YPlaneStdev(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_YInRange(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_YPNorm(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_YStats(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ChanAve(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_LumaPixelsDifferentCount(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_LumaPixelsDifferent(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_QwikScanCreate(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_QwikScan(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_QwikScanEstimateLumaTol(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_AvgLumaDif(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FrameDifference(AVSValue args, void* user_data, IScriptEnvironment* env);
// CHANGED IN 2
	extern double __cdecl   RT_FrameDifference_Lo(const AVSValue &args,const char*name,IScriptEnvironment* env);
// ADDED IN 2
	extern int __cdecl      RandInt_Lo(int randmax);
	extern int __cdecl      QueryFatVolume(const char *relname);
	extern int64_t __cdecl  QueryDiskFreeSpace(const char *relname);
	extern int64_t __cdecl  QueryMaxFileSize(const char *relname);
	extern double __cdecl PVF_LumaCorrelation_Planar(const PVideoFrame &src,const PVideoFrame &src2,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan);
	extern double __cdecl PVF_AverageLuma_Planar(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,const bool altscan);
	extern int __cdecl    PVF_CountLuma_Planar(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,const bool altscan,unsigned int *cnt);
	extern double __cdecl PVF_LumaDifference_Planar(const PVideoFrame &src,const PVideoFrame &src2,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan);
	extern double __cdecl PVF_PixelDifference_Planar(const PVideoFrame &src,const PVideoFrame &src2,const double CW,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,const bool altscan,bool ChromaI);
	extern unsigned int __cdecl PVF_LumaPixelsDifferentCount_Planar(const PVideoFrame &src,const PVideoFrame &src2,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan,const int thresh,double *dp);

	extern double __cdecl PVF_AverageLuma_YUY2(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,const bool altscan);
	extern int __cdecl    PVF_CountLuma_YUY2(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,const bool altscan,unsigned int *cnt);
	extern double __cdecl PVF_LumaDifference_YUY2(const PVideoFrame &src,const PVideoFrame &src2,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan);
	extern double __cdecl PVF_PixelDifference_YUY2(const PVideoFrame &src,const PVideoFrame &src2,const double CW,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,const bool altscan);
	extern unsigned int __cdecl PVF_LumaPixelsDifferentCount_YUY2(const PVideoFrame &src,const PVideoFrame &src2,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan,const int thresh,double *dp);
	extern double __cdecl PVF_LumaCorrelation_YUY2(const PVideoFrame &src,const PVideoFrame &src2,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan);

	extern double __cdecl PVF_AverageLuma_RGB(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,const bool altscan,const int matrix,const bool IsRGB32);
	extern int __cdecl    PVF_CountLuma_RGB(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,const bool altscan,unsigned int *cnt,const int matrix,const bool IsRGB32);
	extern double __cdecl PVF_LumaDifference_RGB(const PVideoFrame &src,const PVideoFrame &src2,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,const bool altscan,const int matrix,const bool IsRGB32);
	extern double __cdecl PVF_PixelDifference_RGB(const PVideoFrame &src,const PVideoFrame &src2,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,const bool altscan,const bool IsRGB32);
	extern unsigned int __cdecl PVF_LumaPixelsDifferentCount_RGB(const PVideoFrame &src,const PVideoFrame &src2,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan,const int thresh,double *dp,const int matrix,const bool IsRGB32);
	extern double __cdecl PVF_LumaCorrelation_RGB(const PVideoFrame &src,const PVideoFrame &src2,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscanconst,int matrix,const bool IsRGB32);
	extern double __cdecl RT_FrameMovement_Lo(const AVSValue &args,const char*name,int *coordsP,double *PercAboveTh,double *BlkAveDf,int *nAboveTh,int *TotalBlks,IScriptEnvironment* env);
//
	extern AVSValue __cdecl RT_FrameMovement(AVSValue args, void* user_data, IScriptEnvironment* env);

	extern AVSValue __cdecl RT_ArrayCheckID(AVSValue args, void* user_data, IScriptEnvironment* env);

	extern AVSValue __cdecl RT_DBaseCheckID(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseGetTypeString(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseFindSeq(AVSValue args, void* user_data, IScriptEnvironment* env);

	extern AVSValue __cdecl RT_GetPid(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_FileDuplicate(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_RandInt(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_Sleep(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_ForceProcess(AVSValue args, void* user_data, IScriptEnvironment* env);

	extern AVSValue __cdecl RT_DBaseInsertSort(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseQuickSort(AVSValue args, void* user_data, IScriptEnvironment* env);

	extern AVSValue __cdecl RT_DBaseReadCSV(AVSValue args, void* user_data, IScriptEnvironment* env);
	extern AVSValue __cdecl RT_DBaseWriteCSV(AVSValue args, void* user_data, IScriptEnvironment* env);


#endif // __RT_EXTDEF_H__
