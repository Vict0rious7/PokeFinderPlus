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
#include <limits>

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

std::optional<FRLGInitialSeedResult> FRLGSeedColumns::result(u32 seed) const
{
    if (!settings.enabled)
    {
        return std::nullopt;
    }
    auto it = cache.find(seed);
    if (it == cache.end())
    {
        auto results
            = FRLGInitialSeed::find(seed, 1, 0, settings.gameVersion, settings.teachyTV ? settings.regularOut : 0, settings.filter);
        it = cache.emplace(seed, results.empty() ? std::nullopt : std::optional(results.front())).first;
    }
    return it->second;
}

QVariant FRLGSeedColumns::data(u32 seed, int column) const
{
    auto found = result(seed);
    if (!found)
    {
        return QString("-");
    }

    const auto &best = *found;
    u32 visualFrame = best.advances;
    u32 ttvAdvances = 0;
    if (settings.teachyTV)
    {
        u32 regular;
        FRLGInitialSeed::teachyTVConversion(best.advances, settings.regularOut, ttvAdvances, regular);
        visualFrame = ttvAdvances + regular;
    }

    if (!settings.teachyTV && column >= 3)
    {
        column += 2;
    }
    switch (column)
    {
    case 0:
        return QString::number(best.initialSeed, 16).toUpper().rightJustified(4, '0');
    case 1:
        return FRLGInitialSeed::frameToMS(best.seedTime / 16.0, settings.console);
    case 2:
        return best.advances;
    case 3:
        return visualFrame;
    case 4:
        return ttvAdvances;
    case 5:
    {
        qint64 ms = FRLGInitialSeed::frameToMS(best.seedTime / 16.0 + visualFrame, settings.console);
        qint64 h = ms / 3600000, m = ms / 60000 % 60, s = ms / 1000 % 60, z = ms % 1000;
        return QString("%1:%2:%3.%4")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'))
            .arg(z, 3, 10, QChar('0'));
    }
    case 6:
        return QString::fromStdString(FRLGInitialSeed::humanizeSettings(best.settings));
    default:
        return QVariant();
    }
}

QVariant FRLGSeedColumns::sortData(u32 seed, int column) const
{
    if (!settings.teachyTV && column >= 3)
    {
        column += 2;
    }
    if (column == 6)
    {
        return QVariant(); // Settings: sort by text
    }
    auto found = result(seed);
    if (!found)
    {
        return std::numeric_limits<qint64>::max();
    }

    u32 visualFrame = found->advances;
    u32 ttvAdvances = 0;
    if (settings.teachyTV)
    {
        u32 regular;
        FRLGInitialSeed::teachyTVConversion(found->advances, settings.regularOut, ttvAdvances, regular);
        visualFrame = ttvAdvances + regular;
    }
    switch (column)
    {
    case 0:
        return static_cast<qint64>(found->initialSeed);
    case 1:
        return static_cast<qint64>(FRLGInitialSeed::frameToMS(found->seedTime / 16.0, settings.console));
    case 2:
        return static_cast<qint64>(found->advances);
    case 3:
        return static_cast<qint64>(visualFrame);
    case 4:
        return static_cast<qint64>(ttvAdvances);
    default:
        return static_cast<qint64>(FRLGInitialSeed::frameToMS(found->seedTime / 16.0 + visualFrame, settings.console));
    }
}

void FRLGSeedColumns::setSettings(const FRLGSeedSettings &settings)
{
    this->settings = settings;
    cache.clear();
}

QString methodName(Method method, bool wild)
{
    switch (method)
    {
    case Method::Method1:
        return wild ? QObject::tr("Wild 1") : QObject::tr("Method 1");
    case Method::Method2:
        return wild ? QObject::tr("Wild 2") : QObject::tr("Method 2");
    case Method::Method4:
        return wild ? QObject::tr("Wild 4") : QObject::tr("Method 4");
    default:
        return QString("-");
    }
}

int GeneratorExtraColumns::count() const
{
    return (seed ? 2 : 0) + (method ? 1 : 0);
}

int GeneratorExtraColumns::map(int column, int &upstream) const
{
    int before = seed ? 2 : 0;
    if (column < before)
    {
        return column;
    }
    if (column == before)
    {
        upstream = 0;
        return -1;
    }
    if (method && column == before + 1)
    {
        return 2;
    }
    upstream = column - before - (method ? 1 : 0);
    return -1;
}

QVariant GeneratorExtraColumns::header(int extra) const
{
    switch (extra)
    {
    case 0:
        return QObject::tr("Initial Seed");
    case 1:
        return QObject::tr("Seed Time (ms)");
    default:
        return QObject::tr("Method");
    }
}

QVariant GeneratorExtraColumns::data(const RowExtra &extra, int column) const
{
    switch (column)
    {
    case 0:
        return extra.initialSeed < 0 ? QVariant("-") : QVariant(QString::number(extra.initialSeed, 16).toUpper().rightJustified(4, '0'));
    case 1:
        return extra.seedTimeMs < 0 ? QVariant("-") : QVariant(extra.seedTimeMs);
    default:
        return methodName(extra.method, wild);
    }
}
