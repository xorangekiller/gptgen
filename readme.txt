README for gptgen v0.2pre

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
Copyright (c) 2009, Gábor A. Stefanik <netrolller.3d@gmail.com>

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
  from an elevated command prompt. On Linux, "sudo" probably suffices.
* Gptgen is completely untested on Linux, though necessary code is in-
  cluded to support the platform. To compile on Linux, comment out the
  "#define WE_ARE_WINDOWS 1" line, and compile gptgen.cpp with g++, no
  makefile is provided at this point (though later versions will likely
  include one). Windows users can use Dev-C++ to compile.
* Endianness is not auto-detected, you need to uncomment the line
  "#define WE_ARE_BIG_ENDIAN 1" in gptgen.cpp to compile on big-endian
  platforms. Big-endian support is also completely untested.
* Gptgen doesn't yet support drives with a block size other than 512
  bytes. This may change in the future. All calculations below assume
  a block size of 512 bytes.
* The GUID partition table requires that no partition can begin before
  logical block 34 (number of GPT entries/4 + 2 - gptgen uses 128
  entries), and that there must be at least 33 (entry count / 4 + 1)
  free blocks at the end of the drive before conversion. The first
  requirement is usually not a problem, since MBR partitions begin in
  the second "track" of the drive (in the BIOS-supplied fake geometry,
  not related to the true geometry in modern drives), and a "track"
  is usually 63 sectors long, making the first partition begin in block
  63; however the second limitation might require the last partition of
  the disk to be shrunk by a cylinder to make way for the backup GPT at
  the end of the disk. Gptgen will warn and stop if the drive doesn't
  meet these requirements.

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
