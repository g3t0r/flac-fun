#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "flac.h"
#include "arpa/inet.h"

int main(int argc, char* argv) {
    FILE * fd = fopen("audio/mount-shrine_all-roads-lead-home.flac", "rb");
    if (fd == 0) {
        printf("Problem opening file");
        exit(1);
    }

    char marker[4];
    fread(&marker, sizeof(marker), 1, fd);

    MetadataBlockHeader header;
    uint32_t rawHeader;

    fread(&rawHeader, 32, 1, fd);
    rawHeader = htonl(rawHeader);
    uint32_t lastBitMask = 1 << (32 - 1);
    uint32_t blockTypeMask = 127 << (32 - 7 - 1);
    uint32_t lengthMask = (2<<23)-1;

    int lastBit = (rawHeader & lastBitMask) >> 31;
    printf("Last block: %d\n", lastBit);
    int blockType = (rawHeader & lastBitMask) >> 24;
    printf("block type: %d\n", blockType);
    int length = (rawHeader & lengthMask);
    printf("length: %d\n", length);

    // printHeader(&header);

    // MetadataBlockStreamInfo streamInfo;
    // fread(&streamInfo, 1, header.length, fd);
    // printMetadataBlockStreamInfo(&streamInfo);

}