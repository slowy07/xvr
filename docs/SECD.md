## SECD = state, environment, control, dump

```
DEFINE = DECLARE + SET
```

>> [!NOTE]
> altough the environment, representing an E, is formed at routine start and destroyed at routine conclusion, closure function as intended since it starts with the environment of the parent procedure

```
READ
    read on value from C onto S

LOAD
    read one value from .data onto S

ASSERT
    if S(-1) is falsy, print S(0) and exit

PRINT
    pop S(0), and print the output

SET
    read one word from C, saves the key E[word] to the value S(0), popping S(0)

GET
    read one word from C, finds the value of E[word], leaves the value on S

DECLARE
    read two word from C, create a new entry in E with the key E[word1], the type defined by word2, the value `null`

DEFINE
    read two words from C, create a new entry in E with the key E[word1], the type defined by word2, the value popped from S(0)
```
