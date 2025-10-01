#include "rebuild_worker.h"
#include "fileio.h"

RebuildWorker::RebuildWorker(QObject *parent) : QObject(parent) {}

void RebuildWorker::rebuild_bank()
{
    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);

    settings.beginGroup("Directorys");
    QString fsbDir = QCoreApplication::applicationDirPath() + "/fsb/";
    QString bankDir = settings.value("BankDir").toString() + "/";
    QString wavDir = settings.value("WavDir").toString() + "/";
    QString rebuildDir = settings.value("RebuildDir").toString() + "/";
    settings.endGroup();

    settings.beginGroup("Options");
    QString format = settings.value("Format").toString();
    unsigned int quality = settings.value("Quality").toUInt();

    QStringList nameFilters;
    nameFilters << "*.txt";

    QDirIterator it(wavDir, nameFilters, QDir::Files, QDirIterator::Subdirectories);
    QStringList wavTxtList;

    QDir bank_directory(bankDir);
    QStringList bankPasswordFileList = bank_directory.entryList(nameFilters);

    while (it.hasNext()) {
        wavTxtList << it.next();
    }

    if (wavTxtList.count() == 0)
    {
        emit taskFinished("\nError could not find txt wav lists !!!");
        emit progressUpdated(0);
        return;
    }

    FSBANK_RESULT result;

    for (int i = 0; i < wavTxtList.count(); i++)
    {
        result = FSBank_Init(FSBANK_FSBVERSION_FSB5, FSBANK_INIT_GENERATEPROGRESSITEMS, 2, 0);
        if (result != FSBANK_OK) { emit taskFinished(FSBank_ErrorString(result)); return; }

        std::vector<FSBANK_SUBSOUND> subsounds;
        QStringList wavFiles = readTextFileToQStringList(wavTxtList[i]);
        QFileInfo bankFileInfo(wavTxtList[i]);
        QString bankFileName = bankDir + bankFileInfo.completeBaseName() + ".bank";
        QString fsbFileName = fsbDir + bankFileInfo.completeBaseName() + ".fsb";
        char* wavFile[wavFiles.size()];

        QString newLineCheck = (i == 0) ? "" : "\n";
        emit updateConsole(newLineCheck + "Fmod Bank file: " + bankFileInfo.completeBaseName() + ".bank");
        QString _format = format == "vorbis" ? "Vorbis" : "PCM";
        emit updateConsole("Format: " + _format);
        emit updateConsole("Thread Count: 2\n");
        emit updateConsole("ReBuilding " + bankFileInfo.completeBaseName() + ".bank" + " has started, Please wait.....\n");

        emit progressUpdated(10);

        for (int j = 0; j < wavFiles.size(); j++)
        {
            QString wavName = wavFiles[j];
            QString wavFilePath = wavDir + bankFileInfo.completeBaseName() + "/" + wavName;
            QByteArray wavFilePathArray = wavFilePath.toUtf8();
            wavFile[j] = new char[wavFilePathArray.size() + 1];
            strcpy(wavFile[j], wavFilePathArray.constData());

            auto &subsound = subsounds.emplace_back();
            std::memset(&subsound, 0, sizeof(FSBANK_SUBSOUND));

            subsound.numFiles = 1;
            subsound.fileNames = &wavFile[j];
        }

        QByteArray fsbFileNameArray = fsbFileName.toUtf8();
        char* outputFile = new char[fsbFileNameArray.size() + 1];
        strcpy(outputFile, fsbFileNameArray.constData());

        emit progressUpdated(35);

        char* encryption = 0;

        if (bankPasswordFileList.size() != 0)
        {
            QString password = "";

            foreach (const QString _pswTxt, bankPasswordFileList)
            {
                if (_pswTxt.contains(bankFileInfo.completeBaseName()))
                {
                    QStringList tmp = readTextFileToQStringList(bankFileName.replace(".bank", ".txt"));
                    password = tmp[0];
                }
            }

            QByteArray encryptionkeyArray = password.toUtf8();
            encryption = new char[encryptionkeyArray.size() + 1];
            strcpy(encryption, encryptionkeyArray.constData());

            if (!password.isEmpty())
                emit emit updateConsole("Encrypting bank file with password: " + encryptionkeyArray + "\n");
        }

        result = FSBank_Build(subsounds.data(), subsounds.size(), format == "vorbis" ? FSBANK_FORMAT_VORBIS : FSBANK_FORMAT_PCM, FSBANK_BUILD_DEFAULT, quality, encryption, outputFile);
        if (result != FSBANK_OK) { emit taskFinished(FSBank_ErrorString(result)); return; }

        bankProgress(wavFiles);

        result = FSBank_Release();
        if (result != FSBANK_OK) { emit taskFinished(FSBank_ErrorString(result)); return; }

        for (int j = 0; j < wavFiles.size(); j++)
        {
            delete[] wavFile[j];
        }

        delete[] outputFile;
        delete[] encryption;
        bankRebuild(bankFileName.replace(".txt", ".bank"), rebuildDir);
    }

    emit progressUpdated(0);
    emit taskFinished("\nRebuilding Bank files has finished");
}

