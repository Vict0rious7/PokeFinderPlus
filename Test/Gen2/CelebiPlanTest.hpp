/*
 * PokeFinderPlus: new file. Checks Core/Gen2/CelebiPlan against the prototype (golden2/plan.json, made by
 * Test/Gen2/tools/make_goldens.py plan): plan discovery reproduces the five plan_<lang>_lp.json and the EN test save's
 * plan; the timer phases of plans 1 and 2 (FR plan 1: 395 / 280 / 527 / 127 / 628); timeline helpers.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#ifndef CELEBIPLANTEST_HPP
#define CELEBIPLANTEST_HPP

#include <QObject>

class CelebiPlanTest : public QObject
{
    Q_OBJECT
private slots:
    void timelineHelpers();
    void discoverPlans();
    void timerPhases();
};

#endif // CELEBIPLANTEST_HPP
