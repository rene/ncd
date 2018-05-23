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
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "ncd.h"

/** Global error indicator */
int ncd_err = 0;

extern int optind;

/* Prototypes */
void show_help(char *prgname);
void show_version(char *prgname);

/* Main */
int main(int argc, char **argv)
{
	static struct option longOpts[] = {
		{"compressor",     required_argument, NULL, 'c'},
		{"directory-mode", required_argument, NULL, 'd'},
		{"help",           no_argument,       NULL, 'h'},
		{"list",           no_argument,       NULL, 'L'},
		{"output",         required_argument, NULL, 'o'},
		{"size",           no_argument,		  NULL, 's'},
		{"threads",        required_argument, NULL, 't'},
		{"verbose",        no_argument,		  NULL, 'v'},
		{"version",        no_argument,		  NULL, 'V'},
		{NULL, no_argument, NULL, 0}
	};
	const char *optstring = "c:d:hLo:st:vV";
	int opt, optli, n_args;
	char optc;
	char mode, *output, *compressor, *inpdir;
	char *input[2], csize;
	int n_threads;

	/* Default values */
	input[0]   = NULL;
	input[1]   = NULL;
	inpdir     = NULL;
	mode       = NCD_FILEMODE;
	output     = DEFAULT_OUTPUT;
	n_threads  = DEFAULT_THREADS;
	compressor = DEFAULT_COMPRESSOR;
	csize      = 0;

	/* Treat command line */
	optc  = 0x00; 
	opt   = getopt_long(argc, argv, optstring, longOpts, &optli);
	while(opt != -1) {
		switch(opt) {
			case 'c':
				compressor = strdup(optarg);
				break;

			case 'd':
				optc   |= ARG_DIRMODE;
				mode    = NCD_DIRMODE;
				inpdir  = strdup(optarg);
				break;
	
			case 'L':
				optc |= ARG_LIST;
				break;

			case 'o':
				output = strdup(optarg);
				break;

			case 's':
				optc |= ARG_SIZE;
				csize = 1;
				break;

			case 't':
				n_threads = atoi(optarg);
				break;

			case 'v':
				optc |= ARG_VERBOSE;
				break;

			case 'V':
				optc |= ARG_VERSION;
				break;

			case 'h':
			default:
				optc |= ARG_HELP;
		}
		opt = getopt_long(argc, argv, optstring, longOpts, &optli);
	}

	/* Check arguments */
	if (ARG_PASSED(optc, ARG_VERSION))  {
		show_version(argv[0]);
		return (EXIT_SUCCESS);
	}
	if (ARG_PASSED(optc, ARG_HELP)) {
		show_help(argv[0]);
		return (EXIT_SUCCESS);
	}

	if (ARG_PASSED(optc, ARG_LIST)) {
		list_compressors();
		return (EXIT_SUCCESS);
	}

	/* Validate parameters */
	if (n_threads <= 0) {
		fprintf(stderr, "Invalid number of threads: %d (should be greater then 0)\n", n_threads);
		return (EXIT_FAILURE);
	}

	/* Read input arguments */
	if ((optind+1) > argc && mode != NCD_DIRMODE) {
		show_help(argv[0]);
		return (EXIT_FAILURE);
	}

	n_args = (argc - optind);
	if (mode == NCD_FILEMODE && n_args < 2 && csize == 0) {
		/* One or less inputs is not valid in NCD file mode */
		fprintf(stderr, "Two files should be passed in file mode.\n");
		return (EXIT_FAILURE);
	} else if (mode == NCD_DIRMODE) {
		/* In directory mode, no other inputs should be allowed */
		if (n_args > 0) {
			fprintf(stderr, "Only one directory should be passed in directory mode.\n");
			return (EXIT_FAILURE);
		}
		input[0] = inpdir;
	} else {
		if (csize == 1) {
			input[0] = argv[argc-1];
		} else { 
			input[0] = argv[argc-2];
			input[1] = argv[argc-1];
		}
	}

	/* Calculate NCD */
	if (do_ncd(input[0], input[1], output, 
				compressor, mode, csize, n_threads) < 0) {
		fprintf(stderr, "Error. NCD cannot be calculated.\n");
	}

	return (EXIT_SUCCESS);
}

/**
 * \brief Show program's help
 */
void show_help(char *prgname)
{
	printf("Usage: %s [options] ... <arg>\n", prgname);
	printf("Use one argument form to do simple single-object or object-vector\n");
	printf("compression or\n\n");
	printf("    ncd [options] ...  <arg1> <arg2>\n\n");
	printf("for compression-matrix or NCD matrix calculation\n\n");
	printf("OPTIONS:\n");
	printf("    -c, --compressor=COMPNAME   set compressor to use\n");
	printf("    -d, --directory-mode        directory of files\n");
	printf("    -h, --help                  print this help message\n");
	printf("    -L, --list                  list compressors\n");
	printf("    -o, --output=FILEOUT        use FILEOUT instead of distmatrix\n");
	printf("    -s, --size                  just compressed sizes in bits no NCD\n");
	printf("    -t, --threads               maximum number of threads\n");
	printf("    -v, --verbose               print extra detailed information\n");
	printf("    -V, --version               print program's version and exit\n");
}

/**
 * \brief Show program's version
 */
void show_version(char *prgname)
{
	printf("%s\n", prgname);
	printf("Version: %s\n", PACKAGE_VERSION);
}

