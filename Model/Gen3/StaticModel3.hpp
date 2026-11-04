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
 * FRLG initial seed columns (port of Ten Lines) after Seed in the searcher model.
 */

#ifndef STATIC3MODEL_HPP
#define STATIC3MODEL_HPP

#include <Core/Parents/States/State.hpp>
#include <Model/Gen3/FRLGSeedColumns.hpp>
#include <Model/TableModel.hpp>

/**
 * @brief Provides a table model implementation to show static encounter information for Gen 3
 */
class StaticGeneratorModel3 final : public TableModel<GeneratorState>
{
    Q_OBJECT
public:
    using TableModel<GeneratorState>::addItems;

    /**
     * @brief PokeFinderPlus: adds states generated from \p extra's initial seed with its method
     */
    void addItems(const std::vector<GeneratorState> &states, const RowExtra &extra);

    /**
     * @brief PokeFinderPlus: clears states and their extras
     */
    void clearModel();

    /**
     * @brief PokeFinderPlus: which extra columns to show (Initial Seed + Seed Time, Method)
     */
    void setExtraColumns(bool seed, bool method);

    /**
     * @brief Construct a new StaticGeneratorModel3 object
     *
     * @param parent Parent object, which takes memory ownership
     */
    StaticGeneratorModel3(QObject *parent);

    /**
     * @brief Returns the number of columns in the model
     *
     * @param parent Unused parent index
     *
     * @return Number of columns
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Returns data at the \p index with \p role
     *
     * @param index Row/column index
     * @param role Model data role
     *
     * @return Data at index
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /**
     * @brief Returns header text at the \p section, \p orientation, and \p role
     *
     * @param section Column index
     * @param orientation Header position
     * @param role Model data role
     *
     * @return Header text at column
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public slots:
    /**
     * @brief Sets flag that controls whether the model display stats or IVs
     *
     * @param flag Whether to show stats or not
     */
    void setShowStats(bool flag);

private:
    std::vector<RowExtra> extras; // PokeFinderPlus
    GeneratorExtraColumns extraColumns; // PokeFinderPlus
    QStringList header = { tr("Advances"), tr("PID"), tr("Shiny"), tr("Nature"), tr("Ability"), tr("HP"),    tr("Atk"),
                           tr("Def"),      tr("SpA"), tr("SpD"),   tr("Spe"),    tr("Hidden"),  tr("Power"), tr("Gender") };
    bool showStats;
};

/**
 * @brief Provides a table model implementation to show static encounter information for Gen 3
 */
class StaticSearcherModel3 : public TableModel<SearcherState>
{
    Q_OBJECT
public:
    using TableModel<SearcherState>::addItems;

    /**
     * @brief PokeFinderPlus: adds results found with \p method (shown when the Method column is on)
     */
    void addItems(const std::vector<SearcherState> &states, Method method);

    /**
     * @brief PokeFinderPlus: clears results and their methods
     */
    void clearModel();

    /**
     * @brief PokeFinderPlus: method of row \p row
     */
    Method getMethod(int row) const;

    /**
     * @brief PokeFinderPlus: shows the Method column ("All methods" searches)
     */
    void setShowMethod(bool flag);

    /**
     * @brief PokeFinderPlus: best FRLG initial seed of row \p row (nullopt if none or FRLG columns are off)
     */
    std::optional<FRLGInitialSeedResult> getFRLGResult(int row) const;

    /**
     * @brief Construct a new StaticSearcherModel3 object
     *
     * @param parent Parent object, which takes memory ownership
     */
    StaticSearcherModel3(QObject *parent);

    /**
     * @brief Returns the number of columns in the model
     *
     * @param parent Unused parent index
     *
     * @return Number of columns
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Returns data at the \p index with \p role
     *
     * @param index Row/column index
     * @param role Model data role
     *
     * @return Data at index
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /**
     * @brief Returns header text at the \p section, \p orientation, and \p role
     *
     * @param section Column index
     * @param orientation Header position
     * @param role Model data role
     *
     * @return Header text at column
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public slots:
    /**
     * @brief PokeFinderPlus: sets the FRLG initial seed columns
     */
    void setFRLGSettings(const FRLGSeedSettings &settings);

    /**
     * @brief Sets flag that controls whether the model display stats or IVs
     *
     * @param flag Whether to show stats or not
     */
    void setShowStats(bool flag);

private:
    std::vector<Method> methods; // PokeFinderPlus
    bool showMethod = false; // PokeFinderPlus
    FRLGSeedColumns frlg; // PokeFinderPlus
    QStringList header = { tr("Seed"), tr("PID"), tr("Shiny"), tr("Nature"), tr("Ability"), tr("HP"),    tr("Atk"),
                           tr("Def"),  tr("SpA"), tr("SpD"),   tr("Spe"),    tr("Hidden"),  tr("Power"), tr("Gender") };
    bool showStats;
};

#endif // STATIC3MODEL_HPP
