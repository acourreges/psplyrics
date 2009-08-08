#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include <stdlib.h>

#include "id3.h"
struct genre
{
	int code;
	char text[112];
};

struct genre genreList[] =
{
   {0 , "Blues"}, {1 , "Classic Rock"}, {2 , "Country"}, {3 , "Dance"}, {4 , "Disco"}, {5 , "Funk"}, {6 , "Grunge"}, {7 , "Hip-Hop"}, {8 , "Jazz"}, {9 , "Metal"}, {10 , "New Age"},
   {11 , "Oldies"}, {12 , "Other"}, {13 , "Pop"}, {14 , "R&B"}, {15 , "Rap"}, {16 , "Reggae"}, {17 , "Rock"}, {18 , "Techno"}, {19 , "Industrial"}, {20 , "Alternative"},
   {21 , "Ska"}, {22 , "Death Metal"}, {23 , "Pranks"}, {24 , "Soundtrack"}, {25 , "Euro-Techno"}, {26 , "Ambient"}, {27 , "Trip-Hop"}, {28 , "Vocal"}, {29 , "Jazz+Funk"}, {30 , "Fusion"},
   {31 , "Trance"}, {32 , "Classical"}, {33 , "Instrumental"}, {34 , "Acid"}, {35 , "House"}, {36 , "Game"}, {37 , "Sound Clip"}, {38 , "Gospel"}, {39 , "Noise"}, {40 , "Alternative Rock"},
   {41 , "Bass"}, {42 , "Soul"}, {43 , "Punk"}, {44 , "Space"}, {45 , "Meditative"}, {46 , "Instrumental Pop"}, {47 , "Instrumental Rock"}, {48 , "Ethnic"}, {49 , "Gothic"}, {50 , "Darkwave"},
   {51 , "Techno-Industrial"}, {52 , "Electronic"}, {53 , "Pop-Folk"}, {54 , "Eurodance"}, {55 , "Dream"}, {56 , "Southern Rock"}, {57 , "Comedy"}, {58 , "Cult"}, {59 , "Gangsta"}, {60 , "Top 40"},
   {61 , "Christian Rap"}, {62 , "Pop/Funk"}, {63 , "Jungle"}, {64 , "Native US"}, {65 , "Cabaret"}, {66 , "New Wave"}, {67 , "Psychadelic"}, {68 , "Rave"}, {69 , "Showtunes"}, {70 , "Trailer"},
   {71 , "Lo-Fi"}, {72 , "Tribal"}, {73 , "Acid Punk"}, {74 , "Acid Jazz"}, {75 , "Polka"}, {76 , "Retro"}, {77 , "Musical"}, {78 , "Rock & Roll"}, {79 , "Hard Rock"}, {80 , "Folk"},
   {81 , "Folk-Rock"}, {82 , "National Folk"}, {83 , "Swing"}, {84 , "Fast Fusion"}, {85 , "Bebob"}, {86 , "Latin"}, {87 , "Revival"}, {88 , "Celtic"}, {89 , "Bluegrass"}, {90 , "Avantgarde"},
   {91 , "Gothic Rock"}, {92 , "Progressive Rock"}, {93 , "Psychedelic Rock"}, {94 , "Symphonic Rock"}, {95 , "Slow Rock"}, {96 , "Big Band"}, {97 , "Chorus"}, {98 , "Easy Listening"}, {99 , "Acoustic"},
   {100 , "Humour"}, {101 , "Speech"}, {102 , "Chanson"}, {103 , "Opera"}, {104 , "Chamber Music"}, {105 , "Sonata"}, {106 , "Symphony"}, {107 , "Booty Bass"}, {108 , "Primus"}, {109 , "Porn Groove"},
   {110 , "Satire"}, {111 , "Slow Jam"}, {112 , "Club"}, {113 , "Tango"}, {114 , "Samba"}, {115 , "Folklore"}, {116 , "Ballad"}, {117 , "Power Ballad"}, {118 , "Rhytmic Soul"}, {119 , "Freestyle"}, {120 , "Duet"},
   {121 , "Punk Rock"}, {122 , "Drum Solo"}, {123 , "Acapella"}, {124 , "Euro-House"}, {125 , "Dance Hall"}, {126 , "Goa"}, {127 , "Drum & Bass"}, {128 , "Club-House"}, {129 , "Hardcore"}, {130 , "Terror"},
   {131 , "Indie"}, {132 , "BritPop"}, {133 , "Negerpunk"}, {134 , "Polsk Punk"}, {135 , "Beat"}, {136 , "Christian Gangsta"}, {137 , "Heavy Metal"}, {138 , "Black Metal"}, {139 , "Crossover"}, {140 , "Contemporary C"},
   {141 , "Christian Rock"}, {142 , "Merengue"}, {143 , "Salsa"}, {144 , "Thrash Metal"}, {145 , "Anime"}, {146 , "JPop"}, {147 , "SynthPop"}
};
int genreNumber = sizeof (genreList) / sizeof (struct genre);


