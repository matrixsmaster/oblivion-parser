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

#ifndef BSA_PARSER_H_
#define BSA_PARSER_H_

#include <inttypes.h>
#include <vector>
#include <string>
#include <list>
#include <map>
#include "dt.h"

#ifdef TES4LIB_USE_VFS
#include "vfshelper.h"
#endif

namespace TES4 {

enum BSAArchiveFlags {
	BSA_HASDIRNAMES = 0x01,
	BSA_HASFILENAMES = 0x02,
	BSA_COMPRESSED = 0x04,
	BSA_INMEM = 0x08,
	BSA_SNDMEM = 0x10,
	BSA_HOLDOFF = 0x20,
	BSA_XBOX = 0x40,
	BSA_UNK1 = 0x80,
	BSA_UNK2 = 0x100,
	BSA_UNK3 = 0x200,
	BSA_UNK4 = 0x300
};

enum BSAFileFlags {
	BSA_MESHES = 0x01,
	BSA_TEXTURES = 0x02,
	BSA_MENUS = 0x04,
	BSA_SOUNDS = 0x08,
	BSA_VOICES = 0x10,
	BSA_SHADERS = 0x20,
	BSA_TREES = 0x40,
	BSA_FONTS = 0x80,
	BSA_MISC = 0x100
};

#define BSA_SIZEKLUDGE (1<<30)

struct BSAHeader {
	char fileid[4];
	uint32_t ver;
	uint32_t off;
	uint32_t aflags;
	uint32_t dirs;
	uint32_t files;
	uint32_t totalFolderNames;
	uint32_t totalFileNames;
	uint32_t fflags;
};

struct BSADirInfo {
	uint64_t hash;
	uint32_t qty;
	uint32_t off;
};

struct BSAFileInfo {
	uint64_t hash;
	uint32_t size;
	uint32_t off;
};

struct BSAFile {
	BSAFileInfo inf;
	std::string name;
	bool compress;
	std::string cont;
};

struct BSADir {
	BSADirInfo inf;
	std::string name;
	std::vector<BSAFile> files;
};

class BSA {
private:
	BSAHeader hdr;
	std::vector<BSADir> cont;
	std::map<std::string,BSAFile*> fmap;
	size_t filecnt = 0;
	std::string filename;
	bool error = true;

	void remap();

public:
	BSA(const char* fn, void* vfs = NULL);
	virtual ~BSA() {}

	static std::string lowercase(std::string in);

	bool isFailed()									{ return error; }
	std::string getBSAFileName()					{ return filename; }
	size_t getNumFiles()							{ return filecnt; }

	bool addSource(const char* fn, void* vfs = NULL);
	std::vector<uint8_t> getFile(std::string fn, void* vfs = NULL);
};

}; //TES4

#endif /* BSA_PARSER_H_ */
