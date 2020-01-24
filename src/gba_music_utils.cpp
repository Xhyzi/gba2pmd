#include "include/aif2pcm/aif2pcm.h"
#include "include/gba_music_utils.h"
#include "include/binary_utils.h"
#include "include/globals.h"
#include <QDebug>
#include <QList>
#include <QDir>

/** Data Init **/
static void InitSongTableOffset();
static void InitSongTableEntries();
/** Parsers **/ //Parse music related data
static void ParseSong(quint16 pos);
static void ParseSongHeader(Song song);
static void ParseVoiceGroup(quint32 vgOffset);
static QString ParseVGEntry(quint32 vgeOffset);
static QString ParseDirectSound(Instrument ins, quint8 mode);
static QString ParseVoiceSquare_1(Instrument ins, quint8 mode);
static QString ParseReadVoiceSquare_2(Instrument ins, quint8 mode);
static QString ParseProgramableWave(Instrument ins, quint8 mode);
static QString ParseVoiceNoise(Instrument ins, quint8 mode);
static QString ParseKeysplit(Instrument ins, quint8 mode);
static void ParseSplit(quint32 offset);
/** Create File Entries **/
static void CreateSongTableEntry(struct Song song);
static void CreateSongConstantEntry(struct Song song);
static void CreateCharmapEntry(struct Song song);
static QString CreateDirectSoundEntry(DirectSound ds, quint8 mode);
static QString CreateSquareSound1Entry(SquareSound ss, quint8 mode);
static QString CreateSquareSound2Entry(SquareSound ss, quint8 mode);
static QString CreateProgramableWaveEntry(ProgramableWave pw, quint8 mode);
static QString CreateVoiceNoise(VoiceNoise vn, quint8 mode);
static QString CreateVoiceKeysplit(VoiceKeysplit vk, quint8 mode);
static void CreateSongMKEntry(struct Song song, struct SongHeader header);
/** Build Files **/ //Build the different music related files
static void BuildSongTableFile();
static void BuildSongConstantsFile();
static void BuildCharmapFiles();
static void BuildCharmapFile(QString dir);
static void BuildVoiceGroupsFile();
static void BuildKeySplitFile();
static void BuildDirectSoundDataFile();
static void BuildProgrammableWaveDataFile();
static void BuildLd_ScriptFile();
static void BuildSongsMKFile();
static void BuildTempSampleBinary(quint32 sample);
static void BuildAifSampleFile(QString path);
static void DeleteTempBinarySampleFiles();
static void BuildPcmSampleFile(quint32 pcm);
/** Utils **/
static void CreatePaths();
static void CreatePath(QString path);
static void AddTextToFile(QString file, QString content, bool blankLine);

static QStringList songTable_list;             //sound/song_table.inc
static QStringList songConstants_list;         //include/constants/songs.h
static QStringList charmaps_list;              //charmap.txt berry_fix/charmap.txt berry_fix/payload/charmap.txt
static QMap<quint32, QStringList> voiceGroups_map; //sound/voice_groups.inc
static QMap<quint32, QStringList> keySplit_map;    //sound/keysplit_tables.inc
static QList<quint32> sample_list;                 //sound/direct_sound_samples/XXX.aif
static QList<quint32> pwSample_list;               //sound/programmable_wave_samples/XXX.pcm
static QMap<quint32, quint16> vgIds_map;       //Contains id's of each map
static QStringList songMK_list;                //songs.mk
static QStringList ld_scripts_list;    //@song_data ld_script.txt


const QString OUTPUT_DIRECTORY = "C:/test"; //TODO:Eliminar, obtener de la GUI

static const QString SOUND_DIR = "/sound";
static const QString CONSTANTS_DIR = "/include/constants";
static const QString BERRY_FIX_DIR = "/berry_fix";
static const QString PAYLOAD_DIR = "/berry_fix/payload";
static const QString PW_SAMPLE_DIR = "sound/programmable_wave_samples";
static const QString DS_SAMPLE_DIR = "sound/direct_sound_samples";
static const QString MIDI_DIR = "sound/songs/midi";

static const QString PWS_EXTENSION = ".pcm";
static const QString BIN_EXTENSION = ".bin";

