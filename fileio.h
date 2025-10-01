#ifndef FILEIO_H
#define FILEIO_H
#include <QFile>
#include <QDebug>
#include <cstdint>
#include <cmath>
#include <vector>

uint32_t chunkAmount(uint32_t unCompressedSize, uint32_t chunkSize = 262144);
std::vector<long> chunkSizes(int unCompressedSize, int chunkAmount, int chunkSize = 262144);

#endif // FILEIO_H
