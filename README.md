# headtail

```
Usage: headtail [OPTION] [FILE]...

Options:
  -n <num>   max. number of head and tail lines (default is half terminal height)
  -l         show line numbers
  -q         never output filename headers
  -h         this help

HeadTail utility (c) magnum 2021

Works similar to 'head' and 'tail' on each file but does both at once, even for
stdin (which is impossible with head/tail).
If file fits terminal height (using defaults), just output the whole file.  If
it is longer, output the head followed by "(... n lines skipped ...)" on a
separate line, then the tail.

If no file name is given, standard input is used.

Unlike 'head' and 'tail', this tools adds a final LF in case the last line
was lacking it.
```

## Build dependencies
None

## Building
Just compile it! For example like this:
```
gcc -O2 -Wall headtail.c -o headtail
```
