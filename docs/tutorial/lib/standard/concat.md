# XVR Standard Library Documentation

## Standard

Standard library in xvr providing set of built-in method for array (list) types, an array is ordered, mutable collection of elements of the same type, denoted as `[T]` where `T` are the element type (e.g, `[int]`, `[string]`, `[float]`, etc).

### `concat`

Return new array which is containing all element from original array, following by all elements from provided second array. It does not modify either of the original arrays

```
var newArray: [T] = originalArray.concat(otherArray);
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `otherArray` | `[T]` | The array whose elements will be appended to the end of the result. The type `T` must be the same as the type of the original array. |

**Return Value**
- type: `[T]`

### Example 

```
var data: [int] = [1, 2, 3];
var secondData: [int] = [4, 5, 6];

// concatenate 'secondData' to `data` and store the result in
// thirdData
var thirdData: [int] = data.concat(secondData);
```
since `concat` return new array, you can chaining multiple call together

```
var otherData: [int] = data.concat(secondData).concat(secondData);

// output:
// [1, 2, 3, 4, 5, 6, 4, 5, 6]
```

> [!NOTE]
> immutability: `concat` method are non-destructive , it does not alter the `originalArray` or the `otherArray`. it always produces new array in memory

## Working on dictionary

Return new dictionary containing all the key-value pairs from the original dictionary, merged with all the key-value pairs from a provided second dictionary. If key exists in both dictionaries, the value from the `otherDictionary` will overwrite the value from the original dictionary in the new, resulting dictionary

```
var newDictionary: [K:V] = originalDictionary.concat(otherDictionary);
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `otherDict` | `[K:V]` | The dictionary whose key-value pairs will be merged into the result. The key `K` and value `V` types must match the original dictionary. |

**Return Value**
- Type: `[K:V]`

```
var firstDictionary: [string : int] = ["first": 1, "second": 2];
var secondDictionary: [string : int] = ["third": 3, "fourth": 4];

var thirdDictionary: [string : int] = firstDictionary.concat(secondDictionary);
```

> [!NOTE]
> immutability: `concat` method is non-destructive for dictionaries. It does not alther the `originalDictionary` or the `otherDictionary`. It always produces a new dictionaries.

## Working with strings

`concat` method for string return a new strings that is the result of joining the original string with a provided second string. It is the primery way to perform explicit string concatenation in XVR.

```
var newString: string = originalString.concat(stringToAppend);
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `stringToAppend` | `string` | The string to append to the end of the `originalString`. |

**Return value**
- Type: `string`

```
var firstName: string = "xaviera ";
var lastName: string = "putri";
var fullName: string = firstName.concat(lastName);
```

> [!NOTE]
> immutability: `concat` method is non-destructive for strings. It does not alter the `originalString`. It always produces new string in memory.


## Combined with procedure

Standard also includes functional-style methods for transforming collections. The most common of these is `map`. `map` method create new collection by appplying a transformation function to every element of the original collection. It is available for both arrats and dictionaries

```
var newArray: [U] = originalArray.map(transformProcedure);
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `transformFunction` | `proc(T): U` | A procedure that takes an element from the array (of type `T`) and returns a new value (of type `U`). |

```
// procedure that takes a value and returns it icremented
// by 1
proc increment(k, v) {
    return v + 1;
}

var data: [int] = [1, 2, 3];

// First map: [1, 2, 3] -> [2, 3, 4]
// Second map: [2, 3, 4] -> [3, 4, 5]
// Third map: [3, 4, 5] -> [4, 5, 6]
var result: [int] = data.map(increment).map(increment).map(increment);
```

```
proc increment(k, v) {
    return v + 1;
}

var dataDictionary: [string : int] = ["four": 4, "five": 5, "six": 6];

// First map: {"four":4, "five":5, "six":6} -> {"four":5, "five":6, "six":7}
// Second map: {"four":5, "five":6, "six":7} -> {"four":6, "five":7, "six":8}
// Third map: {"four":6, "five":7, "six":8} -> {"four":7, "five":8, "six":9}
var result: [string : int] = dataDictionary.map(increment).map(increment).map(increment);
```
