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

AVSValue __cdecl RT_BitNOT(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return ~args[0].AsInt();
}

AVSValue __cdecl RT_BitAND(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return (args[0].AsInt() & args[1].AsInt());
}

AVSValue __cdecl RT_BitOR(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return (args[0].AsInt() | args[1].AsInt());
}

AVSValue __cdecl RT_BitXOR(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return (args[0].AsInt() ^ args[1].AsInt());
}

AVSValue __cdecl RT_BitLSL(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return (int)((unsigned int)(args[0].AsInt()) << (args[1].AsInt() & 0x1F));
}

AVSValue __cdecl RT_BitLSR(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return (int)((unsigned int)(args[0].AsInt()) >> (args[1].AsInt() & 0x1F));
}

AVSValue __cdecl RT_BitASL(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return (args[0].AsInt() << (args[1].AsInt() & 0x1F));
}

AVSValue __cdecl RT_BitASR(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return (args[0].AsInt() >> (args[1].AsInt() & 0x1F));
}

AVSValue __cdecl RT_BitTST(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return ((args[0].AsInt() & (1 << (args[1].AsInt() & 0x1F)))!=0);	
}

AVSValue __cdecl RT_BitCLR(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return ((args[0].AsInt() & (~(1 << (args[1].AsInt() & 0x1F)))));	
}

AVSValue __cdecl RT_BitSET(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return ((args[0].AsInt() | (1 << (args[1].AsInt() & 0x1F))));
}

AVSValue __cdecl RT_BitCHG(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return (int) ((args[0].AsInt() ^ (1U << (args[1].AsInt() & 0x1F))));
}

AVSValue __cdecl RT_BitROR(AVSValue args, void* user_data, IScriptEnvironment* env) {
	unsigned int n		= (unsigned int)args[0].AsInt(); 
	unsigned int cnt	= (unsigned int)args[1].AsInt() & 0x1F;
	return (int)((n >> cnt)  | (n << (32-cnt)));
}

AVSValue __cdecl RT_BitROL(AVSValue args, void* user_data, IScriptEnvironment* env) {
	unsigned int n		= (unsigned int)args[0].AsInt(); 
	unsigned int cnt	= (unsigned int)args[1].AsInt() & 0x1F;
	return (int)((n << cnt)  | (n >> (32-cnt)));
}

AVSValue __cdecl RT_BitSetCount(AVSValue args, void* user_data, IScriptEnvironment* env) {
	unsigned int n		= (unsigned int)args[0].AsInt(); 
	int cnt=0;
	for(int i=0;i<32;n>>=1,++i)
		if(n&0x01) ++cnt;
	return (int)cnt;
}


