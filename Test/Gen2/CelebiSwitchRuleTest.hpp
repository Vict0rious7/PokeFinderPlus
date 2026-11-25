/*
 * PokeFinderPlus: new file. Checks Core/Gen2/CelebiSwitchRule against the prototype (golden2/switch.json, made by
 * Test/Gen2/tools/make_goldens.py switch). ROM-free (grids in the golden data):
 *  - the 68 advice steps of the v0.6 harness blocks: same advice, timer move, misfits, P(F in plan 1 / 2 set) and P(r);
 *  - Victor's 13 retail calibration attempts all fit, P(r = 0) >= 0.99 (realcheck.py).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#ifndef CELEBISWITCHRULETEST_HPP
#define CELEBISWITCHRULETEST_HPP

#include <QObject>

class CelebiSwitchRuleTest : public QObject
{
    Q_OBJECT
private slots:
    void errModels();
    void harnessSteps();
    void retail();
};

#endif // CELEBISWITCHRULETEST_HPP
