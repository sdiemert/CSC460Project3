#include <avr/io.h>
#include <util/delay.h>
#include "music_stream.h"

#include "rtos/os.h" // Task_Create_RR,Task_Create_Periodic, Service_Init
#include "roomba/roomba.h"
#include "roomba/roomba_sci.h"
#include "roomba/roomba_music.h"
#include "rtos/timer.h"


static music_stream_t music_stream;
static volatile OS_TIMER music_stream_timer;
static uint16_t _load_music(music_stream_t* music_stream);
static void _play_music();

// returns the duration of the song uploaded into the board.
uint16_t _load_music(music_stream_t* stream)
{
    roomba_music_song_t song;
    song.len = 0;
    song.song_num = 0;

    // add notes from the music stream into a roomba_music song struct
    while( song.len < ROOMBA_MUSIC_MAX_SONG_LEN &&
            stream->current_note < stream->len)
	{
        Roomba_Music_add_note(&song,
                                stream->notes[stream->current_note][0],
                                stream->notes[stream->current_note][1]);
        stream->current_note += 1;
    }

    // retrieve the duration of the music
    uint16_t duration = Roomba_Music_get_duration_of_song(&song);

    // load the song into roomba memory and then play the song
    // note that we are hard-coded to always using song number 0
    Roomba_Music_load_song(&song);
    Roomba_Music_play_song(0);

    return duration;
}


void _play_music2()
{
    uint16_t duration;
    for(;;){

        // poll the roomba to see if the last song has finished playing
        if(Roomba_Music_is_song_playing() == 0){

            if( music_stream.current_note < music_stream.len){

                // If we have more notes to play, then load them into the roomba
                duration = _load_music(&music_stream);

                if( duration > 15){
                    // don't bother if we only have to wait 15ms
                    duration -= 15;

                    uint16_t target_time = Now() + duration;
                    while( Now() - target_time < duration){
                        Task_Next();
                    }
                    duration = 0;
                }

            }else{
                // Reset the music stream. We are done now.
                music_stream.current_note = 0;
                music_stream.is_playing = 0;
                PORTC ^= ( 1 << PC5);
                Task_Terminate();
            }
        }


        Task_Next();
    }
}



void _play_music()
{
    uint16_t duration;
    for(;;){

        // poll the roomba to see if the last song has finished playing
        if(Roomba_Music_is_song_playing() == 0){

            if( music_stream.current_note < music_stream.len){

                // If we have more notes to play, then load them into the roomba
                duration = _load_music(&music_stream);

                if( duration > 15){
                    // don't bother if we only have to wait 15ms
                    duration -= 15;

                    timer_reset(&music_stream_timer);
                    timer_resume(&music_stream_timer);

                    // wait a certain duration before polling the roomba again
                    while(timer_value(&music_stream_timer) < duration){
                        // continue to yield until the specified amount of time
                        // has pased.
                        Task_Next();
                    }
                    duration = 0;
                }

            }else{
                // Reset the music stream. We are done now.
                music_stream.current_note = 0;
                music_stream.is_playing = 0;
                PORTC ^= ( 1 << PC5);
                Task_Terminate();
            }
        }


        Task_Next();
    }
}

/*
 *****************************
 * PUBLIC API
 *****************************
 */

void Music_Stream_init()
{
    music_stream.len = 0;
    music_stream.current_note = 0;
    music_stream.is_playing = 0;
}


void Music_Stream_play()
{
    if(music_stream.is_playing){return;}
    Task_Create_RR(_play_music,0);
}


// Add a note into the music stream.
void Music_Stream_add_note(uint8_t note, uint8_t duration)
{
    if( music_stream.len >= MUSIC_STREAM_LEN){return;}
    music_stream.notes[music_stream.len][0] = note;
    music_stream.notes[music_stream.len][1] = duration;
    music_stream.len += 1;
}

static uint16_t _strlen(const char* node){
    uint16_t rs = 0;
    while(*(node + rs) != '\0'){rs += 1;}
    return rs;
}

// Add a note into the music stream.
void Music_Stream_add_note_char(const char* note, uint8_t duration)
{
    if( music_stream.len >= MUSIC_STREAM_LEN){return;}

    // convert the note in string representation into the
    // the note number.
    uint16_t len = _strlen(note);
    uint8_t octave = (len == 2) ? note[1] - '0': note[2] -'0';
    uint8_t note_index = 0;
    switch(note[0]){
        case('c'): note_index = (len == 2) ? 0 : 1; break;
        case('d'): note_index = (len == 2) ? 2 : 3; break;
        case('e'): note_index = 4;break;
        case('f'): note_index = (len == 2) ? 5 : 6; break;
        case('g'): note_index = (len == 2) ? 7 : 8; break;
        case('a'): note_index = (len == 2) ? 9 : 10; break;
        case('b'): note_index = 11;break;
        case('r'): note_index = -(octave+1)*12; break; // rest note
    }
    uint8_t note_num = note_index + (octave + 1)*12;

    music_stream.notes[music_stream.len][0] = note_num;
    music_stream.notes[music_stream.len][1] = duration;
    music_stream.len += 1;
}