#include "extract_worker.h"
#include "fileio.h"
#include "bank_extract.h"

ExtractWorker::ExtractWorker(QObject *parent) : QObject(parent) {}

void ExtractWorker::extract_fsb()
{
    FMOD_SYSTEM *system = NULL;
    FMOD_SOUND  *sound = NULL, *sound_to_play = NULL;
    FMOD_RESULT       result;
    FMOD_SOUND_TYPE   stype;
    FMOD_SOUND_FORMAT sformat;
    FMOD_CREATESOUNDEXINFO exinfo;

    unsigned int     length = 0, dataLen = 0, nameLength = 64;
    void             *extradriverdata = 0;
    int              schannels = 0, numsubsounds = 0, sbits = 0, priority = 0;
    char             subsoundsName[64];
    char*            buffer = 0;
    float            ssamplerate = 0;

    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Directorys");
    QString fsbDir = QCoreApplication::applicationDirPath() + "/fsb/";
    QString bankDir = settings.value("BankDir").toString() + "/";
    QString wavDir = settings.value("WavDir").toString() + "/";
    settings.endGroup();

    QDir bank_directory(bankDir);
    QStringList nameBankFilters;
    nameBankFilters << "*.bank";
    QStringList bankFileList = bank_directory.entryList(nameBankFilters);

    QStringList nameTXTFilters;
    nameTXTFilters << "*.txt";
    QStringList bankPasswordFileList = bank_directory.entryList(nameTXTFilters);

    for (int i = 0; i < bankFileList.count(); i++)
    {
        QString bankPath = bankDir + bankFileList[i];
        QStringList txtFileNames;

        int errorCheck = bank_extract::extract(bankPath);

        memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
        exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
        exinfo.length = 0;

        QFileInfo bankFileInfo(bankPath);

        QString newLineCheck = (i == 0) ? "" : "\n";
        emit updateConsole(newLineCheck + "Initializing Fmod Bank file - " + bankFileInfo.fileName() + "\n"); // Emit signal to update console in UI

        char* encryption = 0;

        if (errorCheck == 0)
        {
            emit taskFinished("Error extracting bank file !!!");
            emit progressUpdated(0);
            return;
        }
        else if (errorCheck == 2)
        {
            emit taskFinished("Error, can't find any fsb audio in this bank file !!!");
            emit progressUpdated(0);
            return;
        }
        else if (errorCheck == 5)
        {
            if (bankPasswordFileList.size() != 0)
            {
                QString password = "";

                foreach (const QString _pswTxt, bankPasswordFileList)
                {
                    if (_pswTxt.contains(bankFileList[i].replace(".bank", "")))
                    {
                        QStringList tmp = readTextFileToQStringList(bankPath.replace(".bank", ".txt"));
                        password = tmp[0];
                    }
                }

                QByteArray encryptionkeyArray = password.toUtf8();
                encryption = new char[encryptionkeyArray.size() + 1];
                strcpy(encryption, encryptionkeyArray.constData());
                exinfo.encryptionkey = encryption;

                if (!password.isEmpty())
                    emit emit updateConsole("Decrypting bank file with password: " + encryptionkeyArray + "\n");
                else
                    emit emit updateConsole("Can't find " + bankFileList[i].replace(".bank", ".txt") + " with password for decryption.\n");
            }
            else
            {
                emit emit updateConsole("Can't find " + bankFileList[i].replace(".bank", ".txt") + " with password for decryption.\n");
            }
        }

        QString fsbPath = fsbDir + bankFileInfo.fileName().replace(".bank", ".fsb");
        QFileInfo fsbFileInfo(fsbPath);

        if (!fsbFileInfo.exists())
        {
            emit taskFinished("Error, fsb file is missing !!!");
            emit progressUpdated(0);
            return;
        }

        QDir dir(wavDir);
        dir.mkdir(bankFileInfo.fileName().replace(".bank", ""));

        /*
         Create a System object and initialize
        */
        result = FMOD_System_Create(&system);
        if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); return; }

        //result = system->getVersion(&version);
        //if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); return; }

        result = FMOD_System_Init(system, 1, FMOD_INIT_NORMAL, extradriverdata);
        if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); return; }

        result = FMOD_System_CreateSound(system, fsbPath.toUtf8().constData(), FMOD_OPENONLY, &exinfo, &sound);
        if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); return; }

        result = FMOD_Sound_GetNumSubSounds(sound, &numsubsounds);
        if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); return; }

        for (int j = 0; j < numsubsounds; j++)
        {
            result = FMOD_Sound_GetSubSound(sound, j, &sound_to_play);
            if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); emit progressUpdated(0); return; }

            result = FMOD_Sound_SeekData(sound_to_play, 0);
            if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); emit progressUpdated(0); return; }

            result = FMOD_Sound_GetDefaults(sound_to_play, &ssamplerate, &priority);
            if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); emit progressUpdated(0); return; }

            result = FMOD_Sound_GetFormat(sound_to_play, &stype, &sformat, &schannels, &sbits);
            if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); emit progressUpdated(0); return; }

            result = FMOD_Sound_GetLength(sound_to_play, &length, FMOD_TIMEUNIT_PCMBYTES);
            if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); emit progressUpdated(0); return; }

            result = FMOD_Sound_GetName(sound_to_play, subsoundsName, nameLength);
            if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); emit progressUpdated(0); return; }

            QString subsoundName = QString::fromUtf8(subsoundsName);

            if (subsoundName.isEmpty())
                subsoundName = "sound_" + QString::number(j);

            QString wavName = wavDir + bankFileInfo.fileName().replace(".bank", "") + "/" + subsoundName + ".wav";

            QFile file(wavName);
            file.open(QIODevice::WriteOnly);
            writeWAVHeader(file, ssamplerate, sbits, schannels, length);

            unsigned int chunkCount = chunkAmount(length);
            std::vector<long> _chunkSizes = chunkSizes(length, chunkCount);

            for (unsigned int k = 0; k < chunkCount; k++)
            {
                buffer = (char*)malloc(_chunkSizes[k]);
                result = FMOD_Sound_ReadData(sound_to_play, buffer, _chunkSizes[k], &dataLen);
                if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); emit progressUpdated(0); return; }

                if (buffer != 0)
                {
                    file.write(buffer, _chunkSizes[k]);
                }
                else
                {
                    emit taskFinished("Error reading wav data chunks !!!");
                    emit progressUpdated(0);
                    qInfo() << "Error reading wav data chunks !!!";
                    return;
                }

                free(buffer);
            }

            file.flush();
            file.close();

            int subSoundsPercent = 100 * (j + 1) / numsubsounds;
            emit progressUpdated(subSoundsPercent); // Emit signal to update progress in UI
            QString index = QString::number(j);
            txtFileNames << subsoundName + ".wav";
            emit updateConsole(index + ": (" + subsoundName + ".wav) [Extracting]"); // Emit signal to update console in UI
            //qInfo() << "Bit Depth: " + QString::number(sbits);
        }

        free(extradriverdata);
        delete[] encryption;
        FMOD_Sound_Release(sound_to_play);
        FMOD_Sound_Release(sound);
        FMOD_System_Release(system);

        writeFilenamesToFile(txtFileNames, wavDir + bankFileInfo.fileName().replace(".bank", "") + "/" + bankFileInfo.fileName().replace(".bank", ".txt"));
    }

    if (bankFileList.count() != 0)
    {
        emit taskFinished("\nExtracting Bank files has finished."); // Signal completion
        emit progressUpdated(0);
        qInfo() << "Extracting Bank files has finished.";
    }
    else
    {
        emit taskFinished("No bank files found."); // Signal completion
        emit progressUpdated(0);
        qInfo() << "No bank files found.";
    }

}

