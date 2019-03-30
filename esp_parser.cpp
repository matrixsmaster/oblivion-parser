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

#include "esp_parser.h"
#include "libtes4vfs.h"

using namespace std;
namespace TES4 {

/* WARNING:
 * This code is still unfinished. There are at least two known bugs:
 * 1) If compressed data length > source data length (fool's compression), compression will fail.
 * 2) Bethesda's 64K kludge is implemented for reading only
 * */

#if DEBUG_PARSE || DEBUG_DUMP
static void print4(void* ptr)
{
	char buf[5] = {0,0,0,0,0};
	memcpy(buf,ptr,4);
	cout << buf;
}
#endif

int extract_zip_subrecords(vector<MySubRecord>* to, MFILE from, int to_read, unsigned fin_len)
{
	unsigned char* rdbuf = new unsigned char[to_read];
	assert((int)MFREAD(rdbuf,1,to_read,from) == to_read);
	
	z_stream strm;
	memset(&strm,0,sizeof(strm));
	//if (inflateInit(&strm) != Z_OK) return -1;
	assert(inflateInit(&strm) == Z_OK);
	
	unsigned char* outbuf = new unsigned char[fin_len];
	
	strm.avail_in = to_read;
	strm.next_in = rdbuf;
	strm.avail_out = fin_len;
	strm.next_out = outbuf;
	int r = inflate(&strm,Z_FINISH);
	assert(r == Z_STREAM_END);
	
	inflateEnd(&strm);
	delete[] rdbuf;
	
#if DEBUG_PARSE
	cout << "Subrecords block inflated" << endl;
#endif

	unsigned char* ptr = outbuf;
	for (unsigned l = 0; l < fin_len;) {
		MySubRecord srec;
		//memset(&srec,0,sizeof(srec));
		
		memcpy(&(srec.rec),ptr,sizeof(srec.rec));
		ptr += sizeof(srec.rec);

		if (srec.rec.dataSize) {
			srec.data.resize(srec.rec.dataSize);
			memcpy(&(srec.data[0]),ptr,srec.rec.dataSize);
			ptr += srec.rec.dataSize;
					
			l += srec.rec.dataSize;
			l += sizeof(TES4SubRecord);
			to->push_back(srec);
		}
		
#if DEBUG_PARSE
		cout << "Sub-record ";
		print4(&(srec.rec));
		if (!strncmp(srec.rec.subType,"EDID",4)) {
			uint8_t* ptr1 = &(srec.data[0]);
			cout << " (" << ptr1 << ")";
		}
		cout << " size " << srec.rec.dataSize << endl;
#endif
	}

	delete[] outbuf;
	return 1;
}

int read_next(MFILE esp, MyGroup** grp, MyRecord** rcp)
{
	char buf[5];
	buf[4] = 0;
	*grp = NULL;
	*rcp = NULL;
	
	//read header type
	if (MFREAD(buf,1,4,esp) < 4) return -1;
	
	//rewind back to record begin and store file position
	MFSEEK(esp,-4,SEEK_CUR);
	size_t start = MFTELL(esp);
	
	//determine next action
	if (!strcmp(buf,"GRUP")) {
#if DEBUG_PARSE
		cout << "Group" << endl;
#endif
		MyGroup* gr = new MyGroup();
		*grp = gr;
		
		//read the group header block
		assert(MFREAD(&(gr->grp),sizeof(gr->grp),1,esp));
#if DEBUG_PARSE
		cout << "Size " << gr->grp.groupSize << endl;
#endif

		//while length remainder is positive, read gorup's data block
		for (unsigned l = 0; l < gr->grp.groupSize - sizeof(TES4Group);) {
			MyGroup* pgr;
			MyRecord* prc;
			
			//recursive read of embedded block(s)
			int r = read_next(esp,&pgr,&prc);
			if (r < 1) break;
			assert(pgr || prc);
			
			//create this group's record in our tree
			MyGroupRecord mgr;
			//memset(&mgr,0,sizeof(mgr));
			if (pgr) {
				mgr.isGroup = true;
				mgr.data.grp = pgr;
			} else {
				mgr.isGroup = false;
				mgr.data.rec = prc;
			}
			gr->data.push_back(mgr);
			
			//advance (inside this group)
			l += r;
		}
		
	} else {
		//That's a regular record
#if DEBUG_PARSE
		cout << "Record " << buf << endl;
#endif

		MyRecord* rc = new MyRecord();
		*rcp = rc;
		
		//read record's header
		assert(MFREAD(&(rc->rec),sizeof(rc->rec),1,esp));
#if DEBUG_PARSE
		cout << "Size " << rc->rec.dataSize << endl;
#endif

		//read record's body
		for (unsigned l = 0; l < rc->rec.dataSize;) {
			MySubRecord srec;
			//memset(&srec,0,sizeof(srec));
			bool skip = false;
			
			if ((rc->rec.flags & REC_FLG_ZIP) == 0) {
				
				//read "normal" sub-record data (without zlib stuff)
				assert(MFREAD(&(srec.rec),sizeof(srec.rec),1,esp));
#if DEBUG_PARSE
				memcpy(buf,&(srec.rec.subType),4);
				cout << "Sub-record " << buf << " size " << srec.rec.dataSize << endl;
#endif

				//sub-record can have a zero length
				if (srec.rec.dataSize) {
					srec.data.resize(srec.rec.dataSize);
					assert(MFREAD(&(srec.data[0]),srec.rec.dataSize,1,esp));
					
					//advance inside record's body
					l += srec.rec.dataSize;
					
				} else {
					//The Kludge of Bethesda (are they really was so desperate??)
					if (!rc->data.empty()) {
						if (!strncmp((rc->data.end()-1)->rec.subType,"XXXX",4)) {
#if DEBUG_PARSE
							cout << "Bethesda's kludge detected!" << endl;
#endif
							uint32_t* ulptr = (uint32_t*)(&((rc->data.end()-1)->data[0])); //ehww
#if DEBUG_PARSE
							cout << "Real size is " << *ulptr << endl;
#endif
							srec.kludgeSize = *ulptr;
							
							srec.data.resize(srec.kludgeSize);
							assert(MFREAD(&(srec.data[0]),srec.kludgeSize,1,esp));
							
							l += srec.kludgeSize;
						}
					}
				}
				
				//don't forget to advance by size of current sub-record's header length
				l += sizeof(TES4SubRecord);
				
			} else {
				//Well, zlib stuff - read 4 bytes of decompressed length field
				assert(MFREAD(&(srec.decompLen),sizeof(srec.decompLen),1,esp));
#if DEBUG_PARSE
				cout << "Sub-record compressed with inflated len = " << srec.decompLen << endl;
#endif

				//calculate remainder size
				int nsize = rc->rec.dataSize;
				nsize -= sizeof(srec.decompLen);
				if (nsize > 0) {
#if USE_ZLIB
					//pass zipped sub-records into extractor
					assert(extract_zip_subrecords(&(rc->data),esp,nsize,srec.decompLen) > 0);
					skip = true;
#else
					//or just blindly read'em out and save as-is
					srec.data.resize(nsize);
					assert(MFREAD(&(srec.data[0]),nsize,1,esp));
					srec.dontCompress = true; //next time we'll not compress them "back" - because we haven't decompressed them :)
#endif
				}
				
				//advance by full data block size
				l += rc->rec.dataSize;
			}

			//we don't want to add some subrecords as they are - e.g., they're compressed
			if (!skip)
				rc->data.push_back(srec);
		}
	}
	
	//return number of bytes really read
	return ((int)(MFTELL(esp)) - (int)start);
}

MyESP read_esp(MFILE esp)
{
	MyESP res;
	MyGroup* pgr;
	MyRecord* prc;
	int r;
	
	while ((r = read_next(esp,&pgr,&prc)) > 0) {
		assert(pgr || prc);
		if (pgr) res.grps.push_back(*pgr);
		else if (prc) res.recs.push_back(*prc);
	}
	
#if DEBUG_PARSE
	cout << "ESP final top: " << res.recs.size() << " records and ";
	cout << res.grps.size() << " groups" << endl;
#endif

	return res;
}

unsigned compress_zip_subrecords(vector<uint8_t>* to, vector<MySubRecord>* from)
{
	unsigned total = 0;
	vector<uint8_t> buf;
	for (auto &&i : (*from)) {
		size_t p = buf.size();
		buf.resize(p + sizeof(TES4SubRecord));
		memcpy(&(buf[p]),&(i.rec),sizeof(i.rec));
		buf.insert(buf.end(),i.data.begin(),i.data.end());
		
		total += buf.size() - p;//sizeof(TES4SubRecord) + i.data.size();
	}
#if DEBUG_DUMP
	cout << "Compress total " << total << endl;
#endif
	
	z_stream strm;
	memset(&strm,0,sizeof(strm));
	to->resize(total);
	assert(deflateInit(&strm,OBLIVION_ZLIB_LEVEL) == Z_OK);
	
	strm.avail_in = total;
	strm.avail_out = total;
	strm.next_in = &(buf[0]);
	strm.next_out = &((*to)[0]);
	
#if 0
	int r = deflate(&strm,Z_FINISH);
	assert(r == Z_STREAM_END);
#else
	do {
		strm.avail_out = total;
		strm.next_out = &((*to)[strm.total_out]);

		if (deflate(&strm,Z_FINISH) == Z_STREAM_ERROR) {
			deflateEnd(&strm);
			cerr << "Stream error." << endl;
			abort();
		}

		to->resize(strm.total_out + total);
	} while (strm.avail_out == 0);
	deflateEnd(&strm);
#endif
	
	to->resize(strm.total_out);
#if DEBUG_DUMP
	cout << "Compressed into " << to->size() << endl;
#endif
	
	deflateEnd(&strm);
	buf.clear();
	
#if DEBUG_DUMP
	cout << "Deflated sub-records from " << total << " to " << to->size() << endl;
#endif

	return total;
}

void update_record(MyRecord &todo)
{
	if (todo.rec.flags & REC_FLG_ZIP) {
		if (todo.data.size() == 1 && todo.data[0].dontCompress) {
#if DEBUG_DUMP
			cout << "Sub-record data of record ";
			print4(&todo);
			cout << " is already compressed" << endl;
#endif
			return;
		}
		
		for (auto &&i : todo.data) {
#if DEBUG_DUMP
			cout << "Record ";
			print4(&todo);
			cout << ": updating compressible sub-record ";
			print4(&i);
			cout << endl;
#endif
			i.rec.dataSize = i.data.size();
		}
		
		MySubRecord nw;
		nw.decompLen = compress_zip_subrecords(&(nw.data),&(todo.data));
		nw.dontCompress = true;
		
		todo.data.clear();
		todo.data.push_back(nw);
		
		todo.rec.dataSize = nw.data.size() + 4;
		
	} else {
		todo.rec.dataSize = 0;
		
		for (auto &&i : todo.data) {
#if DEBUG_DUMP
			cout << "Record ";
			print4(&todo);
			cout << ": updating non-compressible sub-record ";
			print4(&i);
			cout << endl;
#endif
			todo.rec.dataSize += sizeof(TES4SubRecord);
			i.rec.dataSize = i.data.size();
			todo.rec.dataSize += i.data.size();
		}
	}
}

int dump_record(MyRecord todo, MFILE esp)
{
	int tot = sizeof(TES4Record);
	
	update_record(todo);
	MFWRITE(&(todo.rec),sizeof(todo.rec),1,esp);
	
	for (auto &&i : todo.data) {
		if ((todo.rec.flags & REC_FLG_ZIP) == 0) {
			MFWRITE(&(i.rec),sizeof(i.rec),1,esp);
			tot += sizeof(i.rec);
			MFWRITE(&(i.data[0]),1,i.data.size(),esp);
			
		} else {
			assert(i.dontCompress);
			MFWRITE(&(i.decompLen),sizeof(i.decompLen),1,esp);
			tot += sizeof(i.decompLen);
			MFWRITE(&(i.data[0]),1,i.data.size(),esp);
		}
		
		tot += i.data.size();
	}
	
	return tot;
}

int update_group(MyGroup &todo)
{
	int tot = sizeof(TES4Group); //group data size includes header size
	for (auto &&i : todo.data) {
		if (i.isGroup)
			tot += update_group(*(i.data.grp));
		else {
			update_record(*(i.data.rec));
			tot += i.data.rec->rec.dataSize;
			tot += sizeof(TES4Record);
		}
	}
	todo.grp.groupSize = tot;
	return tot;
}

int dump_group(MyGroup &todo, MFILE esp)
{
	int tot = sizeof(TES4Group);
	MFWRITE(&(todo.grp),sizeof(todo.grp),1,esp);
	
	for (auto &&i : todo.data) {
		if (i.isGroup)
			tot += dump_group(*(i.data.grp),esp);
		else
			tot += dump_record(*(i.data.rec),esp);
	}
	
	return tot;
}

void write_esp(MyESP &data, MFILE esp)
{
	for (auto &&i : data.recs) {
#if DEBUG_DUMP
		cout << "Record dumped, r = " << dump_record(i,esp) << endl;
#else
		dump_record(i,esp);
#endif
	}
	
	for (auto i : data.grps) {
		update_group(i);
#if DEBUG_DUMP
		cout << "Group dumped, r = " << dump_group(i,esp) << endl;
#else
		dump_group(i,esp);
#endif
	}
}

void remove_group(MyGroup &cur)
{
	for (auto &&i : cur.data) {
		if (i.isGroup) {
			remove_group(*(i.data.grp));
			delete (i.data.grp);
			i.data.grp = NULL;
		} else {
			delete (i.data.rec);
			i.data.rec = NULL;
		}
	}
}

void clear_esp(MyESP &data)
{
	data.recs.clear();
	for (auto &&i : data.grps) remove_group(i);
	data.grps.clear();
}

}; //TES4
