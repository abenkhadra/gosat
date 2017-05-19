
## Building a library ##
It's possible to instruct goSAT to generate C files required to build a dynamic library (`libgofuncs`)
using code generated from several SMT formulas. This library can be provided as input to our `bh_solver`
and `nl_solver` utilities which are located in this folder. 

In ordered to reproduce (most) results published in XSat. 
First, download and unzip the `griggio` benchmarks formulas which are available [online]. 
Then move to folder `griggio/fmcad12`.

```shell
unzip QF_FP_Hierarchy.zip
cd QF_FP/griggio/fmcad12
```
Now, instruct goSAT to generate library files.

```shell
ls */*|while read file; do gosat -mode=cg -fmt=plain -f $file;done
```
If successful, three files will be generated which are `gofuncs.h`, `gofuncs.c`, and
`gofuncs.api`. The latter file simply lists pairs of function name and dimension size
(number of variables) which are required to properly use the function. 
Now, you can compile the library by issuing the following command

```shell
gcc -Wall -O2 -shared -o libgofuncs.so -fPIC gofuncs.c
```

Assuming that you have python3 with packages `numpy` and `scipy` installed, you 
can try solving the benchmarks using this command

```shell
python3 ~/artifact/gosat/extras/bh_solver.py $[path-to-library-and-api-file]
```

Alternatively, you can check out `nl_solver` utility by building it from source. 
Building `nl_solver` can be done using cmake.

  [online]: <http://www.cs.nyu.edu/~barrett/smtlib/QF_FP_Hierarchy.zip>
