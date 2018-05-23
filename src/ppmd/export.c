#include "export.h"
#include "Alloc.h"
#include "Ppmd7.h"
#include <ncd.h>

typedef struct {
  IByteOut i;
  ssize_t size;
} BufferOutStream;

static size_t written(BufferOutStream *os) {
  return os->size;
}

static void write(void *p, Byte b) {
  BufferOutStream *os = (BufferOutStream *)p;
  os->size++;
}

static void setWrite(BufferOutStream *os) {
  os->i.Write = write;
}

static void initBufferOutStream(BufferOutStream *os) {
  os->size = 0;
  setWrite(os);
}

void initPpmdHandle(CPpmd7 *handle, size_t dicmem, unsigned order) {
  Ppmd7_Construct(handle);
  Ppmd7_Alloc(handle, dicmem, &g_Alloc);
  Ppmd7_Init(handle, order);
}

void closePpmdHandle(CPpmd7 *handle) {
  Ppmd7_Free(handle, &g_Alloc);
}


ssize_t ppmd_getcompsize(ncd_file_t *fin)
{
	int order = 9;
	size_t outlen = 0;
	int dicmem = 10 * 1024 * 1024;
	CPpmd7 handle;
	CPpmd7z_RangeEnc desc;
	BufferOutStream os;
	
	initPpmdHandle(&handle, dicmem, order);
	initBufferOutStream(&os);
	desc.Stream = &os.i;
	Ppmd7z_RangeEnc_Init(&desc);

	while (!ncd_feof(fin)) {
		Ppmd7_EncodeSymbol(&handle, &desc, ncd_getc(fin));
	}
	Ppmd7z_RangeEnc_FlushData(&desc);

	outlen = written(&os);
	closePpmdHandle(&handle);

	return outlen;
}

