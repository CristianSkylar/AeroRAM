#pragma once
#include <array>
#include <cstdint>
#include <cwctype>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace skylar {
inline const std::array<const wchar_t*,8> SpecCategories={L"Sistema",L"CPU",L"RAM",L"Gráficos",L"Discos",L"Red",L"Audio",L"Dispositivos"};
inline std::wstring trimSpec(std::wstring value) {
    const auto first=value.find_first_not_of(L" \t\r\n\0",0,5);
    if(first==std::wstring::npos)return {};
    const auto last=value.find_last_not_of(L" \t\r\n\0",std::wstring::npos,5);
    value=value.substr(first,last-first+1);
    for(auto& c:value)if(c<32 && c!=L'\n')c=L' ';
    return value;
}
inline std::wstring cleanSpec(std::wstring value) {
    value=trimSpec(std::move(value));std::wstring lower=value;
    for(auto& c:lower)c=wchar_t(std::towlower(c));
    for(const wchar_t* placeholder:{L"",L"unknown",L"none",L"not specified",L"default string",L"to be filled by o.e.m.",L"system manufacturer",L"system product name",L"not available",L"n/a"})
        if(lower==placeholder)return L"No reportado";
    return value;
}
inline std::optional<uint64_t> specInteger(const std::wstring& source) {
    const auto s=trimSpec(source);if(s.empty())return std::nullopt;
    uint64_t n=0;
    for(wchar_t c:s){if(c<L'0'||c>L'9')return std::nullopt;const auto d=uint64_t(c-L'0');if(n>(std::numeric_limits<uint64_t>::max()-d)/10)return std::nullopt;n=n*10+d;}
    return n;
}
inline std::wstring specDecimal(double value,int digits=2) {std::wostringstream s;s<<std::fixed<<std::setprecision(digits)<<value;return s.str();}
inline std::wstring specBytes(const std::wstring& raw,bool decimal=false) {
    const auto n=specInteger(raw);if(!n)return L"No reportado";
    return specDecimal(double(*n)/(decimal?1e9:1073741824.0))+ (decimal?L" GB":L" GiB");
}
inline std::wstring specUnit(const std::wstring& raw,const wchar_t* unit,bool zeroUnknown=true) {
    const auto n=specInteger(raw);if(!n || (zeroUnknown && *n==0))return L"No reportado";return std::to_wstring(*n)+unit;
}
inline std::wstring specDate(const std::wstring& raw,bool time=false) {
    if(raw.size()<8 || raw.substr(0,4)==L"0000")return L"No reportado";
    for(size_t i=0;i<(time?14U:8U);++i)if(i>=raw.size() || raw[i]<L'0'||raw[i]>L'9')return L"No reportado";
    std::wstring s=raw.substr(6,2)+L"/"+raw.substr(4,2)+L"/"+raw.substr(0,4);
    if(time)s+=L" "+raw.substr(8,2)+L":"+raw.substr(10,2);
    return s;
}
inline std::wstring specRamType(const std::wstring& raw) {
    const auto n=specInteger(raw);if(!n)return L"No reportado";
    switch(*n){case 18:return L"DDR";case 19:return L"DDR2";case 24:return L"DDR3";case 26:return L"DDR4";case 27:return L"LPDDR";case 28:return L"LPDDR2";case 29:return L"LPDDR3";case 30:return L"LPDDR4";case 34:return L"DDR5";case 35:return L"LPDDR5";default:return *n>2?L"SMBIOS: "+std::to_wstring(*n):L"No reportado";}
}
struct SpecField {std::wstring label,value;};
struct SpecBlock {std::wstring title;std::vector<SpecField> fields;};
struct SystemInfo {
    std::array<std::vector<SpecBlock>,8> pages;
    std::array<bool,8> ready{};
    std::wstring collectedAt,note;
    bool complete=false;
};
inline std::wstring specReport(const SystemInfo& info) {
    std::wstring s=L"AeroRAM · Informe del sistema\r\n";
    if(!info.collectedAt.empty())s+=L"Lectura: "+info.collectedAt+L"\r\n";
    for(size_t page=0;page<info.pages.size();++page){
        s+=L"\r\n["+std::wstring(SpecCategories[page])+L"]\r\n";
        if(info.pages[page].empty())s+=info.ready[page]?L"Sin datos reportados.\r\n":L"Lectura pendiente.\r\n";
        for(const auto& block:info.pages[page]){s+=L"\r\n"+block.title+L"\r\n";for(const auto& f:block.fields)s+=f.label+L": "+f.value+L"\r\n";}
    }
    if(!info.note.empty())s+=L"\r\n"+info.note+L"\r\n";
    return s;
}
}
