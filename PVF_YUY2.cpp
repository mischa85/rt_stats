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

double __cdecl PVF_AverageLuma_YUY2(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,const bool altscan) {
	const int ystep	   = (altscan) ? 2:1;
    const int pitch    = src->GetPitch();
	const int ystride  = pitch*ystep;
    const BYTE  *srcp  = src->GetReadPtr()  + (yy * pitch)	 + (xx *2);
	int64_t acc		 = 0;
    unsigned int sum = 0;
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	if(ww == 1) {														// Special case for single pixel width
		for(int y=yhit ; --y>=0;) {
			sum  += srcp[0];
			srcp += ystride;
		}
	} else {
		const int eodd = (ww & 0x07);
		const int wm8 = (ww - eodd) * 2;
		for(int y=yhit; --y>=0 ;) {
			switch(eodd) {
			case 7:	sum += srcp[wm8+(6*2)];
			case 6:	sum += srcp[wm8+(5*2)];
			case 5:	sum += srcp[wm8+(4*2)];
			case 4:	sum += srcp[wm8+(3*2)];
			case 3:	sum += srcp[wm8+(2*2)];
			case 2:	sum += srcp[wm8+(1*2)];
			case 1:	sum += srcp[wm8+(0*2)];
			case 0:	;
			}
			for(int x=wm8; (x-=(8*2))>=0 ; ) {
				sum += (
					srcp[x+(7*2)] +
					srcp[x+(6*2)] +
					srcp[x+(5*2)] +
					srcp[x+(4*2)] +
					srcp[x+(3*2)] +
					srcp[x+(2*2)] +
					srcp[x+(1*2)] +
					srcp[x+(0*2)]
				);
			}
			if(sum & 0x80000000) {acc += sum;sum=0;}					// avoid possiblilty of overflow
			srcp  += ystride;
		}
	}
	acc += sum;
	double result = ((double)acc / Pixels);
    return result;
}


int __cdecl PVF_CountLuma_YUY2(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,
			const bool altscan,unsigned int *cnt) {
	const int ystep	   = (altscan) ? 2:1;
    const int pitch    = src->GetPitch();
	const int ystride  = pitch*ystep;
    const BYTE  *srcp  = src->GetReadPtr()  + (yy * pitch)	 + (xx *2);
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	for(int i=256;--i>=0;cnt[i]=0);
	if(ww == 1) {														// Special case for single pixel width
		for(int y=yhit ; --y>=0 ; ) {
			++cnt[srcp[0]];
			srcp += ystride;
		}
	} else {
		const int eodd = (ww & 0x07);
		const int wm8 = (ww - eodd) * 2;
		for(int y=yhit; --y>=0 ;) {
			switch(eodd) {
			case 7:	++cnt[srcp[wm8+(6*2)]];
			case 6:	++cnt[srcp[wm8+(5*2)]];
			case 5:	++cnt[srcp[wm8+(4*2)]];
			case 4:	++cnt[srcp[wm8+(3*2)]];
			case 3:	++cnt[srcp[wm8+(2*2)]];
			case 2:	++cnt[srcp[wm8+(1*2)]];
			case 1:	++cnt[srcp[wm8+(0*2)]];
			case 0:	;
			}
			for(int x=wm8; (x-=(8*2))>=0 ; ) {
				++cnt[srcp[x+(7*2)]];
				++cnt[srcp[x+(6*2)]];
				++cnt[srcp[x+(5*2)]];
				++cnt[srcp[x+(4*2)]];
				++cnt[srcp[x+(3*2)]];
				++cnt[srcp[x+(2*2)]];
				++cnt[srcp[x+(1*2)]];
				++cnt[srcp[x+(0*2)]];
			}
			srcp  += ystride;
		}
	}
    return Pixels;
}
