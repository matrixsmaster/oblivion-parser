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

#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "zlib.h"
#include "bsa_parser.h"
#include "libtes4vfs.h"

using namespace std;
namespace TES4 {

static string readbzstring(MFILE f)
{
	string out;
	uint8_t len = 0;
	if (!MFREAD(&len,1,1,f)) return out;
	out.resize(len);
	if (MFREAD(&(out[0]),1,len,f) != len) return string();
	out.pop_back();
	out.shrink_to_fit();
	return out;
}

static string readzstring(MFILE f)
{
	string out;
	char c = 0;
	do {
		if (!MFREAD(&c,1,1,f)) return string();
		if (c) out += c;
	} while (c);
	return out;
}

static int extract_zip_data(BSAFileInfo wht, MFILE from, unsigned char** to)
{
	if (!from || !to || !wht.size) return -1;

	uint32_t fin_len, in_len = (wht.size - 4);
	MFSEEK(from,wht.off,SEEK_SET);
	if (!MFREAD(&fin_len,sizeof(fin_len),1,from))
		return -10;

	unsigned char* rdbuf = new unsigned char[in_len];
	if (!MFREAD(rdbuf,in_len,1,from)) {
		delete[] rdbuf;
		return -11;
	}

	z_stream strm;
	memset(&strm,0,sizeof(strm));
	if (inflateInit(&strm) != Z_OK) {
		delete[] rdbuf;
		return -2;
	}

	*to = new unsigned char[fin_len];

	strm.avail_in = in_len;
	strm.next_in = rdbuf;
	strm.avail_out = fin_len;
	strm.next_out = *to;

	int r = inflate(&strm,Z_FINISH);
	inflateEnd(&strm);
	delete[] rdbuf;

	if (r != Z_STREAM_END) {
		delete[] (*to);
		return -3;
	}

	return fin_len;
}

static int extract_data(BSAFile fl, MFILE from, unsigned char** buf)
{
	int r;

	if (fl.compress) {
		r = extract_zip_data(fl.inf,from,buf);
		if (r <= 0) return r;

	} else {
		*buf = new unsigned char[fl.inf.size];
		MFSEEK(from,fl.inf.off,SEEK_SET);
		if (!MFREAD(*buf,fl.inf.size,1,from)) {
			delete[] (*buf);
			return -20;
		}
		r = fl.inf.size;
	}

	return r;
}

BSA::BSA(const char* fn, void* vfs)
{
	MFILE bf = MFOPEN(fn,"rb");
	assert(bf);

	assert(MFREAD(&hdr,sizeof(hdr),1,bf));

	if (strncmp(hdr.fileid,"BSA",3)) {
		printf("ERROR: not a BSA!\n");
		return;
	}

	printf("Header:\n\tVer %u\n\tOff %u\n\tArchive Flags 0x%08X\n\tFolders %u\n\tFiles %u\n\tLen1 %u\n\tLen2 %u\n\tFile Flags 0x%08X\n",
		hdr.ver,hdr.off,hdr.aflags,hdr.dirs,hdr.files,hdr.totalFolderNames,hdr.totalFileNames,hdr.fflags);

	for (size_t i = 0; i < hdr.dirs; i++) {
		BSADirInfo dir;
		assert(MFREAD(&dir,sizeof(dir),1,bf));
//		printf("Folder %lu: hash = %016lX; contains %u files @ 0x%08X\n",i,dir.hash,dir.qty,dir.off);
		BSADir ddir;
		ddir.inf = dir;
		cont.push_back(ddir);
	}

	for (size_t i = 0; i < hdr.dirs; i++) {
		if (hdr.aflags & BSA_HASDIRNAMES) cont[i].name = readbzstring(bf);
//		cout << cont[i].name << endl;

		for (size_t j = 0; j < cont[i].inf.qty; j++) {
			BSAFileInfo fl;
			assert(MFREAD(&fl,sizeof(fl),1,bf));
//			printf("\tFile %lu: hash = %016lX; %u bytes @ 0x%08X\n",j,fl.hash,fl.size,fl.off);
			BSAFile fil;
			fil.inf = fl;
			fil.compress = hdr.aflags & BSA_COMPRESSED;
			fil.cont = fn;
			if (fl.size & BSA_SIZEKLUDGE) {
				fil.compress = !fil.compress;
				fil.inf.size &= ~BSA_SIZEKLUDGE;
//				debug("BSA 'compression-flag-in-size-field' kludge detected\n");
			}
			cont[i].files.push_back(fil);
			filecnt++;
		}
	}

	if (hdr.aflags & BSA_HASFILENAMES) {
		size_t n = 0, m = 0;

		for (size_t i = 0; i < filecnt; i++) {
			string nm = readzstring(bf);
//			cout << nm << endl;

			if (cont.size() > n) {
				if (cont[n].inf.qty <= m) {
					n++;
					m = 0;
				}
				cont[n].files[m].name = nm;
				m++;
			}
		}
	}

	debug("" PRIszT " files loaded from %s\n",filecnt,fn);

	MFCLOSE(bf);
	remap();
	filename = fn;
	error = false;
}

string BSA::lowercase(string in)
{
	string r;
	for (auto &&c : in) r += tolower(c);
	return r;
}

void BSA::remap()
{
	fmap.clear();
	for (auto &&i : cont)
		for (auto &&j : i.files) {
			string fullname = lowercase(i.name + "\\" + j.name);
			fmap[fullname] = &j;
		}
}

bool BSA::addSource(const char* fn, void* vfs)
{
	BSA* nwa = new BSA(fn,vfs);
	if (!nwa || nwa->isFailed()) {
		if (nwa) delete nwa;
		return false;
	}

	cont.insert(cont.end(),nwa->cont.begin(),nwa->cont.end());
	filecnt += nwa->getNumFiles();
	filename += ";" + nwa->getBSAFileName();

	debug("BSA (%s) now contains " PRIszT " files\n",filename.c_str(),filecnt);

	delete nwa;
	remap();
	return true;
}

vector<uint8_t> BSA::getFile(string fn, void* vfs)
{
	vector<uint8_t> res;

	fn = lowercase(fn);
	if (!fmap.count(fn)) {
		printf("WARNING: file '%s' not found in '%s'\n",fn.c_str(),filename.c_str());
		return res;
	}

	MFILE bf = MFOPEN(fmap[fn]->cont.c_str(),"rb");
	if (!bf) {
		printf("ERROR: unable to open file '%s' second time\n",fmap[fn]->cont.c_str());
		return res;
	}

	unsigned char* buf;
	int r = extract_data(*(fmap[fn]),bf,&buf);
	if (r < 0 || !buf) {
		printf("ERROR: unable to read data for '%s'\n",fn.c_str());
		return res;
	}

	res.resize(r);
	memcpy(&(res[0]),buf,r);
	delete[] buf;

	MFCLOSE(bf);
	return res;
}

}; //TES4
