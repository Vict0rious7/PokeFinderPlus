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
 * PokeFinderPlus: new file. XY Trainer ID RNG, Finder side (blink taps → seed → IDs → PokeTimerPlus).
 * Method and values: BZL's 6genTidSearch (used with permission), https://blog.bzl-web.com/entry/2020/08/31/202306
 */

#include "IDs6.hpp"
#include <Core/Gen6/Generators/IDGenerator6.hpp>
#include <Core/Gen6/Searchers/BlinkSearcher6.hpp>
#include <Core/Gen6/Searchers/OrasSearcher6.hpp>
#include <Core/Gen6/XYTid.hpp>
#include <Form/Controls/IDsFilter.hpp>
#include <Form/Controls/TableView.hpp>
#include <Form/Controls/TextBox.hpp>
#include <Model/Gen6/BlinkModel6.hpp>
#include <Model/Gen6/IDModel6.hpp>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocalSocket>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSettings>
#include <QShortcut>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableWidget>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <limits>
#include <memory>

namespace
{
    TextBox *textBox(InputType type, const QString &text, const QString &name)
    {
        auto *box = new TextBox;
        box->setValues(type);
        box->setText(text);
        box->setObjectName(name);
        return box;
    }

    QString clock(qint64 epochMs)
    {
        return QDateTime::fromMSecsSinceEpoch(epochMs).toString("HH:mm:ss.zzz");
    }

    /// Today's (or yesterday's, if later than now) time typed as HH:mm:ss.zzz, in ms since 1970; 0 if invalid
    qint64 fromClock(const QString &text)
    {
        QTime time = QTime::fromString(text.trimmed(), "HH:mm:ss.zzz");
        if (!time.isValid())
        {
            time = QTime::fromString(text.trimmed(), "HH:mm:ss");
        }
        if (!time.isValid())
        {
            return 0;
        }
        QDateTime when(QDate::currentDate(), time);
        if (when > QDateTime::currentDateTime())
        {
            when = when.addDays(-1);
        }
        return when.toMSecsSinceEpoch();
    }
}

