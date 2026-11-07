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

#ifndef STATICTEMPLATE6_HPP
#define STATICTEMPLATE6_HPP

#include <Core/Parents/StaticTemplate.hpp>
#include <array>

/**
 * @brief Gen 6 static encounter, with the settings 3DSRNGTool's Stationary6 reads from its PKM6 templates
 *
 * ability: 0 = random (1 or 2), 1, 2, 3 = hidden ability (3DSRNGTool numbering)
 * gender: if randomGender, the threshold compared with rand(252); else 0 = genderless, 1 = male, 2 = female
 */
class StaticTemplate6 : public StaticTemplate
{
public:
    constexpr StaticTemplate6(Game version, u16 specie, u8 form, bool shinyLocked, u8 ability, u8 gender, bool randomGender, bool iv3,
                              u8 level, const std::array<s8, 6> &ivs, u8 nature, bool alwaysSync, bool syncable, u16 delay,
                              bool instantSync, s32 ottsv, bool gift, bool unstable) :
        StaticTemplate(version, specie, form, shinyLocked ? Shiny::Never : Shiny::Random, ability, gender, iv3 ? 3 : 0, level),
        ivs(ivs),
        ottsv(ottsv),
        delay(delay),
        alwaysSync(alwaysSync),
        gift(gift),
        instantSync(instantSync),
        randomGender(randomGender),
        syncable(syncable),
        unstable(unstable),
        nature(nature)
    {
    }

    /// Whether the nature is always the lead's (gifts, fixed natures): no 60-advance sync check
    bool getAlwaysSync() const
    {
        return alwaysSync;
    }

    /// Advances between the A press and the generation (3DSRNGTool "Delay")
    u16 getDelay() const
    {
        return delay;
    }

    /// Fixed IVs, -1 = random
    std::array<s8, 6> getFixedIVs() const
    {
        return ivs;
    }

    bool getGift() const
    {
        return gift;
    }

    bool getInstantSync() const
    {
        return instantSync;
    }

    /// Fixed nature, 255 = none
    u8 getNature() const
    {
        return nature;
    }

    /// In-game trade: original trainer's shiny value, -1 = none
    s32 getOTTSV() const
    {
        return ottsv;
    }

    bool getRandomGender() const
    {
        return randomGender;
    }

    bool getShinyLocked() const
    {
        return shiny == Shiny::Never;
    }

    /// Whether Synchronize can apply
    bool getSyncable() const
    {
        return syncable;
    }

    /// 3DSRNGTool marks these "(?)": the delay isn't stable
    bool getUnstable() const
    {
        return unstable;
    }

private:
    std::array<s8, 6> ivs;
    s32 ottsv;
    u16 delay;
    bool alwaysSync;
    bool gift;
    bool instantSync;
    bool randomGender;
    bool syncable;
    bool unstable;
    u8 nature;
};

#endif // STATICTEMPLATE6_HPP
