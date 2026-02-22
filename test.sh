#!/bin/sh
#******************************************************************************#
# gptgen integration tests                                                     #
# These tests are intended to run on Linux with GNU CoreUtils                  #
# and GNU Parted. They make work on other Unix-like systems as                 #
# well, but are only supported there on a best-effort basis.                   #
# They run gptgen as a user typically would to ensure that it                  #
# works. Run these tests in the same directory as gptgen.                      #
#                                                                              #
# Copyright (c) 2026, Karl Lenz <xorangekiller@gmail.com>                      #
#                                                                              #
# Permission to use, copy, modify, and/or distribute this software for any     #
# purpose with or without fee is hereby granted, provided that the above       #
# copyright notice and this permission notice appear in all copies.            #
#                                                                              #
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES     #
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF             #
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR      #
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES       #
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN        #
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF      #
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.               #
#******************************************************************************#

# Exit immediately if any non-zero exit codes are returned by any command so
# that we don't have to check it every time.
set -e

# Ensure that we cleanup the files generated during these tests, regardless of
# how or where this script ends.
cleanup() {
	exit_code="$?"

	if [ "$SKIP_CLEANUP" = "1" ]; then
		echo "[test] SKIP_CLEANUP=$SKIP_CLEANUP - skipping test cleanup (for debug)"
	else
		echo "[test] Cleaning up..."
		rm -f disk.img primary.img secondary.img
	fi

	if [ "$exit_code" != 0 ]; then
		echo "[test] Error: Test failed! Last command returned: $exit_code"
	fi
}
trap cleanup EXIT INT TERM QUIT

# Some of the utilities we're using, such as parted, may be in sbin because
# they are ordinarily only intended to be run as root. They may not be in
# PATH for a normal user. Since we're testing on a disk image rather than a
# real disk, we can safely use them on that. Ensure that sbin is in PATH.
for path in \
	/usr/local/sbin \
	/usr/sbin \
	/sbin
do
	if [ -d "$path" ] && ! echo "$PATH" | grep -Eqs "(^|:)${path}(:|$)"; then
		export PATH="${PATH}:${path}"
	fi
done

# Do some sanity checks. This will give nicer error messages if the utilities
# we depend on aren't installed before we get into the tests and try to use
# them.
if [ ! -x ./gptgen ]; then
	echo "[test] Error: gptgen not present in $PWD"
	echo "[test] - Have you built it?"
	echo "[test] - Are you running the tests in the right directory?"
	exit 123
fi
for util in \
	dd \
	du \
	parted \
	mkfs.vfat \
	fsck.vfat \
	md5sum
do
	if ! command -v "$util" 1>/dev/null 2>&1; then
		echo "[test] Error: \"$util\" command not found"
		exit 123
	fi
done

echo "[test] Testing help output..."
./gptgen --help

echo "[test] Creating empty disk image..."
dd if=/dev/zero of=disk.img bs=1M count=64

echo "[test] Creating MSDOS partition table..."
parted -s disk.img -- mktable msdos
parted -s disk.img -- mkpart primary 1MiB 20MiB
parted -s disk.img -- mkpart extended 21MiB 62MiB
parted -s disk.img -- mkpart logical 22MiB 33MiB
parted -s disk.img -- mkpart logical 34MiB 62MiB
# gptgen has special handling for the boot flag, so set it on the first
# partition so that we can test that logic. That's similar to how it would
# typically look on an MSDOS formatted disk with an OS and a bootloader.
# See https://www.gnu.org/software/parted/manual/html_node/set.html
parted -s disk.img -- set 1 boot on # Legacy MS-DOS boot flag
# The partition types don't really matter for our purposes. GNU Parted sets
# them to 0x83 ("Linux") by default. We're going to set them to different
# types just to be slightly more realistic and ensure they're preserved
# during the GPT conversion. See this list for the list of partition IDs:
# https://en.wikipedia.org/wiki/Partition_type
parted -s disk.img -- type 1 0x0c # Primary Partition 1: FAT32 with LBA
parted -s disk.img -- type 5 0x07 # Logical Partition 5: NTFS
parted -s disk.img -- type 6 0x83 # Logical Partition 6: Linux (e.g. ext4)
parted -s disk.img -- print
# TODO: Format with mkfs.vfat and ensure that the filesystem is still intact
# after the GPT conversion?
original_hash="$(md5sum disk.img | awk '{print $1}')"
original_size="$(du -b disk.img | awk '{print $1}')"

