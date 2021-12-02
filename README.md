# headtail & compcat

```
Usage: headtail [OPTION] [FILE]...

Options:
  -n <lines> max. number of head and tail lines (default is half terminal height)
  -w         compress horizontally as well: Snip long lines with "(...)"
  -c <cols>  specify width for -w (default is terminal width)
  -l         show line numbers
  -q         never output filename headers
  -h         this help

HeadTail utility (c) magnum 2021

Works similar to 'head' and 'tail' on each file but does both at once, even for
stdin (which is impossible with head/tail).
If file fits terminal height (using defaults), just output the whole file.  If
it is longer, output the head followed by "(... n lines skipped ...)" on a
separate line, then the tail.
Lines can optionally be horizontally compressed similarly, with -w and/or -c

If no file name is given, standard input is used.

Unlike 'head' and 'tail', this tools adds a final LF in case the last line
was lacking it.
```

```
Usage: compcat [OPTION] [FILE]...

Options:
  -c <cols>  specify width (default is terminal width)
  -l         show line numbers
  -q         never output filename headers
  -h         this help

CompCat utility (c) magnum 2021
Horizontally compress output, as in "Lorem ipsum dolor sit (...) elit."

If no file name is given, standard input is used.

Unlike 'cat', this tools adds a final LF in case the last line was lacking it.
```

## Build dependencies
None!

## Building & installing
Just compile it! The single binary morph when called as "compcat".
For example like this:
```
gcc -O2 -Wall headtail.c -o headtail
mv headtail /usr/local/bin
ln -s headtail /usr/local/bin/compcat
```
