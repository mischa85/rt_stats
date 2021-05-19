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
	No real advantage to using the switch case method to avoid loop counter overhead for RGB, so we dont do it.
*/

#include "RT_Stats.h"

double __cdecl PVF_AverageLuma_RGB(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,const bool altscan,
			const int matrix,const bool IsRGB32) {
	int xstep = (IsRGB32) ? 4 : 3;
	const int ystep	   = (altscan) ? 2:1;
    const int pitch    = src->GetPitch();
    const int height   = src->GetHeight();
	const int ystride  = pitch*ystep;
	const BYTE  *srcp  = src->GetReadPtr() + ((height-1 - yy) * pitch) + (xx * xstep);
	int64_t acc		 = 0;
    unsigned int sum = 0;
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	const int mat = matrix & 0x03;
    // RGB to YUV-Y Conversion
	// Matrix: Default=0=REC601 : 1=REC709 : 2 = PC601 : 3 = PC709
	double				Kr,Kb;
	int					S_y,offset_y;
	if(mat & 0x01)		{Kr = 0.2126; Kb        = 0.0722;}			// 709  1 or 3
	else                {Kr = 0.2990; Kb        = 0.1140;}			// 601  0 or 2
	if(mat & 0x02)		{S_y = 255   ; offset_y  = 0;}				// PC   2 or 3
	else                {S_y = 219   ; offset_y  = 16;}				// TV   0 or 1
	const int			shift	=   15;
	const int           half    =   1 << (shift - 1);
	const double        mulfac  =   double(1<<shift);
	double              Kg      =   1.0 - Kr - Kb;
	const int           Srgb    =   255;
	const int			Yb = int(S_y  * Kb        * mulfac / Srgb + 0.5); //B
	const int			Yg = int(S_y  * Kg        * mulfac / Srgb + 0.5); //G
	const int			Yr = int(S_y  * Kr        * mulfac / Srgb + 0.5); //R
	const int			OffyPlusHalf = (offset_y<<shift) + half;
	//
	if(ww == 1) {														// Special case for single pixel width
		for(int y=yhit ; --y>=0; ) {
			sum += (srcp[0] * Yb + srcp[1] * Yg + srcp[2] * Yr + OffyPlusHalf) >> shift;
			srcp -= ystride;
		}
	} else {
		const int wm = ww * xstep;
		if(IsRGB32) {
			for(int y=yhit; --y>=0 ; ) {
				for(int x=wm; (x-=4)>=0 ; ) {
					sum += (srcp[x+0]*Yb + srcp[x+1]*Yg + srcp[x+2]*Yr + OffyPlusHalf) >> shift;
				}
				if(sum & 0x80000000) {acc += sum;sum=0;}					// avoid possiblilty of overflow
				srcp -= ystride;
			}
		} else {
			for(int y=yhit; --y>=0 ; ) {
				for(int x=wm; (x-=3)>=0 ; ) {
					sum += (srcp[x+0]*Yb + srcp[x+1]*Yg + srcp[x+2]*Yr + OffyPlusHalf) >> shift;
				}
				if(sum & 0x80000000) {acc += sum;sum=0;}					// avoid possiblilty of overflow
				srcp -= ystride;
			}
		}
	}
	acc += sum;
	double result = ((double)acc / Pixels);
    return result;
}

int __cdecl PVF_CountLuma_RGB(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,const bool altscan,
			unsigned int *cnt,const int matrix,const bool IsRGB32) {
	int xstep = (IsRGB32) ? 4 : 3;
	const int ystep	   = (altscan) ? 2:1;
    const int pitch    = src->GetPitch();
    const int height   = src->GetHeight();
	const int ystride  = pitch*ystep;
	const BYTE  *srcp  = src->GetReadPtr() + ((height-1 - yy) * pitch) + (xx * xstep);
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	const int mat = matrix & 0x03;
	for(int i=256;--i>=0;cnt[i]=0);
    // RGB to YUV-Y Conversion
	// Matrix: Default=0=REC601 : 1=REC709 : 2 = PC601 : 3 = PC709
	double				Kr,Kb;
	int					S_y,offset_y;
	if(mat & 0x01)		{Kr = 0.2126; Kb        = 0.0722;}			// 709  1 or 3
	else                {Kr = 0.2990; Kb        = 0.1140;}			// 601  0 or 2
	if(mat & 0x02)		{S_y = 255   ; offset_y  = 0;}				// PC   2 or 3
	else                {S_y = 219   ; offset_y  = 16;}				// TV   0 or 1
	const int			shift	=   15;
	const int           half    =   1 << (shift - 1);
	const double        mulfac  =   double(1<<shift);
	double              Kg      =   1.0 - Kr - Kb;
	const int           Srgb    =   255;
	const int			Yb = int(S_y  * Kb        * mulfac / Srgb + 0.5); //B
	const int			Yg = int(S_y  * Kg        * mulfac / Srgb + 0.5); //G
	const int			Yr = int(S_y  * Kr        * mulfac / Srgb + 0.5); //R
	const int			OffyPlusHalf = (offset_y<<shift) + half;
	//
	if(ww == 1) {														// Special case for single pixel width
		for(int y=yhit ; --y>=0; ) {
			++cnt[(srcp[0] * Yb + srcp[1] * Yg + srcp[2] * Yr + OffyPlusHalf) >> shift];
			srcp -= ystride;
		}
	} else {
		const int wm = ww * xstep;
		if(IsRGB32) {
			for(int y=yhit; --y>=0 ; ) {
				for(int x=wm; (x-=4)>=0 ; ) {
					++cnt[(srcp[x+0]*Yb + srcp[x+1]*Yg + srcp[x+2]*Yr + OffyPlusHalf) >> shift];
				}
				srcp -= ystride;
			}
		} else {
			for(int y=yhit; --y>=0 ; ) {
				for(int x=wm; (x-=3)>=0 ; ) {
					++cnt[(srcp[x+0]*Yb + srcp[x+1]*Yg + srcp[x+2]*Yr + OffyPlusHalf) >> shift];
				}
				srcp -= ystride;
			}
		}
	}
    return Pixels;
}