static const QString PWAVE_DATA_FILE = "/sound/programmable_wave_data.inc";
static const QString DSOUND_DATA_FILE = "/sound/direct_sound_data.inc";
static const QString SONG_MK_FILE = "/songs.mk";
static const QString LD_SCRIPT_FILE = "/ld_script.txt";
static const QString KEYSPLIT_FILE = "/sound/keysplit_tables.inc";
static const QString VOICE_GROUP_FILE = "/sound/voice_groups.inc";
static const QString CHARMAP_FILE = "charmap.txt";
static const QString SONG_TABLE_FILE = "/sound/song_table.inc";

//Initialize ROM Data
void InitROMData()
{
    InitSongTableOffset();
    InitSongTableEntries();
}

//Search song's table
static void InitSongTableOffset()
{
    songTableOffset = ResolveROMHexPointer(ROM_SONG_TABLE_POINTERS[romType]);
}

//Initialize the ammount of song entries in the song table
static void InitSongTableEntries()
{
    songTableSize = -1;

    for(int i=0;; i += SONG_TABLE_PADDING, songTableSize++)
        if (ReadROMWordAt(songTableOffset + i) == 0)
            break;
}

//Starts extraction of music data from ROM between min and max entries of the song table
void DecompileSongData(quint16 min, quint16 max)
{
    //TODO: Do this from gui, limiting max tab to songTableSize
    if (max > songTableSize)
        max = songTableSize;

    for (int i=0; i<max-min; i++)
        ParseSong(min + i + 1);

    BuildSongFiles();
}

/* ****************************** *
 * ***** Music Data Parsers ***** *
 * ****************************** */
//Parses song data at the given position in the song table
static void ParseSong(quint16 pos)
{
    Song song;

    song.id = TEMP_LASTS_SONG + 1 + songTable_list.size();
    song.headerPointer = ResolveROMHexPointer(songTableOffset + pos * SONG_TABLE_PADDING);
    song.ms = ReadROMHWordAt(songTableOffset + pos * SONG_TABLE_PADDING + SONG_MS_OFFSET);
    song.me = ReadROMHWordAt(songTableOffset + pos * SONG_TABLE_PADDING + SONG_ME_OFFSET);

    CreateSongTableEntry(song);
    CreateSongConstantEntry(song);
    CreateCharmapEntry(song);

    ParseSongHeader(song);
}

//Parses SongHeader
static void ParseSongHeader(Song song)
{
    SongHeader header;

    header.priority = ReadROMByteAt(song.headerPointer + 2);
    header.reverb = ReadROMByteAt(song.headerPointer + 3);
    header.voiceGroupPointer = ResolveROMHexPointer(song.headerPointer + 4);

    ParseVoiceGroup(header.voiceGroupPointer);
    CreateSongMKEntry(song, header);
}

//Parses a VoiceGroup, either from songHeader or a keysplit
static void ParseVoiceGroup(quint32 vgOffset)
{
    QList<QString> voiceGroup_list;

    if (!voiceGroups_map.contains(vgOffset))
    {
        //Adds the id beforehand to avoid infinite looping in case the voicegroup contains itself in a keysplit
        voiceGroups_map.insert(vgOffset, voiceGroup_list);
        vgIds_map.insert(vgOffset, TEMP_LAST_VG + voiceGroups_map.size());
        for (int i=0; i<VG_SIZE; i++)
        {
            qDebug() << "";
            qDebug() << "VG Entry " + IntToDecimalQString(i) + " " + IntToHexQString(vgOffset);
            voiceGroups_map[vgOffset].append(ParseVGEntry(vgOffset + VG_ENTRY_LENGTH * i));
        }
    }
}

