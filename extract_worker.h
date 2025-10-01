#ifndef EXTRACT_WORKER_H
#define EXTRACT_WORKER_H

#include <QObject>
#include <QThread>
#include <QFile>
#include <QDir>
#include <qsettings.h>
#include <fmod_errors.h>


class ExtractWorker : public QObject
{
    Q_OBJECT
public:
    explicit ExtractWorker(QObject *parent = nullptr);

private:
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
