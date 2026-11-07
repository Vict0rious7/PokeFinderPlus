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
 * PokeFinderPlus: new file (Gen 6 support, ported from 3DSRNGTool by wwwwwwzx, MIT).
 */

#ifndef STATE6_HPP
#define STATE6_HPP

#include <Core/Parents/States/State.hpp>

/**
 * @brief Gen 6 generator state: a GeneratorState plus whether Synchronize applied
 */
class State6 : public GeneratorState
{
public:
    State6(u32 advances, u32 ec, u32 pid, const std::array<u8, 6> &ivs, u8 ability, u8 gender, u8 level, u8 nature, u8 shiny, bool sync,
           const PersonalInfo *info) :
        GeneratorState(advances, ec, pid, ivs, ability, gender, level, nature, shiny, info), sync(sync)
    {
    }

    /// Whether the nature came from Synchronize
    bool getSync() const
    {
        return sync;
    }

private:
    bool sync;
};

#endif // STATE6_HPP
