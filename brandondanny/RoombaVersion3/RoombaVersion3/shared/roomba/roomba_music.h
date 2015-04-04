#ifndef ROOMBA_MUSIC_H_
#define ROOMBA_MUSIC_H_

#include <avr/io.h>

#define ROOMBA_MUSIC_ISPLAYING_PACKET_CODE 37
#define ROOMBA_MUSIC_CURRENT_SONG_PACKET_CODE 36
#define ROOMBA_MUSIC_LOAD_SONG_OPCODE 140
#define ROOMBA_MUSIC_PLAY_SONG_OPCODE 141
#define ROOMBA_MUSIC_MAX_SONG_LEN 16

typedef struct _roomba_music_song_t {
    uint8_t song_num;
    uint8_t len;
    uint8_t notes[ROOMBA_MUSIC_MAX_SONG_LEN][2];
} roomba_music_song_t;

/*
    Add the following note with the given duration into the song struct.
    If it can't add anymore notes then it fails silently.
    note - the code defined by the roomba manual(pg. 19)
    duration (0-255) - each value is 1/64 of a second.
*/
void Roomba_Music_add_note(roomba_music_song_t* song,uint8_t note, uint8_t duration);

/**
    Returns the length of the song in milliseconds.
*/
uint16_t Roomba_Music_get_duration_of_song(roomba_music_song_t* song);

/*
*   Load the given roomba song into the roomba.
*/
void Roomba_Music_load_song(roomba_music_song_t* song);

/*
*   Send the command to the roomba to start playing the specified song.
*/
void Roomba_Music_play_song(uint8_t song_num);

/* Query the roomba to see if a song is currently being played/
    Return  1 for yes
            0 for no
*/
uint8_t Roomba_Music_is_song_playing();

/*
    If the roomba is playing a song, return the number of the song being played.
    Return 0 - 4
*/
uint8_t Roomba_Music_current_song();


#endif