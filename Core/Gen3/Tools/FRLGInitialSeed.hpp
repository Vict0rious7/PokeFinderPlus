/*
 * This file is part of PokeFinderPlus, an unofficial fork of PokéFinder
 * Copyright (C) 2026 by Vict0rious7
 *
 * Port of Ten Lines' FRLG initial seed lookup (src/wasm/src/initial_seed.{hpp,cpp} and src/tenLines/index.ts)
 * by Lincoln-LM, https://github.com/Lincoln-LM/ten-lines, GPL-3.0.
 * FRLG seeds farmed by blisy, po, HunarPG, 10Ben, Real96, ColdStoneSys, Papa Jefé, and トノ.
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
 */

#ifndef FRLGINITIALSEED_HPP
#define FRLGINITIALSEED_HPP

#include <Core/Global.hpp>
#include <string>
#include <vector>

/**
 * @brief Best reachable FRLG initial seed for a target seed (Ten Lines' InitialSeedResult)
 */
struct FRLGInitialSeedResult
{
    u32 advances; ///< Advances from the initial seed to the target
    u32 seedTime; ///< Ten Lines' seed time: 16 x frame of the title screen press (NX: ms + 5737)
    u16 initialSeed; ///< 16-bit initial seed
    std::string settings; ///< Ten Lines' key: sound_buttonmode_button_heldmodifier_heldbutton, e.g. mono_a_start_startup_select
};

/**
 * @brief A farmed initial seed and its Ten Lines seed time (16 x frame of the title screen press)
 */
struct FRLGSeedTime
{
    u16 initialSeed;
    u32 seedTime;
};

/**
 * @brief Settings filter. An empty string means "Any".
 */
struct FRLGSettingsFilter
{
    std::string sound; ///< "mono" or "stereo"
    std::string buttonMode; ///< "a" (L=A), "h" (Help) or "r" (LR)
    std::string button; ///< "a", "start" or "l"
    std::string held; ///< "none", "startup_select", "startup_a", "blackout_r", "blackout_a", "blackout_l", "blackout_al"
};

namespace FRLGInitialSeed
{
    /**
     * @brief Ten Lines' game versions, in its menu order: fr, fr_eu, fr_jpn_1_0, fr_jpn_1_1, fr_nx, fr_jpn_nx, fr_mgba, lg, ...
     */
    const std::vector<std::string> &gameVersions();

    /**
     * @brief Consoles as in Ten Lines: GBA, GBP, NDS, 3DS, NX, NX2
     */
    const std::vector<std::string> &consoles();

    /**
     * @brief Whether bundled seed data exists for \p gameVersion
     */
    bool hasData(const std::string &gameVersion);

    /**
     * @brief When the bundled seed data was pulled (empty if none)
     */
    std::string dataTimestamp();

    /**
     * @brief Reachable initial seeds closest to \p target, sorted by advances (Ten Lines' ten_lines_frlg)
     *
     * @param target Target PRNG state (the searcher's Seed)
     * @param count Number of results
     * @param offset Advances subtracted from the target first
     * @param gameVersion Ten Lines game version key
     * @param ttvFramesOut Teachy TV mode: minimum regular frames (0 when Teachy TV is off)
     * @param filter Settings filter (PokeFinderPlus addition)
     *
     * @return Results, fewest advances first
     */
    std::vector<FRLGInitialSeedResult> find(u32 target, u16 count, u32 offset, const std::string &gameVersion, u32 ttvFramesOut,
                                            const FRLGSettingsFilter &filter);

    /**
     * @brief Ten Lines' get_contiguous_seed_list: farmed seeds of one setting in frame order
     *
     * @param gameVersion Ten Lines game version key
     * @param settings Full Ten Lines settings key, e.g. mono_a_a_none or mono_a_start_startup_select
     */
    std::vector<FRLGSeedTime> contiguousSeeds(const std::string &gameVersion, const std::string &settings);

    /**
     * @brief Ten Lines' calibration seed list: \p seed and up to \p leeway seeds before and after it in the farmed order
     *
     * @return Empty if \p seed is not in the list for these settings
     */
    std::vector<FRLGSeedTime> neighbourSeeds(const std::string &gameVersion, const std::string &settings, u16 seed, u32 leeway);

    /**
     * @brief Ten Lines' frameToMS: floor(frame / frame rate x 1000) + console offset
     */
    int frameToMS(double frame, const std::string &console);

    /**
     * @brief Ten Lines' teachyTVConversion
     *
     * @param advances Advances to the target
     * @param minimumOut Minimum regular frames
     * @param ttvAdvances Frames spent in Teachy TV (313 advances each)
     * @param regularAdvances Remaining regular advances
     */
    void teachyTVConversion(u32 advances, u32 minimumOut, u32 &ttvAdvances, u32 &regularAdvances);

    /**
     * @brief Ten Lines' humanizeSettings, e.g. "Mono | L=A | Seed Button: Start | Extra Button: Startup Select"
     */
    std::string humanizeSettings(const std::string &settings);
}

#endif // FRLGINITIALSEED_HPP
