# `aprof`: an input-sensitive perfomance profiler #

`aprof` is a [Valgrind](http://valgrind.org/) tool. You can find a more [general user manual](http://valgrind.org/docs/manual/manual.html) about this binary instrumentation framework on its official site.

## Overview ##

Input-Sensitive profiling is a novel performance profiling methodology for helping developers discover hidden asymptotic inefficiencies in the code. It has been described in a [PLDI 2012 paper](http://code.google.com/p/aprof/#Research_papers), where you can find a more detailed description about this technique.

From one or more runs of a program, `aprof` automatically measures how the performance of individual routines scales as a function of the input size, yielding clues to their growth rate and to the ["big-O"](http://xlinux.nist.gov/dads/HTML/bigOnotation.html) of the program. The output of the profiler is, for each executed routine of the program, a set of tuples that aggregate performance costs by input size. A distinguishing feature of this approach is the ability to automatically measure the size of the input given to a generic routine. In classical analysis of algorithms based on theoretical cost models, the input size of a procedure is a parameter known a _priori_ and clear from the abstract description of the underlying algorithm. But in practice, when a routine is invoked in the context of a real application, the size of its input is unknown to the analysis tool.

### Input size estimation: Read Memory Size (RMS) ###

In order to estimate the input size of a routine invocation at runtime, `aprof` uses a new effective metric called **Read Memory Size** (**RMS**):

> The read memory size (RMS) of the execution of a routine `f` is the number of distinct memory cells first accessed by `f`, or by a descendant of `f` in the call tree, with a read operation.

The main idea behind the definition of RMS is that cells that are accessed by a function for the first time with a read operation contain the input values of the routine. Conversely, if a cell is first written and then read by the routine, the read value is not part of the input as it was determined by the routine itself.

To better understand this metric, let's discuss some examples.

#### Example 1 ####

In order to compute the RMS metric you need to see your code as a trace of program operations, including routine activations (`call`), routine completions (`return`), and read/write memory accesses (`read` and `write`). For example, consider this trace:
```
 1. call f
 2.  read x
 3.  write y 
 4.  call g
 5.    read x
 6.    read y
 7.    read z
 8.    write w
 9.    return
10.  read w
11.  return
```
Function `g` performs three first-read operations (lines 5 – 7) and its RMS is thus 3. Function `f` performs five read operations, three of which through its subroutine `g`. However, its RMS is only 2: the read operations at line 5, line 6, and line 10 are not first-reads with respect to `f`. Indeed, `x` has been already read at line 2 and `y` and `w` are written at lines 3 and 8, respectively, before being read. Hence, only the read operations at lines 2 and 7 contribute to the computation of the read memory size of function `f`.

A better explanation (with an animation) of this example is provided inside our [PLDI slides](http://aprof.googlecode.com/files/aprof-PLDI.pdf).

#### Example 2 ####

Let's now discuss an example with real C code:
```
void swap(int * a, int * b) {
    int temp = *a; 
    *a = *b; 
    *b = temp; 
}
```

This function takes as arguments two pointers to integers and simply swaps their values by using a temporary variable. Any call of `swap` has RMS equal to 4, which accounts for the addresses of lvalues a, b, `*`a and `*`b. Notice that each of them is first accessed by a read operation: a and `*`a at line 2, b and `*`b at line 3. Variable temp is not counted in the RMS as it is first accessed by the write operation at line 2.

In the next example we will understand why an estimation of the input size can be really useful.

#### Example 3 ####

```
int count_zero(int v[], int n) {
    int i, count = 0;
    for (i=0; i<n; i++) count += (v[i]==0 ? 1:0);
    return count;
}
```

The function, which counts the number of 0’s in the input array v, has RMS `n + 2`: it reads variables `v` and `n`, and the first `n` cells of the array pointed to by `v`. `aprof` collect one performance tuple for
each distinct value of `n` on which count zero is called during the execution of the program.

Suppose that `count_zero` requires `n` seconds to be executed and your application calls `count_zero` three times. For example:

```
int sum = 0;
int v[1024];
sum += count_zero(v, 256); // count zeroes first 256 int of v[]
sum += count_zero(v, 512); // count zeroes first 512 int of v[]
sum += count_zero(v, 1024); // count zeroes first 1024 int of v[]
```
So, the first call requires 256 seconds, the second one 512 and the third one 1024 seconds. If we now profile our application with a traditional profiler and with `aprof` we get:

| | **traditional profiler** | **`aprof`** |
|:|:-------------------------|:------------|
| `count_zero` | calls: 3 <br> total exec time: 1792 secs <br> average exec time: 597.3 secs <table><thead><th> calls: 3 <br> total exec time: 1792 secs <br> average exec time: 597.3 secs <br> execution<code>[</code>1<code>]</code>: input size 258, time 256 secs <br> execution<code>[</code>2<code>]</code>: input size 514, time 512 secs <br> execution<code>[</code>3<code>]</code>: input size 1026, time 1024 secs </th></thead><tbody></tbody></table>

The info provided by <code>aprof</code> is a lot more precise: for example you can easily draw a chart and evaluate the trend growth rate (a linear trend as expected). A traditional profiler can only give you aggregate info because it does not know the input size of a routine invocation and therefore it treats all the routine calls as executed on the same unknown input size.<br>
<br>
<table><thead><th> <b>traditional profiler</b> </th><th> <b><code>aprof</code></b> </th></thead><tbody>
<tr><td> <img src='http://aprof.googlecode.com/svn/trunk/wiki_images/example-2a.png' /> </td><td> <img src='http://aprof.googlecode.com/svn/trunk/wiki_images/example-2b.png' /> </td></tr></tbody></table>

<h4>Example 4</h4>

The RMS metric is really powerful. Suppose your <code>count_zero</code> code is:<br>
<pre><code>int count_zero(int v[], int n) {<br>
    if (n&lt;1) return 0;<br>
    return (v[n-1]==0 ? 1:0) + count_zero(v, n-1);<br>
}<br>
</code></pre>
This is a recursive implementation of the previous iterative <code>count_zero</code>. Observe that, just like the iterative version, the first invocation of function <code>count_zero</code> has RMS <code>n</code><sub>1</sub><code> = n + 2</code>. However, calling <code>count_zero</code> on parameter <code>n</code> also results in <code>n</code> additional recursive activations of the function for all size values ranging from <code>n − 1</code> down to <code>0</code>. Therefore, <code>aprof</code> collect <code>n + 1</code> performance<br>
tuples from just one starting activation. The read memory size corresponding to the i-th invocation is <code>n</code><sub>i</sub> = <code>n − i + 3</code>, for each i ∈ [1, n + 1].<br>
<br>
The result of recursion is that from just this code:<br>
<pre><code>int sum = 0;<br>
int v[1024];<br>
sum += count_zero(v, 1024);<br>
</code></pre>
on this example <code>aprof</code> gives us all the information needed to draw a chart with 1024 points and thus allows us to perform an accurate evaluation of the trend growth rate for <code>count_zero</code>.<br>
<br>
<h3>Performance Metric</h3>

<code>aprof</code> counts <a href='http://en.wikipedia.org/wiki/Basic_block'>basic blocks</a> as performance measure: we observed that, compared to running time measurements, this adds a light burden to the analysis time overhead, and improves accuracy in characterizing asymptotic behavior even on small workloads. The choice of counting basic blocks rather than measuring time for studying asymptotic trends has several other advantages:<br>
<ul><li><b>accuracy</b>: block counts are exact and therefore issues of insufficient timer resolution do not apply;<br>
</li><li><b>repeatability</b>: if a program is deterministic, so its measure; this measurement metric does not depend on the operating system or architecture if the program’s control flow does not;<br>
</li><li><b>lack of bias</b>: the mechanism of measurement does not affect its result; in contrast, the mechanism of measuring time distorts its own results;<br>
</li><li><b>low overhead</b>: counting basic block execution is cheap because we already need to instrument every basic block in order to trace function calls and returns.</li></ul>

<h2>Usage</h2>

See the <a href='BasicUsage#Profile_a_program.md'>basic usage page</a>

<h2>aprof command-line options</h2>

<code>--memory-resolution=&lt;k&gt; [default: 4]</code>

<blockquote>To reduce the space needed by the shadow memory, <code>aprof</code> allows users to configure the resolution of distinct observable memory objects, trading space for accuracy. This can potentially impact the number of distinct RMS values observed by <code>aprof</code>, and therefore the number of collected performance tuples. We denote by k the size in bytes of the smallest observable objects, which we assume to be aligned to addresses multiple of k. For k = 1, we have the finest resolution, shadowing the addresses of all accessed individual memory bytes. For k = 2, we trace accesses to 2-bytes words aligned at 16-bit boundaries, halving the universe of timestamps. The larger k, the smaller the RMS accuracy for routines working on small objects (e.g., strings of characters) and the smaller the size of the shadow memory.<br>
Possible valid values are: 1, 2, 4, 8, 16.</blockquote>

<code>--log-dir=&lt;PATH&gt; [default: cwd]</code>

<blockquote>Reports will be saved in this directory.</blockquote>

<code>--drms=no|yes [default: no]</code>

<blockquote>Use as input metric the Dynamic Read Memory Size (DRMS).</blockquote>

<code>--collect-cct=no|yes [default: no]</code>

<blockquote>Collect calling contect trees.</blockquote>

<code>--single-log=no|yes [default: no]</code>

<blockquote>Output a single report for all threads.</blockquote>

<code>--incremental-log=no|yes [default: no]</code>

<blockquote>Load previous reports (from cwd or log dir) and add performance tuples of the current run. This implies --single-log option.</blockquote>

<a href='Hidden comment: 
== aprof specific client requests ==

In the stable version of aprof, client requests are used only for external function entry/exit point tracing. See [AprofOnARM_PPC this page] for more details. Two client requests are parsed by aprof:

# VALGRIND_DO_CLIENT_REQUEST(<dummy_var>, 0, VG_USERREQ_TOOL_BASE("V", "A"), <function_identifier>, <function_status>, 0, 0, 0);
# VALGRIND_DO_CLIENT_REQUEST(<dummy_var>, 0, VG_USERREQ_TOOL_BASE("V", "A"), <function_identifier>, <function_status>, 0, 0, 0);

where:
* <dummy_var> is an int variable (not used)
* <function_identifier> is an identifier of the function (e.g., the routine address)
* <function_status>: one if entry point; two if exit point
'></a><br>
<br>
<h2><code>aprof-helper</code></h2>

<code>aprof-helper</code> can be easily used for merging reports and for checking their consistency. Basic usage:<br>
<br>
<pre><code>aprof-X.Y.Z/inst/bin/aprof-helper &lt;action&gt; [&lt;action&gt;] [&lt;options&gt;]<br>
</code></pre>

Valid actions are:<br>
<br>
<code>-k</code>
<blockquote>Check consistency of one or more reports.</blockquote>

<code>-r</code>
<blockquote>Merge reports of different program's runs (different PID). If no -t option is specified then this will not merge reports of different threads.</blockquote>

<code>-t</code>

<blockquote>Merge reports of different threads of the same program's run (same PID).</blockquote>

<code>-i</code>

<blockquote>Merge reports (no criteria).</blockquote>

Other options:<br>
<br>
<code>-d &lt;PATH&gt; [default: working directory]</code>
<blockquote>Report's directory.</blockquote>

<code>-a &lt;PATH&gt;</code>
<blockquote>Perform an action on a specific report.</blockquote>

Some usage examples:<br>
<br>
<ul><li>Merge reports with same PID (different threads of the same run) stored in directory <code>logs</code>:<br>
<pre><code>aprof-X.Y.Z/inst/bin/aprof-helper -t -d logs<br>
</code></pre></li></ul>

<ul><li>Merge reports of different runs of the same program stored in directory <code>logs</code>:<br>
<pre><code>aprof-X.Y.Z/inst/bin/aprof-helper -r -d logs<br>
</code></pre>
<blockquote>Different threads will continue to have distinct reports (add -t option otherwise).</blockquote></li></ul>

<ul><li>Check consistency report <code>prog_129_0_4.aprof</code>:<br>
<pre><code>aprof-X.Y.Z/inst/bin/aprof-helper -k -a prog_129_0_4.aprof<br>
</code></pre></li></ul>

<h2>Report file format</h2>

See the <a href='ReportFileFormat.md'>report file format page</a>.