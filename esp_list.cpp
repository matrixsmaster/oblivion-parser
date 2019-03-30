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

#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <unordered_map>
#include <set>
#include <list>
#include <algorithm>
#include <functional>
#include "esp_list.h"
#include "libtes4vfs.h"

#ifndef TES4LIB_USE_VFS
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#endif

using namespace std;
namespace TES4 {

int load_esp_filelist(string const &listfn, string const &gamedir, esplist &files, VFS* vfs, VFSProgressCb prog_cb)
{
	if (listfn.empty()) return 0;
	vector<string> flist;
	char buf[256];

	//Let's open plugins list file
	MTFILE ff = MFTOPEN(listfn.c_str(),"r");
	if (!ff) return 0;

	//Read out whole plugins list file
	while (MFGETS(buf,sizeof(buf),ff))
		if (strlen(buf) > 2 && !strchr(buf,'#')) { //ignore comment strings
			//trail CR LF
			while (buf[strlen(buf)-1] == '\r' || buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = 0;
			//add file name to list
			flist.push_back(buf);
		}

	//We're done with the list
	MFCLOSE(ff);

	//Now we can actually LOAD the list contents
	return load_esp_filelist(flist,gamedir,files,vfs,prog_cb);
}

int load_esp_filelist(vector<string> const &flist, string gamedir, esplist &files, VFS* vfs, VFSProgressCb prog_cb)
{
	//Our basic names holders
	int files_total = 0; //total number of files loaded
	multimap<long int,MyESPEntry> esm_map,esp_map; //they would be sorted by modification timestamp

#ifndef TES4LIB_USE_VFS
	struct stat st; //file stat
#else
	size_t pdate = 0;
	if (!vfs) return 0;
#endif

	//Gather file data
	for (auto buf : flist) {
		//combine file name
		if (gamedir[gamedir.length()-1] != '/') gamedir += '/';
		buf = gamedir + buf;

#ifndef TES4LIB_USE_VFS
		//check file's stats
		if (stat(buf.c_str(),&st)) {
			cout << "\tNot found!" << endl;

		} else {
			MyESPEntry rec;
			rec.name = buf;
//			cout << "\tmtime = " << st.st_mtim.tv_sec << endl;

			//check whether the file is primary master, secondary master or regular plugin
			if (equ_ignorecase(rec.name,"Oblivion.esm"))
				files.push_back(rec); //primary master
			else if (equ_ignorecase(rec.name.substr(rec.name.size()-4),".esm"))
				esm_map.insert(pair<long int,MyESPEntry>(st.st_mtim.tv_sec,rec)); //secondary master
			else
				esp_map.insert(pair<long int,MyESPEntry>(st.st_mtim.tv_sec,rec)); //regular plugin
		}
#else
		MyESPEntry rec;
		rec.name = buf;
		//check whether the file is primary master, secondary master or regular plugin
		if (equ_ignorecase(rec.name,"Oblivion.esm"))
			files.push_back(rec); //primary master
		else if (equ_ignorecase(rec.name.substr(rec.name.size()-4),".esm"))
			esm_map.insert(pair<long int,MyESPEntry>(pdate++,rec)); //secondary master
		else
			esp_map.insert(pair<long int,MyESPEntry>(pdate++,rec)); //regular plugin
#endif
	}

	bool basepresent = !files.empty(); //if Oblivion.esm is here, don't pre-increment PlugID
	short plugid = basepresent? 0:1; //PluginID byte

	//Compile all maps and stuff into one CORRECT load order list
	for (auto &&i : files) cout << i.name << endl;
	for (auto &&i : esm_map) files.push_back(i.second);
	for (auto &&i : esp_map) files.push_back(i.second);

	//Discard temporary maps
	esm_map.clear();
	esp_map.clear();

	//Start loading process
	MFILE ff; //current file handle
	for (auto &&i : files) {
		cout << "DEBUG: Loading " << i.name << endl;
		fflush(stdout);
		ff = MFOPEN(i.name.c_str(),"rb");
		assert(ff);
		i.data = read_esp(ff);
		MFCLOSE(ff);
		i.plugid = plugid++;
		files_total++;
		if (prog_cb) prog_cb(files_total,files.size());
	}

	return files_total;
}

void unload_esp_filelist(esplist &files)
{
	for (auto &&i : files) clear_esp(i.data);
	files.clear();
}

}; //TES4
