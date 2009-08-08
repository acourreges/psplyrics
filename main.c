#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspsysmem.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <systemctrl.h>
#include "fileman/filebrowser.h"
#include "system/usb.h"
#include "audio/mp3playerME.h"
#include <psppower.h>
#include <pspgu.h>
#include "gfx/graphics.h"
#include <psprtc.h>
#include <pspdebug.h>
#include <time.h>
#include <math.h>
#include "intraFont/intraFont.h"
#include "lrc/lrc.h"

// Defines

#define VERSION "PSPLyrics 0.1"

#define PLAY_MODE_SINGLE 0
#define PLAY_MODE_SEQ	 1
#define PLAY_MODE_RAND	 2
#define PLAY_MODE_REPEAT 3

#define STATE_USB_ON    1
#define STATE_USB_OFF   0

#define RAND_INT(low,high) (int)floor((low+(double)rand()*(high-low+1)/RAND_MAX))  

PSP_MODULE_INFO("PSPLyrics",0, 1, 1); 
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
//PSP_HEAP_SIZE_KB(1024*8);
// Globals

int play_mode = PLAY_MODE_SINGLE;
int usb_state = STATE_USB_OFF;
char out_str[255];
Image *cover = NULL;

// Prototypes from support.prx kernel module 0x1006

int imposeGetVolume();
int imposeGetBrightness();
int imposeSetVolume(int value); // 0-30
void initController();
void readButtons(SceCtrlData *pad_data, int count);

// Prototypes

void increase_volume(int n);
void increase_volume(int n);
void decrease_volume(int n);
void set_track(int track);
void next_track(void);
void prev_track(void);
int display_mp3_info(struct FILE_INFO *file);
int start_mp3(struct FILE_INFO *file);

//intrafonts
intraFont* ltn[16]; 
intraFont* jpn0;

// Functions

int exit_callback(int arg1, int arg2, void *common) {
	sceKernelExitGame();
	return 0;
}

