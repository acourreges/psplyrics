//   ** Modified sources from below author ***
//    Copyright (C) 2007 Sakya
//    sakya_tg@yahoo.it
//    CREDITS:
//    This file contains functions to play mp3 files through the PSP's Media Engine.
//    This code is based upon this sample code from ps2dev.org
//    http://forums.ps2dev.org/viewtopic.php?t=8469
//    and the source code of Music prx by joek2100

#include <string.h>
#include "mp3playerME.h"
#include "../system/mem64.h"
#include <pspkernel.h>
#include <pspsdk.h>
#include <string.h>
#include <stdio.h>
#include <psputility_avmodules.h>
#include <pspaudio.h>

#define THREAD_PRIORITY 12
#define OUTPUT_BUFFER_SIZE	(1152*4)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int mp3_state = STATE_MP3_NONE;

int MP3ME_threadActive = 0;
int MP3ME_threadExited = 1;
//char MP3ME_fileName[264];
static int MP3ME_isPlaying = 0;
SceUID MP3ME_thid = -1;
int MP3ME_audio_channel = 0;
int MP3ME_eof = 0;
//struct MP3_INFO MP3ME_info;
float MP3ME_playingTime = 0;
double MP3ME_filePos = 0;

//Globals for decoding:
SceUID MP3ME_handle;

static int samplerates[4][3] =
{
    {11025, 12000, 8000,},//mpeg 2.5
    {0, 0, 0,}, //reserved
    {22050, 24000, 16000,},//mpeg 2
    {44100, 48000, 32000}//mpeg 1
};
static int bitrates[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };
static int bitrates_v2[] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 };

unsigned char MP3ME_input_buffer[2889]__attribute__((aligned(64)));//mp3 has the largest max frame, at3+ 352 is 2176
unsigned long MP3ME_codec_buffer[65]__attribute__((aligned(64)));
unsigned char MP3ME_output_buffer[2048*4]__attribute__((aligned(64)));//at3+ sample_per_frame*4
short OutputBuffer[2][OUTPUT_BUFFER_SIZE];
short *OutputPtrME = OutputBuffer[0];

int MP3ME_output_index = 0;
long MP3ME_suspendPosition = -1;
long MP3ME_suspendIsPlaying = 0;

//shared global vars for ME
int HW_ModulesInit = 0;
SceUID fd;
u16 data_align;
u32 sample_per_frame;
u16 channel_mode;
u32 samplerate;
long data_start;
long data_size;
u8 getEDRAM;
u32 channels;
SceUID data_memid;
volatile int OutputBuffer_flip;

int openAudio(int channel, int samplecount);
SceUID LoadStartAudioModule(char *modname, int partition);
int initMEAudioModules();
int SeekNextFrameMP3(SceUID fd);
int initAudioLib();
int endAudioLib();
int setAudioFrequency(unsigned short samples, unsigned short freq, char car);
int releaseAudio(void);
int audioOutput(int volume, void *buffer);

