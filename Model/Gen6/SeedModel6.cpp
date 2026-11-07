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
 * PokeFinderPlus: new file, Gen 6 initial seed search results.
 */

#include "SeedModel6.hpp"

SeedModel6::SeedModel6(QObject *parent) : TableModel(parent)
{
}

int SeedModel6::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant SeedModel6::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        const auto &result = model[index.row()];
        switch (index.column())
        {
        case 0:
            return QString::number(result.seed, 16).toUpper().rightJustified(8, '0');
        case 1:
            return result.frame1;
        case 2:
            if (quick)
            {
                // GPU Tools: the seed grows by one per millisecond of boot time
                u32 ms = result.seed - base;
                return QString("%1:%2:%3.%4")
                    .arg(ms / 3600000)
                    .arg(ms / 60000 % 60, 2, 10, QChar('0'))
                    .arg(ms / 1000 % 60, 2, 10, QChar('0'))
                    .arg(ms % 1000, 3, 10, QChar('0'));
            }
            return result.frame2;
        }
    }
    return QVariant();
}

QVariant SeedModel6::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return tr("Seed");
        case 1:
            return quick ? tr("Frame") : tr("Frame 1");
        case 2:
            return quick ? tr("Time since base seed") : tr("Frame 2");
        }
    }
    return QVariant();
}

void SeedModel6::setQuick(bool quick, u32 base)
{
    beginResetModel();
    this->quick = quick;
    this->base = base;
    endResetModel();
}
