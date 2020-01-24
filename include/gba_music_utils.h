#ifndef GBA_MUSIC_UTILS_H
#define GBA_MUSIC_UTILS_H

#include <QByteArray>
#include "include/globals.h"

#define SONG_TABLE_PADDING  8
#define SONG_MS_OFFSET 4
#define SONG_ME_OFFSET 6
#define SAMPLE_LENGTH_OFFSET 0xC
#define VG_SIZE 128
#define VG_ENTRY_LENGTH 0xC
#define SAMPLE_HEADER_LENGTH 0x10
#define REVERB_MASK 0x7F
#define STD_REVERB  50


enum {INSTRUMENT_NORMAL, INSTRUMENT_ALT, INSTRUMENT_NO_RESAMPLE};


void InitROMData();
void DecompileSongData(quint16 min, quint16 max);
void BuildSongFiles();






//void AddTextToFile(QString file, QString content, bool blankLine);

#endif // GBA_MUSIC_UTILS_H
