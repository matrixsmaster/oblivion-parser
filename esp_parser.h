/*
 * TES4 data file library.
 * (C) Dmitry 'MatrixS_Master' Solovyev, 2015-2019. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef ESP_PARSER_H_
#define ESP_PARSER_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <list>
#include "zlib.h"

#ifdef TES4LIB_USE_VFS
#include "vfshelper.h"
#endif

namespace TES4 {

#define DEBUG_PARSE 0
#define DEBUG_DUMP 0
#define USE_ZLIB 1
#define OBLIVION_ZLIB_LEVEL 6

enum {
	REC_FLG_ESM = 0x00000001, 	/* 	ESM file. (TES4.HEDR record only.) */
	REC_FLG_DEL = 0x00000020, 	/* 	Deleted */
	REC_FLG_SHD = 0x00000200, 	/* 	Casts shadows */
	REC_FLG_QST = 0x00000400, 	/* 	Quest item / Persistent reference */
	REC_FLG_DIS = 0x00000800, 	/* 	Initially disabled */
	REC_FLG_IGN = 0x00001000, 	/* 	Ignored */
	REC_FLG_LOD = 0x00008000, 	/* 	Visible when distant */
	REC_FLG_OFF = 0x00020000, 	/* 	Dangerous / Off limits (Interior cell) */
	REC_FLG_ZIP = 0x00040000, 	/* 	Data is compressed */
	REC_FLG_CWT = 0x00080000, 	/*	Can't wait */
};

struct TES4SubRecord {
	char subType[4];
	uint16_t dataSize;
};

struct MySubRecord {
	TES4SubRecord rec;
	uint32_t decompLen = 0;
	uint32_t kludgeSize = 0;
	bool dontCompress = false;
	std::vector<uint8_t> data;
	
	MySubRecord() {
		memset(&rec,0,sizeof(rec));
	}
};

struct TES4Record {
	char type[4];
	uint32_t dataSize;
	uint32_t flags;
	uint32_t formID;
	uint32_t version;
};

struct MyRecord {
	TES4Record rec;
	std::vector<MySubRecord> data;
	
	MyRecord() {
		memset(&rec,0,sizeof(rec));
	}
};

struct TES4Group {
	char type[4];
	uint32_t groupSize;
	char label[4];
	int32_t groupType;
	uint32_t stamp;
};

struct MyGroupRecord;
struct MyGroup {
	TES4Group grp;
	std::vector<MyGroupRecord> data;
	
	MyGroup() {
		memset(&grp,0,sizeof(grp));
	}
};

struct MyGroupRecord {
	bool isGroup;
	union {
		MyGroup* grp;
		MyRecord* rec;
	} data;
};

struct MyESP {
	std::list<MyRecord> recs;
	std::list<MyGroup> grps;
};

#ifdef TES4LIB_USE_VFS
MyESP read_esp(VBFILE* esp);
void write_esp(MyESP &data, VBFILE* esp);
#else
MyESP read_esp(FILE* esp);
void write_esp(MyESP &data, FILE* esp);
#endif
void clear_esp(MyESP &data);

}; //TES4

#endif /*ESP_PARSER_H_*/
