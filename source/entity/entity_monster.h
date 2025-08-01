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


#ifndef ENTITY_MONSTER_H
#define ENTITY_MONSTER_H

#include <stdbool.h>
#include "../cglm/cglm.h"
#include "entity.h"

enum {
    MONSTER_ZOMBIE  = 0,
    MONSTER_CREEPER = 1,
    MONSTER_COUNT
};

void entity_monster(uint32_t id, struct entity* e, bool server, void* world, int monster_id);

#endif

