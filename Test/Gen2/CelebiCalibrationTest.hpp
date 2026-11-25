/*
 * PokeFinderPlus: new file. Checks the calibration and reset-band code of Core/Gen2/CelebiCalibration against the prototype
 * (golden2/cal.json, made by Test/Gen2/tools/make_goldens.py cal). ROM-free (the grids are in the golden data):
 *  - Victor's retail calibrations (X -21 / -1, Kenji 1, accepted; band: cal1 rule A, k 0, "add 2"; cal2 rule A, P(band) 0.02);
 *  - the EN harness calibration b5 at 6 / 8 / 10 attempts (accepted at 10: X +14, Kenji >= 2, carry 2);
 *  - band_posterior / band_advice on 500 random observation sets (posterior to 1e-9).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#ifndef CELEBICALIBRATIONTEST_HPP
#define CELEBICALIBRATIONTEST_HPP

#include <QObject>

class CelebiCalibrationTest : public QObject
{
    Q_OBJECT
private slots:
    void dvCandidates();
    void calibrations();
    void gateNumbers();
    void bandRandom();
};

#endif // CELEBICALIBRATIONTEST_HPP
