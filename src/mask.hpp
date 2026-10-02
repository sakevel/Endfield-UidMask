#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
namespace uid_mask {
inline bool decimal(std::wstring_view value) {
    return !value.empty() && value.size() <= 20 &&
        value.find_first_not_of(L"0123456789") == value.npos;
}
inline bool digit(wchar_t c) { return c >= L'0' && c <= L'9'; }
template<class C> inline bool digit_unit(C c) { return c >= C('0') && c <= C('9'); }
template<class C>
inline std::basic_string<C> mask_units(std::basic_string_view<C> text,
                                      std::basic_string_view<C> own,
                                      std::basic_string_view<C> alias) {
    if(own.empty() || alias.empty() || own==alias || text.size()>65536)return std::basic_string<C>(text);
    std::basic_string<C> result;result.reserve(text.size());bool tag=false;
    for(size_t i=0;i<text.size();) {
        if(text[i]==C('<'))tag=true;
        bool match=!tag && text.substr(i,own.size())==own &&
            (i==0 || !digit_unit(text[i-1])) &&
            (i+own.size()==text.size() || !digit_unit(text[i+own.size()]));
        if(match){result.append(alias);i+=own.size();}
        else {if(text[i]==C('>'))tag=false;result+=text[i++];}
    }
    return result;
}
// Skip rich-text tag attributes and non-matching numeric tokens.
inline std::wstring mask(std::wstring_view text, std::wstring_view own, std::wstring_view alias) {
    if (!decimal(own) || !decimal(alias) || own == alias || text.size() > 65536) return std::wstring(text);
    return mask_units<wchar_t>(text,own,alias);
}
inline std::u32string scalar_text(std::wstring_view s) {
    std::u32string out;
    for(size_t i=0;i<s.size();++i) {
        char32_t c=static_cast<unsigned short>(s[i]);
        if(c>=0xd800 && c<=0xdbff && i+1<s.size() && s[i+1]>=0xdc00 && s[i+1]<=0xdfff) {
            c=0x10000+((c-0xd800)<<10)+(s[++i]-0xdc00);
        }
        out+=c;
    }
    return out;
}
inline bool plain_utf8(std::string_view s,std::u32string& out) {
    if(s.empty() || s.size()>96)return false;
    std::u32string next;
    for(size_t i=0;i<s.size();) {
        auto b=static_cast<unsigned char>(s[i++]);char32_t c=b;unsigned n=0;
        if(b>=0xc2 && b<=0xdf){c=b&31;n=1;}
        else if(b>=0xe0 && b<=0xef){c=b&15;n=2;}
        else if(b>=0xf0 && b<=0xf4){c=b&7;n=3;}
        else if(b>=128)return false;
        if(i+n>s.size())return false;
        for(unsigned j=0;j<n;++j){auto d=static_cast<unsigned char>(s[i++]);if((d&0xc0)!=0x80)return false;c=(c<<6)|(d&63);}
        if((n==1 && c<128)||(n==2 && c<2048)||(n==3 && c<65536)||c>0x10ffff||
           (c>=0xd800 && c<=0xdfff)||c<32||c==127||c==U'<'||c==U'>')return false;
        next+=c;
    }
    out=std::move(next);return true;
}
struct Config {
    bool enabled = true; std::wstring alias = L"1000000000";
    bool maskName=false, maskShort=false;
    std::u32string aliasName=U"管理员", aliasShort=U"0000";
};
struct Identity { std::u32string uid,name,shortId; };
// Build replacement span plan
inline std::u32string render_identity(std::u32string_view text,const Identity& own,const Config& cfg) {
    if(text.size()>65536)return std::u32string(text);
    // Fast path for standard numeric UID masking
    if(!cfg.maskName && !cfg.maskShort) {
        return cfg.enabled ? mask_units<char32_t>(text,own.uid,std::u32string(cfg.alias.begin(),cfg.alias.end())) : std::u32string(text);
    }
    struct Unit {size_t begin,end;}; std::vector<Unit> map;std::u32string visible;
    for(size_t i=0;i<text.size();) {
        if(text[i]==U'<') {auto end=text.find(U'>',i);if(end==text.npos)break;i=end+1;continue;}
        size_t begin=i;char32_t c=text[i++];
        if(c>=0xd800 && c<=0xdbff && i<text.size() && text[i]>=0xdc00 && text[i]<=0xdfff)
            c=0x10000+((c-0xd800)<<10)+(text[i++]-0xdc00);
        visible+=c;map.push_back({begin,i});
    }
    struct Edit {size_t begin,end;std::u32string alias;};std::vector<Edit> edits;
    auto add=[&](size_t start,size_t length,const std::u32string& alias){
        if(!length)return;
        // Preserve style tags within text spans
        for(size_t j=0;j<length;++j)edits.push_back({map[start+j].begin,map[start+j].end,j==0?alias:U""});
    };
    auto space=[](char32_t c){return c==U' '||c==U'\t'||c==U'\n'||c==U'\r';};
    auto word=[](char32_t c){return c>=128 || (c>=U'a'&&c<=U'z') || (c>=U'A'&&c<=U'Z') || digit_unit(c) || c==U'_';};
    bool paired=false;
    if(!own.name.empty() && !own.shortId.empty() && (cfg.maskName||cfg.maskShort)) {
        for(size_t at=0;(at=visible.find(own.name,at))!=visible.npos;) {
            auto start=at;at+=own.name.size();
            if(start && word(visible[start-1]))continue;
            size_t hash=at;while(hash<visible.size() && space(visible[hash]))++hash;
            if(hash>=visible.size() || visible[hash]!=U'#')continue;
            size_t digits=hash+1, end=digits+own.shortId.size();
            if(visible.substr(digits,own.shortId.size())!=own.shortId || (end<visible.size() && word(visible[end])))continue;
            paired=true;
            if(cfg.maskName)add(start,own.name.size(),cfg.aliasName);
            if(cfg.maskShort)add(digits,own.shortId.size(),cfg.aliasShort);
            at=end;
        }
    }
    // Match player name on full visible label
    if(cfg.maskName && !paired && !own.name.empty()) {
        size_t first=0,last=visible.size();while(first<last && space(visible[first]))++first;
        while(last>first && space(visible[last-1]))--last;
        if(visible.substr(first,last-first)==own.name)add(first,last-first,cfg.aliasName);
    }
    if(cfg.enabled && !own.uid.empty()) {
        bool tag=false;
        for(size_t i=0;i<text.size();) {
            if(text[i]==U'<')tag=true;
            auto end=i+own.uid.size();
            bool match=!tag && text.substr(i,own.uid.size())==own.uid &&
                (!i || !digit_unit(text[i-1])) && (end==text.size() || !digit_unit(text[end]));
            if(match) {
                bool overlaps=false;for(auto& e:edits)overlaps|=e.begin<end && e.end>i;
                if(!overlaps)edits.push_back({i,end,std::u32string(cfg.alias.begin(),cfg.alias.end())});
                i=end;
            }else{if(text[i]==U'>')tag=false;++i;}
        }
    }
    std::sort(edits.begin(),edits.end(),[](auto& a,auto& b){return a.begin<b.begin;});
    std::u32string out;size_t at=0;
    for(auto& e:edits){if(e.begin<at)continue;out.append(text.substr(at,e.begin-at));out+=e.alias;at=e.end;}
    out.append(text.substr(at));return out;
}
// Parse flat key=value configuration
inline bool parse_config(std::string_view bytes, Config& config) {
    Config next;
    bool enabled = false, alias = false;
    std::vector<std::string_view> seen;
    if (bytes.size() > 65536 || bytes.find('\0') != bytes.npos) return false;
    while (!bytes.empty()) {
        auto end = bytes.find('\n');
        auto line = bytes.substr(0, end);
        if (end == bytes.npos) bytes = {}; else bytes.remove_prefix(end+1);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.empty() || line.front() == ';' || line.front() == '#') continue;
        if (line.front() == '[') return false;
        auto pos = line.find('=');
        if (pos == line.npos) return false;
        auto key = line.substr(0,pos), value = line.substr(pos+1);
        if(std::find(seen.begin(),seen.end(),key)!=seen.end())return false;
        seen.push_back(key);
        if (key == "enabled") {
            if (enabled || (value != "true" && value != "false")) return false;
            enabled = true; next.enabled = value == "true";
        } else if (key == "alias_uid") {
            if (alias) return false;
            alias = true;
            next.alias.assign(value.begin(),value.end());
            if (!decimal(next.alias)) return false;
        } else if(key=="mask_name" || key=="mask_short_id") {
            if(value!="true" && value!="false")return false;
            (key=="mask_name"?next.maskName:next.maskShort)=value=="true";
        } else if(key=="alias_name") {
            if(!plain_utf8(value,next.aliasName))return false;
        } else if(key=="alias_short_id") {
            std::wstring number(value.begin(),value.end());
            if(!decimal(number))return false;
            next.aliasShort.assign(value.begin(),value.end());
        }
    }
    if (!enabled || !alias) return false;
    config = std::move(next); return true;
}
}
