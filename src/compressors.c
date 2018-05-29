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
#include "ncd.h"

ssize_t ppmd_getcompsize(ncd_file_t *fin);

#if HAVE_ZLIB
ssize_t zlib_getcompsize(ncd_file_t *file);
#endif
#if HAVE_BZLIB
ssize_t bzlib_getcompsize(ncd_file_t *file);
#endif

/** Available compressors definitions */
compressor_t comp_list[] = {
#if HAVE_ZLIB
	{"zlib", zlib_getcompsize},
#endif
#if HAVE_BZLIB
	{"bzlib", bzlib_getcompsize},
#endif
	{"ppmd", ppmd_getcompsize},
	{NULL, NULL}
};


/**
 * \brief Print the list of available compressors
 */
void list_compressors(void)
{
	int i;
	for (i = 0; comp_list[i].name != NULL; i++) {
		printf("%s\n", comp_list[i].name);
	}
}

