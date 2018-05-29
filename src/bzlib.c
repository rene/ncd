#include "config.h"
#if HAVE_BZLIB
#include <stdio.h>
#include <bzlib.h>
#include "ncd.h"

#define BZ_FILE_CHUNK 102400

/**
 * \brief Return compressed file size using bzlib
 *
 * \return ssize_t Compressed size
 */
ssize_t bzlib_getcompsize(ncd_file_t *fin)
{
	int ret, flush;
	char dIn[BZ_FILE_CHUNK];
	char dOut[BZ_FILE_CHUNK];
	unsigned int have;
	ssize_t cSize;
	bz_stream strm;

	/* Allocate inflate state */
	cSize = 0;
	ret   = BZ2_bzCompressInit(&strm, 1, 0, 0);
	if(ret != BZ_OK) {
		ncd_err = -1;
		return 0;
	}

	do {
		strm.avail_in = ncd_fread(dIn, 1, BZ_FILE_CHUNK, fin);
 		if (ncd_ferror(fin)) {
			(void)BZ2_bzCompressEnd(&strm);
			return 0;
		}
		flush = ncd_feof(fin) ? BZ_FINISH : BZ_FLUSH;
		strm.next_in = dIn;

		do {
			strm.avail_out = BZ_FILE_CHUNK;
			strm.next_out  = dOut;
			ret = BZ2_bzCompress(&strm, flush);
			/* assert state */
			if(ret == BZ_SEQUENCE_ERROR) {
				ncd_err = -1;
				return 0;
			}

			have   = BZ_FILE_CHUNK - strm.avail_out;
			cSize += have;
		} while (strm.avail_out == 0);
		if(strm.avail_in != 0) {
			ncd_err = - 1;
			return 0;
		}

	} while(flush != BZ_FINISH);
	if(ret != BZ_STREAM_END) {
		ncd_err = -1;
		return 0;
	}

	/* Clean up */
    (void)BZ2_bzCompressEnd(&strm);

	/* Return */
	if (ret != BZ_STREAM_END) {
		return -1;
	} else {
		return cSize;
	}
}
#endif