void RebuildWorker::bankProgress(const QStringList wavList)
{
    // Build process started successfully, now fetch progress items
    const FSBANK_PROGRESSITEM* progressItem = nullptr;

    int index = 0;

    while (FSBank_FetchNextProgressItem(&progressItem) == FSBANK_OK && progressItem != nullptr)
    {
        // Process the progress item
        switch (progressItem->state)
        {
        case FSBANK_STATE_PREPROCESSING:
            // Item is waiting to be processed
            break;
        case FSBANK_STATE_ANALYSING:
            // Item is analysing
            break;
        case FSBANK_STATE_DECODING:
            // Item is decoding
            break;
        case FSBANK_STATE_ENCODING:
        {
            // Item is currently being built
            break;
        }
        case FSBANK_STATE_WRITING:
        {
            // Item is writing
            break;
        }
        case FSBANK_STATE_FINISHED:
        {
            // Item build complete
            if (progressItem->subSoundIndex != -1)
            {
                emit updateConsole(QString::number(index) + ": (" + wavList[index] + ") [Proccessing]");
                int subSoundsPercent = 100 * (index + 1) / wavList.size();
                emit progressUpdated(subSoundsPercent); // Emit signal to update progress in UI
                index++;
            }
            break;
        }
        case FSBANK_STATE_WARNING:
            // Item warning
            emit updateConsole("\nWarning, there is a issue with one of the wav files.");
            break;
        case FSBANK_STATE_FAILED:
            // Item build failed
            emit updateConsole("\nfsb file failed to build.");
            break;
        default:
            emit updateConsole("\nUnknown error");
            break;
        }

        FSBank_ReleaseProgressItem(progressItem); // Release memory for the item
        progressItem = nullptr; // Reset for next fetch
    }
}

