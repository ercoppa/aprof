## aprof on ARM/PPC platforms ##

aprof uses the same approach as callgrind for predicting/tracing function entry/exit points, so if you are on an ARM/PPC platform, aprof does not work correctly.

Anyway, you can still use aprof combined with [GCC instrumentation](http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Code-Gen-Options.html#index-finstrument_002dfunctions-1976). To do so, you need to recompile your program or any libs you want to correctly profile. Any function not instrumented by GCC is ignored by aprof. This hybrid approach uses source-code instrumentation done by GCC for tracing routine calls/returns and binary-code instrumentation done by Valgrind for tracing memory accesses and collecting performance profiles.

## Instrument your code with GCC ##
Check out from our svn the code invoked by GCC when a function starts or returns:
```
svn co http://aprof.googlecode.com/svn/trunk/gcc-instrument/ gcc-instrument
```

Now you need to recompile your program (or any libs of your interest):
```
gcc -Wall your_code.c gcc-instrument/instr-gcc.c -finstrument-functions -o your_prog
```
Of course if your program/lib uses a makefile, you have to modify it in a similarly way.

## Disabling internal function tracing ##

In order to disable the internal function tracing of aprof, you need
to edit in `/path_to_aprof/valgrind/aprof/aprof.h` the value of `TRACE_FUNCTION` from `1` to `0`. Then you need to recompile aprof.