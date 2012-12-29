EXPERIMENTAL EVALUATION OF DYNAMIC ALL PAIRS SHORTEST PATH ALGORITHMS
Camil Demetrescu, Stefano Emiliozzi, and Giuseppe F. Italiano

---------------------------------------------------------------------
INTRODUCTION:

This package (dsp-1.0) is intended as an electronic companion of the paper: 
Experimental Evaluation of Dynamic All Pairs Shortest Path Algorithms, by 
Camil Demetrescu, Stefano Emiliozzi, and Giuseppe F. Italiano, Technical 
Report ALCOM-FT (available at http://www.dis.uniroma1.it/~demetres/ 
experim/dsp/).  The package includes:

* C implementations of several static and dynamic algorithms for 
single-source and all-pairs shortest path problems (include/, src/, 
ll/include/, ll/scr/core/common/)

* Generators of random and hard inputs (src/main.c and 
ll/scr/core/common/LGraphGen.c)

* Real test sets in Dimacs and XML format, including: 
   - Internet networks obtained from data collected in the University of 
     Oregon Route Views Archive Project (data/real/internet/)
   - US road networks obtained from data gathered in the U.S. Geological 
     Survey Digital Line Graphs (data/real/2M/Dimacs/ and data/real/2M/XML/)

* An interactive driver program for running the implemented algorithms 
(src/main.c)

* Scripts for running the experiments described in the paper (scripts/)

* HTML documentation of all the implemented components (docs/ and ll/docs/)

The software package is implemented in C and is based on the Leonardo 
Library (http://www.leonardo-vm.org).

---------------------------------------------------------------------
HOW TO BUILD THE CODE:

* Using gcc. Type "make" in the dsp directory. The executable code 
will be generated in bin/.

* Using a different compiler.  Use the files in the following directories:

    - .h: include/ and ll/include/
    - .c: src/, ll/src/core/common/, and ll/src/core/posix/

---------------------------------------------------------------------
HOW TO RUN THE CODE:

The driver program (bin/dsp, if compiled using make) can be used either 
interactively, or in a batch mode.

* Interactive mode. The program shows the following main menu:

-- MAIN MENU.
g. Make initial graph [current: none]
s. Make update sequence [current: none]
l. Make insertion/deletion sequence
k. Make structured test sets
t. Setup
a. Run CAPSP_C   (S-DIJ  - Dijkstra n times)
b. Run LDSP      (D-PUP  - potentially uniform paths)
c. Run CDAPSP_DE (D-RRL  - Ramalingam/Reps variant)
d. Run CDAPSP    (D-KIN  - King)
e. Run CDAPSP_D  (D-KIND - King up to D)
f. Run LSP       (S-UP   - uniform paths)
q. Quit

To run the codes, you must first generate an input graph and an input 
update sequence.  To generate the graph, type 'g':

-- GRAPH INIT MENU.
r. Generate random graph
l. Load graph in XML format
d. Load graph in DIMACS format
m. Back to main menu

With this option, you can load a graph from a file, or generate a random 
graph on the fly. To generate the input sequence, go to the main menu and 
type 's'. Other ways to generate inputs are possible using options 'l' and 
'k'.

To run a code, choose the desired option ('a' to 'f').  The experimental 
data will be stored automatically for each code in a file (in bin/) of the form: 
code-name_exp-name.res.  By default, exp-name is empty.  To change the name 
of the experiment or configure other parameters of the implementations, go 
to the main menu and type 't':

-- SETUP MENU.
c. Toggle checker status [current status: OFF]
v. Toggle verbose mode status [current status: OFF]
s. Toggle sequence adjust mode status [current status: OFF]
e. Enter experiment name [current name: ""]
l. Config LDSP (current: T= 0.7 | C= 0.6 | S=100 | R=10)
d. Config CDAPSP (current: ALPHA=0.0000 | BETA=1.0000)
m. Back to main menu

Here we remark that if the checker status is ON ('c' option) then the 
solution produced by each algorithm after each update will be compared with 
the correct solution computed from scratch using S-DIJ (Dijkstra's 
algorithm).  In this case, the time measurements will also include the 
checking time.  We also note that if the sequence adjust mode is ON, then 
at the time of making an update sequence, the edge weights interval will be 
automatically derived from the current input graph.  Observe that options 
'l and 'd' can be used to configure the relevant parameters of algorithms 
D-PUP (LDSP) and D-KIN (CDAPSP), respectively.

* Batch mode.  It is possible to run the driver program under the control 
of a batch script.  Scripts for reproducing the experiments used in the 
preparation of the paper are located in the script/ directory.  To run them 
(in UNIX-like environments) go to bin/ and type, e.g.:

./dsp < ../scripts/fig1.txt

In this way, the driver program will be given as standard input the content 
of the script file fig1.txt, which contains the operations required to run 
the experiment we used to generate Figure 1 of the paper.  The results will 
be written for each code to files of the form bin/code-name_fig1.res.  To 
generate Figure 8, we used the Cachegrind tool 
(http://developer.kde.org/~sewardj), which requires a more complex setup, 
and thus no script file could be made available for it.  Notice that Figure 
4 can be derived from data in Figure 6.

---------------------------------------------------------------------
LICENSE:
The package is distributed under the GNU Lesser General Public License: 
http://www.gnu.org/licenses/lgpl.html

---------------------------------------------------------------------
CONTACT: 
Please send feedback, comments, and questions to: 
<feedback@leonardo-vm.org>

---------------------------------------------------------------------
Camil Demetrescu
July 2003

