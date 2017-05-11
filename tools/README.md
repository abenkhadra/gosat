
## Building a library ##
It's possible to instruct goSAT to generate C files required to build a dynamic library (`libgofuncs`)
using code generated from several SMT formulas. This library can be provided as input to our `bh_solver`
and `nl_solver` utilities. 

For example, in ordered to reproduce (most) results published in XSat. First, download and unzip `griggio` benchmarks formulas which are available [online]. Then
move to `griggio/fmcad12` folder.

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
(number of variables). Now, you can compile the library by issuing the following

```shell
gcc -Wall -O2 -shared -o libgofuncs.so -fPIC gofuncs.c
```

You can find our utilities `bh_solver` and `nl_solver` in this folder. 
Assuming that you have python3 with packages `numpy` and `scipy` installed, you 
can try issuing the following command

```shell
python3 ~/artifact/gosat/extras/bh_solver.py $[path-to-library-and-api-file]
```

Alternatively, you can check out `nl_solver` utility by building it from source. Building `nl_solver` can be
done using cmake.
