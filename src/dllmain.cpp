#include "game_hooks.hpp"
#include <windows.h>

bool HOOK_FINISHED = false;

// this function is included to allow adding dll to import address table
// it is never called
__declspec(dllexport) void __stdcall bean() { std::cout << "strawbby"; }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReasonForCall,
                      LPVOID lpReserved) {
  switch (ulReasonForCall) {
  case DLL_PROCESS_ATTACH:
    if (!HOOK_FINISHED) {
      HOOK_FINISHED = true;
      do_hooks();
    }
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}