char MP3ME_Name[255];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Private functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Decode thread:
int decodeThread(SceSize args, void *argp){
    int res;
    unsigned char MP3ME_header_buf[4];
    int MP3ME_header;
    int version;
    int bitrate;
    int padding;
    int frame_size;
    int size;
	int offset = 0;

	sceAudiocodecReleaseEDRAM(MP3ME_codec_buffer); //Fix: ReleaseEDRAM at the end is not enough to play another mp3.
	MP3ME_threadActive = 1;
    MP3ME_threadExited = 0;
    OutputBuffer_flip = 0;
    OutputPtrME = OutputBuffer[0];

    MP3ME_handle = sceIoOpen(MP3ME_Name, PSP_O_RDONLY, 0777);
    if (MP3ME_handle < 0)
        MP3ME_threadActive = 0;

	//now search for the first sync byte, tells us where the mp3 stream starts
	size = sceIoLseek32(MP3ME_handle, 0, PSP_SEEK_END);
	sceIoLseek32(MP3ME_handle, 0, PSP_SEEK_SET);
	data_start = ID3v2TagSize(MP3ME_Name);
	sceIoLseek32(MP3ME_handle, data_start, PSP_SEEK_SET);
    data_start = SeekNextFrameMP3(MP3ME_handle);

	if (data_start < 0)
		MP3ME_threadActive = 0;

    size -= data_start;

    memset(MP3ME_codec_buffer, 0, sizeof(MP3ME_codec_buffer));

    if ( sceAudiocodecCheckNeedMem(MP3ME_codec_buffer, 0x1002) < 0 )
        MP3ME_threadActive = 0;

    if ( sceAudiocodecGetEDRAM(MP3ME_codec_buffer, 0x1002) < 0 )
        MP3ME_threadActive = 0;

    getEDRAM = 1;

    if ( sceAudiocodecInit(MP3ME_codec_buffer, 0x1002) < 0 )
        MP3ME_threadActive = 0;

    MP3ME_eof = 0;

	while (MP3ME_threadActive){
		while( !MP3ME_eof && MP3ME_isPlaying )
		{
            MP3ME_filePos = sceIoLseek32(MP3ME_handle, 0, PSP_SEEK_CUR);
			if ( sceIoRead( MP3ME_handle, MP3ME_header_buf, 4 ) != 4 ){
				MP3ME_isPlaying = 0;
				MP3ME_threadActive = 0;
				continue;
			}

			MP3ME_header = MP3ME_header_buf[0];
			MP3ME_header = (MP3ME_header<<8) | MP3ME_header_buf[1];
			MP3ME_header = (MP3ME_header<<8) | MP3ME_header_buf[2];
			MP3ME_header = (MP3ME_header<<8) | MP3ME_header_buf[3];

			bitrate = (MP3ME_header & 0xf000) >> 12;
			padding = (MP3ME_header & 0x200) >> 9;
			version = (MP3ME_header & 0x180000) >> 19;
			samplerate = samplerates[version][ (MP3ME_header & 0xC00) >> 10 ];

			if ((bitrate > 14) || (version == 1) || (samplerate == 0) || (bitrate == 0))//invalid frame, look for the next one
			{
				data_start = SeekNextFrameMP3(MP3ME_handle);
				if(data_start < 0)
				{
					MP3ME_eof = 1;
					continue;
				}
				size -= (data_start - offset);
				offset = data_start;
				continue;
			}

			if (version == 3) //mpeg-1
			{
				sample_per_frame = 1152;
				frame_size = 144000*bitrates[bitrate]/samplerate + padding;
			}else{
				sample_per_frame = 576;
				frame_size = 72000*bitrates_v2[bitrate]/samplerate + padding;
			}

			sceIoLseek32(MP3ME_handle, data_start, PSP_SEEK_SET); //seek back

			size -= frame_size;
			if ( size <= 0)
			{
			   MP3ME_eof = 1;
			   continue;
			}

			//since we check for eof above, this can only happen when the file
			// handle has been invalidated by syspend/resume/usb
			if ( sceIoRead( MP3ME_handle, MP3ME_input_buffer, frame_size ) != frame_size ){
                //Resume from suspend:
                if ( MP3ME_handle >= 0 ){
                   sceIoClose(MP3ME_handle);
                   MP3ME_handle = -1;
                }
                MP3ME_handle = sceIoOpen(MP3ME_Name, PSP_O_RDONLY, 0777);
                if (MP3ME_handle < 0){
                    MP3ME_isPlaying = 0;
                    MP3ME_threadActive = 0;
                    continue;
                }
                size = sceIoLseek32(MP3ME_handle, 0, PSP_SEEK_END);
                sceIoLseek32(MP3ME_handle, offset, PSP_SEEK_SET);
                data_start = offset;
				continue;
			}
			data_start += frame_size;
			offset = data_start;

			MP3ME_codec_buffer[6] = (unsigned long)MP3ME_input_buffer;
			MP3ME_codec_buffer[8] = (unsigned long)MP3ME_output_buffer;

			MP3ME_codec_buffer[7] = MP3ME_codec_buffer[10] = frame_size;
			MP3ME_codec_buffer[9] = sample_per_frame * 4;

			res = sceAudiocodecDecode(MP3ME_codec_buffer, 0x1002);

			if ( res < 0 )
			{
				//instead of quitting see if the next frame can be decoded
				//helps play files with an invalid frame
				//we must look for a valid frame, the offset above may be wrong
				data_start = SeekNextFrameMP3(MP3ME_handle);
				if(data_start < 0)
				{
					MP3ME_eof = 1;
					continue;
				}
				size -= (data_start - offset);
				offset = data_start;
				continue;
			}
            MP3ME_playingTime += (float)sample_per_frame/(float)samplerate;

            //Output:
			memcpy( OutputPtrME, MP3ME_output_buffer, sample_per_frame*4);
			OutputPtrME += (sample_per_frame * 4);
			if( OutputPtrME + (sample_per_frame * 4) > &OutputBuffer[OutputBuffer_flip][OUTPUT_BUFFER_SIZE])
			{

                audioOutput(PSP_AUDIO_VOLUME_MAX, OutputBuffer[OutputBuffer_flip]);

				OutputBuffer_flip ^= 1;
				OutputPtrME = OutputBuffer[OutputBuffer_flip];

			}
		}
		sceKernelDelayThread(10000); // Not sure if necessary or purpose
	}
    if (getEDRAM)
        sceAudiocodecReleaseEDRAM(MP3ME_codec_buffer);

    if ( MP3ME_handle >= 0){
      sceIoClose(MP3ME_handle);
      MP3ME_handle = -1;
    }
    MP3ME_threadExited = 1;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Public functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3ME_Init(int channel){
    MP3ME_audio_channel = channel;
	MP3ME_playingTime = 0;
	initMEAudioModules();
}

int MP3ME_Load(struct FILE_INFO *file){
    MP3ME_filePos = 0;
    MP3ME_isPlaying = 0;
	MP3ME_playingTime = 0;
	strcpy(MP3ME_Name, file->filePath); // Needed for decode thread

    releaseAudio();
    if (setAudioFrequency(OUTPUT_BUFFER_SIZE/4, 44100, 2) < 0){
        MP3ME_Stop();
        return ERROR_INVALID_SAMPLE_RATE;
    }

    MP3ME_eof = 0;
		
	MP3ME_thid = sceKernelCreateThread("decodeThread", decodeThread, THREAD_PRIORITY, 0x10000, PSP_THREAD_ATTR_USER, NULL);

		if(MP3ME_thid < 0)
        return ERROR_CREATE_THREAD;

    sceKernelStartThread(MP3ME_thid, 0, NULL);
    return OPENING_OK;
}

int MP3ME_Play(){
    if(MP3ME_thid < 0)
     return -1;
	else
     MP3ME_isPlaying = 1;	
   return 0;
}

void MP3ME_Pause(){
    MP3ME_isPlaying = 0;
}

int MP3ME_Stop(){
    MP3ME_isPlaying = 0;
    MP3ME_threadActive = 0;
    while (!MP3ME_threadExited)
        sceKernelDelayThread(100000);
    sceKernelDeleteThread(MP3ME_thid);
	MP3ME_thid = -1;

    return 0;
}

int MP3ME_EndOfStream(){
    return MP3ME_eof;
}

int MP3ME_GetPercentage(struct FILE_INFO *file){
    int perc = (int)(MP3ME_playingTime/(double)file->mp3Info.length*100.0);
    if (perc > 100)
        perc = 100;
    return perc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions for ME
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Open audio for player:
int openAudio(int channel, int samplecount){
	int audio_channel = sceAudioChReserve(channel, samplecount, PSP_AUDIO_FORMAT_STEREO );
    if(audio_channel < 0)
        audio_channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, samplecount, PSP_AUDIO_FORMAT_STEREO );
	return audio_channel;
}

//Load a module:
SceUID LoadStartAudioModule(char *modname, int partition){
    SceKernelLMOption option;
    SceUID modid;

    memset(&option, 0, sizeof(option));
    option.size = sizeof(option);
    option.mpidtext = partition;
    option.mpiddata = partition;
    option.position = 0;
    option.access = 1;

    modid = sceKernelLoadModule(modname, 0, &option);
    if (modid < 0)
        return modid;

    return sceKernelStartModule(modid, 0, NULL, NULL, NULL);
}

//Load and start needed modules:
int initMEAudioModules(){
   if (!HW_ModulesInit){
        if (sceKernelDevkitVersion() == 0x01050001)
        {
            LoadStartAudioModule("flash0:/kd/me_for_vsh.prx", PSP_MEMORY_PARTITION_KERNEL);
            LoadStartAudioModule("flash0:/kd/audiocodec.prx", PSP_MEMORY_PARTITION_KERNEL);
        }
        else
        {
            sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC);
        }
       HW_ModulesInit = 1;
   }
   return 0;
}

