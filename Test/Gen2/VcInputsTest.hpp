/*
 * PokeFinderPlus: new file. Checks Core/Gen2/VcInputs against the prototype (golden2/inputs.json, made by
 * Test/Gen2/tools/make_goldens.py inputs): for each decrypted CIA in $POKEFINDERPLUS_GEN2_CIAS (folders separated by ':')
 * the title check gives the prototype's hashes, the ROM inside is the cartridge dump, and ROM + the title's own patch is the
 * VC ROM the goldens were made with; encrypted CIAs are refused with a clear message; a cartridge dump alone gives the same
 * Celebi as the patched ROM on 20 golden timelines per language. SHA-1 and the patch format have ROM-free checks.
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
    void patchFormat();
    void notACia();
    void cias();
    void dumpAlone();
};

#endif // VCINPUTSTEST_HPP
