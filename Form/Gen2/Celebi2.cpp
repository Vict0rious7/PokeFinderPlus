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
 * PokeFinderPlus: new file. See Celebi2.hpp. The flow and texts follow the prototype's CLI (celebi_tool.py v0.6).
 */

#include "Celebi2.hpp"
#include <Core/Gen2/VcInputs.hpp>
#include <Core/Gen6/XYTid.hpp>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocalSocket>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <fstream>

using json = nlohmann::json;
using namespace Gen2;

namespace
{
    constexpr const char *STAT_NAMES[6] = { "HP", "Atk", "Def", "SpA", "SpD", "Spe" };
    constexpr int FILE_VERSION = 1;

    Bytes readAll(const QString &path, bool *ok)
    {
        QFile f(path);
        *ok = f.open(QIODevice::ReadOnly);
        if (!*ok)
        {
            return {};
        }
        QByteArray b = f.readAll();
        return Bytes(b.begin(), b.end());
    }

    QString esc(const QString &s)
    {
        return s.toHtmlEscaped();
    }

    QString signedMs(int ms)
    {
        return (ms >= 0 ? "+" : "") + QString::number(ms) + " ms";
    }

    QString statsText(const std::array<int, 6> &s)
    {
        QStringList l;
        for (int v : s)
        {
            l << QString::number(v);
        }
        return l.join(" / ");
    }

    QString dvText(std::uint16_t dv)
    {
        return QString::fromStdString(dvHex(dv));
    }

    QString setText(const std::vector<int> &v)
    {
        return QString::number(v.size()) + "/60";
    }

    QTableWidget *phaseTable()
    {
        auto *t = new QTableWidget(0, 4);
        t->setHorizontalHeaderLabels({ QObject::tr("Press"), QObject::tr("Phase (frames)"), QObject::tr("Beep at frame"),
                                       QObject::tr("GB frame read") });
        t->verticalHeader()->setVisible(false);
        t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setMinimumHeight(180);
        return t;
    }

    void fillPhases(QTableWidget *t, const std::vector<TimerRow> &rows)
    {
        t->setRowCount(static_cast<int>(rows.size()));
        for (int i = 0; i < static_cast<int>(rows.size()); i++)
        {
            const auto &r = rows[i];
            QString press = QString::fromStdString(r.press);
            if (press == "info")
            {
                press = QObject::tr("Info box A (hold through the map load)");
            }
            QStringList cells = { press, QString::number(r.phase), QString::number(r.beep),
                                  QString::number(r.gbFrame) };
            for (int c = 0; c < 4; c++)
            {
                t->setItem(i, c, new QTableWidgetItem(cells[c]));
            }
        }
    }

    QSpinBox *statBox()
    {
        auto *s = new QSpinBox;
        s->setRange(0, 999);
        s->setMinimumWidth(60);
        return s;
    }
}

Celebi2::Celebi2(QWidget *parent) :
    QWidget(parent),
    progressDone(std::make_shared<std::atomic<int>>(0)),
    progressTotal(std::make_shared<std::atomic<int>>(0))
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Gen 2: shiny Celebi (Crystal, 3DS Virtual Console)"));
    resize(980, 760);

    auto *layout = new QVBoxLayout(this);
    tabs = new QTabWidget;
    tabs->addTab(setupTab(), tr("1. Setup"));
    tabs->addTab(calibrateTab(), tr("2. Calibrate"));
    tabs->addTab(plansTab(), tr("3. Plans"));
    tabs->addTab(huntTab(), tr("4. Hunt"));
    tabs->addTab(helpTab(), tr("Help"));
    layout->addWidget(tabs);

    auto *bar = new QHBoxLayout;
    status = new QLabel;
    status->setWordWrap(true);
    progress = new QProgressBar;
    progress->setMaximumWidth(260);
    progress->setVisible(false);
    cancel = new QPushButton(tr("Stop"));
    cancel->setVisible(false);
    connect(cancel, &QPushButton::clicked, this, [this] {
        if (cancelFlag)
        {
            cancelFlag->store(true);
            status->setText(tr("Stopping after the jobs in progress (a few seconds); finished work is kept."));
        }
    });
    bar->addWidget(status, 1);
    bar->addWidget(progress);
    bar->addWidget(cancel);
    layout->addLayout(bar);

    auto *timer = new QTimer(this);
    auto *clock = new QElapsedTimer;
    connect(timer, &QTimer::timeout, this, [this, clock] {
        if (!worker)
        {
            clock->invalidate();
            return;
        }
        if (!clock->isValid())
        {
            clock->start();
        }
        int d = progressDone->load(), t = progressTotal->load();
        progress->setMaximum(std::max(1, t));
        progress->setValue(d);
        QString eta;
        static int d0 = 0;
        if (d == 0 || clock->elapsed() < 1000)
        {
            d0 = d;
        }
        else if (d > d0 && t > d)
        {
            double rate = (d - d0) / (clock->elapsed() / 1000.0);
            int left = static_cast<int>((t - d) / std::max(rate, 1e-6));
            eta = tr(", about %1 min left").arg(std::max(1, (left + 59) / 60));
        }
        if (!cancelFlag || !cancelFlag->load())
        {
            status->setText(tr("%1: %2 / %3%4").arg(workName).arg(d).arg(t).arg(eta));
        }
    });
    timer->start(500);

    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Gen2");
    QString last = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Gen2/celebi-session.json";
    if (QFileInfo::exists(last))
    {
        load(last);
    }
    else
    {
        sessionPath = last;
    }
    refresh();
}

Celebi2::~Celebi2()
{
    if (worker)
    {
        cancelFlag->store(true);
        worker->wait();
    }
}

void Celebi2::closeEvent(QCloseEvent *event)
{
    if (worker)
    {
        cancelFlag->store(true);
        status->setText(tr("Stopping…"));
        worker->wait();
    }
    save();
    event->accept();
}

// ====================================================================== tabs

