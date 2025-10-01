#include "fileio.h"
#include <qdebug.h>

uint32_t chunkAmount(uint32_t unCompressedSize, uint32_t chunkSize) {
    uint32_t chunkCount = 0;
    double chunkSizeDec = static_cast<double>(unCompressedSize) / static_cast<double>(chunkSize);

    if (unCompressedSize < chunkSize) {
        chunkCount = 1;
    } else if (std::round(chunkSizeDec) == chunkSizeDec) {
        chunkCount = unCompressedSize / chunkSize;
    } else {
        chunkCount = (unCompressedSize / chunkSize) + 1;
    }
    return chunkCount;
}

std::vector<long> chunkSizes(int unCompressedSize, int chunkAmount, int chunkSize) {
    std::vector<long> chunkS(chunkAmount);
    double chunkSizeDec = static_cast<double>(unCompressedSize) / chunkSize;

    if (unCompressedSize < chunkSize) {
        chunkS[0] = unCompressedSize;
    } else if (std::round(chunkSizeDec) == chunkSizeDec) {
        for (int i = 0; i < chunkAmount; i++) {
            chunkS[i] = chunkSize;
        }
    } else {
        for (int i = 0; i < chunkAmount - 1; i++) {
            chunkS[i] = chunkSize;
        }

        int chunk = unCompressedSize / chunkSize;
        int lastChunkSize = chunkSize * chunk;
        chunkS[chunkAmount - 1] = unCompressedSize - lastChunkSize;
    }
    return chunkS;
}
