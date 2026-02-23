README for gptgen v1.3
======================

## 1. Introduction

Gptgen is a tool for converting hard drives partitioned according to
the MSDOS-style "MBR" scheme to the GPT (GUID partition table)
partitioning scheme, while retaining all data on the hard disk.
Extended/logical partitions are supported, though GPT doesn't
differentiate between primary and logical partitions, so after
conversion, all partitions will be de facto primary.

## 2. Caveats

* Because GPT's partition types do not map exactly to MBR types,
  the conversion might not be perfectly accurate, especially when
  multiple file systems are assigned the same MBR partition type code.
  This has happened a few times in the past, and is prone to occur
  again in the future. In case of such a type collision, gptgen
  assumes the most common use of a given type ID, and transcribes
  accordingly. So, if you have rare filesystems on your hard disk, then
  double-check partition type GUIDs after conversion. One specific
  example of a collision is type 0xAF, which is used both by macOS
  (to represent HFS and HFS+) and ShagOS (to represent swap space).
  Gptgen assumes 0xAF to be HFS(+).

* For MBR partition types with no known matching type GUID, gptgen
  uses a generic GUID, which is as follows:
  `{1575DA16-F2E2-40DE-B715-C6E376663Bxx}`
  where "xx" is the MBR type ID of the partition. Other users of GPT
  may implement reading this ID, but it is not recommended to rely on
  this ID for creating new partitions; instead it is better to define a
  new GUID to represent the desired partition type.

* Gptgen requires administrator privileges to run. On Windows Vista and
  newer Windows systems with UAC enabled, you may need to run gptgen
  from an elevated command prompt. On Linux, run with `sudo`.

* Big-endian support is completely untested, though necessary code is
  in place.

* The GUID partition table requires that no partition can begin before
  logical block 34 (number of GPT entries*128/block size + 2 - gptgen
  defaults to 128 entries), and that there must be at least 33
  (entry count*128/block size + 1) free blocks at the end of the drive
  before conversion. The first requirement is usually not a problem,
  since MBR partitions begin in the second "track" of the drive (in the
  BIOS-supplied fake geometry, not related to the true geometry in
  modern drives), and a "track" is usually 63 sectors long, making the
  first partition begin in block 63; however the second limitation
  might require the last partition of the disk to be shrunk by a
  cylinder to make way for the backup GPT at the end of the disk. Gptgen
  will warn and stop if the drive doesn't meet these requirements.
  (The actual needed sector numbers can vary with the chosen entry
  count and the drive's block size, "34" and "33" are based on a
  drive block size of 512 bytes and an entry count of 128.)

* Some implementations of GPT (e.g. Windows) require certain partitions
  to be present on the disk (e.g. an EFI boot partition). Gptgen
  doesn't create these partitions, it can only convert the existing
  partition table's contents. To create such partitions, use a
  GPT-ready partition manager such as parted.

* Because booting is handled differently with GPT than with MBR,
  converting a boot drive to GPT may render it unbootable. To make a GPT
  drive bootable on BIOS, you will need to create a BIOS boot partition
  and install a GPT-aware boot loader (such as GRUB 2.02 or later) on
  it.

## 3. Usage

On Windows (a precompiled Windows binary is included in the package),
the syntax of the tool is `gptgen [-w] \\.\physicaldriveX`, where "X"
is the drive number reported by the Disk Management console or the
`list disk` command of the `diskpart` utility. The `-w` option makes
gptgen write the generated GUID partition tables to the disk, otherwise
the primary table will be written to a file named "primary.img",
and the secondary table to "secondary.img", in the directory that the
program was invoked from. You can then use a raw disk utility such as
`dd` to write the tables to the disk.

On Linux, the syntax is `gptgen [-w] /dev/sda`, with `/dev/sda` being
the block device representing the physical drive to be converted. The
`-w` option works the same way as on Windows.

You can add the `-m` parameter to the command line to preserve the
existing MBR on the disk, instead of writing a protective MBR. This is
not recommended, and may prevent recognition of the drive as GPT on some
systems, but it is useful when you want to be extra safe, and verify
the newly-written GPT before wiping out the MBR.

Gptgen, by default, builds a GPT consisting of 128 partition entries.
You can override this using the `-c` (`--count`) parameter, e.g.
`gptgen -w -c 32 \\.\physicaldriveX`. However, this is not recommended,
as some implementations have problems recognizing GPTs with other than
128 entries.

