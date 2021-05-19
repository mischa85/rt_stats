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
#include <algorithm>

double __cdecl PVF_LumaDifference_RGB(const PVideoFrame &src,const PVideoFrame &src2,
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,const bool altscan,
		const int matrix,const bool IsRGB32) {
	// RGB to YUV-Y Conversion
    const int pitch    = src->GetPitch();
    const int pitch2   = src2->GetPitch();
	const int ystep	   = (altscan) ? 2:1;
	const int ystride  = pitch*ystep;
	const int ystride2 = pitch2*ystep;
	const int height   = src->GetHeight();
	const int height2  = src2->GetHeight();
	const int xstep	   = (IsRGB32) ? 4 : 3;
    const BYTE  *srcp  = src->GetReadPtr(PLANAR_Y)  + (height - 1 - yy) * pitch	 + (xx * xstep);
    const BYTE  *srcp2 = src2->GetReadPtr(PLANAR_Y) + (height2- 1 - yy2)* pitch2 + (xx2* xstep);
	int64_t acc		 = 0;
    unsigned int sum = 0;
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	// Matrix: Default=0=REC601 : 1=REC709 : 2 = PC601 : 3 = PC709
	const int mat = matrix & 0x03;
	double				Kr,Kb;
	int					Sy,offset_y;
	if(mat & 0x01)		{Kr = 0.2126; Kb        = 0.0722;}		// 709  1 or 3
	else                {Kr = 0.2990; Kb        = 0.1140;}		// 601  0 or 2
	if(mat & 0x02)		{Sy = 255   ; offset_y  = 0;}			// PC   2 or 3
	else                {Sy = 219   ; offset_y  = 16;}			// TV   0 or 1
	const int			shift	=   15;
	const int           half    =   1 << (shift - 1);
	const double        mulfac  =   double(1<<shift);
	double              Kg      =   1.0 - Kr - Kb;
	const int           Srgb    =   255;
	const int			Yb = int(Sy  * Kb        * mulfac / Srgb + 0.5); //B
	const int			Yg = int(Sy  * Kg        * mulfac / Srgb + 0.5); //G
	const int			Yr = int(Sy  * Kr        * mulfac / Srgb + 0.5); //R
	const int			OffyPlusHalf = (offset_y<<shift) + half;
	//
	if(ww == 1) {														// Special case for single pixel width
		for(int y = yhit; --y>=0 ;) {
			sum += abs(
				((srcp [0]	* Yb + srcp [1]	* Yg + srcp [2]	* Yr + OffyPlusHalf) >> shift) -
				((srcp2[0]	* Yb + srcp2[1]	* Yg + srcp2[2]	* Yr + OffyPlusHalf) >> shift)
			);				
			srcp  -= ystride;
			srcp2 -= ystride2;
		}
	} else {
		const int wm = ww * xstep;
		if(IsRGB32) {
			for(int y = yhit; --y>=0 ;) {
				for(int x=wm; (x-=4)>=0 ; ) {
					sum += abs(
						((srcp [x+0] * Yb + srcp [x+1] * Yg + srcp [x+2] * Yr + OffyPlusHalf) >> shift) -
						((srcp2[x+0] * Yb + srcp2[x+1] * Yg + srcp2[x+2] * Yr + OffyPlusHalf) >> shift)
					);
				}
				if(sum & 0x80000000) {acc += sum; sum=0;}				// avoid overflow
				srcp  -= ystride;
				srcp2 -= ystride2;
			}
		} else {
			for(int y = yhit; --y>=0 ;) {
				for(int x=wm; (x-=3)>=0 ; ) {
					sum += abs(
						((srcp [x+0] * Yb + srcp [x+1] * Yg + srcp [x+2] * Yr + OffyPlusHalf) >> shift) -
						((srcp2[x+0] * Yb + srcp2[x+1] * Yg + srcp2[x+2] * Yr + OffyPlusHalf) >> shift)
					);
				}
				if(sum & 0x80000000) {acc += sum; sum=0;}				// avoid overflow
				srcp  -= ystride;
				srcp2 -= ystride2;
			}
		}
	}
	acc += sum;
	double result = ((double)acc / Pixels);
    return result;
}

