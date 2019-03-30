/*
 * libsgui_io data preparation and conversion library
 * (C) Dmitry 'MatrixS_Master' Solovyev, 2015-2019
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

#include "printer.h"

#ifdef TES4LIB_USE_VFS
#define MFILE VBFILE*
#define MFOPEN(PTH,MD) FBOPEN(PTH,MD,((VFS*)vfs))
#define MFCLOSE(X) FCLOSE(X)
#define MFREAD(TO,SZ,CNT,VF) FREAD(TO,SZ,CNT,VF)
#define MFWRITE(FR,SZ,CNT,VF) FWRITE(FR,SZ,CNT,VF)
#define MFSEEK(VF,OFF,WH) (VF->f_seek(OFF,WH))
#define MFTELL(VF) (VF->f_tell())
#define MTFILE VTFILE*
#define MFTOPEN(PTH,MD) FTOPEN(PTH,MD,((VFS*)vfs))
#define MFGETS(TO,SZ,VF) FGETS(TO,SZ,VF)
#else
#define MFILE FILE*
#define MFOPEN(PTH,MD) fopen(PTH,MD)
#define MFCLOSE(X) fclose(X)
#define MFREAD(TO,SZ,CNT,VF) fread(TO,SZ,CNT,VF)
#define MFWRITE(FR,SZ,CNT,VF) fwrite(FR,SZ,CNT,VF)
#define MFSEEK(VF,OFF,WH) fseek(VF,OFF,WH)
#define MFTELL(VF) ftell(VF)
#define MTFILE FILE*
#define MFTOPEN(PTH,MD) fopen(PTH,MD)
#define MFGETS(TO,SZ,VF) fgets(TO,SZ,VF)
#endif
