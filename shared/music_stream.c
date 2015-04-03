#include <avr/io.h>
#include <util/delay.h>
#include "rtos/os.h" // Task_Create_RR
#include "roomba/roomba.h"
#include "roomba/roomba_sci.h"
#include "roomba/roomba_music.h"
#include "music_stream.h"

static music_stream_t music_stream;
static void _load_music(music_stream_t* music_stream);
static void _play_music();

void _load_music(music_stream_t* stream)
{
    roomba_music_song_t song;
    song.len = 0;
    song.song_num = 0;

    while( song.len < ROOMBA_MUSIC_MAX_SONG_LEN &&
            stream->current_note < stream->len)
	{
        Roomba_Music_add_note(&song,
                                stream->notes[stream->current_note][0],
                                stream->notes[stream->current_note][1]);
        stream->current_note += 1;
    }

    Roomba_Music_load_song(&song);
    Roomba_Music_play_song(0);
}

void _play_music()
{
    for(;;){
        if(Roomba_Music_is_song_playing() == 0){
            if( music_stream.current_note < music_stream.len){
                _load_music(&music_stream);
            }else{
                PORTB ^= (1 << PB6);
                music_stream.current_note = 0;
                music_stream.is_playing = 0;
                Task_Terminate();
            }
        }
        Task_Next();
    }
}

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

void Music_Stream_add_note(uint8_t note, uint8_t duration)
{
    if( music_stream.len >= MUSIC_STREAM_LEN){return;}
    music_stream.notes[music_stream.len][0] = note;
    music_stream.notes[music_stream.len][1] = duration;
    music_stream.len += 1;
}