#ifndef MUSIC_STREAM_H
#define MUSIC_STREAM_H

#define MUSIC_STREAM_LEN 128
typedef struct _music_stream_song_t{
    uint8_t len;
    uint8_t current_note;
    uint8_t is_playing;
    uint8_t notes[MUSIC_STREAM_LEN][2];

} music_stream_t;

/*
    Currently only one music stream can exist at one time.
    It uses song num 0 to store the song.
    Internally this api modifies an internal music_stream_t struct as well
        as creates some interal tasks ( 1 rr, and 1 periodic)

    First call Music_Stream_init() in order to empty out the music stream struct.
    Call Music_Stream_add_note() and add all the notes of the song into the music stream.
    Then call Music_Stream_play(). This will spawn a RR task which constantly
        polls the roomba watching for when the roomba stops playing the song.
        If the song stops playing, but there are more notes in the music stream,
        the task will load the next song into the roomba and continue playing.

    TODO: Make this not shit, by allowing the user to create a music_stream_t
        and pass it to the api.The only reason I need to do it this
        way is because my internal RR task needs someway to access the memory of
        of the music_stream struct
*/

/*
    Clear out the internal music_stream struct
*/
void Music_Stream_init();

/*
    Start playing the music stream.
*/
void Music_Stream_play();

/*
    Add a note into the music stream.
    note - the code defined by the roomba manual(pg. 19)
    duration (0-255) - each value is 1/64 of a second.
*/
void Music_Stream_add_note(uint8_t note, uint8_t duration);

/*
    Add a note into the music stream.
    note - The note specified in ascii format (c#3,a2,etc)
    duration (0-255) - each value is 1/64 of a second.
*/
void Music_Stream_add_note_char(const char* note, uint8_t duration);

#endif
