#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <initializer_list>
#include <memory>
#include <cstdint>
namespace uid_mask {
struct Method { const void* info{}; void* code{}; };
// Export-based IL2CPP introspection.
class Managed {
    HMODULE module{};
    void* (*domain_get)(){};
    const void** (*domain_get_assemblies)(void*,size_t*){};
    const void* (*assembly_get_image)(const void*){};
    const char* (*image_get_name)(const void*){};
    void* (*class_from_name)(const void*,const char*,const char*){};
    const void* (*class_get_methods)(void*,void**){};
    const char* (*method_get_name)(const void*){};
    unsigned (*method_get_param_count)(const void*){};
    const void* (*method_get_param)(const void*,unsigned){};
    const void* (*method_get_return_type)(const void*){};
    unsigned (*method_get_flags)(const void*,unsigned*){};
    char* (*type_get_name)(const void*){};
    void (*free_memory)(void*){};
    void* (*runtime_invoke)(const void*,void*,void**,void**){};
    const void* (*class_get_field_from_name)(void*,const char*){};
    const void* (*field_get_type)(const void*){};
    void (*field_get_value)(void*,const void*,void*){};
    void (*field_set_value)(void*,const void*,void*){};
    void* (*class_from_type)(const void*){};
    bool (*class_is_valuetype)(void*){};
    int (*class_value_size)(void*,unsigned*){};
    void* (*value_box)(void*,void*){};
    void* (*object_unbox)(void*){};
    unsigned (*array_length)(void*){};
    unsigned (*array_object_header_size)(){};
    int (*array_element_size)(void*){};
    void* (*array_new)(void*,uintptr_t){};
    int (*string_length)(void*){};
    const wchar_t* (*string_chars)(void*){};
    void* (*string_new_utf16)(const wchar_t*,int){};
    unsigned (*gchandle_new)(void*,bool){};
    void (*gchandle_free)(unsigned){};
    std::string type_name(const void* type);
    const void* bufferField{};
    const void* arrayField{};
    const void* countField{};
    void* bufferClass{};
    void* uintClass{};
    int bufferSize{};
    unsigned arrayHeader{};
public:
    class BufferSwap {
        Managed& api;
        void* object{};
        void* box{};
        std::vector<unsigned> roots;
        std::vector<unsigned char> original;
        bool swapped=false;
        void protect(void* p);
    public:
        std::u32string text;
        BufferSwap(Managed& api,void* object);
        ~BufferSwap();
        BufferSwap(const BufferSwap&)=delete;
        void replace(std::u32string_view text);
    };
    bool connect();
    void* klass(const char* image,const char* ns,const char* name);
    Method method(void* klass,const char* name,const char* result,
                  std::initializer_list<const char*> params,bool is_static=false);
    const void* field(void* klass,const char* name,const char* type);
    void* get(void* object,const void* field);
    void* call(Method method,void* object=nullptr);
    std::wstring string(void* value);
    void* string(std::wstring_view value);
    unsigned root(void* value);
    void unroot(unsigned handle);
    bool buffer_contract(void* tmp);
};
}
