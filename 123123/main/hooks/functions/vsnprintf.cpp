#include "../hooks.h"

#include "../../hashing/strings/xorstr.h"

int __cdecl n_detoured_functions::vsnprintf( char* dest, int text_length, const char* fmt, ... )
{
	if (!dest || text_length <= 0)
		return 0;

	va_list args = { };
	va_start( args, fmt );
	int v4 = _vsnprintf( dest, text_length, fmt, args );
	va_end( args );

	if ( ( v4 < 0 ) || ( v4 >= text_length ) ) {
		v4                      = text_length - 1;
		dest[ text_length - 1 ] = '\0';
	}

	return v4;
}