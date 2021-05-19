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
#include <algorithm>

double __cdecl PVF_LumaDifference_YUY2(const PVideoFrame &src,const PVideoFrame &src2,
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan) {
    const int pitch    = src->GetPitch();
    const int pitch2   = src2->GetPitch();
	const int ystep	   = (altscan) ? 2:1;
	const int ystride  = pitch*ystep;
	const int ystride2 = pitch2*ystep;
    const BYTE  *srcp  = src->GetReadPtr()  + (yy * pitch)	 + (xx *2);
    const BYTE  *srcp2 = src2->GetReadPtr() + (yy2 * pitch2) + (xx2*2);
	int64_t acc		 = 0;
    unsigned int sum = 0;
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	if(ww == 1) {														// Special case for single pixel width
		for(int y=yhit ; --y>=0; ) {
			sum += abs(srcp[0] - srcp2[0]);
			srcp	+= ystride;
			srcp2	+= ystride2;
		}
	} else {
		const int eodd = (ww & 0x07);
		const int wm8 = (ww - eodd) * 2;
		for(int y = yhit; --y>=0 ;) {
			switch(eodd) {
			case 7:	sum += abs(srcp[wm8+(6*2)] - srcp2[wm8+(6*2)]);
			case 6:	sum += abs(srcp[wm8+(5*2)] - srcp2[wm8+(5*2)]);
			case 5:	sum += abs(srcp[wm8+(4*2)] - srcp2[wm8+(4*2)]);
			case 4:	sum += abs(srcp[wm8+(3*2)] - srcp2[wm8+(3*2)]);
			case 3:	sum += abs(srcp[wm8+(2*2)] - srcp2[wm8+(2*2)]);
			case 2:	sum += abs(srcp[wm8+(1*2)] - srcp2[wm8+(1*2)]);
			case 1:	sum += abs(srcp[wm8+(0*2)] - srcp2[wm8+(0*2)]);
			case 0: ;
			}
			for(int x=wm8; (x-=(8*2))>=0 ; ) {
				sum += (
					abs(srcp[x+(7*2)] - srcp2[x+(7*2)]) +
					abs(srcp[x+(6*2)] - srcp2[x+(6*2)]) +
					abs(srcp[x+(5*2)] - srcp2[x+(5*2)]) +
					abs(srcp[x+(4*2)] - srcp2[x+(4*2)]) +
					abs(srcp[x+(3*2)] - srcp2[x+(3*2)]) +
					abs(srcp[x+(2*2)] - srcp2[x+(2*2)]) +
					abs(srcp[x+(1*2)] - srcp2[x+(1*2)]) +
					abs(srcp[x+(0*2)] - srcp2[x+(0*2)]) 
				);
			}
			if(sum & 0x80000000) {acc += sum; sum=0;}				// avoid overflow
			srcp  += ystride;
			srcp2 += ystride2;
		}
	}
	acc += sum;
    double result = ((double)acc / Pixels);
    return result;
}

