/******************************************************************************\
* gptgen version 0.1                                                           *
* Utility for converting MBR/MSDOS-partitioned disk drives                     *
* to GUID Partition Table.                                                     *
*                                                                              *
* Copyright (c) 2009, G�bor A. Stefanik <netrolller.3d@gmail.com>              *
*                                                                              *
* Permission to use, copy, modify, and/or distribute this software for any     *
* purpose with or without fee is hereby granted, provided that the above       *
* copyright notice and this permission notice appear in all copies.            *
*                                                                              *
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES     *
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF             *
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR      *
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES       *
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN        *
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF      *
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.               *
\******************************************************************************/

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>

#define WE_ARE_WINDOWS 1 // comment this line out on Linux
//#define WE_ARE_BIG_ENDIAN 1 // uncomment this line on big-endian platforms

#ifdef WE_ARE_WINDOWS
#include <windows.h>
#include <winioctl.h>
#else
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#endif

using namespace std;

#define GPT_MAGIC {0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54} // "EFI PART"
#define GPT_V1 {0x00, 0x00, 0x01, 0x00}

#define PART_FLAG_SYSTEM (1ULL<<0)
#define PART_FLAG_RDONLY (1ULL<<60)
#define PART_FLAG_HIDDEN (1ULL<<62)
#define PART_FLAG_NOMOUNT (1ULL<<63)

typedef uint64_t __be64;

#define	swap16(x) (((x)<<8)|((x)>>8))
#define swap32(x) (((x)<<24) | \
	(((x)<<8)  & 0x00FF0000) |\
	(((x)>>8)  & 0x0000FF00) |\
	((x)>>24))
#define swap64(x) (((x)<<56) | \
	(((x)<<40) & 0x00FF000000000000ULL) |\
	(((x)<<24) & 0x0000FF0000000000ULL) |\
	(((x)<<8)  & 0x000000FF00000000ULL) |\
	(((x)>>8)  & 0x00000000FF000000ULL) |\
	(((x)>>24) & 0x0000000000FF0000ULL) |\
	(((x)>>40) & 0x000000000000FF00ULL) |\
	((x)>>56))

// TODO endianness auto-detection
#ifdef WE_ARE_BIG_ENDIAN
#define	cpu_to_be16(x) (x)
#define be16_to_cpu(x) (x)
#define	cpu_to_be32(x) (x)
#define be32_to_cpu(x) (x)
#define	cpu_to_be64(x) (x)
#define be64_to_cpu(x) (x)
#define	cpu_to_le16(x) swap16(x)
#define le16_to_cpu(x) swap16(x)
#define cpu_to_le32(x) swap32(x)
#define le32_to_cpu(x) swap32(x)
#define cpu_to_le64(x) swap64(x)
#define le64_to_cpu(x) swap64(x)
#else
#define	cpu_to_le16(x) (x)
#define le16_to_cpu(x) (x)
#define	cpu_to_le32(x) (x)
#define le32_to_cpu(x) (x)
#define	cpu_to_le64(x) (x)
#define le64_to_cpu(x) (x)
#define	cpu_to_be16(x) swap16(x)
#define be16_to_cpu(x) swap16(x)
#define cpu_to_be32(x) swap32(x)
#define be32_to_cpu(x) swap32(x)
#define cpu_to_be64(x) swap64(x)
#define be64_to_cpu(x) swap64(x)
#endif

struct __guid {
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	__be64 data4;
}__attribute__((packed));

#define NULL_GUID {0x00000000, 0x0000, 0x0000, 0x0000000000000000}
#define EFI_SYS_GUID {cpu_to_le32(0xC12A7328), cpu_to_le16(0xF81F),\
	cpu_to_le16(0x11D2), cpu_to_be64(0xBA4B00A0C93EC93BULL)}
#define MS_DATA_GUID {cpu_to_le32(0xEBD0A0A2), cpu_to_le16(0xB9E5),\
	cpu_to_le16(0x4433), cpu_to_be64(0x87C068B6B72699C7ULL)}
#define MS_META_GUID {cpu_to_le32(0x5808C8AA), cpu_to_le16(0x7E8F),\
	cpu_to_le16(0x42E0), cpu_to_be64(0x85D2E1E90434CFB3ULL)}
