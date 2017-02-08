# mvlf v0.2
A tool for moving logfiles (or other datestamped files) with date format conversion.

## Written by
Björn Westerberg Nauclér (mail@bnaucler.se) 2016

Compiled and tested on Arch Linux 4.9 and FreeBSD 11.

## Installation
`sudo make all install`  
The binary is installed in /usr/bin unless otherwise specified with DESTDIR

## Usage
Output of `mvlf -h`  
```
Move Logfiles v0.2
Usage: mvlf [-achioqtv] [dir/prefix] [dir/prefix]
	-a: Autodetect input pattern (experimental)
	-c: Capitalize output suffix initials
	-h: Show this text
	-i: Input pattern (default: tmY)
	-o: Output pattern (default: Y-n-t)
	-q: Decrease verbosity level
	-t: Test run
	-v: Increase verbosity level
```

## Patterns
Y = Year (1999, 2003...)  
y = Year shorthand (99, 03...)

M = Month (January, February...)  
m = Month shorthand (Jan, Feb...)  
n = Month number (01, 02...)  

D = Day (Monday, Tuesday...)  
d = Day shorthand (Mon, Tue...)  
t = Date (31, 01...)

### Example use
```
$ ls test/
prefix.2016-12-27  prefix.2016-12-28  prefix.NOTADATE
$ mvlf -i Y-n-t -o tmY test/prefix. test2/otherprefix.
Error: could not rename test/prefix.NOTADATE
$ ls test2/
otherprefix.27dec2016  otherprefix.28dec2016
$ ls test/
prefix.NOTADATE
```

## Contributing
This started as a quick hack, but is slowly turning into an actually useful tool. Feel free to send a pull request or raise an issue if you have questions or suggestions.

## License
MIT (do whatever you want)

## Disclaimer
The software has not undergone extensive testing as of yet. A test run (command line option -t) is recommended before altering data.

mvlf is not aware of actual time and dates, but will convert incorrect data to equally incorrect data.
