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

#include "StaticGenerator6.hpp"
#include <Core/Enum/Lead.hpp>
#include <Core/Enum/Method.hpp>
#include <Core/Gen6/States/State6.hpp>
#include <Core/RNG/MT.hpp>

namespace
{
    /// rand(n) as in 3DSRNGTool: (u32 * n) >> 32
    u32 rand(u32 value, u32 n)
    {
        return static_cast<u32>((static_cast<u64>(value) * n) >> 32);
    }
}

StaticGenerator6::StaticGenerator6(u32 initialAdvances, u32 maxAdvances, u32 offset, Lead lead, bool assumeSynced,
                                   const StaticTemplate6 &staticTemplate, const Profile6 &profile, const StateFilter &filter) :
    StaticGenerator(initialAdvances, maxAdvances, offset, Method::None, lead, staticTemplate, profile, filter), assumeSynced(assumeSynced)
{
}

std::vector<State6> StaticGenerator6::generate(u32 seed) const
{
    const PersonalInfo *info = staticTemplate.getInfo();

    // Shiny values: an in-game trade uses its original trainer's TSV (it is shiny-locked anyway)
    const u32 tsv = staticTemplate.getOTTSV() >= 0 ? static_cast<u32>(staticTemplate.getOTTSV()) : profile.getTSV();
    const u32 trv = profile.getTRV();

    const bool alwaysSync = staticTemplate.getAlwaysSync();
    const bool shinyLocked = staticTemplate.getShinyLocked();
    const u8 pidRolls = profile.getShinyCharm() && !shinyLocked && !alwaysSync ? 3 : 1;
    const bool sync = alwaysSync || assumeSynced;
    u8 syncNature = lead <= Lead::SynchronizeEnd ? toInt(lead) : 255;
    if (staticTemplate.getNature() < 25)
    {
        syncNature = staticTemplate.getNature();
    }

    // Worst case per advance: 60 (sync check) + 3 (EC/PID) + IVs (perfect IV rolls repeat) + ability, nature, gender
    const u32 window = 60 + 3 + 64 + 6 + 3;
    std::vector<u32> rands(maxAdvances + offset + window);
    MT rng(seed, initialAdvances);
    for (u32 &rand : rands)
    {
        rand = rng.next();
    }

    std::vector<State6> states;
    for (u32 cnt = 0; cnt <= maxAdvances; cnt++)
    {
        size_t pointer = cnt + offset;
        auto next = [&]() -> u32 {
            if (++pointer >= rands.size()) // Only reachable with many perfect-IV re-rolls
            {
                MT extra(seed, initialAdvances + static_cast<u32>(pointer));
                return extra.next();
            }
            return rands[pointer];
        };

        if (!alwaysSync)
        {
            pointer += 60;
        }

        u32 ec = next();

        u32 pid = 0;
        u8 shiny = 0;
        for (u8 i = pidRolls; i > 0; i--)
        {
            pid = next();
            u32 psv = ((pid >> 16) ^ (pid & 0xffff)) >> 4;
            if (psv == tsv)
            {
                if (shinyLocked)
                {
                    pid ^= 0x10000000;
                }
                else
                {
                    u32 prv = ((pid >> 16) ^ (pid & 0xffff)) & 0xf;
                    shiny = prv == trv ? 2 : 1;
                }
                break;
            }
        }

        std::array<s8, 6> fixed = staticTemplate.getFixedIVs();
        std::array<u8, 6> ivs;
        std::array<bool, 6> set {};
        for (int i = 0; i < 6; i++)
        {
            set[i] = fixed[i] >= 0;
            ivs[i] = set[i] ? static_cast<u8>(fixed[i]) : 0;
        }
        for (u8 i = staticTemplate.getIVCount(); i > 0;)
        {
            u32 index = rand(next(), 6);
            if (!set[index])
            {
                i--;
                set[index] = true;
                ivs[index] = 31;
            }
        }
        for (int i = 0; i < 6; i++)
        {
            if (!set[i])
            {
                ivs[i] = next() >> 27;
            }
        }

        // 3DSRNGTool numbering 1, 2, 3 (hidden) -> PokeFinder 0, 1, 2
        u8 ability = staticTemplate.getAbility() == 0 ? (next() >> 31) : staticTemplate.getAbility() - 1;

        u8 nature = sync && syncNature < 25 ? syncNature : rand(next(), 25);

        // 3DSRNGTool numbering 1 male, 2 female, 0 genderless -> PokeFinder 0, 1, 2
        u8 gender;
        if (staticTemplate.getRandomGender())
        {
            gender = rand(next(), 252) >= staticTemplate.getGender() ? 0 : 1;
        }
        else
        {
            gender = staticTemplate.getGender() == 1 ? 0 : staticTemplate.getGender() == 2 ? 1 : 2;
        }

        State6 state(initialAdvances + cnt, ec, pid, ivs, ability, gender, staticTemplate.getLevel(), nature, shiny, sync, info);
        if (filter.compare(state))
        {
            states.emplace_back(state);
        }
    }

    return states;
}
