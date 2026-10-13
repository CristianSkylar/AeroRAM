#include "memory.hpp"
#include "win_api.hpp"
#include <psapi.h>
#include <cwchar>

namespace skylar {
MemorySample readMemory() {
    MEMORYSTATUSEX s{}; s.dwLength = sizeof(s);
    MemorySample result;
    if (!GlobalMemoryStatusEx(&s)) return result;
    result.valid = true;
    result.total = s.ullTotalPhys;
    result.available = s.ullAvailPhys;
    result.commitTotal = s.ullTotalPageFile;
    result.commitAvailable = s.ullAvailPageFile;
    PERFORMANCE_INFORMATION p{}; p.cb = sizeof(p);
    if (GetPerformanceInfo(&p, sizeof(p))) result.cache = uint64_t(p.SystemCache)*p.PageSize;
    return result;
}
double appMemoryMB() {
    PROCESS_MEMORY_COUNTERS p{}; p.cb = sizeof(p);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &p, sizeof(p))) return double(p.WorkingSetSize)/MiB;
    return 0;
}
class ProfilePrivilege {
    HANDLE token_ = nullptr;
    TOKEN_PRIVILEGES previous_{};
    bool assigned_ = false;
public:
    ProfilePrivilege() {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &token_)) return;
        TOKEN_PRIVILEGES wanted{}; wanted.PrivilegeCount = 1;
        if (!LookupPrivilegeValueW(nullptr, SE_PROF_SINGLE_PROCESS_NAME, &wanted.Privileges[0].Luid)) return;
        wanted.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        DWORD size = sizeof(previous_); SetLastError(ERROR_SUCCESS);
        const BOOL ok = AdjustTokenPrivileges(token_, FALSE, &wanted, sizeof(previous_), &previous_, &size);
        assigned_ = ok && GetLastError() == ERROR_SUCCESS;
    }
    bool valid() const { return assigned_; }
    ~ProfilePrivilege() {
        if (token_) {
            if (assigned_ && previous_.PrivilegeCount) AdjustTokenPrivileges(token_, FALSE, &previous_, 0, nullptr, nullptr);
            CloseHandle(token_);
        }
    }
    ProfilePrivilege(const ProfilePrivilege&) = delete;
};
CleanResult cleanMemory(CleanOptions options) {
    CleanResult r; r.before = readMemory();
    r.requested = int(options.cache)+int(options.apps);
    if (!r.requested) { r.details=L"Activa al menos una opción de limpieza en Ajustes."; r.after=readMemory(); return r; }


    using NtSetInfo = LONG (NTAPI*)(ULONG, PVOID, ULONG);
    auto nt = loadWinApi<NtSetInfo>(GetModuleHandleW(L"ntdll.dll"), "NtSetSystemInformation");
    if (!nt) { r.details=L"Windows no expone esta función de limpieza."; r.after=readMemory(); return r; }
    ProfilePrivilege privilege;
    if (!privilege.valid()) {
        r.details=L"No se pudo habilitar el permiso de limpieza. Ejecuta Skylar Memory como administrador.";
        r.after=readMemory(); return r;
    }
    auto run = [&](ULONG command, const wchar_t* name) {
        const LONG status = nt(80, &command, sizeof(command));
        wchar_t line[160]{};
        if (status >= 0) { ++r.succeeded; swprintf(line, 160, L"%ls: completado.\n", name); }
        else swprintf(line, 160, L"%ls: Windows devolvió 0x%08lX.\n", name, static_cast<unsigned long>(status));
        r.details += line;
    };
    if (options.apps) run(2, L"Reducir memoria de aplicaciones");
    if (options.cache) run(5, L"Caché de baja prioridad");
    r.after = readMemory();
    r.details += L"\nEl cambio mostrado compara la RAM disponible antes y después. Otras aplicaciones pueden asignar memoria durante la limpieza. No se garantiza una mejora de velocidad.";
    return r;
}
}