QWidget *Celebi2::setupTab()
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);

    auto *intro = new QLabel(
        tr("Shiny GS Ball Celebi at the Ilex Forest shrine, in the 3DS Virtual Console release of Pokémon Crystal (FR, DE, IT, ES, EN). "
           "You need a save standing on the tile below the shrine, facing it, walking, with the GS Ball in the bag. Save once there "
           "and don't save again unless the tool tells you to. Read the Help tab first."));
    intro->setWordWrap(true);
    v->addWidget(intro);

    auto *filesBox = new QGroupBox(tr("Game files (read on this computer only, never copied)"));
    auto *fl = new QVBoxLayout(filesBox);
    auto *fb = new QHBoxLayout;
    auto *romButton = new QPushButton(tr("ROM: .gbc (Recommended)…"));
    auto *cia = new QPushButton(tr("Decrypted CIA…"));
    romButton->setDefault(true);
    romButton->setFocus();
    connect(romButton, &QPushButton::clicked, this, [this] { loadFiles("rom"); });
    connect(cia, &QPushButton::clicked, this, [this] { loadFiles("cia"); });
    fb->addWidget(romButton);
    fb->addWidget(cia);
    fb->addStretch();
    fl->addLayout(fb);
    filesLabel = new QLabel(tr("No game files yet."));
    filesLabel->setWordWrap(true);
    filesLabel->setTextFormat(Qt::RichText);
    fl->addWidget(filesLabel);
    auto *langRow = new QFormLayout;
    languageBox = new QComboBox;
    languageBox->addItem(tr("Choose…"), QString());
    for (const char *code : { "fr", "de", "it", "es", "en" })
    {
        languageBox->addItem(QString::fromStdString(languageName(code)), QString::fromLatin1(code));
    }
    connect(languageBox, &QComboBox::currentIndexChanged, this, [this] {
        chosenLang = languageBox->currentData().toString();
        refresh();
    });
    langRow->addRow(tr("Language of your title"), languageBox);
    fl->addLayout(langRow);
    v->addWidget(filesBox);

    auto *opts = new QGroupBox(tr("What you read in your game"));
    auto *form = new QFormLayout(opts);
    textSpeed = new QComboBox;
    textSpeed->addItem(tr("Fast"), "fast");
    textSpeed->addItem(tr("Mid"), "mid");
    textSpeed->addItem(tr("Slow"), "slow");
    textSpeed->addItem(tr("Instant (text delay 0, save editors only)"), "instant");
    sound = new QComboBox;
    sound->addItem(tr("Mono"), "mono");
    sound->addItem(tr("Stereo"), "stereo");
    frame = new QSpinBox;
    frame->setRange(1, 8);
    raikou = new QCheckBox(tr("Raikou is still roaming"));
    entei = new QCheckBox(tr("Entei is still roaming"));
    swarm = new QCheckBox(tr("A swarm today (Dunsparce or Yanma phone call)"));
    lass = new QCheckBox(tr("The Lass stands left of the shrine"));
    lass->setChecked(true);
    playTimeEdit = new QLineEdit;
    playTimeEdit->setPlaceholderText(tr("h:mm, e.g. 42:17"));
    playTimeWarn = new QLabel;
    playTimeWarn->setWordWrap(true);
    connect(playTimeEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        playTimeWarn->setText(t.trimmed().endsWith(":59")
                                  ? tr("Play time minutes are 59: the hour rolls over during the run. Play one more minute and "
                                       "save again before you start.")
                                  : QString());
    });
    form->addRow(tr("Text speed (Options)"), textSpeed);
    form->addRow(tr("Sound (Options)"), sound);
    form->addRow(tr("Frame (Options)"), frame);
    form->addRow(raikou);
    form->addRow(entei);
    form->addRow(swarm);
    form->addRow(lass);
    form->addRow(tr("Play time (CONTINUE box)"), playTimeEdit);
    form->addRow(playTimeWarn);
    v->addWidget(opts);

    auto *you = new QGroupBox(tr("You and your timer"));
    auto *yf = new QFormLayout(you);
    accuracyBox = new QComboBox;
    accuracyBox->addItem(tr("±3 frames (default)"), 3);
    accuracyBox->addItem(tr("±2 frames (seasoned players)"), 2);
    timerMsBox = new QSpinBox;
    timerMsBox->setRange(-5000, 20000);
    timerMsBox->setSuffix(" ms");
    timerMsBox->setValue(0);
    yf->addRow(tr("Press accuracy"), accuracyBox);
    yf->addRow(tr("PokeTimerPlus calibration (Reset confirm → first frame)"), timerMsBox);
    v->addWidget(you);

    auto *sb = new QHBoxLayout;
    start = new QPushButton(tr("Start: prepare the calibration plan"));
    connect(start, &QPushButton::clicked, this, &Celebi2::startSession);
    auto *openB = new QPushButton(tr("Open session…"));
    auto *newB = new QPushButton(tr("New session"));
    auto *exportB = new QPushButton(tr("Export for a report…"));
    connect(openB, &QPushButton::clicked, this, &Celebi2::openSession);
    connect(newB, &QPushButton::clicked, this, &Celebi2::newSession);
    connect(exportB, &QPushButton::clicked, this, &Celebi2::exportSession);
    sb->addWidget(start);
    sb->addStretch();
    sb->addWidget(openB);
    sb->addWidget(newB);
    sb->addWidget(exportB);
    v->addLayout(sb);
    sessionLabel = new QLabel;
    sessionLabel->setWordWrap(true);
    sessionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    v->addWidget(sessionLabel);
    v->addStretch();
    return w;
}

QWidget *Celebi2::phaseButtons(int which)
{
    auto *w = new QWidget;
    auto *h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    auto *send = new QPushButton(tr("Send to PokeTimerPlus"));
    auto *copy = new QPushButton(tr("Copy"));
    connect(send, &QPushButton::clicked, this, [this, which] { sendPhases(which, false); });
    connect(copy, &QPushButton::clicked, this, [this, which] { sendPhases(which, true); });
    h->addWidget(send);
    h->addWidget(copy);
    h->addStretch();
    return w;
}

QWidget *Celebi2::calibrateTab()
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);
    auto *howto = new QLabel(
        tr("Calibration finds your timer offset and two hidden save values. Play the calibration plan below (it is not a shiny "
           "plan): catch Celebi, note its six stats at level 30 and the Pokégear time read just before you reset, then reset "
           "without saving. Keep the same in-game hour for all attempts, and reset about 20 s after the Pokégear minute changes "
           "(rule A) unless the tool says otherwise. At least 6 attempts."));
    howto->setWordWrap(true);
    v->addWidget(howto);
    calPhases = phaseTable();
    v->addWidget(calPhases);
    v->addWidget(phaseButtons(0));

    auto *entry = new QGroupBox(tr("Add an attempt"));
    auto *g = new QGridLayout(entry);
    for (int i = 0; i < 6; i++)
    {
        g->addWidget(new QLabel(STAT_NAMES[i]), 0, i);
        calStats[i] = statBox();
        g->addWidget(calStats[i], 1, i);
    }
    g->addWidget(new QLabel(tr("Pokégear time")), 0, 6);
    calTime = new QLineEdit("20:30");
    calTime->setMaximumWidth(80);
    g->addWidget(calTime, 1, 6);
    g->addWidget(new QLabel(tr("Reset rule")), 0, 7);
    calReset = new QComboBox;
    calReset->addItems({ "A", "B" });
    g->addWidget(calReset, 1, 7);
    calAdd = new QPushButton(tr("Add"));
    connect(calAdd, &QPushButton::clicked, this, &Celebi2::addCalAttempt);
    g->addWidget(calAdd, 1, 8);
    v->addWidget(entry);

    calTable = new QTableWidget(0, 4);
    calTable->setHorizontalHeaderLabels({ tr("Stats (HP / Atk / Def / SpA / SpD / Spe)"), tr("Pokégear time"), tr("Reset"), tr("Note") });
    calTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    calTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    calTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    v->addWidget(calTable);

    auto *b = new QHBoxLayout;
    calRemove = new QPushButton(tr("Remove selected"));
    calRun = new QPushButton(tr("Calibrate"));
    calNewRound = new QPushButton(tr("New round (timer +5 frames)"));
    connect(calRemove, &QPushButton::clicked, this, &Celebi2::removeCalAttempt);
    connect(calRun, &QPushButton::clicked, this, &Celebi2::runCalibration);
    connect(calNewRound, &QPushButton::clicked, this, &Celebi2::newCalRound);
    b->addWidget(calRemove);
    b->addStretch();
    b->addWidget(calNewRound);
    b->addWidget(calRun);
    v->addLayout(b);
    calText = new QTextBrowser;
    v->addWidget(calText);
    return w;
}

QWidget *Celebi2::plansTab()
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);
    planSetup = new QTextBrowser;
    planSetup->setMaximumHeight(140);
    v->addWidget(planSetup);
    auto *b = new QHBoxLayout;
    planRun = new QPushButton(tr("Find plans 1 and 2"));
    connect(planRun, &QPushButton::clicked, this, &Celebi2::runPlanSearch);
    b->addWidget(planRun);
    b->addStretch();
    v->addLayout(b);
    auto *grid = new QHBoxLayout;
    for (int i = 0; i < 2; i++)
    {
        auto *box = new QGroupBox(tr("Plan %1").arg(i + 1));
        auto *bl = new QVBoxLayout(box);
        planLabel[i] = new QLabel;
        planLabel[i]->setWordWrap(true);
        bl->addWidget(planLabel[i]);
        planPhases[i] = phaseTable();
        bl->addWidget(planPhases[i]);
        bl->addWidget(phaseButtons(i + 1));
        grid->addWidget(box);
    }
    v->addLayout(grid);
    planText = new QTextBrowser;
    v->addWidget(planText);
    return w;
}

