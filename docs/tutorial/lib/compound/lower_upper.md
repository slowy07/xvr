# XVR Standard Library Documentation

## Compound

String Library in XVR provides set of built-in methods for string manipulation and case conversion

## `toLower()`
Return new string with all alphabetic character converted to their lowercase sequivalent. Non-alphabetic character (like  number, symbols and whitespace) are not affected. This method does not modify the original string. It can be called on string variables or directly on string literals

```
var lowerString: string = originalString.toLower();
```

**Return value**
- Type: `string`

```
var name: string = "arfy slowy";
var toLowerName: string = name.toLower();
```

## `toUpper()`
Return new string with all alphabetic character converted to their uppercase equivalent. Non-alphabetic character (like  number, symbols and whitespace) are not affected. This method does not modify the original string. It can be called on string variables or directly on string literals

```
var upperString: string = originalString.toUpper();
```

**Return value**
- Type: `string`
    
```
var name: string = "arfy slowy";
var toUpperName: string = name.toUpper();
```

> [!NOTE]
> Common use case: these methods are extremely useful for perfom case insesitive string compare by convert both string to the same case before compare them (e.g `firstString.toLower() == secondString.toLower()`)
