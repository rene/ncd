#include "config.h"
#if HAVE_ZLIB
#include <stdio.h>
#include <zlib.h>
#include "ncd.h"

#define Z_FILE_CHUNK   16384
#define Z_COMP_LEVEL 9

/**
 * \brief Return compressed file size using zlib
 *
 * \return ssize_t Compressed size
 */
ssize_t zlib_getcompsize(ncd_file_t *fin)
{
	int ret, flush;
	unsigned char dIn[Z_FILE_CHUNK];
	unsigned char dOut[Z_FILE_CHUNK];
	unsigned int have;
	ssize_t cSize;
	z_stream strm;

	/* Allocate inflate state */
	cSize         = 0;
	strm.zalloc   = Z_NULL;
	strm.zfree    = Z_NULL;
	strm.opaque   = Z_NULL;
	ret = deflateInit(&strm, Z_COMP_LEVEL);
	if(ret != Z_OK) {
		ncd_err = -1;
		return 0;
	}

	do {
		strm.avail_in = ncd_fread(dIn, 1, Z_FILE_CHUNK, fin);
 		if (ncd_ferror(fin)) {
			(void)deflateEnd(&strm);
			return 0;
		}
		flush = ncd_feof(fin) ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = dIn;

		do {
			strm.avail_out = Z_FILE_CHUNK;
			strm.next_out  = dOut;
			ret = deflate(&strm, flush);
			/* assert state */
			if (ret == Z_STREAM_ERROR) {
				ncd_err = -1;
				return 0;
			}

			have   = Z_FILE_CHUNK - strm.avail_out;
			cSize += have;
		} while (strm.avail_out == 0);
		if(strm.avail_in != 0) {
			ncd_err = -1;
			return 0;
		}

	} while(flush != Z_FINISH);
	if(ret != Z_STREAM_END) {
		ncd_err = -1;
		return 0;
	}

	/* Clean up */
    (void)deflateEnd(&strm);

	/* Return */
	if (ret != Z_STREAM_END) {
		return -1;
	} else {
		return cSize;
	}
}
#endif

