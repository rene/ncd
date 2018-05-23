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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <libgen.h>
#include "ncd.h"

/** List of all files */
file_t *ncd_files;

/** Total files in the list */
unsigned long ncd_total_files = 0;

/** Number of threads */
int ncd_threads = 0;

/** NCD matrix */
mat_t **ncd_matrix;

/** Thread working indicator */
char *working = NULL;
/** Semaphore to access working list */
sem_t semwk;

/** Compressor to use on NCD */
static compressor_t *compalg;


/* Prototypes */
char *get_fullpath(char *basedir, char *filename);
double calc_NCD(double a, double b, double ab);
void *thread_calcsize(void *startline);
void *thread_calcncd(void *startline);

/**
 * \brief Calculate NCD (Normalized Compression Distance) matrix
 *
 * \param inputA First argument (file or directory)
 * \param inputB Second argument (file)
 * \param compressor Compression algorithm
 * \param mode NCD mode (file or directory)
 * \param csize Should be 1 to return only compressed size of the files (no NCD calculation)
 * \param n_threads Maximum number of threads
 * \return 0 on success, -1 otherwise
 */
int do_ncd(char *inputA, char *inputB, char *output, char *compressor, char mode, char csize, int n_threads)
{
	unsigned long i, j, total_files, total_de, startline, group_size;
	char *files[2], *fpath;
	DIR *idir;
	struct dirent *de;
	struct stat statbuf;
	FILE *fout = NULL;
	int res, s;
	pthread_t *threads;

	sem_init(&semwk, 0, 1);

	/* Check compressor and do memory allocation for threads */
	for (i = 0, compalg = NULL; 
			comp_list[i].name != NULL; i++) {
		if (strcmp(compressor, comp_list[i].name) == 0) {
			compalg = &comp_list[i];
			break;
		}
	}

	if (n_threads <= 0) {
		n_threads = 1;
	}
	threads     = (pthread_t*)malloc(sizeof(pthread_t) * n_threads);
	ncd_threads = n_threads;

	if (compalg == NULL || threads == NULL) {
		if (threads != NULL) free(threads);
		sem_destroy(&semwk);
		return (-1);
	}

	/* Create output file (if necessary) */
	if (csize == 0) {
		if (output == NULL) {
			fout = stdout;
		} else if (strcmp(output, "-") == 0) {
			fout = stdout;
		} else {
			fout = fopen(output, "w");
			if (fout == NULL) {
				perror(output);
				return (-1);
			}
		}
	}

	/* Load file information */
	if (mode == NCD_FILEMODE) {
		ncd_files = (file_t*)malloc(sizeof(file_t) * 2);
		working   = (char*)malloc(sizeof(char) * 2);
		if (ncd_files == NULL || working == NULL) {
			perror("doncd()");
			if (fout) fclose(fout);
			return (-1);
		} else {
			files[0]    = inputA;
			files[1]    = inputB;
			total_files = 0;
			for (i = 0; i < 2; i++) {
				if (files[i] == NULL) {
					continue;
				} else {
					total_files++;
				}
				/* Get file information */
				if ((s = stat(files[i], &statbuf)) < 0 ||
					(statbuf.st_mode & S_IFMT) != S_IFREG) {
					if (s < 0) perror(files[i]);
					if (i == 1) {
						free(ncd_files[0].path);
					}
					free(ncd_files);
					if (fout) fclose(fout);
					return (-1);
				} else {
					ncd_files[i].path      = strdup(files[i]);
					ncd_files[i].fd        = -1;
					ncd_files[i].f_errors  = 0;
					ncd_files[i].fsize     = statbuf.st_size;
					ncd_files[i].compsize  = -1;
					ncd_files[i].contents  = NULL;
					ncd_files[i].reference = 0;
					sem_init(&ncd_files[i].lock, 0, 1);
				}
			}
		}
	} else if (mode == NCD_DIRMODE) {
		/* Open input directory */
		idir = opendir(inputA);
		if (idir == NULL) {
			perror(inputA);
			if (fout) fclose(fout);
			return (-1);
		}

		/* Count number of entries
		 * (we won't check entry type now due to performance purposes) */
		total_de = 0;
		while ((de = readdir(idir))) {
			++total_de;
		}
		/* Back to the beginning of directory */
		rewinddir(idir);

		/* Let's allocate memory */
		ncd_files = (file_t*)malloc(sizeof(file_t) * total_de);
		working   = (char*)malloc(sizeof(char) * total_de);
		if (ncd_files == NULL || working == NULL) {
			perror("doncd()");
			closedir(idir);
			if (fout) fclose(fout);
			return (-1);
		} else {
			memset(working, 0, (sizeof(char) * total_de));
		}

		/* Read file information */
		total_files = 0;
		while ((de = readdir(idir))) {
			if (strcmp(de->d_name, ".") == 0 
					|| strcmp(de->d_name, "..") == 0) {
				continue;
			} else {
				/* Build full path */
				fpath = get_fullpath(inputA, de->d_name);

				/* Get file information */
				if (fpath == NULL || stat(fpath, &statbuf) < 0) {
					perror(fpath);
					closedir(idir);
					for (i = 0; i < total_files; i++) {
						free(ncd_files[i].path);
					}
					free(ncd_files);
					free(working);
					if (fout) fclose(fout);
					return (-1);
				} else {
					/* Consider only regular files */
					if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
						ncd_files[total_files].path      = fpath;
						ncd_files[total_files].fd        = -1;
						ncd_files[total_files].f_errors  = 0;
						ncd_files[total_files].fsize     = statbuf.st_size;
						ncd_files[total_files].compsize  = -1;
						ncd_files[total_files].contents  = NULL;
						ncd_files[total_files].reference = 0;
						sem_init(&ncd_files[total_files].lock, 0, 1);
						total_files++;
					}
				}
			}
		}

		/* Close directory */
		closedir(idir);
	} else {
		/* Unknown mode */
		if (fout) fclose(fout);
		return (-1);
	}

	/* Here we have two possibilities: calculate just compressed
	 * sizes or the entire NCD matrix. Let assign the matrix 
	 * (or vector) lines to each thread and start them all */
	ncd_total_files = total_files;
	if (csize == 1) {
		/* Calculate only compressed sizes */
		group_size = total_files / n_threads;
		for (i = 0, startline = 0; 
				i < n_threads; 
				i++, startline += group_size) {
			pthread_create(&threads[i], NULL, thread_calcsize, (void*)startline);
		}

		/* Wait for all threads to be done */
		for (i = 0; i < n_threads; i++) {
			pthread_join(threads[i], NULL);
		}

		/* Plot the results */
		for (i = 0; i < total_files; i++) {
			/* Doing this way just to keep compatibility with NCD from complearn */
			printf("%s: %ld\n", ncd_files[i].path, ncd_files[i].compsize);
		}

		res = 0;
	} else {
		/* Alloc memory for NCD matrix */
		ncd_matrix = new_mat(total_files, total_files);
		if (ncd_matrix == NULL) {
			res = -1;
		} else {
			ncd_err = 0;

			/* Calculate NCD matrix */
			for (i = 0; i < n_threads; i++) {
				pthread_create(&threads[i], NULL, thread_calcncd, (void*)i);
			}
			for (i = 0; i < n_threads; i++) {
				pthread_join(threads[i], NULL);
			}

			if (ncd_err == 0) {
				/* Write the results */
				for (i = 0; i < total_files; i++) {
					fprintf(fout, "%s ", basename(ncd_files[i].path));
					for (j = 0; j < total_files; j++) {
						fprintf(fout, "%.6f ", ncd_matrix[i][j]);
					}
					fprintf(fout, "\n");
				}

				/* Release memory */
				destroy_mat(&ncd_matrix, total_files);
			}
			res = ncd_err;
		}
	}

	/* Clean up and return */
	free(threads);
	sem_destroy(&semwk);
	for (i = 0; i < total_files; i++) {
		free(ncd_files[i].path);
		sem_destroy(&ncd_files[i].lock);
	}
	free(ncd_files);
	free(working);
	if (fout != NULL && fout != stdout) fclose(fout);
	return (res);
}