#define MS_DYN_GUID {cpu_to_le32(0xAF9B60A0), cpu_to_le16(0x1431),\
	cpu_to_le16(0x4F62), cpu_to_be64(0xBC683311714A69ADULL)}
#define MS_WINRE_GUID {cpu_to_le32(0xDE94BBA4), cpu_to_le16(0x06D1),\
	cpu_to_le16(0x4D40), cpu_to_be64(0xA16ABFD50179D6ACULL)}
#define LINUX_SWAP_GUID {cpu_to_le32(0x0657FD6D), cpu_to_le16(0xA4AB),\
	cpu_to_le16(0x43C4), cpu_to_be64(0x84E50933C84B4F4FULL)}
#define LINUX_DATA_GUID {cpu_to_le32(0xEBD0A0A2), cpu_to_le16(0xB9E5),\
	cpu_to_le16(0x4433), cpu_to_be64(0x87C068B6B72699C7ULL)}
#define LINUX_RAID_GUID {cpu_to_le32(0xA19D880F), cpu_to_le16(0x05FC),\
	cpu_to_le16(0x4D3B), cpu_to_be64(0xA006743F0F84911EULL)}
#define LINUX_LVM_GUID {cpu_to_le32(0xE6D6D379), cpu_to_le16(0xF507),\
	cpu_to_le16(0x44C2), cpu_to_be64(0xA23C238F2A3DF928ULL)}
#define APPLE_HFS_GUID {cpu_to_le32(0x48465300), cpu_to_le16(0x0000),\
	cpu_to_le16(0x11AA), cpu_to_be64(0xAA1100306543ECACULL)}
#define APPLE_UFS_GUID {cpu_to_le32(0x55465300), cpu_to_le16(0x0000),\
	cpu_to_le16(0x11AA), cpu_to_be64(0xAA1100306543ECACULL)}
#define APPLE_BOOT_GUID {cpu_to_le32(0x426F6F74), cpu_to_le16(0x0000),\
	cpu_to_le16(0x11AA), cpu_to_be64(0xAA1100306543ECACULL)}
#define SUN_BOOT_GUID {cpu_to_le32(0x6A82CB45), cpu_to_le16(0x1DD2),\
	cpu_to_le16(0x11B2), cpu_to_be64(0x99A6080020736631ULL)}
#define SUN_ROOT_GUID {cpu_to_le32(0x6A85CF4D), cpu_to_le16(0x1DD2),\
	cpu_to_le16(0x11B2), cpu_to_be64(0x99A6080020736631ULL)}

#define MBR2GUID(x) {cpu_to_le32(0x1575DA16), cpu_to_le16(0xF2E2),\
	cpu_to_le16(0x40DE), (cpu_to_be64(0xB715C6E376663B00ULL | x))}

struct part {
	unsigned char type;
	bool active;
	uint32_t start;
	uint32_t len;
};

struct mbrpart {
	unsigned char active;
	unsigned char shead; // CHS start value, not used by program
	unsigned char ssect; // CHS start value, not used by program
	unsigned char scyl; // CHS start value, not used by program
	unsigned char type;
	unsigned char ehead; // CHS end value, not used by program
	unsigned char esect; // CHS end value, not used by program
	unsigned char ecyl; // CHS end value, not used by program
	uint32_t start;
	uint32_t len;
}__attribute__((packed));

struct gptpart {
	struct __guid type;
	struct __guid id;
	uint64_t start;
	uint64_t end;
	uint64_t flags;
	char name[72];
}__attribute__((packed));

const static gptpart empty_record = {
	NULL_GUID,
	NULL_GUID,
	0,
	0,
	0,
	"",
};

struct gpthdr {
	unsigned char magic[8];
	unsigned char version[4];
	uint32_t hdrlen;
	uint32_t hdrsum;
	uint32_t pad;
	uint64_t this_hdr;
	uint64_t other_hdr;
	uint64_t data_start;
	uint64_t data_end;
	struct __guid guid;
	uint64_t first_entry;
	uint32_t entry_cnt;
	uint32_t entry_len;
	uint32_t part_sum;
	unsigned char padding[420]; // XXX 420 shouldn't be hardcoded!	
}__attribute__((packed));

vector<struct part> parts;

// table for CRC32 calculation, polynomial 0x04C11DB7
static uint32_t crc32_tbl[256] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
	0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	0x2d02ef8dL
};

