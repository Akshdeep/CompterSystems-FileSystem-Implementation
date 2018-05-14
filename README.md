Filesystem implementation

Read homework-2.pdf for the deliverables.
Read homework-2-docs.pdf for documentation on FUSE.
Read homework-2-testing.pdf for information on how to write tests.

You will need to compile and run this assignment on a Linux machine with FUSE installed.

To obtain FUSE on the NEU servers the fuse-2.9.7.tar.gz tar has been provided. Use `build-fuse.sh` to build it locally. Then you must set an environment variable before you can compile

```
export PKG_CONFIG_PATH=fuse-2.9.7/install/lib/pkgconfig
```

Or you can use the ubuntu virtual machine for this course:

  http://www.ccs.neu.edu/~jrafkind/cs5600/CS5600-f17-Ubuntu32.ova

On Ubuntu you should install the libfuse-dev package

```
  $ apt-get install libfuse-dev
```
