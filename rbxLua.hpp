#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>

#include <math.h>
#define luai_numadd(a,b)	((a)+(b))
#define luai_numsub(a,b)	((a)-(b))
#define luai_nummul(a,b)	((a)*(b))
#define luai_numdiv(a,b)	((a)/(b))
#define luai_nummod(a,b)	((a) - floor((a)/(b))*(b))
#define luai_numpow(a,b)	(pow(a,b))
#define luai_numunm(a)		(-(a))
#define luai_numeq(a,b)		((a)==(b))
#define luai_numlt(a,b)		((a)<(b))
#define luai_numle(a,b)		((a)<=(b))
#define luai_numisnan(a)	(!luai_numeq((a), (a)))

extern "C" {
#include "Lua/lua.hpp"
#include "Lua/lopcodes.h"
#include "Lua/ldo.h"
#include "Lua/lfunc.h"
#include "Lua/lobject.h"
#include "Lua/llex.h"
}

typedef union r_GCObject r_GCObject;

struct r_GCheader
{
	r_GCObject* next;
	byte marked;
	byte tt;
};

union r_GCObject
{
	r_GCheader gch;
	DWORD ts; /* tstring */
	DWORD u; /* userdata */
	DWORD cl; /* closure */
	DWORD h; /* table */
	DWORD p; /* pointer */
	DWORD uv; /* upvalue */
	DWORD th;  /* thread */
};

typedef union
{
	r_GCObject* gc;
	void* p;
	double n;
	int b;
} r_Value;

#define r_TValuefields    r_Value value; union { r_lua_TValue *upv; int flg; }; int tt

typedef struct r_lua_TValue
{
	r_TValuefields;
} r_TValue;

typedef r_TValue* r_StkId;

#define format(x)(x - 0x400000 + (DWORD)GetModuleHandle(NULL))



#define Log(...) printf("[%s] -> ", __FUNCTION__); printf(__VA_ARGS__)


#define R_LUA_TNIL 0
#define R_LUA_TLIGHTUSERDATA 4
#define R_LUA_TNUMBER 3
#define R_LUA_TBOOLEAN 1
#define R_LUA_TSTRING 5
#define R_LUA_TTHREAD 8
#define R_LUA_TFUNCTION 6
#define R_LUA_TTABLE 7
#define R_LUA_TUSERDATA 9
#define R_LUA_VECTOR 2


#define r_setnilvalue(obj) ((obj)->tt = R_LUA_TNIL)
#define r_setnvalue(obj, x) { r_TValue *i_o = (obj); i_o->value.n = (x); i_o->tt = R_LUA_TNUMBER; }
#define r_setpvalue(obj, x) { r_TValue *i_o = (obj); i_o->value.p = (x); i_o->tt = R_LUA_TLIGHTUSERDATA; }
#define r_setbvalue(obj, x) { r_TValue *i_o = (obj); i_o->value.b = (x); i_o->tt = R_LUA_TBOOLEAN; }
#define r_setsvalue(obj, x) { r_TValue *i_o = (obj); i_o->value.gc = cast(r_GCObject*, (x)); i_o->tt = R_LUA_TSTRING; }
#define r_setuvalue(obj, x) { r_TValue *i_o = (obj); i_o->value.gc = cast(r_GCObject*, (x)); i_o->tt = R_LUA_TUSERDATA; }
#define r_setthvalue(obj, x){ r_TValue *i_o = (obj); i_o->value.gc = cast(r_GCObject*, (x)); i_o->tt = R_LUA_TTHREAD; }
#define r_setclvalue(obj, x){ r_TValue *i_o = (obj); i_o->value.gc = cast(r_GCObject*, (x)); i_o->tt = R_LUA_TFUNCTION; }
#define r_sethvalue(obj, x) { r_TValue *i_o = (obj); i_o->value.gc = cast(r_GCObject*, (x)); i_o->tt = R_LUA_TTABLE; }


#define Declare(address, returnValue, callingConvention, ...) (returnValue(callingConvention*)(__VA_ARGS__))(format(address))

static double XorDouble(double num)
{
	uint64_t U64_Xor = *reinterpret_cast<uint64_t*>(&num) ^ *reinterpret_cast<uint64_t*>(format(0x235F150));
	return *reinterpret_cast<double*>(&U64_Xor);
}

#include <intrin.h>
static DWORD dwFindPattern(unsigned char* pData, unsigned int end_addr, const unsigned char* pat, const char* msk)
{
	const unsigned char* end = (unsigned char*)end_addr - strlen(msk);
	int num_masks = ceil((float)strlen(msk) / (float)16);
	int masks[32]; //32*16 = enough masks for 512 bytes
	memset(masks, 0, num_masks * sizeof(int));
	for (int i = 0; i < num_masks; ++i)
		for (int j = strnlen(msk + i * 16, 16) - 1; j >= 0; --j)
			if (msk[i * 16 + j] == 'x')
				masks[i] |= 1 << j;

	__m128i xmm1 = _mm_loadu_si128((const __m128i*) pat);
	__m128i xmm2, xmm3, mask;
	for (; pData != end; _mm_prefetch((const char*)(++pData + 64), _MM_HINT_NTA)) {
		if (pat[0] == pData[0]) {
			xmm2 = _mm_loadu_si128((const __m128i*) pData);
			mask = _mm_cmpeq_epi8(xmm1, xmm2);
			if ((_mm_movemask_epi8(mask) & masks[0]) == masks[0]) {
				for (int i = 1; i < num_masks; ++i) {
					xmm2 = _mm_loadu_si128((const __m128i*) (pData + i * 16));
					xmm3 = _mm_loadu_si128((const __m128i*) (pat + i * 16));
					mask = _mm_cmpeq_epi8(xmm2, xmm3);
					if ((_mm_movemask_epi8(mask) & masks[i]) == masks[i]) {
						if ((i + 1) == num_masks)
							return (DWORD)pData;
					}
					else goto cont;
				}
				return (DWORD)pData;
			}
		}cont:;
	}
	return NULL;
}

