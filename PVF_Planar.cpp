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


double __cdecl PVF_AverageLuma_Planar(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,const bool altscan) {
	const int ystep	   = (altscan) ? 2:1;
    const int pitch    = src->GetPitch(PLANAR_Y);
	const int ystride  = pitch*ystep;
    const BYTE  *srcp  = src->GetReadPtr(PLANAR_Y)  + (yy * pitch) + xx;
	int64_t acc		   = 0;
    unsigned int sum   = 0;
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);

	if(ww == 1) {														// Special case for single pixel width
		for(int y=yhit ; --y>=0;) {
			sum += srcp[0];
			srcp+= ystride;
		}
	} else {
		const int eodd = (ww & 0x0F);
		const int wm16 = ww - eodd;
		for(int y=yhit; --y>=0 ;) {
			switch(eodd) {
			case 15:	sum += srcp[wm16+14];
			case 14:	sum += srcp[wm16+13];
			case 13:	sum += srcp[wm16+12];
			case 12:	sum += srcp[wm16+11];
			case 11:	sum += srcp[wm16+10];
			case 10:	sum += srcp[wm16+9];
			case 9:		sum += srcp[wm16+8];
			case 8:		sum += srcp[wm16+7];
			case 7:		sum += srcp[wm16+6];
			case 6:		sum += srcp[wm16+5];
			case 5:		sum += srcp[wm16+4];
			case 4:		sum += srcp[wm16+3];
			case 3:		sum += srcp[wm16+2];
			case 2:		sum += srcp[wm16+1];
			case 1:		sum += srcp[wm16+0];
			case 0:	;
			}
			for(int x=wm16; (x-=16)>=0 ; ) {
				sum +=	(
						srcp[x+15] +
						srcp[x+14] +
						srcp[x+13] +
						srcp[x+12] +
						srcp[x+11] +
						srcp[x+10] +
						srcp[x+ 9] +
						srcp[x+ 8] +
						srcp[x+ 7] +
						srcp[x+ 6] +
						srcp[x+ 5] +
						srcp[x+ 4] +
						srcp[x+ 3] +
						srcp[x+ 2] +
						srcp[x+ 1] +
						srcp[x+ 0]
				);
			}
			if(sum & 0x80000000) {acc += sum;sum=0;}					// avoid possiblilty of overflow
			srcp  += ystride;
		}
	}

	acc += sum;
	double dacc = double(acc);
	return dacc / Pixels;
}

int __cdecl PVF_CountLuma_Planar(const PVideoFrame &src,const int xx,const int yy,const int ww,const int hh,
			const bool altscan,unsigned int *cnt) {
	const int ystep	   = (altscan) ? 2:1;
    const int pitch    = src->GetPitch(PLANAR_Y);
	const int ystride  = pitch*ystep;
    const BYTE  *srcp  = src->GetReadPtr(PLANAR_Y)  + (yy * pitch) + xx;
	const int yhit = (altscan) ? (hh +1)>>1 : hh;
    const unsigned int Pixels = (ww * yhit);
	for(int i=256;--i>=0;cnt[i]=0);
	if(ww == 1) {														// Special case for single pixel width
		for(int y=yhit ; --y>=0; ) {
			++cnt[srcp[0]];
			srcp += ystride;
		}
	} else {
		const int eodd = (ww & 0x0F);
		const int wm16 = ww - eodd;
		for(int y=yhit; --y>=0 ;) {
			switch(eodd) {
			case 15:	++cnt[srcp[wm16+14]];
			case 14:	++cnt[srcp[wm16+13]];
			case 13:	++cnt[srcp[wm16+12]];
			case 12:	++cnt[srcp[wm16+11]];
			case 11:	++cnt[srcp[wm16+10]];
			case 10:	++cnt[srcp[wm16+9]];
			case 9:		++cnt[srcp[wm16+8]];
			case 8:		++cnt[srcp[wm16+7]];
			case 7:		++cnt[srcp[wm16+6]];
			case 6:		++cnt[srcp[wm16+5]];
			case 5:		++cnt[srcp[wm16+4]];
			case 4:		++cnt[srcp[wm16+3]];
			case 3:		++cnt[srcp[wm16+2]];
			case 2:		++cnt[srcp[wm16+1]];
			case 1:		++cnt[srcp[wm16+0]];
			case 0:	;
			}
			for(int x=wm16; (x-=16)>=0 ; ) {
				++cnt[srcp[x+15]];
				++cnt[srcp[x+14]];
				++cnt[srcp[x+13]];
				++cnt[srcp[x+12]];
				++cnt[srcp[x+11]];
				++cnt[srcp[x+10]];
				++cnt[srcp[x+9]];
				++cnt[srcp[x+8]];
				++cnt[srcp[x+7]];
				++cnt[srcp[x+6]];
				++cnt[srcp[x+5]];
				++cnt[srcp[x+4]];
				++cnt[srcp[x+3]];
				++cnt[srcp[x+2]];
				++cnt[srcp[x+1]];
				++cnt[srcp[x+0]];
			}
			srcp  += ystride;
		}
	}
    return Pixels;
}







