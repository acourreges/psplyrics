#ifndef ID3_H
#define ID3_H

#include <stdio.h>
#include "../fileman/file_struct.h"

#define JPEG_IMAGE 1
#define PNG_IMAGE 2

int ID3v2TagSize(char *file);
void ParseID3(struct FILE_INFO *file);

//Helper functions (used also by aa3mplayerME to read tags):
void readTagData(FILE *fp, int tagLength, char *tagValue);
int swapInt32BigToHost(int arg);
short int swapInt16BigToHost(short int arg);

#endif
