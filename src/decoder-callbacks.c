#include "decoder-callbacks.h"
#include <FLAC/format.h>
#include <stdint.h>
#include <stdlib.h>

FLAC__StreamDecoderWriteStatus writeCallback(const FLAC__StreamDecoder *decoder,
                                             const FLAC__Frame *frame,
                                             const FLAC__int32 *const buffer[],
                                             void *client_data) {
  return 0;
}

void metadataCallback(const FLAC__StreamDecoder *decoder,
                      const FLAC__StreamMetadata *metadata, void *client_data) {
  DecoderClientData *decoderClientData = (DecoderClientData *)client_data;
  DecoderClientDataMetadata *clientDataMetadata = &decoderClientData->metadata;
  clientDataMetadata->numberOfBlocks++;
  if (clientDataMetadata->allocatedBlocks <
      clientDataMetadata->numberOfBlocks) {

    clientDataMetadata->allocatedBlocks =
        (uint32_t)2 * clientDataMetadata->numberOfBlocks;

    clientDataMetadata->blocks = reallocarray(
        clientDataMetadata->blocks, clientDataMetadata->allocatedBlocks,
        sizeof(FLAC__StreamMetadata *));
  }
}
