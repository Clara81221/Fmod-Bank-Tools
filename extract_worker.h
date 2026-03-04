#ifndef EXTRACT_WORKER_H
#define EXTRACT_WORKER_H

#include <QObject>
#include <QThread>
#include <QFile>
#include <QDir>
#include <qsettings.h>
#include <fmod_errors.h>
#include <cstring>


class ExtractWorker : public QObject
{
    Q_OBJECT
public:
    explicit ExtractWorker(const QString &password,
                           bool overrideEnabled,
                           QObject *parent = nullptr);

private:
    QString m_password;
    bool m_overrideEnabled;

    bool handleExtractionError(int errorCheck, const QString &bankFile, QString bankPath, FMOD_CREATESOUNDEXINFO &exinfo);
    bool handlePasswordProtectedBank(QString bankPath, FMOD_CREATESOUNDEXINFO &exinfo);
    bool processSubSounds(FMOD_SOUND *sound, QFileInfo bankFileInfo, const QString &wavDir, quint32 fsbIndex);
    void writeFilenamesToFile(const QStringList &filenames, const QString &outputFilePath);
    void writeWAVHeader(QFile& file, unsigned int sampleRate, short bitsPerChannel, short numChannels, unsigned int dataLen);
    QStringList readTextFileToQStringList(const QString& filePath);

public slots:
    void extract_fsb();

signals:
    void progressUpdated(int value);
    void updateConsole(QString result);
    void taskFinished(QString result);
};

#endif // EXTRACT_WORKER_H
