# headtail & compcat

```
(c) magnum 2021
Usage: headtail [OPTION]... [FILE]...

Options:
  -n <lines> max. number of head and tail lines (default is half terminal height)
  -w         compress horizontally as well: Snip long lines with "(...)"
  -c <cols>  specify width for -w (minimum 20, default is terminal width)
  -l         show line numbers
  -t <width> set tab width (default 8)
  -H         parse $HEX[6141] --> aA
  -q         never output filename headers
  -h         this help

Works similar to 'head' and 'tail' on each file but does both at once, even for
stdin (which is impossible with head/tail).
If file fits terminal height (using defaults), just output the whole file.  If
it is longer, output the head followed by "(... n lines skipped ...)" on a
separate line, then the tail.  Lines can optionally be horizontally compressed
similarly, using -w and/or -c.  If no file is given, standard input is used.
Unlike 'head'  and 'tail', this tool adds a final LF in case the last line was
lacking it.
```

```
(c) magnum 2021
Usage: compcat [OPTION]... [FILE]...

Options:
  -c <cols>  specify width (minimum 20, default is terminal width)
  -l         show line numbers
  -t <width> set tab width (default 8)
  -H         parse $HEX[6141] --> aA
  -q         never output filename headers
  -h         this help

Horizontally compress output, as in "Lorem ipsum dolor sit (...) elit.".  If no
file is given, standard input is used.  Unlike 'cat', this tool adds a final LF
in case the last line was lacking it.
```

## Build dependencies
None!

## Building & installing
Just compile it! A single binary morph when called as "compcat".

Build and install like this:
```
gcc -O2 -Wall headtail.c -o headtail
mv headtail /usr/local/bin
ln -s headtail /usr/local/bin/compcat
```

## Example output:
```
$ headtail -n 10 rockyou.lst
123456
12345
123456789
password
iloveyou
princess
1234567
rockyou
12345678
abc123
(... 14344334 lines skipped ...)
;bm;ylobitF89i
TRIDHAA
v13w50nic
rhudders
100692k
4.5bbcal
ninosbinos
1lalique
1450500134167
Vk122503
```
```
$ grep -F '$office' all_tests.in | compcat
$office$*2007*20*128*16*8b2c9e8c878844fc842012273be4be (...) 987f679ff4b5b4c2cd
$office$*2007*20*128*16*91f095a1fd02595359fe3938fa9236 (...) 911068c93268ae1d86
$office$*2007*20*128*16*56ea65016fbb4eac14a6770b2dbe7e (...) a72212a8848c2e9c80
$office$*2007*20*128*16*fbd4cc5dab9b8e341778ddcde9eca7 (...) 8ca4ad41eec382e0c8
$office$*2007*20*128*16*fbd4cc5dab9b8e341778ddcde9eca7 (...) b3908a24e7b5a89dd6
$office$*2007*20*128*16*fbd4cc5dab9b8e341778ddcde9eca7 (...) 9cebc3d0ff7cc8b1d8
$office$*2007*20*128*16*fbd4cc5dab9b8e341778ddcde9eca7 (...) d91cf9e7bbf88a1b3b
$office$*2007*20*256*16*3e94c22e93f35e14162402da444dec (...) f07af904ce518b53e6
$office$*2010*100000*128*16*213aefcafd9f9188e78c1936cb (...) e79ee044e663641d5e
$office$*2010*100000*128*16*0907ec6ecf82ede273b7ee87e4 (...) 4384ad631000a5097a
$office$*2010*100000*128*16*71093d08cf950f8e8397b8708d (...) 0fc1a0dc71600dac38
$office$*2010*100000*128*16*71093d08cf950f8e8397b8708d (...) bf7fa3543853e0d11a
$office$*2013*100000*256*16*9b12805dd6d56f46d07315153f (...) 77001edbafba7769cd
$office$*2013*100000*256*16*774a174239a7495a59cac39a12 (...) f0ea1a33770187caef
$office$*2013*100000*256*16*d4fc9302eedabf9872b24ca700 (...) f14275a8c119c3a4fd
$office$*2013*100000*256*16*59b49c64c0d29de733f0025837 (...) 46cb3ad9d847de9ec9
$office$*2013*100000*256*16*f1c23049d85876e6b20e95ab86 (...) b67b566173e89f941d
$office$*2007*20*128*16*7268323350556e5276713670315262 (...) 1cce88cb8b00000000
$office$*2010*100000*128*16*42624931633777446c67354e34 (...) 23dee6287a6ed55f9b
$office$*2013*100000*256*16*36537a3373756b587632386d77 (...) fe1f60c85b044aa125
$office$*2013*100000*256*16*f4984f25c246bb742259ec55b4 (...) 2a9616fb48a583f259
```
