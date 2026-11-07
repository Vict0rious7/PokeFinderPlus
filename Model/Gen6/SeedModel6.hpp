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

#ifndef SEEDMODEL6_HPP
#define SEEDMODEL6_HPP

#include <Core/Gen6/Searchers/SeedSearcher6.hpp>
#include <Model/TableModel.hpp>

/**
 * @brief Initial seeds found from IVs. Quick search rows also show the boot time since the base seed (1 seed = 1 ms).
 */
class SeedModel6 final : public TableModel<SeedResult6>
{
    Q_OBJECT
public:
    SeedModel6(QObject *parent);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    /**
     * @brief Quick search: shows one frame and the time since \p base; full search: two frames
     */
    void setQuick(bool quick, u32 base);

private:
    u32 base = 0;
    bool quick = false;
};

#endif // SEEDMODEL6_HPP
