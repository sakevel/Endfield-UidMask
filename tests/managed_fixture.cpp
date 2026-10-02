// Synthetic IL2CPP + TMP buffers. Fast ingestion bypass reproduces the string-hook gap.
#include <Windows.h>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#define API extern "C" __declspec(dllexport)
namespace {
struct String {std::wstring value;};
struct Array {uint64_t padding[4];uint64_t length;}; // artificial 40-byte header
struct Buffer {Array* array{};int count{};unsigned flag=0xABCDEF12;};
struct Label {uint64_t padding[3]{};Buffer buffer;};Label label;
struct Box {uint64_t padding[3]{};Buffer value;};
std::vector<std::unique_ptr<String>> strings;
std::vector<std::unique_ptr<unsigned char[]>> arrays;
std::vector<std::unique_ptr<Box>> boxes;
void* str(std::wstring s){auto p=std::make_unique<String>();p->value=std::move(s);auto raw=p.get();strings.push_back(std::move(p));return raw;}
Array* make_array(size_t n){
    auto p=std::make_unique<unsigned char[]>(sizeof(Array)+sizeof(char32_t)*n);
    std::memset(p.get(),0,sizeof(Array)+sizeof(char32_t)*n);
    auto a=reinterpret_cast<Array*>(p.get());a->length=n;arrays.push_back(std::move(p));return a;
}
char32_t* data(Array* p){return reinterpret_cast<char32_t*>(reinterpret_cast<unsigned char*>(p)+sizeof(Array));}
void fill(Label* self,std::wstring_view text){
    self->buffer.array=make_array(text.size()+1);self->buffer.count=static_cast<int>(text.size());
    for(size_t i=0;i<text.size();++i)data(self->buffer.array)[i]=text[i];
}
std::wstring decode(Buffer b){std::wstring s;
    for(int i=0;i<b.count;++i){auto c=data(b.array)[i];if(c<=0xffff)s+=static_cast<wchar_t>(c);
        else {c-=0x10000;s+=static_cast<wchar_t>(0xd800+(c>>10));s+=static_cast<wchar_t>(0xdc00+(c&1023));}}
    return s;
}
void* source{};std::wstring rendered,rawBuffer;std::wstring identity=L"1000123456";
int game,player,info,tmp,bufferClass,arrayClass,uintClass;unsigned roots{},invalidReferenceWrites{};int mode{};
int identityField,bufferField,arrayField,countField;
__declspec(noinline) void* get_player(const void*){return &player;}
__declspec(noinline) void* get_role(void*,const void*){return str(identity);}
__declspec(noinline) void* get_text(void*,const void*){return source;}
__declspec(noinline) void backing(void* self,void* s,const void*){fill(static_cast<Label*>(self),s?static_cast<String*>(s)->value:L"");}
__declspec(noinline) void slice(void* self,void* s,int i,int n,const void*){fill(static_cast<Label*>(self),s?static_cast<String*>(s)->value.substr(i,n):L"");}
__declspec(noinline) void processing(void* self,const void*){
    rendered=decode(static_cast<Label*>(self)->buffer);
    if(mode==3)throw std::runtime_error("synthetic original parse exception");
}
struct Method {void* code;const char* name;const char* result;std::vector<const char*> params;bool stat;};
Method methods[]{
    {reinterpret_cast<void*>(&get_player),"get_player","Beyond.Gameplay.GamePlayer",{},true},
    {reinterpret_cast<void*>(&get_role),"get_roleId","System.String",{},false},
    {reinterpret_cast<void*>(&get_text),"get_textForPopulate","System.String",{},false},
    {reinterpret_cast<void*>(&backing),"PopulateTextBackingArray","System.Void",{"System.String"},false},
    {reinterpret_cast<void*>(&slice),"PopulateTextBackingArray","System.Void",{"System.String","System.Int32","System.Int32"},false},
    {reinterpret_cast<void*>(&processing),"PopulateTextProcessingArray","System.Void",{},false},
};
struct Image {const char* name;};Image images[]{ {"Gameplay.Beyond.dll"},{"Unity.TextMeshPro.dll"},{"mscorlib.dll"} };
const void* assemblies[]{&images[0],&images[1],&images[2]};
}
API void* il2cpp_domain_get(){return &game;}
API const void** il2cpp_domain_get_assemblies(void*,size_t* n){*n=3;return assemblies;}
API const void* il2cpp_assembly_get_image(const void* a){return a;}
API const char* il2cpp_image_get_name(const void* a){return static_cast<const Image*>(a)->name;}
API void* il2cpp_class_from_name(const void* img,const char* ns,const char* name){
    if(img==&images[0] && std::strcmp(ns,"Beyond.Gameplay")==0){
        if(std::strcmp(name,"GameInstance")==0)return &game;
        if(std::strcmp(name,"GamePlayer")==0)return &player;
        if(std::strcmp(name,"PlayerInfoSystem")==0)return &info;
    }
    if(img==&images[1] && std::strcmp(ns,"TMPro")==0 && std::strcmp(name,"TMP_Text")==0)return &tmp;
    if(img==&images[2] && std::strcmp(ns,"System")==0 && std::strcmp(name,"UInt32")==0)return &uintClass;
    return nullptr;
}
API const void* il2cpp_class_get_methods(void* cls,void** it){
    size_t i=reinterpret_cast<size_t>(*it);
    size_t first=cls==&game?0:cls==&info?1:2,count=cls==&game||cls==&info?1:cls==&tmp?4:0;
    if(i>=count)return nullptr;*it=reinterpret_cast<void*>(i+1);return &methods[first+i];
}
API const char* il2cpp_method_get_name(const void* m){return static_cast<const Method*>(m)->name;}
API unsigned il2cpp_method_get_param_count(const void* m){return static_cast<unsigned>(static_cast<const Method*>(m)->params.size());}
API const void* il2cpp_method_get_param(const void* m,unsigned i){return static_cast<const Method*>(m)->params.at(i);}
API const void* il2cpp_method_get_return_type(const void* m){
    if(mode==1 && m==&methods[5])return "System.Object";
    return static_cast<const Method*>(m)->result;
}
API unsigned il2cpp_method_get_flags(const void* m,unsigned* flags){*flags=0;return static_cast<const Method*>(m)->stat?0x10:0;}
API char* il2cpp_type_get_name(const void* t){return _strdup(static_cast<const char*>(t));}
API void il2cpp_free(void* p){std::free(p);}
API void* il2cpp_runtime_invoke(const void* m,void* self,void**,void** error){
    *error=nullptr;if(m==&methods[0])return get_player(m);if(m==&methods[1])return get_role(self,m);
    *error=&game;return nullptr;
}
API const void* il2cpp_class_get_field_from_name(void* cls,const char* name){
    if(cls==&player && std::strcmp(name,"playerInfoSystem")==0)return &identityField;
    if(cls==&tmp && std::strcmp(name,"m_TextBackingArray")==0)return &bufferField;
    if(cls==&bufferClass && std::strcmp(name,"m_Array")==0)return &arrayField;
    if(cls==&bufferClass && std::strcmp(name,"m_Count")==0)return &countField;
    return nullptr;
}
API const void* il2cpp_field_get_type(const void* f){
    if(f==&identityField)return "Beyond.Gameplay.PlayerInfoSystem";
    if(f==&bufferField)return "TMPro.TMP_Text/TextBackingContainer";
    if(f==&arrayField)return mode==2?"System.Byte[]":"System.UInt32[]";
    if(f==&countField)return "System.Int32";return "System.Object";
}
API void il2cpp_field_get_value(void* object,const void* f,void* out){
    if(f==&identityField)*static_cast<void**>(out)=identity.empty()?nullptr:&info;
    else if(f==&bufferField)std::memcpy(out,&static_cast<Label*>(object)->buffer,sizeof(Buffer));
    else if(f==&arrayField)*static_cast<Array**>(out)=static_cast<Box*>(object)->value.array;
    else if(f==&countField)*static_cast<int*>(out)=static_cast<Box*>(object)->value.count;
    else std::abort(); // contract violation fails the fixture, not a C-export exception
}
API void il2cpp_field_set_value(void* object,const void* f,void* value){
    if(f==&bufferField)std::memcpy(&static_cast<Label*>(object)->buffer,value,sizeof(Buffer));
    else if(f==&arrayField){
        // Actual IL2CPP reference setter stores value itself, NOT *(void**)value.
        // Reject unknown references safely in the fixture instead of reading
        // the old DLL's stack pointer as an array and causing undefined behavior.
        bool allocated=!value;
        for(auto& a:arrays)allocated |= a.get()==value;
        if(!allocated){++invalidReferenceWrites;return;}
        static_cast<Box*>(object)->value.array=static_cast<Array*>(value);
        if(mode==5)static_cast<Box*>(object)->value.array=nullptr; // faulty write/readback
    }
    else if(f==&countField)static_cast<Box*>(object)->value.count=*static_cast<int*>(value);
    else std::abort(); // attempting any identity write must fail the test
}
API void* il2cpp_class_from_type(const void* type){
    auto name=std::string(static_cast<const char*>(type));
    if(name=="TMPro.TMP_Text/TextBackingContainer")return &bufferClass;
    if(name=="System.UInt32[]")return &arrayClass;return nullptr;
}
API bool il2cpp_class_is_valuetype(void* cls){return cls==&bufferClass;}
API int il2cpp_class_value_size(void*,unsigned* align){*align=alignof(Buffer);return sizeof(Buffer);}
API void* il2cpp_value_box(void*,void* value){auto box=std::make_unique<Box>();std::memcpy(&box->value,value,sizeof(Buffer));auto p=box.get();boxes.push_back(std::move(box));return p;}
API void* il2cpp_object_unbox(void* object){return &static_cast<Box*>(object)->value;}
API unsigned il2cpp_array_length(void* a){return mode==4?0:static_cast<unsigned>(static_cast<Array*>(a)->length);}
API unsigned il2cpp_array_object_header_size(){return sizeof(Array);}
API int il2cpp_array_element_size(void*){return sizeof(char32_t);}
API void* il2cpp_array_new(void* cls,uintptr_t n){return cls==&uintClass?make_array(n):nullptr;}
API int il2cpp_string_length(void* s){return static_cast<int>(static_cast<String*>(s)->value.size());}
API const wchar_t* il2cpp_string_chars(void* s){return static_cast<String*>(s)->value.data();}
API void* il2cpp_string_new_utf16(const wchar_t* s,int n){return str({s,static_cast<size_t>(n)});}
API unsigned il2cpp_gchandle_new(void*,bool){return ++roots;}
API void il2cpp_gchandle_free(unsigned){--roots;}
API const wchar_t* FixtureRender(const wchar_t* raw,int path){
    source=str(raw);
    if(path==0){auto p=get_text(&label,&methods[2]);backing(&label,p,&methods[3]);}
    else if(path==1)backing(&label,source,&methods[3]);
    else if(path==2)slice(&label,source,2,static_cast<int>(static_cast<String*>(source)->value.size())-4,&methods[4]);
    else fill(&label,static_cast<String*>(source)->value); // char[], builder, numeric/inlined fast path
    processing(&label,&methods[5]);return rendered.c_str();
}
API const wchar_t* FixtureRaw(){return static_cast<String*>(source)->value.c_str();}
API const wchar_t* FixtureBackingRaw(){rawBuffer=decode(label.buffer);return rawBuffer.c_str();}
API unsigned FixtureBufferFlag(){return label.buffer.flag;}
API void FixtureIdentity(const wchar_t* s){identity=s;}
API unsigned FixtureRoots(){return roots;}
API unsigned FixtureInvalidReferenceWrites(){return invalidReferenceWrites;}
API void FixtureMode(int n){mode=n;}
