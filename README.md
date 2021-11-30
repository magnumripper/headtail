# headtail

```
Usage: headtail [OPTION] [FILE]...

Options:
  -n  show line numbers
  -h  this help

HeadTail utility (c) magnum 2021

Works like 'head' and 'tail' on each file but does both in at once, even for
stdin (which is impossible with head/tail).
If file is at most 21 lines, print whole file.  If it is longer, print first
ten lines, followed by "(... n lines snipped ...)" on a separate line, then
last ten lines.

If no file name is given, standard input is used.
```

## Build dependencies
None

## Building
Just compile it! For example like this:
```
gcc -O2 -Wall -g headtail.c -o /usr/local/bin/headtail
```
