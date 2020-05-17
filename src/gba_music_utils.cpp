#include "include/aif2pcm/aif2pcm.h"
#include "include/gba_music_utils.h"
#include "include/binary_utils.h"
#include "include/globals.h"
#include <QTextStream>
#include <QList>
#include <QDir>

/** Data Init **/
static void InitROMSongTableOffset();
static void InitROMSongTableEntries();
/** Parsers **/ //Parse music related data
static void ParseSong(quint16 pos);
static void ParseSongHeader(Song song);
static void ParseVoiceGroup(quint32 vgOffset);
static QString ParseVGEntry(quint32 vgeOffset);
static QString ParseDirectSound(Instrument ins, quint8 mode);
static QString ParseVoiceSquare_1(Instrument ins, quint8 mode);
static QString ParseReadVoiceSquare_2(Instrument ins, quint8 mode);
static QString ParseProgrammableWave(Instrument ins, quint8 mode);
static QString ParseVoiceNoise(Instrument ins, quint8 mode);
static QString ParseKeysplit(Instrument ins, quint8 mode);
static void ParseSplit(quint32 offset);
/** Create File Entries **/
static void CreateSongTableEntry(struct Song song);
static void CreateSongConstantEntry(struct Song song);
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
static void BuildVoiceGroupFile(quint32 vgOffset);
static void BuildVoiceGroupsTable();
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
static void RenameKeysplitSVGPointers();
static void RenameKeysplitPointers();

static QStringList songTable_list;             //sound/song_table.inc
static QStringList songConstants_list;         //include/constants/songs.h
static QMap<quint32, QStringList> voiceGroups_map; //sound/voice_groups.inc
static QMap<quint32, QStringList> keySplit_map;    //sound/keysplit_tables.inc
static QList<quint32> sample_list;                 //sound/direct_sound_samples/XXX.aif
static QList<quint32> pwSample_list;               //sound/programmable_wave_samples/XXX.pcm
static QMap<quint32, quint16> vgIds_map;       //Contains id's of each map
static QMap<quint16, quint32> ksplitIds_map;
static QStringList songMK_list;                //songs.mk
static QStringList ld_scripts_list;    //@song_data ld_script.txt

//Initialize ROM Data
void InitROMData(bool unkownRom)
{
    if(!unkownRom)
        InitROMSongTableOffset();
    InitROMSongTableEntries();
}

//Search song's table
static void InitROMSongTableOffset()
{
    romSongTableOffset = ResolveROMHexPointer(ROM_SONG_TABLE_POINTERS[romType]);
}

//Initialize the ammount of song entries in the song table
static void InitROMSongTableEntries()
{
    romSongTableSize = -1;

    for(int i=0;; i += SONG_TABLE_PADDING, romSongTableSize++)
        if (ReadROMWordAt(romSongTableOffset + i) == 0 || romSongTableSize == 999)
            break;
}

//Starts extraction of music data from ROM between min and max entries of the song table
void ExtractROMSongData(quint16 min, quint16 max, MainWindow* mw)
{
    songTable_list.clear();
    songConstants_list.clear();
    voiceGroups_map.clear();
    keySplit_map.clear();
    sample_list.clear();
    pwSample_list.clear();
    vgIds_map.clear();
    ksplitIds_map.clear();
    songMK_list.clear();
    ld_scripts_list.clear();

    quint16 entries = max - min;

    for (int i=0; i<=entries; i++)
    {
        ParseSong(min + i);

        mw->SetPercentage((i+1) * 100 / (entries + 1));
    }

    BuildSongFiles();
}

/* ****************************** *
 * ***** Music Data Parsers ***** *
 * ****************************** */