IDs6::IDs6(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Gen 6 TID/SID"));

    auto *layout = new QVBoxLayout(this);
    games = new QTabWidget;
    games->setObjectName("tabWidgetGames");
    layout->addWidget(games);

    // ======== X/Y ========
    auto *xyPage = new QWidget;
    xyPage->setObjectName("tabXY");
    xyPage->setFocusPolicy(Qt::ClickFocus); // a click on the page takes Space back from the text fields
    auto *xyLayout = new QGridLayout(xyPage);

    // ---- Seed Finder: the professor's blinks ----
    auto *seedFinder = new QGroupBox(tr("Seed Finder"));
    auto *seedFinderLayout = new QVBoxLayout(seedFinder);
    auto *seedTop = new QHBoxLayout;
    auto *blinkInfo = new QGroupBox(tr("RNG Info"));
    auto *blinkForm = new QGridLayout(blinkInfo);
    blink = new QPushButton(tr("Blink (Space)"));
    blink->setObjectName("pushButtonBlink");
    blink->setFocusPolicy(Qt::NoFocus);
    clear = new QPushButton(tr("Clear"));
    clear->setFocusPolicy(Qt::NoFocus);
    tapStatus = new QLabel(tr("No taps yet"));
    tapStatus->setObjectName("labelTapStatus");
    advanceMin = textBox(InputType::Advance32Bit, QString::number(XYTid::DEFAULT_ADVANCE_MIN), "textBoxAdvanceMin");
    advanceMax = textBox(InputType::Advance32Bit, QString::number(XYTid::DEFAULT_ADVANCE_MAX), "textBoxAdvanceMax");
    all = new QRadioButton(tr("All Seeds"));
    near = new QRadioButton(tr("Near Base Seed"));
    near->setObjectName("radioButtonNear");
    all->setChecked(true);
    baseSeed = new TextBox;
    baseSeed->setObjectName("textBoxBaseSeed");
    baseSeed->setValues(InputType::Seed32Bit);
    hours = new TextBox;
    hours->setObjectName("textBoxHours");
    hours->setValues(1, 1193, 4, 10);
    hours->setText("18");
    blinkForm->addWidget(blink, 0, 0, 1, 2);
    blinkForm->addWidget(clear, 1, 0);
    blinkForm->addWidget(tapStatus, 1, 1);
    blinkForm->addWidget(new QLabel(tr("Min Blink Advance")), 2, 0);
    blinkForm->addWidget(advanceMin, 2, 1);
    blinkForm->addWidget(new QLabel(tr("Max Blink Advance")), 3, 0);
    blinkForm->addWidget(advanceMax, 3, 1);
    blinkForm->addWidget(all, 4, 0);
    blinkForm->addWidget(near, 4, 1);
    blinkForm->addWidget(new QLabel(tr("Base Seed")), 5, 0);
    blinkForm->addWidget(baseSeed, 5, 1);
    blinkForm->addWidget(new QLabel(tr("Max Hours")), 6, 0);
    blinkForm->addWidget(hours, 6, 1);
    auto *gapsBox = new QGroupBox(tr("Blink Gaps"));
    auto *gapsLayout = new QVBoxLayout(gapsBox);
    gaps = new QPlainTextEdit;
    gaps->setObjectName("plainTextEditGaps");
    gaps->setPlaceholderText(tr("Frames between blinks"));
    gapsLayout->addWidget(gaps);
    seedTop->addWidget(blinkInfo);
    seedTop->addWidget(gapsBox);
    seedFinderLayout->addLayout(seedTop);
    auto *buttons = new QHBoxLayout;
    search = new QPushButton(tr("Search"));
    search->setObjectName("pushButtonSearch");
    cancel = new QPushButton(tr("Cancel"));
    cancel->setEnabled(false);
    buttons->addWidget(search);
    buttons->addWidget(cancel);
    seedFinderLayout->addLayout(buttons);
    progress = new QProgressBar;
    progress->setRange(0, 100);
    progress->setValue(0);
    seedFinderLayout->addWidget(progress);
    xyLayout->addWidget(seedFinder, 0, 0);

    seedModel = new BlinkModel6(this);
    seedTable = new TableView;
    seedTable->setObjectName("tableViewSeeds");
    seedTable->setModel(seedModel);
    xyLayout->addWidget(seedTable, 1, 0);

    // ---- Generator: IDs from the state at the first tap ----
    auto *generator = new QGroupBox(tr("Generator"));
    auto *generatorLayout = new QHBoxLayout(generator);
    auto *idInfo = new QGroupBox(tr("RNG Info"));
    auto *idForm = new QGridLayout(idInfo);
    state = new QLineEdit;
    state->setObjectName("lineEditState");
    state->setMinimumWidth(280); // 4 hex words
    firstTap = new QLineEdit;
    firstTap->setObjectName("lineEditFirstTap");
    firstTap->setPlaceholderText(tr("HH:mm:ss.zzz"));
    firstTap->setMinimumWidth(110);
    idMin = textBox(InputType::Advance32Bit, "0", "textBoxIDMin");
    idMax = textBox(InputType::Advance32Bit, "5000", "textBoxIDMax");
    generate = new QPushButton(tr("Generate"));
    generate->setObjectName("pushButtonGenerate");
    idForm->addWidget(new QLabel(tr("State")), 0, 0);
    idForm->addWidget(state, 0, 1);
    idForm->addWidget(new QLabel(tr("First Tap")), 1, 0);
    idForm->addWidget(firstTap, 1, 1);
    idForm->addWidget(new QLabel(tr("Initial Advances")), 2, 0);
    idForm->addWidget(idMin, 2, 1);
    idForm->addWidget(new QLabel(tr("Max Advances")), 3, 0);
    idForm->addWidget(idMax, 3, 1);
    idForm->addWidget(generate, 4, 0, 1, 2);
    auto *filterBox = new QGroupBox(tr("Filters"));
    auto *filterLayout = new QVBoxLayout(filterBox);
    filter = new IDsFilter;
    filter->setObjectName("idFilter");
    filterLayout->addWidget(filter);
    generatorLayout->addWidget(idInfo);
    generatorLayout->addWidget(filterBox);
    xyLayout->addWidget(generator, 0, 1);

    auto *idSide = new QVBoxLayout;
    idModel = new IDModel6(this);
    idTable = new TableView;
    idTable->setObjectName("tableViewIDs");
    idTable->setModel(idModel);
    idSide->addWidget(idTable);
    auto *sendRow = new QHBoxLayout;
    send = new QPushButton(tr("Send to PokeTimerPlus"));
    send->setObjectName("pushButtonSend");
    copy = new QPushButton(tr("Copy for PokeTimerPlus"));
    copy->setObjectName("pushButtonCopy");
    sendRow->addWidget(send);
    sendRow->addWidget(copy);
    idSide->addLayout(sendRow);
    xyLayout->addLayout(idSide, 1, 1);
    xyLayout->setRowStretch(1, 1);
    xyLayout->setColumnStretch(0, 1);
    xyLayout->setColumnStretch(1, 1);
    games->addTab(xyPage, tr("X/Y"));

    // ======== OR/AS ========
    auto *orasPage = new QWidget;
    orasPage->setObjectName("tabORAS");
    orasPage->setFocusPolicy(Qt::ClickFocus);
    auto *orasLayout = new QGridLayout(orasPage);

    // ---- Seed Finder: new games launched at known times ----
    auto *launchFinder = new QGroupBox(tr("Seed Finder"));
    auto *launchFinderLayout = new QVBoxLayout(launchFinder);
    auto *launchTop = new QHBoxLayout;
    auto *launchInfo = new QGroupBox(tr("RNG Info"));
    auto *launchForm = new QGridLayout(launchInfo);
    launch = new QPushButton(tr("Launch (Space)"));
    launch->setObjectName("pushButtonLaunch");
    launch->setFocusPolicy(Qt::NoFocus);
    removeLaunch = new QPushButton(tr("Remove"));
    removeLaunch->setFocusPolicy(Qt::NoFocus);
    clearLaunches = new QPushButton(tr("Clear"));
    clearLaunches->setFocusPolicy(Qt::NoFocus);
    launchStatus = new QLabel;
    launchStatus->setObjectName("labelLaunchStatus");
    tolerance = textBox(InputType::Delay, QString::number(OrasTid::DEFAULT_TOLERANCE), "textBoxTolerance");
    tolerance->setValues(0, 60000, 5, 10);
    launchForm->addWidget(launch, 0, 0, 1, 2);
    launchForm->addWidget(removeLaunch, 1, 0);
    launchForm->addWidget(clearLaunches, 1, 1);
    launchForm->addWidget(launchStatus, 2, 0, 1, 2);
    launchForm->addWidget(new QLabel(tr("Tolerance (ms)")), 3, 0);
    launchForm->addWidget(tolerance, 3, 1);
    launchForm->setRowStretch(4, 1);
    auto *launchBox = new QGroupBox(tr("Launches"));
    auto *launchBoxLayout = new QVBoxLayout(launchBox);
    launches = new QTableWidget(0, 2);
    launches->setObjectName("tableWidgetLaunches");
    launches->setHorizontalHeaderLabels({ tr("Time (ms)"), tr("TID") });
    launches->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    launches->setFocusPolicy(Qt::ClickFocus);
    launchBoxLayout->addWidget(launches);
    launchTop->addWidget(launchInfo);
    launchTop->addWidget(launchBox);
    launchFinderLayout->addLayout(launchTop);
    auto *orasButtons = new QHBoxLayout;
    orasSearch = new QPushButton(tr("Search"));
    orasSearch->setObjectName("pushButtonOrasSearch");
    orasCancel = new QPushButton(tr("Cancel"));
    orasCancel->setEnabled(false);
    orasButtons->addWidget(orasSearch);
    orasButtons->addWidget(orasCancel);
    launchFinderLayout->addLayout(orasButtons);
    orasProgress = new QProgressBar;
    orasProgress->setRange(0, 100);
    orasProgress->setValue(0);
    launchFinderLayout->addWidget(orasProgress);
    orasLayout->addWidget(launchFinder, 0, 0);

    baseModel = new QStandardItemModel(this);
    baseModel->setHorizontalHeaderLabels({ tr("Base Seed") });
    baseTable = new TableView;
    baseTable->setObjectName("tableViewBaseSeeds");
    baseTable->setModel(baseModel);
    baseTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    baseTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    orasLayout->addWidget(baseTable, 1, 0);

    // ---- SID Finder: the new game kept ----
    auto *sidFinder = new QGroupBox(tr("SID Finder"));
    auto *sidForm = new QGridLayout(sidFinder);
    orasBase = new TextBox;
    orasBase->setObjectName("textBoxOrasBase");
    orasBase->setValues(InputType::Seed32Bit);
    orasLaunchMs = textBox(InputType::Seed32Bit, "0", "textBoxOrasLaunch");
    orasLaunchMs->setValues(0, 0x7fffffff, 10, 10);
    orasTid = textBox(InputType::TIDSID, "", "textBoxOrasTID");
    orasNo = textBox(InputType::Advance32Bit, "0", "textBoxOrasNo");
    orasTolerance = textBox(InputType::Delay, QString::number(OrasTid::DEFAULT_TOLERANCE), "textBoxOrasTolerance");
    orasTolerance->setValues(0, 60000, 5, 10);
    identify = new QPushButton(tr("Find"));
    identify->setObjectName("pushButtonIdentify");
    sidForm->addWidget(new QLabel(tr("Base Seed")), 0, 0);
    sidForm->addWidget(orasBase, 0, 1);
    sidForm->addWidget(new QLabel(tr("Launch Time (ms)")), 1, 0);
    sidForm->addWidget(orasLaunchMs, 1, 1);
    sidForm->addWidget(new QLabel(tr("TID Obtained")), 2, 0);
    sidForm->addWidget(orasTid, 2, 1);
    sidForm->addWidget(new QLabel(tr("Name \"No\" Count")), 3, 0);
    sidForm->addWidget(orasNo, 3, 1);
    sidForm->addWidget(new QLabel(tr("Tolerance (ms)")), 4, 0);
    sidForm->addWidget(orasTolerance, 4, 1);
    sidForm->addWidget(identify, 5, 0, 1, 2);
    sidForm->setRowStretch(6, 1);
    orasLayout->addWidget(sidFinder, 0, 1);

    identModel = new QStandardItemModel(this);
    identModel->setHorizontalHeaderLabels({ tr("Seed"), tr("Offset (ms)"), tr("TID"), tr("SID"), tr("TSV"), tr("TRV") });
    identTable = new TableView;
    identTable->setObjectName("tableViewIdentified");
    identTable->setModel(identModel);
    identTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    orasLayout->addWidget(identTable, 1, 1);
    orasLayout->setRowStretch(1, 1);
    orasLayout->setColumnStretch(0, 1);
    orasLayout->setColumnStretch(1, 1);
    games->addTab(orasPage, tr("OR/AS"));

    auto *space = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(space, &QShortcut::activated, this, &IDs6::tap);
    connect(blink, &QPushButton::pressed, this, &IDs6::tap); // on press, not release: closer to the blink
    connect(clear, &QPushButton::clicked, this, &IDs6::clearTaps);
    connect(search, &QPushButton::clicked, this, &IDs6::startSearch);
    connect(seedTable->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &IDs6::useSeed);
    connect(generate, &QPushButton::clicked, this, &IDs6::generateIDs);
    connect(send, &QPushButton::clicked, this, &IDs6::sendToTimer);
    connect(copy, &QPushButton::clicked, this, &IDs6::copyForTimer);
    connect(launch, &QPushButton::pressed, this, &IDs6::addLaunch); // on press, together with A
    connect(removeLaunch, &QPushButton::clicked, this, &IDs6::removeLaunchRow);
    connect(clearLaunches, &QPushButton::clicked, this, &IDs6::clearLaunchList);
    connect(launches, &QTableWidget::itemChanged, this, [this] {
        saveLaunches();
        fillFromLastLaunch();
    });
    connect(orasSearch, &QPushButton::clicked, this, &IDs6::startOrasSearch);
    connect(baseTable->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &IDs6::useBaseSeed);
    connect(identify, &QPushButton::clicked, this, &IDs6::identifyLaunch);

    QSettings setting;
    games->setCurrentIndex(setting.value("ids6/tab", 0).toInt());
    tolerance->setText(QString::number(setting.value("ids6/orasTolerance", OrasTid::DEFAULT_TOLERANCE).toInt()));
    launchEpoch = setting.value("ids6/orasLaunchEpoch", 0).toLongLong();
    {
        QSignalBlocker block(launches);
        for (const QString &entry : setting.value("ids6/orasLaunches").toStringList())
        {
            QStringList parts = entry.split(':');
            int row = launches->rowCount();
            launches->insertRow(row);
            launches->setItem(row, 0, new QTableWidgetItem(parts.value(0)));
            launches->setItem(row, 1, new QTableWidgetItem(parts.value(1)));
        }
    }
    if (launches->rowCount() == 0)
    {
        launchEpoch = 0;
    }
    launchStatus->setText(launchEpoch ? tr("First launch at %1").arg(clock(launchEpoch)) : tr("No launch yet"));
    fillFromLastLaunch();
    resize(1150, 650);
    if (setting.contains("ids6/geometryTabs"))
    {
        restoreGeometry(setting.value("ids6/geometryTabs").toByteArray());
    }
    games->currentWidget()->setFocus(); // Space taps (a focused text field would take it)
    qApp->installEventFilter(this);
    connect(games, &QTabWidget::currentChanged, this, [this] { games->currentWidget()->setFocus(); });
}