//Search for FF+D8+FF bytes (first bytes of a jpeg image)
//Returns file position:
int searchJPGstart(FILE *fp, int delta){
    int retValue = -1;
    int i = 0;
    unsigned char threeChar[3];

    int startPos = ftell(fp);
    for (i=0; i<delta; i++){
        fread(threeChar, sizeof(unsigned char), 3, fp);
        if (threeChar[0] == 0xFF && threeChar[1] == 0xD8 && threeChar[2] == 0xFF){
            retValue = ftell(fp) - 3;
            break;
        }
        fseek(fp, -2, SEEK_CUR);
    }
    fseek(fp, startPos, SEEK_SET);
    return retValue;
}

//Search for 89 50 4E 47 0D 0A 1A 0A 00 00 00 0D 49 48 44 52 bytes (first bytes of a PNG image)
//Returns file position:
int searchPNGstart(FILE *fp, int delta){
    int retValue = -1;
    int i = 0;
    int j = 0;
    int testResult = 0;
    unsigned char testChar[16];
    unsigned char pngChar[16] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
                                 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52};
    int startPos = ftell(fp);

    for (i=0; i<delta; i++){
        fread(testChar, sizeof(unsigned char), 16, fp);
        testResult = 1;
        for (j=0; j<16; j++){
            if (testChar[j] != pngChar[j]){
                testResult = 0;
                break;
            }
        }
        if (testResult){
            retValue = ftell(fp) - 16;
            break;
        }
        fseek(fp, -15, SEEK_CUR);
    }
    fseek(fp, startPos, SEEK_SET);
    return retValue;
}

// ID3v2 code taken from libID3 by Xart
// http://www.xart.co.uk
short int swapInt16BigToHost(short int arg)
{
   short int i=0;
   int checkEndian = 1;
   if( 1 == *(char *)&checkEndian )
   {
      // Intel (little endian)
      i=arg;
      i=((i&0xFF00)>>8)|((i&0x00FF)<<8);
   }
   else
   {
      // PPC (big endian)
      i=arg;
   }
   return i;
}

int swapInt32BigToHost(int arg)
{
   int i=0;
   int checkEndian = 1;
   if( 1 == *(char *)&checkEndian )
   {
      // Intel (little endian)
      i=arg;
      i=((i&0xFF000000)>>24)|((i&0x00FF0000)>>8)|((i&0x0000FF00)<<8)|((i&0x000000FF)<<24);
   }
   else
   {
      // PPC (big endian)
      i=arg;
   }
   return i;
}

//Reads tag data purging invalid characters:
void readTagData(FILE *fp, int tagLength, char *tagValue){
    int i;
    int count = 0;
    unsigned char carattere[tagLength];

    strcpy(tagValue, "");
    tagValue[0] = '\0';

    fread(carattere, sizeof(char), tagLength, fp);
    for (i=0; i<tagLength; i++){
        if (carattere[i] >= 0x20 && carattere[i] <= 0xfd) //<= 0x7f
            tagValue[count++] = carattere[i];
    }
    tagValue[count] = '\0';
}

int ID3v2TagSize(char *file)
{
   FILE *fp;
   int size;
   char sig[3];

   fp = fopen(file, "rb");
   if (fp == NULL) return 0;

   fread(sig, sizeof(char), 3, fp);
   if (strncmp("ID3",sig,3) != 0) {
      fclose(fp);
      return 0;
   }

   fseek(fp, 6, SEEK_SET);
   fread(&size, sizeof(unsigned int), 1, fp);
   /*
    *  The ID3 tag size is encoded with four bytes where the first bit
    *  (bit 7) is set to zero in every byte, making a total of 28 bits. The zeroed
    *  bits are ignored, so a 257 bytes long tag is represented as $00 00 02 01.
    */

   size = (unsigned int) swapInt32BigToHost((int)size);
   size = ( ( (size & 0x7f000000) >> 3 ) | ( (size & 0x7f0000) >> 2 ) | ( (size & 0x7f00) >> 1 ) | (size & 0x7f) );
   fclose(fp);
   return size;
}