The parameter `-b <filename>` tells gptgen to back up the original MBR
of the target drive into the file indicated by `<filename>`.

## 4. Compiling and installing

On Linux, you can build gptgen using `cmake` and `make`. To install it,
run `make install`. This installs gptgen to `/usr/local/sbin` by
default. To install it to a different location, use
`cmake -DCMAKE_INSTALL_PREFIX=<prefix> .` to generate the Makefiles.
`make install` will then install gptgen to `<prefix>/sbin`. To relocate
the whole installation, use the `DESTDIR` variable on the make command
line (e.g. `make DESTDIR=<install_root> install`).

**Building with the default compiler (GCC or Clang) on Linux:**
```
$ cmake .
$ make
$ sudo make install
```

On Windows, building with both MinGW and Visual Studio is supported. For
building with MinGW, [MSYS2](https://www.msys2.org) is recommended. For
building with Visual Studio, you may use the freely available
[Visual Studio Community Edition](https://visualstudio.microsoft.com/vs/community/)
with [CMake Project support](https://docs.microsoft.com/en-us/cpp/ide/cmake-tools-for-visual-cpp).

**Building with MinGW on Windows (from a MSYS2 Mingw-w64 shell prompt):**
```
$ cmake -DCMAKE_SYSTEM_NAME=Windows .
$ make
```

**Building with Visual Studio Community 2026 from the Command Prompt:**
```
> "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
> cmake -GNinja .
> ninja
```

**Building with Visual Studio Community 2026 from the IDE:**
1. Open Visual Studio.
2. Go to File->Open->CMake.
3. Open the CMakeLists.txt in the gptgen source directory.
4. Choose the type of build that you would like (e.g. "x64-Release").
5. Run CMake->Build All.
6. Run CMake->Install->gptgen.
7. You can find `gptgen.exe` in the location show in the CMake build log
   in the Visual Studio "output" pane.

**Cross-compiling for Windows on Linux with Mingw-w64 (not recommended):**
```
$ cmake -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ .
$ make
```

To build gptgen with debug support, add the following arguments on the
CMake command line:
`-DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=TRUE`

## 5. Testing

Gptgen is a small, tightly integrated utility that typically requires direct
hardware access to read or modify the disk. In theory it could be unit tested,
but it makes more sense to just integration test it to more closely mimic how a
real user would use it.

The integration tests are implemented in a simple Bourne shell script that
mostly just requires standard Unix system utilities (`dd`, `bc`, `grep`, etc.)
and the [GNU Parted](https://www.gnu.org/software/parted/) partition manager.
It has only been tested on GNU/Linux, but should work in other Unix-like
environments as well (such as macOS, FreeBSD, or *maybe* MSYS2 on Windows) as
long as you have the prerequisites installed. Portability of the tests to other
systems is considered non-blocking since everyone can spin up a lightweight
Linux container on their OS of choice to test the core, OS-agnostic gptgen
logic, but the reverse is not true.

**Installing the integration test dependencies on Debian or Ubuntu:**
```
$ sudo apt install parted dosfstools
```

**Running the integration tests:**
```
$ ./test.sh
```

**Extra tools for debugging:**
* Set the `HEXDUMP_DISK=1` environment variable before running `test.sh` to
  print a hex dump of the disk image used for testing at each stage with Vim's
  [xxd](https://github.com/vim/vim/blob/master/runtime/doc/xxd.man) hex dump
  utility.
* Set the `SKIP_CLEANUP=1` environment variable before running `test.sh` to
  leave the `disk.img` file after running the tests so that you can inspect it
  yourself.
* Run `/sbin/parted disk.img` to enter an interactive GNU Parted session to
  inspect the disk image from the tests.
  * Run `print free` to print the partition table and free space.
* Run `/sbin/gdisk disk.img` to use [GPT fdisk](https://rodsbooks.com/gdisk/)
  to inspect the GPT partition table created by gptgen in the disk image
  created by the tests.
  * GPT fdisk is GPT-specific, and has some more advanced tools than GNU Parted
    for GPT partition tables, but otherwise doesn't have the broader support of
    GNU Parted. They're both useful.
  * Use the `v` command to inspect the partition table and report problems.
    There should be no problems reported on a partition table created by
    gptgen.