bool IDs6::eventFilter(QObject *object, QEvent *event)
{
    // OR/AS: Space always adds a launch, even while a TID or a number is being typed (no field there takes a space)
    if (event->type() == QEvent::ShortcutOverride && isOras() && isAncestorOf(QApplication::focusWidget()))
    {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Space && key->modifiers() == Qt::NoModifier)
        {
            return true; // not accepted by the editor: the Space shortcut fires
        }
    }
    return QWidget::eventFilter(object, event);
}

IDs6::~IDs6()
{
    qApp->removeEventFilter(this);
    if (running)
    {
        running->cancelSearch();
        delete running; // joins the threads
    }
    if (orasRunning)
    {
        orasRunning->cancelSearch();
        delete orasRunning;
    }
    QSettings setting;
    setting.setValue("ids6/orasTolerance", tolerance->getUInt());
    setting.setValue("ids6/tab", games->currentIndex());
    setting.setValue("ids6/geometryTabs", saveGeometry());
}

void IDs6::tap()
{
    if (isOras())
    {
        addLaunch();
        return;
    }
    if (!recording)
    {
        gaps->clear();
        tapTimer.start();
        tapEpoch = QDateTime::currentMSecsSinceEpoch();
        lastTapMs = 0;
        recording = true;
        tapStatus->setText(tr("0 gaps"));
        return;
    }
    // Same rounding as 6genTidSearch: whole ms, then frames truncated
    qint64 now = tapTimer.elapsed();
    u32 frames = XYTid::msToFrames(static_cast<double>(now - lastTapMs));
    lastTapMs = now;
    gaps->appendPlainText(QString::number(frames));
    int count = gaps->toPlainText().split('\n', Qt::SkipEmptyParts).size();
    tapStatus->setText(tr("%1 gaps").arg(count));
}

