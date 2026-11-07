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

#include "SeedFinder6.hpp"
#include <Core/Gen6/Searchers/SeedSearcher6.hpp>
#include <Form/Controls/TableView.hpp>
#include <Form/Controls/TextBox.hpp>
#include <Model/Gen6/SeedModel6.hpp>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
    const QStringList statNames
        = { QObject::tr("HP"), QObject::tr("Atk"), QObject::tr("Def"), QObject::tr("SpA"), QObject::tr("SpD"), QObject::tr("Spe") };

    QSpinBox *spin(int min, int max, int value)
    {
        auto *box = new QSpinBox;
        box->setRange(min, max);
        box->setValue(value);
        return box;
    }

    /// One row of 6 IV boxes with stat labels above
    QWidget *ivRow(std::array<QSpinBox *, 6> &boxes, int value, const QString &name)
    {
        auto *widget = new QWidget;
        auto *grid = new QGridLayout(widget);
        grid->setContentsMargins(0, 0, 0, 0);
        for (int i = 0; i < 6; i++)
        {
            grid->addWidget(new QLabel(statNames[i]), 0, i, Qt::AlignHCenter);
            boxes[i] = spin(0, 31, value);
            boxes[i]->setObjectName(QString("%1%2").arg(name).arg(i));
            grid->addWidget(boxes[i], 1, i);
        }
        return widget;
    }

    template <size_t N>
    std::array<u8, 6> values(const std::array<QSpinBox *, N> &boxes)
    {
        std::array<u8, 6> ivs;
        for (int i = 0; i < 6; i++)
        {
            ivs[i] = static_cast<u8>(boxes[i]->value());
        }
        return ivs;
    }

    QWidget *range(QSpinBox *&min, QSpinBox *&max)
    {
        auto *widget = new QWidget;
        auto *row = new QHBoxLayout(widget);
        row->setContentsMargins(0, 0, 0, 0);
        min = spin(0, SeedSearcher6::MAX_FRAME, 0);
        max = spin(0, SeedSearcher6::MAX_FRAME, 0);
        row->addWidget(min);
        row->addWidget(new QLabel(QObject::tr("to")));
        row->addWidget(max);
        row->addStretch();
        return widget;
    }
}

SeedFinder6::SeedFinder6(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Seed Finder Gen 6"));

    auto *layout = new QVBoxLayout(this);

    auto *settings = new QGroupBox(tr("Settings"));
    auto *settingsForm = new QFormLayout(settings);
    game = new QComboBox;
    game->addItems({ tr("X / Y"), tr("Omega Ruby / Alpha Sapphire") });
    settingsForm->addRow(tr("Game"), game);
    auto *modes = new QWidget;
    auto *modeRow = new QHBoxLayout(modes);
    modeRow->setContentsMargins(0, 0, 0, 0);
    full = new QRadioButton(tr("Full search (two Pokemon, all seeds)"));
    quick = new QRadioButton(tr("Quick search (one Pokemon, from a base seed)"));
    quick->setObjectName("radioButtonQuick");
    full->setChecked(true);
    modeRow->addWidget(full);
    modeRow->addWidget(quick);
    settingsForm->addRow(tr("Search"), modes);
    layout->addWidget(settings);

    pages = new QStackedWidget;

    auto *fullPage = new QGroupBox(tr("Pokemon received right after boot (exact IVs)"));
    auto *fullForm = new QFormLayout(fullPage);
    fullForm->addRow(tr("Pokemon 1 IVs"), ivRow(ivs1, 0, "spinBoxIV1_"));
    fullForm->addRow(tr("Pokemon 1 frames"), range(frame1Min, frame1Max));
    fullForm->addRow(tr("Pokemon 2 IVs"), ivRow(ivs2, 0, "spinBoxIV2_"));
    fullForm->addRow(tr("Pokemon 2 frames"), range(frame2Min, frame2Max));
    pages->addWidget(fullPage);

    auto *quickPage = new QGroupBox(tr("One Pokemon after a later boot"));
    auto *quickForm = new QFormLayout(quickPage);
    baseSeed = new TextBox;
    baseSeed->setObjectName("textBoxBaseSeed");
    baseSeed->setValues(InputType::Seed32Bit);
    quickForm->addRow(tr("Base seed"), baseSeed);
    quickForm->addRow(tr("Minimum IVs"), ivRow(ivMin, 0, "spinBoxIVMin"));
    quickForm->addRow(tr("Maximum IVs"), ivRow(ivMax, 31, "spinBoxIVMax"));
    quickForm->addRow(tr("Frames"), range(quickMin, quickMax));
    hours = spin(1, 71, 18);
    hours->setObjectName("spinBoxHours");
    hours->setSuffix(tr(" h"));
    hours->setToolTip(tr("How long after the base seed's boot to look (1 seed per millisecond). Gen 6 GPU Tools uses about 18.6 h."));
    quickForm->addRow(tr("Search up to"), hours);
    pages->addWidget(quickPage);
    layout->addWidget(pages);

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
    layout->addLayout(buttons);

    model = new SeedModel6(this);
    table = new TableView;
    table->setModel(model);
    layout->addWidget(table, 1);

    auto *note
        = new QLabel(tr("Method of retailrng's XY/ORAS guide: save in front of a gift (fossils), soft reset, start a count-up timer on the "
                        "title screen, receive two Pokemon and find their exact IVs. Frames as in poke6-seed-finder."));
    note->setWordWrap(true);
    layout->addWidget(note);

    connect(game, &QComboBox::currentIndexChanged, this, &SeedFinder6::gameChanged);
    connect(full, &QRadioButton::toggled, this, [this](bool checked) { pages->setCurrentIndex(checked ? 0 : 1); });
    connect(search, &QPushButton::clicked, this, &SeedFinder6::startSearch);

    QSettings setting;
    game->setCurrentIndex(setting.value("seedFinder6/game", 0).toInt());
    gameChanged(game->currentIndex());
    if (setting.contains("seedFinder6/geometry"))
    {
        restoreGeometry(setting.value("seedFinder6/geometry").toByteArray());
    }
}

