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

#ifndef ESP_LIST_H_
#define ESP_LIST_H_

#include "esp_parser.h"
#include "esp_utils.h"
#include "VFS.h"

#ifdef TES4LIB_USE_VFS
#include "vfshelper.h"
#endif

namespace TES4 {

typedef std::list<MyESPEntry> esplist; //that would contain all plugins in CORRECT order (as in game)

int load_esp_filelist(std::string const &listfn, std::string const &gamedir, esplist &files, VFS* vfs = NULL, VFSProgressCb prog_cb = 0);
int load_esp_filelist(std::vector<std::string> const &flist, std::string gamedir, esplist &files, VFS* vfs = NULL, VFSProgressCb prog_cb = 0);
void unload_esp_filelist(esplist &files);

}; //TES4

#endif /* ESP_LIST_H_ */
