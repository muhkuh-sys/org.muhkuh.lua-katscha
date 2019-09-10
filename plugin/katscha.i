%module katscha

%include <stdint.i>

#ifdef SWIGLUA
%include "lua_fnptr.i"
#endif

/* Include the header file in the Lua wrapper.
 */
%header %{
	#include "katscha.h"
%}

%include muhkuh_typemaps.i

/* import interfaces */
%include "katscha.h"