//Seek next valid frame
//NOTE: this function comes from Music prx 0.55 source
//      all credits goes to joek2100.
int SeekNextFrameMP3(SceUID fd)
{
    int offset = 0;
    unsigned char buf[1024];
    unsigned char *pBuffer;
    int i;
    int size = 0;

    offset = sceIoLseek32(fd, 0, PSP_SEEK_CUR);
    sceIoRead(fd, buf, sizeof(buf));
    if (!strncmp((char*)buf, "ID3", 3) || !strncmp((char*)buf, "ea3", 3)) //skip past id3v2 header, which can cause a false sync to be found
    {
        //get the real size from the syncsafe int
        size = buf[6];
        size = (size<<7) | buf[7];
        size = (size<<7) | buf[8];
        size = (size<<7) | buf[9];

        size += 10;

        if (buf[5] & 0x10) //has footer
            size += 10;
    }

    sceIoLseek32(fd, offset, PSP_SEEK_SET); //now seek for a sync
    while(1)
    {
        offset = sceIoLseek32(fd, 0, PSP_SEEK_CUR);
        size = sceIoRead(fd, buf, sizeof(buf));

        if (size <= 2)//at end of file
            return -1;

        if (!strncmp((char*)buf, "EA3", 3))//oma mp3 files have non-safe ints in the EA3 header
        {
            sceIoLseek32(fd, (buf[4]<<8)+buf[5], PSP_SEEK_CUR);
            continue;
        }

        pBuffer = buf;
        for( i = 0; i < size; i++)
        {
            //if this is a valid frame sync (0xe0 is for mpeg version 2.5,2+1)
            if ( (pBuffer[i] == 0xff) && ((pBuffer[i+1] & 0xE0) == 0xE0))
            {
                offset += i;
                sceIoLseek32(fd, offset, PSP_SEEK_SET);
                return offset;
            }
        }
       //go back two bytes to catch any syncs that on the boundary
        sceIoLseek32(fd, -2, PSP_SEEK_CUR);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set frequency for output:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int sceAudioSRCChReserve(int samplecount, int samplerate, int unk);
int setAudioFrequency(unsigned short samples, unsigned short freq, char car){
	return sceAudioSRCChReserve(samples, freq, car);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Release audio:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int sceAudioOutput2GetRestSample();
int sceAudioSRCChRelease(void);
int releaseAudio(void){
	while(sceAudioOutput2GetRestSample() > 0);
	return sceAudioSRCChRelease();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Audio output:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int sceAudioSRCOutputBlocking(int volume, void *buffer);
int audioOutput(int volume, void *buffer){
	return sceAudioSRCOutputBlocking(volume, buffer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Init pspaudiolib:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int initAudioLib(){
    pspAudioInit();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//End pspaudiolib:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int endAudioLib(){
    pspAudioEnd();
    return 0;
}
