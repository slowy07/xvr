![image_banner](.github/images/banner_xvr.png)

## Feature

- intermediate bytecode interpretation

> [!NOTE]
> For more information about project you can check the [docs](docs) for documentation about xvr, and you can check the sample code from [code](code) directory

## Build Instruction

> [!NOTE]
> For Windows using (mingw32 & Cygwin), For linux or Unix already support compiler

Build interpreter
```sh
# make the interpreter
make inter
# the compilation output
# can check on the /out directory
# including external library
```

```
make tests-cases
make inter
```

## Say wello with Xvr
```xvr
proc say_hello(name: string): string {
    return "wello " + name;
}
```

## Print with Format Specifiers
```xvr
print("Hello %s", "World");           // Hello World
print("Number: %d", 42);               // Number: 42
print("Float: %f", 3.14);             // Float: 3.14
print("%s is %d years old", "arfy", 25);  // arfy is 25 years old
```

Supported specifiers: `%s` (string/any), `%d`/`%i` (integer), `%f`/`%g` (float), `%%` (literal %), `\n` (newline)

## Need Tutorial?

You can check on [tutorial](docs/tutorial) for explore some tutorials.

## Side project XvrLang

- Neovim Syntax highlighting for XvrLang: [xvrlang-treesitter](https://github.com/WargaSlowy/xvrlang-treesitter)
- Vscode Syntax highlighting for XvrLang: [xvrlang-vscode](https://github.com/WargaSlowy/xvrlang-vscode)