int CallbackThread(SceSize args, void *argp) {

	int cbid;
	
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

int SetupCallbacks(void) {

	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
		if(thid >= 0) {
			sceKernelStartThread(thid, 0, 0);
		}
	return thid;
} 

void set_volume(int vol) {
	if(vol > 30)
		vol = 30;
	if(vol < 0)
		vol = 0;
		
	imposeSetVolume(vol);
}

void increase_volume(int n) {
	int v = imposeGetVolume();
	
	set_volume(v+n);
}

void decrease_volume(int n) {
	int v = imposeGetVolume();
	
	set_volume(v-n);
}

void set_track(int track) {
	if(track >= num_files_cwd)
		track = num_files_cwd - 1;
	if(track < 0)
		track = 0;

	start_mp3(&files[track]);
	
	file_index_current_mp3 = track;
}	

void next_track(void) {
	set_track(file_index_current_mp3+1);
}

void prev_track(void) {
	set_track(file_index_current_mp3-1);
}

int display_mp3_info(struct FILE_INFO *file) {

	Color color = BLUE;
	Color color2 = VANILLA;
	
	int y_start = 25; //210

	/*
	if(file->cover != NULL)
		blitImageToScreen(0, 0, file->cover->imageWidth, file->cover->imageHeight, file->cover, 305, 23);  
   	*/
   
	sprintf(out_str, "Size  : %s", (file_index_current_mp3>=0)?commas((unsigned int)files[file_index_current_mp3].sizeB/1024):"");
	printTextScreen(DISPLAY_X, 180, out_str, color);			 
  
	sprintf(out_str, "ID3Tag: %s", file->mp3Info.ID3.versionfound); 
	printTextScreen(DISPLAY_X, 190, out_str, color);	 	
	
    sprintf(out_str, "Title : %s", compact_str(file->mp3Info.ID3.ID3Title, 28));
	printTextScreen(DISPLAY_X, 200, out_str, color);			 
 
	sprintf(out_str, "Album : %s", compact_str(file->mp3Info.ID3.ID3Album, 28));
	printTextScreen(DISPLAY_X, 210, out_str, color);			 

	sprintf(out_str, "Year  : %s", file->mp3Info.ID3.ID3Year);
	printTextScreen(DISPLAY_X, 220, out_str, color);			 

	sprintf(out_str, "Artist: %s", compact_str(file->mp3Info.ID3.ID3Artist, 28));
	printTextScreen(DISPLAY_X, 230, out_str, color);			 

	sprintf(out_str, "Genre : %s", compact_str(file->mp3Info.ID3.ID3GenreText, 28));
	printTextScreen(DISPLAY_X, 240, out_str, color);			 	
	
	sprintf(out_str, "Tracks  : %d", num_files_cwd);
	printTextScreen(303, y_start, out_str, color2);			 	
	
	sprintf(out_str, "Selected: %d", current+1);
	printTextScreen(303, y_start+10, out_str, color2);			 	
	
	sprintf(out_str, "Playing : %d", (mp3_state>0)?file_index_current_mp3+1:0);
	printTextScreen(303, y_start+20, out_str, color2);			 	

	sprintf(out_str, "Elapsed : %.0fs ", MP3ME_playingTime);
	printTextScreen(303, y_start+30, out_str, color2);			 	
	
	return 0;
}

int start_mp3(struct FILE_INFO *file) {      
	if(file == NULL || file->fileType != FILE_TYPE_MP3)
		return -1;		
		 
	switch(mp3_state) {
		case STATE_MP3_PLAY:
		case STATE_MP3_PAUSE:
						MP3ME_Stop();
		case STATE_MP3_STOP:
		case STATE_MP3_NONE:
						if(MP3ME_Load(file) == OPENING_OK) {
							file_index_current_mp3 = current;
							
							////******* LOAD AND INIT LRC ******//////
 							//from file->	
 							init_lrc_for_file(file->name);				
 							//////////////
							
						if(MP3ME_Play() == 0)
							mp3_state = STATE_MP3_PLAY;
							
						}
						break;
											
	 }						  

	return 0;
} 


void loadFonts() {
	// Init intraFont library
	intraFontInit();
    
	// Load fonts
	//intraFont* ltn[16];                   //latin fonts (large/small, with/without serif, regular/italic/bold/italic&bold)
	char file[40];
	int i;
	for (i = 0; i < 16; i++) {
		sprintf(file, "flash0:/font/ltn%d.pgf", i); 
		ltn[i] = intraFontLoad(file,0);                                             //<- this is where the actual loading happens 
		intraFontSetStyle(ltn[i], 1.0f, WHITE, DARKGRAY, 0);
		pspDebugScreenSetXY(15,4);
		pspDebugScreenPrintf("%d%%",(i+1)*100/20);
	}

	jpn0 = intraFontLoad("flash0:/font/jpn0.pgf",INTRAFONT_STRING_UTF8); //japanese font with SJIS text string encoding
	intraFontSetStyle(jpn0, 1.0f, WHITE, DARKGRAY, 0);                              //scale to 80%
	pspDebugScreenSetXY(15,4);
	pspDebugScreenPrintf("%d%%",17*100/20);
        /*
	intraFont* kr0 = intraFontLoad("flash0:/font/kr0.pgf", INTRAFONT_STRING_UTF8);  //Korean font (not available on all systems) with UTF-8 encoding
	intraFontSetStyle(kr0, 0.8f, WHITE, DARKGRAY, 0);                               //scale to 80%
	pspDebugScreenSetXY(15,2);
    pspDebugScreenPrintf("%d%%",18*100/20);
           
	intraFont* arib = intraFontLoad("flash0:/font/arib.pgf",0);                     //Symbols (not available on all systems)
	intraFontSetStyle(arib, 0.8f, WHITE, DARKGRAY, 0);                              //scale to 80%
	pspDebugScreenSetXY(15,2);
    pspDebugScreenPrintf("%d%%",19*100/20);

	intraFont* chn = intraFontLoad("flash0:/font/gb3s1518.bwfon", 0);               //chinese font
	intraFontSetStyle(chn, 0.8f, WHITE, DARKGRAY, 0);                               //scale to 80%
	pspDebugScreenSetXY(15,2);*/
	pspDebugScreenPrintf("done\n");

	// Make sure the important fonts for this application are loaded
	if(!ltn[0] || !ltn[4] || !ltn[8]) sceKernelExitGame();

	// Set alternative fonts that are used in case a certain char does not exist in the main font
	intraFontSetAltFont(ltn[8], jpn0);                     //japanese font is used for chars that don't exist in latin
	//intraFontSetAltFont(jpn0, chn);                        //chinese font (bwfon) is used for chars that don't exist in japanese (and latin)
	//intraFontSetAltFont(chn, kr0);                         //korean font is used for chars that don't exist in chinese (and jap and ltn)
	//intraFontSetAltFont(kr0, arib);                        //symbol font is used for chars that don't exist in korean (and chn, jap & ltn)
	// NB: This is an extreme case - usually you need only one alternative font (e.g. japanese & chinese)
	// Also: if one of the fonts failed to load (e.g. chn) the chain breaks and all subequent fonts are not used (e.g. kr0 and arib)
}

void drawLyrics(){

	if (!lrc_available) return;
	
	//float x,y = 100;
	//char utf8_jpn[] = {0x61, 0xE3, 0x81, 0x93, 0xE3, 0x82, 0x93, 0xE3, 0x81, 0xAB, 0xE3, 0x81, 0xA1, 0xE3, 0x81, 0xAF, 0x20, 0xE4, 0xB8, 0x96, 0xE7, 0x95, 0x8C, 0};
		//intraFontSetEncoding(jpn0, INTRAFONT_STRING_UTF8);     //temporarely switch to UTF-8 (INTRAFONT_STRING_SJIS was set in intraFontLoad call)
        
        int time = MP3ME_playingTime*1000;
        
        update_state(time);
        
        intraFontSetStyle(jpn0, 1.25f,WHITE,DARKGRAY,INTRAFONT_ALIGN_CENTER);
        //intraFontPrintColumn(jpn0, 240, y, 425, debug_string);
        
        //295
        intraFontPrintColumn(jpn0, 387, 100, 175, debug_string);
        
        //x = intraFontPrint(jpn0, 10, y, debug_string);            //print UTF-8 encoded string
		//if (x == 110) intraFontPrint(ltn[8], 110, y, "[n/a]");
}

void drawTopBar(){
	int i = 0;
	drawLineScreen(0, i, 480, i, 0xfffaf8f1); i++;

	drawLineScreen(0, i, 480, i, 0xfffaf7f0); i++;
	drawLineScreen(0, i, 480, i, 0xfff7f4ee); i++;
	drawLineScreen(0, i, 480, i, 0xfff5f2eb); i++;
	drawLineScreen(0, i, 480, i, 0xfff2efe9); i++;
	drawLineScreen(0, i, 480, i, 0xffefede6); i++;
	drawLineScreen(0, i, 480, i, 0xffedeae3); i++;
	drawLineScreen(0, i, 480, i, 0xffe9e7e0); i++;
	drawLineScreen(0, i, 480, i, 0xffe5e3dc); i++;
	drawLineScreen(0, i, 480, i, 0xffe3e1d9); i++;
	drawLineScreen(0, i, 480, i, 0xffdfdcd5); i++;
	drawLineScreen(0, i, 480, i, 0xffdcdad3); i++;
	drawLineScreen(0, i, 480, i, 0xffdad7d0); i++;
	drawLineScreen(0, i, 480, i, 0xffd7d6cd); i++;
	//drawLineScreen(0, i, 480, i, 0xffd5d3ca); i++;
	fillScreenRect(0xffd5d3ca, 0, 13, 480, 3);

}

	
int main() {   
	SceCtrlData pad, oldpad;
	int quit = 0;
	pspTime local_time;
	char *pmode[4] = {"Sng", "Seq", "Rnd", "Rpt"};
	char *smode[5] = {"Title", "Genre", "Album", "Artist", "Year"};

	SetupCallbacks();
	pspDebugScreenInit();	
	
	pspDebugScreenPrintf("PSPLyrics\n\n");

	
	SceUID modid = pspSdkLoadStartModule("support.prx", PSP_MEMORY_PARTITION_KERNEL); 
		if(modid < 0)
			sceKernelExitGame();

	pspDebugScreenPrintf("Scanning files in %s...", MP3_ROOT);
	scan_mp3_files();
	sort_mp3_files();
	pspDebugScreenPrintf("finish\n\nLoading fonts: 0%%");	
	
	loadFonts();				
			
	initController();
	initGraphics();

	//scePowerSetClockFrequency(40, 40, 20);	// Moved down so that init operations aren't speed hindered
	
	MP3ME_Init(1);
	
	srand(time(NULL)); 
	
	while(!quit) {
		
		clearScreen(BLACK);
		
		// Must be called before any of the intraFont functions
		guStart();
		
		fillScreenRect(0xff000000, 0, 0, 480, 272);

		if(mp3_state == STATE_MP3_PLAY) {
			if(MP3ME_EndOfStream() == 1) {
				switch(play_mode) {
					case PLAY_MODE_SINGLE:
									MP3ME_Stop();
									mp3_state = STATE_MP3_STOP;
									break;
					case PLAY_MODE_SEQ:
									next_track();
									break;
					case PLAY_MODE_RAND:
									set_track(RAND_INT(0, num_files_cwd - 1));
									break;
					case PLAY_MODE_REPEAT:
									set_track(file_index_current_mp3);
									break;
				}
									
				
			}
	    }
	
	drawTopBar();

	drawLineScreen(295, 70, 480, 70, DARKGRAY);
	drawLineScreen(0, 170, 295, 170, DARKGRAY);
	drawLineScreen(295, 16, 295, 255, DARKGRAY);
	

	drawLineScreen(0, 255, 480, 255, YELLOW);	

	sprintf(out_str,"Bat:%d%%  CPU:%dmhz  Vol:%d%%  Mode:%s  Sort:%s",scePowerGetBatteryLifePercent(), 
			scePowerGetCpuClockFrequencyInt(), (int)((double)imposeGetVolume() / 30.0 * 100.0), 
			pmode[play_mode], smode[sort_method]);
	 
    printTextScreen(DISPLAY_X, 260, out_str, GREEN);			 
	
	printTextScreen(DISPLAY_X, 5, VERSION, BLACK);	
	 	
	display_mp3_files();  	 
	 
	if(sceRtcGetCurrentClockLocalTime(&local_time) == 0) {
		sprintf(out_str, "%02d/%02d/%02d %02d:%02d:%02d", local_time.month, local_time.day, 
				local_time.year, local_time.hour, local_time.minutes, local_time.seconds);
	} else {
		strcpy(out_str, "???");
	}

	printTextScreen(353-strlen(out_str)-DISPLAY_X, 5, out_str, BLACK);

	display_mp3_info(&files[current]);	
	

	drawLyrics();	  
	
	oldpad = pad;
	readButtons(&pad, 1);
		
	if(pad.Ly == 0)
		increase_volume(1);
		
	if(pad.Ly == 255)
		decrease_volume(1);
		
	if(pad.Lx == 255) {
		switch(play_mode) {
			case PLAY_MODE_SINGLE:
			case PLAY_MODE_SEQ:
			case PLAY_MODE_REPEAT:			
							next_track();
							break;
			case PLAY_MODE_RAND:
							set_track(RAND_INT(0, num_files_cwd - 1));
							break;
		}
	}

	if(pad.Lx == 0) {
		switch(play_mode) {
			case PLAY_MODE_SINGLE:
			case PLAY_MODE_SEQ:
			case PLAY_MODE_REPEAT:			
							prev_track();
							break;
			case PLAY_MODE_RAND:
							set_track(RAND_INT(0, num_files_cwd - 1));
							break;
		}
	}
	
 	if(pad.Buttons != oldpad.Buttons) {	 
		
		if(pad.Buttons & PSP_CTRL_SELECT) {
			if(++play_mode > PLAY_MODE_REPEAT)
				play_mode = PLAY_MODE_SINGLE;
		}
		
		if(pad.Buttons & PSP_CTRL_RTRIGGER) {
			if(++sort_method > SORT_YEAR)
				sort_method = SORT_TITLE;
			sort_mp3_files();
		}
			
	    if(pad.Buttons & PSP_CTRL_RIGHT)
			move_down(5);
			
		if(pad.Buttons & PSP_CTRL_LEFT)
			move_up(5);

		if (pad.Buttons & PSP_CTRL_DOWN)
			move_down(1);
			
		if(pad.Buttons & PSP_CTRL_UP)
			move_up(1);
			
		//if(pad.Buttons & PSP_CTRL_TRIANGLE)
			//scan_mp3_files();
			
		if(pad.Buttons & PSP_CTRL_CROSS)
			start_mp3(run_file());
		 /*
		if(pad.Buttons & PSP_CTRL_START) {
			if(usb_state == STATE_USB_OFF) {
				USBActivate();
				usb_state = STATE_USB_ON;
			} else { 
				USBDeactivate();
				usb_state = STATE_USB_OFF;
			}
		}
		*/
		
		if(pad.Buttons & PSP_CTRL_START) quit = 1;
		
		if(pad.Buttons & PSP_CTRL_SQUARE) {
			if(mp3_state == STATE_MP3_PLAY) {
				MP3ME_Pause();
				mp3_state = STATE_MP3_PAUSE;
			} else {
				if(mp3_state == STATE_MP3_PAUSE) {
					MP3ME_Play();
					mp3_state = STATE_MP3_PLAY;
				}
			}
		}

   }
   
   	// End drawing
	sceGuFinish();
	sceGuSync(0,0);
   
	sceDisplayWaitVblankStart();		
	flipScreen();
	   	  						
	}
	
	//USBEnd();
	disableGraphics();
	sceKernelExitGame();
  
	return 0;
}



