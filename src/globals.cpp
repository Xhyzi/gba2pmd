#include "include/globals.h"
#include <QFile>
#include <QMap>
#include <QStringList>

QFile romFile;
QByteArray romHex;
quint8 romType;
quint32 romSongTableOffset;
quint32 romSongTableSize;

QString pretPath;
quint16 pretSongTableSize;
quint16 pretvgTableSize;
quint16 pretKsTableSize;
QString pretVersion;

QString OUTPUT_DIRECTORY;
quint16 minSong;
quint16 maxSong;
bool romReady;
bool pretReady;
bool automaticSongNames = true;
bool overridePret = false;
