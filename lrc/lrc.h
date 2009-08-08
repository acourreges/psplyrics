#ifndef LRC_H
#define LRC_H

#define LYRICS_ROOT "ms0:/LRC"

//debug string
extern char debug_string[1024];

//if a LRC file has been found
extern short lrc_available;

int init_lrc_for_file(char * file_name);
void update_state(int current_time);

#endif

