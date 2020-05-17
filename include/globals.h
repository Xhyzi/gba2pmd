#ifndef GLOBALS_H
#define GLOBALS_H

#include <QtGlobal>
#include <QString>

QT_BEGIN_NAMESPACE
class QFile;
class QStringList;
QT_END_NAMESPACE

const QString SOUND_DIR = "/sound";
const QString CONSTANTS_DIR = "/include/constants";
const QString PW_SAMPLE_DIR = "sound/programmable_wave_samples";
const QString DS_SAMPLE_DIR = "sound/direct_sound_samples";
const QString MIDI_DIR = "sound/songs/midi";
const QString VG_DIR = "/sound/voicegrups";

const QString PWS_EXTENSION = ".pcm";
const QString BIN_EXTENSION = ".bin";

const QString PWAVE_DATA_FILE = "/sound/programmable_wave_data.inc";
const QString DSOUND_DATA_FILE = "/sound/direct_sound_data.inc";
const QString SONG_MK_FILE = "/songs.mk";
const QString LD_SCRIPT_FILE = "/ld_script.txt";
const QString KEYSPLIT_FILE = "/sound/keysplit_tables.inc";
const QString VOICE_GROUP_TABLE_FILE = "/sound/voice_groups.inc";
const QString SONG_TABLE_FILE = "/sound/song_table.inc";

const QString DEFAULT_VG_ENTRY = "\tvoice_square_1 255, 255, 255, 255, 255, 255";

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

#define KEYSPLIT_MAX_ELEMENTS 0x80

extern QFile romFile;
extern QByteArray romHex;
extern quint8 romType;
extern quint32 romSongTableOffset;
extern quint32 romSongTableSize;

extern QString pretPath;
extern quint16 pretSongTableSize;
extern quint16 pretvgTableSize;
extern quint16 pretKsTableSize;
extern QString pretVersion;

extern QString OUTPUT_DIRECTORY;
extern quint16 minSong;
extern quint16 maxSong;
extern bool romReady;
extern bool pretReady;
extern bool automaticSongNames;
extern bool overridePret;

#endif // GLOBALS_H
