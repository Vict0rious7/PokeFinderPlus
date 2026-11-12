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

#ifndef BLINKMODEL6_HPP
#define BLINKMODEL6_HPP

#include <Core/Gen6/Searchers/BlinkSearcher6.hpp>
#include <Model/TableModel.hpp>

class BlinkModel6 final : public TableModel<BlinkResult6>
{
    Q_OBJECT
public:
    BlinkModel6(QObject *parent);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    /**
     * @brief Near a base seed: also shows the time since \p base (the seed grows by one per millisecond of boot)
     */
    void setNear(bool near, u32 base);

private:
    u32 base = 0;
    bool near = false;
};

#endif // BLINKMODEL6_HPP