void IDs6::clearTaps()
{
    gaps->clear();
    recording = false;
    tapEpoch = 0;
    tapStatus->setText(tr("No taps yet"));
}

std::vector<u32> IDs6::parseGaps(bool *ok) const
{
    std::vector<u32> list;
    *ok = true;
    for (const QString &line : gaps->toPlainText().split(QRegularExpression("[\\s,;]+"), Qt::SkipEmptyParts))
    {
        bool number;
        u32 value = line.toUInt(&number);
        if (!number)
        {
            *ok = false;
            return {};
        }
        list.push_back(value);
    }
    return list;
}

void IDs6::startSearch()
{
    if (running)
    {
        return;
    }
    bool ok;
    std::vector<u32> list = parseGaps(&ok);
    if (!ok || list.empty())
    {
        QMessageBox msg(QMessageBox::Warning, tr("No blinks"), tr("No gaps to search."));
        msg.exec();
        return;
    }
    if (list.size() < 5)
    {
        QMessageBox msg(QMessageBox::Question, tr("Few blinks"), tr("Only %1 gaps: many seeds may match. Search anyway?").arg(list.size()),
                        QMessageBox::Yes | QMessageBox::No);
        if (msg.exec() != QMessageBox::Yes)
        {
            return;
        }
    }

    recording = false; // the next tap starts a new recording, as in 6genTidSearch
    searchTapEpoch = tapEpoch;

    QSettings setting;
    int threads = setting.value("settings/threads", QThread::idealThreadCount()).toInt();
    bool nearBase = near->isChecked();
    u32 base = baseSeed->getUInt();
    u32 count = static_cast<u32>(std::min<u64>(static_cast<u64>(hours->getUInt()) * 3600000, 0xffffffffull));
    auto *searcher = new BlinkSearcher6();
    if (!searcher->startSearch(threads, list, advanceMin->getUInt(), advanceMax->getUInt(), base, count, nearBase))
    {
        delete searcher;
        QMessageBox msg(QMessageBox::Warning, tr("Invalid settings"), tr("Advance minimum is above maximum."));
        msg.exec();
        return;
    }

    running = searcher;
    seedModel->clearModel();
    seedModel->setNear(nearBase, base);
    seedModel->setFirstTap(searchTapEpoch);
    search->setEnabled(false);
    cancel->setEnabled(true);
    progress->setValue(0);

    auto *timer = new QTimer(this);
    connect(cancel, &QPushButton::clicked, timer, [this, searcher] {
        searcher->cancelSearch();
        cancel->setEnabled(false);
    });
    connect(timer, &QTimer::timeout, this, [this, searcher, timer] {
        seedModel->addItems(searcher->getResults());
        progress->setValue(searcher->getProgress());
        if (!searcher->isSearching())
        {
            timer->stop();
            seedModel->addItems(searcher->getResults());
            delete searcher;
            running = nullptr;
            search->setEnabled(true);
            cancel->setEnabled(false);
            timer->deleteLater();
        }
    });
    timer->start(200);
}

