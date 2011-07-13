#include <stdio.h>
#include "../valgrind/include/valgrind.h"

void __cyg_profile_func_enter (void *, void *) __attribute__((no_instrument_function));
void __cyg_profile_func_exit (void *, void *) __attribute__((no_instrument_function));

void __cyg_profile_func_enter (void *this_fn, void *call_site) {
	
	#ifdef RUNNING_ON_VALGRIND
	int none;
	VALGRIND_DO_CLIENT_REQUEST(none, 0, VG_USERREQ_TOOL_BASE('V', 'A'), this_fn, 1, 0, 0, 0);	
	#endif
	
	return;

}
void __cyg_profile_func_exit  (void *this_fn, void *call_site) {
	
	#ifdef RUNNING_ON_VALGRIND
	int none;
	VALGRIND_DO_CLIENT_REQUEST(none, 0, VG_USERREQ_TOOL_BASE('V', 'A'), this_fn, 2, 0, 0, 0);
	#endif
	
	return;
	
}

/* Valgrind Wrapper */

/*
void I_WRAP_SONAME_FNNAME_ZU(NONE,__cyg_profile_func_enter)(void *this_fn, void *call_site) __attribute__((no_instrument_function));
void I_WRAP_SONAME_FNNAME_ZU(NONE,__cyg_profile_func_exit)(void *this_fn, void *call_site) __attribute__((no_instrument_function));

void I_WRAP_SONAME_FNNAME_ZU(NONE,__cyg_profile_func_enter)(void *this_fn, void *call_site) { 

	int    result;
	OrigFn fn;

	int none;
	VALGRIND_DO_CLIENT_REQUEST(none, 0, VG_USERREQ_TOOL_BASE('V', 'A'), this_fn, 1, 0, 0, 0);	

	VALGRIND_GET_ORIG_FN(fn);   
	CALL_FN_W_WW(result, fn, this_fn, call_site);

	return;
}

void I_WRAP_SONAME_FNNAME_ZU(NONE,__cyg_profile_func_exit)(void *this_fn, void *call_site) {

	int    result;
	OrigFn fn;

	int none;
	VALGRIND_DO_CLIENT_REQUEST(none, 0, VG_USERREQ_TOOL_BASE('V', 'A'), this_fn, 2, 0, 0, 0);

	VALGRIND_GET_ORIG_FN(fn);   
	CALL_FN_W_WW(result, fn, this_fn, call_site);

	return;
}

*/