double __cdecl PVF_PixelDifference_RGB(const PVideoFrame &src,const PVideoFrame &src2,
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,const bool altscan,
		const bool IsRGB32) {
	// Average pixel difference
    const int pitch    = src->GetPitch();
    const int pitch2   = src2->GetPitch();
	const int ystep	   = (altscan) ? 2:1;
	const int ystride  = pitch*ystep;
	const int ystride2 = pitch2*ystep;
	const int height   = src->GetHeight();
	const int height2  = src2->GetHeight();
	const int xstep	   = (IsRGB32) ? 4 : 3;
    const BYTE  *srcp  = src->GetReadPtr()  + (height - 1 - yy) * pitch	 + (xx * xstep);
    const BYTE  *srcp2 = src2->GetReadPtr() + (height2- 1 - yy2)* pitch2 + (xx2* xstep);
	int64_t acc		 = 0;
    unsigned int sum = 0;
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	double result = 0.0;
	int x,y;
	const int wm = ww * xstep;
	const int hhy = (ystep==2) ? (hh+1)>>1:hh;
	if(ww == 1) {														// Special case for single pixel width
		for(int y = hhy; --y>=0 ;) {
			sum += abs(srcp[0]-srcp2[0]) + abs(srcp[1]-srcp2[1]) + abs(srcp[2]-srcp2[2]) ;
			srcp  -= ystride;
			srcp2 -= ystride2;
		}
	} else {
		if(IsRGB32) {
			for(y=hhy; --y>=0 ;) {
				for(x=wm; (x-=4)>=0 ; ) {
					sum += abs(srcp[x+0]-srcp2[x+0]) + abs(srcp[x+1]-srcp2[x+1]) + abs(srcp[x+2]-srcp2[x+2]) ;
				}
				if(sum & 0x80000000) {acc += sum; sum=0;}						// avoid possiblilty of overflow
				srcp  -= ystride;
				srcp2 -= ystride2;
			}
		} else {
			for(y=hhy; --y>=0 ;) {
				for(x=wm; (x-=3)>=0 ; ) {
					sum += abs(srcp[x+0]-srcp2[x+0]) + abs(srcp[x+1]-srcp2[x+1]) + abs(srcp[x+2]-srcp2[x+2]) ;
				}
				if(sum & 0x80000000) {acc += sum; sum=0;}						// avoid possiblilty of overflow
				srcp  -= ystride;
				srcp2 -= ystride2;
			}
		}
	}
	acc += sum;
    result = ((double)acc / (Pixels*3));
    return result;
}

unsigned int __cdecl PVF_LumaPixelsDifferentCount_RGB(const PVideoFrame &src,const PVideoFrame &src2,
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan,
		const int thresh,double *dp,const int matrix,const bool IsRGB32) {
    const int pitch    = src->GetPitch();
    const int pitch2   = src2->GetPitch();
	const int ystep	   = (altscan) ? 2:1;
	const int ystride  = pitch*ystep;
	const int ystride2 = pitch2*ystep;
	const int height   = src->GetHeight();
	const int height2  = src2->GetHeight();
	const int xstep	   = (IsRGB32) ? 4 : 3;
    const BYTE  *srcp  = src->GetReadPtr()  + (height - 1 - yy) * pitch	 + (xx * xstep);
    const BYTE  *srcp2 = src2->GetReadPtr() + (height2- 1 - yy2)* pitch2 + (xx2* xstep);
    unsigned int sum = 0;
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	const int th=std::min(std::max(thresh,0),255);
	// Matrix: Default=0=REC601 : 1=REC709 : 2 = PC601 : 3 = PC709
	const int mat = matrix & 0x03;
	double				Kr,Kb;
	int					Sy,offset_y;
	if(mat & 0x01)		{Kr = 0.2126; Kb        = 0.0722;}		// 709  1 or 3
	else                {Kr = 0.2990; Kb        = 0.1140;}		// 601  0 or 2
	if(mat & 0x02)		{Sy = 255   ; offset_y  = 0;}			// PC   2 or 3
	else                {Sy = 219   ; offset_y  = 16;}			// TV   0 or 1
	const int			shift	=   15;
	const int           half    =   1 << (shift - 1);
	const double        mulfac  =   double(1<<shift);
	double              Kg      =   1.0 - Kr - Kb;
	const int           Srgb    =   255;
	const int			Yb = int(Sy  * Kb        * mulfac / Srgb + 0.5); //B
	const int			Yg = int(Sy  * Kg        * mulfac / Srgb + 0.5); //G
	const int			Yr = int(Sy  * Kr        * mulfac / Srgb + 0.5); //R
	const int			OffyPlusHalf = (offset_y<<shift) + half;

	if(ww == 1) {														// Special case for single pixel width
		for(int y=yhit ; --y>=0;) {                       
			if (abs(
				((srcp [0] * Yb + srcp [1] * Yg + srcp [2] * Yr + OffyPlusHalf) >> shift) -
				((srcp2[0] * Yb + srcp2[1] * Yg + srcp2[2] * Yr + OffyPlusHalf) >> shift)
				) > th) ++sum;
			srcp  -= ystride;
			srcp2 -= ystride2;
		}
	} else {
		const int wm = ww * xstep;
		if(IsRGB32) {
			for(int y=yhit ; --y>=0;) {                       
				for(int x=wm; (x-=4)>=0 ; ) {
					if (abs(
						((srcp [x+0] * Yb + srcp [x+1] * Yg + srcp [x+2] * Yr + OffyPlusHalf) >> shift) -
						((srcp2[x+0] * Yb + srcp2[x+1] * Yg + srcp2[x+2] * Yr + OffyPlusHalf) >> shift)
						) > th) ++sum;
				}
				srcp  -= ystride;
				srcp2 -= ystride2;
			}
		} else {
			for(int y=yhit ; --y>=0;) {                       
				for(int x=wm; (x-=3)>=0 ; ) {
					if (abs(
						((srcp [x+0] * Yb + srcp [x+1] * Yg + srcp [x+2] * Yr + OffyPlusHalf) >> shift) -
						((srcp2[x+0] * Yb + srcp2[x+1] * Yg + srcp2[x+2] * Yr + OffyPlusHalf) >> shift)
						) > th) ++sum;
				}
				srcp  -= ystride;
				srcp2 -= ystride2;
			}
		}
	}
	double result =  (sum * 255.0) / Pixels;
	if(dp != NULL) {
		*dp = result;
	}
    return sum;
}

