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
#include <Core/Gen6/XYTid.hpp>
#include <Form/Controls/TableView.hpp>
#include <Form/Controls/TextBox.hpp>
#include <Model/Gen6/BlinkModel6.hpp>
#include <Model/Gen6/IDModel6.hpp>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
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
#include <QSpinBox>
#include <QTabWidget>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

namespace
{
    QSpinBox *spin(int min, int max, int value, const QString &name)
    {
        auto *box = new QSpinBox;
        box->setRange(min, max);
        box->setValue(value);
        box->setObjectName(name);
        return box;
    }

    QWidget *range(QSpinBox *min, QSpinBox *max)
    {
        auto *widget = new QWidget;
        auto *row = new QHBoxLayout(widget);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(min);
        row->addWidget(new QLabel(QObject::tr("to")));
        row->addWidget(max);
        row->addStretch();
        return widget;
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
    setWindowTitle(tr("IDs (X / Y)"));

    auto *layout = new QVBoxLayout(this);
    auto *top = new QFormLayout;
    game = new QComboBox;
    game->addItems({ tr("X"), tr("Y") });
    game->setObjectName("comboBoxGame");
    top->addRow(tr("Game"), game);
    layout->addLayout(top);

    tabs = new QTabWidget;
    layout->addWidget(tabs, 1);

    // ---- Seed (blinks) ----
    auto *seedPage = new QWidget;
    seedPage->setObjectName("seedPage");
    seedPage->setFocusPolicy(Qt::ClickFocus); // a click on the page takes Space back from the text fields
    auto *seedLayout = new QVBoxLayout(seedPage);

    auto *taps = new QGroupBox(tr("Professor's blinks"));
    auto *tapsLayout = new QGridLayout(taps);
    blink = new QPushButton(tr("Blink  (Space)"));
    blink->setObjectName("pushButtonBlink");
    blink->setMinimumHeight(48);
    blink->setFocusPolicy(Qt::NoFocus);
    clear = new QPushButton(tr("Clear"));
    clear->setFocusPolicy(Qt::NoFocus);
    tapStatus = new QLabel(tr("No taps yet"));
    tapStatus->setObjectName("labelTapStatus");
    gaps = new QPlainTextEdit;
    gaps->setObjectName("plainTextEditGaps");
    gaps->setPlaceholderText(tr("Frames between blinks"));
    gaps->setMaximumWidth(220);
    tapsLayout->addWidget(blink, 0, 0, 1, 2);
    tapsLayout->addWidget(clear, 1, 0);
    tapsLayout->addWidget(tapStatus, 1, 1);
    tapsLayout->addWidget(gaps, 0, 2, 3, 1);
    seedLayout->addWidget(taps);

    auto *settings = new QGroupBox(tr("Search"));
    auto *form = new QFormLayout(settings);
    advanceMin = spin(0, 10000, XYTid::DEFAULT_ADVANCE_MIN, "spinBoxAdvanceMin");
    advanceMax = spin(0, 10000, XYTid::DEFAULT_ADVANCE_MAX, "spinBoxAdvanceMax");
    form->addRow(tr("First blink's advance"), range(advanceMin, advanceMax));
    auto *modes = new QWidget;
    auto *modeRow = new QHBoxLayout(modes);
    modeRow->setContentsMargins(0, 0, 0, 0);
    all = new QRadioButton(tr("All seeds"));
    near = new QRadioButton(tr("Near base seed"));
    near->setObjectName("radioButtonNear");
    all->setChecked(true);
    modeRow->addWidget(all);
    modeRow->addWidget(near);
    modeRow->addStretch();
    form->addRow(tr("Seeds"), modes);
    auto *baseRow = new QWidget;
    auto *baseLayout = new QHBoxLayout(baseRow);
    baseLayout->setContentsMargins(0, 0, 0, 0);
    baseSeed = new TextBox;
    baseSeed->setObjectName("textBoxBaseSeed");
    baseSeed->setValues(InputType::Seed32Bit);
    hours = spin(1, 1193, 18, "spinBoxHours");
    hours->setSuffix(tr(" h"));
    baseLayout->addWidget(baseSeed);
    baseLayout->addWidget(new QLabel(tr("up to")));
    baseLayout->addWidget(hours);
    baseLayout->addStretch();
    form->addRow(tr("Base seed"), baseRow);
    seedLayout->addWidget(settings);

    auto *buttons = new QHBoxLayout;
    search = new QPushButton(tr("Search"));
    search->setObjectName("pushButtonSearch");
    cancel = new QPushButton(tr("Cancel"));
    cancel->setEnabled(false);
    progress = new QProgressBar;
    progress->setRange(0, 100);
    progress->setValue(0);
    buttons->addWidget(search);
    buttons->addWidget(cancel);
    buttons->addWidget(progress, 1);
    seedLayout->addLayout(buttons);

    seedModel = new BlinkModel6(this);
    seedTable = new TableView;
    seedTable->setObjectName("tableViewSeeds");
    seedTable->setModel(seedModel);
    seedLayout->addWidget(seedTable, 1);
    tabs->addTab(seedPage, tr("1. Seed from blinks"));

    // ---- IDs ----
    auto *idPage = new QWidget;
    auto *idLayout = new QVBoxLayout(idPage);
    auto *idSettings = new QGroupBox(tr("IDs"));
    auto *idForm = new QFormLayout(idSettings);
    state = new QLineEdit;
    state->setObjectName("lineEditState");
    idForm->addRow(tr("State at first tap"), state);
    firstTap = new QLineEdit;
    firstTap->setObjectName("lineEditFirstTap");
    firstTap->setPlaceholderText(tr("HH:mm:ss.zzz"));
    idForm->addRow(tr("First tap"), firstTap);
    idMin = spin(0, 1000000, 0, "spinBoxIDMin");
    idMax = spin(0, 1000000, 5000, "spinBoxIDMax");
    idForm->addRow(tr("Advances"), range(idMin, idMax));
    auto *filters = new QWidget;
    auto *filterRow = new QHBoxLayout(filters);
    filterRow->setContentsMargins(0, 0, 0, 0);
    const QStringList names = { tr("TID"), tr("SID"), tr("TSV"), tr("TRV") };
    const int maxima[] = { 65535, 65535, 4095, 15 };
    for (int i = 0; i < 4; i++)
    {
        filterOn[i] = new QCheckBox(names[i]);
        filterOn[i]->setObjectName("checkBox" + names[i]);
        filterValue[i] = spin(0, maxima[i], 0, "spinBox" + names[i]);
        filterValue[i]->setEnabled(false);
        connect(filterOn[i], &QCheckBox::toggled, filterValue[i], &QSpinBox::setEnabled);
        filterRow->addWidget(filterOn[i]);
        filterRow->addWidget(filterValue[i]);
    }
    filterRow->addStretch();
    idForm->addRow(tr("Filters"), filters);
    generate = new QPushButton(tr("Generate"));
    generate->setObjectName("pushButtonGenerate");
    idForm->addRow(generate);
    idLayout->addWidget(idSettings);

    idModel = new IDModel6(this);
    idTable = new TableView;
    idTable->setObjectName("tableViewIDs");
    idTable->setModel(idModel);
    idLayout->addWidget(idTable, 1);

    auto *sendRow = new QHBoxLayout;
    send = new QPushButton(tr("Send to PokeTimerPlus"));
    send->setObjectName("pushButtonSend");
    copy = new QPushButton(tr("Copy for PokeTimerPlus"));
    copy->setObjectName("pushButtonCopy");
    sendRow->addWidget(send);
    sendRow->addWidget(copy);
    sendRow->addStretch();
    idLayout->addLayout(sendRow);
    tabs->addTab(idPage, tr("2. IDs"));

    auto *space = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(space, &QShortcut::activated, this, &IDs6::tap);
    connect(blink, &QPushButton::pressed, this, &IDs6::tap); // on press, not release: closer to the blink
    connect(clear, &QPushButton::clicked, this, &IDs6::clearTaps);
    connect(search, &QPushButton::clicked, this, &IDs6::startSearch);
    connect(seedTable->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &IDs6::useSeed);
    connect(generate, &QPushButton::clicked, this, &IDs6::generateIDs);
    connect(send, &QPushButton::clicked, this, &IDs6::sendToTimer);
    connect(copy, &QPushButton::clicked, this, &IDs6::copyForTimer);

    QSettings setting;
    game->setCurrentIndex(setting.value("ids6/game", 0).toInt());
    if (setting.contains("ids6/geometry"))
    {
        restoreGeometry(setting.value("ids6/geometry").toByteArray());
    }
    seedPage->setFocus(); // Space taps (a focused text field would take it)
}

IDs6::~IDs6()
{
    if (running)
    {
        running->cancelSearch();
        delete running; // joins the threads
    }
    QSettings setting;
    setting.setValue("ids6/game", game->currentIndex());
    setting.setValue("ids6/geometry", saveGeometry());
}

void IDs6::tap()
{
    if (tabs->currentIndex() != 0)
    {
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
    u32 count = static_cast<u32>(std::min<u64>(static_cast<u64>(hours->value()) * 3600000, 0xffffffffull));
    auto *searcher = new BlinkSearcher6();
    if (!searcher->startSearch(threads, list, advanceMin->value(), advanceMax->value(), base, count, nearBase))
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
    if (idMin->value() > idMax->value())
    {
        return;
    }
    IDFilter6 filter;
    if (filterOn[0]->isChecked())
    {
        filter.tid = filterValue[0]->value();
    }
    if (filterOn[1]->isChecked())
    {
        filter.sid = filterValue[1]->value();
    }
    if (filterOn[2]->isChecked())
    {
        filter.tsv = filterValue[2]->value();
    }
    if (filterOn[3]->isChecked())
    {
        filter.trv = filterValue[3]->value();
    }
    idModel->clearModel();
    idModel->addItems(generateIDs6(words, idMin->value(), idMax->value(), filter));
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
    handoff.game = game->currentIndex() == 0 ? 'X' : 'Y';
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
