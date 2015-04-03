#include <avr/io.h>
#include <util/delay.h>
#include "rtos/os.h" // Task_Create_RR
#include "roomba/roomba.h"
#include "roomba/roomba_sci.h"
#include "roomba/roomba_music.h"
#include "music_stream.h"
#include "profiler.h"

static music_stream_t music_stream;
static SERVICE* music_stream_wait_service = 0;

static uint16_t _load_music(music_stream_t* music_stream);
static void _p_wait_music_stream();
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

// A simple periodic task which all that it does is
// publish to the music_stream_wait_service and then terminate.
// The purpose of the task is in order to time a certain duration
// before waking up the round-robinn _play_music task.
void _p_wait_music_stream()
{
    Task_Next();
    Service_Publish(music_stream_wait_service,0);
    Task_Terminate();
}

void _play_music()
{
    uint16_t duration;
    int16_t value;
    for(;;){
        PORTB |= (1 << PB6);

        // poll the roomba to see if the last song has finished playing
        if(Roomba_Music_is_song_playing() == 0){

            if( music_stream.current_note < music_stream.len){

                // If we have more notes to play, then load them into the roomba
                duration = _load_music(&music_stream);

                if( duration > 30){
                    // -6 TICKS = 30ms
                    // Create a periodic task which will wake up this task once the
                    // specified duration has passed. We do this so that we aren't just
                    // constantly polling the Roomba to see if the song is still playing.
                    Task_Create_Periodic(_p_wait_music_stream,0,duration-6,50,Now() + 1);
					Service_Subscribe(music_stream_wait_service,&value);
                }

            }else{
                // Reset the music stream. We are done now.
                music_stream.current_note = 0;
                music_stream.is_playing = 0;
                Task_Terminate();
            }
        }
        PORTB &= (1 << PB6);
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
    if( music_stream_wait_service != 0){
        music_stream_wait_service = Service_Init();
    }

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