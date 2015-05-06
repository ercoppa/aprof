# DSP #
We run [DSP](http://www.dis.uniroma1.it/~demetres/experim/dsp/README.txt) with [this input](http://code.google.com/p/aprof/source/browse/trunk/benchmark/dsp/scripts/aprof_benchmark1.txt). We measure execution time with the _time_ command.
Under GNU/Linux, space is measured with:
```
cat /proc/PID/status | grep VmPeak
```

## Download DSP ##
```
svn co http://aprof.googlecode.com/svn/trunk/benchmark/dsp
```

## Build DSP and run under Aprof ##
```
cd aprof_valgrind
make
time ./inst/bin/valgrind --tool=aprof path/to/benchmark/dsp/bin/dsp < path/to/benchmark/dsp/scripts/aprof/qaprof_benchmark1.txt
```

## Platform ##

### P1 ###
  * Macbook 5,1
  * Intel(R) Core(TM)2 Duo CPU     P7350  @ 2.00GHz
  * 2GB DDR3
  * Ubuntu 11.04 **32bit**

### P2 ###
  * Macbook 5,1
  * Intel(R) Core(TM)2 Duo CPU     P7350  @ 2.00GHz
  * 4GB DDR3
  * Ubuntu 11.04 **64bit**

### P3 ###
  * Macbook 5,1
  * Intel(R) Core(TM)2 Duo CPU     P7350  @ 2.00GHz
  * 4GB DDR3
  * Mac OS X 10.5.8 **32bit**

### P4 ###
  * Macbook 5,1
  * Intel(R) Core(TM)2 Duo CPU     P7350  @ 2.00GHz
  * 4GB DDR3
  * Ubuntu 12.04 **64bit**

### P5 ###
  * Macbook 5,1
  * Intel(R) Core(TM)2 Duo CPU     P7350  @ 2.00GHz
  * 4GB DDR3
  * Mac OS X Lion 10.7.4

## Perfomance reference ##
```
P1 - DSP(native): space 15696 kB ~ time real 0.36 sec
P1 - DSP(callgrind-3.6.0): space 73308 kB ~  time real 16.08 sec
P1 - DSP(callgrind-3.6.0 --cache-sim=yes): space 73308 ~ time real 0m55.567s
P1 - DSP(memcheck-3.6.0): space 145236 kB ~ time real 9.85 sec
P1 - DSP(TOOL:none-3.6.0): space 59372 kB ~ time real 0m2.010s
P3 - DSP(native): space 16310 kB ~ time real 3.737s 
P2 - DSP(native): space 32040 kB ~ time real 0.424s
```
Valgrind tools are those shipped with Valgrind 3.6.1

Aprof based on [PIN](http://www.pintool.org/):
```
P1 - DSP(aprof-PIN): space 158512 kB ~ time real 3m48.569s ~ TIME_GETTIMEOFDAY
P1 - DSP(aprof-PIN): space 158548 kB ~ time real 2m5.545s ~ TIME_RDTSC
```

## Results ##
[Revision 0](https://code.google.com/p/aprof/source/detail?r=0) (old repo):
```
P1 - DSP(aprof): real 18.90 sec ~ MEM+FN+INSTR, EMPTY_ANALYSIS
P1 - DSP(aprof): real 12.20 sec ~ MEM+FN, EMPTY_ANALYSIS
```

[Revision 65](https://code.google.com/p/aprof/source/detail?r=65) (old repo):
```
P1 - DSP(aprof): real 11.38 sec ~ MEM+FN, EMPTY_ANALYSIS
P1 - DSP(aprof): real 18.81 sec ~ MEM+FN+INSTR, EMPTY_ANALYSIS
P1 - DSP(aprof): real 11.31 sec ~ MEM+FN+RDTSC, EMPTY_ANALYSIS
```

[Revision 71](https://code.google.com/p/aprof/source/detail?r=71) (old repo):
```
P1 - DSP(aprof): space 235mb ~ time real 2m8.854s ~ CCT=0, ADDR_MULTIPLE=4, TIME=RDTSC, SUF=1
```

[Revision 26](https://code.google.com/p/aprof/source/detail?r=26):
```
DSP(aprof): space 100mb ~ time real 1m33s ~ CCT=0, ADDR_MULTIPLE=4, TIME=RDTSC, SUF=1
DSP(aprof): space 51mb ~ time real 43s ~ CCT=0, ADDR_MULTIPLE=4, TIME=RDTSC, SUF=2
```

[Revision 31](https://code.google.com/p/aprof/source/detail?r=31):
```
P1 - DSP(aprof): space 36mb ~ time real 5.689s ~ EVENTCOUNT=1 (only mem access)
P1 - DSP(aprof): space 36mb ~ time real 8.128s ~ EVENTCOUNT=2 (only function), TIME=NO_TIME
P1 - DSP(aprof): space 37mb ~ time real 13.744s ~ EVENTCOUNT=3 (memory and function), TIME=NO_TIME
```

[Revision 32](https://code.google.com/p/aprof/source/detail?r=32):
```
P1 - DSP(aprof): space 127100 kB ~ time real 1m23.533s ~ CCT=0, ADDR_MULTIPLE=4, TIME=RDTSC, SUF=1
P1 - DSP(aprof): space 73852 kB ~ time real 0m37.827s ~ CCT=0, ADDR_MULTIPLE=4, TIME=RDTSC, SUF=2
P1 - DSP(aprof): space 61544 kB ~ time real 0m5.223s ~ EVENTCOUNT=1 (only mem access)
P1 - DSP(aprof): space 61564 kB ~ time real 8.200s ~ EVENTCOUNT=2 (only function), TIME=NO_TIME
P1 - DSP(aprof): space 61564 kB ~ time real 0m13.943s ~ EVENTCOUNT=3 (memory and function), TIME=NO_TIME
```

[Revision 107](https://code.google.com/p/aprof/source/detail?r=107) (branch function):
```
P1 - DSP(aprof): space 78000 kB ~ time real 0m20.138s ~ CCT=0, SUF=2, TIME=BB_COUNT, ADDR_MULTIPLE=4, COSTANT_MEM_ACCESS=1
```

[Revision 190](https://code.google.com/p/aprof/source/detail?r=190):
```
DSP(aprof): space 78000 kB ~ time real 0m19.460s ~ CCT=0, SUF=2, TIME=BB_COUNT, ADDR_MULTIPLE=4, COSTANT_MEM_ACCESS=1, IGNORE_DL_RUNTIME=1
```

[Revision 214](https://code.google.com/p/aprof/source/detail?r=214):
```
DSP(aprof): space 73904 kB ~ time real 0m19.286s ~ CCT=0, SUF=2, TIME=BB_COUNT, ADDR_MULTIPLE=4, COSTANT_MEM_ACCESS=1, IGNORE_DL_RUNTIME=1, DISCARD_UNKNOWN=1
```

[Revision 218](https://code.google.com/p/aprof/source/detail?r=218):
```
P1 - DSP(aprof): space 70664 kB ~ time real 0m19.168s ~ CCT=0, SUF=2, TIME=BB_COUNT, ADDR_MULTIPLE=4, COSTANT_MEM_ACCESS=1, IGNORE_DL_RUNTIME=1, DISCARD_UNKNOWN=1
```

[Revision 317](https://code.google.com/p/aprof/source/detail?r=317):
```
P2 - DSP(aprof): space 132036 kB ~ time real 0m17.548s ~ CCT=0, SUF=2, TIME=BB_COUNT, ADDR_MULTIPLE=4, COSTANT_MEM_ACCESS=1, IGNORE_DL_RUNTIME=1, DISCARD_UNKNOWN=1
P3 - DSP(aprof): space 54208 kB ~ time real 0m37.513s ~ CCT=0, SUF=2, TIME=BB_COUNT, ADDR_MULTIPLE=4, COSTANT_MEM_ACCESS=1, IGNORE_DL_RUNTIME=1, DISCARD_UNKNOWN=1
```

[Revision 396](https://code.google.com/p/aprof/source/detail?r=396):
```
P4 - DSP(aprof): space 110848 kB ~ time real 0m17.151s ~ CCT=0, SUF=2, TIME=BB_COUNT, ADDR_MULTIPLE=4, COSTANT_MEM_ACCESS=1, IGNORE_DL_RUNTIME=1, DISCARD_UNKNOWN=1
P5 - DSP(aprof): space 76360 kB ~ time real 0m42.567s ~ CCT=0, SUF=2, TIME=BB_COUNT, ADDR_MULTIPLE=4, COSTANT_MEM_ACCESS=1, IGNORE_DL_RUNTIME=1, DISCARD_UNKNOWN=1
```