//Parses a VoiceGroup entry
static QString ParseVGEntry(quint32 vgeOffset)
{
    QString entry;
    Instrument ins;

    ins.type = ReadROMByteAt(vgeOffset);

    for (int i=0; i<11; i++)
    {
        ins.data[i] = ReadROMByteAt(vgeOffset + i + 1);
    }

    switch (ins.type) {

        case DIRECT_SOUND:
            qDebug() << "Direct_Sound";
            entry = ParseDirectSound(ins, INSTRUMENT_NORMAL);
            break;

        case DIRECT_SOUND_NO_R:
            qDebug() << "Direct_Sound no resample";
            entry = ParseDirectSound(ins, INSTRUMENT_ALT);
            break;

        case DIRECT_SOUND_ALT:
            qDebug() << "Direct_Sound_Alt";
            entry = ParseDirectSound(ins, INSTRUMENT_NO_RESAMPLE);
            break;

        case VOICE_SQUARE_1:
            qDebug() << "VSquare_1";
            entry = ParseVoiceSquare_1(ins, INSTRUMENT_NORMAL);
            break;

        case VOICE_SQUARE_1_ALT:
            qDebug() << "VSquare_1_Alt";
            entry = ParseVoiceSquare_1(ins, INSTRUMENT_ALT);
            break;

        case VOICE_SQUARE_2:
            qDebug() << "VSquare_2";
            entry = ParseReadVoiceSquare_2(ins, INSTRUMENT_NORMAL);
            break;

        case VOICE_SQUARE_2_ALT:
            qDebug() << "VSQuare_2_Alt";
            entry = ParseReadVoiceSquare_2(ins, INSTRUMENT_ALT);
            break;

        case VOICE_PROGRAMABLE_WAVE:
            qDebug() << "Programable Wave";
            entry = ParseProgramableWave(ins, INSTRUMENT_NORMAL);
            break;

        case VOICE_PROGRAMABLE_WAVE_ALT:
            qDebug() << "Programable_Wave_Alt";
            entry = ParseProgramableWave(ins, INSTRUMENT_ALT);
            break;

        case VOICE_NOISE:
            qDebug() << "Voice_Noise";
            entry = ParseVoiceNoise(ins, INSTRUMENT_NORMAL);
            break;

        case VOICE_NOISE_ALT:
            qDebug() << "Voice_Noise_alt";
            entry = ParseVoiceNoise(ins, INSTRUMENT_ALT);
            break;

        case VOICE_KEYSPLIT:
            qDebug() << "Voice_Keysplit";
            entry = ParseKeysplit(ins, INSTRUMENT_NORMAL);
            break;

        case VOICE_KEYSPLIT_ALL:
            qDebug() << "Voice_Keysplit_Alt";
            entry = ParseKeysplit(ins, INSTRUMENT_ALT);
            break;
    }

    return entry;
}

//Parses a DirectSound entry
static QString ParseDirectSound(Instrument ins, quint8 mode)
{
    DirectSound dsound;

    dsound.note = ins.data[0];
    dsound.pan = ins.data[2];
    dsound.sample = (ins.data[5] << 16) + (ins.data[4] << 8) + ins.data[3];
    dsound.atk = ins.data[7];
    dsound.dec = ins.data[8];
    dsound.sus = ins.data[9];
    dsound.rel = ins.data[10];

    if (!sample_list.contains(dsound.sample))
    {
        sample_list.append(dsound.sample);
        BuildTempSampleBinary(dsound.sample);
    }
    return CreateDirectSoundEntry(dsound, mode);
}

//Parses a VoiceSquare_1 entry
static QString ParseVoiceSquare_1(Instrument ins, quint8 mode)
{
    SquareSound ssound;

    ssound.sweep = ins.data[2];
    ssound.duty_cycle = ins.data[3];
    ssound.atk = ins.data[7];
    ssound.dec = ins.data[8];
    ssound.sus = ins.data[9];
    ssound.rel = ins.data[10];

    return CreateSquareSound1Entry(ssound, mode);
}

//Parses a VoiceSquare_2 entry
static QString ParseReadVoiceSquare_2(Instrument ins, quint8 mode)
{
    SquareSound ssound;

    ssound.duty_cycle = ins.data[3];
    ssound.atk = ins.data[7];
    ssound.dec = ins.data[8];
    ssound.sus = ins.data[9];
    ssound.rel = ins.data[10];

    return CreateSquareSound2Entry(ssound, mode);
}

//Parses a ProgramableWave entry
static QString ParseProgramableWave(Instrument ins, quint8 mode)
{
    ProgramableWave pwave;

    pwave.data = (ins.data[5] << 16) + (ins.data[4] << 8) + ins.data[3];
    pwave.atk = ins.data[7];
    pwave.dec = ins.data[8];
    pwave.sus = ins.data[9];
    pwave.rel = ins.data[10];

    if (!pwSample_list.contains(pwave.data))
    {
        pwSample_list.append(pwave.data);
        BuildPcmSampleFile(pwave.data);
    }

    return CreateProgramableWaveEntry(pwave, mode);
}

