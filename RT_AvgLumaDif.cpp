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

AVSValue __cdecl RT_AvgLumaDif(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *myName="RT_AvgLumaDif: ";

	PClip child			= args[0].AsClip();

	int n;
	
	if(args[1].IsInt()) {n  = args[2].AsInt(); }			// Frame n
	else {
		AVSValue cn       =	GetVar(env,"current_frame");
		if (!cn.IsInt())	env->ThrowError("%s'current_frame' only available in runtime scripts",myName);
		n                 =	cn.AsInt();										// current_frame
	}
	
	const bool slices	= args[2].AsBool(false);				// slices, default false

    const VideoInfo &vi = child->GetVideoInfo();
	// width=0 means no video, most plugins dont bother with below check (I dont either usually, but probably should)
	if(vi.num_frames <= 0 || vi.width==0) env->ThrowError("%sClip must have video",myName);

	n = (n < 0) ? 0 : (n >= vi.num_frames) ? vi.num_frames - 1 : n;		// range limit n to valid frames

	const int matrix	= args[3].AsInt(vi.width>1100 || vi.height>600?3:2);
	// Matrix: REC601 : 1=REC709 : 2 = PC601 : 3 = PC709

	const int step = (slices) ? 4 : 1;

    // Ignore RHS odds and ends, FULL 4x4 pixel blocks only
	const int W = vi.width  & (~0x03);							// clear bits 0 and 1, ie make mod 4
	const int H = vi.height & (~0x03);
	if(W<=0 || H <=0)	env->ThrowError("%sIllegal Frame Size (at least 4x4)",myName);
	
	PVideoFrame src		= child->GetFrame(n,env);				// pointer to video frame
    const int   pitch   = src->GetPitch(PLANAR_Y);
    const BYTE  *srcp   = src->GetReadPtr(PLANAR_Y);
	const int	ystride	= pitch*step;							// how much to step data pointer for each y iteration

	int	dif = 0;												// init, dif sum as int (not double)
	const int x_end=W-3, y_end=H-3;								// end limits
	int x,y;
	if(vi.IsPlanar()) {
		srcp  += pitch;												// y=1
		if(slices) {
			for(y=1; y <= y_end; y += 4) {
				for(x=1; x <= x_end; x += 4) {						// x step appropriate for slices
					const int tl=srcp[x];                           // x  , y
					const int tr=srcp[x+1];                         // x+1, y
					const int bl=srcp[x+pitch];                     // x  , y+1
					const int br=srcp[x+1+pitch];                   // x+1, y+1
					dif += abs(tl-tr);								// x,y   <-> x+1,y
					dif += abs(tl-bl);                              // x,y   <-> x,y+1
					dif += abs(tr-br);                              // x+1,y <-> x+1,y+1
					dif += abs(bl-br);                              // x,y+1 <-> x+1,y+1
				}
				srcp += ystride;                                    // y step appropriate for slices
			}
		} else {
			for(y=1; y <= y_end; ++y) {
				for(x=1; x <= x_end; ++x) {							// x step appropriate for slices
					const int tl=srcp[x];							// x  , y
					const int tr=srcp[x+1];							// x+1, y
					const int bl=srcp[x+pitch];						// x  , y+1
					dif += abs(tl-tr);								// x,y   <-> x+1,y
					dif += abs(tl-bl);								// x,y	 <-> x,y+1
				}
				// rhs column, x = W - 2
				const int tl=srcp[x];								// x = W - 2, y
				const int bl=srcp[x+pitch];							// x = W - 2, y+1
				dif += abs(tl-bl);
				srcp += ystride;									// y step appropriate for slices
			}
			// y = H - 2
			for(x=1; x <= x_end; ++x) {								// bottom row
				const int tl=srcp[x];						        // x  , y = H - 2
				const int tr=srcp[x+1];								// x+1, y = H - 2
				dif += abs(tl-tr);
			}
		}
	} else if(vi.IsYUY2()) {
		srcp  += pitch;												// y=1
		const int x_lim = x_end * 2;								// luma samples 2 apart for YUY2
		if(slices) {
			for(y=1; y <= y_end;  y += 4) {
				for(x=2; x <= x_lim; x += (4*2)) {					// x=2 is offset to YUY2(1).Y
					const int tl=srcp[x];							// x  , y
					const int tr=srcp[x+2];							// x+1, y
					const int bl=srcp[x+pitch];						// x  , y+1
					const int br=srcp[x+2+pitch];					// x+1, y+1
					dif += abs(tl-tr);                              // x,y   <-> x+1,y
					dif += abs(tl-bl);                              // x,y   <-> x,y+1
					dif += abs(tr-br);                              // x+1,y <-> x+1,y+1
					dif += abs(bl-br);                              // x,y+1 <-> x+1,y+1
				}
				srcp += ystride;                                    // y step appropriate for slices
			}
		} else {
			for(y=1; y <= y_end;  ++y) {
				for(x=2; x <= x_lim; x += 2 ) {						// x=2 is offset to YUY2(1).Y
					const int tl=srcp[x];							// x  , y
					const int tr=srcp[x+2];							// x+1, y
					const int bl=srcp[x+pitch];						// x  , y+1
					dif += abs(tl-tr);								// x,y   <-> x+1,y
					dif += abs(tl-bl);								// x,y	 <-> x,y+1
				}
				// rhs column, x = W - 2
				const int tl=srcp[x];								// x = W - 2, y
				const int bl=srcp[x+pitch];							// x = W - 2, y+1
				dif += abs(tl-bl);
				srcp += ystride;									// y step appropriate for slices
			}
			// y = H - 2
			for(x=2; x <= x_lim; x+=2) {							// bottom row
				const int tl=srcp[x];						        // x  , y = H - 2
				const int tr=srcp[x+2];								// x+1, y = H - 2
				dif += abs(tl-tr);
			}
		}
	} else if(vi.IsRGB()) {
        // RGB to YUV-Y Conversion
		// Matrix: REC601 : 1=REC709 : 2 = PC601 : 3 = PC709
		double				Kr,Kb;
		int					Sy,offset_y;
		if(matrix & 0x01)	{Kr = 0.2126; Kb        = 0.0722;}			// 709  1 or 3
		else                {Kr = 0.2990; Kb        = 0.1140;}			// 601  0 or 2
		if(matrix & 0x02)	{Sy = 255   ; offset_y  = 0;}				// PC   2 or 3
		else                {Sy = 219   ; offset_y  = 16;}				// TV   0 or 1
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
		// RGB is weird upside down frame
		srcp += ((vi.height-1) * pitch);	// Upside down RGB, height-1 is top line (y=0)
		srcp -= pitch;						// y=1, (could of course be combined with above, NOTE subtract)
        const int xstep   = (vi.IsRGB24()) ? 3 : 4;
		const int xstride = step  * xstep;							// scale step for  RGB24/32
		const int x_lim   = x_end * xstep;							// scale x_end for RGB24/32
		if(slices) {
			for(y=1; y <= y_end;  y += step) {
				for(x=xstep; x <= x_lim;x += xstride) {                 // x stride appropriate for slices
					int tl = (srcp[x] * Yb + srcp[x+1] * Yg + srcp[x+2] * Yr + OffyPlusHalf) >> shift;       // x  ,y
					int x2= x + xstep;
					int tr = (srcp[x2] * Yb + srcp[x2+1] * Yg + srcp[x2+2] * Yr + OffyPlusHalf) >> shift;    // x+1, y
					x2= x - pitch;          // NOTE - pitch
					int bl = (srcp[x2] * Yb + srcp[x2+1] * Yg + srcp[x2+2] * Yr + OffyPlusHalf) >> shift;    // x  ,y+1
					x2= x + xstep - pitch;  // NOTE - pitch
					int br = (srcp[x2] * Yb + srcp[x2+1] * Yg + srcp[x2+2] * Yr + OffyPlusHalf) >> shift;    // x+1,y+1
					dif += abs(tl - tr);    // x,y   <-> x+1,y
					dif += abs(tl - bl);    // x,y   <-> x,y+1
					dif += abs(tr - br);    // x+1,y <-> x+1,y+1
					dif += abs(bl - br);    // x,y+1 <-> x+1,y+1
				}
				srcp -= ystride;
			}
		} else {
			for(y=1; y <= y_end;  y += step) {
				for(x=xstep; x <= x_lim;x += xstride) {					// x stride appropriate for slices
					int tl = (srcp[x] * Yb + srcp[x+1] * Yg + srcp[x+2] * Yr + OffyPlusHalf) >> shift;		// x  ,y
					int x2= x + xstep;
					int tr = (srcp[x2] * Yb + srcp[x2+1] * Yg + srcp[x2+2] * Yr + OffyPlusHalf) >> shift;	// x+1, y
					x2= x - pitch;			// NOTE - pitch
					int bl = (srcp[x2] * Yb + srcp[x2+1] * Yg + srcp[x2+2] * Yr + OffyPlusHalf) >> shift;	// x  ,y+1
					dif += abs(tl - tr);		// x,y   <-> x+1,y
					dif += abs(tl - bl);		// x,y	 <-> x,y+1
				}
				// rhs column, x = W - 2
				const int tl = (srcp[x] * Yb + srcp[x+1] * Yg + srcp[x+2] * Yr + OffyPlusHalf) >> shift;		// x = W - 2, y
				int x2= x - pitch;			// NOTE - pitch
				const int bl = (srcp[x2] * Yb + srcp[x2+1] * Yg + srcp[x2+2] * Yr + OffyPlusHalf) >> shift;	// x = W - 2, y+1
				dif += abs(tl-bl);
				srcp -= ystride;									// y step appropriate for slices, NOTE subtract
			}
			// y = H - 2
			for(x=xstep; x <= x_lim; x+=xstride) {							// bottom row
				const int tl = (srcp[x] * Yb + srcp[x+1] * Yg + srcp[x+2] * Yr + OffyPlusHalf) >> shift;		// x  , y = H - 2
				int x2= x + xstep;
				const int tr = (srcp[x2] * Yb + srcp[x2+1] * Yg + srcp[x2+2] * Yr + OffyPlusHalf) >> shift;	// x  , y = H - 2
				dif += abs(tl-tr);
			}
		}
	} else {
		env->ThrowError("%sUnknown Colorspace",myName);
	}
	int count = (slices) ? (W/4 * H/4) * 4 : 2*((W-3)*(H-3)) + (H-3) + (W-3);
    double value = double(dif) / count;		// count will be coerced to double during division
	return value;
}
