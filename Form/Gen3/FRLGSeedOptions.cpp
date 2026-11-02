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

#include "FRLGSeedOptions.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QSettings>
#include <QSpinBox>

namespace
{
    // Ten Lines' labels, in its menu order
    const std::vector<std::pair<QString, std::string>> GAMES = { { "FireRed (ENG)", "fr" },
                                                                 { "FireRed (SPA/FRE/ITA/GER)", "fr_eu" },
                                                                 { "FireRed (JPN) (1.0)", "fr_jpn_1_0" },
                                                                 { "FireRed (JPN) (1.1)", "fr_jpn_1_1" },
                                                                 { "Switch FireRed (ENG/SPA/FRE/ITA/GER)", "fr_nx" },
                                                                 { "Switch FireRed (JPN)", "fr_jpn_nx" },
                                                                 { "FireRed (ENG) (MGBA 10.5)", "fr_mgba" },
                                                                 { "LeafGreen (ENG)", "lg" },
                                                                 { "LeafGreen (SPA/FRE/ITA/GER)", "lg_eu" },
                                                                 { "LeafGreen (JPN)", "lg_jpn" },
                                                                 { "Switch LeafGreen (ENG/SPA/FRE/ITA/GER)", "lg_nx" },
                                                                 { "Switch LeafGreen (JPN)", "lg_jpn_nx" },
                                                                 { "LeafGreen (ENG) (MGBA 10.5)", "lg_mgba" } };

    QComboBox *combo(QWidget *parent, const std::vector<std::pair<QString, QString>> &items)
    {
        auto *box = new QComboBox(parent);
        for (const auto &[text, data] : items)
        {
            box->addItem(text, data);
        }
        return box;
    }

    std::string value(const QComboBox *box)
    {
        return box->currentData().toString().toStdString();
    }
}

FRLGSeedOptions::FRLGSeedOptions(QWidget *parent) : QGroupBox(tr("FRLG initial seed (Ten Lines data)"), parent)
{
    game = new QComboBox(this);
    for (const auto &[text, key] : GAMES)
    {
        game->addItem(text, QString::fromStdString(key));
    }
    console = new QComboBox(this);
    sound = combo(this, { { tr("Any"), "" }, { tr("Mono"), "mono" }, { tr("Stereo"), "stereo" } });
    buttonMode = combo(this, { { tr("Any"), "" }, { "L=A", "a" }, { tr("Help"), "h" }, { "LR", "r" } });
    button = combo(this, { { tr("Any"), "" }, { "A", "a" }, { "Start", "start" }, { "L", "l" } });
    held = combo(this,
                 { { tr("Any"), "" },
                   { tr("None"), "none" },
                   { tr("Startup Select"), "startup_select" },
                   { tr("Startup A"), "startup_a" },
                   { tr("Blackout R"), "blackout_r" },
                   { tr("Blackout A"), "blackout_a" },
                   { tr("Blackout L"), "blackout_l" },
                   { tr("Blackout A+L"), "blackout_al" } });
    teachyTV = new QCheckBox(tr("Teachy TV"), this);
    regularOut = new QSpinBox(this);
    regularOut->setRange(0, 1000000);
    regularOut->setValue(3600);
    regularOut->setEnabled(false);
    regularOut->setToolTip(tr("Teachy TV mode: minimum regular frames (Ten Lines default 3600)"));

    status = new QLabel(this);
    status->setWordWrap(true);
    QString stamp = QString::fromStdString(FRLGInitialSeed::dataTimestamp());
    status->setText(tr("Best reachable initial seed per result, as in Ten Lines (github.com/Lincoln-LM/ten-lines). FRLG seeds farmed by "
                       "blisy, po, HunarPG, 10Ben, Real96, ColdStoneSys, Papa Jefé, and トノ.%1")
                        .arg(stamp.isEmpty() ? QString() : tr(" Data as of %1.").arg(stamp)));

    auto *layout = new QGridLayout(this);
    QStringList labels = { tr("Game"), tr("Console"), tr("Sound"), tr("Button Mode"), tr("Seed Button"), tr("Extra Button") };
    QList<QWidget *> widgets = { game, console, sound, buttonMode, button, held };
    for (int i = 0; i < labels.size(); i++)
    {
        layout->addWidget(new QLabel(labels[i], this), 0, i);
        layout->addWidget(widgets[i], 1, i);
    }
    layout->addWidget(teachyTV, 0, labels.size());
    layout->addWidget(regularOut, 1, labels.size());
    layout->addWidget(status, 2, 0, 1, labels.size() + 1);

    QSettings setting;
    setting.beginGroup("frlgSeedOptions");
    if (setting.contains("game"))
    {
        game->setCurrentIndex(std::max(0, game->findData(setting.value("game"))));
    }
    gameChanged();
    console->setCurrentIndex(std::max(0, console->findData(setting.value("console", "GBA"))));
    sound->setCurrentIndex(std::max(0, sound->findData(setting.value("sound", ""))));
    buttonMode->setCurrentIndex(std::max(0, buttonMode->findData(setting.value("buttonMode", ""))));
    button->setCurrentIndex(std::max(0, button->findData(setting.value("button", ""))));
    held->setCurrentIndex(std::max(0, held->findData(setting.value("held", ""))));
    teachyTV->setChecked(setting.value("teachyTV", false).toBool());
    regularOut->setValue(setting.value("regularOut", 3600).toInt());
    regularOut->setEnabled(teachyTV->isChecked());
    setting.endGroup();

    connect(game, &QComboBox::currentIndexChanged, this, &FRLGSeedOptions::gameChanged);
    for (auto *box : { console, sound, buttonMode, button, held })
    {
        connect(box, &QComboBox::currentIndexChanged, this, &FRLGSeedOptions::update);
    }
    connect(teachyTV, &QCheckBox::toggled, regularOut, &QSpinBox::setEnabled);
    connect(teachyTV, &QCheckBox::toggled, this, &FRLGSeedOptions::update);
    connect(regularOut, &QSpinBox::editingFinished, this, &FRLGSeedOptions::update);

    setVisible(false);
}

