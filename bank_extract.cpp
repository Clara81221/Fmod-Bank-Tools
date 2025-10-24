#include "bank_extract.h"
#include "qdebug.h"
#include "qfileinfo.h"

int bank_extract::extract(QString bankPath)
{
    int check = 0;

    QFile file(bankPath);

    if (!file.open(QIODevice::ReadOnly))
        return check;

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    in.setByteOrder(QDataStream::LittleEndian);

    char* magicArray = (char*)malloc(4);
    in.readRawData(magicArray, 4);
    QString magic = QString::fromUtf8(magicArray, 4);
    free(magicArray);

    if (magic != "RIFF")
        return check;

    file.seek(0x08);

    char* fevStringArray = (char*)malloc(4);
    in.readRawData(fevStringArray, 4);
    QString fevString = QString::fromUtf8(fevStringArray, 4);
    free(fevStringArray);

    if (fevString != "FEV ")
        return check;

    file.seek(0x14);

    quint32 version;
    in >> version;

    if (version == 0)
        return check;

    file.seek(0x1c);

    char* listStringArray = (char*)malloc(4);
    in.readRawData(listStringArray, 4);
    QString listString = QString::fromUtf8(listStringArray, 4);
    free(listStringArray);

    if (listString != "LIST")
        return check;

    file.seek(file.pos() + 0x04);

    char* projStringArray = (char*)malloc(4);
    in.readRawData(projStringArray, 4);
    QString projString = QString::fromUtf8(projStringArray, 4);
    free(projStringArray);

    char* BnkiStringArray = (char*)malloc(4);
    in.readRawData(BnkiStringArray, 4);
    QString BnkiString = QString::fromUtf8(BnkiStringArray, 4);
    free(BnkiStringArray);

    if (projString != "PROJ" || BnkiString != "BNKI")
        return check;

    unsigned int sndh_unknown = 0, sndh_fsbOffset = 0, sndh_fsbSize = 0;

    quint32 chunk_size;
    in >> chunk_size;

    file.seek(file.pos() + chunk_size);

    while (sndh_fsbOffset == 0 && file.pos() < file.size())
    {
        quint32 chunk_type;
        in >> chunk_type;

        quint32 chunk_size;
        in >> chunk_size;

        if (chunk_type == 0xFFFFFFFF || chunk_size == 0xFFFFFFFF)
            return check;

        //qInfo() << "FSB5 FEV: chunk = " + QString::number(chunk_type) + " offset = " + QString::number((file.pos() - 0x08)) + " size = " + QString::number(chunk_size) + " \n";

        switch(chunk_type)
        {
           case 0x48444E53: /* "SNDH" */;
            in >> sndh_unknown;
            in >> sndh_fsbOffset;
            in >> sndh_fsbSize;
            break;
        }

        file.seek(file.pos() + chunk_size);
    }

    if (sndh_fsbOffset == 0 || sndh_fsbSize == 0)
    {
        check = 2;
        return check;
    }

    file.seek(sndh_fsbOffset);

    char* fsbStringArray = (char*)malloc(4);
    in.readRawData(fsbStringArray, 4);
    QString fsbMagic = QString::fromUtf8(fsbStringArray, 4);
    free(fsbStringArray);

    check = 1;

    if (fsbMagic != "FSB5")
        check = 5;

    file.seek(sndh_fsbOffset);

    char* fsbData = (char*)malloc(sndh_fsbSize);
    in.readRawData(fsbData, sndh_fsbSize);

    QString bankName = file.fileName();

    file.close();

    QFileInfo fileInfo(bankName);
    QString fsbNameTmp = fileInfo.fileName().replace(".bank", ".fsb");

    QFile fsboutFile(QCoreApplication::applicationDirPath() + "/fsb/" + fsbNameTmp);

    if (!fsboutFile.open(QIODevice::WriteOnly))
        return 0;

    fsboutFile.open(QIODevice::WriteOnly);
    fsboutFile.write(fsbData, sndh_fsbSize);
    fsboutFile.flush();
    fsboutFile.close();
    free(fsbData);
    return check;
}
