#include "include/binary_utils.h"
#include "include/globals.h"

//Opens ROM file
bool InitROMFile(QString filePath)
{
    romFile.setFileName(filePath);

    if (romFile.open(QIODevice::ReadOnly))
    {
        romHex = romFile.readAll();
        romFile.close();
        return true;
    }
    return false;
}

//Checks the loaded ROM version
bool CheckRomVersion()
{
    quint32 header = ReadROMWordAt(ROM_HEADER_OFFSET);

    for (uint i=0; i < sizeof(ROM_HEADERS)/sizeof(ROM_HEADERS[0]); i++)
        if (ROM_HEADERS[i] == header)
        {
            romType = i;
            return true;
        }

    return false;
}

//Resolve the pointer at the given offset
quint32 ResolveROMHexPointer(quint32 pointerOffset)
{
    quint32 destOffset = 0;

    for (int i = 3; i >= 0; i--)
    {
        destOffset += ReadROMByteAt(pointerOffset+i) << 8 * i;
    }

    //transforms 08->00 and 09->01 (ROM & Extended ROM)
    destOffset &= BINARY_POINTER_MASK;

    return destOffset;
}

//Reads the byte at the given ROM offset
quint8 ReadROMByteAt(quint32 offset)
{
    return static_cast<quint8>(romHex[offset]);;
}

//Reads the Half Word at the given ROM offset (16bit)
quint16 ReadROMHWordAt(quint32 offset)
{
    quint16 hWord = 0;

    for (int i=1; i>=0; i--)
    {
        hWord += ReadROMByteAt(offset + i) << 8 * i;
    }

    return hWord;
}

//Reads the word at the given ROM offset (32 bit)
quint32 ReadROMWordAt(quint32 offset)
{
    quint32 word = 0;

    for (int i=3; i>=0; i--)
    {
        word += ReadROMByteAt(offset + i) << i * 8;
    }

    return word;
}

//Convert number into hex format string
QString IntToHexQString(quint32 decimal)
{
    QString hex;

    return hex.setNum(decimal, 16);
}

//Convert number into decimal format string
QString IntToDecimalQString(quint32 decimal)
{
    QString hex;

    return hex.setNum(decimal, 10);
}

QString HWordToPermutedString(quint16 hword)
{
    QString aux_s, string="";
    quint8 aux_i;

    if ((aux_i = hword & 0xFF) < 0x10)
        string += "0";
    string += aux_s.setNum(aux_i, 16) + " ";

    if ((aux_i = hword >> 8) < 0x10)
        string += "0";
    string += aux_s.setNum(aux_i, 16);

    return string;
}

bool IsROMFile()
{
    if (romFile.size() == (8 << 20) ||
            romFile.size() == (16 << 20) ||
            romFile.size() == (32 << 20))
        return true;
    else
        return false;
}
