/*
 * This file is part of PokeFinderPlus, an unofficial fork of PokéFinder
 * Copyright (C) 2026 by Vict0rious7
 *
 * FRLG initial seed columns for the Gen 3 searchers, using the port of Ten Lines (Lincoln-LM, GPL-3.0).
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

#include "FRLGSeedColumns.hpp"
#include <QObject>
#include <QString>

int FRLGSeedColumns::count() const
{
    return settings.enabled ? (settings.teachyTV ? 7 : 5) : 0;
}

QVariant FRLGSeedColumns::header(int column) const
{
    // Ten Lines' Initial Seed table columns
    QStringList headers = { QObject::tr("Initial Seed"), QObject::tr("Seed Time (ms)"), QObject::tr("Advances") };
    if (settings.teachyTV)
    {
        headers << QObject::tr("Final A Press Frame") << QObject::tr("Teachy TV Advances");
    }
    headers << QObject::tr("Total Time") << QObject::tr("Settings");
    return column < headers.size() ? headers[column] : QVariant();
}

QVariant FRLGSeedColumns::data(u32 seed, int column) const
{
    auto it = cache.find(seed);
    if (it == cache.end())
    {
        auto results
            = FRLGInitialSeed::find(seed, 1, 0, settings.gameVersion, settings.teachyTV ? settings.regularOut : 0, settings.filter);
        it = cache.emplace(seed, results.empty() ? std::nullopt : std::optional(results.front())).first;
    }
    if (!it->second)
    {
        return QString("-");
    }

    const auto &result = *it->second;
    u32 visualFrame = result.advances;
    u32 ttvAdvances = 0;
    if (settings.teachyTV)
    {
        u32 regular;
        FRLGInitialSeed::teachyTVConversion(result.advances, settings.regularOut, ttvAdvances, regular);
        visualFrame = ttvAdvances + regular;
    }

    if (!settings.teachyTV && column >= 3)
    {
        column += 2;
    }
    switch (column)
    {
    case 0:
        return QString::number(result.initialSeed, 16).toUpper().rightJustified(4, '0');
    case 1:
        return FRLGInitialSeed::frameToMS(result.seedTime / 16.0, settings.console);
    case 2:
        return result.advances;
    case 3:
        return visualFrame;
    case 4:
        return ttvAdvances;
    case 5:
    {
        qint64 ms = FRLGInitialSeed::frameToMS(result.seedTime / 16.0 + visualFrame, settings.console);
        qint64 h = ms / 3600000, m = ms / 60000 % 60, s = ms / 1000 % 60, z = ms % 1000;
        return QString("%1:%2:%3.%4")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'))
            .arg(z, 3, 10, QChar('0'));
    }
    case 6:
        return QString::fromStdString(FRLGInitialSeed::humanizeSettings(result.settings));
    default:
        return QVariant();
    }
}

void FRLGSeedColumns::setSettings(const FRLGSeedSettings &settings)
{
    this->settings = settings;
    cache.clear();
}