static void unprotect_all() {
	DWORD base = reinterpret_cast<DWORD>(GetModuleHandle(NULL));
	unsigned long eip = base + 0x1000;
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(reinterpret_cast<void*>(eip), &mbi, sizeof(MEMORY_BASIC_INFORMATION));
	while (eip - base - 0x1000 < mbi.RegionSize) {
		eip = dwFindPattern((unsigned char*)eip, mbi.RegionSize + base, (unsigned char*)"\x72\x00\xA1\x00\x00\x00\x00\x8B", "x?x????x");
		if (!eip)
			break;
		DWORD oldProtect;
		VirtualProtect((LPVOID)eip, 1,
			PAGE_EXECUTE_READWRITE,
			(PDWORD)&oldProtect);
		memcpy((LPVOID)eip, "\xEB", 1);
		VirtualProtect((LPVOID)eip, 1,
			oldProtect, (PDWORD)&oldProtect);
	}
}


namespace RBX
{
	using luaD_precallFn = int(__cdecl*)(int, r_TValue*, int);
	using luaV_gettableFn = void(__cdecl*)(int, r_TValue*, r_TValue*, r_TValue*);
	using luaV_settableFn = void(__cdecl*)(int, r_TValue*, r_TValue*, r_TValue*);
	using luaS_newlstrFn = const char* (__fastcall*)(uintptr_t, const char*, size_t);
	using ArtihFn = void(__cdecl*)(int, r_TValue*, r_TValue*, r_TValue*, int);
	using luaHNew = void* (__cdecl*)(int, int, int);
	using luaH_getnFn = int(__cdecl*)(Table*);
	using luaV_concatFn = int(__cdecl*)(int, int, int);
	using r_equalobj = int(__cdecl*)(int, r_TValue*, r_TValue*);
	using luaVLessthan = int(__cdecl*)(int, r_TValue*, r_TValue*);
	using lessequal_fn = int(__cdecl*)(int, r_TValue*, r_TValue*);
	using luaD_poscallFn = int(__cdecl*)(int, r_TValue*);
	namespace Lua
	{
		static auto luaD_precall = reinterpret_cast<luaD_precallFn>(format(0x11EBD20));
		static auto luaS_newlstr = reinterpret_cast<luaS_newlstrFn>(format(0x11F5510));
		static auto luaV_gettable = reinterpret_cast<luaV_gettableFn>(format(0x11f7580));
		static auto luaV_settable = reinterpret_cast<luaV_settableFn>(format(0x11f77e0));//Declare(0x11f77e0, void, __cdecl, int, r_TValue*, r_TValue*, r_TValue*);
		static auto Arith = reinterpret_cast<ArtihFn>(format(0x11F7140));//Declare(0x11F7140, void, __cdecl, int, r_TValue*, r_TValue*, r_TValue*, int);
		static auto luaH_new = reinterpret_cast<luaHNew>(format(0x11f5d70));

		static auto luaH_getn = reinterpret_cast<luaH_getnFn>(format(0x11F7A40));
		static auto luaV_concat = reinterpret_cast<luaV_concatFn>(format(0x11F6E00));
		static auto equalobj = reinterpret_cast<r_equalobj>(format(0x11F7450));
		static auto luaV_lessthan = reinterpret_cast<luaVLessthan>(format(0x11F7740));
		static auto lessequal = reinterpret_cast<lessequal_fn>(format(0x11F76A0));
		static auto luaD_poscall = reinterpret_cast<luaD_poscallFn>(format(0x11EBC90));
		static auto luaF_close = Declare(0x11F3C10, int, __cdecl, int, r_TValue*);
		static auto luaF_newLClosure = Declare(0x11F3F20, int, __cdecl, int, int, int, int);
		static auto luaF_newproto = Declare(0x11F3F90, int, __cdecl, int);
		static auto lua_newthread = Declare(0x11E05B0, int, __cdecl, int);
		static auto luaM_realloc = Declare(0x11F7D60, void*, __cdecl, int, int);
		static auto luaH_setnum = Declare(0x11F6110, r_TValue*, __cdecl, int, Table*, int);
		static auto VNameCall = Declare(0x11EC090, int, __cdecl, int, r_TValue*);
	}
}

#define protected_call(x, f) { __try { x; } __except(1) { f; } }
#define Protect(f) protected_call(f)


using spawnFn = void(__cdecl*)(int);
static auto r_spawn = (spawnFn)format(0x726f30);
static auto r_lua_settop = Declare(0x11E1840, void, __stdcall, int, int);
static auto r_luaH_resizeArray = Declare(0x11F5FA0, void, __cdecl, int, Table*, int);