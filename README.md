## NBD server with stable plugin ABI and permissive license

nbdkit is an NBD server.  NBD — Network Block Device — is a protocol
for accessing Block Devices (hard disks and disk-like things) over a
Network.

The key features of nbdkit are:

* Multithreaded NBD server written in C with good performance.
* Minimal dependencies for the basic server.
* Liberal license (BSD) allows nbdkit to be linked to proprietary
  libraries or included in proprietary code.
* Well-documented, simple plugin API with a stable ABI guarantee.
  Lets you export “unconventional” block devices easily.
* You can write plugins in C/C++, Go, Lua, Perl, Python, OCaml,
  Rust, shell script or Tcl.
* Filters can be stacked in front of plugins to transform the output.
* Server can run standalone or can be invoked from other programs.

For documentation, see the [docs directory](docs/).

For plugins and filters, see the [plugins](plugins/) and
[filters](filters/) directories.  Example plugins can be found in
`plugins/example*`.

There are many NBD clients, but the nbdkit project has a companion
client library called https://gitlab.com/nbdkit/libnbd

Upstream repository: https://gitlab.com/nbdkit/nbdkit

## License

This software is Copyright Red Hat and licensed under a BSD license.
See [LICENSE](LICENSE) for details.

Examples are usually licensed even more permissively ("as close to
public domain as possible"), see the individual files for details.

## Building from source

### Requirements

* Linux, macOS, Windows, FreeBSD, OpenBSD or Haiku
* GCC or Clang
* bash ≥ 4
* GNU make

Recommended for TLS (authentication and encryption) support, but it is
possible to build without it:

* gnutls ≥ 3.5.18

For Windows support see [the Windows section below](README.md#windows).

For macOS support see [the macOS section below](README.md#macos).

### Optional dependencies

To build the man pages, you will optionally need to install:

* Perl
* `Pod::Man` and `Pod::Simple` (Perl libraries)

For SELinux socket labelling support:

* libselinux

For the gzip filter, or for supporting compressed qcow2:

* zlib or zlib-ng

For the bzip2 filter:

* bzip2

For the xz and lzip filters:

* liblzma

For the memory plugin with allocator=zstd, or for supporting compressed qcow2:

* zstd

For the curl (HTTP/FTP) plugin:

* libcurl

For the ssh plugin:

* libssh ≥ 0.8.0
  (this is a different library from libssh2 - that will not work)

For the iso plugin:

* xorriso or genisoimage or mkisofs

For the floppy plugin:

* iconv (on Linux this is built into glibc, on other systems
  it may be a separate library)

For the libvirt plugin:

* libvirt

For the libguestfs plugin, and to run parts of the test suite:

* libguestfs
* guestfish (from libguestfs)

For the ext2 filter:

* ext2fs
* com_err

For the linuxdisk plugin:

* mke2fs ≥ 1.42.10 (from e2fsprogs)

For the nbd plugin, to get URI and TLS support, and also to run parts
of the test suite:

* libnbd ≥ 0.9.8

For the bittorrent plugin:

* [libtorrent-rasterbar](https://www.libtorrent.org)

For the blkio plugin:

* [libblkio](https://libblkio.gitlab.io/libblkio)

For the containerized data importer (CDI) plugin:

* podman
* jq

For the Perl and example4 plugins:

* perl interpreter
* perl development libraries
* perl module `ExtUtils::Embed`

For the Python plugin:

* python interpreter (version 3 only)
* python development libraries
* python unittest to run the test suite
* boto3 is required to run the S3 plugin written in Python
* google-cloud-storage is required to run the gcs plugin written in Python

For the OCaml plugin:

* OCaml ≥ 4.03

For the Tcl plugin:

* Tcl development library and headers

For the Lua plugin:

* Lua development library and headers

For the Rust plugin:

* cargo (other dependencies will be downloaded at build time)

For the golang plugin:

* go ≥ 1.13

For bash tab completion:

* bash-completion ≥ 1.99

To test for memory leaks (`make check-valgrind`):

* valgrind program and development headers

For non-essential enhancements to the test suite:

* expect
* fdisk, sfdisk (from util-linux)
* flake8
* hexdump
* ip, ss (from iproute package)
* jq
* libc_malloc_debug.so.0 (from glibc-utils)
* losetup (from util-linux)
* lzip
* mke2fs (from e2fsprogs)
* nbdcopy, nbdinfo, nbdsh (from libnbd)
* qemu-img, qemu-io, qemu-nbd (usually shipped with qemu)
* socat
* stat (from coreutils)

### Building

```
autoreconf -i    # only required when building from git clone
./configure
make
make check
```

On FreeBSD and OpenBSD which do not have GNU make by default you must
use `gmake` instead of `make`.

To run nbdkit from the source directory, use the top level ./nbdkit
wrapper.  It will run nbdkit and plugins from the locally compiled
directory:

```
$ ./nbdkit example1 -f -v
./server/nbdkit ./plugins/example1/.libs/nbdkit-example1-plugin.so -f -v
[etc]
```

Optionally run this command as root to install everything:

```
make install
```

### Python

Since nbdkit ≥ 1.16, only Python ≥ 3.6 is supported.

By default nbdkit uses the Python version of the Python interpreter
called “python” on the current $PATH.  If you have parallel versions
of Python installed then you can choose a different version by setting
the PYTHON variable when configuring.  For example:

```
./configure PYTHON=/usr/bin/python3.8
```

### OCaml

Most recent (less than, say, 8 years old) versions of the OCaml
compiler should work.  By default the configure script should detect
the bytecode and native code compilers and enable either one or both.

To enable OCaml warnings and turn them into hard errors (developers only):

```
./configure OCAMLOPTFLAGS="-warn-error +A-3"
```

### Running the tests

To run the test suite:

```
make check
```

If there is a failure, look at the corresponding `tests/*.log` file
for debug information.

A few tests require root privileges, and are skipped by default.  To
run them you must do:

```
make check-root
```

If you have the proprietary VDDK library, you can test
[nbdkit-vddk-plugin](plugins/vddk/) against the library like this:

```
make check-vddk vddkdir=vmware-vix-disklib-distrib
```

### Running the benchmarks

To run benchmarks:

```
make bench
```

## Download tarballs

Tarballs are available from:
http://libguestfs.org/download/nbdkit

## Downstream packagers

If you are packaging nbdkit, use:

```
./configure --with-extra='...'
```

providing extra information about the distribution, and/or
distro-specific versions.  It helps us with troubleshooting bug
reports.  (Also, talk to us!)

## Developers

Install the valgrind program and development headers.

Use:

```
./configure --enable-gcc-warnings --enable-valgrind
```

When testing use:

```
make check
make check-valgrind
```

For development ideas, see the [TODO](TODO) file.

The upstream git repository is:
https://gitlab.com/nbdkit/nbdkit

Please send patches to the libguestfs mailing list:
https://lists.libguestfs.org

For further information, see:
https://libguestfs.org/
https://github.com/NetworkBlockDevice/nbd/blob/master/doc/proto.md

## Address sanitizer (ASAN)

You can compile nbdkit with clang and ASAN with:

```
./configure CC=clang CXX=clang++ \
            CFLAGS="-O0 -g -fsanitize=address -fno-omit-frame-pointer" \
            --disable-linker-script \
            --disable-golang
make clean
make
ASAN_OPTIONS="allocator_may_return_null=1:detect_leaks=0" make check
```

## Test coverage

```
./configure CFLAGS="--coverage -g" LDFLAGS="--coverage -g"
make clean
make
make check

lcov -c -d . -o gcov.info
genhtml -o coverage gcov.info
```

Open your browser and examine the `coverage/` directory.  At the time
of writing (2024-05) test coverage of the server is reasonable, but
things are much worse for certain plugins and filters.

## macOS

nbdkit works on macOS and is mostly feature complete.  Some tests may
fail or be skipped.

We have tested macOS 12 (Monterey), 13 (Ventura) and 14 (Sonoma),
using MacPorts.  Other versions of macOS and Homebrew may work too.

We installed the following MacPorts packages:

* autoconf
* autoconf-archive
* automake
* bash
* coreutils
* gcc
* gmake
* gsed
* jq
* libtool
* m4
* ocaml
* pkg-config
* qemu
* ranlib

We configured nbdkit using:

```
./configure --enable-gcc-warnings --disable-perl \
	    CFLAGS="-g -O2 -I/opt/local/include" \
	    LDFLAGS="-L/opt/local/lib"
```

## Windows

Experimentally, the server can be compiled on Windows or
cross-compiled from Linux using mingw-w64.  Only a small subset of
features are available.  To find out what is missing read the TODO
"Windows port".

Note that GCC or Clang is required even if you are compiling on
Windows because of some C language extensions we use.  MSVC will not
work.

### Cross-compiling from Linux to Windows

For the rest of this section we talk about cross-compiling for Windows
using Linux and mingw-w64.  At a minimum you will need:

* mingw-w64 gcc
* mingw-w64 dlfcn
* mingw-w64 winpthreads
* mingw-w64 gnutls  (optional, but highly recommended)
* wine              (if you want to run it on Linux)

You may want to patch Wine with support for `AF_UNIX` sockets.  It is
needed to get most of the test suite to work, but if you don't care
about the -U option then it is not needed.
https://bugs.winehq.org/show_bug.cgi?id=52568

Other mingw-w64 libraries may be installed which will add
functionality (see full list of requirements above), but you may end
up hitting areas we have not compiled or tested before.

To cross compile under Fedora or RHEL, ensure you have the following
prerequisites:

```
dnf install -y mingw64-{gcc,dlfcn,winpthreads,gnutls}
dnf install -y autoconf automake libtool make
```

then do:

```
autoreconf -i  # when building from git
mingw64-configure --disable-ocaml --disable-perl
make
```

You can test if the server is working by doing:

```
./nbdkit.exe --dump-config
```

(This usually runs wine automatically.  If not, you may need to prefix
the command "wine nbdkit.exe ...")

To see which plugins and filters were compiled:

```
find plugins filters -name '*.dll'
```

You can run them under Wine without installing using eg:

```
./nbdkit.exe -f -v memory 1G
```

You can run the test suite in the usual way:

```
make check
```

### Running the cross-compiled binary on Windows

To test the binary on a real Windows system you will need to copy
server/.libs/nbdkit.exe, any plugins and filters you need
(`plugins/*/.libs/*.dll` and `filters/*/.libs/*.dll`), and many mingw
DLLs (`libdl.dll`, `libgnutls.dll`, `libwinpthread.dll`, etc.).

Notes:

1. libtool creates files called nbdkit.exe in the top level directory
and in [server](server/).  These are the stripped binaries.  The
"real" binaries are located in hidden `.libs/` subdirectories.

2. The top level `.libs/nbdkit.exe` is the wrapper used to run nbdkit
from the build directory, not the server binary.  You need
`server/.libs/nbdkit.exe` instead.

3. To find the list of mingw DLLs I ran nbdkit.exe iteratively until
Windows stopped complaining about missing libraries.  You could also
use a tool like Dependency Walker.

If you copy everything into a single directory on the Windows server
then you should be able to run nbdkit normally, eg:

```
nbdkit.exe -f -v nbdkit-memory-plugin.dll 1G
```

You will probably need to open port 10809 on the Windows Firewall.
