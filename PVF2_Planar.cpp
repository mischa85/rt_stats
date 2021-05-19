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
#include "algorithm"

double __cdecl PVF_LumaDifference_Planar(const PVideoFrame &src,const PVideoFrame &src2,
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan) {
    const int pitch    = src->GetPitch(PLANAR_Y);
    const int pitch2   = src2->GetPitch(PLANAR_Y);
	const int ystep	   = (altscan) ? 2:1;
	const int ystride  = pitch*ystep;
	const int ystride2 = pitch2*ystep;
    const BYTE  *srcp  = src->GetReadPtr(PLANAR_Y)  + (yy * pitch)	 + xx;
    const BYTE  *srcp2 = src2->GetReadPtr(PLANAR_Y) + (yy2 * pitch2) + xx2;
	int64_t acc		 = 0;
    unsigned int sum = 0;
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	if(ww == 1) {														// Special case for single pixel width
		for(int y = yhit; --y>=0 ;) {
			sum		+= abs(srcp[0] - srcp2[0]);
			srcp	+= ystride;
			srcp2	+= ystride2;
		}
	} else {
		const int eodd = (ww & 0x07);
		const int wm8 = ww - eodd;
		for(int y = yhit; --y>=0 ;) {
			switch(eodd) {
			case 7:	sum += abs(srcp[wm8+6] - srcp2[wm8+6]);
			case 6:	sum += abs(srcp[wm8+5] - srcp2[wm8+5]);
			case 5:	sum += abs(srcp[wm8+4] - srcp2[wm8+4]);
			case 4:	sum += abs(srcp[wm8+3] - srcp2[wm8+3]);
			case 3:	sum += abs(srcp[wm8+2] - srcp2[wm8+2]);
			case 2:	sum += abs(srcp[wm8+1] - srcp2[wm8+1]);
			case 1:	sum += abs(srcp[wm8+0] - srcp2[wm8+0]);
			case 0: ;
			}
			for(int x=wm8; (x-=8)>=0 ; ) {
				sum += (
					abs(srcp[x+7] - srcp2[x+7]) +
					abs(srcp[x+6] - srcp2[x+6]) +
					abs(srcp[x+5] - srcp2[x+5]) +
					abs(srcp[x+4] - srcp2[x+4]) +
					abs(srcp[x+3] - srcp2[x+3]) +
					abs(srcp[x+2] - srcp2[x+2]) +
					abs(srcp[x+1] - srcp2[x+1]) +
					abs(srcp[x+0] - srcp2[x+0])
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

double __cdecl PVF_PixelDifference_Planar(const PVideoFrame &src,const PVideoFrame &src2,
		const double CW,const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,const bool altscan,bool ChromaI
		) {
	double result = (CW<1.0) ? PVF_LumaDifference_Planar(src,src2,xx,yy,ww,hh,xx2,yy2,altscan) : 0.0;
	const int ystep  = (altscan) ? 2:1;
	const int yhit   = (altscan) ? (hh +1)>>1 : hh;
    unsigned int PixelsC = (ww * yhit);
	int xSubS=1,ySubS=1;
	int64_t acc = 0;
    unsigned int sum=0;

	const BYTE  *srcp	= src->GetReadPtr(PLANAR_U);
	const BYTE  *srcp2	= src2->GetReadPtr(PLANAR_U);
	const BYTE *srcpV	= src->GetReadPtr(PLANAR_V);
	const BYTE *srcpV2	= src2->GetReadPtr(PLANAR_V);
	const int pitchUV   = src->GetPitch(PLANAR_U);
	const int pitchUV2  = src2->GetPitch(PLANAR_U);
	const int RowSizeUV = src->GetRowSize(PLANAR_U);
	if(RowSizeUV) {
		xSubS = src->GetRowSize(PLANAR_Y)/RowSizeUV;
		ySubS = src->GetHeight(PLANAR_Y)/src->GetHeight(PLANAR_U);
	}
	const int xshift	= (xSubS==4)?2:(xSubS==2)?1:0; 
	const int yshift	= (ySubS==2)?1:0; 

	ChromaI=(ChromaI && ySubS==2 && xSubS==2);									// Switch off ChromaI if not YV12
	int x,y;		
	int cstep = 
		(hh==1)																	? 1     :				 
		(ySubS==1)																? ystep : 
		(ChromaI&&ystep==2&&(((yy|yy2)&0x02)==0)&&((((hh+1)>>1)&0x01)==0))		? 2     :
		(!ChromaI && ystep == 2)												? 1     :				 
		(ystep == 1 && ((hh|yy|yy2)&((ChromaI?0x03:1)==0)))						? 1     :
		0;

	if(cstep!=0) { // Vertical visiting Chroma Samples ONLY ONCE each
		const int yt1 = (ChromaI) ? ((yy >>2)<<1) | (yy & 0x01)  : yy  >> yshift;
		srcp   += yt1 * pitchUV;
		srcpV  += yt1 * pitchUV;
		const int yt2 = (ChromaI) ? ((yy2>>2)<<1) | (yy2 & 0x01) : yy2 >> yshift;				
		srcp2  += yt2 * pitchUV2;
		srcpV2 += yt2 * pitchUV2;
		const int strideUV	=	cstep * pitchUV;
		const int strideUV2	=	cstep * pitchUV2;
		const int hhuv = (ChromaI && ystep==2) ? (hh+3)>>2 : (ystep==2) ? (hh+1)>>1 : hh;
		if(ww==1) {
			const int xxt = xx >> xshift;
			srcp   += xxt;
			srcpV  += xxt;
			const int xxt2 = xx2 >> xshift;
			srcp2  += xxt2;
			srcpV2 += xxt2;
			PixelsC = hhuv;
			for(y=hhuv; --y>=0 ;) {
				sum += (abs(srcp [0] - srcp2 [0]) + abs(srcpV[0] - srcpV2[0]));
				srcp   += strideUV;
				srcpV  += strideUV;
				srcp2  += strideUV2;
				srcpV2 += strideUV2;
			}
		} else if(((ww|xx|xx2)&(xSubS-1))==0) { // Chroma sample both vertical and horizontal ONLY ONCE
			const int xxt = xx >> xshift;
			srcp   += xxt;
			srcpV  += xxt;
			const int xxt2 = xx2 >> xshift;
			srcp2  += xxt2;
			srcpV2 += xxt2;
			const int wwuv = ww >> xshift;					// Horizontal Sample count
			PixelsC = hhuv * wwuv;

			const int eodd = (wwuv & 0x07);
			const int wwmod8 = wwuv - eodd;

			for(y=hhuv; --y>=0 ;) {
				switch(eodd) {
				case 7:	sum += (abs(srcp[wwmod8+6] - srcp2[wwmod8+6]) + abs(srcpV[wwmod8+6] - srcpV2[wwmod8+6]));
				case 6:	sum += (abs(srcp[wwmod8+5] - srcp2[wwmod8+5]) + abs(srcpV[wwmod8+5] - srcpV2[wwmod8+5]));
				case 5:	sum += (abs(srcp[wwmod8+4] - srcp2[wwmod8+4]) + abs(srcpV[wwmod8+4] - srcpV2[wwmod8+4]));
				case 4:	sum += (abs(srcp[wwmod8+3] - srcp2[wwmod8+3]) + abs(srcpV[wwmod8+3] - srcpV2[wwmod8+3]));
				case 3:	sum += (abs(srcp[wwmod8+2] - srcp2[wwmod8+2]) + abs(srcpV[wwmod8+2] - srcpV2[wwmod8+2]));
				case 2:	sum += (abs(srcp[wwmod8+1] - srcp2[wwmod8+1]) + abs(srcpV[wwmod8+1] - srcpV2[wwmod8+1]));
				case 1:	sum += (abs(srcp[wwmod8+0] - srcp2[wwmod8+0]) + abs(srcpV[wwmod8+0] - srcpV2[wwmod8+0]));
				case 0:	;
				}
				for(x=wwmod8; (x-=8)>=0 ; ) {
					sum += (
						abs(srcp [x+7] - srcp2 [x+7]) +	abs(srcpV[x+7] - srcpV2[x+7]) +
						abs(srcp [x+6] - srcp2 [x+6]) +	abs(srcpV[x+6] - srcpV2[x+6]) +
						abs(srcp [x+5] - srcp2 [x+5]) +	abs(srcpV[x+5] - srcpV2[x+5]) +
						abs(srcp [x+4] - srcp2 [x+4]) +	abs(srcpV[x+4] - srcpV2[x+4]) +
						abs(srcp [x+3] - srcp2 [x+3]) +	abs(srcpV[x+3] - srcpV2[x+3]) +
						abs(srcp [x+2] - srcp2 [x+2]) +	abs(srcpV[x+2] - srcpV2[x+2]) +
						abs(srcp [x+1] - srcp2 [x+1]) +	abs(srcpV[x+1] - srcpV2[x+1]) +
						abs(srcp [x+0] - srcp2 [x+0]) +	abs(srcpV[x+0] - srcpV2[x+0])
					);
				}
				if(sum & 0x80000000) {acc += sum; sum=0;}						// avoid possiblilty of overflow
				srcp   += strideUV;
				srcpV  += strideUV;
				srcp2  += strideUV2;
				srcpV2 += strideUV2;
			}
		} else {
			PixelsC = hhuv * ww;
			for(y=0 ; y<hhuv ;++y) {
				for(x=0 ; x<ww ;++x) {
					const int xt1 = (xx  + x) >> xshift;
					const int xt2 = (xx2 + x) >> xshift;
					sum += (abs(srcp[xt1] - srcp2[xt2]) + abs(srcpV[xt1]- srcpV2[xt2]));
				}
				if(sum &  0x80000000)     {acc      += sum;      sum =0;}
				srcp   += strideUV;
				srcpV  += strideUV;
				srcp2  += strideUV2;
				srcpV2 += strideUV2;
			}
		}
	} else if(((ww|xx|xx2)&(xSubS-1))==0) {		// Multi-visits vertical, Single visit horizontal
		PixelsC >>= xshift;
		const int xxt = xx >> xshift;
		srcp   += xxt;
		srcpV  += xxt;
		const int xxt2 = xx2 >> xshift;
		srcp2  += xxt2;
		srcpV2 += xxt2;
		const int wwuv = ww >> xshift;					// Horizontal Sample count

		const int eodd = (wwuv & 0x07);
		const int wwmod8 = wwuv - eodd;

		for(y=0 ; y < hh; y += ystep) {
			const int y1 = yy +y;
			const int yt1 = (ChromaI) ? ((y1>>2)<<1) | (y1 & 0x01) : y1 >> yshift;
			const int offy = yt1 * pitchUV;
			const BYTE *rpu  = srcp   + offy;
			const BYTE *rpv  = srcpV  + offy;
			const int y2 = yy2+y;
			const int yt2 = (ChromaI) ? ((y2>>2)<<1) | (y2 & 0x01) : y2 >> yshift;
			const int offy2 = yt2 * pitchUV2;
			const BYTE *rpu2 = srcp2  + offy2;
			const BYTE *rpv2 = srcpV2 + offy2;

			switch(eodd) {
			case 7:	sum += (abs(rpu[wwmod8+6] - rpu2[wwmod8+6]) + abs(rpv[wwmod8+6] - rpv2[wwmod8+6]));
			case 6:	sum += (abs(rpu[wwmod8+5] - rpu2[wwmod8+5]) + abs(rpv[wwmod8+5] - rpv2[wwmod8+5]));
			case 5:	sum += (abs(rpu[wwmod8+4] - rpu2[wwmod8+4]) + abs(rpv[wwmod8+4] - rpv2[wwmod8+4]));
			case 4:	sum += (abs(rpu[wwmod8+3] - rpu2[wwmod8+3]) + abs(rpv[wwmod8+3] - rpv2[wwmod8+3]));
			case 3:	sum += (abs(rpu[wwmod8+2] - rpu2[wwmod8+2]) + abs(rpv[wwmod8+2] - rpv2[wwmod8+2]));
			case 2:	sum += (abs(rpu[wwmod8+1] - rpu2[wwmod8+1]) + abs(rpv[wwmod8+1] - rpv2[wwmod8+1]));
			case 1:	sum += (abs(rpu[wwmod8+0] - rpu2[wwmod8+0]) + abs(rpv[wwmod8+0] - rpv2[wwmod8+0]));
			case 0:	;
			}
			for(x=wwmod8; (x-=8)>=0 ; ) {
				sum += (
					abs(rpu[x+7] - rpu2[x+7]) +	abs(rpv[x+7] - rpv2[x+7]) +
					abs(rpu[x+6] - rpu2[x+6]) +	abs(rpv[x+6] - rpv2[x+6]) +
					abs(rpu[x+5] - rpu2[x+5]) +	abs(rpv[x+5] - rpv2[x+5]) +
					abs(rpu[x+4] - rpu2[x+4]) +	abs(rpv[x+4] - rpv2[x+4]) +
					abs(rpu[x+3] - rpu2[x+3]) +	abs(rpv[x+3] - rpv2[x+3]) +
					abs(rpu[x+2] - rpu2[x+2]) +	abs(rpv[x+2] - rpv2[x+2]) +
					abs(rpu[x+1] - rpu2[x+1]) +	abs(rpv[x+1] - rpv2[x+1]) +
					abs(rpu[x+0] - rpu2[x+0]) +	abs(rpv[x+0] - rpv2[x+0])
				);
			}
			if(sum & 0x80000000) {acc += sum; sum=0;}						// avoid possiblilty of overflow
		}
	} else {	// vertical visiting chroma samples multiple times
		// Multi-visits both x and y
		for(y=0 ; y < hh; y += ystep) {
			const int y1 = yy +y;
			const int yt1 = (ChromaI) ? ((y1>>2)<<1) | (y1 & 0x01) : y1 >> yshift;
			const int offy = yt1 * pitchUV;
			const BYTE *rpu  = srcp   + offy;
			const BYTE *rpv  = srcpV  + offy;
			const int y2 = yy2+y;
			const int yt2 = (ChromaI) ? ((y2>>2)<<1) | (y2 & 0x01) : y2 >> yshift;
			const int offy2 = yt2 * pitchUV2;
			const BYTE *rpu2 = srcp2  + offy2;
			const BYTE *rpv2 = srcpV2 + offy2;
			for(x=0 ; x < ww ; ++x) {
				const int xt1 = (xx  + x) >> xshift;
				const int xt2 = (xx2 + x) >> xshift;
				sum += (abs(rpu[xt1] - rpu2[xt2]) + abs(rpv[xt1] - rpv2[xt2]));
			}
			if(sum & 0x80000000) {acc += sum; sum =0;}
		}
	}
	acc += sum;
	double resultC = ((double)acc / (PixelsC*2));	// PixelsC * 2, ie U+V
	double resultY=result;
	result = (1.0 - CW) * result + CW * resultC;
	return result;
}

unsigned int __cdecl PVF_LumaPixelsDifferentCount_Planar(const PVideoFrame &src,const PVideoFrame &src2,
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan,const int thresh,double *dp) {
    const int pitch    = src->GetPitch(PLANAR_Y);
    const int pitch2   = src2->GetPitch(PLANAR_Y);
	const int ystep	   = (altscan) ? 2:1;
	const int ystride  = pitch*ystep;
	const int ystride2 = pitch2*ystep;
    const BYTE  *srcp  = src->GetReadPtr(PLANAR_Y)  + (yy * pitch)	 + xx;
    const BYTE  *srcp2 = src2->GetReadPtr(PLANAR_Y) + (yy2 * pitch2) + xx2;
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
		const int wm8 = ww - eodd;
		for(int y=yhit ; --y>=0;) {                       
			switch(eodd) {
			case 7:	if(abs(srcp[wm8+6] - srcp2[wm8+6]) > th)	++sum;
			case 6:	if(abs(srcp[wm8+5] - srcp2[wm8+5]) > th)	++sum;
			case 5:	if(abs(srcp[wm8+4] - srcp2[wm8+4]) > th)	++sum;
			case 4:	if(abs(srcp[wm8+3] - srcp2[wm8+3]) > th)	++sum;
			case 3:	if(abs(srcp[wm8+2] - srcp2[wm8+2]) > th)	++sum;
			case 2:	if(abs(srcp[wm8+1] - srcp2[wm8+1]) > th)	++sum;
			case 1:	if(abs(srcp[wm8+0] - srcp2[wm8+0]) > th)	++sum;
			case 0: ;
			}
			for(int x=wm8; (x-=8)>=0 ; ) {
				if(abs(srcp[x+7] - srcp2[x+7]) > th)	++sum;
				if(abs(srcp[x+6] - srcp2[x+6]) > th)	++sum;
				if(abs(srcp[x+5] - srcp2[x+5]) > th)	++sum;
				if(abs(srcp[x+4] - srcp2[x+4]) > th)	++sum;
				if(abs(srcp[x+3] - srcp2[x+3]) > th)	++sum;
				if(abs(srcp[x+2] - srcp2[x+2]) > th)	++sum;
				if(abs(srcp[x+1] - srcp2[x+1]) > th)	++sum;
				if(abs(srcp[x+0] - srcp2[x+0]) > th)	++sum;
			}
			srcp  += ystride;
			srcp2 += ystride2;
		}
	}
	double result =  (sum * 255.0) / Pixels;
	if(dp != NULL) {
		*dp = result;
	}
    return sum;
}

double __cdecl PVF_LumaCorrelation_Planar(const PVideoFrame &src,const PVideoFrame &src2,
		const int xx,const int yy,const int ww,const int hh,const int xx2,const int yy2,bool altscan) {
    const int pitch    = src->GetPitch(PLANAR_Y);
    const int pitch2   = src2->GetPitch(PLANAR_Y);
	const int ystep	   = (altscan) ? 2:1;
	const int ystride  = pitch*ystep;
	const int ystride2 = pitch2*ystep;
    const BYTE  *srcp  = src->GetReadPtr(PLANAR_Y)  + (yy * pitch)	 + xx;
    const BYTE  *srcp2 = src2->GetReadPtr(PLANAR_Y) + (yy2 * pitch2) + xx2;
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
			Sxy_lo  += dx * dy;
			srcp	+= ystride;
			srcp2	+= ystride2;
		}
	} else {
		const int eodd = (ww & 0x03);
		const int wm4 = ww - eodd;
		for(int y=yhit; --y>=0 ;) {
			switch(eodd) {
			case 3:	{
					const int dx=srcp [wm4+2];	
					const int dy=srcp2[wm4+2];
					++cntx[dx];		
					++cnty[dy];		
					Sxy_lo += dx * dy;}
			case 2:	{
					const int dx=srcp [wm4+1];	
					const int dy=srcp2[wm4+1];
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;}
			case 1:	{
					const int dx=srcp [wm4+0];
					const int dy=srcp2[wm4+0];
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;}
			case 0: ;
			}
			for(int x=wm4; (x-=4)>=0 ; ) {
				{
					const int dx=srcp [x+3];	
					const int dy=srcp2[x+3];
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;
				}
				{
					const int dx=srcp [x+2];
					const int dy=srcp2[x+2];
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;
				}
				{
					const int dx=srcp [x+1];
					const int dy=srcp2[x+1];
					++cntx[dx];
					++cnty[dy];
					Sxy_lo += dx * dy;
				}
				{
					const int dx=srcp [x+0];
					const int dy=srcp2[x+0];
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

