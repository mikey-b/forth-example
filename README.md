
# Forth Example
 
Code example to compile a small subset of Forth code. 
Compiles to Windows x64 binaries

```
g++ -O2 -std=c++20 *.cpp -o forth
```
(Makefile also provided)

Read <input> and produces a binary at <output>
```
./forth.exe <input> <output>
```
Read <input> and produce asssembly to stdout
```
./forth.exe <input> -
```

Example demonstrates
* If
* Loop
* Word Definition and Calling
* Local Variables
* \+ \- \* \/ \% Operators
* \. Print Integer