void IDs6::useSeed(const QModelIndex &index)
{
    if (!index.isValid())
    {
        return;
    }
    const BlinkResult6 &result = seedModel->getItem(index.row());
    state->setText(seedModel->data(seedModel->index(index.row(), 2), Qt::DisplayRole).toString());
    firstTap->setText(searchTapEpoch ? clock(searchTapEpoch) : QString());
    baseSeed->setText(QString::number(result.seed, 16).toUpper().rightJustified(8, '0')); // next reset: search near it
    generateIDs();
}

bool IDs6::parseState(std::array<u32, 4> &words) const
{
    QStringList parts = state->text().split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts);
    if (parts.size() != 4)
    {
        return false;
    }
    for (int i = 0; i < 4; i++)
    {
        bool ok;
        words[3 - i] = parts[i].toUInt(&ok, 16);
        if (!ok)
        {
            return false;
        }
    }
    return true;
}

void IDs6::generateIDs()
{
    std::array<u32, 4> words;
    if (!parseState(words))
    {
        QMessageBox msg(QMessageBox::Warning, tr("No state"), tr("State: 4 hex words."));
        msg.exec();
        return;
    }
    if (idMin->getUInt() > idMax->getUInt())
    {
        return;
    }
    idModel->clearModel();
    idModel->addItems(generateIDs6(words, idMin->getUInt(), idMax->getUInt(), filter->getFilter(false)));
}

