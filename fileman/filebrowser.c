#ifndef _FILEBROWSE_C
#define _FILEBROWSE_C

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <string.h>
#include <stdio.h>
#include <pspsdk.h>
#include <unistd.h>
#include <pspiofilemgr.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>

#include "filebrowser.h"
#include "../audio/mp3playerME.h"
#include "../gfx/graphics.h"

/* Globals */

struct FILE_INFO *files = NULL;

int num_files_cwd;
int current;
int curScroll;
int file_index_current_mp3 = 0;
int sort_method = SORT_TITLE;

/* Functions */

int get_file_type(char *file_name) {
	int i;
	char *suffix = NULL;
	char *types[7] = {"MP3", "PBP", "PNG", "PMP", "ISO", "CSO", "JPG"};
	
	suffix = strrchr(file_name, '.');
		if(suffix != NULL)
			suffix++; // Skip . and point to the 3 character suffix
		else
			return FILE_TYPE_UNKNOWN;
 
	for(i = 0; i < 3; i++)
		suffix[i] = toupper(suffix[i]);
	
	for(i=0; i<7; i++)
		if(strcmp(suffix, types[i]) == 0)
			return i;

	return FILE_TYPE_UNKNOWN;
} 

char *compact_str(char *s, int max_length) {
	char *suffix;
	char t[max_length+1];
 
	if(strlen(s) > max_length) {
		suffix = strrchr(s, '.');
			if(suffix != NULL) {			
				strncpy(t, s, max_length-4);
				t[max_length-4] = '\0';
				s = strcat(t, suffix);   	
			} else {
				strncpy(t, s, max_length-1);
				t[max_length] = '\0';
				strcpy(s, t);
			}
	}

	return s;
}

char *commas(unsigned long n)
{
	static int comma = '\0';
	static char retbuf[30];
	char *p = &retbuf[sizeof(retbuf)-1];
	int i = 0;

	if(comma == '\0') {
		struct lconv *lcp = localeconv();
		if(lcp != NULL) {
			if(lcp->thousands_sep != NULL &&
				*lcp->thousands_sep != '\0')
				comma = *lcp->thousands_sep;
			else	comma = ',';
		}
	}

	*p = '\0';

	do {
		if(i%3 == 0 && i != 0)
			*--p = comma;
		*--p = '0' + n % 10;
		n /= 10;
		i++;
	} while(n != 0);

	return p;
}

int allocate_files_array(void) {

	SceIoDirent dir;
	int total_files = 0;
	void *p = NULL;
		
	SceUID fd = sceIoDopen(MP3_ROOT);
		if(fd < 0)
			return -1;

	sceIoDread(fd, &dir); // Read .
	sceIoDread(fd, &dir); // Read ..
		
	while(sceIoDread(fd, &dir) > 0) {
		if((dir.d_stat.st_attr & FIO_SO_IFDIR) == 0)
			total_files++;
	}
  
  	sceIoDclose(fd);

	if(files == NULL) {
		p = realloc((void *)files, total_files * sizeof(struct FILE_INFO));
		files = (struct FILE_INFO *)p;
	}
	else
		files = malloc(total_files * sizeof(struct FILE_INFO));

	if(files == NULL) {
		printf("Failed to allocate malloc");
	    sceKernelSleepThread();
	}
		
	return total_files;
}

int sort_by_title(const void *a, const void *b) {

	const struct FILE_INFO *file1 = (struct FILE_INFO *)a;
	const struct FILE_INFO *file2 = (struct FILE_INFO *)b;

	return strcmp(file1->mp3Info.ID3.ID3Title, file2->mp3Info.ID3.ID3Title);
}

int sort_by_genre(const void *a, const void *b) {

	const struct FILE_INFO *file1 = (struct FILE_INFO *)a;
	const struct FILE_INFO *file2 = (struct FILE_INFO *)b;

	return strcmp(file1->mp3Info.ID3.ID3GenreText, file2->mp3Info.ID3.ID3GenreText);
}

int sort_by_album(const void *a, const void *b) {

	const struct FILE_INFO *file1 = (struct FILE_INFO *)a;
	const struct FILE_INFO *file2 = (struct FILE_INFO *)b;

	return strcmp(file1->mp3Info.ID3.ID3Album, file2->mp3Info.ID3.ID3Album);
}

int sort_by_artist(const void *a, const void *b) {

	const struct FILE_INFO *file1 = (struct FILE_INFO *)a;
	const struct FILE_INFO *file2 = (struct FILE_INFO *)b;

	return strcmp(file1->mp3Info.ID3.ID3Artist, file2->mp3Info.ID3.ID3Artist);
}