QWidget *Celebi2::huntTab()
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);
    auto *howto = new QLabel(tr("Hunt on plan 1 at the in-game time shown in Plans. After each attempt that isn't shiny, add its stats "
                                "here: the tool says when to move your timer, switch to plan 2, re-save or recalibrate."));
    howto->setWordWrap(true);
    v->addWidget(howto);
    auto *entry = new QGroupBox(tr("Add an attempt"));
    auto *g = new QGridLayout(entry);
    g->addWidget(new QLabel(tr("Plan")), 0, 0);
    huntPlan = new QComboBox;
    huntPlan->addItems({ "1", "2" });
    g->addWidget(huntPlan, 1, 0);
    for (int i = 0; i < 6; i++)
    {
        g->addWidget(new QLabel(STAT_NAMES[i]), 0, i + 1);
        huntStats[i] = statBox();
        g->addWidget(huntStats[i], 1, i + 1);
    }
    g->addWidget(new QLabel(tr("Timer moved (frames)")), 0, 7);
    huntMoved = new QSpinBox;
    huntMoved->setRange(-20, 20);
    g->addWidget(huntMoved, 1, 7);
    huntAdd = new QPushButton(tr("Add"));
    connect(huntAdd, &QPushButton::clicked, this, &Celebi2::addHuntAttempt);
    g->addWidget(huntAdd, 1, 8);
    v->addWidget(entry);
    huntTable = new QTableWidget(0, 4);
    huntTable->setHorizontalHeaderLabels({ tr("Plan"), tr("Stats"), tr("Timer moved"), tr("Note") });
    huntTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    huntTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    huntTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    v->addWidget(huntTable);
    auto *b = new QHBoxLayout;
    huntRemove = new QPushButton(tr("Remove selected"));
    huntResave = new QPushButton(tr("I re-saved"));
    huntRecal = new QPushButton(tr("Recalibrate"));
    connect(huntRemove, &QPushButton::clicked, this, &Celebi2::removeHuntAttempt);
    connect(huntResave, &QPushButton::clicked, this, &Celebi2::resave);
    connect(huntRecal, &QPushButton::clicked, this, &Celebi2::recalibrate);
    b->addWidget(huntRemove);
    b->addStretch();
    b->addWidget(huntRecal);
    b->addWidget(huntResave);
    v->addLayout(b);
    huntText = new QTextBrowser;
    v->addWidget(huntText);
    return w;
}

QWidget *Celebi2::helpTab()
{
    auto *t = new QTextBrowser;
    t->setOpenExternalLinks(true);
    t->setHtml(tr(R"(
<h3>What you need</h3>
<ul>
<li>Pokémon Crystal on the 3DS Virtual Console, FR, DE, IT, ES or EN (JP is not supported).</li>
<li><b>Your ROM</b> (.gbc), read on this computer only: a dump of Pokémon Crystal in your title's language. The tool recognises
it and shows a check mark (✓ French, ✓ English Rev A, …). The ROM alone is enough: the Virtual Console patch never changes Celebi.
Or a <b>decrypted CIA</b> of your VC title: the tool takes the ROM from inside it and also checks the emulator build.</li>
<li><b>Only the VC title, no .gbc?</b> The ROM is inside the title's romfs, as <code>rom/&lt;code&gt;</code> (a file with no
extension, for example <code>rom/CGBBYTF0.849</code> for French). With GodMode9 on your 3DS: open <i>[A:] SYSNAND SD</i> →
<code>title</code> → <code>00040000</code> → your Crystal title's folder → <code>content</code>, press A on the <code>.app</code>
file → <i>NCCH image options…</i> → <i>Mount image to drive</i>; open <code>romfs</code> → <code>rom</code>, press A on the
file → <i>Copy to 0:/gm9/out</i>. On your computer, take that file from the SD card's <code>gm9/out</code> folder and choose it
with <i>ROM: .gbc</i> (pick "All files" in the dialog, or rename it to .gbc). Or build a decrypted CIA in GodMode9 (press A on
the title → <i>Build CIA (decrypted)</i>) and choose it with <i>Decrypted CIA</i>.</li>
<li>PokeTimerPlus (3DS → Crystal VC Celebi), with your calibration in ms: the time from the Reset confirm to the game's first frame.</li>
<li>A save on the tile below the shrine, facing it, walking (not on the bike), with the GS Ball in the bag (Kurt has given it back).
Save once there. Don't save again during the hunt unless the tool tells you to re-save.</li>
</ul>
<h3>One attempt</h3>
<ol>
<li>Start the timer on the VC Reset confirm (HOME menu → Reset).</li>
<li><b>CONTINUE</b> on the first beep; it has a window, the other presses are frame-exact.</li>
<li><b>Info box A</b> on the next beep; hold it through the map load.</li>
<li><b>Hold B</b> from right after the info box until the final A (it may start up to about 0.9 s after A is released).</li>
<li>Text prompts 1 to 5: tap A firmly (spamming is fine) but <b>stop after the 5th box closes</b>.</li>
<li><b>6th prompt</b>, <b>YES</b> and the <b>final A</b>: each on its beep.</li>
<li>Catch Celebi. Note its six stats (summary screen, level 30) and the Pokégear time, then reset without saving.</li>
</ol>
<h3>The steps</h3>
<ol>
<li><b>Setup</b>: your files, the options you read in the game, your timer calibration.</li>
<li><b>Calibrate</b>: 6 attempts on the calibration plan; add 2 at a time if asked; after 10 without a result, a new round with
the timer moved +5 frames (+84 ms). Reset about 20 s after the Pokégear minute changes (rule A) unless told otherwise.</li>
<li><b>Plans</b>: the tool searches plans 1 and 2 for your save (a few minutes to about 20), then the hunt-advice grids (about 20 min
on 8 threads; start hunting plan 1 meanwhile). Set your new timer calibration as shown.</li>
<li><b>Hunt</b>: enter each attempt; follow the advice (move your timer, switch to plan 2, re-save, recalibrate).</li>
</ol>
<h3>Limits (read before you start)</h3>
<ul>
<li>A player who is always late (or always early) by the same amount on one press is not detected; the tool may then move your
timer or ask you to recalibrate. If you are told to recalibrate twice, check whether you press late on the prompts.</li>
<li>For a few saves (3 of 60 hidden clock-seconds values), about 6–8% of attempts fall in another clock class whatever you do;
the tool still plans for the likely one.</li>
<li>After a re-save, hunt on a later in-game day (Pokégear weekday) than the re-save.</li>
<li>Expected effort: median about 25–30 attempts in total (calibration included), 90% within 57–74.</li>
<li>Steps that compute grids take minutes (about 17 min for a calibration or the hunt grids on 8 threads).</li>
</ul>
<h3>Report a problem</h3>
<p>Setup → <i>Export for a report</i> writes your session (options, attempts, results; no game files and no file paths). Attach it
to an issue on the PokeFinderPlus GitHub page.</p>
)"));
    return t;
}

// ====================================================================== worker

bool Celebi2::busy() const
{
    return worker != nullptr;
}

void Celebi2::run(const QString &name, std::function<void(const Progress &, const Cancel *)> work, std::function<void()> done)
{
    if (busy())
    {
        return;
    }
    cancelFlag = std::make_shared<std::atomic<bool>>(false);
    progressDone->store(0);
    progressTotal->store(0);
    workName = name;
    auto c = cancelFlag;
    auto pd = progressDone, pt = progressTotal;
    worker = QThread::create([work, c, pd, pt] {
        work(
            [pd, pt](int d, int t) {
                pd->store(d);
                pt->store(t);
            },
            c.get());
    });
    connect(worker, &QThread::finished, this, [this, done] {
        bool stopped = cancelFlag && cancelFlag->load();
        worker->deleteLater();
        worker = nullptr;
        done();
        if (stopped)
        {
            status->setText(tr("%1 stopped; finished work is kept. Run it again to continue.").arg(workName));
        }
        save();
        refresh();
    });
    worker->start();
    refresh();
}

// ====================================================================== setup

