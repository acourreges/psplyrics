#ifndef FILESTRUCT_H
#define FILESTRUCT_H

#include "../gfx/graphics.h"

struct ID3Tag {
    char   ID3Title[100];
    char   ID3Artist[100];
    char   ID3Album[100];
	char   ID3Year[12];
	char   ID3Comment[200];
	char   ID3GenreCode[12];
	char   ID3GenreText[100];
    char   versionfound[12];
    int    ID3Track;
    char   ID3TrackText[8];
    int    ID3EncapsulatedPictureType;
    int    ID3EncapsulatedPictureOffset; /* Offset address of an attached picture, NULL if no attached picture exists */
    int    ID3EncapsulatedPictureLength;
    int    ID3Length;
};

struct MP3_INFO {
	int  kbit;
	long hz;
	char mode[24];
	long length;	
	struct ID3Tag ID3;
};

struct FILE_INFO {	
	int		key;
	char	name[128];
	char	filePath[255];
	int		fileType;
	struct MP3_INFO mp3Info;
	Image   *cover;
	unsigned int sizeB;
};

#endif