SeedFinder6::~SeedFinder6()
{
    if (running)
    {
        running->cancelSearch();
        delete running; // joins the threads
    }
    QSettings setting;
    setting.setValue("seedFinder6/game", game->currentIndex());
    setting.setValue("seedFinder6/geometry", saveGeometry());
}

void SeedFinder6::gameChanged(int index)
{
    // retailrng's ranges (Basic Shiny Manip): XY 750-900 then 1500-1900, ORAS 600-800 then 1500-1700
    bool xy = index == 0;
    frame1Min->setValue(xy ? 750 : 600);
    frame1Max->setValue(xy ? 900 : 800);
    frame2Min->setValue(1500);
    frame2Max->setValue(xy ? 1900 : 1700);
    quickMin->setValue(xy ? 750 : 600);
    quickMax->setValue(xy ? 900 : 800);
}

void SeedFinder6::startSearch()
{
    QSettings setting;
    int threads = setting.value("settings/threads", QThread::idealThreadCount()).toInt();

    if (running)
    {
        return;
    }
    auto *searcher = new SeedSearcher6();
    bool ok;
    u32 base = baseSeed->getUInt();
    if (full->isChecked())
    {
        ok = searcher->startFullSearch(threads, values(ivs1), { frame1Min->value(), frame1Max->value() }, values(ivs2),
                                       { frame2Min->value(), frame2Max->value() });
    }
    else
    {
        u32 count = static_cast<u32>(std::min<u64>(static_cast<u64>(hours->value()) * 3600000, 0xffffffffull - base + 1));
        ok = searcher->startQuickSearch(threads, base, count, values(ivMin), values(ivMax), { quickMin->value(), quickMax->value() });
    }
    if (!ok)
    {
        delete searcher;
        QMessageBox msg(
            QMessageBox::Warning, tr("Invalid settings"),
            tr("Check the ranges: minimum <= maximum, frames <= 3000, and Pokemon 2's frames start at least 6 after Pokemon 1's."));
        msg.exec();
        return;
    }

    running = searcher;
    model->clearModel();
    model->setQuick(quick->isChecked(), base);
    search->setEnabled(false);
    cancel->setEnabled(true);
    progress->setValue(0);

    auto *timer = new QTimer(this);
    connect(cancel, &QPushButton::clicked, timer, [this, searcher] {
        searcher->cancelSearch();
        cancel->setEnabled(false);
    });
    connect(timer, &QTimer::timeout, this, [this, searcher, timer] {
        model->addItems(searcher->getResults());
        progress->setValue(searcher->getProgress());
        if (!searcher->isSearching())
        {
            timer->stop();
            model->addItems(searcher->getResults());
            delete searcher;
            running = nullptr;
            search->setEnabled(true);
            cancel->setEnabled(false);
            timer->deleteLater();
        }
    });
    timer->start(500);
}
