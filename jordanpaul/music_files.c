#include "music_files.h"
#include "../shared/music_stream.h"

void music_files_load(MUSIC_FILES_E song){
    Music_Stream_init();
    if( song == MUSIC_FILES_ZELDA_EPONA){
        Music_Stream_add_note(65, 16);
        Music_Stream_add_note(69, 16);
        Music_Stream_add_note(71, 24);
        Music_Stream_add_note(65, 16);
        Music_Stream_add_note(69, 16);
        Music_Stream_add_note(71, 24);
        Music_Stream_add_note(65, 16);
        Music_Stream_add_note(69, 16);
        Music_Stream_add_note(71, 16);
        Music_Stream_add_note(76, 16);
        Music_Stream_add_note(74, 24);
        Music_Stream_add_note(71, 16);
        Music_Stream_add_note(72, 16);
        Music_Stream_add_note(71, 16);
        Music_Stream_add_note(67, 16);
        Music_Stream_add_note(64, 36);

    }else if( song == MUSIC_FILES_ZELDA_TREASURE){
        // treasure chest opening song
        Music_Stream_init();
        Music_Stream_add_note_char("g2",24);
        Music_Stream_add_note_char("a2",24);
        Music_Stream_add_note_char("b2",24);
        Music_Stream_add_note_char("c#3",24);
        Music_Stream_add_note_char("g2",24);
        Music_Stream_add_note_char("a2",24);
        Music_Stream_add_note_char("b2",24);
        Music_Stream_add_note_char("c#3",24);
        Music_Stream_add_note_char("g#2",22);
        Music_Stream_add_note_char("a#2",22);
        Music_Stream_add_note_char("c3",22);
        Music_Stream_add_note_char("d3",22);
        Music_Stream_add_note_char("g#2",22);
        Music_Stream_add_note_char("a#2",22);
        Music_Stream_add_note_char("c3",22);
        Music_Stream_add_note_char("d3",22);
        Music_Stream_add_note_char("a2",18);
        Music_Stream_add_note_char("b2",18);
        Music_Stream_add_note_char("c#3",18);
        Music_Stream_add_note_char("d#3",18);
        Music_Stream_add_note_char("a2",18);
        Music_Stream_add_note_char("b2",18);
        Music_Stream_add_note_char("c#3",18);
        Music_Stream_add_note_char("d#3",18);
        Music_Stream_add_note_char("a#2",14);
        Music_Stream_add_note_char("c3",14);
        Music_Stream_add_note_char("d3",14);
        Music_Stream_add_note_char("e3",14);
        Music_Stream_add_note_char("a#2",14);
        Music_Stream_add_note_char("c3",14);
        Music_Stream_add_note_char("d3",14);
        Music_Stream_add_note_char("e3",14);
        Music_Stream_add_note_char("b2",12);
        Music_Stream_add_note_char("c#3",12);
        Music_Stream_add_note_char("d#3",12);
        Music_Stream_add_note_char("f3",12);
        Music_Stream_add_note_char("c3",10);
        Music_Stream_add_note_char("d3",10);
        Music_Stream_add_note_char("e3",10);
        Music_Stream_add_note_char("f#3",10);
        Music_Stream_add_note_char("c#3",8);
        Music_Stream_add_note_char("d#3",8);
        Music_Stream_add_note_char("f3",8);
        Music_Stream_add_note_char("g3",8);
        Music_Stream_add_note_char("d3",6);
        Music_Stream_add_note_char("e3",6);
        Music_Stream_add_note_char("f#3",6);
        Music_Stream_add_note_char("g#3",6);
        Music_Stream_add_note_char("a4",26);
        Music_Stream_add_note_char("a#4",26);
        Music_Stream_add_note_char("b4",26);
        Music_Stream_add_note_char("c5",128);
    }
}