double __cdecl PVF_PixelDifference_YUY2(const PVideoFrame &src,const PVideoFrame &src2,
		const double CW,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,const bool altscan
		) {
    const int pitch    = src->GetPitch();
    const int pitch2   = src2->GetPitch();
	const int ystep	   = (altscan) ? 2:1;
	const int ystride  = pitch*ystep;
	const int ystride2 = pitch2*ystep;
	const int height   = src->GetHeight();
	const int height2  = src2->GetHeight();
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	double result = 0.0;			
	int64_t acc = 0,accC=0;
    unsigned int sum=0,sumC=0;
	unsigned int PixelsC = Pixels;
	int x1=xx,x2=xx2,xw=ww,xflgs=0;
	if(x1 & 0x01) {
		xflgs |= 0x01;
		++x1;
		++x2;
		--xw;
	}
	if(xw & 0x01) {
		xflgs |= 0x02;
		--xw;
	}
	if(xw>0) xflgs |= 0x04;
    const BYTE  *srcp  = src->GetReadPtr(PLANAR_Y)   + (yy  * pitch)   + x1*2;
    const BYTE  *srcp2 = src2->GetReadPtr(PLANAR_Y)  + (yy2  * pitch2) + x2*2;

	const int wwoff = xw*2;
	int y,x;
	if((xx & 0x01) == (xx2 & 0x01)) {				
		unsigned int sumC_Half=0;
		for(y=0 ; y < hh; y += ystep) {
			if(xflgs & 0x01) {
				sum			+= abs(srcp[-2] - srcp2[-2]);						// Y
				sumC_Half	+= abs(srcp[-3] - srcp2[-3]);						// U
				sumC_Half	+= abs(srcp[-1] - srcp2[-1]);						// V
			}					
			if(xflgs & 0x04) {
				for(x=wwoff; (x-=4)>=0 ;) {
					sum  += (abs(srcp[x  ] - srcp2[x  ]) + abs(srcp[x+2] - srcp2[x+2])); // Y
					sumC += (abs(srcp[x+1] - srcp2[x+1]) + abs(srcp[x+3] - srcp2[x+3])); // U + V
				}
				if(sum  & 0x80000000) {acc  += sum;  sum =0;}
				if(sumC & 0x80000000) {accC += sumC; sumC=0;}
			}
			if(xflgs & 0x02) {
				sum			+= abs(srcp[wwoff  ] - srcp2[wwoff  ]);				// Y
				sumC_Half	+= abs(srcp[wwoff+1] - srcp2[wwoff+1]);				// U
				sumC_Half	+= abs(srcp[wwoff+3] - srcp2[wwoff+3]);				// V
			}
			srcp	+= ystride;
			srcp2	+= ystride2;
		}
		accC = ((accC += sumC)*2) + sumC_Half;			
	} else {
		for(y=0 ; y < hh; y += ystep) {
			if(xflgs & 0x01) {
				sum	 += abs(srcp[-2] - srcp2[-2]);									// Y
				sumC += (abs(srcp[-3] - srcp2[-1]) + abs(srcp[-1] - srcp2[ 1]));	// U + V
			}	
			if(xflgs & 0x04) {
				int u2=srcp2[wwoff-1],v2=srcp2[wwoff+1];
				for(x=wwoff;(x-=4)>=0;) {
					sum  += (abs(srcp[x  ] - srcp2[x  ]) + abs(srcp[x+2] - srcp2[x+2]));	// Y
					const int u1 = srcp[x+1];
					const int v1 = srcp[x+3];
					sumC += (abs(u1 - u2) + abs(v1 - v2));									// U + V
					sumC += (abs(u1 - (u2=srcp2[x-1])) + abs(v1 - (v2=srcp2[x+1])));		// U + V
				}
				if(sum  & 0x80000000) {acc  += sum;  sum =0;}
				if(sumC & 0x80000000) {accC += sumC; sumC=0;}
			}
			if(xflgs & 0x02) {
				sum	 += abs(srcp[wwoff  ] - srcp2[wwoff  ]);	// Y
				sumC += (abs(srcp[wwoff+1] - srcp2[wwoff-1]) + abs(srcp[wwoff+3] - srcp2[wwoff+1]));	// U + V
			}
			srcp	+= ystride;
			srcp2	+= ystride2;
		}
		accC = (accC += sumC);			
	}
	acc += sum;
	double Yresult = ((double)acc / Pixels);
	double resultC = ((double)accC/ (PixelsC * 2));			// * 2 as U + V
	result = (1.0 - CW) * result + CW * resultC;
	return result;
}

