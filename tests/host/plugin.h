#pragma once
#include <cstdint>
namespace plugin {
inline std::uintptr_t GetGlobalAddress(std::uintptr_t p){return p;}
template<class R,std::uintptr_t A,class...Args> R CallMethodAndReturn(Args...){return static_cast<R>(77);}
}
