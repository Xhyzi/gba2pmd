#ifndef GLOBALS_H
#define GLOBALS_H

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QFile;
class QStringList;
QT_END_NAMESPACE


struct Song {
    quint16 id;
    quint32 headerPointer;
    quint16 ms;
    quint16 me;
};

struct SongHeader {
    quint8 tracks;
    quint8 blocks;
    quint8 priority;
    quint8 reverb;
    quint32 voiceGroupPointer;
};

struct Instrument {
    quint8 type;
    quint8 data[11];
};

struct VoiceGroup {
    quint32 offset;
    struct Instrument instruments[256];
};

struct DirectSound {
    quint8 note;
    quint8 pan;
    quint32 sample;
    quint8 atk;
    quint8 dec;
    quint8 sus;
    quint8 rel;
};

struct SquareSound {
    quint8 sweep;
    quint8 duty_cycle;
    quint8 atk;
    quint8 dec;
    quint8 sus;
    quint8 rel;
};

struct ProgramableWave {
    quint32 data;
    quint8 atk;
    quint8 dec;
    quint8 sus;
    quint8 rel;
};

struct VoiceNoise {
    quint8 period;
    quint8 atk;
    quint8 dec;
    quint8 sus;
    quint8 rel;
};

struct VoiceKeysplit {
    quint32 svg;
    quint32 keysplit;
};

//TODO:Get from GUI
#define TEMP_LASTS_SONG 609
#define TEMP_LAST_VG 190
#define TEMP_LAST_KEYSPLIT 5


#define DIRECT_SOUND        0x00
#define DIRECT_SOUND_NO_R   0x08
#define DIRECT_SOUND_ALT    0x10
#define VOICE_SQUARE_1      0x01
#define VOICE_SQUARE_1_ALT  0x09
#define VOICE_SQUARE_2      0x02
#define VOICE_SQUARE_2_ALT  0x0A
#define VOICE_PROGRAMABLE_WAVE  0x03
#define VOICE_PROGRAMABLE_WAVE_ALT 0x0B
#define VOICE_NOISE         0x04
#define VOICE_NOISE_ALT     0x0C
#define VOICE_KEYSPLIT      0x40
#define VOICE_KEYSPLIT_ALL  0x80

#define KEYSPLIT_MAX_ELEMENTS 108

extern QFile romFile;
extern QByteArray romHex;
extern quint8 romType;
extern bool romOpened;

extern quint32 songTableOffset;
extern quint32 songTableSize;

extern quint16 minSong;
extern quint16 maxSong;


#endif // GLOBALS_H
