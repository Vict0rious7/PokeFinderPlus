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
 * PokeFinderPlus: new file (Gen 6 support).
 */

#ifndef ENCOUNTERS6_HPP
#define ENCOUNTERS6_HPP

#include <Core/Global.hpp>
#include <string>
#include <vector>

class StaticTemplate6;
enum class Game : u32;

namespace Encounters6
{
    /**
     * @brief Names of the static categories for \p version (XY and ORAS have different lists)
     */
    std::vector<std::string> getStaticCategories(Game version);

    /**
     * @brief Static encounters of \p category for one game (X, Y, OR or AS), in 3DSRNGTool's order
     *
     * @param version Single game
     * @param category Category index (see getStaticCategories)
     * @param size Set to the number of templates
     *
     * @return Templates, or nullptr if none
     */
    const StaticTemplate6 *getStaticEncounters(Game version, int category, int *size = nullptr);

    /**
     * @brief One static encounter, or nullptr if out of range
     */
    const StaticTemplate6 *getStaticEncounter(Game version, int category, int pokemon);
}

#endif // ENCOUNTERS6_HPP
