# goSAT
goSAT is an SMT solver for the theory of floating-point arithmetic.  

# Overview
goSAT is an SMT solver based on mathematical global optimization. It casts the satisfiability
problem of FPA to a corresponding global optimization problem.
It's builds on the ideas proposed in [XSat]. Compared to XSat, we implemented
the following key features:

- JIT execution of SMT formulas using LLVM.
- Integration with NLopt, a features-rich mathematical optimization backend.

More details are available in our [Report] and this [Appendix]

# Dependencies

This project depends on:

- [Z3] library for input file parsing and model validation
- [LLVM] for JIT execution of SMT formulas.
- [NLopt] for finding the minima of the objective function.

This software is known to work with Z3 v4.5 and LLVM v3.9.1.

# Building

You can build the project using a command like,

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/home/gosat/local/ -DLLVM_DIR=/home/gosat/local/llvm/lib/cmake/llvm/ ..
make
```
assuming that `libnlopt` and `libz3` are installed at the prefix `${HOME}/local/`
while `llvm` is installed at `${HOME}/local/llvm`.

# Usage
An SMT file need to be provided using `-f` option. Additionally, operation mode
can be set. goSAT supports three operation modes:

 - **Native solving**. This the default mode where a given formula is first transformed
 to an objective function in LLVM IR . Then, the objective function is jitted using MCJIT
 and solved using the NLopt backend.  This mode can be explicitly chosen
 using `-mode=go` option.

 - **Code generation**. Mimics the behavior of [XSat] where C code representing the
  objective function is generated.
  Generated code needs to be compiled and solved using on of the tools we provided in `tools` folder.
  This mode can be enabled using `-mode=cg` option which makes goSAT output
  an objective function in C language. Moreover, you can specify an API dump format
  `-mode=cg -fmt=plain` for generating several objective functions. The latter is useful in
  building a library as discussed [here](tools/README.md).

 - **Formula analysis**. A simple analysis to show the number of variables, their
 types, and other misc facts about a given SMT formula. This mode is enabled
 using `-mode=fa` option.


  [Z3]: <https://github.com/Z3Prover/z3>
  [LLVM]: <http://llvm.org/>
  [online]: <http://www.cs.nyu.edu/~barrett/smtlib/QF_FP_Hierarchy.zip>
  [XSat]: <http://dx.doi.org/10.1007/978-3-319-41540-6_11>
  [NLopt]: <https://github.com/stevengj/nlopt>
  [Report]: <https://blog.formallyapplied.com/docs/gosat.pdf>
  [Appendix]: <https://blog.formallyapplied.com/posts/2017/05/gosat-faq/>
