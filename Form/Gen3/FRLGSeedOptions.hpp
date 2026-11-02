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
class QLabel;
class QSpinBox;

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

private:
    QComboBox *game;
    QComboBox *console;
    QComboBox *sound;
    QComboBox *buttonMode;
    QComboBox *button;
    QComboBox *held;
    QCheckBox *teachyTV;
    QSpinBox *regularOut;
    QLabel *status;
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
