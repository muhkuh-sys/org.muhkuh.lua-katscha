
/* This typemap adds "SWIGTYPE_" to the name of the input parameter to
 * construct the swig typename. The parameter name must match the definition
 * in the wrapper.
 */
%typemap(in, numinputs=0) swig_type_info *
%{
	$1 = SWIGTYPE_$1_name;
%}


/* Swig 3.0.5 has no lua implementation of the cstring library. The following
 * typemaps are a subset of the library.
 */
%typemap(in) (const char *pcBUFFER_IN, size_t sizBUFFER_IN)
{
	size_t sizBuffer;
	$1 = (char*)lua_tolstring(L, $argnum, &sizBuffer);
	$2 = sizBuffer;
}

%typemap(in, numinputs=0) (const char **ppcBUFFER_OUT, size_t *psizBUFFER_OUT)
%{
	const char *pcOutputData;
	size_t sizOutputData;
	$1 = (char**)(&pcOutputData);
	$2 = &sizOutputData;
%}

/* NOTE: This "argout" typemap can only be used in combination with the above "in" typemap. */
%typemap(argout) (const char **ppcBUFFER_OUT, size_t *psizBUFFER_OUT)
%{
	if( pcOutputData!=NULL && sizOutputData!=0 )
	{
		lua_pushlstring(L, pcOutputData, sizOutputData);
	}
	else
	{
		lua_pushnil(L);
	}
	++SWIG_arg;
%}


/* This typemap expects a table as input and replaces it with the Lua state.
 * This allows the function to add elements to the table without the overhead
 * of creating and deleting a C array.
 */
%typemap(in,checkfn="lua_istable") lua_State *ptLuaStateForTableAccess
%{
	$1 = L;
%}


/* This typemap passes Lua state to the function. The function must create one
 * lua object on the stack. This is passes as the return value to lua.
 * No further checks are done!
 */
%typemap(in, numinputs=0) lua_State *MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT
%{
	$1 = L;
	++SWIG_arg;
%}


/* This typemap passes the Lua state to the function. This allows the function
 * to call functions of the Swig Runtime API and the Lua C API.
 */
%typemap(in, numinputs=0) lua_State *
%{
	$1 = L;
%}


/* This typemap converts the output of the plugin reference's "Create"
 * function from the general "muhkuh_plugin" type to the type of this
 * interface. It transfers the ownership of the object to Lua (this is the
 * last parameter in the call to "SWIG_NewPointerObj").
 */
%typemap(out) muhkuh_plugin *
%{
	SWIG_NewPointerObj(L,result,((muhkuh_plugin_reference const *)arg1)->GetTypeInfo(),1); SWIG_arg++;
%}



%typemap(in, numinputs=0) (unsigned long *pulARGUMENT_OUT)
%{
	unsigned long ulArgument_$argnum;
	$1 = &ulArgument_$argnum;
%}
%typemap(argout) (unsigned long *pulARGUMENT_OUT)
%{
	lua_pushnumber(L, ulArgument_$argnum);
	++SWIG_arg;
%}



%typemap(out) RESULT_INT_TRUE_OR_NIL_WITH_ERR
%{
	if( $1>=0 )
	{
		lua_pushboolean(L, 1);
		SWIG_arg = 1;
	}
	else
	{
		lua_pushnil(L);
		lua_pushstring(L, arg1->get_error_message());
		SWIG_arg = 2;
	}
%}



%typemap(out) RESULT_INT_NOTHING_OR_NIL_WITH_ERR
%{
	if( $1<0 )
	{
		lua_pushnil(L);
		lua_pushstring(L, arg1->get_error_message());
		SWIG_arg = 2;
	}
	else
	{
%}
%typemap(ret) RESULT_INT_NOTHING_OR_NIL_WITH_ERR
%{
	}
%}

