#ifndef BINARY_UTILS_H
#define BINARY_UTILS_H

#include <QByteArray>
#include <QFile>

#define ROM_HEADER_OFFSET 0xAC
#define AXVE_HEADER 0x45565841
#define BPRE_HEADER 0x45525042
#define BPEE_HEADER 0x45455042
#define AXVS_HEADER 0x53565841
#define BPRS_HEADER 0x53525042
#define BPES_HEADER 0x53455042
#define AXVJ_HEADER 0x4A565841
#define BPRJ_HEADER 0x4A525042
#define BPEJ_HEADER 0x4A455042
#define AFEJ_HEADER 0x4A454641
#define AE7E_HEADER 0x45374541
#define BE8E_HEADER 0x45384542
#define BZ6P_HEADER 0x50365A42

#define BINARY_POINTER_MASK 0x1FFFFFF

const quint32 ROM_HEADERS[] = {AXVE_HEADER, BPRE_HEADER, BPEE_HEADER,
                              AXVS_HEADER, BPRS_HEADER, BPES_HEADER,
                              AXVJ_HEADER, BPRJ_HEADER, BPEJ_HEADER,
                              AFEJ_HEADER, AE7E_HEADER, BE8E_HEADER,
                              BZ6P_HEADER};
const QString ROM_NAMES[] = {"Ruby [Eng]", "Fire Red [Eng]", "Emerald [Eng]",
                            "Rubí [Esp]", "Rojo Fuego [Esp]", "Esmeralda [Esp]",
                            "Ruby [Jap]", "Fire Red [Jap]", "Emerald [Jap]",
                            "FE 6 [Eng]", "FE 7 [Eng]", "FE 8 [Eng]",
                            "Final Fantasy VI"};
const QString ROM_CODES[] = {"AXVE", "BPRE", "BPEE",
                            "AXVS", "BPRS", "BPES",
                            "AXVJ", "BPRJ", "BPEJ",
                            "AFEJ", "AE7E", "BE8E",
                            "BZ6P"};
const quint32 ROM_SONG_TABLE_POINTERS[] = {0x1DDF20, 0x1DD11C, 0x2E0158,
                                          0x1E2C30, 0x1DCC50, 0x2E78E0,
                                          0x1AE9B8, 0x1C10D8, 0x28E6E0,
                                          0x003748, 0x003F50, 0x0028BC,
                                          0x134A50};
const quint32 ROM_SONG_TABLE_POINTERS_ALT[] = {0x1DDF20, 0x1DD11C, 0x2E0158,
                                              0x1E2C64, 0x1DCC84, 0x2E7918,
                                              0x1AE9EC, 0x1C110C, 0x28E714,
                                              0x01545C, 0x014DE8, 0x014B80,
                                              0x134A84};
const QString PRET_NAMES[] = {"pokeruby", "pokefirered", "pokeemerald"};
const QString PRET_SHA_FILE[] = {"pokeruby.sha1", "pokefirered.sha1", "pokeemerald.sha1"};

bool InitROMFile(QString filePath);
bool CheckRomVersion();
quint32 ResolveROMHexPointer(quint32 pointerOffset);
quint8 ReadROMByteAt(quint32 offset);
quint16 ReadROMHWordAt(quint32 offset);
quint32 ReadROMWordAt(quint32 offset);
QString IntToHexQString(quint32 decimal);
QString IntToDecimalQString(quint32 decimal);
QString HWordToPermutedString(quint16 hword);
bool IsROMFile();

#endif // BINARY_UTILS_H