/******************************************************************************\
* crc32: calculate an EFI-style CRC32 checksum                                 *
* buf: buffer holding the data to be CRCed                                     *
* len: length of the data                                                      *
\******************************************************************************/
uint32_t crc32(const unsigned char *buf, unsigned long len)
{
	uint32_t crc32val;
	
	crc32val = ~0L;
	for (int i = 0; i < len; i++)
		crc32val = crc32_tbl[(crc32val ^ buf[i]) & 0xff] ^ (crc32val >> 8);
	crc32val ^= ~0L;
	return crc32val;
}

/******************************************************************************\
* cmp: compare the starting offsets of two partitions                          *
* a, b: the partitions to be compared                                          *
* Primarily for internal use for sorting the partition vector.                 *
\******************************************************************************/
bool cmp(part a, part b)
{
	return a.start < b.start;
}

#ifdef WE_ARE_WINDOWS
/******************************************************************************\
* read_tbl: read an MSDOS-style partition table from a block of a device       *
* drive: filename of the device (e.g. \\.\physicaldrive0)                      *
* lba: logical address of the block to parse                                   *
* block_size: size of a block on the device                                    *
* buf: buffer to read data into                                                *
\******************************************************************************/
int read_tbl(string drive, uint64_t lba, int block_size, char *buf)
{
	HANDLE fin;
	DWORD writelen;
	LARGE_INTEGER offset;
	unsigned char tmpbuf[block_size];
	
	offset.QuadPart = lba*block_size;

	fin = CreateFile(drive.c_str(), GENERIC_READ,
					 FILE_SHARE_READ|FILE_SHARE_WRITE,
					 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (fin == INVALID_HANDLE_VALUE) {
		CloseHandle(fin);
		return -1;
	}
	
	SetFilePointerEx(fin, offset, NULL, FILE_BEGIN);
	ReadFile(fin, tmpbuf, block_size, &writelen, NULL);
	CloseHandle(fin);
	memcpy(buf, tmpbuf+446, 64);
	return 0;
}

/******************************************************************************\
* write_data: write blocks to a device                                         *
* drive: filename of the device (e.g. \\.\physicaldrive0)                      *
* lba: logical address of the first block to write                             *
* block_size: size of a block on the device                                    *
* buf: buffer holding the data to be written                                   *
* len: number of blocks to write                                               *
\******************************************************************************/
int write_data(string drive, uint64_t lba, int block_size, char *buf, int len)
{
	HANDLE fout;
	DWORD writelen;
	LARGE_INTEGER offset;
	
	offset.QuadPart = lba*block_size;

	fout = CreateFile(drive.c_str(), GENERIC_READ|GENERIC_WRITE,
					  FILE_SHARE_READ|FILE_SHARE_WRITE,
					  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (fout == INVALID_HANDLE_VALUE) {
		CloseHandle(fout);
		return -1;
	}
	
	SetFilePointerEx(fout, offset, NULL, FILE_BEGIN);
	WriteFile(fout, buf, len*block_size, &writelen, NULL);
	CloseHandle(fout);
	return 0;
}

/******************************************************************************\
* get_capacity: return the capacity of a drive in bytes, or 0 on error         *
* drive: filename of the device (e.g. \\.\physicaldrive0)                      *
\******************************************************************************/
uint64_t get_capacity(string drive)
{
	HANDLE fin;
	DWORD writelen;
	GET_LENGTH_INFORMATION capacity;
	
	fin = CreateFile(drive.c_str(), GENERIC_READ,
					 FILE_SHARE_READ|FILE_SHARE_WRITE,
					 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (fin == INVALID_HANDLE_VALUE) {
		CloseHandle(fin);
		return 0;
	}
	
	DeviceIoControl(fin, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &capacity,
					sizeof(GET_LENGTH_INFORMATION), &writelen, NULL);
	CloseHandle(fin);
	
	return capacity.Length.QuadPart;
}		
#else
/******************************************************************************\
* read_tbl: read an MSDOS-style partition table from a block of a device       *
* drive: filename of the device (e.g. /dev/hda or /dev/sda)                    *
* lba: logical address of the block to parse                                   *
* block_size: size of a block on the device                                    *
* buf: buffer to read data into                                                *
\******************************************************************************/
int read_tbl(string drive, uint64_t lba, int block_size, char *buf)
{
	ifstream fin;
#ifndef SIMPLE_LINUX_RW
	unsigned char tmpbuf[block_size];
#endif
	
	fin.open(drive.c_str(), ios_base::binary);
	if (!fin)
		return -1;
#ifdef SIMPLE_LINUX_RW
	fin.seekg((lba*block_size)+446);
	fin.read(buf, sizeof(struct mbrpart)*4);
#else
	fin.seekg(lba*block_size);
	fin.read(tmpbuf, block_size);
	memcpy(buf, tmpbuf+446, 64);
#endif
	fin.close();
	return 0;
}

/******************************************************************************\
* write_data: write blocks to a device                                         *
* drive: filename of the device (e.g. /dev/hda or /dev/sda)                    *
* lba: logical address of the first block to write                             *
* block_size: size of a block on the device                                    *
* buf: buffer holding the data to be written                                   *
* len: number of blocks to write                                               *
\******************************************************************************/
int write_data(string drive, uint64_t lba, int block_size, char *buf, int len)
{
	ofstream fout;
	
	fout.open(drive.c_str(), ios_base::binary);
	if (!fout)
		return -1;
	fout.seekp((lba*block_size));
	fout.write(buf, len);
	fout.close();
	return 0;
}


/******************************************************************************\
* get_capacity: return the capacity of a drive in bytes, or 0 on error         *
* drive: filename of the device (e.g. /dev/hda or /dev/sda)                    *
\******************************************************************************/
uint64_t get_capacity(string drive)
{
#ifdef BLKGETSIZE64
	uint64_t ret = 0;
#else
	uint32_t ret = 0;
#endif
	int fin = open(drive.c_str(), O_RDONLY);
	if (!fin)
		return 0;
	
#ifdef BLKGETSIZE64
	ioctl(fin, BLKGETSIZE64, &ret);
#else
	ioctl(fin, BLKGETSIZE, &numblocks);
#endif
	close(fin);
	
	return ret;
}

#if 0
uint64_t get_capacity(string drive)
{
	ifstream fin;
	uint64_t ret;
	
	fin.open(drive.c_str(), ios_base::binary);
	if (!fin)
		return 0;
	fin.seekg(0, ios::end);
	ret = fin.tellg();
	fin.close;
	return ret ? ret : 0;
}
#endif
#endif

/******************************************************************************\
* parse_tbl: parse an MSDOS-style partition table extracted from a boot record *
* curr: buffer holding the partition table data                                *
* curr_lba: logical address of the block holding the table being parsed        *
* first_ebr_lba: logical addr. of the first EBR on the drive, 0 if parsing MBR *
* return value if an EBR is found: logical address of the next EBR             *
* return value if no EBR is found: 0                                           *
\******************************************************************************/
uint32_t parse_tbl(struct mbrpart *curr,
						uint32_t curr_lba, uint32_t first_ebr_lba)
{
	struct part tmp;
	uint64_t ret = 0;

	for (int i = 0; i < 4; i++) {
		if (curr[i].type == 0x0f || curr[i].type == 0x05) {
			ret = first_ebr_lba + curr[i].start;
		}
		else if (curr[i].type != 0x00) {
			tmp.active = (curr[i].active == 0x80 ? true : false);
			tmp.type = curr[i].type;
			tmp.start = curr[i].start + curr_lba;
			tmp.len = curr[i].len;
			parts.push_back(tmp);
		}
	}
	return ret;
}

/******************************************************************************\
* usage: print usage information.                                              *
* name: name of the program, call with argv[0]                                 *
\******************************************************************************/
void usage(char *name)
{
	cout << "Usage: " << endl;
	cout << "Write to separate files: " << name << " <device_path>" << endl;
	cout << "Write directly to disk: " << name << " -w <device_path>" << endl;
	cout << "where device_path is the full path to the device file," << endl;
	cout << "e.g. /dev/hda or \\\\.\\physicaldrive0." << endl;
	cout << name << " -h or " << name << " --help displays this help." << endl;
	return;
}

/******************************************************************************\
* main: do the actual conversion from MBR to GPT                               *
\******************************************************************************/
int main(int argc, char *argv[])
{
	ofstream fout;
	char buf[8];
	struct mbrpart curr[4];
	vector<struct gptpart> gptparts;
	struct gptpart gpttable[128]; // XXX shouldn't hardcode 128 here!
	string drive;
	uint64_t disk_len;
	uint32_t first_ebr = 0, curr_ebr = 0;
	bool write = false, badlayout = false, boot = false;

	memset((void *)curr, 0, 64);
	
	cout << argv[0] << ": Convert an MBR partition table "
		 << "to a GPT one." << endl;
	cout << endl;
	
	// XXX The command-line parsing code has room for improvements...
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-w")) {
			write = true;
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			usage(argv[0]);
			return EXIT_SUCCESS;
		} else if (argv[i][0] == '-') {
			usage(argv[0]);
			cout << argv[0] << ": Invalid argument: " << argv[i] << "." << endl;
			return EXIT_FAILURE;
		} else {
			drive = argv[i];
		}
	}
	
	if (argc <= 1) {
		usage(argv[0]);
		return EXIT_SUCCESS;
	}
	
	if (argc > 3 || (argc == 3 && write == false)) {
		usage(argv[0]);
		cout << argv[0] << ": Too many arguments (" << argc << ")." << endl;
		return EXIT_FAILURE;
	}
	
	if (!drive.length()) {
		usage(argv[0]);
		cout << argv[0] << ": No drive specified." << endl;
		return EXIT_FAILURE;
	}
	
	// read and parse the MBR
	if (read_tbl(drive, curr_ebr, 512, (char *)curr) < 0) {
		cout << "Block read failed, check permissions!" << endl;
		return EXIT_FAILURE;
	}
	first_ebr = parse_tbl(curr, 0, 0);
	curr_ebr = first_ebr;
	
	// read and parse the EBR chain, if present
	while (curr_ebr > 0) {
		if (read_tbl(drive, curr_ebr, 512, (char *)curr) < 0) {
			cout << "Block read failed, check permissions!" << endl;
			return EXIT_FAILURE;
		}
		curr_ebr = parse_tbl(curr, curr_ebr, first_ebr);
	};
	
	disk_len = get_capacity(drive)/512; //XXX Block size is not necessarily 512!
	
	if (!disk_len) {
		cout << "Unable to auto-determine the capacity of the disk." << endl;
		cout << "Please enter the LBA capacity by hand to continue. " << endl
			 << ">";
		cin >> disk_len;
	}
	
	if (parts.size() && parts[0].start < 34) {
		cout << "Not enough space at the beginning of the disk (need at least"
			 << endl << "34 sectors before the start of the first partition)."
			 << endl << "Re-partition the disk to meet this requirement, and"
			 << "run this utility again." << endl;
		badlayout = true;
	}
	
	if (parts.size() &&
		parts[parts.size()-1].start + parts[parts.size()-1].len > disk_len-34) {
		if (badlayout) cout << endl;
		cout << "Not enough space at the end of the disk (need at least"
			 << endl << "34 sectors after the end of the last partition)."
			 << endl << "Re-partition the disk to meet this requirement, and"
			 << "run this utility again." << endl;
		badlayout = true;
	}
	
	if (badlayout)
		return EXIT_FAILURE;
	
	sort(parts.begin(), parts.end(), cmp);
	
	for (int i = 0; i < parts.size(); i++) {
		struct gptpart gptout;
		
		cout << "Boot: " << parts[i].active << ", Type: 0x" 
			 << hex << (int)parts[i].type << dec
			 << ", Start: sector " << parts[i].start
			 << ", Length: " << parts[i].len << " sectors" << endl;
		if (parts[i].active) boot = true;
		{
			__guid gtmp = NULL_GUID;
			gptout.id = gtmp;
		}
		gptout.flags = 0;
		switch (parts[i].type) {
		case 0x11:
		case 0x14:
		case 0x16:
		case 0x17:
		case 0x1B:
		case 0x1C:
		case 0x1E:
		case 0xBB: // MS partition hidden by Acronis OS selector
		case 0xBC: // Acronis Secure Zone, in fact hidden FAT32
		case 0xFE:
			gptout.flags |= cpu_to_le64(PART_FLAG_HIDDEN);
		case 0x01:
		case 0x04:
		case 0x06:
		case 0x07:
		case 0x0B:
		case 0x0C:
		case 0x0E:
			{
				__guid gtmp = MS_DATA_GUID;
				gptout.type = gtmp;
			}
			break;
		case 0x27:
			{
				__guid gtmp = MS_WINRE_GUID;
				gptout.type = gtmp;
			}
			break;
		case 0x3C:
			cout << "ERROR: PartitionMagic work partition (ID 0x3C) detected." << endl 
				 << "This is a sign of an interrupted PartitionMagic session." << endl
				 << "Correct this error, and run this utility again." << endl;
			return EXIT_FAILURE;
		case 0x42:
			cout << "FATAL: Dynamic disk detected. Support for dynamic disks is" << endl
				 << "not yet implemented. Writing a GPT to a dynamic disk is" << endl
				 << "dangerous. Operation aborted." << endl;
			return EXIT_FAILURE;
#if 0
			/*
			 * TODO: Find the metadata table at the end of the disk, and make it
			 * into an MS_META_GUID partition. This will probably require moving
			 * the metadata table to a different location on the disk. This may
			 * well be beyond the scope of this tool, but patches are welcome.
			 */
			{
				__guid gtmp = MS_DYN_GUID;
				gptout.type = gtmp;
			}
#endif
		case 0xC3:
			gptout.flags |= cpu_to_le64(PART_FLAG_HIDDEN);
		case 0x82:
			{
				__guid gtmp = LINUX_SWAP_GUID;
				gptout.type = gtmp;
			}
			break;
		case 0x93:
		case 0xC2:
			gptout.flags |= cpu_to_le64(PART_FLAG_HIDDEN);
		case 0x81: // XXX not sure if this is correct...
		case 0x83:
			{
				__guid gtmp = LINUX_DATA_GUID;
				gptout.type = gtmp;
			}
			break;
		case 0x86:
		case 0xFD:
			{
				__guid gtmp = LINUX_RAID_GUID;
				gptout.type = gtmp;
			}
			break;
		case 0x8E:
			{
				__guid gtmp = LINUX_LVM_GUID;
				gptout.type = gtmp;
			}
			break;
		case 0xA8:
			{
				__guid gtmp = APPLE_UFS_GUID;
				gptout.type = gtmp;
			}
			break;
		case 0xAB:
			{
				__guid gtmp = APPLE_BOOT_GUID;
				gptout.type = gtmp;
			}
			break;
		case 0xAF:
			{
				__guid gtmp = APPLE_HFS_GUID;
				gptout.type = gtmp;
			}
			break;
		case 0xBE:
			{
				__guid gtmp = SUN_BOOT_GUID;
				gptout.type = gtmp;
			}
			break;
		case 0xBF:
			{
				__guid gtmp = SUN_ROOT_GUID;
				gptout.type = gtmp;
			}
			break;
		case 0xEE: // protective MBR
			cout << "ERROR: This drive already has a GUID partition table." << endl 
				 << "There is no need to run this utility on this drive again." << endl;
			return EXIT_FAILURE;
		case 0xEF:
			{
				__guid gtmp = EFI_SYS_GUID;
				gptout.type = gtmp;
			}
			break;
		default:
			cout << "WARNING: Unknown partition type in record " << i
			<< " (" << hex << (int)parts[i].type << dec << ")." << endl;
			cout << "A generic GUID will be used." << endl;
		fallback:
			{
				__guid gtmp = MBR2GUID(parts[i].type);
				gptout.type = gtmp;
			}
		}
		{
			__guid gtmp = NULL_GUID;
			gptout.id = gtmp;
		}
		gptout.start = cpu_to_le64((uint64_t)le32_to_cpu(parts[i].start));
		gptout.end = cpu_to_le64(((uint64_t)le32_to_cpu(parts[i].start) +
								 (uint64_t)le32_to_cpu(parts[i].len) - 1));
		memset((void *)gptout.name, 0x20, 72);
		gptparts.push_back(gptout);
	}
	
	if (boot)
		cout << endl << "WARNING: Boot partition(s) found. This tool cannot "
			 << "guarantee that" << endl << "such partitions will remain "
			 << "bootable after conversion." << endl;
			 
	cout << endl;
	
	/* Generate a complete partition array */
	for (int i = 0; i < parts.size(); i++)
		gpttable[i] = gptparts[i];
	for (int i = parts.size(); i < 128; i++)
		gpttable[i] = empty_record;
	
	int table_crc = crc32((unsigned char *)gpttable,
						  sizeof(gptpart) * 128);
	
	struct gpthdr hdr1 = {
		GPT_MAGIC,
		GPT_V1,
		cpu_to_le32(92),
		0,
		0,
		cpu_to_le64(1ULL),
		cpu_to_le64(disk_len-1),
		cpu_to_le64(34ULL),
		cpu_to_le64(disk_len-34),
		NULL_GUID,
		cpu_to_le64(2ULL),
		cpu_to_le32(128), // total number of records, not only used ones
		cpu_to_le32(128),
		table_crc,
	};
	
	struct gpthdr hdr2 = {
		GPT_MAGIC,
		GPT_V1,
		cpu_to_le32(92),
		0,
		0,
		cpu_to_le64(disk_len-1),
		cpu_to_le64(1ULL),
		cpu_to_le32(34ULL),
		cpu_to_le64(disk_len-34),
		NULL_GUID,
		cpu_to_le64(disk_len-33),
		cpu_to_le32(128), // total number of records, not only used ones
		cpu_to_le32(128),
		table_crc,
	};
	
	memset((void *)hdr1.padding, 0, 420);
	memset((void *)hdr2.padding, 0, 420);
	
	hdr1.hdrsum = cpu_to_le32(crc32((unsigned char *)&hdr1, 92));
	hdr2.hdrsum = cpu_to_le32(crc32((unsigned char *)&hdr2, 92));
	
	struct mbrpart prot_mbr = {
		0,
		0,
		2,
		0,
		0xEE,
		0xFF,
		0xFF,
		0xFF,
		1,
		(disk_len-1 < 0xFFFFFFFF) ? disk_len-1 : 0xFFFFFFFF,
	};
	
	if (write) { // FIXME Write-to-disk could be improved...
		cout << "Writing primary GPT and protective MBR to LBA address 0..." << endl;
		
		char outbuf[20000];
		
		memset((char *)outbuf, 0, 20000);
		
		memcpy((char *)outbuf+446, (char *)&prot_mbr, sizeof(struct mbrpart));
		outbuf[510] = 0x55;
		outbuf[511] = 0xAA;
		memcpy((char *)outbuf+512, (char *)&hdr1, sizeof(struct gpthdr));
		memcpy((char *)outbuf+1024, (char *)gpttable, sizeof(gpttable));
		if (write_data(drive, 0, 512, outbuf, 34) < 0) {
			cout << "Failed to write primary GPT!" << endl;
			return EXIT_FAILURE;
		}
		
		cout << "Writing secondary GPT to LBA address " << disk_len-33 << "..." << endl;
		memset((char *)outbuf, 0, 20000);
		memcpy((char *)outbuf, (char *)gpttable, sizeof(gpttable));
		memcpy((char *)outbuf+sizeof(gpttable), (char *)&hdr1, sizeof(struct gpthdr));
		if (write_data(drive, disk_len-33, 512, (char *)outbuf, 33) < 0) {
			cout << "Failed to write secondary GPT!" << endl;
			return EXIT_FAILURE;
		}
		cout << "Success!" << endl;
	} else {
		cout << "Writing primary GPT and protective MBR to primary.img..." << endl;
		fout.open("primary.img", ios_base::binary);
		for (int i = 0; i < 446; i++)
			fout << '\0';
		fout.write((char *)&prot_mbr, sizeof(struct mbrpart));
		for (int i = 0; i < 48; i++)
			fout << '\0';
		fout << (char)0x55 << (char)0xAA;
		fout.write((char *)&hdr1, sizeof(struct gpthdr));
		fout.write((char *)gpttable, sizeof(gpttable));
		fout.close();

		cout << "Writing secondary GPT to secondary.img..." << endl;
		fout.open("secondary.img", ios_base::binary);
		fout.write((char *)gpttable, sizeof(gpttable));
		fout.write((char *)&hdr2, sizeof(struct gpthdr));
		fout.close();

		cout << "Success!" << endl;
		cout << "Write primary.img to LBA address 0." << endl;
		cout << "Write secondary.img to LBA address " << disk_len-33 << "." << endl;
	}
	
    return EXIT_SUCCESS;
}