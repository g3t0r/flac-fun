#ifndef ENCODER_CALLBACKS_H_
#define ENCODER_CALLBACKS_H_

#include "FLAC/stream_encoder.h"


void progressCallback(const FLAC__StreamEncoder *encoder,
                      FLAC__uint64 bytes_written, FLAC__uint64 samples_written,
                      uint32_t frames_written, uint32_t total_frames_estimate,
                      void *client_data);



#endif // ENCODER_CALLBACKS_H_
