## Source organization ##
  * **branches/**:
    * **stable/**: stable version of aprof, check out this!
    * **external\_input**: experimental version of aprof with support for external input (e.g., disk I/O)
  * **trunk/**: development branch
    * **aprofplot/**: a java GUI for displaying aprof logs
    * **benchmark/**: some tools/code used for aprof's benchmarks
    * **valgrind/**: [snapshot of valgrind code](http://valgrind.org/)
      * **aprof/**: source code of the development version of aprof
      * **tf/**: source code of TF, _toy_ tool implementing basic logic of callgrind for tracing fuunctions, very limited (no multithreading, no signal handling, only correct under `x86/x86_64`), very inefficient
  * **tags/vg1**: first implementation of aprof under Valgrind, traces functions only through GCC instrumentation (`-finstrument-functions`), deprecated