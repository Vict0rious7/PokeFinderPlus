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
 * PokeFinderPlus: new file. Results of the XY blink seed search.
 */

#include "BlinkModel6.hpp"

namespace
{
    QString hex(u32 value)
    {
        return QString::number(value, 16).toUpper().rightJustified(8, '0');
    }
}

BlinkModel6::BlinkModel6(QObject *parent) : TableModel(parent)
{
}

int BlinkModel6::columnCount(const QModelIndex &parent) const
{
    return near ? 4 : 3;
}

QVariant BlinkModel6::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        const auto &result = model[index.row()];
        switch (index.column())
        {
        case 0:
            return hex(result.seed);
        case 1:
            return result.advance;
        case 2:
            return QString("%1,%2,%3,%4").arg(hex(result.state[3]), hex(result.state[2]), hex(result.state[1]), hex(result.state[0]));
        case 3:
        {
            u32 ms = result.seed - base;
            return QString("%1:%2:%3.%4")
                .arg(ms / 3600000)
                .arg(ms / 60000 % 60, 2, 10, QChar('0'))
                .arg(ms / 1000 % 60, 2, 10, QChar('0'))
                .arg(ms % 1000, 3, 10, QChar('0'));
        }
        }
    }
    return QVariant();
}

QVariant BlinkModel6::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return tr("Initial seed");
        case 1:
            return tr("Advance");
        case 2:
            return tr("State at first tap");
        case 3:
            return tr("Time since base seed");
        }
    }
    return QVariant();
}

void BlinkModel6::setNear(bool near, u32 base)
{
    beginResetModel();
    this->near = near;
    this->base = base;
    endResetModel();
}
