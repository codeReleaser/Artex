#pragma once
#include "Translator.hpp"
#include "Virtual Machine.hpp"
#include <TlHelp32.h>


#define proto_obfuscate(p,v) *(int*)(p) = (int)(v) - (int)(p)

DWORD CreateString(DWORD RS, TString* Str)
{
	DWORD Ptr = (DWORD)RBX::Lua::luaS_newlstr(RS, getstr(Str), Str->tsv.len);
	//l_setbit(*(BYTE*)(Ptr + GCO_MARKED), FIXEDBIT);
	return Ptr;
}

DWORD CreateString(DWORD RS, const std::string& Str)
{
	DWORD Ptr = (DWORD)RBX::Lua::luaS_newlstr(RS, Str.c_str(), Str.length());
	//l_setbit(*(BYTE*)(Ptr + GCO_MARKED), FIXEDBIT);
	return Ptr;
}

void convertTValue(DWORD RS, TValue* LVal, r_TValue* RVal)
{
	switch (ttype(LVal))
	{
	case LUA_TNIL:
		r_setnilvalue(RVal);
		break;
	case LUA_TBOOLEAN:
		r_setbvalue(RVal, bvalue(LVal));
		break;
	case LUA_TNUMBER:
		r_setnvalue(RVal, XorDouble(nvalue(LVal)));
		break;
	case LUA_TSTRING:
		r_setsvalue(RVal, CreateString(RS, rawtsvalue(LVal)));
		break;
	case LUA_TLIGHTUSERDATA:
		RVal->tt = R_LUA_TLIGHTUSERDATA;
		RVal->value.p = LVal->value.p;
		break;
	case LUA_TUSERDATA:
		Log("TUSERRD\n");
		RVal->tt = R_LUA_TUSERDATA;
		RVal->value.p = LVal->value.p;
		break;
	}
}

using _DWORD = DWORD;//lazyness
using _BYTE = BYTE;

DWORD ConvertProto(DWORD r_L, Proto* p)
{



	DWORD rp = RBX::Lua::luaF_newproto(r_L);

	auto instrs = InitatateInstruction(p);

	proto_obfuscate(rp + 20, RBX::Lua::luaS_newlstr(r_L, getstr(p->source), strlen(getstr(p->source))));

	*(_BYTE*)(rp + 82) = p->maxstacksize;
	*(_BYTE*)(rp + 80) = p->numparams;
	*(_BYTE*)(rp + 81) = p->nups;
	*(_BYTE*)(rp + 83) = p->is_vararg | 8;
	
	*(DWORD*)(rp + 68) = p->sizecode | instrs.size();
	auto* code = reinterpret_cast<Instruction*>(RBX::Lua::luaM_realloc(r_L, sizeof(Instruction) * instrs.size()));
	proto_obfuscate(rp + 12, code);

	*(DWORD*)(rp + 76) = p->sizek;
	auto* k = reinterpret_cast<r_TValue*>(RBX::Lua::luaM_realloc(r_L, sizeof(r_TValue) * p->sizek));
	proto_obfuscate(rp + 36, k);

	*(DWORD*)(rp + 56) = p->sizep;
	auto* p_p = reinterpret_cast<DWORD*>(RBX::Lua::luaM_realloc(r_L, sizeof(DWORD) * p->sizep));
	proto_obfuscate(rp + 8, p_p);


	*(_DWORD*)(rp + 60) = p->sizelineinfo;
	auto v83 = *(_DWORD*)(rp + 68);
	auto v84 = (v83 + 3) & 0xFFFFFFFC;
	auto v166 = ((v83 - 1) >> 0x18) + 1;
	auto v85 = v84 + 4 * v166;
	*(_DWORD*)(rp + 52) = v85;
	auto* v86 = (int*)RBX::Lua::luaM_realloc(r_L, sizeof(uintptr_t) * v85);
	auto v87 = rp;
	auto v88 = (int)v86 - (rp + 16);
	*(_DWORD*)(rp + 16) = v88;
	auto v89 = v88 + v84 - (v87 + 28) + v87 + 16;
	auto v90 = 0;
	*(_DWORD*)(v87 + 28) = v89;

	for (auto j = 0; j < p->sizelineinfo; j++)
	{
		v86[j] = p->lineinfo[j];
	}

	for (auto i = 0; i < p->sizek; i++)
	{
		r_TValue* ro = &k[i];
		convertTValue(r_L, &p->k[i], ro);
	}


	for (auto i = 0; i < instrs.size(); i++)
	{
		code[i] = (Instruction)instrs[i];
	}
	
	
	for (auto i = 0; i < p->sizep; i++)
	{
		p_p[i] = ConvertProto(r_L, p->p[i]);
	}



	return rp;
}

void call_closure(DWORD r_L, LClosure* lcl)
{
	Proto* p = lcl->p;
	auto proto = ConvertProto(r_L, p);
	auto rlcl = RBX::Lua::luaF_newLClosure(r_L, p->nups, *(int*)(r_L + 56), proto);
	*(DWORD*)(rlcl + 24) = rlcl + 24 - proto;
	auto v141 = *(r_TValue**)(r_L + 24);
	v141->tt = R_LUA_TFUNCTION;
	v141->value.gc = reinterpret_cast<r_GCObject*>(rlcl);
	*(DWORD*)(r_L + 24) += 16;
	Log("spawning!\n");
	r_spawn(r_L);
}

using HookedUFn = int(__cdecl*)(uintptr_t rL);
HookedUFn HookedU;

void pause() {
	THREADENTRY32 te32;
	te32.dwSize = sizeof(THREADENTRY32);
	HANDLE hThreads = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, NULL);
	if (Thread32First(hThreads, &te32)) {
		while (Thread32Next(hThreads, &te32)) {
			if (te32.th32OwnerProcessID == GetCurrentProcessId() && te32.th32ThreadID != GetCurrentThreadId()) {
				HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, false, te32.th32ThreadID);
				SuspendThread(hThread);
				CloseHandle(hThread);
			}
		}
	}
	CloseHandle(hThreads);
}

void resume() {
	THREADENTRY32 te32;
	te32.dwSize = sizeof(THREADENTRY32);
	HANDLE hThreads = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, NULL);
	if (Thread32First(hThreads, &te32)) {
		while (Thread32Next(hThreads, &te32)) {
			if (te32.th32OwnerProcessID == GetCurrentProcessId() && te32.th32ThreadID != GetCurrentThreadId()) {
				HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, false, te32.th32ThreadID);
				ResumeThread(hThread);
				CloseHandle(hThread);
			}
		}
	}
	CloseHandle(hThreads);
}

int ExecuteVM(int rL)
{
	auto v2 = *(DWORD*)(rL + 28);
	auto v3 = *(DWORD*)(v2 + 8);
	auto v413 = **(DWORD**)v2;
	auto v4 = v413 + 24 - *(DWORD*)(v413 + 24);

	uintptr_t cl = v413 + 24 - *(DWORD*)(v413 + 24);

	if (*(BYTE*)(cl + 83) & 8)
	{

		auto vm = Luau();

//		pause();

		vm.RunVirtualMachine(rL, 0, 1);

		//resume();

		return 1;
	}
	else
	{
		return HookedU(rL);
	}
}