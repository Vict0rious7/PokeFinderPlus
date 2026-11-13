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
 * PokeFinderPlus: new file. XY Trainer ID RNG, Finder side: blink taps → initial seed, seed → IDs, and the handoff to
 * PokeTimerPlus. Method and values: BZL's 6genTidSearch (used with permission), https://blog.bzl-web.com/entry/2020/08/31/202306
 * 2026-11-13: ORAS (OR / AS in the game list): base seed from the TIDs of new games launched at known times, then the
 * SID / TSV of a new game. BZL's ORAS method, https://blog.bzl-web.com/entry/2020/08/24/225424
 */

#ifndef IDS6_HPP
#define IDS6_HPP

#include <Core/Global.hpp>
#include <QElapsedTimer>
#include <QWidget>
#include <array>
#include <vector>

class BlinkModel6;
class IDsFilter;
class BlinkSearcher6;
class IDModel6;
class OrasSearcher6;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QStandardItemModel;
class QTableWidget;
class QTabWidget;
class TableView;
class TextBox;

/**
 * @brief XY IDs: find the initial seed from the professor's blinks, list the IDs, send the pick to PokeTimerPlus.
 * ORAS IDs: find the base seed from launches, then the SID / TSV of a new game.
 */
class IDs6 final : public QWidget
{
    Q_OBJECT
public:
    IDs6(QWidget *parent = nullptr);

    ~IDs6() override;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    QTabWidget *games; ///< X/Y, OR/AS

    // X/Y: Seed Finder (blinks)
    QPushButton *blink;
    QPushButton *clear;
    QLabel *tapStatus;
    QPlainTextEdit *gaps;
    TextBox *advanceMin;
    TextBox *advanceMax;
    QRadioButton *all;
    QRadioButton *near;
    TextBox *baseSeed;
    TextBox *hours;
    QPushButton *search;
    QPushButton *cancel;
    QProgressBar *progress;
    BlinkModel6 *seedModel;
    TableView *seedTable;

    // X/Y: Generator
    QLineEdit *state;
    QLineEdit *firstTap;
    TextBox *idMin;
    TextBox *idMax;
    IDsFilter *filter;
    QPushButton *generate;
    IDModel6 *idModel;
    TableView *idTable;
    QPushButton *send;
    QPushButton *copy;

    // OR/AS: Seed Finder (launches)
    QPushButton *launch;
    QPushButton *removeLaunch;
    QPushButton *clearLaunches;
    QLabel *launchStatus;
    QTableWidget *launches;
    TextBox *tolerance;
    QPushButton *orasSearch;
    QPushButton *orasCancel;
    QProgressBar *orasProgress;
    QStandardItemModel *baseModel;
    TableView *baseTable;

    // OR/AS: SID Finder
    TextBox *orasBase;
    TextBox *orasLaunchMs;
    TextBox *orasTid;
    TextBox *orasNo;
    TextBox *orasTolerance;
    QPushButton *identify;
    QStandardItemModel *identModel;
    TableView *identTable;
    qint64 launchEpoch = 0; ///< first launch (ms since 1970), 0 if none; kept across window openings
    OrasSearcher6 *orasRunning = nullptr;

    QElapsedTimer tapTimer;
    qint64 tapEpoch = 0; ///< first tap of the current recording (ms since 1970), 0 if none
    qint64 lastTapMs = 0;
    bool recording = false;
    qint64 searchTapEpoch = 0; ///< first tap of the recording the last search used
    BlinkSearcher6 *running = nullptr;

    std::vector<u32> parseGaps(bool *ok) const;
    bool parseState(std::array<u32, 4> &words) const;
    bool isOras() const;
    void saveLaunches() const;
    void fillFromLastLaunch();
    QString handoffText(bool *ok);

private slots:
    void tap();
    void clearTaps();
    void startSearch();
    void useSeed(const QModelIndex &index);
    void generateIDs();
    void sendToTimer();
    void copyForTimer();
    void addLaunch();
    void removeLaunchRow();
    void clearLaunchList();
    void startOrasSearch();
    void useBaseSeed(const QModelIndex &index);
    void identifyLaunch();
};

#endif // IDS6_HPP
