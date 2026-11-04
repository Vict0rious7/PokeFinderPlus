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

#include "WildModel3.hpp"
#include <Core/Util/Translator.hpp>
#include <QColor>
#include <QFont>

WildGeneratorModel3::WildGeneratorModel3(QObject *parent) : TableModel(parent), showStats(false)
{
}

int WildGeneratorModel3::columnCount(const QModelIndex &parent) const
{
    return 16 + extraColumns.count();
}

QVariant WildGeneratorModel3::data(const QModelIndex &index, int role) const
{
    const auto &state = model[index.row()];
    if (role == Qt::FontRole)
    {
        if (!state.isValid())
        {
            QFont font;
            font.setItalic(true);
            return font;
        }
    }
    else if (role == Qt::ForegroundRole)
    {
        if (!state.isValid())
        {
            return QColor(128, 128, 128);
        }
    }
    else if (role == Qt::DisplayRole)
    {
        int column = index.column();
        // PokeFinderPlus: Initial Seed and Seed Time before Advances, Method after it
        int extra = extraColumns.map(column, column);
        if (extra != -1)
        {
            return extraColumns.data(index.row() < static_cast<int>(extras.size()) ? extras[index.row()] : RowExtra(), extra);
        }
        if (!state.isValid() && column > 0)
        {
            return "-";
        }

        switch (column)
        {
        case 0:
            return state.getAdvances();
        case 1:
            return QString("%1: %2")
                .arg(state.getEncounterSlot())
                .arg(QString::fromStdString(Translator::getSpecie(state.getSpecie(), state.getForm())));
        case 2:
            return state.getLevel();
        case 3:
            return QString::number(state.getPID(), 16).toUpper().rightJustified(8, '0');
        case 4:
        {
            u8 shiny = state.getShiny();
            return shiny == 2 ? tr("Square") : shiny == 1 ? tr("Star") : tr("No");
        }
        case 5:
            return QString::fromStdString(Translator::getNature(state.getNature()));
        case 6:
            return QString("%1: %2").arg(state.getAbility()).arg(QString::fromStdString(Translator::getAbility(state.getAbilityIndex())));
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            return showStats ? state.getStat(column - 7) : state.getIV(column - 7);
        case 13:
            return QString::fromStdString(Translator::getHiddenPower(state.getHiddenPower()));
        case 14:
            return state.getHiddenPowerStrength();
        case 15:
            return QString::fromStdString(Translator::getGender(state.getGender()));
        }
    }
    return QVariant();
}

QVariant WildGeneratorModel3::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        // PokeFinderPlus: Initial Seed and Seed Time before Advances, Method after it
        int extra = extraColumns.map(section, section);
        return extra != -1 ? extraColumns.header(extra) : QVariant(header[section]);
    }
    return QVariant();
}

void WildGeneratorModel3::setShowStats(bool flag)
{
    showStats = flag;
    emit dataChanged(index(0, 7 + extraColumns.count()), index(rowCount() - 1, 12 + extraColumns.count()), { Qt::DisplayRole });
}

WildSearcherModel3::WildSearcherModel3(QObject *parent) : TableModel(parent), showStats(false)
{
}

int WildSearcherModel3::columnCount(const QModelIndex &parent) const
{
    return 16 + frlg.count() + (showMethod ? 1 : 0);
}

QVariant WildSearcherModel3::data(const QModelIndex &index, int role) const
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
            return methodName(index.row() < static_cast<int>(methods.size()) ? methods[index.row()] : Method::None, true);
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
            return QString("%1: %2")
                .arg(state.getEncounterSlot())
                .arg(QString::fromStdString(Translator::getSpecie(state.getSpecie(), state.getForm())));
        case 2:
            return state.getLevel();
        case 3:
            return QString::number(state.getPID(), 16).toUpper().rightJustified(8, '0');
        case 4:
        {
            u8 shiny = state.getShiny();
            return shiny == 2 ? tr("Square") : shiny == 1 ? tr("Star") : tr("No");
        }
        case 5:
            return QString::fromStdString(Translator::getNature(state.getNature()));
        case 6:
            return QString("%1: %2").arg(state.getAbility()).arg(QString::fromStdString(Translator::getAbility(state.getAbilityIndex())));
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            return showStats ? state.getStat(column - 7) : state.getIV(column - 7);
        case 13:
            return QString::fromStdString(Translator::getHiddenPower(state.getHiddenPower()));
        case 14:
            return state.getHiddenPowerStrength();
        case 15:
            return QString::fromStdString(Translator::getGender(state.getGender()));
        }
    }
    return QVariant();
}

QVariant WildSearcherModel3::headerData(int section, Qt::Orientation orientation, int role) const
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

void WildSearcherModel3::setShowStats(bool flag)
{
    showStats = flag;
    emit dataChanged(index(0, 7 + frlg.count() + (showMethod ? 1 : 0)), index(rowCount() - 1, 12 + frlg.count() + (showMethod ? 1 : 0)),
                     { Qt::DisplayRole });
}

void WildSearcherModel3::setFRLGSettings(const FRLGSeedSettings &settings)
{
    beginResetModel();
    frlg.setSettings(settings);
    endResetModel();
}

void WildSearcherModel3::addItems(const std::vector<WildSearcherState> &states, Method method)
{
    methods.insert(methods.end(), states.size(), method);
    TableModel<WildSearcherState>::addItems(states);
}

void WildSearcherModel3::clearModel()
{
    methods.clear();
    TableModel<WildSearcherState>::clearModel();
}

int WildSearcherModel3::getTotalTimeColumn() const
{
    return frlg.count() > 0 ? frlg.totalTimeColumn() : -1;
}

Method WildSearcherModel3::getMethod(int row) const
{
    return row < static_cast<int>(methods.size()) ? methods[row] : Method::None;
}

void WildSearcherModel3::setShowMethod(bool flag)
{
    beginResetModel();
    showMethod = flag;
    endResetModel();
}

std::optional<FRLGInitialSeedResult> WildSearcherModel3::getFRLGResult(int row) const
{
    return frlg.result(model[row].getSeed());
}

void WildGeneratorModel3::addItems(const std::vector<WildGeneratorState> &states, const RowExtra &extra)
{
    extras.insert(extras.end(), states.size(), extra);
    TableModel<WildGeneratorState>::addItems(states);
}

void WildGeneratorModel3::clearModel()
{
    extras.clear();
    TableModel<WildGeneratorState>::clearModel();
}

void WildGeneratorModel3::setExtraColumns(bool seed, bool method)
{
    if (extraColumns.seed != seed || extraColumns.method != method)
    {
        beginResetModel();
        extraColumns.seed = seed;
        extraColumns.method = method;
        extraColumns.wild = true;
        endResetModel();
    }
}
