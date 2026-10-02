#include "zml_plugin.h"
#include "managed.hpp"
#include "mask.hpp"
#include "patch.hpp"
#include <MinHook.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <atomic>
#include <memory>
namespace {
using namespace uid_mask;
struct Runtime {
    const ZmlHost* host{};
    Managed api;
    Method player, role;
    const void* infoField{};
    std::filesystem::path configPath;
    Config config;
    std::mutex configMutex;
    FILETIME stamp{};
    DWORD size{};
    bool knownStamp=false;
    std::string helper;
    void* targets[1]{};
    void (*processing)(void*,const void*){};
    std::atomic_bool hit=false, masked=false, failed=false;
    std::atomic_bool identityAvailable=false, identityUnavailable=false, candidate=false, sameAlias=false;
    std::atomic_bool active=false;
    void log(const char* message) {host->log(host->owner,message);}
    Config settings() {
        std::lock_guard lock(configMutex);
        WIN32_FILE_ATTRIBUTE_DATA info{};
        if(!GetFileAttributesExW(configPath.c_str(),GetFileExInfoStandard,&info))return config;
        if(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))return config;
        if(knownStamp && CompareFileTime(&stamp,&info.ftLastWriteTime)==0 && size==info.nFileSizeLow)return config;
        // Cache rejected revisions as well; don't flood logs or reread every glyph.
        knownStamp=true;stamp=info.ftLastWriteTime;size=info.nFileSizeLow;
        if(info.nFileSizeHigh || size>65536){log("config_rejected; previous valid settings retained");return config;}
        std::ifstream input(configPath,std::ios::binary);
        std::string bytes{std::istreambuf_iterator<char>(input),{}};
        Config next;
        if(input.bad() || !parse_config(bytes,next))log("config_rejected; previous valid settings retained");
        else {config=std::move(next);log("config_applied");}
        return config;
    }
    std::wstring own() {
        // No timer/identity cache: logout or account switch takes effect on
        // the next render instead of matching another player's text to old UID.
        auto p=api.call(player);
        auto system=api.get(p,infoField);
        auto id=system?api.string(api.call(role,system)):std::wstring{};
        return decimal(id)?id:std::wstring{};
    }
};
Runtime* rt{}; // successful hooks/DLL are process-lifetime; never unload live callbacks.
void error() {if(!rt->failed.exchange(true))rt->log("render_error; original display preserved");}
void processing(void* self,const void* method) {
    if(!rt->active.load()){rt->processing(self,method);return;}
    if(!rt->hit.exchange(true))rt->log("processing_hook_hit");
    std::unique_ptr<Managed::BufferSwap> swap;
    bool replaced=false;
    try {
        swap=std::make_unique<Managed::BufferSwap>(rt->api,self);
        if(swap->text.find_first_of(U"0123456789")!=swap->text.npos) {
            auto id=rt->own();
            if(id.empty()) {if(!rt->identityUnavailable.exchange(true))rt->log("identity_unavailable");}
            else {
                if(!rt->identityAvailable.exchange(true))rt->log("identity_available");
                std::u32string own(id.begin(),id.end());
                if(swap->text.find(own)!=swap->text.npos) {
                    if(!rt->candidate.exchange(true))rt->log("uid_buffer_seen");
                    auto cfg=rt->settings();
                    if(cfg.enabled) {
                        std::u32string alias(cfg.alias.begin(),cfg.alias.end());
                        if(alias==own && !rt->sameAlias.exchange(true))rt->log("alias_matches_identity");
                        auto value=mask_units<char32_t>(swap->text,own,alias);
                        if(value!=swap->text) {
                            swap->replace(value);
                            replaced=true;
                        }
                    }
                }
            }
        }
    }catch(...){swap.reset();error();}
    // All ingestion paths converge here. Restore the original backing buffer
    // on normal return or C++ unwinding; only generated processing data is fake.
    rt->processing(self,method);
    if(replaced && !rt->masked.exchange(true))rt->log("render_masked; buffer readback verified and original processing returned");
}
int rewrite(void*,const char* src,size_t n,ZmlSink sink,void* writer) {
    try {
        std::string output;
        if(!src || !sink || !patch(std::string_view(src,n),rt->helper,output)) {
            rt->log("refresh_contract_rejected; masking hooks remain independent");return 0;
        }
        sink(writer,output.data(),output.size());return 1;
    }catch(...){return 0;}
}
int start(const ZmlHost* host) {
    if(!host || host->size!=sizeof(ZmlHost) || host->abi!=1 || !host->log || !host->transform_lua ||
       !host->mod_directory || !host->state_directory || rt)return 0;
    auto r=std::make_unique<Runtime>();r->host=host;
    bool initialized=false;unsigned created{};
    try {
        auto path=[](const char* p){return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(p)));};
        auto file=path(host->mod_directory)/L"refresh.lua";
        if(std::filesystem::file_size(file)>32768)return 0;
        std::ifstream input(file,std::ios::binary);r->helper.assign(std::istreambuf_iterator<char>(input),{});
        if(r->helper.empty() || r->helper.find('\0')!=r->helper.npos)return 0;
        r->configPath=path(host->state_directory)/L"config.ini";
        if(!r->api.connect())throw std::runtime_error("IL2CPP exports unavailable");
        auto game=r->api.klass("Gameplay.Beyond.dll","Beyond.Gameplay","GameInstance");
        auto player=r->api.klass("Gameplay.Beyond.dll","Beyond.Gameplay","GamePlayer");
        auto info=r->api.klass("Gameplay.Beyond.dll","Beyond.Gameplay","PlayerInfoSystem");
        auto tmp=r->api.klass("Unity.TextMeshPro.dll","TMPro","TMP_Text");
        r->player=r->api.method(game,"get_player","Beyond.Gameplay.GamePlayer",{},true);
        r->role=r->api.method(info,"get_roleId","System.String",{});
        r->infoField=r->api.field(player,"playerInfoSystem","Beyond.Gameplay.PlayerInfoSystem");
        auto proc=r->api.method(tmp,"PopulateTextProcessingArray","System.Void",{});
        if(!r->player.code || !r->role.code || !r->infoField || !proc.code || !r->api.buffer_contract(tmp))
            throw std::runtime_error("read-only identity/render signature contract unavailable");
        r->targets[0]=proc.code;
        if(MH_Initialize()!=MH_OK)throw std::runtime_error("private MinHook init failed");
        initialized=true;
        void* detours[]{reinterpret_cast<void*>(&processing)};
        void** originals[]{reinterpret_cast<void**>(&r->processing)};
        for(unsigned i=0;i<1;++i) {
            if(MH_CreateHook(r->targets[i],detours[i],originals[i])!=MH_OK)throw std::runtime_error("render hook creation failed");
            ++created;
        }
        if(!host->transform_lua(host->owner,"UI/Panels/UIDPanel/UIDPanelCtrl",rewrite,nullptr))
            throw std::runtime_error("refresh transform registration failed");
        r->settings();
        rt=r.get();
        // Queue our exact targets only; never touch another plugin's hooks.
        for(auto target:r->targets)if(MH_QueueEnableHook(target)!=MH_OK)throw std::runtime_error("render hook queue failed");
        if(MH_ApplyQueued()!=MH_OK)throw std::runtime_error("render hook enable failed");
        rt->active=true;
        rt->log("Render hook ready: shared processing stage; raw backing restored after parse");r.release();return 1;
    }catch(const std::exception& e) {
        host->log(host->owner,e.what());
        r->active=false;bool clean=true;
        for(unsigned i=0;i<created;++i){
            auto disabled=MH_DisableHook(r->targets[i]);
            if(disabled==MH_OK || disabled==MH_ERROR_DISABLED)clean &= MH_RemoveHook(r->targets[i])==MH_OK;
            else clean=false;
        }
        if(clean){if(initialized)MH_Uninitialize();rt=nullptr;}
        else {rt=r.release();host->log(host->owner,"rollback_incomplete; inactive hook context retained for process lifetime");}
        return 0;
    }catch(...) {
        r->active=false;bool clean=true;
        for(unsigned i=0;i<created;++i){
            auto disabled=MH_DisableHook(r->targets[i]);
            if(disabled==MH_OK || disabled==MH_ERROR_DISABLED)clean &= MH_RemoveHook(r->targets[i])==MH_OK;
            else clean=false;
        }
        if(clean){if(initialized)MH_Uninitialize();rt=nullptr;}else rt=r.release();
        return 0;
    }
}
const ZmlPlugin plugin{sizeof(ZmlPlugin),1,"uid-mask",start};
}
extern "C" __declspec(dllexport) const ZmlPlugin* ZML_PluginV1(){return &plugin;}
