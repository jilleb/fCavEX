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

#ifndef COMPLEX_BLOCK_ARCHIVE_H
#define COMPLEX_BLOCK_ARCHIVE_H

#include <m-lib/m-string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../cNBT/nbt.h"
#include "../cglm/cglm.h"

#include "../item/inventory.h"
#include "../item/items.h"
#include "../world.h"
#include "server_local.h"

void chest_archive_read(struct complex_block_pos* pos, struct item_data* items, string_t path);
void chest_archive_write(struct complex_block_pos* pos, struct item_data* items, string_t path);
void sign_archive_read(struct complex_block_pos* pos, char* texts, string_t path);
void sign_archive_write(struct complex_block_pos* pos, char* texts, string_t path);
#endif
