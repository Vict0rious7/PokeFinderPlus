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

#include "StaticModel3.hpp"
#include <Core/Util/Translator.hpp>

StaticGeneratorModel3::StaticGeneratorModel3(QObject *parent) : TableModel(parent), showStats(false)
{
}

int StaticGeneratorModel3::columnCount(const QModelIndex &parent) const
{
    return 14 + extraColumns.count();
}

QVariant StaticGeneratorModel3::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        const auto &state = model[index.row()];
        int column = index.column();
        // PokeFinderPlus: Initial Seed and Seed Time before Advances, Method after it
        int extra = extraColumns.map(column, column);
        if (extra != -1)
        {
            return extraColumns.data(index.row() < static_cast<int>(extras.size()) ? extras[index.row()] : RowExtra(), extra);
        }
        switch (column)
        {
        case 0:
            return state.getAdvances();
        case 1:
            return QString::number(state.getPID(), 16).toUpper().rightJustified(8, '0');
        case 2:
        {
            u8 shiny = state.getShiny();
            return shiny == 2 ? tr("Square") : shiny == 1 ? tr("Star") : tr("No");
        }
        case 3:
            return QString::fromStdString(Translator::getNature(state.getNature()));
        case 4:
            return QString("%1: %2").arg(state.getAbility()).arg(QString::fromStdString(Translator::getAbility(state.getAbilityIndex())));
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            return showStats ? state.getStat(column - 5) : state.getIV(column - 5);
        case 11:
            return QString::fromStdString(Translator::getHiddenPower(state.getHiddenPower()));
        case 12:
            return state.getHiddenPowerStrength();
        case 13:
            return QString::fromStdString(Translator::getGender(state.getGender()));
        }
    }
    return QVariant();
}

QVariant StaticGeneratorModel3::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        // PokeFinderPlus: Initial Seed and Seed Time before Advances, Method after it
        int extra = extraColumns.map(section, section);
        return extra != -1 ? extraColumns.header(extra) : QVariant(header[section]);
    }
    return QVariant();
}

void StaticGeneratorModel3::setShowStats(bool flag)
{
    showStats = flag;
    emit dataChanged(index(0, 5 + extraColumns.count()), index(rowCount() - 1, 10 + extraColumns.count()), { Qt::DisplayRole });
}

StaticSearcherModel3::StaticSearcherModel3(QObject *parent) : TableModel(parent), showStats(false)
{
}

int StaticSearcherModel3::columnCount(const QModelIndex &parent) const
{
    return 14 + frlg.count() + (showMethod ? 1 : 0);
}

QVariant StaticSearcherModel3::data(const QModelIndex &index, int role) const
{
    // PokeFinderPlus: numeric sort keys for the FRLG columns (see FRLGSortProxyModel)
    if (role == Qt::UserRole)
    {
        int column = index.column();
        return column >= 1 && column <= frlg.count() ? frlg.sortData(model[index.row()].getSeed(), column - 1) : QVariant();
    }
    if (role == Qt::DisplayRole)
    {
        const auto &state = model[index.row()];
        int column = index.column();
        // PokeFinderPlus: FRLG initial seed columns, then Method, right after Seed
        if (column >= 1 && column <= frlg.count())
        {
            return frlg.data(state.getSeed(), column - 1);
        }
        if (showMethod && column == frlg.count() + 1)
        {
            return methodName(index.row() < static_cast<int>(methods.size()) ? methods[index.row()] : Method::None);
        }
        if (column > frlg.count())
        {
            column -= frlg.count() + (showMethod ? 1 : 0);
        }
        switch (column)
        {
        case 0:
            return QString::number(state.getSeed(), 16).toUpper().rightJustified(8, '0');
        case 1:
            return QString::number(state.getPID(), 16).toUpper().rightJustified(8, '0');
        case 2:
        {
            u8 shiny = state.getShiny();
            return shiny == 2 ? tr("Square") : shiny == 1 ? tr("Star") : tr("No");
        }
        case 3:
            return QString::fromStdString(Translator::getNature(state.getNature()));
        case 4:
            return QString("%1: %2").arg(state.getAbility()).arg(QString::fromStdString(Translator::getAbility(state.getAbilityIndex())));
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            return showStats ? state.getStat(column - 5) : state.getIV(column - 5);
        case 11:
            return QString::fromStdString(Translator::getHiddenPower(state.getHiddenPower()));
        case 12:
            return state.getHiddenPowerStrength();
        case 13:
            return QString::fromStdString(Translator::getGender(state.getGender()));
        }
    }
    return QVariant();
}

QVariant StaticSearcherModel3::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        // PokeFinderPlus: FRLG initial seed columns right after Seed
        if (section >= 1 && section <= frlg.count())
        {
            return frlg.header(section - 1);
        }
        if (showMethod && section == frlg.count() + 1)
        {
            return tr("Method");
        }
        return header[section > frlg.count() ? section - frlg.count() - (showMethod ? 1 : 0) : section];
    }
    return QVariant();
}

void StaticSearcherModel3::setShowStats(bool flag)
{
    showStats = flag;
    emit dataChanged(index(0, 5 + frlg.count() + (showMethod ? 1 : 0)), index(rowCount() - 1, 10 + frlg.count() + (showMethod ? 1 : 0)),
                     { Qt::DisplayRole });
}

void StaticSearcherModel3::setFRLGSettings(const FRLGSeedSettings &settings)
{
    beginResetModel();
    frlg.setSettings(settings);
    endResetModel();
}

void StaticSearcherModel3::addItems(const std::vector<SearcherState> &states, Method method)
{
    methods.insert(methods.end(), states.size(), method);
    TableModel<SearcherState>::addItems(states);
}

void StaticSearcherModel3::clearModel()
{
    methods.clear();
    TableModel<SearcherState>::clearModel();
}

int StaticSearcherModel3::getTotalTimeColumn() const
{
    return frlg.count() > 0 ? frlg.totalTimeColumn() : -1;
}

Method StaticSearcherModel3::getMethod(int row) const
{
    return row < static_cast<int>(methods.size()) ? methods[row] : Method::None;
}

void StaticSearcherModel3::setShowMethod(bool flag)
{
    beginResetModel();
    showMethod = flag;
    endResetModel();
}

std::optional<FRLGInitialSeedResult> StaticSearcherModel3::getFRLGResult(int row) const
{
    return frlg.result(model[row].getSeed());
}

void StaticGeneratorModel3::addItems(const std::vector<GeneratorState> &states, const RowExtra &extra)
{
    extras.insert(extras.end(), states.size(), extra);
    TableModel<GeneratorState>::addItems(states);
}

void StaticGeneratorModel3::clearModel()
{
    extras.clear();
    TableModel<GeneratorState>::clearModel();
}

void StaticGeneratorModel3::setExtraColumns(bool seed, bool method)
{
    if (extraColumns.seed != seed || extraColumns.method != method)
    {
        beginResetModel();
        extraColumns.seed = seed;
        extraColumns.method = method;
        endResetModel();
    }
}
