/* ncd - Calculate NCD as fast as possible
 * Copyright (C) 2018  Rene de Souza Pinto
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "ncd.h"

extern file_t *ncd_files;


/**
 * \brief Open a single or concatenated file
 *
 * \param fileA Reference to the first file
 * \param fileB Reference to the second file (NULL for single files)
 * \return Pointer to allocated structure which represents the file
 */
ncd_file_t *ncd_open(file_t *fileA, file_t *fileB)
{
	int i;
	ncd_file_t *fp;
	unsigned char *fcontents = NULL;

	/* Initialize structure */
	fp = (ncd_file_t*)malloc(sizeof(ncd_file_t));
	if (fp == NULL) {
		return (fp);
	} else {
		fp->fileref[0] = fileA;
		fp->fileref[1] = fileB;
		fp->fpos       = 0;
	}

	/* Map files */
	for (i = 0; i < 2; i++) {
		if (fp->fileref[i] == NULL) {
			continue;
		}
		/* Acquire lock */
		sem_wait(&fp->fileref[i]->lock);

		/* Check memory mapping */
		if (fp->fileref[i]->reference <= 0) {
			/* Open and map file to memory */
			if ((fp->fileref[i]->fd = open(fp->fileref[i]->path, O_RDONLY)) < 0) {
				sem_post(&fp->fileref[i]->lock);
				free(fp);
				return (NULL);
			}

			fcontents = (unsigned char*)mmap(fcontents, fp->fileref[i]->fsize,
												PROT_READ, MAP_SHARED,
												fp->fileref[i]->fd, 0);
			if (fcontents == (unsigned char*)MAP_FAILED) {
				close(fp->fileref[i]->fd);
				fp->fileref[i] = NULL;
				sem_post(&fp->fileref[i]->lock);
				ncd_close(fp);
				break;
			} else {
				fp->fileref[i]->contents = fcontents;
				fcontents = NULL;
			}
		}

		/* Increase reference */
		fp->fileref[i]->reference++;

		/* Release lock */
		sem_post(&fp->fileref[i]->lock);
	}

	return (fp);
}


/**
 * \brief Closes NCD file
 *
 * \param m Pointer to the opened file
 */
void ncd_close(ncd_file_t *fp)
{
	int i;

	if (fp == NULL) {
		return;
	}

	for (i = 0; i < 2; i++) {
		if (fp->fileref[i] == NULL) {
			continue;
		}
		/* Acquire lock */
		sem_wait(&fp->fileref[i]->lock);

		/* Decrease reference */
		fp->fileref[i]->reference--;
		if (fp->fileref[i]->reference == 0) {
			/* Unmap and close file */
			munmap(fp->fileref[i]->contents, fp->fileref[i]->fsize);
			close(fp->fileref[i]->fd);
			fp->fileref[i]->fd       = -1;
			fp->fileref[i]->contents = NULL;
		}

		/* Release lock */
		sem_post(&fp->fileref[i]->lock);
	}

	/* Release memory */
	free(fp);
	fp = NULL;
}


/**
 * \brief Read data from NCD file
 *
 * \param ptr Pointer to the buffer
 * \param size Size of each item
 * \param nmemb Number of items to read
 * \return ssize_t Number of **items** read
 */
ssize_t ncd_fread(void *ptr, size_t size, size_t nmemb, ncd_file_t *stream)
{
	int i;
	size_t total_bytes, total_fsize, chunk, cnt;
	unsigned char *dest = (unsigned char*)ptr;

	if (ptr == NULL || stream == NULL || size == 0) {
		return (0);
	}

	/* Total file size */
	total_fsize = 0;
	for (i = 0; i < 2; i++) {
		if (stream->fileref[i] != NULL) {
			total_fsize += stream->fileref[i]->fsize;
		}
	}
	if (stream->fpos >= total_fsize) {
		/* End of file reached */
		return (0);
	}

	/* Total bytes to read */
	total_bytes = size * nmemb;

	/* Total bytes we will read (effectively) */
	if ((stream->fpos + total_bytes) > total_fsize) {
		cnt = total_fsize - stream->fpos;
	} else {
		cnt = total_bytes;
	}
	if (cnt == 0) {
		return (0);
	}

	/* Check for file transition during reading */
	if (stream->fileref[0] != NULL && stream->fileref[1] != NULL) {
		if (stream->fpos < stream->fileref[0]->fsize &&
				(stream->fpos + cnt) > stream->fileref[0]->fsize) {
			/* Read first chunk */
			chunk = stream->fileref[0]->fsize - stream->fpos;
			dest  = memcpy(dest, &stream->fileref[0]->contents[stream->fpos], chunk);

			/* Read second chunk */
			memcpy(&dest[chunk], stream->fileref[1]->contents, (cnt - chunk));
			stream->fpos += cnt;
		} else if (stream->fpos < stream->fileref[0]->fsize) {
			/* Need to read just first file area */
			dest = memcpy(dest, &stream->fileref[0]->contents[stream->fpos], cnt);
			stream->fpos += cnt;
		} else {
			/* Need to read just second file area */
			dest = memcpy(dest, &stream->fileref[1]->contents[stream->fpos - stream->fileref[0]->fsize], cnt);
			stream->fpos += cnt;
		}
	} else {
		/* Read contents */
		dest = memcpy(dest, &stream->fileref[0]->contents[stream->fpos], cnt);
		stream->fpos += cnt;
	}

	ptr = dest;
	return (cnt/size);
}


/**
 * \brief Reads the next character from stream
 *
 * \param stream File stream
 * \return int Unsigned char cast to an int, or -1 on end of file or error
 */
int ncd_getc(ncd_file_t *stream)
{
	unsigned char buf[2];
	int c;

	if (ncd_fread(buf, 1, 1, stream) > 0) {
		c = (int)buf[0];
	} else {
		c = -1;
	}
	return c;
}


/**
 * \brief Writes a single character to stream
 *
 * \param c Character
 * \return int Always -1
 * \note This function is not implemented and is declared just for compatibility purposes
 */
int ncd_putc(int c, ncd_file_t *stream)
{
	return -1;
}


/**
 * \brief Check and reset stream status
 * \param stream NCD file stream
 * \return int
 */
int ncd_ferror(ncd_file_t *stream)
{
	int i, ferr = 0;

	if (stream == NULL) {
		return (0);
	} else {
		for (i = 0; i < 2; i++) {
			if (stream->fileref[i] != NULL) {
				ferr |= stream->fileref[i]->f_errors;
				stream->fileref[i]->f_errors = 0;
			}
		}
		return (ferr);
	}
}


/**
 * \brief Check end-of-file
 *
 * \param stream NCD file stream
 * \return int
 */
int ncd_feof(ncd_file_t *stream)
{
	int i;
	size_t tsize;
	if (stream == NULL) {
		return (0);
	} else {
		tsize = 0;
		for (i = 0; i < 2; i++) {
			if (stream->fileref[i] != NULL) {
				tsize += stream->fileref[i]->fsize;
			}
		}
		if (stream->fpos >= tsize) {
			return (1);
		} else {
			return (0);
		}
	}
}

