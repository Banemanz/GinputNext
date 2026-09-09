// Test-only portability adapters. Production MSVC project does not include this.
#include <string>
#include <cstring>
#include <cwchar>
namespace std {
template<> struct char_traits<unsigned char> {
 using char_type=unsigned char; using int_type=unsigned int;
 using off_type=streamoff; using pos_type=streampos; using state_type=mbstate_t;
 static void assign(char_type&a,const char_type&b){a=b;}
 static bool eq(char_type a,char_type b){return a==b;}
 static bool lt(char_type a,char_type b){return a<b;}
 static int compare(const char_type*a,const char_type*b,size_t n){return memcmp(a,b,n);}
 static size_t length(const char_type*s){size_t n=0;while(s[n])++n;return n;}
 static const char_type* find(const char_type*s,size_t n,const char_type&c){return static_cast<const char_type*>(memchr(s,c,n));}
 static char_type* move(char_type*a,const char_type*b,size_t n){return static_cast<char_type*>(memmove(a,b,n));}
 static char_type* copy(char_type*a,const char_type*b,size_t n){return static_cast<char_type*>(memcpy(a,b,n));}
 static char_type* assign(char_type*a,size_t n,char_type c){return static_cast<char_type*>(memset(a,c,n));}
 static constexpr int_type not_eof(int_type c){return c==eof()?0:c;}
 static constexpr char_type to_char_type(int_type c){return static_cast<char_type>(c);}
 static constexpr int_type to_int_type(char_type c){return c;}
 static constexpr bool eq_int_type(int_type a,int_type b){return a==b;}
 static constexpr int_type eof(){return static_cast<int_type>(-1);}
};
}
#include "PluginBase.h"
// The III metadata casts to a function type, an MSVC extension.
#undef META_BEGIN_OVERLOADED
#define META_BEGIN_OVERLOADED(func, decl) template<> struct meta<static_cast<std::conditional_t<std::is_function_v<decl>,std::add_pointer_t<decl>,decl>>(&func)> {