//Parses VoiceNoise entry
static QString ParseVoiceNoise(Instrument ins, quint8 mode)
{
    VoiceNoise vnoise;

    vnoise.period = ins.data[3];
    vnoise.atk = ins.data[7];
    vnoise.dec = ins.data[8];
    vnoise.sus = ins.data[9];
    vnoise.rel = ins.data[10];

    return CreateVoiceNoise(vnoise, mode);
}

//Parse Keysplit
static QString ParseKeysplit(Instrument ins, quint8 mode)
{
    VoiceKeysplit vksplit;

    vksplit.svg = (ins.data[5] << 0x10) + (ins.data[4] << 0x8) + ins.data[3];

    if (mode == INSTRUMENT_NORMAL)
    {
        vksplit.keysplit = (ins.data[9] << 0x10) + (ins.data[8] << 0x8) + ins.data[7];
        if (!keySplit_map.contains(vksplit.keysplit))
            ParseSplit(vksplit.keysplit);
    }

    ParseVoiceGroup(vksplit.svg);

    return CreateVoiceKeysplit(vksplit, mode);
}

//Parse split from a keysplit_all
static void ParseSplit(quint32 offset)
{
    QStringList keySplit_list;
    quint8 backwardsOffset = 0;
    quint8 elements = 0;

    //Gets the backwards offset and the number of elements of the split
    for (int i=0; i < KEYSPLIT_MAX_ELEMENTS; i++)
    {
        if (ReadROMByteAt(offset + i) == 0)
        {
            backwardsOffset = i;
            elements = KEYSPLIT_MAX_ELEMENTS - backwardsOffset;
            break;
        }
    }

    //Reads the split
    quint8 splitData[elements];
    for (int j=0; j<elements; j++)
    {
        splitData[j] = ReadROMByteAt(offset + backwardsOffset + j);
    }

    for (int i=0; i<elements; i++)
    {
        keySplit_list.append(IntToHexQString(splitData[i]));
    }

    keySplit_map.insert(offset, keySplit_list);
}


/* ****************************** *
 * ****** Entry Generation ****** *
 * ****************************** */
static void CreateSongTableEntry(struct Song song)
{
    QString entry = "\tsong mus_" + IntToDecimalQString(song.id) +
            ", " + IntToDecimalQString(song.ms) + ", " + IntToDecimalQString(song.me);
    songTable_list.append(entry);
}

static void CreateSongConstantEntry(struct Song song)
{
    QString entry = "#define MUS_" + IntToDecimalQString(song.id);
    songConstants_list.append(entry);
}

static void CreateCharmapEntry(struct Song song)
{
    QString entry = "MUS_" + IntToDecimalQString(song.id) + " = " + HWordToPermutedString(song.id);
    charmaps_list.append(entry);
}

static QString CreateDirectSoundEntry(DirectSound ds, quint8 mode)
{
    QString entry="";

    switch (mode)
    {
    case INSTRUMENT_NORMAL:
        entry += "\tvoice_directsound ";
        break;

    case INSTRUMENT_ALT:
        entry += "\tvoice_directsound_no_resample ";
        break;

    case INSTRUMENT_NO_RESAMPLE:
        entry += "\tvoice_directsound_alt ";
        break;
    }

    entry += IntToDecimalQString(ds.note) + ", " +
            IntToDecimalQString(ds.pan) + ", " +
            IntToHexQString(ds.sample) + ", " +
            IntToDecimalQString(ds.atk) + ", " +
            IntToDecimalQString(ds.dec) + ", " +
            IntToDecimalQString(ds.sus) + ", " +
            IntToDecimalQString(ds.rel);

    return entry;
}

