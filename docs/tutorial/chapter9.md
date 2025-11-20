## Chapter 9

Congratulations we Finish!. You are coding with XvrLang, so, you need deep dive about this lang?. You can check on [code](../../code/), next we can adding more tutorial.

## Explore more about XvrLang

### Scope

Scope are isolation area that only some variable and statement can only execute. In Xvr scope are defining using `{...}`

```
{
    // first scope
    var numbers: int = 20;
    var names: string = "wello xvr";
}

// you cannot access numbers here, this will be thrown are error
print(numbers);

{
    // second scope
    // can use a same name cause are other
    // area of scope
    var numbers: int = 12;
    var names: string = "jane doe";
}

// same as before, this will be thrown are error
print(names);
```

### Shebang

this are special line at very beginning of script file in Unix-like operating system. Which is tell the system which the interpreter should be using to execute the Xvr script

```
#!/usr/bin/xvr

print("wello");
```

### Custom procedure for working with variables

```
// make custom procedure to work with variables
proc _increment(self) {
    return self + 1;
}

var number: int = 1;

print(number.increment());
```

```
// make custom procedure to work with variables
proc _toString(self) {
    return string self;
}

var number: int = 20;

print("my number are: " + number.toString() + " looks great boys");
```

this will be telling the system to running the script using xvr interpreter apps located at /bin/xvr

<!-- TODO: update more tutorials explore, some 3rd library -->
