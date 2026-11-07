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
 * PokeFinderPlus: new file, Gen 6 initial seed finder from IVs (retailrng's fossil method: poke6-seed-finder's full search,
 * Gen 6 GPU Tools' quick search).
 */

#ifndef SEEDFINDER6_HPP
#define SEEDFINDER6_HPP

#include <QWidget>
#include <array>

class QComboBox;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QStackedWidget;
class SeedModel6;
class SeedSearcher6;
class TableView;
class TextBox;

/**
 * @brief Finds the Gen 6 initial MT seed from the IVs of Pokemon received right after boot
 */
class SeedFinder6 final : public QWidget
{
    Q_OBJECT
public:
    SeedFinder6(QWidget *parent = nullptr);

    ~SeedFinder6() override;

private:
    QComboBox *game;
    QRadioButton *full;
    QRadioButton *quick;
    QStackedWidget *pages;

    std::array<QSpinBox *, 6> ivs1;
    std::array<QSpinBox *, 6> ivs2;
    QSpinBox *frame1Min;
    QSpinBox *frame1Max;
    QSpinBox *frame2Min;
    QSpinBox *frame2Max;

    TextBox *baseSeed;
    std::array<QSpinBox *, 6> ivMin;
    std::array<QSpinBox *, 6> ivMax;
    QSpinBox *quickMin;
    QSpinBox *quickMax;
    QSpinBox *hours;

    QPushButton *search;
    QPushButton *cancel;
    QProgressBar *progress;
    SeedModel6 *model;
    TableView *table;
    SeedSearcher6 *running = nullptr; // search in progress, stopped if the window closes

private slots:
    void gameChanged(int index);

    void startSearch();
};

#endif // SEEDFINDER6_HPP