static QString CreateSquareSound1Entry(SquareSound ss, quint8 mode)
{
    QString entry="";

    if (mode == INSTRUMENT_NORMAL)
        entry += "\tvoice_square_1 ";
    else
        entry += "\tvoice_square_1_alt ";

    entry += IntToDecimalQString(ss.sweep) + ", " +
            IntToDecimalQString(ss.duty_cycle) + ", " +
            IntToDecimalQString(ss.atk) + ", " +
            IntToDecimalQString(ss.dec) + ", " +
            IntToDecimalQString(ss.sus) + ", " +
            IntToDecimalQString(ss.rel);

    return entry;
}

static QString CreateSquareSound2Entry(SquareSound ss, quint8 mode)
{
    QString entry="";

    if (mode == INSTRUMENT_NORMAL)
        entry += "\tvoice_square_2 ";
    else
        entry += "\tvoice_square_2_alt ";

    entry += IntToDecimalQString(ss.duty_cycle) + ", " +
            IntToDecimalQString(ss.atk) + ", " +
            IntToDecimalQString(ss.dec) + ", " +
            IntToDecimalQString(ss.sus) + ", " +
            IntToDecimalQString(ss.rel);

    return entry;
}

static QString CreateProgramableWaveEntry(ProgramableWave pw, quint8 mode)
{
    QString entry="";

    if (mode == INSTRUMENT_NORMAL)
        entry += "\tvoice_programmable_wave ";
    else
        entry += "\tvoice_programmable_wave_alt ";

    entry += IntToHexQString(pw.data) + ", " +
            IntToDecimalQString(pw.atk) + ", " +
            IntToDecimalQString(pw.dec) + ", " +
            IntToDecimalQString(pw.sus) + ", " +
            IntToDecimalQString(pw.rel);

    return entry;
}

static QString CreateVoiceNoise(VoiceNoise vn, quint8 mode)
{
    QString entry="";

    if (mode == INSTRUMENT_NORMAL)
        entry += "\tvoice_noise ";
    else
        entry += "\tvoice_noise_alt ";

    entry += IntToDecimalQString(vn.period) + ", " +
            IntToDecimalQString(vn.atk) + ", " +
            IntToDecimalQString(vn.dec) + ", " +
            IntToDecimalQString(vn.sus) + ", " +
            IntToDecimalQString(vn.rel);

    return entry;
}

static QString CreateVoiceKeysplit(VoiceKeysplit vk, quint8 mode)
{
    QString entry="";

    if (mode == INSTRUMENT_NORMAL)
        entry += "\tvoice_keysplit " +
                IntToHexQString(vk.svg) + ", " +
                IntToHexQString(vk.keysplit);
    else    //keysplit_all
        entry += "\tvoice_keysplit_all " +
                IntToHexQString(vk.svg);

    return entry;
}

static void CreateSongMKEntry(struct Song song, struct SongHeader header)
{
    QString reverb, priority, voicegroup;

    //Calculates reverb value
    if ((header.reverb & REVERB_MASK) == STD_REVERB)
        reverb = " -R$(STD_REVERB)";
    else if (header.reverb != 0)
        reverb = " -R" + IntToDecimalQString(header.reverb & REVERB_MASK);
    else
        reverb = "";

    //Calculates priority string
    if (header.priority == 0)
        priority = "";
    else
        priority = "-P" + IntToDecimalQString(header.priority);

    //Calculates VoiceGroup id
    if (vgIds_map[header.voiceGroupPointer] < 100)
        voicegroup = "0" + IntToDecimalQString(vgIds_map[header.voiceGroupPointer]);
    else
        voicegroup = IntToDecimalQString(vgIds_map[header.voiceGroupPointer]);

    QString entry = "$(MID_SUBDIR)/mus_" + IntToDecimalQString(song.id) + ".s: %.s: %.mid" +
            "\n\t$(MID) $< $@ -E" + reverb + " -G" + voicegroup + " -V100 " + priority;

    songMK_list.append(entry);
}

/* ****************************** *
 * ******* File Builders ******** *
 * ****************************** */
void BuildSongFiles()
{
    CreatePaths();
    BuildSongTableFile();
    BuildSongConstantsFile();
    BuildCharmapFiles();
    BuildVoiceGroupsFile();
    BuildKeySplitFile();
    BuildDirectSoundDataFile();
    BuildProgrammableWaveDataFile();
    BuildLd_ScriptFile();
    BuildSongsMKFile();
    DeleteTempBinarySampleFiles();
}

