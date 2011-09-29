#include <stdio.h>

unsigned long num_enter, num_exit;

#if INSTRUMENT_FUNCTIONS == 1
void __attribute__((__no_instrument_function__)) __cyg_profile_func_enter(void *this_fn, void *call_site) {
//    printf("enter: %p\n", this_fn);
    num_enter++;
}

void __attribute__((__no_instrument_function__)) __cyg_profile_func_exit(void *this_fn, void *call_site) {
//    printf("exit: %p\n", this_fn);
    num_exit++;
}
#endif

