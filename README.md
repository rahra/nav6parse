# nav6parse

This small Perl program converts user data generated by a Seatec NAV6 chart
plotter into the well-known GPX format. It reads the data from standard input
and outputs the GPX data to standard output. Thus just run it as follows:

```Shell
./nav6parse <kpxx99.odf >kpxx99.gpx
```

The resulting file can then be imported into any GPX-aware software (e.g.
Goolge Earth).

# untuf

Untuf is a small programm in extract all files from the .tuf file which is
found in the update packages for the Seatec NAV6 chartplotter.
Untuf depends on the zlib which is typically found in the package zlib1g-dev.
To compile it just run `make`.
Untuf reads the .tuf file from the standard input, thus run it as follows:

```Shell
./untuf <NAV6_140224_1.tuf
```

