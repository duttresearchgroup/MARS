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
