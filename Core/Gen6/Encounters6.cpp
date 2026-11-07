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
 * PokeFinderPlus: new file (Gen 6 support). Encounter data generated from 3DSRNGTool 1.0.6 (wwwwwwzx, MIT) PKM6 templates.
 */

#include "Encounters6.hpp"
#include <Core/Enum/Game.hpp>
#include <Core/Gen6/StaticTemplate6.hpp>
#include <array>

namespace
{
    constexpr std::array<StaticTemplate6, 6> STATIC_X_0
        = { StaticTemplate6(Game::X, 716, 0, true, 1, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 158, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 718, 0, true, 1, 0, false, true, 70, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1, false,
                            false),
            StaticTemplate6(Game::X, 150, 0, true, 1, 0, false, true, 70, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 2, false, -1, false,
                            false),
            StaticTemplate6(Game::X, 144, 0, true, 1, 0, false, true, 70, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 114, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 145, 0, true, 1, 0, false, true, 70, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 114, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 146, 0, true, 1, 0, false, true, 70, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 114, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 2> STATIC_X_1
        = { StaticTemplate6(Game::X, 131, 0, false, 1, 126, true, false, 30, { 31, 20, 20, 20, 20, 20 }, 6, true, false, 18, false, -1,
                            true, false),
            StaticTemplate6(Game::X, 448, 0, true, 1, 1, false, false, 32, { 6, 25, 16, 25, 19, 31 }, 11, true, false, 20, false, -1, true,
                            false) };

    constexpr std::array<StaticTemplate6, 2> STATIC_X_2
        = { StaticTemplate6(Game::X, 16, 0, true, 1, 126, true, false, 3, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 14, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 143, 0, true, 0, 30, true, false, 15, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 6> STATIC_X_3
        = { StaticTemplate6(Game::X, 650, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 280, false, -1,
                            true, true),
            StaticTemplate6(Game::X, 653, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 280, false, -1,
                            true, true),
            StaticTemplate6(Game::X, 656, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 330, false, -1,
                            true, true),
            StaticTemplate6(Game::X, 1, 0, false, 0, 30, true, false, 10, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 4, false, -1, true,
                            false),
            StaticTemplate6(Game::X, 4, 0, false, 0, 30, true, false, 10, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 4, false, -1, true,
                            false),
            StaticTemplate6(Game::X, 7, 0, false, 0, 30, true, false, 10, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 4, false, -1, true,
                            false) };

    constexpr std::array<StaticTemplate6, 11> STATIC_X_4
        = { StaticTemplate6(Game::X, 138, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::X, 140, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::X, 142, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::X, 345, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::X, 347, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::X, 408, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::X, 410, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::X, 564, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::X, 566, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::X, 696, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::X, 698, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false) };

    constexpr std::array<StaticTemplate6, 9> STATIC_X_5
        = { StaticTemplate6(Game::X, 129, 0, true, 1, 1, false, false, 5, { -1, -1, -1, -1, -1, -1 }, 3, true, true, 0, false, 2767, false,
                            false),
            StaticTemplate6(Game::X, 133, 0, true, 1, 2, false, false, 5, { -1, -1, -1, -1, -1, -1 }, 6, true, true, 0, false, 1830, false,
                            false),
            StaticTemplate6(Game::X, 83, 0, true, 1, 1, false, false, 10, { -1, -1, -1, -1, -1, 31 }, 13, true, true, 0, false, 11, false,
                            false),
            StaticTemplate6(Game::X, 208, 0, true, 1, 2, false, false, 20, { -1, -1, 31, -1, -1, -1 }, 8, true, true, 0, false, 1203, false,
                            false),
            StaticTemplate6(Game::X, 625, 0, true, 1, 1, false, false, 50, { -1, 31, -1, -1, -1, -1 }, 3, true, true, 0, false, 215, false,
                            false),
            StaticTemplate6(Game::X, 656, 0, true, 1, 1, false, false, 5, { 20, 20, 20, 20, 20, 31 }, 13, true, true, 0, false, 2, false,
                            false),
            StaticTemplate6(Game::X, 650, 0, true, 1, 1, false, false, 5, { 20, 31, 20, 20, 20, 20 }, 3, true, true, 0, false, 2, false,
                            false),
            StaticTemplate6(Game::X, 653, 0, true, 1, 1, false, false, 5, { 20, 20, 20, 31, 20, 20 }, 15, true, true, 0, false, 2, false,
                            false),
            StaticTemplate6(Game::X, 280, 0, true, 1, 2, false, false, 5, { 20, 20, 20, 31, 20, 31 }, 15, true, true, 0, false, 2319, false,
                            false) };

    constexpr std::array<StaticTemplate6, 6> STATIC_X_6
        = { StaticTemplate6(Game::X, 165, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 313, 0, false, 0, 1, false, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 314, 0, false, 0, 2, false, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 412, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 415, 0, false, 0, 30, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 665, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 13> STATIC_X_7
        = { StaticTemplate6(Game::X, 527, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 76, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 597, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 76, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 168, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 76, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 714, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 76, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 75, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 76, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 543, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 54, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 531, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 54, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 632, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 54, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 631, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 54, false, -1,
                            false, false),
            StaticTemplate6(Game::X, 568, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 18, false, -1,
                            false, true),
            StaticTemplate6(Game::X, 569, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 18, false, -1,
                            false, true),
            StaticTemplate6(Game::X, 354, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 18, false, -1,
                            false, true),
            StaticTemplate6(Game::X, 479, 0, false, 0, 0, false, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 18, false, -1,
                            false, true) };

    constexpr std::array<StaticTemplate6, 6> STATIC_Y_0
        = { StaticTemplate6(Game::Y, 717, 0, true, 1, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 208, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 718, 0, true, 1, 0, false, true, 70, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1, false,
                            false),
            StaticTemplate6(Game::Y, 150, 0, true, 1, 0, false, true, 70, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 2, false, -1, false,
                            false),
            StaticTemplate6(Game::Y, 144, 0, true, 1, 0, false, true, 70, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 114, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 145, 0, true, 1, 0, false, true, 70, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 114, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 146, 0, true, 1, 0, false, true, 70, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 114, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 2> STATIC_Y_1
        = { StaticTemplate6(Game::Y, 131, 0, false, 1, 126, true, false, 30, { 31, 20, 20, 20, 20, 20 }, 6, true, false, 18, false, -1,
                            true, false),
            StaticTemplate6(Game::Y, 448, 0, true, 1, 1, false, false, 32, { 6, 25, 16, 25, 19, 31 }, 11, true, false, 20, false, -1, true,
                            false) };

    constexpr std::array<StaticTemplate6, 2> STATIC_Y_2
        = { StaticTemplate6(Game::Y, 16, 0, true, 1, 126, true, false, 3, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 14, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 143, 0, true, 0, 30, true, false, 15, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 6> STATIC_Y_3
        = { StaticTemplate6(Game::Y, 650, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 280, false, -1,
                            true, true),
            StaticTemplate6(Game::Y, 653, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 280, false, -1,
                            true, true),
            StaticTemplate6(Game::Y, 656, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 330, false, -1,
                            true, true),
            StaticTemplate6(Game::Y, 1, 0, false, 0, 30, true, false, 10, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 4, false, -1, true,
                            false),
            StaticTemplate6(Game::Y, 4, 0, false, 0, 30, true, false, 10, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 4, false, -1, true,
                            false),
            StaticTemplate6(Game::Y, 7, 0, false, 0, 30, true, false, 10, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 4, false, -1, true,
                            false) };

    constexpr std::array<StaticTemplate6, 11> STATIC_Y_4
        = { StaticTemplate6(Game::Y, 138, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::Y, 140, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::Y, 142, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::Y, 345, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::Y, 347, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::Y, 408, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::Y, 410, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::Y, 564, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::Y, 566, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::Y, 696, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::Y, 698, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false) };

    constexpr std::array<StaticTemplate6, 9> STATIC_Y_5
        = { StaticTemplate6(Game::Y, 129, 0, true, 1, 1, false, false, 5, { -1, -1, -1, -1, -1, -1 }, 3, true, true, 0, false, 2767, false,
                            false),
            StaticTemplate6(Game::Y, 133, 0, true, 1, 2, false, false, 5, { -1, -1, -1, -1, -1, -1 }, 6, true, true, 0, false, 1830, false,
                            false),
            StaticTemplate6(Game::Y, 83, 0, true, 1, 1, false, false, 10, { -1, -1, -1, -1, -1, 31 }, 13, true, true, 0, false, 11, false,
                            false),
            StaticTemplate6(Game::Y, 208, 0, true, 1, 2, false, false, 20, { -1, -1, 31, -1, -1, -1 }, 8, true, true, 0, false, 1203, false,
                            false),
            StaticTemplate6(Game::Y, 625, 0, true, 1, 1, false, false, 50, { -1, 31, -1, -1, -1, -1 }, 3, true, true, 0, false, 215, false,
                            false),
            StaticTemplate6(Game::Y, 656, 0, true, 1, 1, false, false, 5, { 20, 20, 20, 20, 20, 31 }, 13, true, true, 0, false, 2, false,
                            false),
            StaticTemplate6(Game::Y, 650, 0, true, 1, 1, false, false, 5, { 20, 31, 20, 20, 20, 20 }, 3, true, true, 0, false, 2, false,
                            false),
            StaticTemplate6(Game::Y, 653, 0, true, 1, 1, false, false, 5, { 20, 20, 20, 31, 20, 20 }, 15, true, true, 0, false, 2, false,
                            false),
            StaticTemplate6(Game::Y, 280, 0, true, 1, 2, false, false, 5, { 20, 20, 20, 31, 20, 31 }, 15, true, true, 0, false, 2319, false,
                            false) };

    constexpr std::array<StaticTemplate6, 6> STATIC_Y_6
        = { StaticTemplate6(Game::Y, 165, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 313, 0, false, 0, 1, false, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 314, 0, false, 0, 2, false, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 412, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 415, 0, false, 0, 30, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 665, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 13> STATIC_Y_7
        = { StaticTemplate6(Game::Y, 527, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 76, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 597, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 76, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 168, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 76, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 714, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 76, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 75, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 76, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 543, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 54, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 531, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 54, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 632, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 54, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 631, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 54, false, -1,
                            false, false),
            StaticTemplate6(Game::Y, 568, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 18, false, -1,
                            false, true),
            StaticTemplate6(Game::Y, 569, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 18, false, -1,
                            false, true),
            StaticTemplate6(Game::Y, 354, 0, false, 0, 126, true, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 18, false, -1,
                            false, true),
            StaticTemplate6(Game::Y, 479, 0, false, 0, 0, false, false, 0, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 18, false, -1,
                            false, true) };

    constexpr std::array<StaticTemplate6, 8> STATIC_OR_0
        = { StaticTemplate6(Game::OR, 381, 0, false, 1, 1, false, true, 30, { -1, -1, -1, -1, -1, -1 }, 255, true, true, 0, false, -1, true,
                            false),
            StaticTemplate6(Game::OR, 380, 0, false, 0, 2, false, true, 30, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, true, -1,
                            false, false),
            StaticTemplate6(Game::OR, 383, 0, true, 0, 0, false, true, 45, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 3300, false, -1,
                            false, true),
            StaticTemplate6(Game::OR, 384, 0, true, 0, 0, false, true, 70, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, true, -1, false,
                            false),
            StaticTemplate6(Game::OR, 386, 0, true, 0, 0, false, true, 80, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 300, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 377, 0, false, 0, 0, false, true, 40, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 158, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 378, 0, false, 0, 0, false, true, 40, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 154, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 379, 0, false, 0, 0, false, true, 40, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 164, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 4> STATIC_OR_1
        = { StaticTemplate6(Game::OR, 243, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 244, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 245, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 250, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 20, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 8> STATIC_OR_2
        = { StaticTemplate6(Game::OR, 480, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 481, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 482, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 484, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 88, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 485, 0, false, 0, 126, true, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 486, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 487, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 88, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 488, 0, false, 0, 2, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 7> STATIC_OR_3
        = { StaticTemplate6(Game::OR, 638, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 639, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 640, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 641, 0, false, 0, 1, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 88, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 645, 0, false, 0, 1, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 88, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 643, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 646, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 7> STATIC_OR_4
        = { StaticTemplate6(Game::OR, 360, 0, false, 1, 126, true, false, 1, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 175, 0, false, 1, 30, true, false, 1, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 374, 0, false, 1, 0, false, false, 1, { -1, -1, 31, -1, 31, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 351, 0, false, 1, 2, false, false, 30, { -1, -1, -1, 31, -1, -1 }, 9, true, false, 0, false, -1, true,
                            false),
            StaticTemplate6(Game::OR, 319, 0, false, 1, 2, false, false, 40, { -1, -1, -1, -1, -1, -1 }, 3, true, false, 0, false, -1, true,
                            false),
            StaticTemplate6(Game::OR, 323, 0, false, 1, 2, false, false, 40, { -1, -1, -1, -1, -1, -1 }, 17, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 25, 1, true, 3, 2, false, true, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, true, 0, false, -1, true,
                            false) };

    constexpr std::array<StaticTemplate6, 3> STATIC_OR_5
        = { StaticTemplate6(Game::OR, 296, 0, true, 2, 1, false, false, 9, { -1, 31, -1, -1, -1, -1 }, 2, true, true, 0, false, 1920, false,
                            false),
            StaticTemplate6(Game::OR, 300, 0, true, 1, 2, false, false, 30, { -1, -1, -1, -1, -1, 31 }, 4, true, true, 0, false, 202, false,
                            false),
            StaticTemplate6(Game::OR, 222, 0, true, 3, 2, false, false, 50, { 31, -1, -1, -1, 31, -1 }, 20, true, true, 0, false, 20, false,
                            false) };

    constexpr std::array<StaticTemplate6, 8> STATIC_OR_6
        = { StaticTemplate6(Game::OR, 352, 0, false, 0, 126, true, false, 30, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, true, -1,
                            false, false),
            StaticTemplate6(Game::OR, 352, 0, false, 0, 2, false, false, 40, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, true, -1,
                            false, false),
            StaticTemplate6(Game::OR, 352, 0, false, 3, 126, true, false, 45, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, true, -1,
                            false, false),
            StaticTemplate6(Game::OR, 101, 0, false, 0, 0, false, false, 40, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 122, true, -1,
                            false, false),
            StaticTemplate6(Game::OR, 100, 0, false, 0, 0, false, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 120, true, -1,
                            false, false),
            StaticTemplate6(Game::OR, 442, 0, false, 0, 126, true, false, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 265, 0, true, 0, 126, true, false, 3, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 14, false, -1,
                            false, false),
            StaticTemplate6(Game::OR, 261, 0, false, 0, 126, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 8, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 12> STATIC_OR_7
        = { StaticTemplate6(Game::OR, 252, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 186, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 255, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 138, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 258, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 138, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 152, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 155, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 158, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 495, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 498, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 501, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 387, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 390, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 393, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false) };

    constexpr std::array<StaticTemplate6, 11> STATIC_OR_8
        = { StaticTemplate6(Game::OR, 138, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 140, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 142, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 345, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 347, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 408, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 410, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 564, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 566, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 696, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::OR, 698, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false) };

    constexpr std::array<StaticTemplate6, 8> STATIC_AS_0
        = { StaticTemplate6(Game::AS, 380, 0, false, 1, 2, false, true, 30, { -1, -1, -1, -1, -1, -1 }, 255, true, true, 0, false, -1, true,
                            false),
            StaticTemplate6(Game::AS, 381, 0, false, 0, 1, false, true, 30, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, true, -1,
                            false, false),
            StaticTemplate6(Game::AS, 382, 0, true, 0, 0, false, true, 45, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 3300, false, -1,
                            false, true),
            StaticTemplate6(Game::AS, 384, 0, true, 0, 0, false, true, 70, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, true, -1, false,
                            false),
            StaticTemplate6(Game::AS, 386, 0, true, 0, 0, false, true, 80, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 300, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 377, 0, false, 0, 0, false, true, 40, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 158, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 378, 0, false, 0, 0, false, true, 40, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 154, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 379, 0, false, 0, 0, false, true, 40, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 164, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 4> STATIC_AS_1
        = { StaticTemplate6(Game::AS, 243, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 244, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 245, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 249, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 20, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 8> STATIC_AS_2
        = { StaticTemplate6(Game::AS, 480, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 481, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 482, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 483, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 88, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 485, 0, false, 0, 126, true, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 486, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 487, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 88, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 488, 0, false, 0, 2, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 7> STATIC_AS_3
        = { StaticTemplate6(Game::AS, 638, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 639, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 640, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 642, 0, false, 0, 1, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 88, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 645, 0, false, 0, 1, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 88, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 644, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 646, 0, false, 0, 0, false, true, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 16, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 7> STATIC_AS_4
        = { StaticTemplate6(Game::AS, 360, 0, false, 1, 126, true, false, 1, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 175, 0, false, 1, 30, true, false, 1, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 374, 0, false, 1, 0, false, false, 1, { -1, -1, 31, -1, 31, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 351, 0, false, 1, 2, false, false, 30, { -1, -1, -1, 31, -1, -1 }, 9, true, false, 0, false, -1, true,
                            false),
            StaticTemplate6(Game::AS, 319, 0, false, 1, 2, false, false, 40, { -1, -1, -1, -1, -1, -1 }, 3, true, false, 0, false, -1, true,
                            false),
            StaticTemplate6(Game::AS, 323, 0, false, 1, 2, false, false, 40, { -1, -1, -1, -1, -1, -1 }, 17, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 25, 1, true, 3, 2, false, true, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, true, 0, false, -1, true,
                            false) };

    constexpr std::array<StaticTemplate6, 3> STATIC_AS_5
        = { StaticTemplate6(Game::AS, 296, 0, true, 2, 1, false, false, 9, { -1, 31, -1, -1, -1, -1 }, 2, true, true, 0, false, 1920, false,
                            false),
            StaticTemplate6(Game::AS, 300, 0, true, 1, 2, false, false, 30, { -1, -1, -1, -1, -1, 31 }, 4, true, true, 0, false, 202, false,
                            false),
            StaticTemplate6(Game::AS, 222, 0, true, 3, 2, false, false, 50, { 31, -1, -1, -1, 31, -1 }, 20, true, true, 0, false, 20, false,
                            false) };

    constexpr std::array<StaticTemplate6, 8> STATIC_AS_6
        = { StaticTemplate6(Game::AS, 352, 0, false, 0, 126, true, false, 30, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, true, -1,
                            false, false),
            StaticTemplate6(Game::AS, 352, 0, false, 0, 2, false, false, 40, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, true, -1,
                            false, false),
            StaticTemplate6(Game::AS, 352, 0, false, 3, 126, true, false, 45, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, true, -1,
                            false, false),
            StaticTemplate6(Game::AS, 101, 0, false, 0, 0, false, false, 40, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 122, true, -1,
                            false, false),
            StaticTemplate6(Game::AS, 100, 0, false, 0, 0, false, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 120, true, -1,
                            false, false),
            StaticTemplate6(Game::AS, 442, 0, false, 0, 126, true, false, 50, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 0, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 265, 0, true, 0, 126, true, false, 3, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 14, false, -1,
                            false, false),
            StaticTemplate6(Game::AS, 261, 0, false, 0, 126, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, false, true, 8, false, -1,
                            false, false) };

    constexpr std::array<StaticTemplate6, 12> STATIC_AS_7
        = { StaticTemplate6(Game::AS, 252, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 186, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 255, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 138, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 258, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 138, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 152, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 155, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 158, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 495, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 498, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 501, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 387, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 390, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 393, 0, false, 0, 30, true, false, 5, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 0, false, -1,
                            true, false) };

    constexpr std::array<StaticTemplate6, 11> STATIC_AS_8
        = { StaticTemplate6(Game::AS, 138, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 140, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 142, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 345, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 347, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 408, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 410, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 564, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 566, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 696, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false),
            StaticTemplate6(Game::AS, 698, 0, false, 0, 30, true, false, 20, { -1, -1, -1, -1, -1, -1 }, 255, true, false, 80, false, -1,
                            true, false) };

    template <size_t N>
    const StaticTemplate6 *get(const std::array<StaticTemplate6, N> &array, int *size)
    {
        if (size)
        {
            *size = N;
        }
        return array.data();
    }
}

namespace Encounters6
{
    std::vector<std::string> getStaticCategories(Game version)
    {
        if ((version & Game::XY) != Game::None)
        {
            return { "Legendary", "In-Game Gift",  "Normal Stationary", "Starters",
                     "Fossils",   "In-Game Trade", "Berry Tree",        "Ambush Encounters" };
        }
        return { "Hoenn Legendary", "Johto Legendary",   "Sinnoh Legendary", "Unova Legendary", "In-Game Gift",
                 "In-Game Trade",   "Normal Stationary", "Starters",         "Fossils" };
    }

    const StaticTemplate6 *getStaticEncounters(Game version, int category, int *size)
    {
        if (version == Game::X)
        {
            switch (category)
            {
            case 0:
                return get(STATIC_X_0, size);
            case 1:
                return get(STATIC_X_1, size);
            case 2:
                return get(STATIC_X_2, size);
            case 3:
                return get(STATIC_X_3, size);
            case 4:
                return get(STATIC_X_4, size);
            case 5:
                return get(STATIC_X_5, size);
            case 6:
                return get(STATIC_X_6, size);
            case 7:
                return get(STATIC_X_7, size);
            default:
                break;
            }
        }
        if (version == Game::Y)
        {
            switch (category)
            {
            case 0:
                return get(STATIC_Y_0, size);
            case 1:
                return get(STATIC_Y_1, size);
            case 2:
                return get(STATIC_Y_2, size);
            case 3:
                return get(STATIC_Y_3, size);
            case 4:
                return get(STATIC_Y_4, size);
            case 5:
                return get(STATIC_Y_5, size);
            case 6:
                return get(STATIC_Y_6, size);
            case 7:
                return get(STATIC_Y_7, size);
            default:
                break;
            }
        }
        if (version == Game::OR)
        {
            switch (category)
            {
            case 0:
                return get(STATIC_OR_0, size);
            case 1:
                return get(STATIC_OR_1, size);
            case 2:
                return get(STATIC_OR_2, size);
            case 3:
                return get(STATIC_OR_3, size);
            case 4:
                return get(STATIC_OR_4, size);
            case 5:
                return get(STATIC_OR_5, size);
            case 6:
                return get(STATIC_OR_6, size);
            case 7:
                return get(STATIC_OR_7, size);
            case 8:
                return get(STATIC_OR_8, size);
            default:
                break;
            }
        }
        if (version == Game::AS)
        {
            switch (category)
            {
            case 0:
                return get(STATIC_AS_0, size);
            case 1:
                return get(STATIC_AS_1, size);
            case 2:
                return get(STATIC_AS_2, size);
            case 3:
                return get(STATIC_AS_3, size);
            case 4:
                return get(STATIC_AS_4, size);
            case 5:
                return get(STATIC_AS_5, size);
            case 6:
                return get(STATIC_AS_6, size);
            case 7:
                return get(STATIC_AS_7, size);
            case 8:
                return get(STATIC_AS_8, size);
            default:
                break;
            }
        }
        if (size)
        {
            *size = 0;
        }
        return nullptr;
    }

    const StaticTemplate6 *getStaticEncounter(Game version, int category, int pokemon)
    {
        int size = 0;
        const StaticTemplate6 *templates = getStaticEncounters(version, category, &size);
        return pokemon < size ? &templates[pokemon] : nullptr;
    }
}
