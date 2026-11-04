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

#include <Core/Enum/Method.hpp>
#include <Core/Gen3/Tools/FRLGInitialSeed.hpp>
#include <Model/SortFilterProxyModel.hpp>
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
     * @brief Numeric sort key of extra column \p column (Qt::UserRole); results without an initial seed sort last
     */
    QVariant sortData(u32 seed, int column) const;

    /**
     * @brief Table column of Total Time, counting the Seed column before the extra columns
     */
    int totalTimeColumn() const
    {
        return 1 + (settings.teachyTV ? 5 : 3);
    }

    /**
     * @brief Replaces the options and clears the cache
     */
    void setSettings(const FRLGSeedSettings &settings);

    /**
     * @brief Best initial seed for \p seed (nullopt if none or disabled)
     */
    std::optional<FRLGInitialSeedResult> result(u32 seed) const;

    /**
     * @brief Current options
     */
    const FRLGSeedSettings &getSettings() const
    {
        return settings;
    }

private:
    FRLGSeedSettings settings;
    mutable std::unordered_map<u32, std::optional<FRLGInitialSeedResult>> cache;
};

/**
 * @brief Per-row data the Gen 3 states don't carry: the method that produced the row, and (Generator) the initial seed used
 */
struct RowExtra
{
    Method method = Method::None;
    int initialSeed = -1; ///< -1 when not from the FRLG seed list
    int seedTimeMs = -1; ///< -1 when unknown
};

/**
 * @brief "Method 1", "Method 2", "Method 4" (static) or "Wild 1", "Wild 2", "Wild 4" (wild), as in the method lists
 */
QString methodName(Method method, bool wild = false);

/**
 * @brief Extra Generator columns: Initial Seed and Seed Time (ms) before Advances, Method after it
 */
class GeneratorExtraColumns
{
public:
    bool seed = false; ///< Show Initial Seed and Seed Time
    bool method = false; ///< Show Method
    bool wild = false; ///< Wild method names

    /**
     * @brief Number of extra columns
     */
    int count() const;

    /**
     * @brief Maps a table column
     *
     * @param column Table column
     * @param upstream Set to the model's own column when the table column is not an extra one
     *
     * @return Extra column (0 Initial Seed, 1 Seed Time, 2 Method), or -1 for the model's own columns
     */
    int map(int column, int &upstream) const;

    /**
     * @brief Header of extra column \p extra (0 Initial Seed, 1 Seed Time, 2 Method)
     */
    QVariant header(int extra) const;

    /**
     * @brief Value of extra column \p column (0 Initial Seed, 1 Seed Time, 2 Method) for \p extra
     */
    QVariant data(const RowExtra &extra, int column) const;
};

/**
 * @brief Sorts by Qt::UserRole when both cells have one (FRLG columns: numbers, Total Time in ms), by the shown text otherwise
 */
class FRLGSortProxyModel : public SortFilterProxyModel
{
public:
    FRLGSortProxyModel(QObject *parent, QAbstractItemModel *model) : SortFilterProxyModel(parent, model)
    {
    }

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override
    {
        QVariant l = left.data(Qt::UserRole);
        QVariant r = right.data(Qt::UserRole);
        if (l.isValid() && r.isValid())
        {
            return l.toLongLong() < r.toLongLong();
        }
        return SortFilterProxyModel::lessThan(left, right);
    }
};

#endif // FRLGSEEDCOLUMNS_HPP
