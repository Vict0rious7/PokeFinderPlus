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
 * PokeFinderPlus: new file, Gen 6 (XY, ORAS) static window, adapted from Static8.
 */

#include "Static6.hpp"
#include "ui_Static6.h"
#include <Core/Enum/Lead.hpp>
#include <Core/Gen6/Encounters6.hpp>
#include <Core/Gen6/Generators/StaticGenerator6.hpp>
#include <Core/Gen6/Profile6.hpp>
#include <Core/Gen6/StaticTemplate6.hpp>
#include <Core/Parents/ProfileLoader.hpp>
#include <Core/Util/Translator.hpp>
#include <Form/Controls/Controls.hpp>
#include <Form/Gen6/Profile/ProfileManager6.hpp>
#include <Model/Gen6/StaticModel6.hpp>
#include <QMessageBox>
#include <QSettings>

static const QString settingPrefix = QStringLiteral("static6");

Static6::Static6(QWidget *parent) : QWidget(parent), ui(new Ui::Static6), shownVersion(Game::None)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);

    ui->profileDisplay->setup(settingPrefix, Game::Gen6);

    model = new StaticModel6(ui->tableView);
    ui->tableView->setModel(model);

    ui->textBoxSeed->setValues(InputType::Seed32Bit);
    ui->textBoxInitialAdvances->setValues(InputType::Advance32Bit);
    ui->textBoxMaxAdvances->setValues(InputType::Advance32Bit);
    ui->textBoxOffset->setValues(InputType::Advance32Bit);

    ui->comboMenuLead->addAction(tr("None"), toInt(Lead::None));
    ui->comboMenuLead->addMenu(tr("Synchronize"), Translator::getNatures());

    ui->comboBoxShiny->setup({ toInt(Shiny::Never), toInt(Shiny::Random) });
    ui->comboBoxAbility->setup({ 0, 1, 2, 255 });

    // Read-only template information, as in Static8
    ui->spinBoxLevel->setEnabled(false);
    ui->comboBoxAbility->setEnabled(false);
    ui->comboBoxShiny->setEnabled(false);
    ui->spinBoxIVCount->setEnabled(false);

    ui->filter->disableControls(Controls::Wild | Controls::Height | Controls::Weight);

    connect(ui->profileDisplay, &ProfileDisplay6::profileChanged, this, &Static6::profileChanged);
    connect(ui->profileDisplay, &ProfileDisplay6::profilesChanged, this, &Static6::profilesChanged);
    connect(ui->pushButtonGenerate, &QPushButton::clicked, this, &Static6::generate);
    connect(ui->comboBoxCategory, &QComboBox::currentIndexChanged, this, &Static6::categoryIndexChanged);
    connect(ui->comboBoxPokemon, &QComboBox::currentIndexChanged, this, &Static6::pokemonIndexChanged);
    connect(ui->filter, &Filter::showStatsChanged, model, &StaticModel6::setShowStats);

    updateProfiles();

    QSettings setting;
    setting.beginGroup(settingPrefix);
    if (setting.contains("geometry"))
    {
        this->restoreGeometry(setting.value("geometry").toByteArray());
    }
    setting.endGroup();
}

Static6::~Static6()
{
    QSettings setting;
    setting.beginGroup(settingPrefix);
    setting.setValue("geometry", this->saveGeometry());
    setting.endGroup();

    delete ui;
}

void Static6::updateProfiles()
{
    ui->profileDisplay->updateProfiles();
}

void Static6::categoryIndexChanged(int index)
{
    if (index >= 0)
    {
        int size;
        const StaticTemplate6 *templates = Encounters6::getStaticEncounters(currentProfile->getVersion(), index, &size);

        ui->comboBoxPokemon->clear();
        for (int i = 0; i < size; i++)
        {
            QString name = QString::fromStdString(Translator::getSpecie(templates[i].getSpecie(), templates[i].getForm()));
            if (templates[i].getUnstable())
            {
                name += " (?)"; // 3DSRNGTool: the delay is not stable
            }
            ui->comboBoxPokemon->addItem(name, i);
        }
    }
}

void Static6::generate()
{
    if (!ui->filter->isValid())
    {
        return;
    }

    const StaticTemplate6 *staticTemplate = Encounters6::getStaticEncounter(
        currentProfile->getVersion(), ui->comboBoxCategory->currentIndex(), ui->comboBoxPokemon->getCurrentInt());
    if (staticTemplate == nullptr)
    {
        return;
    }

    model->clearModel();

    u32 seed = ui->textBoxSeed->getUInt();
    u32 initialAdvances = ui->textBoxInitialAdvances->getUInt();
    u32 maxAdvances = ui->textBoxMaxAdvances->getUInt();
    u32 offset = ui->textBoxOffset->getUInt();
    auto lead = ui->comboMenuLead->getEnum<Lead>();

    auto filter = ui->filter->getFilter<StateFilter>();
    StaticGenerator6 generator(initialAdvances, maxAdvances, offset, lead, ui->checkBoxAssumeSynced->isChecked(), *staticTemplate,
                               *currentProfile, filter);

    auto states = generator.generate(seed);
    model->addItems(states);
}

void Static6::pokemonIndexChanged(int index)
{
    if (index >= 0)
    {
        const StaticTemplate6 *staticTemplate = Encounters6::getStaticEncounter(
            currentProfile->getVersion(), ui->comboBoxCategory->currentIndex(), ui->comboBoxPokemon->getCurrentInt());
        if (staticTemplate == nullptr)
        {
            return;
        }
        ui->spinBoxLevel->setValue(staticTemplate->getLevel());
        // 3DSRNGTool ability 0 = random, 1, 2, 3 = hidden -> combo values 255, 0, 1, 2
        u8 ability = staticTemplate->getAbility();
        ui->comboBoxAbility->setCurrentIndex(ui->comboBoxAbility->findData(ability == 0 ? 255 : ability - 1));
        ui->comboBoxShiny->setCurrentIndex(ui->comboBoxShiny->findData(toInt(staticTemplate->getShiny())));
        ui->spinBoxIVCount->setValue(staticTemplate->getIVCount());
        ui->textBoxOffset->setText(QString::number(staticTemplate->getDelay()));

        // Synchronize only where 3DSRNGTool allows it; a fixed nature or a gift always uses its own rule
        ui->comboMenuLead->setEnabled(staticTemplate->getSyncable() && staticTemplate->getNature() >= 25);
        ui->checkBoxAssumeSynced->setEnabled(!staticTemplate->getAlwaysSync());
    }
}

void Static6::profileChanged(const Profile6 &profile)
{
    currentProfile = &profile;

    // XY and ORAS have different categories; X and Y (OR and AS) have different Pokemon
    Game version = profile.getVersion();
    if (version != shownVersion)
    {
        bool sameFamily = ((version & Game::XY) != Game::None) == ((shownVersion & Game::XY) != Game::None) && shownVersion != Game::None;
        int category = ui->comboBoxCategory->currentIndex();
        shownVersion = version;

        ui->comboBoxCategory->blockSignals(true);
        ui->comboBoxCategory->clear();
        for (const auto &name : Encounters6::getStaticCategories(version))
        {
            ui->comboBoxCategory->addItem(QString::fromStdString(name));
        }
        ui->comboBoxCategory->setCurrentIndex(sameFamily && category >= 0 ? category : 0);
        ui->comboBoxCategory->blockSignals(false);
        categoryIndexChanged(ui->comboBoxCategory->currentIndex());
    }
}
