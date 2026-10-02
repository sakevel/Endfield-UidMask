#include "zml_plugin.h"
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <source_location>
void check(bool ok,std::source_location loc=std::source_location::current()){
    if(!ok)throw std::runtime_error("native hook check failed at line "+std::to_string(loc.line()));
}
std::vector<std::string> logs;bool deny=false;
void log(void*,const char* s){logs.emplace_back(s);}
int transforms(void*,const char* p,ZmlLuaTransform,void*){check(std::string(p)=="UI/Panels/UIDPanel/UIDPanelCtrl");return !deny;}
std::string utf8(const std::filesystem::path& p){auto s=p.u8string();return {reinterpret_cast<const char*>(s.data()),s.size()};}
int main(int argc,char** argv){try{
    check(argc>=4);
    auto fixture=LoadLibraryW(std::filesystem::path(argv[1]).c_str());check(fixture!=nullptr);
    auto render=reinterpret_cast<const wchar_t*(*)(const wchar_t*,int)>(GetProcAddress(fixture,"FixtureRender"));
    auto raw=reinterpret_cast<const wchar_t*(*)()>(GetProcAddress(fixture,"FixtureRaw"));
    auto backingRaw=reinterpret_cast<const wchar_t*(*)()>(GetProcAddress(fixture,"FixtureBackingRaw"));
    auto flag=reinterpret_cast<unsigned(*)()>(GetProcAddress(fixture,"FixtureBufferFlag"));
    auto id=reinterpret_cast<void(*)(const wchar_t*)>(GetProcAddress(fixture,"FixtureIdentity"));
    auto roots=reinterpret_cast<unsigned(*)()>(GetProcAddress(fixture,"FixtureRoots"));
    auto mode=reinterpret_cast<void(*)(int)>(GetProcAddress(fixture,"FixtureMode"));check(render && raw && id && roots && mode);
    auto invalidWrites=reinterpret_cast<unsigned(*)()>(GetProcAddress(fixture,"FixtureInvalidReferenceWrites"));check(invalidWrites);
    auto dll=LoadLibraryW(std::filesystem::path(argv[2]).c_str());check(dll!=nullptr);
    auto entry=reinterpret_cast<ZmlPluginEntry>(GetProcAddress(dll,"ZML_PluginV1"));check(entry!=nullptr);
    auto plugin=entry();check(plugin->size==sizeof(ZmlPlugin) && plugin->abi==1 && std::string(plugin->id)=="uid-mask");
    check(!plugin->start(nullptr));
    auto state=std::filesystem::temp_directory_path()/(L"ZML-UidMaskTest-"+std::to_wstring(GetCurrentProcessId()));
    check(!std::filesystem::exists(state));std::filesystem::create_directory(state);
    auto dir=utf8(std::filesystem::absolute(argv[3])),data=utf8(state);
    ZmlHost host{sizeof(ZmlHost),1,nullptr,dir.c_str(),data.c_str(),log,transforms};
    auto wrong=host;wrong.abi=99;check(!plugin->start(&wrong));
    std::string scenario=argc==5?argv[4]:"positive";
    if(scenario=="reject")mode(1);
    if(scenario=="layout-reject")mode(2);
    if(scenario=="rollback")deny=true;
    int success=plugin->start(&host);
    if(scenario=="legacy-reference-repro"){
        check(success==1);
        render(L"UID: 1000123456",3);
        check(invalidWrites()>0);check(roots()==0);
        check(std::wstring(backingRaw())==L"UID: 1000123456");
        std::cout<<"REPRODUCED: v0.1.1 sends a stack address to the object-reference setter\n";
    }else if(scenario=="legacy-repro"){
        check(success==1);check(std::wstring(render(L"UID: 1000123456",3))==L"UID: 1000123456");
        std::cout<<"REPRODUCED: legacy string hooks leave direct-buffer UID unchanged\n";
    }else if(scenario!="positive"){
        check(!success);check(std::wstring(render(L"UID: 1000123456",0))==L"UID: 1000123456");
    }else{
        check(success==1);
        for(int path=0;path<6;++path){
            auto text=path==2?L"xx1000123456yy":L"UID: 1000123456";
            check(std::wstring(render(text,path))==(path==2?L"1000000000":L"UID: 1000000000"));
            check(std::wstring(raw())==text);check(roots()==0);
            check(std::wstring(backingRaw())==(path==2?L"1000123456":text));check(flag()==0xABCDEF12);
        }
        check(std::wstring(render(L"1000123457 / 11000123456",0))==L"1000123457 / 11000123456");
        check(std::wstring(render(L"<link=1000123456>1000123456</link>",0))==L"<link=1000123456>1000000000</link>");
        check(std::wstring(render(L"🙂1000123456",3))==L"🙂1000000000");
        mode(3);bool thrown=false;
        try{render(L"1000123456",3);}catch(const std::exception&){thrown=true;}
        check(thrown && std::wstring(backingRaw())==L"1000123456" && roots()==0);mode(0);
        mode(4);check(std::wstring(render(L"1000123456",3))==L"1000123456");check(roots()==0);mode(0);
        mode(5);check(std::wstring(render(L"1000123456",3))==L"1000123456");
        check(std::wstring(backingRaw())==L"1000123456" && roots()==0);mode(0);
        auto save=[&](std::string bytes){std::ofstream f(state/L"new.ini",std::ios::binary);f<<bytes;f.close();check(MoveFileExW((state/L"new.ini").c_str(),(state/L"config.ini").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0);};
        save("alias_uid=876543210\nenabled=true\n");
        check(std::wstring(render(L"1000123456",0))==L"876543210");
        save("alias_uid=123456\nenabled=true\n");
        check(std::wstring(render(L"UID: 1000123456",3))==L"UID: 123456");
        save("alias_uid=12345678901234567890\nenabled=true\n");
        check(std::wstring(render(L"1000123456",3))==L"12345678901234567890");
        save("alias_uid=876543210\nenabled=false\n");
        check(std::wstring(render(L"1000123456",0))==L"1000123456");
        save("alias_uid=123456789\nenabled=true\n");
        check(std::wstring(render(L"1000123456",1))==L"123456789");
        save("alias_uid=<bad>\nenabled=true\n");
        check(std::wstring(render(L"1000123456",1))==L"123456789");
        id(L"2000123456");
        check(std::wstring(render(L"1000123456 / 2000123456",0))==L"1000123456 / 123456789");
        id(L"");check(std::wstring(render(L"2000123456",0))==L"2000123456");
        check(roots()==0);
        check(invalidWrites()==0);
        for(auto& s:logs)check(s.find("1000123456")==s.npos && s.find("876543210")==s.npos);
    }
    // Remove only test-owned files. DLL/hooks intentionally remain until this test process exits.
    for(auto file:{L"new.ini",L"config.ini"})std::filesystem::remove(state/file);
    std::filesystem::remove(state);
    std::cout<<"PASS: actual DLL/MinHook synthetic native pipeline, scenario="<<scenario<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
