/*
 * PokeFinderPlus: new file. Checks Core/Gen2/CelebiSearch (and the calibration job list) against the prototype
 * (golden2/search.json, made by Test/Gen2/tools/make_goldens.py search):
 *  - pick on the stored fsearch directories gives the plans of record (SPEC v0.6 section 3.2 and the class >= 2 plans);
 *  - the job lists (fsearch, switch-rule grids, calibration grids) contain the stored job keys;
 *  - sampled fsearch jobs, 60 grid jobs (fg3_fr, fgx_fr, cal3_fr) and regr.py's 20 rows recomputed byte for byte;
 *  - clock sweeps and windows of FR and IT plan 1;
 *  - the threaded runners: fsearch resumed from 100 ES jobs stops where the prototype stopped (164) with its hits; the
 *    threaded calibration index equals the one built from the prototype's grids.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#ifndef CELEBISEARCHTEST_HPP
#define CELEBISEARCHTEST_HPP

#include <QObject>

class CelebiSearchTest : public QObject
{
    Q_OBJECT
private slots:
    void plansOfRecord();
    void jobKeys();
    void windowsFromSweep();
    void fsearchJobs();
    void gridJobs();
    void regrRows();
    void sweeps();
    void fsearchThreads();
    void calibrationThreads();
};

#endif // CELEBISEARCHTEST_HPP
