#include "C:\Users\jaden\Desktop\minhook-master\minhook-master\include\MinHook.h"
#include "Conversion.h"
#include "memcheck.h"

uintptr_t Hook = format(0x11EB1F0);

void HookInitialize()
{
	MH_Initialize();
	MH_CreateHook(reinterpret_cast<LPVOID>(Hook), ExecuteVM, reinterpret_cast<LPVOID*>(&HookedU));
	MH_EnableHook(reinterpret_cast<LPVOID>(Hook));
}

inline uintptr_t GetDataModel()
{
    DWORD dm[4] = { 0 };
    ((DWORD(__thiscall*)(DWORD, DWORD))format(0xe4d1e0))(((DWORD(__cdecl*)())format(0xe4d090))(), (DWORD)dm);
    return dm[0] + 0xC;
}


const char* GetClass(int self)
{
    return (const char*)(*(int(**)(void))(*(int*)self + 16))();
}

int FindFirstClass(int Instance, const char* Name)
{
    DWORD StartOfChildren = *(DWORD*)(Instance + 0x2C);
    DWORD EndOfChildren = *(DWORD*)(StartOfChildren + 4);

    for (int i = *(int*)StartOfChildren; i != EndOfChildren; i += 8)
    {
        if (memcmp(GetClass(*(int*)i), Name, strlen(Name)) == 0)
        {
            return *(int*)i;
        }
    }
}

void EnableDebugConsole()//Simply Enables You To Open A Roblox Console
{
    DWORD OldPerm;
    VirtualProtect(&FreeConsole, 1, PAGE_EXECUTE_READWRITE, &OldPerm);
    *(BYTE*)&FreeConsole = 0xC3;
    VirtualProtect(&FreeConsole, 1, OldPerm, NULL);
    AllocConsole();
    SetConsoleTitleA("Console");
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);
}

#define r_lua_pop(State,n)		r_lua_settop(State, -(n)-1)

DWORD r_L = 0;

DWORD WINAPI input(PVOID lvpParameter)
{
    using namespace std;
    string WholeScript = "";
    HANDLE hPipe;
    char buffer[999999];
    DWORD dwRead;
    hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\artexexamplepipe"),
        PIPE_ACCESS_DUPLEX | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
        PIPE_WAIT,
        1,
        999999,
        999999,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL);
    while (hPipe != INVALID_HANDLE_VALUE)
    {
        if (ConnectNamedPipe(hPipe, NULL) != FALSE)
        {
            while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
            {
                buffer[dwRead] = '\0';
                try {
                    try {
                        WholeScript = WholeScript + buffer;
                    }
                    catch (...) {
                    }
                }
                catch (std::exception e) {

                }
                catch (...) {

                }
            }

            auto* L = luaL_newstate();

            auto State = RBX::Lua::lua_newthread(r_L);

            int identity[] = { 7 };
            int unk[] = { 0, 0 };
            using sandboxFn = int(__cdecl*)(int, int[], int[]);
            sandboxFn r_sandbox = (sandboxFn)format(0x726bc0);
            r_sandbox(State, identity, unk);

    
          
           
            int err = luaL_loadbuffer(L, WholeScript.c_str(), WholeScript.length(), "artex[hvm]");
            if (err == 0)
            {
                TValue* top = L->top - 1;
                LClosure* func = reinterpret_cast<LClosure*>(top->value.gc);
                auto* P = func->p;
              
                call_closure(State, func);
                r_lua_pop(State, 1);
            }
            else
            {
                std::cout << "[Compiler Error] " << lua_tostring(L, -1) << std::endl;
               
                lua_pop(L, 1);
            }

            WholeScript = "";
        }
        DisconnectNamedPipe(hPipe);
    }
}



void main()
{
   
    EnableDebugConsole();


	memcheck::bypass();
    unprotect_all();
 
    
	auto dm = GetDataModel();

    auto SC = FindFirstClass(dm, "ScriptContext");
    auto v2 = SC;
    auto v3 = 0;
    r_L = v2 + 56 * v3 + 164 + *(DWORD*)(v2 + 56 * v3 + 164);

    auto* L = luaL_newstate();

    

    auto State = RBX::Lua::lua_newthread(r_L);

    int identity[] = { 7 };
    int unk[] = { 0, 0 };
    using sandboxFn = int(__cdecl*)(int, int[], int[]);
    sandboxFn r_sandbox = (sandboxFn)format(0x726bc0);
    r_sandbox(State, identity, unk);

   

    std::string source;
    std::cout << "-->";
    while (std::getline(std::cin, source))
    {
        int err = luaL_loadbuffer(L, source.c_str(), source.length(), "artex[hvm]");
        if (err == 0)
        {
            G(L)->convert = true;
            TValue* top = L->top - 1;
            LClosure* func = reinterpret_cast<LClosure*>(top->value.gc);
            auto* P = func->p;
            P->lastlinedefined = 1;
            call_closure(State, func);
            r_lua_pop(State, 1);
        }
        else
        {
            std::cout << "[Compiler Error] " << lua_tostring(L, -1) << std::endl;
           
            lua_pop(L, 1);
            G(L)->convert = false;
        }
    }
}

BOOL APIENTRY DllMain(HMODULE Module, DWORD Reason, void* Reserved)
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(Module);
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)main, NULL, NULL, NULL);
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)input, NULL, NULL, NULL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    default: break;
    }

    return TRUE;
}