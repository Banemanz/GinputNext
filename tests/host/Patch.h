#pragma once
#include <array>
#include <map>
#include <cstring>
#include <cstdint>
namespace plugin::patch {
inline std::map<std::uintptr_t,std::array<unsigned char,5>> code;
inline void GetRaw(std::uintptr_t p,void*d,std::size_t n,bool){std::memcpy(d,code[p].data(),n);}
inline void SetRaw(std::uintptr_t p,void*d,std::size_t n,bool){std::memcpy(code[p].data(),d,n);}
inline void RedirectCall(std::uintptr_t p,void*t,bool){auto&b=code[p];b[0]=0xE8;
 auto rel=static_cast<std::int32_t>(reinterpret_cast<std::uintptr_t>(t)-p-5);std::memcpy(b.data()+1,&rel,4);}
}
