/*
 * This file is part of PokeFinderPlus, an unofficial fork of PokéFinder
 * Copyright (C) 2026 by Vict0rious7
 *
 * Tests for the port of Ten Lines' FRLG initial seed lookup (Ten Lines by Lincoln-LM, GPL-3.0).
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

#ifndef FRLGINITIALSEEDTEST_HPP
#define FRLGINITIALSEEDTEST_HPP

#include <QObject>

class FRLGInitialSeedTest : public QObject
{
    Q_OBJECT
private slots:
    void find_data();
    void find();
    void filterExcludesEverything();
    void teachyTV();
    void humanize();
    void neighbourSeeds();
};

#endif // FRLGINITIALSEEDTEST_HPP
