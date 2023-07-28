#ifndef __FFUN__PLAYBACK__
#define __FFUN__PLAYBACK__

/* ----- GENERAL IDEA -----
 *
 * In separate thread request new data from the server as needed.
 *    Would be the best to use external callback for it, to abstract
 *    data retrieval from the playback
 *
 * In main thread, process stream_decoder until end of the file.
 *    - Read callback should read from circle buffer feeded by
 *      the separate thread
 *
 *    - Metadata callback should save metadata as it will be required later
 *      by a write callback
 *
 *    - Write callback should convert flac decoded frame to raw audio data,
 *      and then play it using ao_play
 *
 *
 *
 * */

#endif