int ID3v2(char *file)
{
   char sig[3];
   unsigned short int version;

   FILE *fp = fopen(file, "rb");
   if (fp == NULL) 
    return -1;

   fread(sig, sizeof(char), 3, fp);
   if (!strncmp("ID3",sig,3)) {
      fread(&version, sizeof(unsigned short int), 1, fp);
      version = (unsigned short int) swapInt16BigToHost((short int)version);
      version /= 256;
   } else {
	  fclose(fp);
	  return -1;
   }

   fclose(fp);
   return (int)version;
}

void ParseID3v2_2(struct FILE_INFO *file)
{
   FILE *fp = NULL;

   int size;
   int tag_length;
   char tag[3];
   char buffer[20];

      fp = fopen(file->filePath, "rb");
      if (fp == NULL) return;
	  
	  size = ID3v2TagSize(file->filePath);   
	  	  
      fseek(fp, 10, SEEK_SET);

      while (size != 0) {
         fread(tag, sizeof(char), 3, fp);
         size -= 3;

         /* read 3 byte big endian tag length */
         fread(&tag_length, sizeof(unsigned int), 1, fp);
         fseek(fp, -1, SEEK_CUR);

         tag_length = (unsigned int) swapInt32BigToHost((int)tag_length);
         tag_length = (tag_length / 256);
         size -= 3;

         /* Perform checks for end of tags and tag length overflow or zero */
         if(*tag == 0 || tag_length > size || tag_length == 0) break;

         if(!strncmp("TP1",tag,3)) /* Artist */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Artist);
         }
         else if(!strncmp("TP2",tag,3)) /* Title */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Title);
         }
         else if(!strncmp("TAL",tag,3)) /* Album */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Album);
         }
         else if(!strncmp("TRK",tag,3)) /* Track No. */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3TrackText);
         }
         else if(!strncmp("TYE",tag,3)) /* Year */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Year);
         }
         else if(!strncmp("TLE",tag,3)) /* Length */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, buffer);
         }
         else if(!strncmp("COM",tag,3)) /* Comment */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Comment);
         }
         else if(!strncmp("TCO",tag,3)) /* Genre */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3GenreText);
         }
         else if(!strncmp("PIC",tag,3)) /* Picture */
         {
            fseek(fp, 1, SEEK_CUR);
            fseek(fp, 5, SEEK_CUR);
            file->mp3Info.ID3.ID3EncapsulatedPictureOffset = ftell(fp);
            file->mp3Info.ID3.ID3EncapsulatedPictureLength = tag_length-6;
            fseek(fp, tag_length - 6, SEEK_CUR);
         }
         else
         {
            fseek(fp, tag_length, SEEK_CUR);
         }
         size -= tag_length;
      }
      strcpy(file->mp3Info.ID3.versionfound, "2.2");
      fclose(fp);
}

void ParseID3v2_3(struct FILE_INFO *file)
{
   FILE *fp = NULL;

   int size;
   int tag_length;
   char tag[4];
         
      fp = fopen(file->filePath, "rb");
      if (fp == NULL) return;
	  
	  size = ID3v2TagSize(file->filePath);   
      fseek(fp, 10, SEEK_SET);

      while (size != 0) {
         fread(tag, sizeof(char), 4, fp);
         size -= 4;

         /* read 4 byte big endian tag length */
         fread(&tag_length, sizeof(unsigned int), 1, fp);
         tag_length = (unsigned int) swapInt32BigToHost((int)tag_length);
         size -= 4;

         fseek(fp, 2, SEEK_CUR);
         size -= 2;

         /* Perform checks for end of tags and tag length overflow or zero */
         if(*tag == 0 || tag_length > size || tag_length == 0) break;

         if(!strncmp("TPE1",tag,4)) /* Artist */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Artist);
         }
         else if(!strncmp("TIT2",tag,4)) /* Title */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Title);
         }
         else if(!strncmp("TALB",tag,4)) /* Album */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Album);
         }/*
         else if(!strncmp("TRCK",tag,4)) 
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3TrackText);
            file->mp3Info.ID3.ID3Track = atoi(file->mp3Info.ID3.ID3TrackText);
         }*/
         else if(!strncmp("TYER",tag,4)) /* Year */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Year);
         }/*
         else if(!strncmp("TLEN",tag,4)) 
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, buffer);
			file->mp3Info.ID3.ID3Length = atoi(buffer);
         }*/
         else if(!strncmp("TCO",tag,3)) /* Genre */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3GenreText);
         }		 /*
         else if(!strncmp("COMM",tag,4))
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Comment);
         }*/
         else if(!strncmp("APIC",tag,4)) /* Picture */
         {
            fseek(fp, 1, SEEK_CUR);
            fseek(fp, 12, SEEK_CUR);
            file->mp3Info.ID3.ID3EncapsulatedPictureType = JPEG_IMAGE;
            file->mp3Info.ID3.ID3EncapsulatedPictureOffset = searchJPGstart(fp, 20);
            if (file->mp3Info.ID3.ID3EncapsulatedPictureOffset < 0){
                file->mp3Info.ID3.ID3EncapsulatedPictureType = PNG_IMAGE;
                file->mp3Info.ID3.ID3EncapsulatedPictureOffset = searchPNGstart(fp, 20);
            }
            file->mp3Info.ID3.ID3EncapsulatedPictureLength = tag_length-13;
            fseek(fp, tag_length-13, SEEK_CUR);
            if (file->mp3Info.ID3.ID3EncapsulatedPictureOffset < 0){
                file->mp3Info.ID3.ID3EncapsulatedPictureType = 0;
                file->mp3Info.ID3.ID3EncapsulatedPictureOffset = 0;
                file->mp3Info.ID3.ID3EncapsulatedPictureLength = 0;
            }
         }
         else
         {
            fseek(fp, tag_length, SEEK_CUR);
         }
         size -= tag_length;
      }
      strcpy(file->mp3Info.ID3.versionfound, "2.3");
      fclose(fp);
   
}

