#ifndef MUSIC_STREAM_H
#define MUSIC_STREAM_H

#define MUSIC_STREAM_LEN 128
typedef struct _music_stream_song_t{
    uint8_t len;
    uint8_t current_note;
    uint8_t is_playing;
    uint8_t notes[MUSIC_STREAM_LEN][2];

} music_stream_t;

void Music_Stream_init();
void Music_Stream_play();
void Music_Stream_add_note(uint8_t note, uint8_t duration);

#endif
