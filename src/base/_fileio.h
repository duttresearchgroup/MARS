/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef __core_fileio_h
#define __core_fileio_h

typedef struct {
	void* native_file;
	bool big_endian;
} vfile_t;

//opens file for reading. It always reads the first 4 bytes to check endianess
vfile_t open_file_rd(const char* filepath);
//opens file for writing. It always writes the first 4 bytes to check endianess
vfile_t open_file_wr(const char* filepath);

void close_file(vfile_t *file);

//generic rd/wr
void file_rd(vfile_t *a_file, char *ptr, int count);
void file_wr(vfile_t *a_file, const char *ptr, int count);

//reads/writes words. Automatically converts between big/little endian
uint32_t file_rd_word(vfile_t *a_file);
void file_wr_word(vfile_t *a_file, uint32_t data);


#endif
