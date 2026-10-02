#include "managed.hpp"
#include <cstring>
#include <stdexcept>
namespace uid_mask {
bool Managed::connect() {
    module = GetModuleHandleW(L"GameAssembly.dll");
    if (!module) return false;
#define EXPORT(member, name) member=reinterpret_cast<decltype(member)>(GetProcAddress(module,"il2cpp_" name)); if(!member)return false
    EXPORT(domain_get,"domain_get"); EXPORT(domain_get_assemblies,"domain_get_assemblies");
    EXPORT(assembly_get_image,"assembly_get_image"); EXPORT(image_get_name,"image_get_name");
    EXPORT(class_from_name,"class_from_name"); EXPORT(class_get_methods,"class_get_methods");
    EXPORT(method_get_name,"method_get_name"); EXPORT(method_get_param_count,"method_get_param_count");
    EXPORT(method_get_param,"method_get_param"); EXPORT(method_get_return_type,"method_get_return_type");
    EXPORT(method_get_flags,"method_get_flags"); EXPORT(type_get_name,"type_get_name");
    EXPORT(free_memory,"free"); EXPORT(runtime_invoke,"runtime_invoke");
    EXPORT(class_get_field_from_name,"class_get_field_from_name"); EXPORT(field_get_type,"field_get_type");
    EXPORT(field_get_value,"field_get_value"); EXPORT(string_length,"string_length");
    EXPORT(field_set_value,"field_set_value"); EXPORT(class_from_type,"class_from_type");
    EXPORT(class_is_valuetype,"class_is_valuetype"); EXPORT(class_value_size,"class_value_size");
    EXPORT(value_box,"value_box"); EXPORT(object_unbox,"object_unbox");
    EXPORT(array_length,"array_length"); EXPORT(array_object_header_size,"array_object_header_size");
    EXPORT(array_element_size,"array_element_size"); EXPORT(array_new,"array_new");
    EXPORT(string_chars,"string_chars"); EXPORT(string_new_utf16,"string_new_utf16");
    EXPORT(gchandle_new,"gchandle_new"); EXPORT(gchandle_free,"gchandle_free");
#undef EXPORT
    return domain_get()!=nullptr;
}
std::string Managed::type_name(const void* type) {
    char* p=type_get_name(type); if(!p)return {};
    std::string out(p); free_memory(p); return out;
}
void* Managed::klass(const char* image,const char* ns,const char* name) {
    size_t count{};auto assemblies=domain_get_assemblies(domain_get(),&count);
    if(!assemblies || count>4096)return nullptr;
    void* found{};
    for(size_t i=0;i<count;++i) {
        auto img=assembly_get_image(assemblies[i]);
        if(img && std::strcmp(image_get_name(img),image)==0) {
            auto k=class_from_name(img,ns,name);
            if(k){if(found)return nullptr;found=k;}
        }
    }
    return found;
}
Method Managed::method(void* cls,const char* name,const char* result,
                       std::initializer_list<const char*> params,bool is_static) {
    if(!cls)return {};
    Method found{}; unsigned hits{};void* it{};
    while(auto m=class_get_methods(cls,&it)) {
        if(std::strcmp(method_get_name(m),name)!=0 || method_get_param_count(m)!=params.size() ||
           type_name(method_get_return_type(m))!=result)continue;
        unsigned impl{};
        if(((method_get_flags(m,&impl)&0x10)!=0)!=is_static)continue;
        unsigned i{};bool ok=true;
        for(auto p:params)ok &= type_name(method_get_param(m,i++))==p;
        if(!ok)continue;
        ++hits;void* code{};std::memcpy(&code,m,sizeof(code));
        MEMORY_BASIC_INFORMATION mbi{};
        if(code && VirtualQuery(code,&mbi,sizeof(mbi)) && mbi.AllocationBase==module &&
           (mbi.Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))found={m,code};
    }
    return hits==1?found:Method{};
}
const void* Managed::field(void* cls,const char* name,const char* type) {
    auto f=cls?class_get_field_from_name(cls,name):nullptr;
    return f && type_name(field_get_type(f))==type?f:nullptr;
}
void* Managed::get(void* object,const void* f) {
    void* value{};if(object && f)field_get_value(object,f,&value);return value;
}
void* Managed::call(Method m,void* object) {
    if(!m.info)throw std::runtime_error("metadata contract");
    void* error{};void* value=runtime_invoke(m.info,object,nullptr,&error);
    if(error)throw std::runtime_error("managed call failed");return value;
}
std::wstring Managed::string(void* value) {
    if(!value)return {};
    int n=string_length(value);
    if(n<0 || n>65536)throw std::runtime_error("text bound");
    return {string_chars(value),static_cast<size_t>(n)};
}
void* Managed::string(std::wstring_view value) {
    return string_new_utf16(value.data(),static_cast<int>(value.size()));
}
unsigned Managed::root(void* value) {
    auto h=value?gchandle_new(value,false):0;
    if(value && !h)throw std::runtime_error("root failed");return h;
}
void Managed::unroot(unsigned handle) { if(handle)gchandle_free(handle); }
bool Managed::buffer_contract(void* tmp) {
    bufferField=tmp?class_get_field_from_name(tmp,"m_TextBackingArray"):nullptr;
    if(!bufferField)return false;
    auto type=field_get_type(bufferField);auto name=type_name(type);
    if(name!="TMPro.TMP_Text.TextBackingContainer" && name!="TMPro.TMP_Text/TextBackingContainer")return false;
    bufferClass=class_from_type(type);
    if(!bufferClass || !class_is_valuetype(bufferClass))return false;
    arrayField=field(bufferClass,"m_Array","System.UInt32[]");
    countField=field(bufferClass,"m_Count","System.Int32");
    unsigned alignment{};bufferSize=class_value_size(bufferClass,&alignment);
    arrayHeader=array_object_header_size();
    auto arrayClass=arrayField?class_from_type(field_get_type(arrayField)):nullptr;
    uintClass=klass("mscorlib.dll","System","UInt32");
    return arrayField && countField && uintClass && arrayClass && array_element_size(arrayClass)==4 &&
        bufferSize>0 && bufferSize<=128 && alignment>0 && alignment<=16 &&
        arrayHeader>0 && arrayHeader<=128 && arrayHeader%alignof(uint32_t)==0;
}
void Managed::BufferSwap::protect(void* p) {
    if(!p)return;
    auto h=api.gchandle_new(p,true);if(!h)throw std::runtime_error("render buffer root failed");
    try{roots.push_back(h);}catch(...){api.unroot(h);throw;}
}
Managed::BufferSwap::BufferSwap(Managed& a,void* self):api(a),object(self) {
    try {
        if(!object || !api.bufferField)throw std::runtime_error("render buffer unavailable");
        protect(object);original.resize(api.bufferSize);
        api.field_get_value(object,api.bufferField,original.data());
        box=api.value_box(api.bufferClass,original.data());
        if(!box)throw std::runtime_error("render buffer box failed");protect(box);
        void* array{};int count{};
        api.field_get_value(box,api.arrayField,&array);
        api.field_get_value(box,api.countField,&count);
        if(count<0 || count>65536 || (!array && count!=0))throw std::runtime_error("render buffer count invalid");
        if(!array)return;
        auto capacity=api.array_length(array);
        if(capacity<static_cast<unsigned>(count) || capacity>16*1024*1024)throw std::runtime_error("render buffer bounds invalid");
        protect(array);
        auto data=reinterpret_cast<const char32_t*>(static_cast<const unsigned char*>(array)+api.arrayHeader);
        text.assign(data,data+count);
    }catch(...){for(auto h:roots)api.unroot(h);roots.clear();throw;}
}
void Managed::BufferSwap::replace(std::u32string_view value) {
    if(value.size()>131072)throw std::runtime_error("render replacement too large");
    auto array=api.array_new(api.uintClass,value.size()+1);
    if(!array)throw std::runtime_error("render array allocation failed");protect(array);
    auto data=reinterpret_cast<char32_t*>(static_cast<unsigned char*>(array)+api.arrayHeader);
    std::memcpy(data,value.data(),value.size()*sizeof(char32_t));data[value.size()]=0;
    auto count=static_cast<int>(value.size());
    // Pass managed object directly for reference field setter
    api.field_set_value(box,api.arrayField,array);
    api.field_set_value(box,api.countField,&count);
    void* storedArray{};int storedCount{};
    api.field_get_value(box,api.arrayField,&storedArray);
    api.field_get_value(box,api.countField,&storedCount);
    if(storedArray!=array || storedCount!=count || api.array_length(array)!=value.size()+1 ||
       std::memcmp(data,value.data(),value.size()*sizeof(char32_t))!=0 || data[value.size()]!=0)
        throw std::runtime_error("render replacement readback failed");
    auto valueBytes=api.object_unbox(box);
    if(!valueBytes)throw std::runtime_error("render box unavailable");
    api.field_set_value(object,api.bufferField,valueBytes);
    swapped=true;
    std::vector<unsigned char> readback(api.bufferSize);
    api.field_get_value(object,api.bufferField,readback.data());
    if(std::memcmp(readback.data(),valueBytes,api.bufferSize)!=0)
        throw std::runtime_error("render container readback failed");
}
Managed::BufferSwap::~BufferSwap() {
    if(swapped)api.field_set_value(object,api.bufferField,original.data());
    for(auto h:roots)api.unroot(h);
}
}
