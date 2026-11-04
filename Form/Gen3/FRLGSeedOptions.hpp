/*
 * This file is part of PokeFinderPlus, an unofficial fork of PokéFinder
 * Copyright (C) 2026 by Vict0rious7
 *
 * FRLG initial seed options for the Gen 3 searchers (Ten Lines' Initial Seed inputs; Ten Lines by Lincoln-LM, GPL-3.0).
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

#ifndef FRLGSEEDOPTIONS_HPP
#define FRLGSEEDOPTIONS_HPP

#include <Model/Gen3/FRLGSeedColumns.hpp>
#include <QGroupBox>

class QCheckBox;
class QComboBox;
class QGridLayout;
class QSpinBox;

/**
 * @brief A seed to generate from in the Generator, with what to show in its Initial Seed / Seed Time columns
 */
struct CalibrationSeed
{
    u32 seed;
    RowExtra extra;
};

/**
 * @brief Game, console, settings and Teachy TV options for the FRLG initial seed columns
 */
class FRLGSeedOptions final : public QGroupBox
{
    Q_OBJECT
signals:
    /**
     * @brief Emitted when any option changes
     */
    void settingsChanged(const FRLGSeedSettings &settings);

public:
    /**
     * @brief Construct a new FRLGSeedOptions object
     *
     * @param parent Parent widget, which takes ownership
     */
    FRLGSeedOptions(QWidget *parent = nullptr);

    /**
     * @brief Destroy the FRLGSeedOptions object, saving the options
     */
    ~FRLGSeedOptions() override;

    /**
     * @brief Shows the options for an FRLG profile and hides them otherwise
     *
     * @param frlg Whether the current profile is FireRed or LeafGreen
     * @param leafGreen Whether it is LeafGreen (picks the matching game the first time)
     */
    void setProfile(bool frlg, bool leafGreen);

    /**
     * @brief Current options
     */
    FRLGSeedSettings getSettings() const;

    /**
     * @brief Sets sound, button mode, seed button and extra button from a Ten Lines settings key (e.g. mono_a_start_startup_select)
     */
    void setSettingsKey(const std::string &settings);

    /**
     * @brief Full Ten Lines settings key (e.g. mono_a_a_none), empty if any option is "Any"
     */
    std::string settingsKey() const;

    /**
     * @brief Seeds for the Generator: \p seed and, for an FRLG profile with Seed ± > 0, its neighbours in the farmed list
     * (Ten Lines' calibration). Falls back to \p seed alone, and explains why in \p note.
     */
    std::vector<CalibrationSeed> calibrationSeeds(u32 seed, u32 leeway, QString &note) const;

    /**
     * @brief Puts a "Seed ±" spin box (Ten Lines default 20) to the right of \p seedBox in \p grid
     *
     * @return The spin box; it and its label only show for FRLG profiles
     */
    QSpinBox *addSeedLeeway(QGridLayout *grid, QWidget *seedBox);

private:
    QComboBox *game;
    QComboBox *console;
    QComboBox *sound;
    QComboBox *buttonMode;
    QComboBox *button;
    QComboBox *held;
    QCheckBox *teachyTV;
    QSpinBox *regularOut;
    QString savedGame;
    bool frlg = false;

private slots:
    /**
     * @brief Refills the console list for the selected game and emits the new options
     */
    void gameChanged();

    /**
     * @brief Emits the new options
     */
    void update();
};

#endif // FRLGSEEDOPTIONS_HPP