/**
 * \brief Build full path of a file
 *
 * \param basedir Base directory (which contains the file)
 * \param filename File name
 * \return Null terminated (allocated with malloc()) full path string
 */
char *get_fullpath(char *basedir, char *filename)
{
	size_t bdir_len, out_len;
	char *fpath;
	char prefix[] = {'\0', '\0', '\0'};

	out_len  = strlen(filename);
	bdir_len = strlen(basedir);

	if (bdir_len == 0) {
		fpath = strdup(filename);
		return (fpath);
	}

	if (basedir[0] == '/') {
		out_len += bdir_len;
	} else {
		if (bdir_len >= 2) {
			if (strncmp(basedir, "./", 2) == 0) {
				out_len += bdir_len;
			} else {
				out_len += 2;
				strncpy(prefix, "./", 2);
			}
		} else {
			out_len += 2;
			strncpy(prefix, "./", 2);
		}
	}

	if (basedir[bdir_len-1] == '/') {
		bdir_len--;
		out_len += bdir_len;
	}
	out_len++;

	fpath = (char*)malloc(sizeof(char) * (out_len+1));
	if (fpath == NULL) {
		return (NULL);
	}
	memset(fpath, '\0', out_len);
	if (prefix[0] != '\0') {
		strncpy(fpath, prefix, 2);
	}
	fpath = strncat(fpath, basedir, bdir_len);
	fpath = strncat(fpath, "/", 1);
	fpath = strcat(fpath, filename);

	return (fpath);
}


/**
 * \brief Calculate NCD distance
 *
 * \param a Compressed size of file A
 * \param b Compressed size of file B
 * \param ab Compressed size of file AB (concatenation of A and B)
 * \return double NCD(a,b)
 */