QString IDs6::handoffText(bool *ok)
{
    *ok = false;
    std::array<u32, 4> words;
    if (!parseState(words))
    {
        QMessageBox msg(QMessageBox::Warning, tr("No state"), tr("State: 4 hex words."));
        msg.exec();
        return {};
    }
    qint64 tap = fromClock(firstTap->text());
    if (tap == 0)
    {
        QMessageBox msg(QMessageBox::Warning, tr("No first tap"), tr("First tap: HH:mm:ss.zzz."));
        msg.exec();
        return {};
    }
    XYTid::Handoff handoff;
    handoff.state = words;
    handoff.tapEpochMs = tap;
    QModelIndexList selected = idTable->selectionModel()->selectedRows();
    if (selected.isEmpty() && idTable->currentIndex().isValid())
    {
        selected.append(idTable->currentIndex());
    }
    if (!selected.isEmpty())
    {
        const IDState6 &id = idModel->getItem(selected.first().row());
        handoff.target = id.getAdvances();
        handoff.tid = id.getTID();
        handoff.sid = id.getSID();
    }
    *ok = true;
    return QString::fromStdString(handoff.toString());
}

void IDs6::sendToTimer()
{
    bool ok;
    QString text = handoffText(&ok);
    if (!ok)
    {
        return;
    }
    QLocalSocket socket;
    socket.connectToServer(XYTid::SOCKET_NAME);
    QByteArray reply;
    if (socket.waitForConnected(1000))
    {
        socket.write((text + "\n").toUtf8());
        if (socket.waitForBytesWritten(1000) && socket.waitForReadyRead(3000))
        {
            reply = socket.readLine().trimmed();
        }
        socket.disconnectFromServer();
    }
    if (reply == "OK")
    {
        return;
    }
    QApplication::clipboard()->setText(text);
    QString why = reply == "BUSY" ? tr("PokeTimerPlus is running another timer.")
        : reply.isEmpty()         ? tr("PokeTimerPlus isn't open.")
                                  : tr("PokeTimerPlus refused the details.");
    QMessageBox msg(QMessageBox::Information, tr("Not sent"), why + " " + tr("Copied to the clipboard instead."));
    msg.exec();
}

