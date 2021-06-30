# Fast NCD
This is an implementation of *ncd* utility (from [libcomplearn](https://github.com/rudi-cilibrasi/libcomplearn)
package) written from scratch focusing on parallel performance, i.e., when is
desired to obtain NCD from large or huge directories, with thousands or millions
of files (either small, medium or large in their lengths).

## Advantages over *libcomplearn ncd*
This implementation uses some strategies to calculate NCD as fast as possible,
such as:
* Multithreading support
* Map multiple files at memory so they can be shared by multiple threads
* Compressed data are written only in memory (reducing I/O disk operations)
* No external tools (programs) are required to run in order to compress files

## How to compile and run

```bash
./autogen.sh
./configure
make
```
You can run *ncd* directly from **src** directory or install in the system with
*make install* command (as superuser).

## Usage
*ncd* tries to keep compatibility with *libcomplearn* ncd, but there are some
differences in the command line utilization:

```
Usage: ncd [options] ... <arg>
Use one argument form to do simple single-object or object-vector
compression or
    ncd [options] ...  <arg1> <arg2>
for compression-matrix or NCD matrix calculation
OPTIONS:
    -c, --compressor=COMPNAME   set compressor to use
    -d, --directory-mode        directory of files
    -h, --help                  print this help message
    -L, --list                  list compressors
    -o, --output=FILEOUT        use FILEOUT instead of distmatrix
    -s, --size                  just compressed sizes in bits no NCD
    -t, --threads               maximum number of threads
    -v, --verbose               print extra detailed information
    -V, --version               print program's version and exit
```

**Examples:**

```bash
ncd -c zlib -d dirtest1/
ncd -c ppmd -s -d dirtest2/
ncd -c bzlib file1 file2
ncd -c ppmd -t 8 -d largedir/
ncd -c ppmd -t 8 -o matrix.txt -d dirtest2/
ncd -c ppmd -t 8 -o - -d dirtest2/
```
Note that if you are leading with small directory and/or files, there is no
special reason to use this utility, you can keep using *ncd* from *libcomplearn*.

### Supported compressors

* ppmd: Source code from LZMA SDK embedded on this implementation
* zlib: Provided by zlib library
* bzlib: Provided by bzlib library


## Bug reports

* Please, report them to [rene@renesp.com.br](mailto:rene@renesp.com.br)
