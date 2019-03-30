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

#ifndef ESP_UTILS_H_
#define ESP_UTILS_H_

#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <unordered_map>
#include <set>
#include <list>
#include <algorithm>
#include <functional>
#include "esp_parser.h"

namespace TES4 {

struct MyESPEntry {
	std::string name;
	MyESP data;
	short plugid;
};

typedef std::multimap<uint32_t,std::pair<std::string,MyRecord*>> FORMIDS;

unsigned harvest(MyESPEntry &rec, FORMIDS &fmap);
MyRecord* retrieve(MyESPEntry &rec, uint32_t fid);
MyGroup* get_parent_group(MyESPEntry &rec, MyRecord* child);

bool equ_ignorecase(std::string a, std::string b);
bool have_subfield(MyRecord* ptr, const char* type);

std::string get_type(MyRecord* ptr);
std::string get_subfield(MyRecord* ptr, const char* type);
std::vector<uint8_t> get_subfield_u8(MyRecord* ptr, const char* type);
uint32_t get_subfield_ref(MyRecord* ptr, const char* type, int cnt = 0);

void set_subfield(MyRecord* ptr, const char* type, std::string content);
void set_subfield_u8(MyRecord* ptr, const char* type, std::vector<uint8_t> content);
void set_subfield_u8(MyRecord* ptr, const char* type, std::string content);

}; //TES4

#endif /*ESP_UTILS_H_*/
