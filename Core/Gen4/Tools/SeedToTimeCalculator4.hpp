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
 * Modified 2026-11-02 by Vict0rious7 for PokeFinderPlus (unofficial fork):
 * added the "Today" seed picker (parseSeedList, calculateTimesToday, rankToday).
 */

#ifndef SEEDTOTIMECALCULATOR4_HPP
#define SEEDTOTIMECALCULATOR4_HPP

#include <Core/Gen4/SeedTime4.hpp>
#include <string>
#include <vector>

/**
 * @brief Seed that can still be hit today without changing the clock (PokeFinderPlus)
 */
struct TodaySeed4
{
    u32 seed; ///< PRNG state
    std::vector<SeedTime4> times; ///< Remaining date/times today, one per try
};

namespace SeedToTimeCalculator4
{
    /**
     * @brief Generates date/times from a seed. Can filter by a specific second with \p forceSecond and \p forcedSecond
     *
     * @param seed PRNG state
     * @param year Year to search
     * @param forceSecond Whether to filter by second or not
     * @param forcedSecond Second to filter by
     *
     * @return Vector of generated date/times
     */
    std::vector<SeedTime4> calculateTimes(u32 seed, u16 year, bool forceSecond, u8 forcedSecond);

    /**
     * @brief Generates calibration results within -/+ \p delayCalibration and -/+ \p secondCalibration from the selected \p target.
     *
     * @param delayCalibration -/+ delay range
     * @param secondCalibration -/+ second range
     * @param target Date/time target
     *
     * @return Vector of calibration date/times
     */
    std::vector<SeedTimeCalibrate4> calibrate(int delayCalibration, int secondCalibration, const SeedTime4 &target);

    /**
     * @brief Generates calibration results within -/+ \p delayCalibration and -/+ \p secondCalibration from the selected \p target.
     *
     * @param delayCalibration -/+ delay range
     * @param secondCalibration -/+ second range
     * @param playerRoute Player location
     * @param roamers Which roamers are active
     * @param routes Which route active roamers are on
     * @param target Date/time target
     *
     * @return Vector of calibration date/times
     */
    std::vector<SeedTimeCalibrate4> calibrate(int delayCalibration, int secondCalibration, u8 playerRoute, const std::array<bool, 3> &roamers,
                                              std::array<u8, 3> &routes, const SeedTime4 &target);

    /**
     * @brief Reads one seed per line from \p text: the first value on each line (so whole copied table rows work).
     * Blank lines are ignored. Lines whose first value is not 1-8 hex digits (optional 0x) are counted in \p rejected.
     *
     * @param text Pasted text
     * @param rejected Number of rejected lines
     *
     * @return Seeds in input order, duplicates removed
     */
    std::vector<u32> parseSeedList(const std::string &text, u32 &rejected);

    /**
     * @brief calculateTimes() restricted to the date of \p now, from the next full minute on.
     * Each returned row is one try: wait for its minute, then hit its second with its delay.
     *
     * @param seed PRNG state
     * @param now Current date/time
     *
     * @return Remaining date/times today
     */
    std::vector<SeedTime4> calculateTimesToday(u32 seed, const DateTime &now);

    /**
     * @brief Ranks the seeds that can still be hit today: earliest hour first, then most tries left, then shortest delay,
     * then input order.
     *
     * @param seeds PRNG states
     * @param now Current date/time
     * @param parity -1 for any delay, 0 for even delays only, 1 for odd delays only
     *
     * @return Ranked seeds with their remaining times; empty if none can be hit today
     */
    std::vector<TodaySeed4> rankToday(const std::vector<u32> &seeds, const DateTime &now, int parity);
};

#endif // SEEDTOTIMECALCULATOR4_HPP
