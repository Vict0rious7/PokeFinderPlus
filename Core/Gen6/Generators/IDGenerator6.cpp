/*
 * This file is part of PokéFinder
 * Copyright (C) 2017-2024 by Admiral_Fish, bumba, and EzPzStreamz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * PokeFinderPlus: new file. XY Trainer ID RNG: the ID each TinyMT output would give (6genTidSearch, BZL).
 */

#include "IDGenerator6.hpp"
#include <Core/Gen6/XYTid.hpp>

std::vector<IDState6> generateIDs6(const std::array<u32, 4> &state, u32 min, u32 max, const IDFilter6 &filter)
{
    std::vector<IDState6> states;
    XYTid::Tiny mt = XYTid::Tiny::fromState(state);
    mt.advance(min);
    for (u32 advance = min; advance <= max; advance++)
    {
        IDState6 id(advance, mt.next());
        if (filter.compare(id))
        {
            states.emplace_back(id);
        }
        if (advance == 0xffffffff)
        {
            break;
        }
    }
    return states;
}
