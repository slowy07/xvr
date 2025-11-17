# Chapter 1

## Installation

Currently Xvr can be installation from release zip and manual installation or latest interpreter can be clone on github by

**requirements**
- Operating System: `Linux`, `Windows` or `MacOS`
- Tools
    - `Windows`: CygWin with `gcc` compiler or just `gcc` compiler.
    - `Linux`, `MacOS`: `gcc` compiler, you can worry about that, its Currently available.

```sh
git clone https://github.com/slowy07/xvr
# go to directory
cd xvr
# make interpreter
make interpreter

# the interpreter application was installed on `out` folder with shared library xvr

# for install access to globally you can do by
# installed on bin/
sudo make install
```

## Hello world

first step to learning, we create some xvr files , with extension `.xvr` 

```
// hello_world.xvr

print("hello world");

// or using like this
print "hello world";
```

> [!NOTE]
>  to write output xvr using `print` builtin function to stdout, `print` support with or without parentheses (round bracket)

```sh
# using --file or -f with filename
# xvr -f | --file [filename.xvr]
xvr --file hello_world.xvr
```
```
hello world
```

and congratulations, you are programmer
