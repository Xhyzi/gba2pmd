#ifndef BINARY_UTILS_H
#define BINARY_UTILS_H

#include <QByteArray>
#include <QFile>

enum {AXVE, BPRE, BPEE};
#define ROM_HEADER_OFFSET 0xAC
#define AXVE_HEADER 0x45565841
#define BPRE_HEADER 0x45525042
#define BPEE_HEADER 0x45455042

#define BINARY_POINTER_MASK 0x1FFFFFF

const quint32 ROM_HEADERS[] = {AXVE_HEADER, BPRE_HEADER, BPEE_HEADER};
const QString ROM_NAMES[] = {"Ruby", "Fire Red", "Emerald"};
const quint32 ROM_SONG_TABLE_POINTERS[] = {0x1DDF20, 0x1DD11C, 0x2E0158};

bool OpenRom(QString filePath);
bool CheckRomVersion();
quint32 ResolveROMHexPointer(quint32 pointerOffset);
quint8 ReadROMByteAt(quint32 offset);
quint16 ReadROMHWordAt(quint32 offset);
quint32 ReadROMWordAt(quint32 offset);
QString IntToHexQString(quint32 decimal);
QString IntToDecimalQString(quint32 decimal);
QString HWordToPermutedString(quint16 hword);



#endif // BINARY_UTILS_H
