/*
 * PokeFinderPlus: new file. Checks Core/Gen2/VcCore against the research prototype's vccore.c (golden2/core.json, made
 * by Test/Gen2/tools/make_goldens.py core): enemy bytes, VBlank count and full state hash at the encounter for 1,200
 * random timelines (5 languages), the r8_13 harness replay, and the same results from 2 threads.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#ifndef VCCORETEST_HPP
#define VCCORETEST_HPP

#include <QObject>

class VcCoreTest : public QObject
{
    Q_OBJECT
private slots:
    void goldenLoaded();
    void snapshotRestore();
    void timelines();
    void r8_13();
    void threads();
};

#endif // VCCORETEST_HPP
