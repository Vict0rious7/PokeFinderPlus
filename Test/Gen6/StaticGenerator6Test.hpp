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
 * PokeFinderPlus: new file. Checks the Gen 6 static generator against 3DSRNGTool 1.0.6's own output
 * (Test/Gen6/static6_3dsrngtool.json, made by running 3DSRNGTool's Stationary6 code; see the golden3ds tool).
 */

#ifndef STATICGENERATOR6TEST_HPP
#define STATICGENERATOR6TEST_HPP

#include <QObject>

class StaticGenerator6Test : public QObject
{
    Q_OBJECT
private slots:
    void generate_data();
    void generate();
    void categories();
};

#endif // STATICGENERATOR6TEST_HPP
