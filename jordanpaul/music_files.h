#ifndef MUSIC_FILES_H
#define MUSIC_FILES_H

typedef enum _music_files_e
{
    MUSIC_FILES_ZELDA_EPONA= 0,
    MUSIC_FILES_ZELDA_TREASURE,
} MUSIC_FILES_E;

void music_files_load(MUSIC_FILES_E song);

#endif
