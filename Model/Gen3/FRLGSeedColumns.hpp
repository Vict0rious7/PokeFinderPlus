/*
 * This file is part of PokeFinderPlus, an unofficial fork of PokéFinder
 * Copyright (C) 2026 by Vict0rious7
 *
 * FRLG initial seed columns for the Gen 3 searchers, using the port of Ten Lines (Lincoln-LM, GPL-3.0).
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

#ifndef FRLGSEEDCOLUMNS_HPP
#define FRLGSEEDCOLUMNS_HPP

#include <Core/Gen3/Tools/FRLGInitialSeed.hpp>
#include <QVariant>
#include <optional>
#include <unordered_map>

/**
 * @brief Options for the FRLG initial seed columns (Ten Lines' Initial Seed tab inputs)
 */
struct FRLGSeedSettings
{
    bool enabled = false; ///< FRLG profile and seed data available
    std::string gameVersion = "fr"; ///< Ten Lines game version key
    std::string console = "GBA"; ///< GBA, GBP, NDS, 3DS, NX, NX2
    FRLGSettingsFilter filter; ///< Settings to search, empty = Any
    bool teachyTV = false; ///< Teachy TV mode
    u32 regularOut = 3600; ///< Teachy TV mode: minimum regular frames
};

/**
 * @brief Extra searcher columns: best initial seed for each result's seed
 */
class FRLGSeedColumns
{
public:
    /**
     * @brief Number of extra columns (0 when disabled)
     */
    int count() const;

    /**
     * @brief Header of extra column \p column
     */
    QVariant header(int column) const;

    /**
     * @brief Value of extra column \p column for a result whose seed is \p seed
     */
    QVariant data(u32 seed, int column) const;

    /**
     * @brief Replaces the options and clears the cache
     */
    void setSettings(const FRLGSeedSettings &settings);

private:
    FRLGSeedSettings settings;
    mutable std::unordered_map<u32, std::optional<FRLGInitialSeedResult>> cache;
};

#endif // FRLGSEEDCOLUMNS_HPP
