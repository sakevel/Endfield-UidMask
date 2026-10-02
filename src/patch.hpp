#pragma once
#include <string>
#include <string_view>
namespace uid_mask {
inline bool patch(std::string_view source,std::string_view helper,std::string& out) {
    if(helper.empty() || helper.find('\0')!=helper.npos || source.find('\0')!=source.npos ||
       source.find("_zmlUidMask")!=source.npos)return false;
    std::string s(source);
    for(size_t p=0;(p=s.find("\r\n",p))!=s.npos;)s.erase(p,1);
    auto unique=[&](std::string_view token) {
        auto p=s.find(token);return p!=s.npos && s.find(token,p+token.size())==s.npos;
    };
    const std::string cls="UIDPanelCtrl = HL.Class('UIDPanelCtrl', uiCtrl.UICtrl)";
    const std::string tail="\nend\n\nUIDPanelCtrl.OnClose";
    const std::string close="UIDPanelCtrl.OnClose = HL.Override() << function(self)\n";
    for(auto token:{std::string_view(cls),std::string_view(tail),std::string_view(close),
                    std::string_view("UIDPanelCtrl.OnCreate = HL.Override(HL.Any) << function(self, arg)"),
                    std::string_view("GameInstance.player.playerInfoSystem.roleId"),
                    std::string_view("HL.Commit(UIDPanelCtrl)")}) {
        // Native OnCreate has two branches reading roleId, so presence only here.
        if(token=="GameInstance.player.playerInfoSystem.roleId") {if(s.find(token)==s.npos)return false;}
        else if(!unique(token))return false;
    }
    s.insert(s.find(close)+close.size(),"    _zmlUidMask.close(self)\n");
    s.insert(s.find(tail),"\n    _zmlUidMask.bind(self)");
    s.insert(s.find(cls),"local _zmlUidMask = (function()\n"+std::string(helper)+"\nend)()\n");
    out=std::move(s);return true;
}
}