void RebuildWorker::bankRebuild(const QString bankFile, const QString buildPath)
{
    QFile file(bankFile);

    if (!file.open(QIODevice::ReadOnly)) { emit taskFinished("\nError opening file: " + bankFile); return; }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    in.setByteOrder(QDataStream::LittleEndian);

    char* magicArray = (char*)malloc(4);
    in.readRawData(magicArray, 4);
    QString magic = QString::fromUtf8(magicArray, 4);
    free(magicArray);

    if (magic != "RIFF") { emit taskFinished("\nError, has no RIFF in header"); return; }

    file.seek(0x08);

    char* fevStringArray = (char*)malloc(4);
    in.readRawData(fevStringArray, 4);
    QString fevString = QString::fromUtf8(fevStringArray, 4);
    free(fevStringArray);

    if (fevString != "FEV ") { emit taskFinished("\nError, has no FEV in header"); return; }

    file.seek(0x14);

    quint32 version;
    in >> version;

    if (version == 0) { emit taskFinished("\nError, version not supported"); return; }

    file.seek(0x1c);

    char* listStringArray = (char*)malloc(4);
    in.readRawData(listStringArray, 4);
    QString listString = QString::fromUtf8(listStringArray, 4);
    free(listStringArray);

    if (listString != "LIST") { emit taskFinished("\nError, has no LIST in header"); return; }

    file.seek(file.pos() + 0x04);

    char* projStringArray = (char*)malloc(4);
    in.readRawData(projStringArray, 4);
    QString projString = QString::fromUtf8(projStringArray, 4);
    free(projStringArray);

    char* BnkiStringArray = (char*)malloc(4);
    in.readRawData(BnkiStringArray, 4);
    QString BnkiString = QString::fromUtf8(BnkiStringArray, 4);
    free(BnkiStringArray);

    if (projString != "PROJ" || BnkiString != "BNKI") { emit taskFinished("\nError, has no PROJ or BNKI in header"); return; }

    unsigned int sndh_offset = 0, sndh_size = 0, sndh_location = 0, snd_location = 0;

    quint32 chunk_size;
    in >> chunk_size;

    file.seek(file.pos() + chunk_size);

    while (snd_location == 0 && file.pos() < file.size())
    {
        quint32 chunk_type;
        in >> chunk_type;

        quint32 chunk_size;
        in >> chunk_size;

        if (chunk_type == 0xFFFFFFFF || chunk_size == 0xFFFFFFFF) { emit taskFinished("\nError, chunk_type or chunk_size is wrong"); return; }

        //qInfo() << "FSB5 FEV: chunk=" + QString::number(chunk_type) + " offset=" + QString::number((file.pos() - 0x08)) + " size=" + QString::number(chunk_size) + " \n";

        switch(chunk_type)
        {
            case 0x48444E53: /* "SNDH" */
            {
                unsigned int pos = file.pos();
                file.seek(file.pos() + 4);
                sndh_location = file.pos();
                in >> sndh_offset;
                in >> sndh_size;
                file.seek(pos);
                break;
            }
            case 0x20444E53: /* "SND " */
            {
                snd_location = file.pos();
                break;
            }
        }

        file.seek(file.pos() + chunk_size);
    }

    if (sndh_offset == 0 || sndh_size == 0) { emit taskFinished("\nError, sndh_offset or sndh_size should not be 0"); return; }
    if (sndh_location == 0) { emit taskFinished("\nError, sndh_location should not be 0"); return; }
    if (snd_location == 0) { emit taskFinished("\nError, snd_location should not be 0"); return; }

    file.seek(0);

    char* bankHeader = (char*)malloc(sndh_offset);
    in.readRawData(bankHeader, sndh_offset);

    QString bankName = file.fileName();

    file.close();

    QFileInfo fileInfo(bankName);
    QString bankNameTmp = fileInfo.fileName();

    QFile bankoutFile(buildPath + bankNameTmp);

    if (!bankoutFile.open(QIODevice::WriteOnly)) { emit taskFinished("\nError, writing to: " + buildPath + bankNameTmp); return; }

    bankoutFile.write(bankHeader, sndh_offset);
    unsigned int bankPos = bankoutFile.pos();

    QString fsbFileName = QCoreApplication::applicationDirPath() + "/fsb/" + bankNameTmp.replace(".bank", ".fsb");
    QFile fsbInFile(fsbFileName);
    if (!fsbInFile.open(QIODevice::ReadOnly)) { emit taskFinished("\nError, reading: " + fsbFileName); return; }

    unsigned int fsbSize = fsbInFile.size();

    bankoutFile.seek(4);
    unsigned int headerSize = (sndh_offset + fsbSize) - 8;
    bankoutFile.write(reinterpret_cast<const char*>(&headerSize), 4);
    bankoutFile.seek(sndh_location + 4);
    bankoutFile.write(reinterpret_cast<const char*>(&fsbSize), 4);
    bankoutFile.seek(snd_location - 4);
    unsigned int lastFsbSize = fsbSize + (bankPos - snd_location);
    bankoutFile.write(reinterpret_cast<const char*>(&lastFsbSize), 4);
    bankoutFile.seek(bankPos);

    unsigned int chunkCount = chunkAmount(fsbSize);
    std::vector<long> _chunkSizes = chunkSizes(fsbSize, chunkCount);

    char* fsbBuffer = nullptr;

    for (unsigned int k = 0; k < chunkCount; k++)
    {
        fsbBuffer = (char*)malloc(_chunkSizes[k]);
        qint64 read = fsbInFile.read(fsbBuffer, _chunkSizes[k]);
        if (read == -1) { emit taskFinished("\nError reading fsb chunk: " + QString::number(k)); return; }
        qint64 write = bankoutFile.write(fsbBuffer, _chunkSizes[k]);
        if (write == -1) { emit taskFinished("\nError writing fsb chunk: " + QString::number(k)); return; }
        free(fsbBuffer);
    }

    bankoutFile.flush();
    bankoutFile.close();
    fsbInFile.close();
    free(bankHeader);
}

QStringList RebuildWorker::readTextFileToQStringList(const QString& filePath) {
    QStringList stringList;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit taskFinished("\nCould not open file: " + filePath);
        return stringList; // Return empty list if file cannot be opened
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        stringList.append(line);
    }

    file.close();
    return stringList;
}
