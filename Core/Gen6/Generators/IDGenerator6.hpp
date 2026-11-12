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
 * PokeFinderPlus: new file. XY Trainer ID RNG: the ID each TinyMT output would give, from the state at the first blink tap
 * (6genTidSearch's 目標ID検索 tab, BZL).
 */

#ifndef IDGENERATOR6_HPP
#define IDGENERATOR6_HPP

#include <Core/Parents/States/IDState.hpp>
#include <array>
#include <optional>
#include <vector>

/**
 * @brief An XY ID: TID, SID, TSV and TRV from one TinyMT output
 */
class IDState6 : public IDState
{
public:
    IDState6(u32 advances, u32 rand) :
        IDState(advances, rand & 0xffff, rand >> 16, ((rand & 0xffff) ^ (rand >> 16)) >> 4), trv(((rand & 0xffff) ^ (rand >> 16)) & 15)
    {
    }

    u8 getTRV() const
    {
        return trv;
    }

private:
    u8 trv;
};

/**
 * @brief Exact values to keep; an empty one accepts everything
 */
struct IDFilter6
{
    std::optional<u16> tid;
    std::optional<u16> sid;
    std::optional<u16> tsv;
    std::optional<u8> trv;

    bool compare(const IDState6 &state) const
    {
        return (!tid || *tid == state.getTID()) && (!sid || *sid == state.getSID()) && (!tsv || *tsv == state.getTSV())
            && (!trv || *trv == state.getTRV());
    }
};

/**
 * @brief Lists the IDs of TinyMT outputs \p min to \p max after \p state (advance 0 = the first output, i.e. the first
 * tapped blink's interval)
 */
std::vector<IDState6> generateIDs6(const std::array<u32, 4> &state, u32 min, u32 max, const IDFilter6 &filter);

#endif // IDGENERATOR6_HPP
