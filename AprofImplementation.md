<h2>Table of contents</h2>

## Instrumentation ##
We need these _events_:
  * memory read & write
  * thread start, exit and currently running
  * function entry/exit point

Our first implementation of aprof was based on PIN (that provides the first two types of _events_). Function entry/exit points is predicted through a stack simulation and SP synchronization when there is a branch. For different reasons, aprof under PIN was not very fast, so we switched to Valgrind.

Valgrind, similarly to PIN, does not provide by default function entry/exit point events, so again we have to implement a predict logic. This time we study a very mature tool of Valgrind, called `Callgrind`, that already does this. Its approach is a bit more sophisticated than what we do with PIN. Unfortunately this mechanism is not reliable on ARM/PPC architecture (in this you can use aprof but you have to recompile your program, more info [here](AprofOnARM_PPC.md)).
Memory accesses are instrumentated with the `standard` approach under valgrind (see tool `lackey` or `main.c` of `callgrind` tool).

### Details about function entry/exit point prediction ###
### Approximations of memory accesses ###
## Metric ##
  * Count basic blocks:
  * Count guest instructions:
  * Timestamp through RDSTC:

## Data Structures ##
### Memory access logic ###
#### `[`PIN`]` Union find: disjoint-set linked list ####
#### `[`VG:SUF1`]` Union find: disjoint-set forests ####
#### `[`VG:SUF2`]` Stack union find ####
### Other ###
  * `Function`: ...
  * `Object`: ...
  * `ThreadData`: ...
  * `RoutineInfo`: ...
  * `SMSInfo`: ...
  * `Activation`: ...
  * `CCT`: ...