# gptgen can only automatically determine the block/sector size for block
# devices by asking the OS. For disk images, it will prompt the user to supply
# it. Set the block size for our test disk image so that we can supply it to
# gptgen when prompted and our tests run automatically.
block_size=512 # disk image's block size in bytes

if [ "$HEXDUMP_DISK" = "1" ]; then
	echo "[test] Printing MSDOS partitioned disk image (for debugging)..."
	# We created the disk image from /dev/zero and didn't write any files to it,
	# just the partition table. It should still be mostly zeroes that would just
	# be distracting to see. Tell xxd to omit adjacent zeroes with the "-a" flag.
	xxd -a disk.img
fi

echo "[test] Converting MBR to GPT with default options (non-destructive)..."
printf "${block_size}\rY\r" | ./gptgen disk.img
run1_hash="$(md5sum disk.img | awk '{print $1}')"

echo "[test] Is the original disk image left unmodified?"
echo "[test] $original_hash == $run1_hash?"
test "$original_hash" = "$run1_hash"

echo "[test] Were the primary and secondary GPT images created?"
test -f primary.img
test -f secondary.img
if [ "$HEXDUMP_DISK" = "1" ]; then
	echo "[test] Printing the primary GPT image (for debugging)..."
	xxd -a primary.img
	echo "[test] Printing the secondary GPT image (for debugging)..."
	xxd -a secondary.img
fi
rm -f primary.img secondary.img

echo "[test] Converting MBR to GPT in place (destructively on disk)..."
printf "${block_size}\r" | ./gptgen -w -b mbr.img -k disk.img
run2_hash="$(md5sum disk.img | awk '{print $1}')"
run2_size="$(du -b disk.img | awk '{print $1}')"

if [ "$HEXDUMP_DISK" = "1" ]; then
	echo "[test] Printing GPT partitioned disk image (for debugging)..."
	xxd -a disk.img
fi

echo "[test] Was the original disk image modified?"
echo "[test] $original_hash != $run2_hash?"
test "$original_hash" != "$run2_hash"
echo "[test] $original_size bytes == $run2_size bytes?"
test "$original_size" = "$run2_size"

echo "[test] Were the primary and secondary GPT images created?"
test ! -e primary.img
test ! -e secondary.img

echo "[test] Was the backup MBR image created?"
test -e mbr.img
rm -f mbr.img

echo "[test] Is the new partition table GPT?"
run2_part_info="$(parted -s disk.img -- print 2>&1)"
echo "$run2_part_info"
if echo "$run2_part_info" | grep -Fqs 'Error'; then
	echo "[test] Parted encountered an error reading the disk image."
	exit 1
fi
echo "$run2_part_info" | grep -Eqs 'Partition Table: gpt'
echo "[test] Does partition 1 exist?"
echo "$run2_part_info" | grep -Eqs '^\s*1\s+'
echo "[test] Does partition 2 exist?"
echo "$run2_part_info" | grep -Eqs '^\s*2\s+'
echo "[test] Does partition 3 exist?"
echo "$run2_part_info" | grep -Eqs '^\s*3\s+'
echo "[test] Is partition 4 absent?"
! echo "$run2_part_info" | grep -Eqs '^\s*4\s+'
echo "[test] Is partition 5 absent?"
! echo "$run2_part_info" | grep -Eqs '^\s*5\s+'
echo "[test] Is partition 6 absent?"
! echo "$run2_part_info" | grep -Eqs '^\s*6\s+'

echo "[test] Success! All tests passed."
