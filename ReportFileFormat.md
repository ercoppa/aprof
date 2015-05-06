## Report file format (vs. 1.4) ##

Report file data are specified one item per line. Each line starts with a tag:


---

**v: report version number**

Syntax: `v version`, where `version` is the version number of the file report.

If the version line is missing, the default version number is zero.


---

**e: executable file modification date**

Syntax: `e mtime`, where `mtime` is the modification time of the profiled executable file.


---

**t: report creation date**

Syntax: `t date-time`, where `date-time` is the creation date and time of the report.


---

**c: comment line**

Syntax: `c comment`, where `comment` is any text fragment.


---

**f: command line of the analized program**

Syntax: `f command-line`


---

**a: executable file name of the analized program**

Syntax: `a application-name`


---

**m: performance metric**

Syntax: `m metric`, where `metric` is the metric type of performance measurements (program costs). It can be any of the following:

  * `bb-count`: count of executed basic blocks;
  * `time-usec`: elapsed time in microseconds.

If missing, the default metric is `bb-count`.


---

**k: total program cost**

Syntax: `k total-program-cost`, where `total-program-cost` is the total cost (64-bit unsigned integer) required by the execution of the program.


---

**r: routine info**

Syntax: `r "routine-name" "image-name" routine-id`, where:

  * `"routine-name"`: string specifying the (possibly demangled) name of the routine
  * `"image-name"`: string specifying the pathname of the image containing the routine
  * `routine-id`: distinct numerical id associated to the routine (32 bit unsigned integer)

Example:

`r "mmap" "/lib/i386-linux-gnu/ld-2.13.so" 30`


---


**u: routine mangled name**

Syntax: `u routine-id "mangled-name"`, where:

  * `routine-id`: id of the routine (32 bit unsigned integer)
  * `mangled-name`: string specifying mangled name of the routine
<a href='Hidden comment: 
----
* d: routine full demangled name*

Syntax: d routine-id "full-demangled-name", where:

* routine-id: id of the routine (32 bit unsigned integer)
* "full-demangled-name": string specifying full demangled name of the routine, including function arguments

Example:

d 431 "ogdf::VariableEmbeddingInserter::doCall(ogdf::PlanRep&, ogdf::List<ogdf::EdgeElement*> const&, bool, ogdf::EdgeArray<int> const*, ogdf::EdgeArray<bool> const*, ogdf::EdgeArray<unsigned int> const*)"
'></a>

---

**p: routine performance point**

Syntax: `p routine-id rms min max sum sqr-sum occ real-sum self-sum self-min self-max self-sqr`, where:

  * `routine-id`: id of the routine (32 bit unsigned integer)
  * `rms`: read memory size of the point (32 bit unsigned integer)
  * `min`: minimum routine cost on the given rms (64 bit unsigned integer)
  * `max`: maximum routine cost on the given rms (64 bit unsigned integer)
  * `sum`: sum of routine cumulative costs on the given rms (64 bit unsigned integer)
  * `sqr-sum`: sum of squares of routine cumulative costs on the given rms (64 bit unsigned integer)
  * `occ`: number of routine calls on the given rms (64 bit unsigned integer)
  * `real-sum`: _real_ sum of routine cumulative costs on the given rms (cost of an activation is ignored if there is an active ancestor in the call stack with the same function name)(64 bit unsigned integer)
  * `self-sum`: sum of routine self costs on the given rms (64 bit unsigned integer)
  * `self-min`: minimum routine self cost on the given rms (64 bit unsigned integer)
  * `self-max`:  maximum routine self cost on the given rms (64 bit unsigned integer)
  * `self-sqr`: sum of squares of routine self costs on the given rms (64 bit unsigned integer)

Example:

`p 7 23 15 37 270 3000 10 199 200 10 10 3000`

means that the routine with id 7 was called 10 times on rms 23, requiring a minimum of 15 cost units, a maximum of 37 cost units, a total cost of 270 units, a total real cost of 199, a total self cost of 200 units, a minimum self of 10 cost units, a maximum self of 10 cost units and sum of squares 3000 (both cumulative and self).


---

**x: distinct calling context info**

Each `x`-line corresponds to a node of the calling context tree (CCT).

Syntax: `x routine-id context-id parent-context-id`, where:

  * `routine-id`: routine id (32 bit unsigned integer)
  * `context-id`: unique id of CCT node (32 bit unsigned integer)
  * `parent-context-id`: id of parent context in the CCT, or -1 if the context has no parent (node is the root of the CCT).

Example:

`x 24 352 350`

means that node 352 of the CCT is a context of routine 24 and its parent node is 350.


---

**q: calling context performance point**

Syntax: `q context-id rms min max sum sqr-sum occ real-sum self-sum self-min self-max self-sqr`, where:

  * `context-id`: id of the calling context (32 bit unsigned integer)
  * `rms`: read memory size of the point (32 bit unsigned integer)
  * `min`: minimum context cost on the given rms (64 bit unsigned integer)
  * `max`: maximum context cost on the given rms (64 bit unsigned integer)
  * `sum`: sum of context costs on the given rms (64 bit unsigned integer)+
  * `sqr-sum`: sum of squares of routine costs on the given rms (64 bit unsigned integer)
  * `occ`: number of context activations on the given rms (64 bit unsigned integer)
  * `real-sum`: _real_ sum of routine cumulative costs on the given rms (cost of an activation is ignored if there is an active ancestor in the call stack with the same function name)(64 bit unsigned integer)
  * `self-sum`: sum of routine self costs on the given rms (64 bit unsigned integer)
  * `self-min`: minimum routine self cost on the given rms (64 bit unsigned integer)
  * `self-max`:  maximum routine self cost on the given rms (64 bit unsigned integer)
  * `self-sqr`: sum of squares of routine self costs on the given rms (64 bit unsigned integer)

Example:

`q 624 23 15 37 270 3000 10 199 200 10 10 3000`

means that the calling context 624 was activated 10 times on rms 23, requiring a minimum of 15 cost units, a maximum of 37 cost units, a total cost of 270 units, a total real cost of 199, a total self cost of 200 units, a minimum self of 10 cost units, a maximum self of 10 cost units and sum of squares 3000 (both cumulative and self).