void Celebi2::loadFiles(const QString &kind)
{
    if (busy())
    {
        return;
    }
    QString path = kind == "cia"
        ? QFileDialog::getOpenFileName(this, tr("Decrypted CIA of your Crystal VC title"), QString(), "CIA (*.cia);;All (*)")
        : QFileDialog::getOpenFileName(this, tr("Your Pokémon Crystal ROM"), QString(), tr("GBC ROM (*.gbc *.gb);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }
    QString error;
    if (!readFiles(kind, { path }, error))
    {
        QMessageBox::warning(this, tr("Game files"), error);
        return;
    }
    if (files && files->known && chosenLang.isEmpty())
    {
        languageBox->setCurrentIndex(languageBox->findData(QString::fromStdString(files->lang()))); // sets chosenLang
    }
    if ((calResult || !calAttempts.empty()) && files && plan && files->lang() != plan->lang)
    {
        QMessageBox::information(this, tr("Game files"), tr("This ROM is another language than this session: start a new session for it."));
    }
    save();
    refresh();
}

bool Celebi2::readFiles(const QString &kind, const QStringList &paths, QString &error)
{
    bool ok = true;
    Bytes b = readAll(paths.value(0), &ok);
    if (!ok)
    {
        error = tr("Can't read %1.").arg(paths.value(0));
        return false;
    }
    if (kind == "cia")
    {
        auto r = loadCia(b);
        if (!r.first)
        {
            error = QString::fromStdString(r.second);
            return false;
        }
        files = r.first;
    }
    else
    {
        files = loadRom(b);
    }
    filesKind = kind;
    filePaths = { paths.value(0) };
    return true;
}

QString Celebi2::filesProblem() const
{
    if (!files)
    {
        return tr("Choose your ROM first.");
    }
    if (!files->known)
    {
        return tr("This isn't a Pokémon Crystal ROM the tool knows for a supported language (French, German, Italian, Spanish, "
                  "English Rev 0 or Rev A). Check that you picked your title's ROM; the Japanese version isn't supported.");
    }
    if (chosenLang.isEmpty())
    {
        return tr("Choose the language of your title.");
    }
    if (QString::fromStdString(files->lang()) != chosenLang)
    {
        return tr("Your ROM is %1 but the language is set to %2: use the ROM of your title's language, or fix the setting.")
            .arg(QString::fromStdString(files->known->language), QString::fromStdString(languageName(chosenLang.toStdString())));
    }
    return {};
}

void Celebi2::readOptions()
{
    visible.textSpeed = textSpeed->currentData().toString() == "instant" ? TextSpeed::Instant
        : textSpeed->currentData().toString() == "mid"                   ? TextSpeed::Mid
        : textSpeed->currentData().toString() == "slow"                  ? TextSpeed::Slow
                                                                         : TextSpeed::Fast;
    visible.stereo = sound->currentData().toString() == "stereo";
    visible.frame = frame->value();
    visible.raikou = raikou->isChecked();
    visible.entei = entei->isChecked();
    visible.swarm = swarm->isChecked();
    visible.lass = lass->isChecked();
    accuracy = accuracyBox->currentData().toInt();
    timerMs = timerMsBox->value();
    playTime = playTimeEdit->text().trimmed();
}

void Celebi2::clearAfterOptions()
{
    plan.reset();
    calResult.reset();
    setup.reset();
    fsDone.clear();
    fsStopped = false;
    plan1.reset();
    plan2.reset();
    calCache.clear();
    switchCache.clear();
}

void Celebi2::startSession()
{
    if (busy())
    {
        return;
    }
    if (QString why = filesProblem(); !why.isEmpty())
    {
        QMessageBox::information(this, tr("Setup"), why);
        return;
    }
    Visible before = visible;
    int accBefore = accuracy;
    readOptions();
    if (playTime.endsWith(":59"))
    {
        QMessageBox::information(this, tr("Setup"), playTimeWarn->text());
        return;
    }
    bool changed = !(before == visible) || accBefore != accuracy;
    if (changed && (calResult || !calAttempts.empty()))
    {
        if (QMessageBox::question(this, tr("Setup"),
                                  tr("Your options changed after calibration attempts were made. The grids and results for the old "
                                     "options will be cleared (attempts are kept). Continue?"))
            != QMessageBox::Yes)
        {
            return;
        }
    }
    if (changed)
    {
        clearAfterOptions();
    }
    RomPtr rom = files->rom;
    Visible vis = visible;
    std::string lang = files->lang();
    auto result = std::make_shared<std::optional<Plan>>();
    run(
        tr("Finding the presses for your game"),
        [rom, vis, lang, result](const Progress &, const Cancel *) {
            Plan p = discover(rom, toolSave(vis, 2), chipRtc(hm(PLAN_INGAME), 0));
            p.lang = lang;
            *result = p;
        },
        [this, result] {
            plan = *result;
            updatePhases();
            session.lang = files->lang();
            session.accuracy = accuracy;
            session.timerMs = timerMs;
            tabs->setCurrentIndex(calResult && calResult->state == CalState::Accepted ? 2 : 1);
        });
}

// ====================================================================== calibration

void Celebi2::addCalAttempt()
{
    Attempt a;
    for (int i = 0; i < 6; i++)
    {
        a.stats[i] = calStats[i]->value();
    }
    QRegularExpression re("^([01]?\\d|2[0-3]):([0-5]\\d)$");
    auto m = re.match(calTime->text().trimmed());
    if (!m.hasMatch())
    {
        QMessageBox::information(this, tr("Calibration"), tr("Enter the Pokégear time as hh:mm (24 h)."));
        return;
    }
    a.ingame = QString("%1:%2").arg(m.captured(1).toInt(), 2, 10, QChar('0')).arg(m.captured(2)).toStdString();
    a.reset = calReset->currentText().at(0).toLatin1();
    if (impossibleStats(a.stats))
    {
        QMessageBox::information(this, tr("Calibration"),
                                 tr("No level 30 Celebi has these stats: check them (HP / Atk / Def / SpA / SpD / Spe)."));
        return;
    }
    calAttempts.push_back(a);
    for (auto *s : calStats)
    {
        s->setValue(0);
    }
    save();
    refresh();
}

void Celebi2::removeCalAttempt()
{
    auto rows = calTable->selectionModel()->selectedRows();
    std::vector<int> r;
    for (const auto &i : rows)
    {
        r.push_back(i.row());
    }
    std::sort(r.rbegin(), r.rend());
    for (int i : r)
    {
        if (i < static_cast<int>(calAttempts.size()))
        {
            calAttempts.erase(calAttempts.begin() + i);
        }
    }
    calResult.reset();
    save();
    refresh();
}

void Celebi2::runCalibration()
{
    if (busy() || !plan || !files)
    {
        return;
    }
    if (static_cast<int>(calAttempts.size()) < MIN_ATT)
    {
        QMessageBox::information(this, tr("Calibration"), tr("At least %1 attempts.").arg(MIN_ATT));
        return;
    }
    RomPtr rom = files->rom;
    Plan p = *plan;
    Visible vis = visible;
    auto att = calAttempts;
    int acc = accuracy;
    int tms = timerMs;
    auto cache = std::make_shared<GridCache>(calCache);
    auto result = std::make_shared<CalOutcome>();
    run(
        tr("Calibration grids"),
        [=](const Progress &pr, const Cancel *c) { *result = calibrate(rom, p, vis, att, acc, tms, *cache, 0, pr, c); },
        [this, cache, result] {
            calCache = std::move(*cache);
            saveGrids((basePath() + ".cal.grids").toStdString(), calCache);
            if (!result->complete)
            {
                return;
            }
            calResult = *result;
            const auto &r = *result;
            if (r.state == CalState::Accepted && !(r.band && r.band->more))
            {
                sessionAfterCalibration(session, r.verdict, r.band, static_cast<int>(calAttempts.size()), timerMs, accuracy, files->lang());
                int k = r.band && r.band->kPlan ? *r.band->kPlan : (r.verdict.best.carries.empty() ? 0 : r.verdict.best.carries[0]);
                HuntSetup h = huntSetup(visible, r.verdict.best.kenji, k, r.hour, r.minute);
                if (!setup || setup->kenji != h.kenji || setup->k != h.k || setup->hour != h.hour)
                {
                    fsDone.clear();
                    fsStopped = false;
                    plan1.reset();
                    plan2.reset();
                    switchCache.clear();
                }
                setup = h;
                updatePhases();
            }
        });
}

void Celebi2::newCalRound()
{
    if (busy())
    {
        return;
    }
    int ms = framesToMs(SHIFT);
    if (QMessageBox::question(this, tr("New calibration round"),
                              tr("Move your PokeTimerPlus calibration by %1 (to %2 ms) and make %3 new attempts. The attempts so far are "
                                 "kept in the session history. Continue?")
                                  .arg(signedMs(ms))
                                  .arg(timerMs + ms)
                                  .arg(MIN_ATT))
        != QMessageBox::Yes)
    {
        return;
    }
    json round = json::array();
    for (const auto &a : calAttempts)
    {
        round.push_back(Gen2::toJson(a));
    }
    history.push_back({ { "event", "calibration round" }, { "timer_ms", timerMs }, { "attempts", round } });
    calAttempts.clear();
    calResult.reset();
    timerMs += ms;
    timerMsBox->setValue(timerMs);
    session.timerMs = timerMs;
    save();
    refresh();
}

// ====================================================================== plans

void Celebi2::runPlanSearch()
{
    if (busy() || !plan || !setup || !files)
    {
        return;
    }
    if (fsStopped)
    {
        runSwitchGrids();
        return;
    }
    RomPtr rom = files->rom;
    Plan p = *plan;
    HuntSetup h = *setup;
    auto done = std::make_shared<std::map<std::string, std::vector<Hit>>>(fsDone);
    auto result = std::make_shared<FsearchResult>();
    run(
        tr("Searching plans 1 and 2"),
        [=](const Progress &pr, const Cancel *c) { *result = planSearch(rom, p, h, *done, 0, pr, c); },
        [this, result] {
            for (const auto &[k, v] : result->jobs)
            {
                fsDone[k] = v;
            }
            fsStopped = result->stopped || result->committed == result->total;
            auto hits = result->hits();
            auto [a, b] = pick(hits);
            plan1.reset();
            plan2.reset();
            if (a)
            {
                plan1 = hits[*a];
            }
            if (b)
            {
                plan2 = hits[*b];
            }
            updatePhases();
            if (fsStopped && plan1 && plan2)
            {
                QTimer::singleShot(0, this, &Celebi2::runSwitchGrids);
            }
        });
}

void Celebi2::runSwitchGrids()
{
    if (busy() || !plan || !setup || !plan1 || !plan2 || !files)
    {
        return;
    }
    RomPtr rom = files->rom;
    Plan p = *plan;
    HuntSetup h = *setup;
    Hit a = *plan1, b = *plan2;
    int acc = accuracy;
    auto cache = std::make_shared<GridCache>(switchCache);
    run(
        tr("Hunt-advice grids (you can hunt plan 1 meanwhile)"),
        [=](const Progress &pr, const Cancel *c) { switchGridsCached(rom, p, h, a, b, acc, *cache, 0, pr, c); },
        [this, cache] {
            switchCache = std::move(*cache);
            saveGrids((basePath() + ".switch.grids").toStdString(), switchCache);
        });
}

// ====================================================================== hunt

void Celebi2::addHuntAttempt()
{
    HuntAttempt a;
    a.plan = huntPlan->currentIndex() + 1;
    for (int i = 0; i < 6; i++)
    {
        a.stats[i] = huntStats[i]->value();
    }
    a.timerMoved = huntMoved->value();
    if (impossibleStats(a.stats))
    {
        QMessageBox::information(this, tr("Hunt"), tr("No level 30 Celebi has these stats: check them (HP / Atk / Def / SpA / SpD / Spe)."));
        return;
    }
    huntAttempts.push_back(a);
    for (auto *s : huntStats)
    {
        s->setValue(0);
    }
    if (shinyStats(a.stats))
    {
        QMessageBox::information(this, tr("Shiny!"),
                                 tr("These stats are a shiny Celebi's. Congratulations! Save the game now."));
    }
    save();
    refresh();
}

void Celebi2::removeHuntAttempt()
{
    auto rows = huntTable->selectionModel()->selectedRows();
    std::vector<int> r;
    for (const auto &i : rows)
    {
        r.push_back(i.row());
    }
    std::sort(r.rbegin(), r.rend());
    for (int i : r)
    {
        if (i < static_cast<int>(huntAttempts.size()))
        {
            huntAttempts.erase(huntAttempts.begin() + i);
        }
    }
    save();
    refresh();
}

void Celebi2::resave()
{
    if (busy())
    {
        return;
    }
    if (!session.kenjiClass && session.classHistory.empty())
    {
        QMessageBox::information(this, tr("Re-save"), tr("Calibrate first: the re-save step needs the Kenji class from calibration."));
        return;
    }
    if (QMessageBox::question(this, tr("Re-save"),
                              tr("Record a re-save? Do it only when the tool told you to re-save, and after you saved again at "
                                 "the shrine."))
        != QMessageBox::Yes)
    {
        return;
    }
    json hunt = json::array();
    for (const auto &a : huntAttempts)
    {
        hunt.push_back(Gen2::toJson(a));
    }
    history.push_back({ { "event", "hunt before re-save" }, { "attempts", hunt } });
    huntAttempts.clear();
    huntMoved->setValue(0);
    auto lines = resaveStep(session);
    // the tool's own next steps
    fsDone.clear();
    fsStopped = false;
    plan1.reset();
    plan2.reset();
    switchCache.clear();
    if (session.kenjiClass && setup)
    {
        // class 1 -> class >= 2: re-plan, no new attempts, timer unchanged
        setup = huntSetup(visible, *session.kenjiClass, setup->k, setup->hour, setup->minute);
        lines.back() = tr("Your save is now Kenji class ≥2: no new calibration attempts and no timer change. Go to Plans and click "
                          "\"Find plans 1 and 2\" again.")
                           .toStdString();
    }
    else
    {
        json round = json::array();
        for (const auto &a : calAttempts)
        {
            round.push_back(Gen2::toJson(a));
        }
        history.push_back({ { "event", "calibration round" }, { "timer_ms", timerMs }, { "attempts", round } });
        calAttempts.clear();
        calResult.reset();
        setup.reset();
        int ms = session.X ? framesToMs(*session.X) : 0;
        timerMs += ms;
        timerMsBox->setValue(timerMs);
        lines.back() = tr("Your save may now be Kenji class 1 (the tool can't see it): calibrate again with 6 new attempts. Your timer "
                          "calibration is now %1 ms (your current one); the plans are made again after calibration.")
                           .arg(timerMs)
                           .toStdString();
    }
    QStringList l;
    for (const auto &s : lines)
    {
        l << QString::fromStdString(s);
    }
    updatePhases();
    QMessageBox::information(this, tr("Re-save"), l.join("\n\n"));
    save();
    refresh();
    tabs->setCurrentIndex(session.needsCalibration ? 1 : 2);
}

void Celebi2::recalibrate()
{
    if (busy())
    {
        return;
    }
    if (QMessageBox::question(this, tr("Recalibrate"),
                              tr("Start a new calibration with fresh attempts (same save)? Your hunt attempts so far are kept in the "
                                 "session history."))
        != QMessageBox::Yes)
    {
        return;
    }
    json hunt = json::array();
    for (const auto &a : huntAttempts)
    {
        hunt.push_back(Gen2::toJson(a));
    }
    json round = json::array();
    for (const auto &a : calAttempts)
    {
        round.push_back(Gen2::toJson(a));
    }
    history.push_back({ { "event", "recalibrate" }, { "hunt", hunt }, { "calibration", round }, { "timer_ms", timerMs } });
    int ms = session.X ? framesToMs(*session.X) : 0;
    int moved = huntAttempts.empty() ? 0 : huntAttempts.back().timerMoved;
    timerMs += ms + framesToMs(moved);
    timerMsBox->setValue(timerMs);
    huntAttempts.clear();
    huntMoved->setValue(0);
    calAttempts.clear();
    calResult.reset();
    session.needsCalibration = true;
    session.log.push_back({ { "event", "recalibrate" } });
    save();
    refresh();
    tabs->setCurrentIndex(1);
}

// ====================================================================== PokeTimerPlus

void Celebi2::updatePhases()
{
    for (int i = 0; i < 3; i++)
    {
        phases[i] = phasesOf(i);
    }
}

std::vector<TimerRow> Celebi2::phasesOf(int which) const
{
    if (!plan || !files)
    {
        return {};
    }
    if (which == 0)
    {
        return timer(files->rom, toolSave(visible, 2), *plan, 0, 0, CAL_FINAL);
    }
    const auto &h = which == 1 ? plan1 : plan2;
    if (!h || !setup)
    {
        return {};
    }
    return timer(files->rom, setup->sram, *plan, h->d6, h->dyes, h->final);
}

void Celebi2::sendPhases(int which, bool copyOnly)
{
    const auto &rows = phases[which];
    if (rows.empty())
    {
        return;
    }
    int cal = timerMs;
    if (which > 0 && session.X && !session.needsCalibration)
    {
        int moved = huntAttempts.empty() ? huntMoved->value() : std::max(huntMoved->value(), huntAttempts.back().timerMoved);
        cal = timerMs + framesToMs(*session.X) + framesToMs(moved);
    }
    QStringList ph, phMs;
    for (const auto &r : rows)
    {
        ph << QString::number(r.phase);
        phMs << QString::number(std::lround(r.phase * 1000.0 / FPS));
    }
    QString name = which == 0 ? "calibration" : QString("plan%1").arg(which);
    QString line = QString("celebi plan=%1 lang=%2 calibration=%3 phases=%4").arg(name, QString::fromStdString(files->lang())).arg(cal).arg(ph.join(","));
    QString human = tr("Crystal VC Celebi, %1: phases %2 frames at 59.8312 fps = %3 ms; calibration %4 ms on the first phase "
                       "(timer starts on the Reset confirm)")
                        .arg(which == 0 ? tr("calibration plan") : tr("plan %1").arg(which), ph.join(" / "), phMs.join(" / "))
                        .arg(cal);
    if (copyOnly)
    {
        QApplication::clipboard()->setText(human);
        status->setText(tr("Copied: %1").arg(human));
        return;
    }
    QLocalSocket socket;
    socket.connectToServer(XYTid::SOCKET_NAME);
    QByteArray reply;
    if (socket.waitForConnected(1000))
    {
        socket.write((line + "\n").toUtf8());
        if (socket.waitForBytesWritten(1000) && socket.waitForReadyRead(3000))
        {
            reply = socket.readLine().trimmed();
        }
        socket.disconnectFromServer();
    }
    if (reply == "OK")
    {
        status->setText(tr("Sent to PokeTimerPlus: %1").arg(human));
        return;
    }
    QApplication::clipboard()->setText(human);
    QString why = reply == "BUSY" ? tr("PokeTimerPlus is running another timer.")
        : reply.isEmpty()         ? tr("PokeTimerPlus isn't open.")
                                  : tr("PokeTimerPlus refused the details (update it to the version with Crystal VC Celebi).");
    QMessageBox::information(this, tr("Not sent"), why + " " + tr("Copied to the clipboard instead."));
}

// ====================================================================== session file

QString Celebi2::basePath() const
{
    QString p = sessionPath;
    if (p.endsWith(".json"))
    {
        p.chop(5);
    }
    return p;
}

json Celebi2::toJson() const
{
    json j;
    j["app"] = "PokeFinderPlus Gen 2 Celebi";
    j["file_version"] = FILE_VERSION;
    j["visible"] = Gen2::toJson(visible);
    j["accuracy"] = accuracy;
    j["timer_ms"] = timerMs;
    j["play_time"] = playTime.toStdString();
    j["language"] = chosenLang.toStdString();
    if (files)
    {
        j["files"] = { { "kind", filesKind.toStdString() }, { "lang", files->lang() },
                       { "label", files->known ? files->known->label() : std::string("unknown ROM") },
                       { "same_emulator", files->sameEmulator } };
        json paths = json::array();
        for (const auto &p : filePaths)
        {
            paths.push_back(p.toStdString());
        }
        j["files"]["paths"] = paths;
    }
    json cal = json::array();
    for (const auto &a : calAttempts)
    {
        cal.push_back(Gen2::toJson(a));
    }
    j["calibration_attempts"] = cal;
    if (calResult)
    {
        const auto &r = *calResult;
        j["calibration_result"] = { { "state", static_cast<int>(r.state) }, { "X", r.verdict.best.X }, { "kenji", r.verdict.best.kenji },
                                    { "explained", r.verdict.best.explained }, { "hour", r.hour }, { "minute", r.minute } };
        if (r.band)
        {
            j["calibration_result"]["band"] = { { "rule", std::string(1, r.band->rule) }, { "p_band", r.band->pBand }, { "value", r.band->value } };
        }
    }
    j["session"] = session.toJson();
    if (setup)
    {
        j["setup"] = Gen2::toJson(*setup);
    }
    json fs = json::object();
    for (const auto &[k, v] : fsDone)
    {
        json hs = json::array();
        for (const auto &h : v)
        {
            hs.push_back(Gen2::toJson(h));
        }
        fs[k] = hs;
    }
    j["fsearch_jobs"] = fs;
    j["fsearch_done"] = fsStopped;
    json hunt = json::array();
    for (const auto &a : huntAttempts)
    {
        hunt.push_back(Gen2::toJson(a));
    }
    j["hunt_attempts"] = hunt;
    j["history"] = history;
    return j;
}

void Celebi2::fromJson(const json &j)
{
    visible = visibleFromJson(j.value("visible", json::object()));
    accuracy = j.value("accuracy", 3);
    timerMs = j.value("timer_ms", 0);
    playTime = QString::fromStdString(j.value("play_time", std::string()));
    chosenLang = QString::fromStdString(j.value("language", j.contains("files") ? j["files"].value("lang", std::string()) : std::string()));
    calAttempts.clear();
    for (const auto &a : j.value("calibration_attempts", json::array()))
    {
        calAttempts.push_back(attemptFromJson(a));
    }
    session = Session::fromJson(j.value("session", json::object()));
    fsDone.clear();
    for (const auto &[k, v] : j.value("fsearch_jobs", json::object()).items())
    {
        std::vector<Hit> hs;
        for (const auto &h : v)
        {
            hs.push_back(hitFromJson(h));
        }
        fsDone[k] = hs;
    }
    fsStopped = j.value("fsearch_done", false);
    huntAttempts.clear();
    for (const auto &a : j.value("hunt_attempts", json::array()))
    {
        huntAttempts.push_back(huntAttemptFromJson(a));
    }
    history = j.value("history", json::array());
    setup.reset();
    if (j.contains("setup"))
    {
        setup = huntSetupFromJson(visible, j["setup"]);
    }
    plan1.reset();
    plan2.reset();
    std::vector<Hit> hits;
    for (const auto &[k, v] : fsDone)
    {
        hits.insert(hits.end(), v.begin(), v.end());
    }
    auto [a, b] = pick(hits);
    if (a)
    {
        plan1 = hits[*a];
    }
    if (b)
    {
        plan2 = hits[*b];
    }
    files.reset();
    filePaths.clear();
    if (j.contains("files"))
    {
        filesKind = QString::fromStdString(j["files"].value("kind", std::string()));
        if (filesKind != "cia")
        {
            filesKind = "rom"; // sessions from 4.4.2-plus.2: "dump" or "rom+patch" (its first file is the ROM)
        }
        for (const auto &p : j["files"].value("paths", json::array()))
        {
            filePaths << QString::fromStdString(p.get<std::string>());
        }
        QString error;
        if (!readFiles(filesKind, filePaths, error))
        {
            status->setText(tr("Couldn't reopen the game files (%1): choose them again.").arg(error));
        }
    }
    // widgets
    textSpeed->setCurrentIndex(std::max(0, textSpeed->findData(QString::fromStdString(Gen2::toJson(visible)["textspeed"].get<std::string>()))));
    sound->setCurrentIndex(visible.stereo ? 1 : 0);
    frame->setValue(visible.frame);
    raikou->setChecked(visible.raikou);
    entei->setChecked(visible.entei);
    swarm->setChecked(visible.swarm);
    lass->setChecked(visible.lass);
    accuracyBox->setCurrentIndex(accuracy == 2 ? 1 : 0);
    {
        QString keep = chosenLang;
        languageBox->setCurrentIndex(std::max(0, languageBox->findData(keep)));
        chosenLang = keep;
    }
    timerMsBox->setValue(timerMs);
    playTimeEdit->setText(playTime);
    if (!huntAttempts.empty())
    {
        huntMoved->setValue(huntAttempts.back().timerMoved);
    }
    // calibration result: recompute from the cached grids when complete (fast)
    calResult.reset();
    plan.reset();
    if (files)
    {
        RomPtr rom = files->rom;
        Visible vis = visible;
        std::string lang = files->lang();
        auto planOut = std::make_shared<std::optional<Plan>>();
        auto calOut = std::make_shared<std::optional<CalOutcome>>();
        auto cache = std::make_shared<GridCache>(calCache);
        auto att = calAttempts;
        int acc = accuracy, tms = timerMs;
        bool wantCal = j.contains("calibration_result");
        QTimer::singleShot(0, this, [=, this] {
            run(
                tr("Reopening the session"),
                [=](const Progress &pr, const Cancel *c) {
                    Plan p = discover(rom, toolSave(vis, 2), chipRtc(hm(PLAN_INGAME), 0));
                    p.lang = lang;
                    *planOut = p;
                    if (wantCal && att.size() >= static_cast<std::size_t>(MIN_ATT)
                        && calibrationTodo(p, vis, att, acc, *cache) == 0)
                    {
                        *calOut = calibrate(rom, p, vis, att, acc, tms, *cache, 0, pr, c);
                    }
                },
                [=, this] {
                    plan = *planOut;
                    updatePhases();
                    if (*calOut && (*calOut)->complete)
                    {
                        calResult = *calOut;
                    }
                });
        });
    }
}

void Celebi2::save()
{
    if (sessionPath.isEmpty())
    {
        return;
    }
    QString tmp = sessionPath + ".tmp";
    {
        std::ofstream o(tmp.toStdString());
        o << toJson().dump(1);
    }
    QFile::remove(sessionPath);
    QFile::rename(tmp, sessionPath);
}

bool Celebi2::load(const QString &path)
{
    std::ifstream in(path.toStdString());
    json j;
    try
    {
        j = json::parse(in);
    }
    catch (...)
    {
        return false;
    }
    sessionPath = path;
    calCache = loadGrids((basePath() + ".cal.grids").toStdString());
    switchCache = loadGrids((basePath() + ".switch.grids").toStdString());
    try
    {
        fromJson(j);
    }
    catch (const std::exception &e)
    {
        QMessageBox::warning(this, tr("Session"), tr("This session file is damaged (%1).").arg(e.what()));
        return false;
    }
    return true;
}

void Celebi2::openSession()
{
    if (busy())
    {
        return;
    }
    QString p = QFileDialog::getOpenFileName(this, tr("Open a Celebi session"),
                                             QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Gen2", "JSON (*.json)");
    if (p.isEmpty())
    {
        return;
    }
    save();
    load(p);
    refresh();
}

void Celebi2::newSession()
{
    if (busy())
    {
        return;
    }
    QString p = QFileDialog::getSaveFileName(this, tr("New Celebi session"),
                                             QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Gen2/celebi-"
                                                 + QDateTime::currentDateTime().toString("yyyyMMdd-HHmm") + ".json",
                                             "JSON (*.json)");
    if (p.isEmpty())
    {
        return;
    }
    save();
    sessionPath = p;
    calAttempts.clear();
    huntAttempts.clear();
    history = json::array();
    session = Session();
    clearAfterOptions();
    save();
    refresh();
    tabs->setCurrentIndex(0);
}

void Celebi2::exportSession()
{
    QString p = QFileDialog::getSaveFileName(this, tr("Export for a report"), QDir::homePath() + "/celebi-report.json", "JSON (*.json)");
    if (p.isEmpty())
    {
        return;
    }
    json j = toJson();
    if (j.contains("files"))
    {
        j["files"].erase("paths");
    }
    j.erase("fsearch_jobs");
    json pl = json::array();
    for (const auto &h : { plan1, plan2 })
    {
        if (h)
        {
            pl.push_back(Gen2::toJson(*h));
        }
    }
    j["plans"] = pl;
    if (files)
    {
        j["files"]["rom_sha1"] = sha1Hex(*files->rom);
    }
    std::ofstream(p.toStdString()) << j.dump(1);
    status->setText(tr("Exported to %1 (no game files, no file paths).").arg(p));
}

// ====================================================================== display

void Celebi2::refresh()
{
    bool b = busy();
    progress->setVisible(b);
    cancel->setVisible(b);
    if (!b && !status->text().contains(tr("stopped")))
    {
        status->setText(sessionPath.isEmpty() ? QString() : tr("Session saved automatically."));
    }
    {
        QString html;
        if (!files)
        {
            html = tr("No game files yet. Your ROM is enough: the VC patch never changes Celebi.");
        }
        else if (files->known)
        {
            html = "<span style=\"color:#2e7d32\"><b>✓ " + esc(QString::fromStdString(files->known->label())) + "</b></span> "
                + (files->kind == "cia" ? tr("(the ROM inside your CIA)") : tr("(ROM)"));
            if (!files->titleCheck.empty())
            {
                html += "<br>" + esc(QString::fromStdString(files->titleCheck));
            }
        }
        QString why = filesProblem();
        if (files && !why.isEmpty())
        {
            html += (html.isEmpty() ? "" : "<br>") + QString("<span style=\"color:#c62828\"><b>⚠</b> ") + esc(why) + "</span>";
        }
        filesLabel->setText(html);
    }
    sessionLabel->setText(tr("Session file: %1").arg(sessionPath));
    start->setEnabled(!b && filesProblem().isEmpty());
    for (auto *w : std::initializer_list<QWidget *> { calAdd, calRemove, calRun, calNewRound, planRun, huntResave, huntRecal })
    {
        w->setEnabled(!b);
    }
    refreshCal();
    refreshPlans();
    refreshHunt();
}

void Celebi2::refreshCal()
{
    fillPhases(calPhases, phases[0]);
    calTable->setRowCount(static_cast<int>(calAttempts.size()));
    for (int i = 0; i < static_cast<int>(calAttempts.size()); i++)
    {
        const auto &a = calAttempts[i];
        QString note;
        if (calResult && i < static_cast<int>(calResult->verdict.best.errorNote.size()))
        {
            note = QString::fromStdString(calResult->verdict.best.errorNote[i]);
        }
        QStringList cells = { statsText(a.stats), QString::fromStdString(a.ingame), QString(QChar(a.reset)), note };
        for (int c = 0; c < 4; c++)
        {
            calTable->setItem(i, c, new QTableWidgetItem(cells[c]));
        }
    }
    int N = static_cast<int>(calAttempts.size());
    QString html;
    if (!plan)
    {
        html = tr("Do the Setup first.");
    }
    else if (!calResult)
    {
        int todo = files ? calibrationTodo(*plan, visible, calAttempts, accuracy, calCache) : 0;
        html = N < MIN_ATT ? tr("%1 attempts so far; calibrate from %2.").arg(N).arg(MIN_ATT)
                           : tr("%1 attempts. Click Calibrate (%2 grid jobs to compute, about %3 min on this computer's threads).")
                                 .arg(N)
                                 .arg(todo)
                                 .arg(std::max(1, todo * 13 / std::max(1, QThread::idealThreadCount()) / 60 + 1)); // ~13 s per grid job per thread (M2, 4P + 4E)
    }
    else
    {
        const auto &r = *calResult;
        const Fit &best = r.verdict.best;
        QStringList p;
        switch (r.state)
        {
        case CalState::Accepted:
            p << (r.band && r.band->more ? tr("<b>Calibration accepted</b> (%1 of %2 attempts explained), <b>but the reset band needs "
                                              "more attempts</b> (below) before the plans.")
                                               .arg(best.explained)
                                               .arg(N)
                                         : tr("<b>Calibration accepted</b> (%1 of %2 attempts explained).").arg(best.explained).arg(N));
            break;
        case CalState::NeedMore:
            p << tr("<b>At least %1 attempts.</b>").arg(MIN_ATT);
            break;
        case CalState::AddTwo:
            p << tr("<b>Not conclusive yet: add 2 attempts</b> (same timer, same in-game hour) and click Calibrate again.");
            break;
        case CalState::ShiftTimer:
            p << tr("<b>Not conclusive after %1 attempts.</b> Another hidden class predicts the same Celebi at this timer offset. Click "
                    "<i>New round</i>: move your timer %2 and make %3 new attempts. If this repeats, check your options in Setup.")
                     .arg(N)
                     .arg(signedMs(framesToMs(SHIFT)))
                     .arg(MIN_ATT);
            break;
        }
        p << tr("Best fit: timer offset %1 frames → <b>timer correction %2</b>; Kenji class %3%4.")
                 .arg(best.X)
                 .arg(signedMs(best.ms))
                 .arg(best.kenji == 1 ? "1" : "≥2")
                 .arg(r.verdict.runnerUp ? tr("; runner-up %1/%2 (Kenji %3, X %4)")
                                               .arg(r.verdict.runnerUp->explained)
                                               .arg(N)
                                               .arg(r.verdict.runnerUp->kenji)
                                               .arg(r.verdict.runnerUp->X)
                                         : QString());
        if (r.band)
        {
            const auto &bd = *r.band;
            if (bd.confirm)
            {
                p << tr("<b>Your save is likely in the coin-flip band of the default reset moment.</b> Make 2 more attempts resetting "
                        "<b>right after the Pokégear minute changes</b> (rule B), add them with reset rule B, and calibrate again.");
            }
            else if (bd.more)
            {
                p << tr("<b>Reset band unclear:</b> add 2 attempts (same reset moment) and calibrate again.");
            }
            p << tr("Reset rule: <b>%1</b> (%2).")
                     .arg(QChar(bd.rule))
                     .arg(bd.rule == 'A' ? tr("reset about 20 s after the Pokégear minute changes")
                                         : tr("reset right after the Pokégear minute changes"));
        }
        if (r.state == CalState::Accepted && setup)
        {
            p << tr("Next: <b>set your PokeTimerPlus calibration to %1 ms</b> (%2 %3) for the hunt, then go to Plans.")
                     .arg(timerMs + framesToMs(best.X))
                     .arg(timerMs)
                     .arg(signedMs(framesToMs(best.X)));
        }
        html = p.join("<br><br>");
    }
    calText->setHtml(html);
}

void Celebi2::refreshPlans()
{
    QString s;
    if (!setup || !calResult || calResult->state != CalState::Accepted)
    {
        s = tr("Plans come after an accepted calibration.");
    }
    else
    {
        char rule = calResult->band ? calResult->band->rule : 'A';
        int cal = timerMs + framesToMs(calResult->verdict.best.X);
        s = tr("Your save: Kenji class %1, clock carry class %2. Hunt at in-game <b>%3:xx</b> (the hour of your calibration attempts, "
               "minutes around %4), reset rule <b>%5</b>, PokeTimerPlus calibration <b>%6 ms</b>.")
                .arg(setup->kenji == 1 ? "1" : "≥2")
                .arg(setup->k)
                .arg(setup->hour, 2, 10, QChar('0'))
                .arg(setup->minute, 2, 10, QChar('0'))
                .arg(QChar(rule))
                .arg(cal);
    }
    planSetup->setHtml(s);
    int gridsLeft = switchTodo(accuracy, switchCache);
    planRun->setText(fsDone.empty() ? tr("Find plans 1 and 2")
                         : !fsStopped ? tr("Continue the plan search")
                         : gridsLeft  ? tr("Compute the hunt-advice grids")
                                      : tr("Plans and grids ready"));
    planRun->setEnabled(!busy() && setup.has_value() && (!fsStopped || (gridsLeft > 0 && plan1 && plan2)));
    for (int i = 0; i < 2; i++)
    {
        const auto &h = i == 0 ? plan1 : plan2;
        if (h)
        {
            planLabel[i]->setText(tr("6th %1, YES %2, final A %3 · Celebi DVs %4 · shiny for %5 frame-counter values, %6 seconds values")
                                      .arg(h->d6 >= 0 ? "+" + QString::number(h->d6) : QString::number(h->d6))
                                      .arg(h->dyes >= 0 ? "+" + QString::number(h->dyes) : QString::number(h->dyes))
                                      .arg(h->final)
                                      .arg(dvText(dvOf(h->enemy)))
                                      .arg(setText(h->fOk))
                                      .arg(setText(h->sOk)));
            fillPhases(planPhases[i], phases[i + 1]);
        }
        else
        {
            planLabel[i]->setText(i == 0 ? tr("Not found yet.") : tr("Not found yet (plan 2 covers what plan 1 misses)."));
            fillPhases(planPhases[i], {});
        }
    }
    QStringList t;
    if (!fsDone.empty())
    {
        int total = static_cast<int>(fsJobs(plan ? *plan : Plan {}, { setup ? setup->rtc : Rtc {} }).size());
        t << tr("Plan search: %1 of %2 jobs done%3.")
                 .arg(fsDone.size())
                 .arg(total)
                 .arg(fsStopped ? tr(", target reached or search complete") : tr("; continue it to improve the plans"));
    }
    if (plan1 && plan2)
    {
        std::set<int> u(plan1->fOk.begin(), plan1->fOk.end());
        u.insert(plan2->fOk.begin(), plan2->fOk.end());
        t << tr("Plans 1 + 2 together cover %1/60 values of the hidden frame counter.").arg(u.size());
        int todo = switchTodo(accuracy, switchCache);
        t << (todo == 0 ? tr("Hunt-advice grids: ready.")
                        : tr("Hunt-advice grids: %1 of 600 jobs left. They start after the plan search; hunt plan 1 meanwhile and "
                             "enter your attempts.")
                              .arg(todo));
    }
    planText->setHtml(t.join("<br>"));
}

void Celebi2::refreshHunt()
{
    huntTable->setRowCount(static_cast<int>(huntAttempts.size()));
    for (int i = 0; i < static_cast<int>(huntAttempts.size()); i++)
    {
        const auto &a = huntAttempts[i];
        QString note = shinyStats(a.stats) ? tr("SHINY") : QString();
        QStringList cells = { QString::number(a.plan), statsText(a.stats), QString::number(a.timerMoved), note };
        for (int c = 0; c < 4; c++)
        {
            huntTable->setItem(i, c, new QTableWidgetItem(cells[c]));
        }
    }
    QStringList p;
    if (!plan1 || !plan2)
    {
        p << tr("Find the plans first (Plans tab).");
    }
    else if (switchTodo(accuracy, switchCache) > 0)
    {
        p << tr("Hunt plan 1 and enter your attempts; advice appears when the hunt-advice grids are ready (Plans tab).");
        if (!busy())
        {
            p << tr("The grids aren't computing: Plans tab → <i>Compute the hunt-advice grids</i>.");
        }
    }
    else if (huntAttempts.empty())
    {
        p << tr("Advice: <b>hunt plan 1</b>.");
    }
    else
    {
        auto adv = huntAdvice(switchCache, *plan1, *plan2, accuracy, huntAttempts);
        if (adv.recalibrate)
        {
            p << tr("<b>RECALIBRATE</b>: 2 of your last 3 attempts fit no frame counter at any timer offset within 2 frames (the "
                    "calibration is likely off). Click <i>Recalibrate</i> and make fresh calibration attempts (same save).");
        }
        else
        {
            if (adv.move)
            {
                int c = huntAttempts.back().timerMoved;
                p << tr("<b>MOVE YOUR TIMER %1 frame(s) (%2)</b> and keep hunting; set \"Timer moved\" to %3 on the next attempts.")
                         .arg(adv.move > 0 ? "+" + QString::number(adv.move) : QString::number(adv.move))
                         .arg(signedMs(framesToMs(adv.move)))
                         .arg(c + adv.move);
                huntMoved->setValue(c + adv.move);
            }
            QString what = QString::fromStdString(adv.advice);
            QString nice = what == "stay on plan 1" ? tr("stay on plan 1")
                : what == "stay on plan 2"          ? tr("stay on plan 2")
                : what == "switch to plan 2"        ? tr("<b>switch to plan 2</b> (Plans tab: its phases)")
                : what == "re-save"                 ? tr("<b>re-save</b>: save again at the shrine (same spot), then click <i>I re-saved</i>")
                                                    : what;
            p << tr("P(your frame counter is in plan 1's set) %1 · in plan 2's set %2 → %3.").arg(adv.p1, 0, 'f', 2).arg(adv.p2, 0, 'f', 2).arg(nice);
            huntPlan->setCurrentIndex(what == "switch to plan 2" || what == "stay on plan 2" ? 1 : 0);
        }
        if (session.resaves >= 2)
        {
            p << QString::fromUtf8(RESAVE_HINT);
        }
    }
    huntText->setHtml(p.join("<br><br>"));
}