void ParseID3v2_4(struct FILE_INFO *file)
{
   FILE *fp = NULL;

   int size;
   int tag_length;
   char tag[4];
   char buffer[20];

    
    
      fp = fopen(file->filePath, "rb");
      if (fp == NULL) return;
	  
	  size = ID3v2TagSize(file->filePath);   
	  
      fseek(fp, 10, SEEK_SET);

      while (size != 0) {
         fread(tag, sizeof(char), 4, fp);
         size -= 4;

         /* read 4 byte big endian tag length */
         fread(&tag_length, sizeof(unsigned int), 1, fp);
         tag_length = (unsigned int) swapInt32BigToHost((int)tag_length);
         size -= 4;

         fseek(fp, 2, SEEK_CUR);
         size -= 2;

         /* Perform checks for end of tags and tag length overflow or zero */
         if(*tag == 0 || tag_length > size || tag_length == 0) break;

         if(!strncmp("TPE1",tag,4)) /* Artist */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Artist);
         }
         else if(!strncmp("TIT2",tag,4)) /* Title */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Title);
         }
         else if(!strncmp("TALB",tag,4)) /* Album */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Album);
         }
         else if(!strncmp("TRCK",tag,4)) /* Track No. */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3TrackText);
            file->mp3Info.ID3.ID3Track = atoi(file->mp3Info.ID3.ID3TrackText);
         }
         else if(!strncmp("TYER",tag,4)) /* Year */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Year);
         }
         else if(!strncmp("TLEN",tag,4)) /* Length */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, buffer);
			file->mp3Info.ID3.ID3Length = atoi(buffer);
         }
         else if(!strncmp("TCO",tag,3)) /* Genre */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3GenreText);
         }		 
         else if(!strncmp("COMM",tag,4)) /* Comment */
         {
            fseek(fp, 1, SEEK_CUR);
            readTagData(fp, tag_length - 1, file->mp3Info.ID3.ID3Comment);
         }
         else if(!strncmp("APIC",tag,4)) /* Picture */
         {
            fseek(fp, 1, SEEK_CUR);
            fseek(fp, 12, SEEK_CUR);
            file->mp3Info.ID3.ID3EncapsulatedPictureType = JPEG_IMAGE;
            file->mp3Info.ID3.ID3EncapsulatedPictureOffset = searchJPGstart(fp, 20);
            if (file->mp3Info.ID3.ID3EncapsulatedPictureOffset < 0){
                file->mp3Info.ID3.ID3EncapsulatedPictureType = PNG_IMAGE;
                file->mp3Info.ID3.ID3EncapsulatedPictureOffset = searchPNGstart(fp, 20);
            }
            file->mp3Info.ID3.ID3EncapsulatedPictureLength = tag_length-13;
            fseek(fp, tag_length-13, SEEK_CUR);
            if (file->mp3Info.ID3.ID3EncapsulatedPictureOffset < 0){
                file->mp3Info.ID3.ID3EncapsulatedPictureType = 0;
                file->mp3Info.ID3.ID3EncapsulatedPictureOffset = 0;
                file->mp3Info.ID3.ID3EncapsulatedPictureLength = 0;
            }
         }
         else
         {
            fseek(fp, tag_length, SEEK_CUR);
         }
         size -= tag_length;
      }
      strcpy(file->mp3Info.ID3.versionfound, "2.4");
      fclose(fp);
   
}

