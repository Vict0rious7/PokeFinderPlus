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
 * PokeFinderPlus: new file (Gen 6 / Gen 7 support, ported from 3DSRNGTool by wwwwwwzx, MIT).
 */

#ifndef PROFILE6_HPP
#define PROFILE6_HPP

#include <Core/Parents/Profile.hpp>

enum class Game : u32;

/**
 * @brief Provides additional storage specific to Gen6
 */
class Profile6 : public Profile
{
public:
    /**
     * @brief Construct a new Profile6 object
     *
     * @param name Profile name
     * @param version Game version
     * @param tid Trainer ID
     * @param sid Secret ID
     * @param shinyCharm Whether the shiny charm is obtained
     */
    Profile6(const std::string &name, Game version, u16 tid, u16 sid, bool shinyCharm) :
        Profile(name, version, tid, sid), shinyCharm(shinyCharm)
    {
    }

    /**
     * @brief Returns the shiny charm status
     *
     * @return true Shiny charm is obtained
     * @return false Shiny charm is not obtained
     */
    bool getShinyCharm() const
    {
        return shinyCharm;
    }

    /**
     * @brief Returns the trainer shiny value (TSV): (TID ^ SID) >> 4
     */
    u16 getTSV() const
    {
        return (tid ^ sid) >> 4;
    }

    /**
     * @brief Returns the trainer residual value (TRV): (TID ^ SID) & 0xF, used for square shinies
     */
    u8 getTRV() const
    {
        return (tid ^ sid) & 0xf;
    }

    /**
     * @brief Checks if two profiles are equal
     *
     * @param other Profile to compare
     *
     * @return true Profiles are equal
     * @return false Profiles are not equal
     */
    bool operator==(const Profile6 &other) const;

    /**
     * @brief Checks if two profiles are not equal
     *
     * @param other Profile to compare
     *
     * @return true Profiles are not equal
     * @return false Profiles are equal
     */
    bool operator!=(const Profile6 &other) const;

private:
    bool shinyCharm;
};

#endif // PROFILE6_HPP