double __cdecl PVF_LumaCorrelation_RGB(const PVideoFrame &src,const PVideoFrame &src2,
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan,
		const int matrix,const bool IsRGB32) {
	const int xstep    = (IsRGB32) ? 4 : 3;
    const int pitch    = src->GetPitch();
    const int pitch2   = src2->GetPitch();
	const int ystep	   = (altscan) ? 2:1;
	const int ystride  = pitch*ystep;
	const int ystride2 = pitch2*ystep;
	const int height   = src->GetHeight();
	const int height2  = src2->GetHeight();
    const BYTE  *srcp  = src->GetReadPtr()  + ((height-1 - yy)   * pitch)  + (xx * xstep);
    const BYTE  *srcp2 = src2->GetReadPtr() + ((height2-1 - yy2) * pitch2) + (xx2* xstep);
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	const int mat = matrix & 0x03;
	//
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

	unsigned long cntx[256],cnty[256];	
	int i;
	for(i=256;--i>=0;) {
		cntx[i]=0;									// Clear counts
		cnty[i]=0;
	}

	unsigned int Sxy_lo = 0;
	int64_t Sxy=0;
	if(ww==1) {
		for(int y=yhit ; --y>=0;) {
			const unsigned int dx=(srcp [0]	* Yb + srcp [1]	* Yg + srcp [2]	* Yr + OffyPlusHalf) >> shift;
			const unsigned int dy=(srcp2[0] * Yb + srcp2[1]	* Yg + srcp2[2]	* Yr + OffyPlusHalf) >> shift;
			++cntx[dx];
			++cnty[dy];
			Sxy_lo += dx * dy;
			srcp  -= ystride;
			srcp2 -= ystride2;
		}
	} else {
		const int wm = ww * xstep;
		if(IsRGB32) {
			for(int y=yhit ;--y>=0;) {
				for(int x=wm; (x-=4)>=0 ; ) {
					const unsigned int dx=(srcp [x+0]* Yb + srcp [x+1]* Yg + srcp [x+2]* Yr + OffyPlusHalf)>> shift;
					const unsigned int dy=(srcp2[x+0]* Yb + srcp2[x+1]* Yg + srcp2[x+2]* Yr + OffyPlusHalf)>> shift;
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;
				}
				if(Sxy_lo & 0x80000000) {Sxy += Sxy_lo; Sxy_lo = 0;}
				srcp  -= ystride;
				srcp2 -= ystride2;
			}
		} else {
			for(int y=yhit ;--y>=0;) {
				for(int x=wm; (x-=3)>=0 ; ) {
					const unsigned int dx=(srcp [x+0]* Yb + srcp [x+1]* Yg + srcp [x+2]* Yr + OffyPlusHalf)>> shift;
					const unsigned int dy=(srcp2[x+0]* Yb + srcp2[x+1]* Yg + srcp2[x+2]* Yr + OffyPlusHalf)>> shift;
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;
				}
				if(Sxy_lo & 0x80000000) {Sxy += Sxy_lo; Sxy_lo = 0;}
				srcp  -= ystride;
				srcp2 -= ystride2;
			}
		}
	}

	Sxy += Sxy_lo;
	int64_t Sx				= 0;
	int64_t Sy				= 0;
	int64_t Sx2				= 0;
	int64_t Sy2				= 0;

	for (i=256;--i>=0;) {
		int64_t z;
		z	= 	i	* cntx[i];		Sx += 	z;		Sx2+=	i	* z;
		z	= 	i	* cnty[i];		Sy += 	z;		Sy2+=	i	* z;
	}

	double num	=	((double)Pixels * (double)Sxy) - ((double)Sx*Sy);

	double div1	=	((double)Pixels * Sx2) - ((double)Sx*Sx);
	double div2	=	((double)Pixels * Sy2) - ((double)Sy*Sy);

	if(div1 < 0.0)	div1 = - div1;	// fabs
	if(div2 < 0.0)	div2 = - div2;	// fabs

	double div;
	if ((div1<0.0001)||(div2<0.0001)) {
	  div = 0.000001;
	} else {
	  div = sqrt(div1) * sqrt(div2);
	}
	double ret=num / div;
	ret = std::max(std::min(ret,1.0),-1.0);
	return ret;
// http://en.wikipedia.org/wiki/Pearson_product-moment_correlation_coefficient#Mathematical_properties
}


