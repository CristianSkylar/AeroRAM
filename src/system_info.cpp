#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wbemidl.h>
#include <initguid.h>
#include <dxdiag.h>
#include <dxgi.h>
#include <d3d11.h>
#include <oleauto.h>
#include <map>
#include <memory>
#include <utility>
#include "system_info.hpp"

namespace skylar { namespace {
template<class T> struct Com {
    T* p=nullptr;
    ~Com(){if(p)p->Release();}
    Com()=default;Com(const Com&)=delete;Com& operator=(const Com&)=delete;
    T** out(){return &p;}T* operator->()const{return p;}
};
struct BStr {BSTR p;explicit BStr(const std::wstring& s):p(SysAllocStringLen(s.data(),UINT(s.size()))){}~BStr(){SysFreeString(p);}};
struct Variant {VARIANT v;Variant(){VariantInit(&v);}~Variant(){VariantClear(&v);}};
std::wstring variantText(const VARIANT& v){
    if(v.vt==VT_NULL || v.vt==VT_EMPTY)return {};
    if(v.vt==VT_BOOL)return v.boolVal==VARIANT_FALSE?L"No":L"Sí";
    if(v.vt==VT_BSTR)return v.bstrVal?trimSpec(std::wstring(v.bstrVal,SysStringLen(v.bstrVal))):L"";
    if(v.vt & VT_ARRAY){
        SAFEARRAY* a=v.parray;if(!a || SafeArrayGetDim(a)!=1)return {};
        LONG low=0,high=-1;SafeArrayGetLBound(a,1,&low);SafeArrayGetUBound(a,1,&high);
        std::wstring result;
        for(LONG n=low;n<=high && n-low<128;++n){
            std::wstring item;
            if((v.vt & VT_TYPEMASK)==VT_BSTR){BSTR s=nullptr;if(SUCCEEDED(SafeArrayGetElement(a,&n,&s))){if(s)item=trimSpec(std::wstring(s,SysStringLen(s)));SysFreeString(s);}}
            else if((v.vt & VT_TYPEMASK)==VT_VARIANT){Variant x;if(SUCCEEDED(SafeArrayGetElement(a,&n,&x.v)))item=variantText(x.v);}
            if(!item.empty()){if(!result.empty())result+=L"; ";result+=item;}
        }
        return result;
    }
    Variant converted;
    if(SUCCEEDED(VariantChangeType(&converted.v,const_cast<VARIANT*>(&v),0,VT_BSTR)))return variantText(converted.v);
    return {};
}
using Row=std::map<std::wstring,std::wstring>;
std::wstring value(const Row& r,const wchar_t* key){auto i=r.find(key);return i==r.end()?L"":i->second;}
std::wstring hexCode(HRESULT hr){wchar_t b[20]{};swprintf(b,20,L"0x%08lX",static_cast<unsigned long>(hr));return b;}
class Wmi {
    Com<IWbemServices> service;
    const std::atomic_bool& cancelled;
    ULONGLONG deadline;
public:
    std::wstring error;
    explicit Wmi(const std::atomic_bool& c,const wchar_t* space=L"ROOT\\CIMV2"):cancelled(c),deadline(GetTickCount64()+60000){
        Com<IWbemLocator> locator;
        HRESULT hr=CoCreateInstance(CLSID_WbemLocator,nullptr,CLSCTX_INPROC_SERVER,IID_IWbemLocator,reinterpret_cast<void**>(locator.out()));
        if(SUCCEEDED(hr)){BStr path(space);hr=locator->ConnectServer(path.p,nullptr,nullptr,nullptr,WBEM_FLAG_CONNECT_USE_MAX_WAIT,nullptr,nullptr,service.out());}
        if(SUCCEEDED(hr))hr=CoSetProxyBlanket(service.p,RPC_C_AUTHN_WINNT,RPC_C_AUTHZ_NONE,nullptr,RPC_C_AUTHN_LEVEL_CALL,RPC_C_IMP_LEVEL_IMPERSONATE,nullptr,EOAC_NONE);
        if(FAILED(hr))error=L"WMI no disponible ("+hexCode(hr)+L").";
    }
    std::vector<Row> query(const wchar_t* cls,std::initializer_list<const wchar_t*> props,const wchar_t* where=L"",size_t limit=64){
        std::vector<Row> rows;
        if(!service.p || !error.empty() || cancelled || GetTickCount64()>deadline)return rows;
        std::wstring q=L"SELECT ";for(const auto p:props){if(q.size()>7)q+=L",";q+=p;}q+=L" FROM ";q+=cls;q+=where;
        BStr lang(L"WQL"),statement(q);Com<IEnumWbemClassObject> enumerator;
        HRESULT hr=service->ExecQuery(lang.p,statement.p,WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY,nullptr,enumerator.out());
        if(FAILED(hr) || !enumerator.p)return rows;
        const auto queryEnd=GetTickCount64()+8000;
        while(!cancelled && GetTickCount64()<deadline && GetTickCount64()<queryEnd && rows.size()<limit){
            Com<IWbemClassObject> object;ULONG returned=0;hr=enumerator->Next(250,1,object.out(),&returned);
            if(returned && object.p){Row row;for(const auto p:props){Variant v;if(SUCCEEDED(object->Get(p,0,&v.v,nullptr,nullptr)))row[p]=variantText(v.v);}rows.push_back(std::move(row));}
            if(FAILED(hr) || hr==WBEM_S_FALSE)break;
        }
        return rows;
    }
};
void field(SpecBlock& b,const wchar_t* name,std::wstring v){b.fields.push_back({name,cleanSpec(std::move(v))});}
void fields(SpecBlock& b,const Row& r,std::initializer_list<std::pair<const wchar_t*,const wchar_t*>> list){for(auto p:list)field(b,p.first,value(r,p.second));}
void addRows(SystemInfo& info,int page,const std::vector<Row>& rows,const wchar_t* name,const std::function<void(SpecBlock&,const Row&)>& fn){
    for(size_t n=0;n<rows.size();++n){SpecBlock block;block.title=name;if(rows.size()>1)block.title+=L" "+std::to_wstring(n+1);fn(block,rows[n]);info.pages[page].push_back(std::move(block));}
}
std::wstring registryString(const wchar_t* path,const wchar_t* key){
    DWORD type=0,bytes=0;if(RegGetValueW(HKEY_LOCAL_MACHINE,path,key,RRF_RT_REG_SZ,nullptr,nullptr,&bytes)!=ERROR_SUCCESS || bytes>65536)return {};
    std::wstring s(bytes/sizeof(wchar_t),L'\0');if(RegGetValueW(HKEY_LOCAL_MACHINE,path,key,RRF_RT_REG_SZ,&type,s.data(),&bytes)!=ERROR_SUCCESS)return {};return trimSpec(s);
}
void basicFallback(SystemInfo& info){
    if(info.pages[0].empty()){
        SpecBlock b{L"Equipo",{}};field(b,L"Fabricante",registryString(L"HARDWARE\\DESCRIPTION\\System\\BIOS",L"SystemManufacturer"));field(b,L"Modelo",registryString(L"HARDWARE\\DESCRIPTION\\System\\BIOS",L"SystemProductName"));
        wchar_t computer[256]{};DWORD length=256;if(GetComputerNameW(computer,&length))field(b,L"Nombre del equipo",computer);
        SYSTEM_INFO s{};GetNativeSystemInfo(&s);field(b,L"Procesadores lógicos",std::to_wstring(s.dwNumberOfProcessors));
        MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);if(GlobalMemoryStatusEx(&memory))field(b,L"RAM utilizable",specBytes(std::to_wstring(memory.ullTotalPhys)));
        info.pages[0].push_back(std::move(b));
        SpecBlock board{L"Placa base",{}};field(board,L"Fabricante",registryString(L"HARDWARE\\DESCRIPTION\\System\\BIOS",L"BaseBoardManufacturer"));field(board,L"Modelo",registryString(L"HARDWARE\\DESCRIPTION\\System\\BIOS",L"BaseBoardProduct"));info.pages[0].push_back(std::move(board));
    }
    if(info.pages[1].empty()){
        SpecBlock b{L"Procesador",{}};field(b,L"Modelo",registryString(L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",L"ProcessorNameString"));field(b,L"Fabricante",registryString(L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",L"VendorIdentifier"));info.pages[1].push_back(std::move(b));
    }
}
std::wstring gpuVendor(UINT id){switch(id){case 0x8086:return L"Intel";case 0x10DE:return L"NVIDIA";case 0x1002:return L"AMD";case 0x1414:return L"Microsoft";default:{wchar_t b[20]{};swprintf(b,20,L"PCI 0x%04X",id);return b;}}}
std::wstring storageBus(const std::wstring& raw){const auto n=specInteger(raw);if(!n)return L"No reportado";switch(*n){case 1:return L"SCSI";case 2:return L"ATAPI";case 3:return L"ATA";case 7:return L"USB";case 8:return L"RAID";case 9:return L"iSCSI";case 10:return L"SAS";case 11:return L"SATA";case 12:return L"SD";case 13:return L"MMC";case 15:return L"Virtual (archivo)";case 16:return L"Storage Spaces";case 17:return L"NVMe";default:return *n?L"Bus "+std::to_wstring(*n):L"No reportado";}}
void dxgiInfo(SystemInfo& info,const std::atomic_bool& cancelled){
    Com<IDXGIFactory1> factory;HRESULT hr=CreateDXGIFactory1(IID_IDXGIFactory1,reinterpret_cast<void**>(factory.out()));if(FAILED(hr))return;
    for(UINT n=0;n<16 && !cancelled;++n){Com<IDXGIAdapter1> adapter;if(factory->EnumAdapters1(n,adapter.out())!=S_OK)break;DXGI_ADAPTER_DESC1 d{};if(FAILED(adapter->GetDesc1(&d)) || (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))continue;
        SpecBlock b{L"Memoria gráfica · "+std::wstring(d.Description),{}};field(b,L"Fabricante PCI",gpuVendor(d.VendorId));field(b,L"Memoria de vídeo dedicada",specBytes(std::to_wstring(d.DedicatedVideoMemory)));field(b,L"Memoria del sistema dedicada",specBytes(std::to_wstring(d.DedicatedSystemMemory)));field(b,L"Memoria compartida disponible",specBytes(std::to_wstring(d.SharedSystemMemory)));
        wchar_t pci[100]{};swprintf(pci,100,L"VEN_%04X · DEV_%04X · SUBSYS_%08X · REV_%02X",d.VendorId,d.DeviceId,d.SubSysId,d.Revision);field(b,L"Identificadores PCI",pci);info.pages[3].push_back(std::move(b));
        const D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0,D3D_FEATURE_LEVEL_9_3,D3D_FEATURE_LEVEL_9_2,D3D_FEATURE_LEVEL_9_1};D3D_FEATURE_LEVEL supported{};
        HRESULT probe=D3D11CreateDevice(adapter.p,D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,levels,7,D3D11_SDK_VERSION,nullptr,&supported,nullptr);
        if(probe==E_INVALIDARG)probe=D3D11CreateDevice(adapter.p,D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,levels+1,6,D3D11_SDK_VERSION,nullptr,&supported,nullptr);
        field(info.pages[3].back(),L"Nivel disponible en Direct3D 11",SUCCEEDED(probe)?std::to_wstring(unsigned(supported)>>12)+L"."+std::to_wstring((unsigned(supported)>>8)&15):L"No disponible ("+hexCode(probe)+L")");
    }
}
Row dxProperties(IDxDiagContainer* container){
    Row row;DWORD count=0;if(FAILED(container->GetNumberOfProps(&count)))return row;
    for(DWORD n=0;n<count && n<512;++n){wchar_t name[512]{};if(FAILED(container->EnumPropNames(n,name,512)))continue;Variant v;if(SUCCEEDED(container->GetProp(name,&v.v)))row[name]=variantText(v.v);}
    return row;
}
void dxdiagInfo(SystemInfo& info,const std::atomic_bool& cancelled){
    if(cancelled)return;
    Com<IDxDiagProvider> provider;HRESULT hr=CoCreateInstance(CLSID_DxDiagProvider,nullptr,CLSCTX_INPROC_SERVER,IID_IDxDiagProvider,reinterpret_cast<void**>(provider.out()));
    DXDIAG_INIT_PARAMS parameters{sizeof(DXDIAG_INIT_PARAMS),DXDIAG_DX9_SDK_VERSION,FALSE,nullptr};
    if(SUCCEEDED(hr))hr=provider->Initialize(&parameters);
    if(FAILED(hr)){info.note+=L" Diagnóstico DirectX no disponible ("+hexCode(hr)+L").";return;}
    Com<IDxDiagContainer> root;if(FAILED(provider->GetRootContainer(root.out())))return;
    Com<IDxDiagContainer> system;
    if(SUCCEEDED(root->GetChildContainer(L"DxDiag_SystemInfo",system.out()))){
        const auto r=dxProperties(system.p);SpecBlock b{L"DirectX · diagnóstico de Windows",{}};
        fields(b,r,{{L"Versión de DirectX",L"szDirectXVersionLongEnglish"},{L"Sistema operativo",L"szOSExLongEnglish"},{L"BIOS",L"szBIOS"}});info.pages[3].push_back(std::move(b));
    }
    Com<IDxDiagContainer> displays;if(FAILED(root->GetChildContainer(L"DxDiag_DisplayDevices",displays.out())))return;
    DWORD count=0;displays->GetNumberOfChildContainers(&count);
    for(DWORD n=0;n<count && n<16 && !cancelled;++n){
        wchar_t name[512]{};if(FAILED(displays->EnumChildContainerNames(n,name,512)))continue;Com<IDxDiagContainer> child;if(FAILED(displays->GetChildContainer(name,child.out())))continue;
        const auto r=dxProperties(child.p);SpecBlock b{L"Pantalla · diagnóstico "+std::to_wstring(n+1),{}};
        fields(b,r,{{L"Dispositivo",L"szDescription"},{L"Fabricante",L"szManufacturer"},{L"Chip gráfico",L"szChipType"},{L"Modo de pantalla",L"szDisplayModeEnglish"},{L"Versión del controlador",L"szDriverVersion"},{L"Fecha del controlador",L"szDriverDateEnglish"}});
        for(const auto& p:std::initializer_list<std::pair<const wchar_t*,const wchar_t*>>{{L"Modelo del controlador",L"szDriverModelEnglish"},{L"Niveles de funciones Direct3D",L"szD3DFeatureLevels"},{L"Versión DDI",L"szDDIVersionEnglish"},{L"Archivos del controlador",L"szDriverName"},{L"Identificador de dispositivo",L"szDeviceIdentifier"}})if(!value(r,p.second).empty())field(b,p.first,value(r,p.second));
        info.pages[3].push_back(std::move(b));
    }
}
}

