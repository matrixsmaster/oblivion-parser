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

#include "esp_utils.h"

static const char hex_tab[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','X' };

using namespace std;
namespace TES4 {

static unsigned harvest_group(MyGroup &grp, short num, function<void(uint32_t,MyRecord*,MyGroup*)> action)
{
	unsigned tot = 0;
	for (auto &&i : grp.data) {
		if (i.isGroup)
			tot += harvest_group(*(i.data.grp),num,action);
			
		else {
			tot++;
			uint32_t fid = i.data.rec->rec.formID;
			if (fid >= 0x01000000) {
				fid &= 0x00FFFFFF;
				fid |= (uint32_t)num << (3*8);
			}
			//printf("formid 0x%08X\n",fid);
			
			action(fid,i.data.rec,&grp);
		}
	}
	return tot;
}

unsigned harvest(MyESPEntry &rec, FORMIDS &fmap)
{
	unsigned tot = 0;
	for (auto &&i : rec.data.grps) {
		//tot += harvest_group(i,fmap,rec.name,num);
		tot += harvest_group(i,rec.plugid, [&] (uint32_t fid, MyRecord* p, MyGroup*) {
			fmap.insert(pair<uint32_t,pair<string,MyRecord*>> (fid,pair<string,MyRecord*>(rec.name,p))); } );
	}
	return tot;
}

MyRecord* retrieve(MyESPEntry &rec, uint32_t fid)
{
	MyRecord* ptr = NULL;
	for (auto &&i : rec.data.grps) {
		harvest_group(i,rec.plugid, [&] (uint32_t id, MyRecord* p, MyGroup*) {
			if (id == fid) ptr = p; } );
		if (ptr) break;
	}
	return ptr;
}

MyGroup* get_parent_group(MyESPEntry &rec, MyRecord* child)
{
	MyGroup* ptr = NULL;
	for (auto &&i : rec.data.grps) {
		harvest_group(i,rec.plugid, [&] (uint32_t, MyRecord* cur, MyGroup* grp) {
			if (cur == child) ptr = grp; } );
		if (ptr) break;
	}
	return ptr;
}

bool equ_ignorecase(string a, string b)
{
	for (auto &i : a) i = toupper(i);
	for (auto &i : b) i = toupper(i);
	return (a == b);
}

bool have_subfield(MyRecord* ptr, const char* type)
{
	bool r = false;
	for (auto &&i : ptr->data) 
		if (!strncmp(i.rec.subType,type,4)) {
			r = true;
			break;
		}
	return r;
}

string get_type(MyRecord* ptr)
{
	string ret;
	ret.resize(4);
	memcpy(&(ret[0]),ptr->rec.type,4);
	return ret;
}

string get_subfield(MyRecord* ptr, const char* type)
{
	string ret;
	for (auto &&i : ptr->data) 
		if (!strncmp(i.rec.subType,type,4)) {
			//assert(i.rec.dataSize < 512);
			if (i.rec.dataSize < 1) return ret;
			ret.resize(i.rec.dataSize-1);
			memcpy(&(ret[0]),&(i.data[0]),i.rec.dataSize-1);
			break;
		}
	return ret;
}

vector<uint8_t> get_subfield_u8(MyRecord* ptr, const char* type)
{
	vector<uint8_t> ret;
	for (auto &&i : ptr->data) 
		if (!strncmp(i.rec.subType,type,4)) {
			ret = i.data;
			ret.resize(i.rec.dataSize);
			break;
		}
	return ret;
}

uint32_t get_subfield_ref(MyRecord* ptr, const char* type, int cnt)
{
	uint32_t r = 0xFFFFFFFF;
	for (auto &&i : ptr->data)
		if (!strncmp(i.rec.subType,type,4) && !cnt--) {
			r = *(uint32_t*)&(i.data[0]);
			break;
		}
	return r;
}

void set_subfield(MyRecord* ptr, const char* type, string content)
{
	for (auto &&i : ptr->data) {
		if (strncmp(i.rec.subType,type,4)) continue;
		//assert(content.size() < 512);
		i.rec.dataSize = content.size() + 1;
		i.data.clear();
		i.data.resize(i.rec.dataSize,0);
		memcpy(&(i.data[0]),content.c_str(),i.rec.dataSize);
		break;
	}
}

void set_subfield_u8(MyRecord* ptr, const char* type, vector<uint8_t> content)
{
	for (auto &&i : ptr->data) 
		if (!strncmp(i.rec.subType,type,4)) {
			i.data = content;
			i.rec.dataSize = content.size();
			break;
		}
}

void set_subfield_u8(MyRecord* ptr, const char* type, string content)
{
	vector<uint8_t> buf;
	bool lsb = true;
	uint8_t n,v = 0;
	
	for (auto i = content.rbegin(); i != content.rend(); ++i) {
		for (n = 0; n < 16; n++)
			if (toupper(*i) == hex_tab[n]) break;
		if (n > 15) break;
		
		if (!lsb) {
			v |= n << 4;
			buf.push_back(v);
		} else
			v = n;
			
		lsb ^= true;
	}
	
	set_subfield_u8(ptr,type,buf);
}

}; //TES4
