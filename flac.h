#ifndef __FLAC__
#define __FLAC__

#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <netinet/in.h>

struct {

} typedef MetadataBlockHeader;

printHeader(MetadataBlockHeader * header) {
    // printf("LastBlock: %u\n", header->lastBlock); 
    // printf("Block type: %u\n", header->blockType); 
    // printf("Block length: %u\n", htonl(header->length)); 
}


struct {
    uint16_t minimumBlockSize;
    uint16_t maximumBlockSize;
    unsigned int maximumFrameSize: 24;
    unsigned int minimumFrameSize: 24;
    unsigned int sampleRateHz: 20;
    unsigned int numberOfChannels: 3;
    unsigned int bitsPerSample: 5;
    unsigned int totalSamplesInStream;
    uint16_t md5Signature;
} typedef MetadataBlockStreamInfo;

struct {
    char marker[8];
    MetadataBlockStreamInfo metadataBlockStreamInfo;
} typedef FlacFile;

void printInt(unsigned int i) {
    printf("%u\n", ntohs(i));
}

void printMetadataBlockStreamInfo(MetadataBlockStreamInfo * ptr) {
    printInt(ptr->minimumBlockSize);
    printInt(ptr->maximumBlockSize);
    printInt(ptr->minimumFrameSize);
    printInt(ptr->maximumFrameSize);
    printInt(ptr->sampleRateHz);
    printInt(ptr->numberOfChannels);
    printInt(ptr->bitsPerSample);
    printInt(ptr->totalSamplesInStream);
    printInt(ptr->md5Signature);
}

#endif