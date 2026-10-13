#include "../src/spec_types.hpp"
#include <cassert>
#include <iostream>
using namespace skylar;
int main(){
    assert(cleanSpec(L"  Kingston  ")==L"Kingston");
    assert(cleanSpec(L"To Be Filled By O.E.M.")==L"No reportado");
    assert(cleanSpec(L"Unknown")==L"No reportado");
    assert(cleanSpec(L" ")==L"No reportado");
    assert(specInteger(L"18446744073709551615")==UINT64_MAX);
    assert(!specInteger(L"18446744073709551616"));
    assert(!specInteger(L"-1") && !specInteger(L"12GB"));
    assert(specBytes(L"8589934592")==L"8.00 GiB");
    assert(specBytes(L"0")==L"0.00 GiB");
    assert(specUnit(L"0",L" MHz")==L"No reportado");
    assert(specRamType(L"24")==L"DDR3" && specRamType(L"34")==L"DDR5");
    assert(specDate(L"20261012170600.000000-300",true)==L"12/10/2026 17:06");
    assert(specDate(L"invalid")==L"No reportado");
    SystemInfo i;i.ready.fill(true);i.complete=true;
    i.pages[0].push_back({L"Equipo",{{L"Fabricante",L"ASUSTeK"},{L"Modelo",L"Modelo específico"}}});
    const std::wstring longId(1500,L'A');
    i.pages[7].push_back({L"Entrada",{{L"ID del dispositivo",longId}}});
    const auto report=specReport(i);
    assert(report.find(L"ASUSTeK")!=std::wstring::npos && report.find(longId)!=std::wstring::npos);
    for(const wchar_t* c:SpecCategories)assert(report.find(L"["+std::wstring(c)+L"]")!=std::wstring::npos);
    assert(report.find(L"Lectura pendiente")==std::wstring::npos);
    std::cout<<"OEM placeholders, 64-bit capacities, units and complete untruncated report: passed.\n";
}
