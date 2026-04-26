#define _CRT_SECURE_NO_WARNINGS
#include "../hooks.hpp"

int __cdecl sdk::hooks::vsnprintf::vsnprintf( char* dest, int text_len, const char* fmt, ... ) {   
    if (!dest || text_len <= 0)
        return 0;

    va_list args;
    va_start( args, fmt );
    int v4 = _vsnprintf( dest, text_len, fmt, args );
    va_end( args );

    if (( v4 < 0 ) || ( v4 >= text_len )) {
        v4 = text_len - 1;
        dest[ text_len - 1 ] = '\0';
    }

    return v4;
}