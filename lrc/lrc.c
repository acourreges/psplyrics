
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

#include "lrc.h"

struct sentence {
	int time_stamp;
	char raw_string[1024];
	char raw_lyrics[1024];
	struct sentence * next;
};
typedef struct sentence *SENTENCE;


//declarations
int parse_file(int file);
void update_state(int current_time);
void free_sentences();
void parse_one_sentence(SENTENCE previous, char * c_seek, char * max_range);
int parse_time_stamp(char * s, char** end_offset);
int get_int_from_char(char c);

char debug_string[1024];
short lrc_available;
SENTENCE first_sentence;
int last_update;


int init_lrc_for_file(char * file_name){

	lrc_available = 0;

	//Look for a LRC file: /LRC/musicFileName.lrc
	
	//remove the extension of the file name
	char raw_name[128];
	strcpy(raw_name,file_name);
	char *suffix = strrchr(raw_name, '.');
	*suffix = '\0';
	
	//Path of the LRC file
	char lrc_path[256];	
	sprintf(lrc_path, "%s/%s.lrc", LYRICS_ROOT, raw_name );
	
	
	
	//Try to open file
	int fd = sceIoOpen(lrc_path, PSP_O_RDONLY, 0777);
	
	
	if (fd < 0) {
		lrc_available = 0;
		strcpy(debug_string, "No lyrics found.");
		return 1;
	}
	
	//Now we know we have a lrc file
	strcpy(debug_string, "Lrc file found.");
	
	//free the previous structures if necessary
	free_sentences();

	parse_file(fd);
	
	sceIoClose(fd);
	
	lrc_available = 1;
	
	return 0;

}

void update_state(int current_time){

	SENTENCE last_seen = first_sentence;
	SENTENCE following = last_seen->next;
	while ((current_time > last_seen->time_stamp) && (following != NULL) && (current_time > following->time_stamp)) {
		last_seen = following;
		following = last_seen->next;
	}
	
	strcpy(debug_string, last_seen->raw_lyrics); 
	
	
	//sprintf(debug_string, "value: %d | %s", first_sentence->next->next->time_stamp, first_sentence->next->next->raw_lyrics);
	//sprintf(debug_string, "value: %d | %s", current_time, first_sentence->next->next->raw_lyrics);
	
}

void free_sentences() {
	SENTENCE sentence = first_sentence; 
	
	while (sentence != NULL) {
		SENTENCE next = sentence->next;
		free(sentence);
		sentence = next; 
	}
}

int parse_file(int file){
	
	long lSize = sceIoLseek(file, 0, SEEK_END);
	
	sceIoLseek(file, 0, SEEK_SET);

	
	// allocate memory to contain the whole file:
	char * buffer = (char*) malloc (sizeof(char)*lSize);
	if (buffer == NULL) {strcpy(debug_string, "Memory error"); exit (2);}

	

	// copy the file into the buffer:
	int result = sceIoRead(file, buffer,lSize);
	if (result != lSize) {strcpy(debug_string, "Read error"); exit (3);}
	
	first_sentence = (SENTENCE)malloc(sizeof(struct sentence));
	first_sentence->time_stamp = -1;
	strcpy(first_sentence->raw_string," ");
	strcpy(first_sentence->raw_lyrics," ");
	first_sentence->next = NULL;
	
	parse_one_sentence(first_sentence, buffer, buffer + lSize - 1);
	
	/*
	SENTENCE tests = first_sentence;
	while (tests->next != NULL) tests = tests->next;
	strcpy(debug_string, tests->raw_string);
	*/
	//sprintf(debug_string, "value: %d | %s", first_sentence->next->time_stamp, first_sentence->next->raw_lyrics);
	

	return 0;
	
}

void parse_one_sentence(SENTENCE previous, char * c_seek, char * max_range){
	
	//skeep to the first '['
	while((*c_seek != 0x5B) && (c_seek < max_range)) c_seek++;
	
	//if reach end of file
	if (c_seek >= max_range) return;
	
	//create an empty sentence
	SENTENCE new_sentence = (SENTENCE)malloc(sizeof(struct sentence));
	new_sentence->time_stamp = -1;
	new_sentence->next = NULL;
	
	char * begin = c_seek;
	
	//skip to end of line
	while((*c_seek != 0x0A) && (c_seek < max_range)) c_seek++;
	
	memcpy(new_sentence->raw_string, begin, c_seek - begin);
	new_sentence->raw_string[c_seek - begin] = '\0';
	
	char * begin_phrase = new_sentence->raw_string;
	
	//time_stamp will be -1 if parsing failed
	new_sentence->time_stamp = parse_time_stamp(new_sentence->raw_string,&begin_phrase);
	if (*begin_phrase == '\0') new_sentence->raw_lyrics[0] = '\0'; else strcpy(new_sentence->raw_lyrics, begin_phrase);
	
	//link to the previous sentence
	previous->next = new_sentence;
	
	c_seek++;
	if (c_seek >= max_range) return;
	
	//recursive call for the following
	parse_one_sentence(new_sentence, c_seek, max_range);
	
}

//Extracts the time in ms from [01:27.45]....
//-1 if parsing fails
int parse_time_stamp(char * s, char** end_offset){
	char * pos = s;
	int time = 0;
	//test '['
	if (*pos != 0x5b) return -1;
	pos++;
	
	int nb = get_int_from_char(*pos);
	if (nb == -1) return -1;
	
	//10*min
	time += nb * 600000;
	pos++;
	
	nb = get_int_from_char(*pos);
	if (nb == -1) return -1;
	
	//min
	time += nb * 60000;
	pos++;
	
	//test ':'
	if (*pos != 0x3a) return -1;
	pos++;
	
	nb = get_int_from_char(*pos);
	if (nb == -1) return -1;
	
	//10*sec
	time += nb * 10000;
	pos++;
	
	nb = get_int_from_char(*pos);
	if (nb == -1) return -1;
	
	//sec
	time += nb * 1000;
	pos++;
	
	//test ']'
	if (*pos == 0x5d) {
		*end_offset = pos+1;
		return time;
	}
	
	//test '.'
	if (*pos != 0x2e)return -1; 
	pos++;
	
	nb = get_int_from_char(*pos);
	if (nb == -1) return -1;
	
	//d-sec
	time += nb * 100;
	pos++;
	
	nb = get_int_from_char(*pos);
	if (nb == -1) return -1;
	
	//c-sec
	time += nb * 10;
	pos++;
	
	//test ']'
	if (*pos != 0x5d) return -1;
	
	*end_offset = pos+1;	
	return time;
	
}

int get_int_from_char(char c){
	int nb = c - 0x30;
	if (nb > 9) return -1;
	if (nb < 0) return -1;
	return nb;
}