SystemInfo readSystemInfo(const std::atomic_bool& cancelled,const std::function<void(const SystemInfo&)>& progress){
    SystemInfo info;SYSTEMTIME time{};GetLocalTime(&time);wchar_t stamp[64]{};swprintf(stamp,64,L"%02u/%02u/%04u %02u:%02u",time.wDay,time.wMonth,time.wYear,time.wHour,time.wMinute);info.collectedAt=stamp;
    info.note=L"Lectura local de Windows, WMI/SMBIOS, DXGI y DxDiag. Los campos no informados por el fabricante aparecen como «No reportado».";
    HRESULT initialized=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(FAILED(initialized)){basicFallback(info);info.note+=L" COM no disponible ("+hexCode(initialized)+L").";info.ready.fill(true);info.complete=true;return info;}
    struct Uninit{~Uninit(){CoUninitialize();}} uninit;
    const auto security=CoInitializeSecurity(nullptr,-1,nullptr,nullptr,RPC_C_AUTHN_LEVEL_DEFAULT,RPC_C_IMP_LEVEL_IMPERSONATE,nullptr,EOAC_NONE,nullptr);
    if(FAILED(security) && security!=RPC_E_TOO_LATE)info.note+=L" Seguridad COM: "+hexCode(security)+L".";
    Wmi wmi(cancelled);
    auto publish=[&](int p){info.ready[p]=true;if(progress && !cancelled)progress(info);};
    auto query=[&](const wchar_t* c,std::initializer_list<const wchar_t*> f,const wchar_t* condition=L""){return wmi.query(c,f,condition);};
    addRows(info,0,query(L"Win32_ComputerSystem",{L"Manufacturer",L"Model",L"Name",L"SystemType",L"TotalPhysicalMemory",L"NumberOfProcessors",L"NumberOfLogicalProcessors",L"HypervisorPresent"}),L"Equipo",[](auto& b,const auto& r){
        fields(b,r,{{L"Fabricante",L"Manufacturer"},{L"Modelo",L"Model"},{L"Nombre del equipo",L"Name"},{L"Arquitectura",L"SystemType"},{L"Procesadores físicos",L"NumberOfProcessors"},{L"Procesadores lógicos",L"NumberOfLogicalProcessors"},{L"Hipervisor presente",L"HypervisorPresent"}});field(b,L"RAM utilizable por Windows",specBytes(value(r,L"TotalPhysicalMemory")));
    });
    addRows(info,0,query(L"Win32_OperatingSystem",{L"Caption",L"Version",L"BuildNumber",L"OSArchitecture",L"InstallDate",L"LastBootUpTime"}),L"Windows",[](auto& b,const auto& r){
        fields(b,r,{{L"Edición",L"Caption"},{L"Versión",L"Version"},{L"Compilación",L"BuildNumber"},{L"Arquitectura",L"OSArchitecture"}});field(b,L"Fecha de instalación",specDate(value(r,L"InstallDate"),true));field(b,L"Último arranque",specDate(value(r,L"LastBootUpTime"),true));
    });
    addRows(info,0,query(L"Win32_BaseBoard",{L"Manufacturer",L"Product",L"Version",L"SerialNumber"}),L"Placa base",[](auto& b,const auto& r){fields(b,r,{{L"Fabricante",L"Manufacturer"},{L"Modelo",L"Product"},{L"Revisión",L"Version"},{L"Número de serie",L"SerialNumber"}});});
    addRows(info,0,query(L"Win32_BIOS",{L"Manufacturer",L"SMBIOSBIOSVersion",L"ReleaseDate",L"SerialNumber",L"SMBIOSMajorVersion",L"SMBIOSMinorVersion"}),L"BIOS / SMBIOS",[](auto& b,const auto& r){fields(b,r,{{L"Fabricante",L"Manufacturer"},{L"Versión de BIOS",L"SMBIOSBIOSVersion"},{L"Número de serie",L"SerialNumber"}});field(b,L"Fecha de BIOS",specDate(value(r,L"ReleaseDate")));field(b,L"Versión SMBIOS",value(r,L"SMBIOSMajorVersion").empty() || value(r,L"SMBIOSMinorVersion").empty()?L"No reportado":value(r,L"SMBIOSMajorVersion")+L"."+value(r,L"SMBIOSMinorVersion"));});
    FIRMWARE_TYPE firmware=FirmwareTypeUnknown;SpecBlock boot{L"Modo de arranque",{}};const bool known=GetFirmwareType(&firmware)!=FALSE;field(boot,L"Firmware de arranque",known && firmware==FirmwareTypeUefi?L"UEFI":known && firmware==FirmwareTypeBios?L"BIOS heredada":L"No reportado");
    addRows(info,0,query(L"Win32_ComputerSystemProduct",{L"Vendor",L"Name",L"Version",L"IdentifyingNumber",L"UUID"}),L"Identificación del fabricante",[](auto& b,const auto& r){fields(b,r,{{L"Fabricante",L"Vendor"},{L"Producto",L"Name"},{L"Versión",L"Version"},{L"Serie del producto",L"IdentifyingNumber"},{L"UUID",L"UUID"}});});
    basicFallback(info);info.pages[0].push_back(std::move(boot));publish(0);
    addRows(info,1,query(L"Win32_Processor",{L"Name",L"Manufacturer",L"NumberOfCores",L"NumberOfLogicalProcessors",L"MaxClockSpeed",L"CurrentClockSpeed",L"L2CacheSize",L"L3CacheSize",L"SocketDesignation",L"AddressWidth",L"ProcessorId"}),L"Procesador",[](auto& b,const auto& r){
        fields(b,r,{{L"Modelo",L"Name"},{L"Fabricante",L"Manufacturer"},{L"Núcleos",L"NumberOfCores"},{L"Hilos lógicos",L"NumberOfLogicalProcessors"},{L"Socket",L"SocketDesignation"},{L"ID del procesador",L"ProcessorId"}});field(b,L"Frecuencia máxima informada",specUnit(value(r,L"MaxClockSpeed"),L" MHz"));field(b,L"Frecuencia actual informada",specUnit(value(r,L"CurrentClockSpeed"),L" MHz"));field(b,L"Caché L2",specUnit(value(r,L"L2CacheSize"),L" KiB"));field(b,L"Caché L3",specUnit(value(r,L"L3CacheSize"),L" KiB"));field(b,L"Ancho de direcciones",specUnit(value(r,L"AddressWidth"),L" bits"));
    });
    if(info.pages[1].size()>1 && info.pages[1][0].fields.size()==2)info.pages[1].erase(info.pages[1].begin());
    publish(1);
    auto modules=query(L"Win32_PhysicalMemory",{L"Capacity",L"Manufacturer",L"PartNumber",L"SerialNumber",L"Speed",L"ConfiguredClockSpeed",L"SMBIOSMemoryType",L"FormFactor",L"BankLabel",L"DeviceLocator",L"ConfiguredVoltage",L"DataWidth",L"TotalWidth"});
    uint64_t installed=0;for(const auto& m:modules)if(auto n=specInteger(value(m,L"Capacity")))installed+=*n;
    SpecBlock summary{L"Memoria física",{}};field(summary,L"Módulos detectados",modules.empty()?L"No reportado":std::to_wstring(modules.size()));if(installed)field(summary,L"RAM instalada (módulos)",specBytes(std::to_wstring(installed)));
    MEMORYSTATUSEX mem{};mem.dwLength=sizeof(mem);if(GlobalMemoryStatusEx(&mem)){field(summary,L"RAM utilizable por Windows",specBytes(std::to_wstring(mem.ullTotalPhys)));field(summary,L"Límite de memoria comprometida",specBytes(std::to_wstring(mem.ullTotalPageFile)));if(installed>=mem.ullTotalPhys)field(summary,L"Diferencia instalada / utilizable",specBytes(std::to_wstring(installed-mem.ullTotalPhys)));}info.pages[2].push_back(std::move(summary));
    addRows(info,2,modules,L"Módulo RAM",[](auto& b,const auto& r){
        field(b,L"Capacidad",specBytes(value(r,L"Capacity")));fields(b,r,{{L"Fabricante",L"Manufacturer"},{L"Referencia",L"PartNumber"},{L"Número de serie",L"SerialNumber"},{L"Ranura",L"DeviceLocator"},{L"Banco",L"BankLabel"}});field(b,L"Tipo de memoria",specRamType(value(r,L"SMBIOSMemoryType")));const auto form=value(r,L"FormFactor");field(b,L"Formato",form==L"8"?L"DIMM":form==L"12"?L"SO-DIMM":L"No reportado");field(b,L"Frecuencia configurada (WMI)",specUnit(value(r,L"ConfiguredClockSpeed"),L" MHz"));field(b,L"Velocidad nominal (WMI)",specUnit(value(r,L"Speed"),L" MHz"));field(b,L"Voltaje configurado",specUnit(value(r,L"ConfiguredVoltage"),L" mV"));field(b,L"Ancho de datos",specUnit(value(r,L"DataWidth"),L" bits"));field(b,L"Ancho total",specUnit(value(r,L"TotalWidth"),L" bits"));
    });
    addRows(info,2,query(L"Win32_PhysicalMemoryArray",{L"MemoryDevices",L"MaxCapacity",L"MaxCapacityEx"}),L"Ranuras de la placa",[](auto& b,const auto& r){field(b,L"Ranuras informadas",value(r,L"MemoryDevices"));const auto ex=specInteger(value(r,L"MaxCapacityEx")),old=specInteger(value(r,L"MaxCapacity"));field(b,L"Capacidad máxima informada",ex && *ex?specBytes(std::to_wstring(*ex)):old && *old?specBytes(std::to_wstring(*old*1024)):L"No reportado");});publish(2);
    addRows(info,3,query(L"Win32_VideoController",{L"Name",L"AdapterCompatibility",L"VideoProcessor",L"DriverVersion",L"DriverDate",L"VideoModeDescription",L"CurrentRefreshRate",L"PNPDeviceID"}),L"Adaptador gráfico",[](auto& b,const auto& r){fields(b,r,{{L"Nombre",L"Name"},{L"Fabricante",L"AdapterCompatibility"},{L"Chip",L"VideoProcessor"},{L"Controlador",L"DriverVersion"},{L"Resolución / modo",L"VideoModeDescription"},{L"ID del dispositivo",L"PNPDeviceID"}});field(b,L"Fecha del controlador",specDate(value(r,L"DriverDate")));field(b,L"Frecuencia de pantalla",specUnit(value(r,L"CurrentRefreshRate"),L" Hz"));});dxgiInfo(info,cancelled);publish(3);
    addRows(info,4,query(L"Win32_DiskDrive",{L"Model",L"Manufacturer",L"FirmwareRevision",L"SerialNumber",L"Size",L"InterfaceType",L"MediaType",L"Partitions",L"Status",L"PNPDeviceID"}),L"Disco físico",[](auto& b,const auto& r){fields(b,r,{{L"Modelo",L"Model"},{L"Fabricante",L"Manufacturer"},{L"Firmware",L"FirmwareRevision"},{L"Número de serie",L"SerialNumber"},{L"Interfaz informada",L"InterfaceType"},{L"Tipo de medio",L"MediaType"},{L"Particiones",L"Partitions"},{L"Estado",L"Status"},{L"ID del dispositivo",L"PNPDeviceID"}});field(b,L"Capacidad del disco",specBytes(value(r,L"Size"),true));});
    addRows(info,4,query(L"Win32_LogicalDisk",{L"DeviceID",L"VolumeName",L"FileSystem",L"Size",L"FreeSpace"},L" WHERE DriveType=3"),L"Volumen",[](auto& b,const auto& r){fields(b,r,{{L"Unidad",L"DeviceID"},{L"Etiqueta",L"VolumeName"},{L"Sistema de archivos",L"FileSystem"}});field(b,L"Capacidad",specBytes(value(r,L"Size"),true));field(b,L"Espacio libre al consultar",specBytes(value(r,L"FreeSpace"),true));});publish(4);
    if(!cancelled){
        Wmi storage(cancelled,L"ROOT\\Microsoft\\Windows\\Storage");
        addRows(info,4,storage.query(L"MSFT_PhysicalDisk",{L"FriendlyName",L"FirmwareVersion",L"Size",L"BusType",L"MediaType",L"LogicalSectorSize",L"PhysicalSectorSize",L"SpindleSpeed",L"IsWriteCacheEnabled",L"IsPowerProtected",L"PhysicalLocation"}),L"Detalles de almacenamiento",[](auto& b,const auto& r){
            fields(b,r,{{L"Dispositivo",L"FriendlyName"},{L"Firmware",L"FirmwareVersion"},{L"Caché de escritura activa",L"IsWriteCacheEnabled"},{L"Protección frente a pérdida de energía",L"IsPowerProtected"},{L"Ubicación",L"PhysicalLocation"}});field(b,L"Capacidad",specBytes(value(r,L"Size"),true));const auto media=value(r,L"MediaType");field(b,L"Tecnología",media==L"3"?L"HDD":media==L"4"?L"SSD":media==L"5"?L"SCM":L"No reportado");field(b,L"Bus de almacenamiento",storageBus(value(r,L"BusType")));field(b,L"Sector lógico",specUnit(value(r,L"LogicalSectorSize"),L" bytes"));field(b,L"Sector físico",specUnit(value(r,L"PhysicalSectorSize"),L" bytes"));field(b,L"Rotación informada",specUnit(value(r,L"SpindleSpeed"),L" rpm"));
        });publish(4);
    }
    addRows(info,5,query(L"Win32_NetworkAdapter",{L"Name",L"Manufacturer",L"MACAddress",L"Speed",L"NetEnabled",L"NetConnectionID",L"PNPDeviceID"},L" WHERE PhysicalAdapter=True"),L"Adaptador de red",[](auto& b,const auto& r){fields(b,r,{{L"Nombre",L"Name"},{L"Fabricante",L"Manufacturer"},{L"Conexión",L"NetConnectionID"},{L"Activo",L"NetEnabled"},{L"Dirección MAC",L"MACAddress"},{L"ID del dispositivo",L"PNPDeviceID"}});const auto speed=specInteger(value(r,L"Speed"));field(b,L"Velocidad de enlace informada",speed && *speed?specDecimal(double(*speed)/1e6,0)+L" Mbps":L"No reportado");});
    addRows(info,5,query(L"Win32_NetworkAdapterConfiguration",{L"Description",L"IPAddress",L"IPSubnet",L"DefaultIPGateway",L"DHCPEnabled",L"DNSServerSearchOrder"},L" WHERE IPEnabled=True"),L"Configuración de red",[](auto& b,const auto& r){fields(b,r,{{L"Adaptador",L"Description"},{L"Direcciones IP",L"IPAddress"},{L"Máscara / prefijo",L"IPSubnet"},{L"Puerta de enlace",L"DefaultIPGateway"},{L"DHCP",L"DHCPEnabled"},{L"Servidores DNS",L"DNSServerSearchOrder"}});});publish(5);
    addRows(info,6,query(L"Win32_SoundDevice",{L"Name",L"Manufacturer",L"Status",L"PNPDeviceID"}),L"Dispositivo de audio",[](auto& b,const auto& r){fields(b,r,{{L"Nombre",L"Name"},{L"Fabricante",L"Manufacturer"},{L"Estado",L"Status"},{L"ID del dispositivo",L"PNPDeviceID"}});});publish(6);
    addRows(info,7,query(L"Win32_PnPEntity",{L"Name",L"Manufacturer",L"PNPClass",L"Status",L"PNPDeviceID"},L" WHERE PNPClass='Keyboard' OR PNPClass='Mouse' OR PNPClass='HIDClass' OR PNPClass='Monitor'"),L"Entrada / pantalla",[](auto& b,const auto& r){fields(b,r,{{L"Nombre",L"Name"},{L"Fabricante",L"Manufacturer"},{L"Clase",L"PNPClass"},{L"Estado",L"Status"},{L"ID del dispositivo",L"PNPDeviceID"}});});publish(7);
    dxdiagInfo(info,cancelled);
    if(!wmi.error.empty())info.note+=L" "+wmi.error;
    info.ready.fill(true);info.complete=!cancelled;
    return info;
}
}