//Parses song data at the given position in the song table
static void ParseSong(quint16 pos)
{
    Song song;

    song.id = pretSongTableSize + 1 + songTable_list.size();
    song.headerPointer = ResolveROMHexPointer(romSongTableOffset + pos * SONG_TABLE_PADDING);
    song.ms = ReadROMHWordAt(romSongTableOffset + pos * SONG_TABLE_PADDING + SONG_MS_OFFSET);
    song.me = ReadROMHWordAt(romSongTableOffset + pos * SONG_TABLE_PADDING + SONG_ME_OFFSET);

    CreateSongTableEntry(song);
    CreateSongConstantEntry(song);

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
        //Adds the id beforehand to avoid infinite looping
        //Just in case the voicegroup contains itself in a keysplit
        vgIds_map.insert(vgOffset, pretvgTableSize + voiceGroups_map.size());
        voiceGroups_map.insert(vgOffset, voiceGroup_list);
        for (int i=0; i<VG_SIZE; i++)
        {
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

    try {

        switch (ins.type) {

            case DIRECT_SOUND:
                entry = ParseDirectSound(ins, INSTRUMENT_NORMAL);
                break;

            case DIRECT_SOUND_NO_R:
                entry = ParseDirectSound(ins, INSTRUMENT_ALT);
                break;

            case DIRECT_SOUND_ALT:
                entry = ParseDirectSound(ins, INSTRUMENT_NO_RESAMPLE);
                break;

            case VOICE_SQUARE_1:
                entry = ParseVoiceSquare_1(ins, INSTRUMENT_NORMAL);
                break;

            case VOICE_SQUARE_1_ALT:
                entry = ParseVoiceSquare_1(ins, INSTRUMENT_ALT);
                break;

            case VOICE_SQUARE_2:
                entry = ParseReadVoiceSquare_2(ins, INSTRUMENT_NORMAL);
                break;

            case VOICE_SQUARE_2_ALT:
                entry = ParseReadVoiceSquare_2(ins, INSTRUMENT_ALT);
                break;

            case VOICE_PROGRAMABLE_WAVE:
                entry = ParseProgrammableWave(ins, INSTRUMENT_NORMAL);
                break;

            case VOICE_PROGRAMABLE_WAVE_ALT:
                entry = ParseProgrammableWave(ins, INSTRUMENT_ALT);
                break;

            case VOICE_NOISE:
                entry = ParseVoiceNoise(ins, INSTRUMENT_NORMAL);
                break;

            case VOICE_NOISE_ALT:
                entry = ParseVoiceNoise(ins, INSTRUMENT_ALT);
                break;

            case VOICE_KEYSPLIT:
                entry = ParseKeysplit(ins, INSTRUMENT_NORMAL);
                break;

            case VOICE_KEYSPLIT_ALL:
                entry = ParseKeysplit(ins, INSTRUMENT_ALT);
                break;

            default:
                QString msg = "Unkown voice entry type: " + IntToDecimalQString(ins.type);
                throw msg;
        }

        return entry;

    } catch(QString msg) {
        return DEFAULT_VG_ENTRY + " \t\t@PLACEHOLDER: " + msg;
    }
}

//Parses a DirectSound entry
static QString ParseDirectSound(Instrument ins, quint8 mode)
{
    DirectSound dsound;

    dsound.sample = ((ins.data[6] << 24) + (ins.data[5] << 16) + (ins.data[4] << 8) + ins.data[3]); //& BINARY_POINTER_MASK;

    if (dsound.sample < 0x8000000 || dsound.sample > 0x9FFFFFF)
    {
        QString msg = "Bad sample pointer \"" + IntToHexQString(dsound.sample) + "\"";
        throw msg;
    }

    dsound.note = ins.data[0];
    dsound.pan = ins.data[2];
    dsound.sample &= BINARY_POINTER_MASK;
    dsound.atk = ins.data[7];
    dsound.dec = ins.data[8];
    dsound.sus = ins.data[9];
    dsound.rel = ins.data[10];

    if (!sample_list.contains(dsound.sample))
    {
        BuildTempSampleBinary(dsound.sample);
        sample_list.append(dsound.sample);
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
static QString ParseProgrammableWave(Instrument ins, quint8 mode)
{
    ProgramableWave pwave;

    pwave.data = (ins.data[6] << 24) + (ins.data[5] << 16) + (ins.data[4] << 8) + ins.data[3];

    if (pwave.data < 0x8000000 || pwave.data > 0x9FFFFFF)
    {
        QString msg = "Bad programmable Wave pointer \"" + IntToHexQString(pwave.data) + "\"";
        throw msg;
    }

    pwave.data &= BINARY_POINTER_MASK;
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

    vksplit.svg = ((ins.data[6] << 24) + (ins.data[5] << 0x10) + (ins.data[4] << 0x8) + ins.data[3]);

    if (vksplit.svg < 0x8000000 || vksplit.svg > 0x9FFFFFF)
    {
        QString msg = "Bad voiceGroup pointer \"" + IntToDecimalQString(vksplit.svg) + "\"";
        throw msg;
    }

    vksplit.svg &= BINARY_POINTER_MASK;

    if (mode == INSTRUMENT_NORMAL)
    {
        vksplit.keysplit = ((ins.data[10] << 0x18) + (ins.data[9] << 0x10) + (ins.data[8] << 0x8) + ins.data[7]) & BINARY_POINTER_MASK;
        if (!keySplit_map.contains(vksplit.keysplit))
        {
            ParseSplit(vksplit.keysplit);
            ksplitIds_map.insert(pretKsTableSize + keySplit_map.size(), vksplit.keysplit);
        }
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
        splitData[j] = ReadROMByteAt(offset + j);
    }

    for (int i=0; i<elements; i++)
    {
        keySplit_list.append(IntToDecimalQString(splitData[i]));
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
    QString entry = "#define MUS_" + IntToDecimalQString(song.id) +
            " " + IntToDecimalQString(song.id);
    songConstants_list.append(entry);
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
            "DirectSoundWaveData_" + IntToHexQString(ds.sample) + ", " +
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

    entry += "ProgrammableWaveData_" + IntToHexQString(pw.data) + ", " +
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
        entry += "\tvoice_keysplit svg_" +
                IntToHexQString(vk.svg) + ", ksplit_" +
                IntToHexQString(vk.keysplit);
    else    //keysplit_all
        entry += "\tvoice_keysplit_all svg_" +
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
    RenameKeysplitSVGPointers();
    RenameKeysplitPointers();
    CreatePaths();
    BuildSongTableFile();
    BuildSongConstantsFile();

    BuildVoiceGroupsTable();
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

//VG Individual .inc file
static void BuildVoiceGroupFile(quint32 vgOffset)
{
    QStringList vg;

    QFile f(OUTPUT_DIRECTORY +
            VG_DIR + "/voicegroup" +
            IntToDecimalQString(vgIds_map[vgOffset]) +
            ".inc");

    if (f.open(QIODevice::ReadWrite))
    {
        QTextStream out(&f);

        vg = voiceGroups_map[vgOffset];

        out << "\n\t.align 2\n";
        out << "voicegroup" +
                    IntToDecimalQString(vgIds_map[vgOffset]) +
                    ":: @ " + IntToHexQString(vgOffset) + "\n";

        for (int j=0; j<VG_SIZE; j++)
            out << vg[j] + "\n";
    }
}

//Table with all voicegroups
static void BuildVoiceGroupsTable()
{
    //QStringList vg;

    QMap<quint16, quint32> vgById;
    QMapIterator<quint32, quint16> it(vgIds_map);

    while (it.hasNext())
    {
        it.next();
        vgById.insert(it.value(), it.key());
    }

    QFile f(OUTPUT_DIRECTORY + VOICE_GROUP_TABLE_FILE);

    if (f.open(QIODevice::ReadWrite))
    {
        QTextStream out(&f);

        for (int i=pretvgTableSize; i<pretvgTableSize+vgIds_map.size(); i++)
        {
            out << "\n.include \"sound/voicegroups/voicegroup" +
                   IntToDecimalQString(i) + ".inc\"";
            BuildVoiceGroupFile(vgById[i]);
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
        QMapIterator<quint16, quint32> it(ksplitIds_map);


        while (it.hasNext())
        {
            it.next();
            ks = keySplit_map[it.value()];

            out << "\n\n.set KeySplitTable" +
                   IntToDecimalQString(it.key()) + ", . - " +
                   IntToDecimalQString(0);  //KEYSPLIT_MAX_ELEMENTS - ks.size()

            for (int i=0; i<ks.size(); i++)
            {
                out << "\n\t.byte " + ks[i];
            }
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
                   IntToDecimalQString(pretSongTableSize + i +1)+ ".o(.rodata);";
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
            out << "\n\t.incbin \"" + PW_SAMPLE_DIR +
                   "/" + IntToHexQString(pwSample_list[i]) +
                   PWS_EXTENSION + "\"";
        }
        out.flush();
        f.close();
    }
}

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
            out << ReadROMByteAt(sample + i);
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
        path = OUTPUT_DIRECTORY + "/" + DS_SAMPLE_DIR + "/" +
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
    CreatePath(OUTPUT_DIRECTORY + VG_DIR);
}

//Creates a path if does not exist
static void CreatePath(QString path)
{
    QDir dir;

    if(!dir.exists(path))
        dir.mkpath(path);
}

static void RenameKeysplitSVGPointers()
{
    QStringList vg;
    quint32 offset;
    quint16 id;

    QMapIterator<quint32, quint16> i(vgIds_map);

    while(i.hasNext())
    {
        i.next();
        offset = i.key();
        id = i.value();

        QMapIterator<quint32, QStringList> i_vg(voiceGroups_map);

        while(i_vg.hasNext())
        {
            i_vg.next();
            voiceGroups_map[i_vg.key()].replaceInStrings("svg_" + IntToHexQString(offset),
                                                         "voicegroup" + IntToDecimalQString(id));
        }
    }
}

static void RenameKeysplitPointers()
{
    QStringList vg;
    quint32 offset;
    quint16 id;

    QMapIterator<quint16, quint32> i(ksplitIds_map);

    while(i.hasNext())
    {
        i.next();
        offset = i.value();
        id = i.key();

        QMapIterator<quint32, QStringList> i_vg(voiceGroups_map);

        while (i_vg.hasNext())
        {
            i_vg.next();
            voiceGroups_map[i_vg.key()].replaceInStrings("ksplit_" + IntToHexQString(offset),
                                                         "KeySplitTable" + IntToDecimalQString(id));
        }
    }
}
