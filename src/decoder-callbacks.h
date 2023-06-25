#ifndef DECODER_CALLBACKS_H_
#define DECODER_CALLBACKS_H_

#include "FLAC/stream_decoder.h"
#include "FLAC/stream_encoder.h"
#include <FLAC/format.h>
#include <stddef.h>
#include <stdint.h>

struct {
  FLAC__StreamMetadata **blocks;
  uint32_t numberOfBlocks;
  uint32_t allocatedBlocks;
} typedef DecoderClientDataMetadata;

struct {
  FLAC__StreamEncoder *encoder;
  uint64_t totalSamples;
  DecoderClientDataMetadata metadata;
} typedef DecoderClientData;

FLAC__StreamDecoderWriteStatus writeCallback(const FLAC__StreamDecoder *decoder,
                                             const FLAC__Frame *frame,
                                             const FLAC__int32 *const buffer[],
                                             void *client_data);

void metadataCallback(const FLAC__StreamDecoder *decoder,
                      const FLAC__StreamMetadata *metadata, void *client_data);

void errorCallback(const FLAC__StreamDecoder *decoder,
                   FLAC__StreamDecoderErrorStatus status, void *client_data);

#endif // DECODER_CALLBACKS_H_
