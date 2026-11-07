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
 * Modified 2026-11-07 by Vict0rious7 for PokeFinderPlus (unofficial fork):
 * copying a selection is one pass in table order; columns resize once per batch of new rows.
 */

#include "TableView.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QSettings>
#include <QTimer>
#include <algorithm>

TableView::TableView(QWidget *parent) : QTableView(parent)
{
    auto *outputTXT = addAction(tr("Output Results to TXT"));
    auto *outputCSV = addAction(tr("Output Results to CSV"));

    connect(outputTXT, &QAction::triggered, this, [this] { outputModel(); });
    connect(outputCSV, &QAction::triggered, this, [this] { outputModel(true); });

    QHeaderView *horizontal = this->horizontalHeader();
    horizontal->setSectionResizeMode(QHeaderView::Interactive);

    QHeaderView *vertical = this->verticalHeader();
    vertical->hide();

    QTimer::singleShot(200, this, [horizontal] {
        QSettings setting;
        horizontal->resizeSections(setting.value("settings/headerSize").value<QHeaderView::ResizeMode>());
    });
}

void TableView::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);
    // PokeFinderPlus: resize once per batch of inserted rows, not once per row. A sorted proxy (FRLG searchers, sorted by total
    // time) inserts each new result at its own place, so one batch emits one rowsInserted per result; resizing to contents each
    // time measured the whole table again and again and froze the window ("not responding") a few seconds into the search.
    connect(this->model(), &QAbstractItemModel::rowsInserted, this, [this] {
        if (resizePending)
        {
            return;
        }
        resizePending = true;
        QTimer::singleShot(0, this, [this] {
            resizePending = false;
            QSettings setting;
            this->horizontalHeader()->resizeSections(setting.value("settings/headerSize").value<QHeaderView::ResizeMode>());
        });
    });
}

void TableView::contextMenuEvent(QContextMenuEvent *event)
{
    if (model()->rowCount() != 0)
    {
        QModelIndex index = indexAt(event->pos());
        if (index.isValid())
        {
            selectRow(index.row());
            setCurrentIndex(index);
        }
        QMenu::exec(actions(), event->globalPos(), nullptr, this);
    }
}

void TableView::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_C) && (event->modifiers() == Qt::ControlModifier))
    {
        setSelectionToClipBoard();
    }
    else
    {
        QTableView::keyPressEvent(event);
    }
}

void TableView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->type() == QMouseEvent::MouseButtonDblClick)
    {
        setSelectionToClipBoard();
    }
}

void TableView::outputModel(bool csv) const
{
    QString caption = tr(csv ? "Save Output to CSV" : "Save Output to TXT");
    QString filter = tr(csv ? "CSV File (*.csv);;All Files (*)" : "Text File (*.txt);;All Files (*)");

    QString fileName = QFileDialog::getSaveFileName(nullptr, caption, QDir::currentPath(), filter);

    if (fileName.isEmpty())
    {
        return;
    }

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly))
    {
        QAbstractItemModel *model = this->model();

        QTextStream ts(&file);
        int rows = model->rowCount();
        int columns = model->columnCount();

        for (int i = 0; i < columns; i++)
        {
            ts << model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
            if (i != columns - 1)
            {
                ts << (csv ? "," : "\t");
            }
        }
        ts << "\n";

        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < columns; j++)
            {
                QString entry = model->data(model->index(i, j)).toString();
                ts << (entry.isEmpty() ? "-" : entry);
                if (j != columns - 1)
                {
                    ts << (csv ? "," : "\t");
                }
            }

            if (i != rows - 1)
            {
                ts << "\n";
            }
        }
    }
}

void TableView::resizeEvent(QResizeEvent *event)
{
    QSettings setting;
    this->horizontalHeader()->resizeSections(setting.value("settings/headerSize").value<QHeaderView::ResizeMode>());
    QTableView::resizeEvent(event);
}

void TableView::setSelectionToClipBoard()
{
    // PokeFinderPlus: one pass over the selection. The loop used to call selectedIndexes() on every cell, which rebuilds the
    // whole selection list each time: copying a whole column of 100,000 results took hours and looked like a crash.
    QModelIndexList indexes = this->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty())
    {
        // Selection ranges can come in any order (e.g. Shift+click upwards): copy in table order
        std::sort(indexes.begin(), indexes.end(), [](const QModelIndex &left, const QModelIndex &right) {
            return left.row() != right.row() ? left.row() < right.row() : left.column() < right.column();
        });

        QString selectedText;
        for (qsizetype i = 0; i < indexes.size(); i++)
        {
            const QModelIndex &current = indexes[i];
            selectedText += current.data().toString();
            if (i + 1 < indexes.size())
            {
                selectedText += indexes[i + 1].row() != current.row() ? QLatin1Char('\n') : QLatin1Char('\t');
            }
        }

        QApplication::clipboard()->setText(selectedText);
    }
}
