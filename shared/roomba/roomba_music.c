#include "roomba_music.h"
#include "roomba_sci.h"
#include "../uart/uart.h"
#include "roomba.h"
#include "../rtos/os.h"

void Roomba_Music_add_note(roomba_music_song_t* song,uint8_t note, uint8_t duration)
{
    if( song->len >= ROOMBA_MUSIC_MAX_SONG_LEN){return;}
    song->notes[song->len][0] = note;
    song->notes[song->len][1] = duration;
    song->len += 1;
}

#define ROOMBA_MUSIC_MULT 16 // 16 ms
uint16_t Roomba_Music_get_duration_of_song(roomba_music_song_t* song)
{
    uint16_t song_len = 0;
    uint8_t i = 0;
    for(i = 0;i < song->len; ++i){
        song_len += song->notes[i][1];
    }
    // return (song_len*64)/1000;
    return song_len * ROOMBA_MUSIC_MULT;
    // return ((song_len*100)/64)*10;
}

/*
*   Load the given roomba song into the roomba.
*/
void Roomba_Music_load_song(roomba_music_song_t* song){
    Roomba_Send_Byte(ROOMBA_MUSIC_LOAD_SONG_OPCODE);
    Roomba_Send_Byte(song->song_num);
    Roomba_Send_Byte(song->len);
    int8_t i = 0;
    for(i = 0;i < song->len;++i){
        Roomba_Send_Byte(song->notes[i][0]);
        Roomba_Send_Byte(song->notes[i][1]);
    }
}

void Roomba_Music_play_song(uint8_t song_num)
{
    Roomba_Send_Byte(ROOMBA_MUSIC_PLAY_SONG_OPCODE);
    Roomba_Send_Byte(song_num);
}

uint8_t Roomba_Music_is_song_playing()
{
    Roomba_Send_Byte(SENSORS);
    Roomba_Send_Byte(ROOMBA_MUSIC_ISPLAYING_PACKET_CODE);


    // maximum wait is 100 ms
    uint16_t T = Now() + 100;
    while( uart_bytes_received() != 1){
        if( Now() - T >= 100){
            uart_reset_receive();
            // NOT GOOD, but atleast we don't hang forever..
            return 0;
        }
    }

    uint8_t rs = uart_get_byte(0);
    uart_reset_receive();
    return rs;
}

uint8_t Roomba_Music_current_song()
{
    Roomba_Send_Byte(SENSORS);
    Roomba_Send_Byte(ROOMBA_MUSIC_CURRENT_SONG_PACKET_CODE);

    uint16_t T = Now() + 100;
    while( uart_bytes_received() != 1){
        if( Now() - T >= 100){
            uart_reset_receive();
            return 0;
        }
    }
    uint8_t rs = uart_get_byte(0);
    uart_reset_receive();
    return rs;
}