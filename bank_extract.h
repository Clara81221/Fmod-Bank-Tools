#ifndef BANK_EXTRACT_H
#define BANK_EXTRACT_H

#include <QFile>
#include <QApplication>

class bank_extract
{
public:
    static int extract(QString bankPath);

private:
    bank_extract() = delete;
    bank_extract(const bank_extract&) = delete;
    bank_extract& operator=(const bank_extract&) = delete;
};

#endif // BANK_EXTRACT_H
