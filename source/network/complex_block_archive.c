/*
	Copyright (c) 2022 ByteBit/xtreme8000

	This file is part of CavEX.

	CavEX is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	CavEX is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with CavEX.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>

#include "complex_block_archive.h"

static int32_t swap_int32(int32_t n) {
	n = ((n << 8) & 0xFF00FF00) | ((n >> 8) & 0xFF00FF);
	return (n << 16) | ((n >> 16) & 0xFFFF);
}

static uint16_t swap_uint16(uint16_t n) {
	return (n << 8) | (n >> 8);
}

static int is_little_endian() {
	volatile uint32_t i=0x01234567;
	return (*((uint8_t*)(&i))) == 0x67;
}

static void swap_endianness(struct complex_block_pos* pos, struct item_data* items) {
	if(is_little_endian()) {
		for(int i=0; i<MAX_CHESTS; i++) {
			pos[i].x = swap_int32(pos[i].x);	
			pos[i].y = swap_int32(pos[i].y);	
			pos[i].z = swap_int32(pos[i].z);	
			if(items) {
				for(int j=0; j<MAX_CHEST_SLOTS; j++) {
					(items + i*MAX_CHEST_SLOTS + j)->id = swap_uint16((items + i*MAX_CHEST_SLOTS + j)->id);
				}
			}
		}
	}
}

void chest_archive_write(struct complex_block_pos* pos, struct item_data* items, string_t path) {
	assert(pos && items && path);

	swap_endianness(pos, items);

	string_t filename;
	string_init_printf(filename, "%s/chests.dat",
					   string_get_cstr(path));
	FILE* f = fopen(string_get_cstr(filename), "wb");
	string_clear(filename);
	if (!f) return;

	fputc(0, f); //format version byte
	fwrite(pos, 1, MAX_CHESTS*sizeof(struct complex_block_pos), f);
	fwrite(items, 1, MAX_CHEST_SLOTS*MAX_CHESTS*sizeof(struct item_data), f);
	fclose(f);
}

void chest_archive_read(struct complex_block_pos* pos, struct item_data* items, string_t path) {
	assert(pos && items && path);
	memset(pos, -1, MAX_CHESTS*sizeof(struct complex_block_pos));
	memset(items, 0, MAX_CHEST_SLOTS*MAX_CHESTS*sizeof(struct item_data));

	string_t filename;
	string_init_printf(filename, "%s/chests.dat",
					   string_get_cstr(path));
	FILE* f = fopen(string_get_cstr(filename), "rb");
	string_clear(filename);
	if(!f) return;

	uint8_t format_version = fgetc(f);
	if(format_version != 0) puts("Warning: chests.dat uses a newer format");
	fread(pos, 1, MAX_CHESTS*sizeof(struct complex_block_pos), f);
	fread(items, 1, MAX_CHEST_SLOTS*MAX_CHESTS*sizeof(struct item_data), f);
	fclose(f);

	swap_endianness(pos, items);
}

void sign_archive_write(struct complex_block_pos* pos, char* texts, string_t path) {
	assert(pos && texts && path);

	swap_endianness(pos, NULL);

	string_t filename;
	string_init_printf(filename, "%s/signs.dat",
					   string_get_cstr(path));
	FILE* f = fopen(string_get_cstr(filename), "wb");
	string_clear(filename);
	if (!f) return;

	fputc(0, f); //format version byte
	fwrite(pos, 1, MAX_SIGNS*sizeof(struct complex_block_pos), f);
	fwrite(texts, 1, MAX_SIGNS*SIGN_SIZE*sizeof(char), f);
	fclose(f);
}

void sign_archive_read(struct complex_block_pos* pos, char* texts, string_t path) {
	assert(pos && texts && path);
	memset(pos, -1, MAX_SIGNS*sizeof(struct complex_block_pos));
	memset(texts, ' ', MAX_SIGNS*SIGN_SIZE*sizeof(char));

	string_t filename;
	string_init_printf(filename, "%s/signs.dat",
					   string_get_cstr(path));
	FILE* f = fopen(string_get_cstr(filename), "rb");
	string_clear(filename);
	if(!f) return;

	uint8_t format_version = fgetc(f);
	if(format_version != 0) puts("Warning: signs.dat uses a newer format");
	fread(pos, 1, MAX_CHESTS*sizeof(struct complex_block_pos), f);
	fread(texts, 1, MAX_CHEST_SLOTS*MAX_CHESTS*sizeof(struct item_data), f);
	fclose(f);

	swap_endianness(pos, NULL);
}


