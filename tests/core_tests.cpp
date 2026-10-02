#include "mask.hpp"
#include "patch.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("core check failed");}
std::string read(const char* p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){try {
    using namespace uid_mask;
    if(argc==4){
        check(!std::filesystem::exists(argv[3]));std::string out;
        check(patch(read(argv[1]),read(argv[2]),out));
        std::ofstream f(argv[3],std::ios::binary);f<<out;f.close();check(bool(f));
        std::cout<<"PASS: actual source patched atomically\n";return 0;
    }
    check(decimal(L"0") && decimal(L"99999999999999999999"));
    for(auto s:{L"",L"-12",L"1e9",L" 12",L"<b>12",L"123456789012345678901",L"１２３"})check(!decimal(s));
    const std::wstring own=L"1000123456",alias=L"876543210";
    check(mask(L"UID: 1000123456",own,alias)==L"UID: 876543210");
    check(mask(L"1000123456",own,alias)==alias);
    check(mask(L"用户1000123456 / 1000123456",own,alias)==L"用户876543210 / 876543210");
    check(mask(L"11000123456 10001234560 1000123457",own,alias)==L"11000123456 10001234560 1000123457");
    check(mask(L"<link=1000123456><color=#123>1000123456</color></link>",own,alias)==L"<link=1000123456><color=#123>876543210</color></link>");
    check(mask(L"UID: 1000123456",own,L"bad")==L"UID: 1000123456");
    check(mask(mask(own,own,alias),own,alias)==alias);
    check(mask_units<char32_t>(U"🙂1000123456",U"1000123456",U"42")==U"🙂42");
    Config c;check(parse_config("alias_uid=876543210\nenabled=false\n",c));check(!c.enabled && c.alias==alias);
    for(auto bytes:{"", "enabled=true\n", "enabled=true\nalias_uid=bad\n", "enabled=true\nalias_uid=123\nalias_uid=4\n", "[values]\nenabled=true\nalias_uid=4\n"}){
        check(!parse_config(bytes,c));check(!c.enabled && c.alias==alias);
    }
    check(parse_config("enabled=true\r\nalias_uid=0\r\n",c) && c.enabled && c.alias==L"0");
    check(!c.maskName && !c.maskShort); // legacy saved config keeps UID behavior, new fields opt-in.
    check(parse_config("enabled=true\nalias_uid=4\nmask_name=true\nalias_name=昵称🙂\nmask_short_id=true\nalias_short_id=0007\n",c));
    check(c.aliasName==U"昵称🙂" && c.aliasShort==U"0007");
    for(auto bad:{"alias_name=<b>x</b>\n","alias_name=\xff\n","alias_name=\n","mask_name=yes\n","mask_name=true\nmask_name=false\n","alias_short_id=#001\n"})check(!parse_config(std::string("enabled=true\nalias_uid=4\n")+bad,c));
    Identity identity{U"1000123456",U"测试🙂",U"0042"};
    c.aliasName=U"1000123456";c.alias=L"123";
    // Verify nickname does not override UID alias
    check(render_identity(U"测试🙂#0042 / UID: 1000123456",identity,c)==U"1000123456#0007 / UID: 123");
    check(render_identity(U"正文测试🙂#0042",identity,c)==U"正文测试🙂#0042");
    check(render_identity(U"他人#0042 / 测试🙂#0043",identity,c)==U"他人#0042 / 测试🙂#0043");
    check(render_identity(U"<link=测试🙂#0042>普通</link>",identity,c)==U"<link=测试🙂#0042>普通</link>");
    check(render_identity(U"  测试🙂  ",identity,c)==U"  1000123456  ");
    check(render_identity(U"测试🙂#0042suffix",identity,c)==U"测试🙂#0042suffix");
    check(scalar_text(L"测试🙂")==U"测试🙂");
    const std::string fixture=
        "UIDPanelCtrl = HL.Class('UIDPanelCtrl', uiCtrl.UICtrl)\n"
        "UIDPanelCtrl.OnCreate = HL.Override(HL.Any) << function(self, arg)\n"
        "    self.view.text.text=GameInstance.player.playerInfoSystem.roleId\n"
        "    _zmlHudFields.bind(self)\nend\n\n"
        "UIDPanelCtrl.OnClose = HL.Override() << function(self)\nend\nHL.Commit(UIDPanelCtrl)\n";
    std::string out;check(patch(fixture,"return {}",out));
    check(out.find("_zmlHudFields.bind(self)\n    _zmlUidMask.bind(self)")!=out.npos);
    check(out.find("_zmlUidMask.close(self)")!=out.npos);
    std::string kept="unchanged";
    for(auto bad:{std::string(""),fixture+"HL.Commit(UIDPanelCtrl)",out}){check(!patch(bad,"return {}",kept));check(kept=="unchanged");}
    auto crlf=fixture;for(size_t i=0;(i=crlf.find('\n',i))!=crlf.npos;i+=2)crlf.insert(i,"\r");
    check(patch(crlf,"return {}",out));
    std::cout<<"PASS: numeric masking, rich tags, boundaries, config rejection, atomic patch, HUD composition\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
