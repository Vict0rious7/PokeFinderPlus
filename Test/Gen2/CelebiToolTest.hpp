/*
 * PokeFinderPlus: tests of the Gen 2 Celebi tool layer (Core/Gen2/CelebiTool): the shipped template (Victor's save with
 * names blanked) keeps the values that change Celebi, the tool's save and plans equal the prototype's, JSON and grid files
 * round-trip, and (opt-in, POKEFINDERPLUS_GEN2_SLOW=1 and QTEST_FUNCTION_TIMEOUT=3600000, ~17 min) Victor's retail calibration 1 through the tool's own
 * calibration path gives the prototype's rank and verdict.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#ifndef CELEBITOOLTEST_HPP
#define CELEBITOOLTEST_HPP

#include <QObject>

class CelebiToolTest : public QObject
{
    Q_OBJECT
private slots:
    void templateValues();
    void toolSaveEqualsMakesave();
    void roundTrips();
    void plansFromTemplate();
    void calibrationEndToEnd();
};

#endif // CELEBITOOLTEST_HPP