static void BuildSongTableFile()
{
    QFile f (OUTPUT_DIRECTORY + SONG_TABLE_FILE);

    if (f.open(QIODevice::ReadWrite))
    {
        QTextStream s(&f);

        for(int i=0; i<songTable_list.size(); i++)
        {
            s << songTable_list[i]+"\n";
        }
        s.flush();
        f.close();
    }
}

static void BuildSongConstantsFile()
{
    QFile f(OUTPUT_DIRECTORY+"/include/constants/songs.h");

    if (f.open(QIODevice::ReadWrite))
    {
        QTextStream out(&f);

        for (int i=0; i<songConstants_list.size(); i++)
        {
            out << songConstants_list[i]+"\n";
        }
        out.flush();
        f.close();
    }
}

static void BuildCharmapFiles()
{
    BuildCharmapFile("");
    BuildCharmapFile(BERRY_FIX_DIR);
    BuildCharmapFile(PAYLOAD_DIR);
}

static void BuildCharmapFile(QString dir)
{
    QFile f (OUTPUT_DIRECTORY + dir + CHARMAP_FILE);

    if (f.open(QIODevice::ReadWrite))
    {
        QTextStream out(&f);

        for (int i=0; i<charmaps_list.size(); i++)
        {
            out << charmaps_list[i]+"\n";
        }
        out.flush();
        f.close();
        qDebug() << "charmap.txt Created!!";
    }
}

static void BuildVoiceGroupsFile()
{
    QStringList vg;

    QFile f(OUTPUT_DIRECTORY + VOICE_GROUP_FILE);

    if (f.open(QIODevice::ReadWrite))
    {
        QTextStream out(&f);
        QMapIterator<quint32, QStringList> i(voiceGroups_map);

        for (int vg_id=TEMP_LAST_VG + 1; i.hasNext(); vg_id++)
        {
            i.next();
            vg = i.value();

            out << "\n\t.align 2\n";
            out << "voicegroup" +
                   IntToDecimalQString(vg_id) +
                   ":: @ " + IntToHexQString(i.key()) + "\n";

            for(int j=0; j<VG_SIZE; j++)
                out << vg[j] + "\n";
        }
        out.flush();
        f.close();
    }
}

static void BuildKeySplitFile()
{
    QStringList ks;
    QFile f(OUTPUT_DIRECTORY + KEYSPLIT_FILE);

    if(f.open(QIODevice::ReadWrite))
    {
        QTextStream out(&f);
        QMapIterator<quint32, QStringList> i(keySplit_map);

        for (int ks_id = TEMP_LAST_KEYSPLIT + 1; i.hasNext(); ks_id++)
        {
            i.next();
            ks = i.value();

            out << "\n\n.set KeySplitTable" +
                   IntToDecimalQString(ks_id) + ", . - " +
                   IntToDecimalQString(KEYSPLIT_MAX_ELEMENTS - ks.size());

            for (int j=0; j<ks.size(); j++)
                out << "\n\t.byte " + ks[j];
        }
        out.flush();
        f.close();
    }
}

static void BuildLd_ScriptFile()
{
    QFile f(OUTPUT_DIRECTORY + LD_SCRIPT_FILE);

    if (f.open(QIODevice::ReadWrite))
    {
        QTextStream out(&f);

        for(int i=0; i<songTable_list.size(); i++)
        {
            out << "\n\t\t" + MIDI_DIR + "/mus_" +
                   IntToDecimalQString(TEMP_LASTS_SONG + i +1)+ ".o(.rodata);";
        }
        out.flush();
    }
    f.close();
}

static void BuildSongsMKFile()
{
    QFile f(OUTPUT_DIRECTORY + SONG_MK_FILE);

    if (f.open(QIODevice::ReadWrite))
    {
        QTextStream out(&f);

        for (int i=0; i<songMK_list.size(); i++)
        {
            out << "\n" + songMK_list[i] + "\n";
        }
        out.flush();
        f.close();
    }
}