double calc_NCD(double a, double b, double ab)
{
	double min, max, ncd;

	if (a <= b) {
		min = a;
		max = b;
	} else {
		min = b;
		max = a;
	}
	
	ncd = (ab - min) / max;
	return (ncd);
}


/**
 * \brief Thread function to calculate compressed size
 *
 * \param startline Which line (at vector containing file information) the thread should start
 * \return NULL
 */
void *thread_calcsize(void *startline)
{
	unsigned long i, sl = (unsigned long)startline;
	ssize_t csize;
	char turn;
	ncd_file_t *fp;

	i    = sl;
	turn = 0;
	while (i < ncd_total_files && turn < 2) {
		/* Acquire lock and find some file to work on it */
		sem_wait(&semwk);
		if (working[i] != 0) {
			/* File has been taken by another thread,
			 * let's try to find another file to work */
			sem_post(&semwk);
			i++;
			if (i >= ncd_total_files) {
				i = 0;
				turn++;
			}
			continue;
		} else {
			working[i] = 1;
		}
		sem_post(&semwk);

		/* Work on the file */
		fp = ncd_open(&ncd_files[i], NULL);
		if (fp != NULL) {
			csize = compalg->get_compressed_size(fp);
			ncd_files[i].compsize = csize;
			ncd_close(fp);
		}

		/* Go to next position */
		i++;
		if (i >= ncd_total_files) {
			i = 0;
			turn++;
		}
	}

	return (NULL);
}


/**
 * \brief Calculate NCD matrix
 *
 * \param startline Which line (at vector containing file information) the thread should start
 * \return NULL
 * \note Each vector position represents one matrix line, so one thread 
 *       should proccess the entire matrix line
 */
void *thread_calcncd(void *startline)
{
	unsigned long i, j, sl = (unsigned long)startline;
	ssize_t csizes[] = {0, 0, 0};
	char turn;
	int k;
	ncd_file_t *fa, *fp;

	i    = sl;
	turn = 0;
	while (i < ncd_total_files && turn < 2) {
		/* Acquire lock and find some line (file) to work on it */
		sem_wait(&semwk);
		if (working[i] != 0) {
			/* Line has been taken by another thread,
			 * let's try to find another line to work */
			sem_post(&semwk);
			if (turn == 0) {
				i += ncd_threads; /* trying to be smart */
			} else {
				i++;
			}
			if (i >= ncd_total_files) {
				i = 0;
				turn++;
			}
			continue;
		} else {
			working[i] = 1;
		}
		sem_post(&semwk);

		/* Work on the entire matrix line */

		/* Get compressed size of all files: A, B and AB (concatenated)
		 * checking if we already know the compressed size of each of them */
		sem_wait(&ncd_files[i].lock);
		if (ncd_files[i].compsize > 0) {
			csizes[0] = ncd_files[i].compsize;
		} else {
			csizes[0] = -1;
		}
		sem_post(&ncd_files[i].lock);
		
		/* let file A opened to keep mapped at memory */
		fa = ncd_open(&ncd_files[i], NULL);
		if (fa == NULL) {
			ncd_err = -2;
			return (NULL);
		} else if (csizes[0] == -1) {
			csizes[0] = compalg->get_compressed_size(fa);
			ncd_files[i].compsize = csizes[0];
		}

		for (j = 0; j < ncd_total_files; j++) {
			if (i == j) {
				ncd_matrix[i][j] = 0;
				continue;
			}
			for (k = 1; k < 3; k++) {
				if (k < 2) {
					/* Single file */
					sem_wait(&ncd_files[j].lock);
					if (ncd_files[j].compsize > 0) {
						csizes[1] = ncd_files[j].compsize;
						sem_post(&ncd_files[j].lock);
					} else {
						sem_post(&ncd_files[j].lock);
						fp = ncd_open(&ncd_files[j], NULL);
						if (fp != NULL) {
							csizes[1] = compalg->get_compressed_size(fp);
							ncd_files[j].compsize = csizes[1];
							ncd_close(fp);
						} else {
							ncd_err = -2;
							break;
						}
					}
				} else {
					/* Concatenated file */
					fp = ncd_open(&ncd_files[i], &ncd_files[j]);
					if (fp != NULL) {
						csizes[2] = compalg->get_compressed_size(fp);
						ncd_close(fp);
					} else {
						ncd_err = -2;
						break;
					}
				}
			}

			/* Compute NCD */
			ncd_matrix[i][j] = calc_NCD((double)csizes[0], (double)csizes[1], (double)csizes[2]);
		}
		/* Close file A */
		ncd_close(fa);
		
		/* Go to next line */
		if (turn == 0) {
			i += ncd_threads;
		} else {
			i++;
		}
		if (i >= ncd_total_files) {
			i = 0;
			turn++;
		}
	}

	return (NULL);
}

