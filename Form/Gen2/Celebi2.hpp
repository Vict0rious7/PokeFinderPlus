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
 * PokeFinderPlus: new file. Gen 2: shiny GS Ball Celebi in Pokémon Crystal (3DS Virtual Console). One window, the
 * steps of SPEC v0.6: game files and options, calibration, plans 1 and 2, the hunt with the switch rule, re-saves.
 */

#ifndef CELEBI2_HPP
#define CELEBI2_HPP

#include <Core/Gen2/CelebiSession.hpp>
#include <Core/Gen2/CelebiTool.hpp>
#include <QWidget>
#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <optional>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QTableWidget;
class QTextBrowser;
class QThread;

/**
 * @brief Shiny GS Ball Celebi (Crystal, 3DS Virtual Console): calibration, plans, hunt advice.
 */
class Celebi2 final : public QWidget
{
    Q_OBJECT
public:
    Celebi2(QWidget *parent = nullptr);
    ~Celebi2() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    // ---- state (saved in the session file)
    QString sessionPath;
    QString filesKind; // cia, rom+patch, dump
    QStringList filePaths;
    std::optional<Gen2::GameFiles> files;
    Gen2::Visible visible;
    int accuracy = 3;
    int timerMs = 0; // PokeTimerPlus calibration, ms (moves with calibration rounds and timer advice)
    QString playTime;
    std::optional<Gen2::Plan> plan; // discovered for the language + options (not saved: ~3 s)
    std::vector<Gen2::Attempt> calAttempts;
    nlohmann::json history = nlohmann::json::array(); // earlier calibration rounds and hunts
    std::optional<Gen2::CalOutcome> calResult;
    Gen2::Session session;
    std::optional<Gen2::HuntSetup> setup;
    std::map<std::string, std::vector<Gen2::Hit>> fsDone;
    bool fsStopped = false;
    std::optional<Gen2::Hit> plan1, plan2;
    std::vector<Gen2::HuntAttempt> huntAttempts;
    Gen2::GridCache calCache, switchCache;
    std::array<std::vector<Gen2::TimerRow>, 3> phases; // calibration plan, plan 1, plan 2

    // ---- worker
    QThread *worker = nullptr;
    std::shared_ptr<std::atomic<bool>> cancelFlag;
    std::shared_ptr<std::atomic<int>> progressDone, progressTotal;
    QString workName;

    // ---- widgets
    QTabWidget *tabs;
    QLabel *status;
    QProgressBar *progress;
    QPushButton *cancel;

    // setup
    QLabel *filesLabel;
    QComboBox *textSpeed, *sound, *accuracyBox;
    QSpinBox *frame, *timerMsBox;
    QCheckBox *raikou, *entei, *lass, *swarm;
    QLineEdit *playTimeEdit;
    QLabel *playTimeWarn;
    QPushButton *start;
    QLabel *sessionLabel;

    // calibrate
    QTableWidget *calPhases;
    QTableWidget *calTable;
    QSpinBox *calStats[6];
    QLineEdit *calTime;
    QComboBox *calReset;
    QPushButton *calAdd, *calRemove, *calRun, *calNewRound;
    QTextBrowser *calText;

    // plans
    QTextBrowser *planSetup;
    QPushButton *planRun;
    QTableWidget *planPhases[2];
    QLabel *planLabel[2];
    QTextBrowser *planText;

    // hunt
    QComboBox *huntPlan;
    QSpinBox *huntStats[6];
    QSpinBox *huntMoved;
    QPushButton *huntAdd, *huntRemove, *huntResave, *huntRecal;
    QTableWidget *huntTable;
    QTextBrowser *huntText;

    // ---- building
    QWidget *setupTab();
    QWidget *calibrateTab();
    QWidget *plansTab();
    QWidget *huntTab();
    QWidget *helpTab();
    QWidget *phaseButtons(int which);

    // ---- actions
    void loadFiles(const QString &kind);
    bool readFiles(const QString &kind, const QStringList &paths, QString &error);
    void readOptions();
    void startSession();
    void addCalAttempt();
    void removeCalAttempt();
    void runCalibration();
    void newCalRound();
    void runPlanSearch();
    void runSwitchGrids();
    void addHuntAttempt();
    void removeHuntAttempt();
    void resave();
    void recalibrate();
    void sendPhases(int which, bool copyOnly);
    void exportSession();
    void openSession();
    void newSession();

    // ---- helpers
    void run(const QString &name, std::function<void(const Gen2::Progress &, const Gen2::Cancel *)> work, std::function<void()> done);
    bool busy() const;
    void refresh();
    void refreshCal();
    void refreshPlans();
    void refreshHunt();
    std::vector<Gen2::TimerRow> phasesOf(int which) const; // 0 calibration plan, 1 plan 1, 2 plan 2
    void updatePhases();
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json &j);
    void save();
    bool load(const QString &path);
    QString basePath() const;
    void clearAfterOptions();
};

#endif // CELEBI2_HPP
