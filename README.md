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
make test
make inter
```

## Say wello with Xvr
```xvr
proc say_hello(name: string): string {
    return "wello " + name;
}
```

```xvr
import compound;

proc even(k, v) {
    return v % 2 == 0;
}

var data: [int] = [1, 2, 3, 4, 5, 6, 7, 8];
print(data.filter(even));
```

## Need Tutorial?

You can check on [tutorial](docs/tutorial) for explore some tutorials.

## Side project XvrLang

- Neovim Syntax highlighting for XvrLang: [xvrlang-treesitter](https://github.com/WargaSlowy/xvrlang-treesitter)
- Vscode Syntax highlighting for XvrLang: [xvrlang-vscode](https://github.com/WargaSlowy/xvrlang-vscode)
