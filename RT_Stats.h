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
*/

#ifndef __RT_STATS_H__


	// #include <linux/compiler.h>

	#define VERSION_NUMBER	2.0				// 2 Digits of precision
	#define VERSION_BETA	13				// writes 2 digits
	#define VERSION_DATE	"29 Dec 2020"

	//

	// #include <windows.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <math.h>
	#include <float.h>
	// #include <tlhelp32.h>
	// #include <direct.h>
	#include <sys/io.h>							// _chsize_s
	#include <errno.h>
	#include <time.h>
	// #include <commdlg.h>					// FSEL, OS Specific WINVER (Mainly)
	// #include <Shlobj.h>						// FSEL, IE Specific _WIN32_IE
	// #include <process.h>					// _getpid

	#ifdef AVISYNTH_PLUGIN_25
		#include "avisynth25.h"
	#else
		#include <avisynth/avisynth.h>
	#endif

	enum {	RTMIN,
			RTMAX,
			RTMINMAX,
			RTMEDIAN,
			RTAVE,
			RTSTDEV,
			RTYINRNG,
			RTPNORM,
			RTHIST=31,
			//
			RTMIN_F		=(1<<RTMIN),		//	$01
			RTMAX_F		=(1<<RTMAX),		//	$02
			RTMINMAX_F	=(1<<RTMINMAX),		//	$04
			RTMEDIAN_F	=(1<<RTMEDIAN),		//	$08
			RTAVE_F		=(1<<RTAVE),		//	$10
			RTSTDEV_F	=(1<<RTSTDEV),		//	$20
			RTYINRNG_F	=(1<<RTYINRNG),		//	$40
			RTPNORM_F	=(1<<RTPNORM),		//	$80
			RTHIST_F	=(1<<RTHIST),		//	$80000000
	};

	enum {	STD_CLIP,
			STD_FRAME,
			STD_DELTA,
			STD_XX,
			STD_YY,
			STD_WW,
			STD_HH,
			STD_LACED,
			STD_SIZE
	};

	enum {	XTRA_THRESH,
			XTRA_RGBIX,			// RGB Channel or Matrix
			XTRA_LO,
			XTRA_HI,
			XTRA_MU,
			XTRA_D,
			XTRA_P,
			XTRA_U,
			XTRA_MASK,
			XTRA_MASKMIN,
			XTRA_MASKMAX,
			XTRA_SIZE
	};

	struct mydat_stc{
		int		idat[RTMEDIAN+1];
		double	ddat[RTPNORM-RTAVE+1];
	};

	typedef struct mydat_stc MYDAT;

	struct my_stc{
		MYDAT	dat;
		int pixels;
	};

	typedef struct my_stc MYLO;

	// -------------------------------------------------------------

	struct mrgbdat_stc{
		int		idat[RTMEDIAN+1];
		double	ddat[RTPNORM-RTAVE+1];
	};

	typedef struct mrgbdat_stc MRGBDAT;

	struct mrgba_stc{
		MRGBDAT chans[4];
		int		pixels;
		int		chmin;
		int		chmax;
	};

	typedef struct mrgba_stc MRGBALO;

	enum {	RED_MIN,
			RED_MAX,
			GRN_MIN,
			GRN_MAX,
			BLU_MIN,
			BLU_MAX,
			RGBMINMAX_SIZE
	};

	// -------------------------------------------------------------

	// Attributes
	enum {
		ATTR_INT	= 0,
		ATTR_FLOAT	= 1
	};
	
	union attribute_un {
		int		i;
		float	f;
	};

	typedef union attribute_un MYATUN;

	struct myatel_stc{
		int		attype;
		MYATUN	attrib;
	};

	typedef struct myatel_stc MYATEL;

	// -------------------------------------------------------------
	union id_un {
		int		i;
		float	f;
	};

	typedef union id_un id2; 

	// Array

	enum {
		ARRAY_BOOL			= 0,
		ARRAY_INT			= 1,
		ARRAY_FLOAT			= 2,
		ARRAY_STRING		= 3,
		ARRAY_BIN			= 4,
		ARRAY_DOUBLE		= 5
	};

	enum {
		DB_BOOL			= 0,
		DB_INT			= 1,
		DB_FLOAT		= 2,
		DB_STRING		= 3,
		DB_BIN			= 4,
		DB_DOUBLE		= 5
	};

	struct myarr_stc{
		unsigned int name;
		unsigned int ver;
		unsigned int idtype[4];
		id2		id[128];
		int		offset;
		int		attriboffset;
		int     stroffset;
		BYTE	type;
		BYTE	resvd1[3];
		int		dim[4];							// [0]=num dims, [1]=size of dim 1 etc
		int		mul[3];
		int		elsz;
		int		dim1max;
		int		dim1Bytes;
		int     attribs;
		int     ustrings;
		int     ustrlen;
	};

	typedef struct myarr_stc MYARR;

	// -------------------------------------------------------------

	// DBase

	struct mydb_stc{
		unsigned int name;
		unsigned int ver;
		unsigned int idtype[4];
		id2		id[128];
		int		offset;
		int		infoffset;
		int		attriboffset;
		int     stroffset;
		int		records;
		int		recordsz;
		int		recordmax;
		int		maxfieldsz;
		int		fields;
		int     attribs;
		int     ustrings;
		int     ustrlen;
	};

	typedef struct mydb_stc MYDB;

	struct mydbinf_stc{
		int		fieldtype;
		int		fieldsize;
		int		fieldoff;
	};

	typedef struct mydbinf_stc MYDBINF;

	///////////////////////////////////////////////

	#define QBC_SAMPLES  40
	#define QBC_THRESH   40.0f

	#define QLMM_SAMPLES 40
	#define QLMM_IGNORE  0.0f

	#include "RT_ExtDef.h"
#endif // __RT_STATS_H__

