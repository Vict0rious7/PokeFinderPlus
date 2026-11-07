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

#ifndef STATICGENERATOR6_HPP
#define STATICGENERATOR6_HPP

#include <Core/Gen6/Profile6.hpp>
#include <Core/Gen6/StaticTemplate6.hpp>
#include <Core/Parents/Filters/StateFilter.hpp>
#include <Core/Parents/Generators/StaticGenerator.hpp>

class State6;

/**
 * @brief Gen 6 (XY, ORAS) static encounter generator: port of 3DSRNGTool's Stationary6 ("Normal" search, no TinyMT timeline)
 *
 * Advance n uses MT outputs from n + 1 (+ delay when considered, + 60 for the sync check when the template isn't always synced).
 */
class StaticGenerator6 : public StaticGenerator<StaticTemplate6, Profile6, StateFilter>
{
public:
    /**
     * @param initialAdvances Initial number of advances
     * @param maxAdvances Maximum number of advances
     * @param offset Delay added to every advance (3DSRNGTool's "Consider Delay": usually the template's delay)
     * @param lead Synchronize nature, or Lead::None
     * @param assumeSynced Whether the TinyMT sync check passed (3DSRNGTool "Assume Synced")
     * @param staticTemplate Pokemon template
     * @param profile Profile information
     * @param filter State filter
     */
    StaticGenerator6(u32 initialAdvances, u32 maxAdvances, u32 offset, Lead lead, bool assumeSynced, const StaticTemplate6 &staticTemplate,
                     const Profile6 &profile, const StateFilter &filter);

    /**
     * @brief Generates states
     *
     * @param seed Initial MT seed
     *
     * @return Vector of computed states
     */
    std::vector<State6> generate(u32 seed) const;

private:
    bool assumeSynced;
};

#endif // STATICGENERATOR6_HPP
