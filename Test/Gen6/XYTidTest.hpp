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
 * PokeFinderPlus: new file. Checks the XY TID code (Core/Gen6/XYTid, BlinkSearcher6, IDGenerator6) against 6genTidSearch's
 * own code (Test/Gen6/xytid_6gentidsearch.json, see the golden6tid tool).
 */

#ifndef XYTIDTEST_HPP
#define XYTIDTEST_HPP

#include <QObject>

class XYTidTest : public QObject
{
    Q_OBJECT
private slots:
    void tinyMT();
    void blinkInterval();
    void hitFrame();
    void timeline_data();
    void timeline();
    void searchNearBase();
    void idList();
    void handoff();
};

#endif // XYTIDTEST_HPP
