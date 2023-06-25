#include "encoder-callbacks.h"

void progressCallback(const FLAC__StreamEncoder *encoder,
                      FLAC__uint64 bytes_written, FLAC__uint64 samples_written,
                      uint32_t frames_written, uint32_t total_frames_estimate,
                      void *client_data) {
  float progress = (float)frames_written / (float) total_frames_estimate * 100.0;
  printf("Progress: %f.2\n", progress);
}
