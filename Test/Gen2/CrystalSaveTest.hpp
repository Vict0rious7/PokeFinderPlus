/*
 * PokeFinderPlus: new file. Checks Core/Gen2/CrystalSave against the prototype's savetool.py / savebuild.py
 * (golden2/save.json, made by Test/Gen2/tools/make_goldens.py save): 50 random minimal-value sets on 2 templates give
 * byte-identical saves (compared by hash), and read_minimal reads them back.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#ifndef CRYSTALSAVETEST_HPP
#define CRYSTALSAVETEST_HPP

#include <QObject>

class CrystalSaveTest : public QObject
{
    Q_OBJECT
private slots:
    void checksumsAndCopies();
    void readTemplates();
    void applyMinimal();
};

#endif // CRYSTALSAVETEST_HPP