FRLGSeedOptions::~FRLGSeedOptions()
{
    QSettings setting;
    setting.beginGroup("frlgSeedOptions");
    setting.setValue("game", game->currentData());
    setting.setValue("console", console->currentData());
    setting.setValue("sound", sound->currentData());
    setting.setValue("buttonMode", buttonMode->currentData());
    setting.setValue("button", button->currentData());
    setting.setValue("held", held->currentData());
    setting.setValue("teachyTV", teachyTV->isChecked());
    setting.setValue("regularOut", regularOut->value());
    setting.endGroup();
}

void FRLGSeedOptions::setProfile(bool frlg, bool leafGreen)
{
    this->frlg = frlg;
    QSettings setting;
    if (frlg && !setting.contains("frlgSeedOptions/game"))
    {
        game->setCurrentIndex(game->findData(leafGreen ? "lg" : "fr"));
    }
    else if (frlg)
    {
        // keep the chosen language/console, but follow the profile's game
        QString key = game->currentData().toString();
        QString wanted = (leafGreen ? "lg" : "fr") + key.mid(2);
        int index = game->findData(wanted);
        if (index != -1 && wanted != key)
        {
            game->setCurrentIndex(index);
        }
    }
    setVisible(frlg);
    update();
}

FRLGSeedSettings FRLGSeedOptions::getSettings() const
{
    FRLGSeedSettings settings;
    settings.gameVersion = value(game);
    settings.enabled = frlg && FRLGInitialSeed::hasData(settings.gameVersion);
    settings.console = console->currentData().toString().toStdString();
    settings.filter = { value(sound), value(buttonMode), value(button), value(held) };
    settings.teachyTV = teachyTV->isChecked();
    settings.regularOut = regularOut->value();
    return settings;
}

void FRLGSeedOptions::gameChanged()
{
    // Ten Lines' fixGameConsole: Switch games use the Switch consoles, the others the GBA ones
    QString previous = console->currentData().toString();
    bool nx = game->currentData().toString().endsWith("nx");
    console->blockSignals(true);
    console->clear();
    if (nx)
    {
        console->addItem(tr("Nintendo Switch 1"), "NX");
        console->addItem(tr("Nintendo Switch 2"), "NX2");
    }
    else
    {
        console->addItem(tr("Game Boy Advance"), "GBA");
        console->addItem(tr("Game Boy Player"), "GBP");
        console->addItem(tr("Nintendo DS"), "NDS");
        console->addItem(tr("Nintendo 3DS (open_agb_firm)"), "3DS");
    }
    console->setCurrentIndex(std::max(0, console->findData(previous)));
    console->blockSignals(false);
    update();
}

void FRLGSeedOptions::update()
{
    if (frlg && !FRLGInitialSeed::hasData(value(game)))
    {
        setTitle(tr("FRLG initial seed (Ten Lines data): no seed data bundled for this game"));
    }
    else
    {
        setTitle(tr("FRLG initial seed (Ten Lines data)"));
    }
    emit settingsChanged(getSettings());
}
