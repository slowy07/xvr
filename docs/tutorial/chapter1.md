# Chapter 1

## Installation

Currently Xvr can be installation from release zip and manual installation or latest interpreter can be clone on github by

**requirements**
- Operating System: `Linux`, `Windows` or `MacOS`
- Tools
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

## Installing the syntax highlighting

Your code does not look cool?, you can using syntax highlighter which is available for [vscode](https://github.com/WargaSlowy/xvrlang-vscode) and [neovim](https://github.com/WargaSlowy/xvrlang-treesitter)

## Hello world

first step to learning, we create some xvr files , with extension `.xvr` 

```
// hello_world.xvr

print("hello world")
```


```sh
# using --file or -f with filename
# xvr -f | --file [filename.xvr]
xvr --file hello_world.xvr
```
```
hello world
```

and congratulations, you are programmer
