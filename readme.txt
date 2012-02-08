README for gptgen v1.1

1. Introduction

Gptgen is a tool for converting hard drives partitioned according to
the MSDOS-style "MBR" scheme to the GPT (GUID partition table)
partitioning scheme, while retaining all data on the hard disk.
Extended/logical partitions are supported, though GPT doesn't
differentiate between primary and logical partitions, so after
conversion, all partitions will be de facto primary.

2. License

Gptgen is licensed under the ISC license, the full text of which is
reproduced below:

-----------------------------------------------------------------------
Copyright (c) 2009-2012, Gábor A. Stefanik <netrolller.3d@gmail.com>

Permission to use, copy, modify, and/or distribute this software for
any purpose with or without fee is hereby granted, provided that the
above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRAN-
TIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
-----------------------------------------------------------------------

(I wish to encourage partitioning software vendors, both free and prop-
rietary, to implement this feature in their products. Feel free to
reuse this code for that purpose.)

3. Caveats

* Because GPT's partition types do not map exactly to MBR types,
  the conversion might not be perfectly accurate, especially when mul-
  tiple file systems are assigned the same MBR partition type code.
  This has happened a few times in the past, and is prone to occur
  again in the future. In case of such a type collision, gptgen assu-
  mes the most common use of a given type ID, and transcribes accor-
  dingly. So, if you have rare filesystems on your hard disk, then
  double-check partition type GUIDs after conversion. One specific
  example of a collision is type 0xAF, which is used both by Mac OS X
  (to represent HFS and HFS+) and ShagOS (to represent swap space).
  Gptgen assumes 0xAF to be HFS(+).
* For MBR partition types with no known matching type GUID, gptgen
  uses a generic GUID, which is as follows:
  {1575DA16-F2E2-40DE-B715-C6E376663Bxx}
  where "xx" is the MBR type ID of the partition. Other users of GPT
  may implement reading this ID, but it is not recommended to rely on
  this ID for creating new partitions; instead it is better to define a
  new GUID to represent the desired partition type.
* Gptgen requires administrator privileges to run. On Windows Vista and
  newer Windows systems with UAC enabled, you may need to run gptgen
  from an elevated command prompt. On Linux, run with "sudo".
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
  might require the last partition of the disk to be shrunk by a cylin-
  der to make way for the backup GPT at the end of the disk. Gptgen
  will warn and stop if the drive doesn't  meet these requirements.
  (The actual needed sector numbers can vary with the chosen entry
  count and the drive's block size, "34" and "33" are based on a
  drive block size of 512 bytes and an entry count of 128.)
* Some implementations of GPT (e.g. Windows) require certain partions
  to be present on the disk (e.g. an EFI boot partition). Gptgen
  doesn't create these partitions, it can only convert the existing
  partition table's contents. To create such partitions, use a
  GPT-ready partition manager such as parted.
* Because booting is handled differently with GPT than with MBR, con-
  verting a boot drive to GPT may render it unbootable. To make a GPT
  drive bootable on BIOS, you will need to create a BIOS boot partition
  and install a GPT-aware boot loader (such as Grub 2 SVN) on it.

4. Usage

On Windows (a precompiled Windows binary is included in the package),
the syntax of the tool is "gptgen [-w] \\.\physicaldriveX", where X
is the drive number reported by the Disk Management console or the
"list disk" command of the diskpart utility. The -w option makes
gptgen write the generated GUID partition tables to the disk, otherwise
the primary table will be written to a file named "primary.img",
and the secondary table to "secondary.img", in the directory that the
program was invoked from. You can then use e.g. dd to write the tables
to the disk.

On Linux, the syntax is "gptgen [-w] /dev/hda", "/dev/hda" being the
block device representing the physical drive to be converted. The -w
option works the same way as on Windows.

You can add the -m parameter to the command line to preserve the exis-
ting MBR on the disk, instead of writing a protective MBR. This is not
recommended, and may prevent recognition of the drive as GPT on some
systems, but it is useful when you want to be extra safe, and verify
the newly-written GPT before wiping out the MBR.

Gptgen, by default, builds a GPT consisting of 128 partition entries.
You can override this using the -c (--count) parameter, e.g.
"gptgen -w -c 32 \\.\physicaldriveX". However, this is not recommended,
as some implementations have problems recognizing GPTs with other than
128 entries.

The parameter "-b <filename>" tells gptgen to back up the original MBR
of the target drive into the file indicated by <filename>.

5. Compiling and installing
On Linux, you can build gptgen using make. To install it,
run "make install". This installs gptgen to /usr/local/sbin by default,
to install to a different location, use "make prefix=<prefix> install".
This will install gptgen to <prefix>/sbin.

On Windows, use Dev-C++ or Code::Blocks to compile.

6. Version history

v0.1:
-First proof-of-concept code.

v1.0:
-Original production release.

v1.1: 
-Added "-b" ("--backup") option for backing up the original MBR.
-Added support for Acer/Lenovo recovery partitions (ID 0x12).
-Fixed a crash in the argument parsing code.
-The hidden status of Windows Recovery Environment partitions
 is now preserved correctly.
-Added Code::Blocks support.