int ParseID3v2(struct FILE_INFO *file)
{
 int version;
 
 version = ID3v2(file->filePath);
  if(version == -1)
   return -1;
   
   switch (version) {
      case 2:
         ParseID3v2_2(file);
         break;
      case 3:
         ParseID3v2_3(file);
         break;
      case 4:
         ParseID3v2_4(file);
         break;
      default:
         return -1;
   }

   /* If no Title is found, uses filename - extension for Title. */
   if(*file->mp3Info.ID3.ID3Title == 0) {
      strcpy(file->mp3Info.ID3.ID3Title,strrchr(file->filePath,'/') + 1);
      if (*strrchr(file->mp3Info.ID3.ID3Title,'.') != 0) *strrchr(file->mp3Info.ID3.ID3Title,'.') = 0;
   }
   return 0;
}

int ParseID3v1(struct FILE_INFO *file){
    int id3fd; //our local file descriptor
    char id3buffer[128]; // why not 128 bytes?
    id3fd = sceIoOpen(file->filePath, 0x0001, 0777);
    if (id3fd < 0)
        return -1;
    sceIoLseek(id3fd, -128, SEEK_END);
    sceIoRead(id3fd,id3buffer,128);

    if (strstr(id3buffer,"TAG") != NULL){
        sceIoLseek(id3fd, -125, SEEK_END);
        sceIoRead(id3fd,file->mp3Info.ID3.ID3Title,30);
    	file->mp3Info.ID3.ID3Title[30] = '\0';

        sceIoLseek(id3fd, -95, SEEK_END);
        sceIoRead(id3fd,file->mp3Info.ID3.ID3Artist,30);
    	file->mp3Info.ID3.ID3Artist[30] = '\0';

        sceIoLseek(id3fd, -65, SEEK_END);
        sceIoRead(id3fd,file->mp3Info.ID3.ID3Album,30);
    	file->mp3Info.ID3.ID3Album[30] = '\0';

        sceIoLseek(id3fd, -35, SEEK_END);
        sceIoRead(id3fd,file->mp3Info.ID3.ID3Year,4);
    	file->mp3Info.ID3.ID3Year[4] = '\0';

        sceIoLseek(id3fd, -31, SEEK_END);
        sceIoRead(id3fd,file->mp3Info.ID3.ID3Comment,30);
    	file->mp3Info.ID3.ID3Comment[30] = '\0';

        sceIoLseek(id3fd, -1, SEEK_END);
        sceIoRead(id3fd,file->mp3Info.ID3.ID3GenreCode,1);
    	file->mp3Info.ID3.ID3GenreCode[1] = '\0';

        /* Track */
        if (*(file->mp3Info.ID3.ID3Comment + 28) == 0 && *(file->mp3Info.ID3.ID3Comment + 29) > 0) {
           file->mp3Info.ID3.ID3Track = (int)*(file->mp3Info.ID3.ID3Comment + 29);
           strcpy(file->mp3Info.ID3.versionfound, "1.1");
        } else {
           file->mp3Info.ID3.ID3Track = 1;
           strcpy(file->mp3Info.ID3.versionfound, "1.0");
        }

    	if (((int)file->mp3Info.ID3.ID3GenreCode[0] >= 0) & ((int)file->mp3Info.ID3.ID3GenreCode[0] < genreNumber)){
    		strcpy(file->mp3Info.ID3.ID3GenreText, genreList[(int)file->mp3Info.ID3.ID3GenreCode[0]].text);
    	}
    	else{
    		strcpy(file->mp3Info.ID3.ID3GenreText, "");
    	}
    	file->mp3Info.ID3.ID3GenreText[30] = '\0';

        sceIoClose(id3fd);
        return 0;
     }else{
        sceIoClose(id3fd);
        return -1; // Not an ID3v1 tag
     }
	 
    sceIoClose(id3fd);
    return 0;
}

// Main function:
void ParseID3(struct FILE_INFO *file)
{
	
	memset(&file->mp3Info.ID3, 0, sizeof(struct ID3Tag));

     if(ParseID3v2(file) != 0)
      ParseID3v1(file); // Not a v2 tag, parse as v1 (was reversed in lightmp3)	  	  
}
