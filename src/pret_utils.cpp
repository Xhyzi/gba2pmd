#include "include/pret_utils.h"
#include "include/globals.h"
#include <QFile>
#include <QTextStream>

static bool InitPretSongTableLength();
static bool InitPretVoiceGroupTableLength();
static bool InitPretKeySplitTableLength();
static bool InitPretType();

bool InitPretRepoData()
{
    bool success;
    success = InitPretSongTableLength();
    success = InitPretVoiceGroupTableLength();
    success = InitPretKeySplitTableLength();
    success = InitPretType();

    return success;
}

static bool InitPretSongTableLength()
{
    quint16 songNumber = 0;
    QString line;
    QFile f(pretPath + SONG_TABLE_FILE);

    if (f.open(QIODevice::ReadOnly))
    {
        QTextStream in(&f);

        while(!in.atEnd())
        {
            line = in.readLine();
            if (line.contains("song "))
                songNumber++;
        }
        pretSongTableSize = songNumber - 1;
        f.close();
        return true;
    }
    return false;
}

static bool InitPretVoiceGroupTableLength()
{
    quint16 vgNumber = 0;
    QString line;
    QFile f(pretPath + VOICE_GROUP_TABLE_FILE);

    if (f.open(QIODevice::ReadOnly))
    {
        QTextStream in(&f);

        while (!in.atEnd())
        {
            line = in.readLine();
            if(line.contains(QRegExp("voicegroup\\d\\d\\d::")))
                vgNumber++;
        }
        pretvgTableSize = vgNumber;
        f.close();
        return true;
    }

    return false;
}

static bool InitPretKeySplitTableLength()
{
    quint16 keysplitNumber = 0;
    QString line;
    QFile f(pretPath + KEYSPLIT_FILE);

    if (f.open(QIODevice::ReadOnly))
    {
        QTextStream in(&f);

        while(!in.atEnd())
        {
            line = in.readLine();
            if (line.contains(" KeySplitTable"))
                keysplitNumber++;
        }
        pretKsTableSize = keysplitNumber;
        f.close();
        return true;
    }
    return false;
}

static bool InitPretType()
{
    QFile f(pretPath + "/README.md");
    QString line;

    if (f.open(QIODevice::ReadOnly))
    {
        QTextStream in(&f);

        while (!in.atEnd())
        {
            line = in.readLine();

            if (line.contains("pokeruby.gba"))
            {
                pretVersion = "pokeruby";
                return true;
            }
            if (line.contains("pokefirered.gba"))
            {
                pretVersion = "pokefirered";
                return true;
            }
            if (line.contains("pokeemerald.gba"))
            {
                pretVersion = "pokeemerald";
                return true;
            }
        }
    }
    return false;
}
