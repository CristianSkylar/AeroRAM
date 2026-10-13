#pragma once
#include <windows.h>
#include <cstring>
template<class T> T loadWinApi(HMODULE module,const char* name) {
    const FARPROC raw=GetProcAddress(module,name);
    T function=nullptr;
    static_assert(sizeof(function)==sizeof(raw),"Windows procedure pointer size mismatch");
    std::memcpy(&function,&raw,sizeof(function));
    return function;
}