void ExtractWorker::writeFilenamesToFile(const QStringList &filenames, const QString &outputFilePath) {
    QFile file(outputFilePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &filename : filenames) {
            out << filename << "\n"; // Write each filename followed by a newline
        }
        file.close();
        qDebug() << "Filenames successfully written to" << outputFilePath;
    } else {
        qDebug() << "Error opening file for writing:" << file.errorString();
    }
}

void ExtractWorker::writeWAVHeader(QFile& file, unsigned int sampleRate, short bitsPerChannel, short numChannels, unsigned int dataLen)
{
    const unsigned int fmtChunkLen = 18;
    const unsigned short formatType = 1;
    short bytesPerSample = bitsPerChannel / 8;
    unsigned int bytesPerSecond = sampleRate * numChannels * bytesPerSample;
    unsigned int headerLen = dataLen + 38;

    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&headerLen), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    file.write(reinterpret_cast<const char*>(&fmtChunkLen), 4);
    file.write(reinterpret_cast<const char*>(&formatType), 2);
    file.write(reinterpret_cast<const char*>(&numChannels), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    file.write(reinterpret_cast<const char*>(&bytesPerSecond), 4);
    file.write(reinterpret_cast<const char*>(&bytesPerSample), 2);
    file.write(reinterpret_cast<const char*>(&bitsPerChannel), 4);
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataLen), 4);
}

QStringList ExtractWorker::readTextFileToQStringList(const QString& filePath) {
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