static void BuildDirectSoundDataFile()
{
    QFile f(OUTPUT_DIRECTORY + DSOUND_DATA_FILE);

    if (f.open(QIODevice::ReadWrite))
    {
        QTextStream out(&f);

        for (int i=0; i<sample_list.size(); i++)
        {
            out << "\n\t.align 2";
            out << "\nDirectSoundWaveData_" +
                   IntToHexQString(sample_list[i]) + "::";
            out << "\n\t.incbin \"" + DS_SAMPLE_DIR +
                   "/" + IntToHexQString(sample_list[i]) +
                   BIN_EXTENSION +"\"\n";
        }
        out.flush();
        f.close();
    }
}

static void BuildProgrammableWaveDataFile()
{
    QFile f(OUTPUT_DIRECTORY + PWAVE_DATA_FILE);

    if (f.open(QIODevice::ReadWrite))
    {
        QTextStream out(&f);

        for (int i=0; i<pwSample_list.size(); i++)
        {
            out << "\n\nProgrammableWaveData_" +
                   IntToHexQString(pwSample_list[i]) + "::";
            out << "\n\t.incbin \"" + DS_SAMPLE_DIR +
                   "/" + IntToHexQString(pwSample_list[i]) +
                   PWS_EXTENSION + "\"";
        }
        out.flush();
        f.close();
    }
}

//TODO: Erase temp files
static void BuildTempSampleBinary(quint32 sample)
{
    quint32 sampleLenght;
    QString path = OUTPUT_DIRECTORY + "/" + DS_SAMPLE_DIR;
    CreatePath(path);
    path += "/" + IntToHexQString(sample) + BIN_EXTENSION;

    QFile f(path);
    if (f.open(QIODevice::ReadWrite))
    {
        QDataStream out(&f);
        sampleLenght = ReadROMHWordAt(sample + SAMPLE_LENGTH_OFFSET);   //Change by C

        for (quint32 i=0; i<(sampleLenght+SAMPLE_HEADER_LENGTH); i++)
        {

            out << ReadROMByteAt(sample + i);
        }
        f.close();
    }
    BuildAifSampleFile(path);
}

static void BuildAifSampleFile(QString path)
{
    //This conversion might not be optimal, not really sure about this
    //Converting QString into char*
    std::string str = path.toStdString();
    char *cstr = new char[str.length()+1];
    strcpy(cstr, str.c_str());

    main_aif2pcm(cstr); //calls aif2pcm by @huderlem
}

static void DeleteTempBinarySampleFiles()
{
    QString path;

    for (int i=0; i<sample_list.size(); i++)
    {
        path = OUTPUT_DIRECTORY + DS_SAMPLE_DIR + "/" +
                IntToHexQString(sample_list[i]) + BIN_EXTENSION;
        QFile f(path);
        f.remove();
        f.close();
    }
}

static void BuildPcmSampleFile(quint32 pcm)
{
    QString path = OUTPUT_DIRECTORY + "/" + PW_SAMPLE_DIR;
    CreatePath(path);
    path += "/" + IntToHexQString(pcm) + PWS_EXTENSION;

    QFile f(path);
    if (f.open(QIODevice::ReadWrite))
    {
        QDataStream out(&f);
        for (int i=0; i<SAMPLE_HEADER_LENGTH; i++)
            out << ReadROMByteAt(pcm + i);
    }
    f.close();
}

/* ****************************** *
 * ********** Utils ************* *
 * ****************************** */
static void CreatePaths()
{
    CreatePath(OUTPUT_DIRECTORY + SOUND_DIR);
    CreatePath(OUTPUT_DIRECTORY + CONSTANTS_DIR);
    CreatePath(OUTPUT_DIRECTORY + BERRY_FIX_DIR);
    CreatePath(OUTPUT_DIRECTORY + PAYLOAD_DIR);
}

//Creates a path if does not exist
static void CreatePath(QString path)
{
    QDir dir;

    if(!dir.exists(path))
        dir.mkpath(path);
}

static void AddTextToFile(QString file, QString content, bool blankLine)
{
    QString fileName= OUTPUT_DIRECTORY + file;
    QFile f(fileName);

    if (blankLine)
       content = "\n" + content;

    if (f.open(QIODevice::ReadWrite))
    {
        QTextStream out(&f);

        while(!out.atEnd()) {out.readLine();}

        out << content << endl;
        out.flush();  //Makes sure to flush buffer  before closing QString->string
    }
    f.close();
}