int sort_by_year(const void *a, const void *b) {

	const struct FILE_INFO *file1 = (struct FILE_INFO *)a;
	const struct FILE_INFO *file2 = (struct FILE_INFO *)b;

	return strcmp(file1->mp3Info.ID3.ID3Year, file2->mp3Info.ID3.ID3Year);
}

void scan_mp3_files(void)
{
	int i = 0;
	unsigned int offset = 0;
	SceIoDirent g_dir;
	
	curScroll = 0;
	current = 0;	        

	num_files_cwd = allocate_files_array();
		if(num_files_cwd < 0) {
			printf("Failed to allocate files array %d", num_files_cwd);
			sceKernelSleepThread();
		}	
	
	memset(&g_dir, 0, sizeof(SceIoDirent)); 
	
	int fd = sceIoDopen(MP3_ROOT);	

	sceIoDread(fd, &g_dir);		// get rid of '.' and '..'
	sceIoDread(fd, &g_dir);

		while ( sceIoDread(fd, &g_dir) > 0) {
			if((g_dir.d_stat.st_attr & FIO_SO_IFDIR) == 0) {
				files[i].key = i;
				strcpy( files[i].name, g_dir.d_name);
				sprintf( files[i].filePath, "%s/%s", MP3_ROOT, files[i].name );
				files[i].sizeB = (unsigned int)g_dir.d_stat.st_size;
				files[i].fileType = get_file_type(files[i].filePath);
				
				if(files[i].fileType == FILE_TYPE_MP3) { // don't assume file is an mp3 just because of location
					ParseID3(&files[i]);
				
					offset = files[i].mp3Info.ID3.ID3EncapsulatedPictureOffset;							

					if(offset != (int)NULL) {
						files[i].cover = loadJpegImage(files[i].filePath, 
													   files[i].mp3Info.ID3.ID3EncapsulatedPictureOffset);																			
					}
				
					i++;
				}
			}
		}
	
	sceIoDclose(fd);
}

struct FILE_INFO * run_file(void) {
	if(files[current].fileType == FILE_TYPE_MP3)
		return &files[current];
	else
		return NULL;
}

void move_up(int n) {
	if (current - n < 0)
	 current = 0; // Stop the ">" from moving past the minimum file
	else
	 current -= n;
		if ((current <= curScroll)) {
			curScroll = current; // To do with how it scrolls
		}	
}

void move_down(int n) {	
	if(current + n < num_files_cwd)
	 current += n;
	else
	 current = num_files_cwd - 1;

	 if (current >= (curScroll+MAX_DISPLAY) ) curScroll += current - (curScroll+MAX_DISPLAY) +1;
}

void sort_mp3_files(void) {
	void *sorter = NULL;
	int c=0, key;

	key = files[file_index_current_mp3].key; // Offer easy compare to find after sort	
	
	switch(sort_method) {
		case SORT_TITLE:
					sorter = sort_by_title;
					break;
		case SORT_GENRE:
					sorter = sort_by_genre;
					break;
		case SORT_ALBUM:
					sorter = sort_by_album;
					break;
		case SORT_ARTIST:
					sorter = sort_by_artist;
					break;
		case SORT_YEAR:
					sorter = sort_by_year;
					break;
	}
	
	qsort(files, num_files_cwd, sizeof(struct FILE_INFO), sorter);
	
	while(key != files[c].key)	
		c++;
		
	file_index_current_mp3 = c;	
}

void display_mp3_files(void) {
	int i;
	int y = (current-curScroll)*10+DISPLAY_Y;
	Color color = 0;
	
	fillScreenRect(BLUE, 0, y-1, 292, 9);
   
	// Displays the directories, while also incorporating the scrolling
	for(i=curScroll; i<MAX_DISPLAY+curScroll; i++) {
		if(i < num_files_cwd)
			if(i == file_index_current_mp3) {
				switch(mp3_state) {
					case STATE_MP3_PLAY: color = GREEN; break;
					case STATE_MP3_PAUSE: color = YELLOW; break;
					case STATE_MP3_STOP: color = RED; break;
				}
			
				printTextScreen(0, (i - curScroll)*10+DISPLAY_Y, "*", color);
			}

			printTextScreen(DISPLAY_X, (i - curScroll)*10+DISPLAY_Y, compact_str(files[i].mp3Info.ID3.ID3Title, 
							MAX_FILENAME_STR), ORANGE);	
		
	}
			
}

#endif

