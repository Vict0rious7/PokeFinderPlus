/*
 * PokeFinderPlus: new file. Checks Core/Gen2/CelebiSession against the prototype (golden2/session.json, made by
 * Test/Gen2/tools/make_goldens.py session):
 *  - resave_step texts and session fields on the class-1 and class->=2 paths (ROM-free), the calibrate --session update;
 *  - the re-save gate on the C++ core: 6 saves x 2 re-save moments, Celebi of the re-saved file = the rebuild with the
 *    class the game uses (20/20 per case), same results as the prototype core.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#ifndef CELEBISESSIONTEST_HPP
#define CELEBISESSIONTEST_HPP

#include <QObject>

class CelebiSessionTest : public QObject
{
    Q_OBJECT
private slots:
    void resavePaths();
    void calibrateUpdate();
    void fileRoundTrip();
    void resaveGate();
};

#endif // CELEBISESSIONTEST_HPP