unsigned int __cdecl PVF_LumaPixelsDifferentCount_YUY2(const PVideoFrame &src,const PVideoFrame &src2,
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan,const int thresh,double *dp) {
    const int pitch    = src->GetPitch();
    const int pitch2   = src2->GetPitch();
	const int ystep	   = (altscan) ? 2:1;
	const int ystride  = pitch*ystep;
	const int ystride2 = pitch2*ystep;
    const BYTE  *srcp  = src->GetReadPtr()  + (yy * pitch)	 + (xx * 2);
    const BYTE  *srcp2 = src2->GetReadPtr() + (yy2 * pitch2) + (xx2* 2);
    unsigned int sum = 0;
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	const int th=std::min(std::max(thresh,0),255);
	if(ww == 1) {														// Special case for single pixel width
		for(int y=yhit ; --y>=0;) {                       
			if(abs(srcp[0] - srcp2[0]) > th)	++sum;
			srcp	+= ystride;
			srcp2	+= ystride2;
		}
	} else {
		const int eodd = (ww & 0x07);
		const int wm8 = (ww - eodd) * 2;
		for(int y=yhit ; --y>=0;) {                       
			switch(eodd) {
			case 7:	if(abs(srcp[wm8+(6*2)] - srcp2[wm8+(6*2)]) > th)	++sum;
			case 6:	if(abs(srcp[wm8+(5*2)] - srcp2[wm8+(5*2)]) > th)	++sum;
			case 5:	if(abs(srcp[wm8+(4*2)] - srcp2[wm8+(4*2)]) > th)	++sum;
			case 4:	if(abs(srcp[wm8+(3*2)] - srcp2[wm8+(3*2)]) > th)	++sum;
			case 3:	if(abs(srcp[wm8+(2*2)] - srcp2[wm8+(2*2)]) > th)	++sum;
			case 2:	if(abs(srcp[wm8+(1*2)] - srcp2[wm8+(1*2)]) > th)	++sum;
			case 1:	if(abs(srcp[wm8+(0*2)] - srcp2[wm8+(0*2)]) > th)	++sum;
			case 0: ;
			}
			for(int x=wm8; (x-=(8*2))>=0 ; ) {
				if(abs(srcp[x+(7*2)] - srcp2[x+(7*2)]) > th)	++sum;
				if(abs(srcp[x+(6*2)] - srcp2[x+(6*2)]) > th)	++sum;
				if(abs(srcp[x+(5*2)] - srcp2[x+(5*2)]) > th)	++sum;
				if(abs(srcp[x+(4*2)] - srcp2[x+(4*2)]) > th)	++sum;
				if(abs(srcp[x+(3*2)] - srcp2[x+(3*2)]) > th)	++sum;
				if(abs(srcp[x+(2*2)] - srcp2[x+(2*2)]) > th)	++sum;
				if(abs(srcp[x+(1*2)] - srcp2[x+(1*2)]) > th)	++sum;
				if(abs(srcp[x+(0*2)] - srcp2[x+(0*2)]) > th)	++sum;
			}
			srcp  += ystride;
			srcp2 += ystride2;
		}
	}
	double result = (sum * 255.0) / Pixels;
	if(dp != NULL) {
		*dp = result;
	}
    return sum;
}

double __cdecl PVF_LumaCorrelation_YUY2(const PVideoFrame &src,const PVideoFrame &src2,
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan) {
    const int pitch    = src->GetPitch();
    const int pitch2   = src2->GetPitch();
	const int ystep	   = (altscan) ? 2:1;
	const int ystride  = pitch*ystep;
	const int ystride2 = pitch2*ystep;
    const BYTE  *srcp  = src->GetReadPtr()  + (yy  * pitch)	 + (xx*2);
    const BYTE  *srcp2 = src2->GetReadPtr() + (yy2 * pitch2) + (xx2*2);
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);

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
			const unsigned int dx=srcp[0];
			const unsigned int dy=srcp2[0];
			++cntx[dx];
			++cnty[dy];
			Sxy_lo += dx * dy;
			srcp	+= ystride;
			srcp2	+= ystride2;
		}
	} else {
		const int eodd = (ww & 0x03);
		const int wm4 = (ww - eodd) * 2;
		for(int y=yhit; --y>=0;) {
			switch(eodd) {
			case 3:	{
					const int dx=srcp [wm4+(2*2)];	
					const int dy=srcp2[wm4+(2*2)];
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;
					}
			case 2:	{
					const int dx=srcp [wm4+(1*2)];	
					const int dy=srcp2[wm4+(1*2)];
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;
					}
			case 1:	{
					const int dx=srcp [wm4+(0*2)];
					const int dy=srcp2[wm4+(0*2)];
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;
					}
			case 0: ;
			}
			for(int x=wm4; (x-=(4*2))>=0 ; ) {
				{
					const int dx=srcp [x+(3*2)];
					const int dy=srcp2[x+(3*2)];
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;
				}
				{
					const int dx=srcp [x+(2*2)];
					const int dy=srcp2[x+(2*2)];
					++cntx[dx];	
					++cnty[dy];
					Sxy_lo += dx * dy;
				}
				{
					const int dx=srcp [x+(1*2)];
					const int dy=srcp2[x+(1*2)];
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;
				}
				{
					const int dx=srcp [x+(0*2)];
					const int dy=srcp2[x+(0*2)];
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;
				}
			}
			if(Sxy_lo & 0x80000000) {Sxy += Sxy_lo; Sxy_lo = 0;}
			srcp  += ystride;
			srcp2 += ystride2;
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
