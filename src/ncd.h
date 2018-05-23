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
#ifndef NCD_H
#define NCD_H

#include <stdlib.h>
#include <semaphore.h>
#include "config.h"

#define DEFAULT_OUTPUT     "distmatrix.txt"
#define DEFAULT_THREADS    2
#define DEFAULT_COMPRESSOR "ppmd"

#define ARG_HELP    0x01
#define ARG_DIRMODE 0x02
#define ARG_LIST    0x04
#define ARG_SIZE    0x08
#define ARG_VERBOSE 0x10
#define ARG_VERSION 0x20

#define ARG_PASSED(a,mask) ((a & mask) == mask)

#define NCD_FILEMODE 0x01
#define NCD_DIRMODE  0x02

/** Single file structure */
typedef struct _file_t {
	/** File path */
	char *path;
	/** File descriptor */
	int fd;
	/** File error */
	int f_errors;
	/** File size */
	ssize_t fsize;
	/** Compressed size */
	ssize_t compsize;
	/** File contents */
	unsigned char *contents;
	/** Reference counter */
	ssize_t reference;
	/** Semaphore to atomic access */
	sem_t lock;
} file_t;

/** NCD File structure (could be single or concatenated file(s))*/
typedef struct _ncd_file_t {
	/** Index at vector of files */
	file_t *fileref[2];
	/** File position (for read function) */
	unsigned long fpos;
} ncd_file_t;

/** Compressor type */
typedef struct _compressor {
	/** Compressor's name */
	char *name;
	/** Function to give compressed file size */
	ssize_t (*get_compressed_size)(ncd_file_t*);
} compressor_t;

/** Type of each matrix element */
typedef double mat_t;

/** List of available compressor */
extern compressor_t comp_list[];

/** Global error indicator */
extern int ncd_err;

/* Prototypes */
mat_t **new_mat(unsigned long i, unsigned long j);
void destroy_mat(mat_t ***m, int i);
int	do_ncd(char *inputA, char *inputB, char *output, char *compressor, char mode, char csize, int n_threads);
void list_compressors(void);
ncd_file_t *ncd_open(file_t *fileA, file_t *fileB);
void ncd_close(ncd_file_t *fp);
ssize_t ncd_fread(void *ptr, size_t size, size_t nmemb, ncd_file_t *stream);
int ncd_getc(ncd_file_t *stream);
int ncd_putc(int c, ncd_file_t *stream);
int ncd_ferror(ncd_file_t *stream);
int ncd_feof(ncd_file_t *stream);

#endif /* NCD_H */
