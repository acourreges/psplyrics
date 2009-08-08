#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include "../gfx/graphics.h"
#include "file_struct.h"

/* Defines */

#define MAX_DISPLAY			14 // change this accordingly (max amount of files you want to display on-screen)

#define DISPLAY_X			8 // change this accordingly (X value of where you want the filebrowser displayed)
#define DISPLAY_Y			25 // change this accordingly (Y value of where you want the filebrowser displayed)

#define FILE_TYPE_UNKNOWN	-1
#define FILE_TYPE_MP3		0
#define FILE_TYPE_EBOOT		1
#define FILE_TYPE_PNG		2
#define FILE_TYPE_PMP		3
#define FILE_TYPE_ISO		4
#define FILE_TYPE_CSO		5
#define FILE_TYPE_JPG		6
#define FILE_TYPE_DOTDOT    7
#define FILE_TYPE_FOLDER	8

#define MAX_FILENAME_STR 35

#define SORT_TITLE 	  0
#define SORT_GENRE 	  1
#define SORT_ALBUM    2
#define SORT_ARTIST   3
#define SORT_YEAR     4

#define MP3_ROOT "ms0:/MUSIC"

#define printf	pspDebugScreenPrintf

/* Globals */

extern int current;
extern int file_index_current_mp3;
extern int num_files_cwd;
extern struct FILE_INFO *files;
extern int sort_method;

/* Prototypes */

void scan_mp3_files(void);
struct FILE_INFO *run_file(void);
void move_up(int n);
void move_down(int n);
void display_mp3_files(void);
char *commas(unsigned long n);
int get_file_type(char *file_name);
char *compact_str(char *s, int max_length);
void sort_mp3_files(void);

#endif
