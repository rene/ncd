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
#include "ncd.h"

/**
 * \brief Creates a new ixj matrix
 *
 * \param i Number of rows
 * \param j Number of columns
 * \return Pointer to the new allocated matrix, otherwise NULL (in case of error)
 */
mat_t **new_mat(unsigned long i, unsigned long j)
{
	int p;
	mat_t **m;

	/* Alloc memory for the new matrix */
	m = (mat_t**)malloc(sizeof(mat_t*) * i);
	if(m != NULL) {
		for(p=0; p < i; p++) {
			m[p] = (mat_t*)malloc(sizeof(mat_t) * j);
			if(m[p] == NULL) {
				free(m);
				return (NULL);
			}
		}
	}
	return (m);
}

/**
 * \brief Destroy matrix
 *
 * \param m Pointer to the matrix
 * \param i Number of rows
 */
void destroy_mat(mat_t ***m, int i)
{
	int p;
	if(*m != NULL) {
		for(p=0; p < i; p++) {
			free((*m)[p]);
		}
		free(*m);
	}
}

