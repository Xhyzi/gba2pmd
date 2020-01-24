#include "include/globals.h"
#include <QFile>
#include <QMap>
#include <QStringList>

QFile romFile;
QByteArray romHex;
quint8 romType;
bool romOpened;

quint32 songTableOffset;
quint32 songTableSize;

quint16 minSong;
quint16 maxSong;
bool generateSongNames;
bool override;
