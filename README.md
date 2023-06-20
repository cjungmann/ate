# PROJECT ARRAY TABLE EXTENSION

Turn a Bash array into a quick-access virtual table.

## INTRODUCTION

### Bash Arrays are Slow

Random access to Bash array elements is very inefficient.  Asking
for the value of `myarray[100]` sends Bash on a walk through the
previous 99 elements to find element 100.  Changing `myarray[100]`
prompts another search.  The performance penalty is trivial for
small arrays, but unmistakable for large arrays.

### EXTENDING BASH ARRAYS

`ate` generates a direct-access internal array alongside the
linked-list contents of the Bash array.  Typically, a developer
would define a row size, and the index would reference each
element that anchors a table row.  However, one could also
create a row-size of 1 to index every array element.






## ALTERNATE NAMES

I've considered `bat` for **Bash Array Table**, but decided against
competing with [bat][bat].

Using `ate` for now, but alternative names (in case `ate` seems
inappropriate) might be `bia` for **Bash Indexed Array** or `iat` for
**Indexed Array Table**.






[bat]:  "https://github.com/sharkdp/bat"