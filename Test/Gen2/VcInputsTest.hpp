/*
 * PokeFinderPlus: new file. Checks Core/Gen2/VcInputs: the ROM hash table and its matching (ROM-free); every supported
 * ROM in $POKEFINDERPLUS_GBC_ROMS gets its check mark and none with one byte changed; each ROM alone gives the VC ROM's
 * Celebi (20 golden timelines per ROM, all 400 EN timelines for EN Rev 0); for each decrypted CIA in
 * $POKEFINDERPLUS_GEN2_CIAS (folders separated by ':', ';' on Windows) the title check gives the prototype's hashes
 * (golden2/inputs.json) and the ROM inside gets the check mark; encrypted CIAs are refused with a clear message.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#ifndef VCINPUTSTEST_HPP
#define VCINPUTSTEST_HPP

#include <QObject>

class VcInputsTest : public QObject
{
    Q_OBJECT
private slots:
    void sha1();
    void knownRoms();
    void romCheckMarks();
    void notACia();
    void cias();
    void romsGiveVcCelebi();
};

#endif // VCINPUTSTEST_HPP
