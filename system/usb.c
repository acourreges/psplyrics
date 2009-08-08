//    LightMP3
//    Copyright (C) 2007, 2008 Sakya
//    sakya_tg@yahoo.it
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#include <pspkernel.h>
#include <pspusb.h>
#include <pspusbstor.h>
#include <kubridge.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SceUID modules[7];


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USB init:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int LoadStartModule(char *path)
{
    u32 loadResult;
    u32 startResult;
    int status;

    loadResult = kuKernelLoadModule(path, 0, NULL);
    if (loadResult & 0x80000000)
       return -1;
       
    startResult = sceKernelStartModule(loadResult, 0, NULL, &status, NULL);
    if (loadResult != startResult)
       return -2;
    return loadResult;
} 

int StopUnloadModule(SceUID modID){
    int status;
    sceKernelStopModule(modID, 0, NULL, &status, NULL);
    sceKernelUnloadModule(modID);
    return 0;
}

int USBInit(){
	u32 retVal;

    //start necessary drivers
    modules[0] = LoadStartModule("flash0:/kd/chkreg.prx");
    if (modules[0] < 0)
        return -1;
    modules[1] = LoadStartModule("flash0:/kd/npdrm.prx");
    if (modules[1] < 0)
        return -1;
    modules[2] = LoadStartModule("flash0:/kd/semawm.prx");
    if (modules[2] < 0)
        return -1;
    modules[3] = LoadStartModule("flash0:/kd/usbstor.prx");
    if (modules[3] < 0)
        return -1;
    modules[4] = LoadStartModule("flash0:/kd/usbstormgr.prx");
    if (modules[4] < 0)
        return -1;
    modules[5] = LoadStartModule("flash0:/kd/usbstorms.prx");
    if (modules[5] < 0)
        return -1;
    modules[6] = LoadStartModule("flash0:/kd/usbstorboot.prx");
    if (modules[6] < 0)
        return -1;
        
    //setup USB drivers
    retVal = sceUsbStart(PSP_USBBUS_DRIVERNAME, 0, 0);
    if (retVal != 0)
		return -6;

    retVal = sceUsbStart(PSP_USBSTOR_DRIVERNAME, 0, 0);
    if (retVal != 0)
		return -7;

    retVal = sceUsbstorBootSetCapacity(0x800000);
    if (retVal != 0)
		return -8;
		
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USB get current state:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int USBGetState(){
    return sceUsbGetState();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USB activate
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int USBActivate(){
    sceUsbActivate(0x1c8);
    //sceUsbActivate(0x2d2);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USB deactivate
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int USBDeactivate(){
    sceUsbDeactivate(0x1c8);
    //sceUsbDeactivate(0x2d2);
    sceIoDevctl("fatms0:", 0x0240D81E, NULL, 0, NULL, 0 ); //Avoid corrupted files
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USB end
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int USBEnd(){
    int i;
    
    sceUsbStop(PSP_USBSTOR_DRIVERNAME, 0, 0);
    sceUsbStop(PSP_USBBUS_DRIVERNAME, 0, 0);
    for (i=6; i>=0; i--)
        StopUnloadModule(modules[i]);
    return 0;
}
