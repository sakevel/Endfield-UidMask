#pragma once
#include <string>
#include <string_view>
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
// Presentation only: skip rich-text tag attributes and longer number tokens.
inline std::wstring mask(std::wstring_view text, std::wstring_view own, std::wstring_view alias) {
    if (!decimal(own) || !decimal(alias) || own == alias || text.size() > 65536) return std::wstring(text);
    return mask_units<wchar_t>(text,own,alias);
}
struct Config { bool enabled = true; std::wstring alias = L"1000000000"; };
// Read our own loader-persisted flat key=value, not schema or account data.
inline bool parse_config(std::string_view bytes, Config& config) {
    Config next;
    bool enabled = false, alias = false;
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
        if (key == "enabled") {
            if (enabled || (value != "true" && value != "false")) return false;
            enabled = true; next.enabled = value == "true";
        } else if (key == "alias_uid") {
            if (alias) return false;
            alias = true;
            next.alias.assign(value.begin(),value.end());
            if (!decimal(next.alias)) return false;
        }
    }
    if (!enabled || !alias) return false;
    config = std::move(next); return true;
}
}
