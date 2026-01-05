# magnetic-vm
Experimental AOT-compiling JVM using LLVM.

This is an old incomplete project of mine from 2022; it is no longer actively
developed.

## Building

GCC or Clang is recommended. I have not tested MSVC; proceed at your own risk.

Required libraries:
- LLVM (tested with 18.1.4)
- CJBP (C++ Java Bytecode Parser) - use [this old version](https://github.com/lunbun/cjbp-old)
- [fmt](https://fmt.dev/12.0/) (tested with 12.0)
- [kuba-zip](https://github.com/kuba--/zip)

A `conanfile.txt` is provided for installing `fmt` and `kuba-zip`, but you can
also install them manually.

Build with CMake:
```bash
cd magnetic-vm
mkdir build
cd build
cmake -DCJBP_DIR=/path/to/cjbp/build ..
make
```

## Usage

At the moment, magnetic-vm has no command-line interface. You will need to
modify `compiler/src/main.cc` to specify which Java .class files to compile.

I do not really recommend using this for anything serious. It is missing a lot
of key JVM features like garbage collection and the `invokedynamic` opcode.