void IDs6::copyForTimer()
{
    bool ok;
    QString text = handoffText(&ok);
    if (ok)
    {
        QApplication::clipboard()->setText(text);
    }
}

bool IDs6::isOras() const
{
    return games->currentIndex() == 1;
}

void IDs6::saveLaunches() const
{
    QStringList list;
    for (int row = 0; row < launches->rowCount(); row++)
    {
        QTableWidgetItem *m = launches->item(row, 0);
        QTableWidgetItem *t = launches->item(row, 1);
        list.append((m ? m->text().trimmed() : QString()) + ":" + (t ? t->text().trimmed() : QString()));
    }
    QSettings setting;
    setting.setValue("ids6/orasLaunches", list);
    setting.setValue("ids6/orasLaunchEpoch", launchEpoch);
}

void IDs6::addLaunch()
{
    // Wall clock, so a launch list survives closing the window (the first launch's time is saved)
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (games->currentWidget()->isAncestorOf(QApplication::focusWidget()))
    {
        games->currentWidget()->setFocus(); // commits the TID being typed
    }
    if (launches->rowCount() == 0 || launchEpoch == 0)
    {
        launches->setRowCount(0);
        launchEpoch = now;
    }
    QSignalBlocker block(launches);
    int row = launches->rowCount();
    launches->insertRow(row);
    launches->setItem(row, 0, new QTableWidgetItem(QString::number(now - launchEpoch)));
    launches->setItem(row, 1, new QTableWidgetItem());
    launches->scrollToBottom();
    launchStatus->setText(tr("First launch at %1").arg(clock(launchEpoch)));
    saveLaunches();
}

void IDs6::removeLaunchRow()
{
    int row = launches->currentRow();
    if (row < 0)
    {
        row = launches->rowCount() - 1;
    }
    if (row < 0)
    {
        return;
    }
    launches->removeRow(row);
    if (launches->rowCount() == 0)
    {
        launchEpoch = 0;
        launchStatus->setText(tr("No launch yet"));
    }
    saveLaunches();
}

void IDs6::clearLaunchList()
{
    if (launches->rowCount() > 0)
    {
        QMessageBox msg(QMessageBox::Question, tr("Clear"), tr("Remove every launch?"), QMessageBox::Yes | QMessageBox::No);
        if (msg.exec() != QMessageBox::Yes)
        {
            return;
        }
    }
    launches->setRowCount(0);
    launchEpoch = 0;
    launchStatus->setText(tr("No launch yet"));
    saveLaunches();
}

void IDs6::startOrasSearch()
{
    if (orasRunning)
    {
        return;
    }
    std::vector<OrasTid::Launch> list;
    for (int row = 0; row < launches->rowCount(); row++)
    {
        QTableWidgetItem *m = launches->item(row, 0);
        QTableWidgetItem *t = launches->item(row, 1);
        bool okMs = false, okTid = false;
        qint64 ms = m ? m->text().trimmed().toLongLong(&okMs) : 0;
        int tid = t ? t->text().trimmed().toInt(&okTid) : 0;
        if (!okMs || !okTid || ms < 0 || tid < 0 || tid > 65535)
        {
            QMessageBox msg(QMessageBox::Warning, tr("Launch %1").arg(row + 1),
                            tr("Launch %1 needs a time in ms and a TID (0 to 65535).").arg(row + 1));
            msg.exec();
            return;
        }
        list.emplace_back(OrasTid::Launch { static_cast<u32>(ms), static_cast<u16>(tid) });
    }
    if (list.size() < 2)
    {
        QMessageBox msg(QMessageBox::Warning, tr("Launches"), tr("At least 2 launches."));
        msg.exec();
        return;
    }
    // Times are counted from the first launch
    u32 first = list[0].ms;
    for (auto &l : list)
    {
        l.ms -= first;
    }
    if (list.size() < 4)
    {
        QMessageBox msg(QMessageBox::Question, tr("Few launches"),
                        tr("Only %1 launches: many seeds may match. Search anyway?").arg(list.size()), QMessageBox::Yes | QMessageBox::No);
        if (msg.exec() != QMessageBox::Yes)
        {
            return;
        }
    }

    QSettings setting;
    int threads = setting.value("settings/threads", QThread::idealThreadCount()).toInt();
    auto *searcher = new OrasSearcher6();
    if (!searcher->startSearch(threads, list, tolerance->getUInt()))
    {
        delete searcher;
        return;
    }

    orasRunning = searcher;
    baseModel->clear();
    QStringList headers { tr("Base Seed") };
    for (size_t i = 0; i < list.size(); i++)
    {
        headers.append(tr("Launch %1").arg(i + 1));
    }
    baseModel->setHorizontalHeaderLabels(headers);
    orasTolerance->setText(tolerance->text());
    orasSearch->setEnabled(false);
    orasCancel->setEnabled(true);
    orasProgress->setValue(0);

    auto collected = std::make_shared<std::vector<OrasTid::BaseMatch>>();
    auto *timer = new QTimer(this);
    connect(orasCancel, &QPushButton::clicked, timer, [this, searcher] {
        searcher->cancelSearch();
        orasCancel->setEnabled(false);
    });
    connect(timer, &QTimer::timeout, this, [this, searcher, timer, collected] {
        auto more = searcher->getResults();
        collected->insert(collected->end(), more.begin(), more.end());
        orasProgress->setValue(searcher->getProgress());
        if (searcher->isSearching())
        {
            return;
        }
        timer->stop();
        more = searcher->getResults();
        collected->insert(collected->end(), more.begin(), more.end());
        std::stable_sort(collected->begin(), collected->end(), [](const auto &a, const auto &b) { return a.seed < b.seed; });
        for (const auto &match : *collected)
        {
            QList<QStandardItem *> row { new QStandardItem(QString::number(match.seed, 16).toUpper().rightJustified(8, '0')) };
            for (s32 offset : match.offsets)
            {
                row.append(new QStandardItem(QString::number(offset)));
            }
            baseModel->appendRow(row);
        }
        if (collected->empty() && !searcher->isCancelled())
        {
            QMessageBox msg(QMessageBox::Information, tr("No seed"), tr("No base seed explains these launches."));
            msg.exec();
        }
        delete searcher;
        orasRunning = nullptr;
        orasSearch->setEnabled(true);
        orasCancel->setEnabled(false);
        orasProgress->setValue(100);
        timer->deleteLater();
    });
    timer->start(200);
}

void IDs6::useBaseSeed(const QModelIndex &index)
{
    if (!index.isValid())
    {
        return;
    }
    orasBase->setText(baseModel->item(index.row(), 0)->text());
    fillFromLastLaunch();
    if (!orasTid->text().isEmpty())
    {
        identifyLaunch();
    }
}

void IDs6::identifyLaunch()
{
    if (orasBase->text().isEmpty())
    {
        QMessageBox msg(QMessageBox::Warning, tr("No base seed"), tr("Base seed: select one on tab 1 or type it."));
        msg.exec();
        return;
    }
    if (orasTid->text().isEmpty())
    {
        QMessageBox msg(QMessageBox::Warning, tr("No TID"), tr("TID Obtained: the TID of the new game."));
        msg.exec();
        return;
    }
    u32 expected = orasBase->getUInt() + orasLaunchMs->getUInt();
    auto found = OrasTid::identify(expected, orasTid->getUShort(), orasNo->getUInt(), orasTolerance->getUInt());
    identModel->clear();
    identModel->setHorizontalHeaderLabels({ tr("Seed"), tr("Off (ms)"), tr("TID"), tr("SID"), tr("TSV"), tr("TRV") });
    for (const auto &f : found)
    {
        identModel->appendRow({ new QStandardItem(QString::number(f.seed, 16).toUpper().rightJustified(8, '0')),
                                new QStandardItem(QString::number(f.offset)), new QStandardItem(QString::number(f.ids.tid)),
                                new QStandardItem(QString::number(f.ids.sid)), new QStandardItem(QString::number(f.ids.tsv)),
                                new QStandardItem(QString::number(f.ids.trv)) });
    }
}

void IDs6::fillFromLastLaunch()
{
    // The SID Finder starts from the latest launch with a TID (the new game to keep is launched last)
    for (int row = launches->rowCount() - 1; row >= 0; row--)
    {
        QTableWidgetItem *t = launches->item(row, 1);
        if (t && !t->text().trimmed().isEmpty())
        {
            QTableWidgetItem *m = launches->item(row, 0);
            orasLaunchMs->setText(m ? m->text().trimmed() : "0");
            orasTid->setText(t->text().trimmed());
            return;
        }
    }
}
