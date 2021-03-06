/*
 * piDrummer.c
 *
 *  Created on: 6 juil. 2016
 *	  Author: Ludovic Lemarinel
 */


/*
 * Install  Missing libraries :
 * sudo apt install libasound2-dev libsndfile1-dev libconfig++-dev libsdl-image1.2-dev libsdl-ttf2.0-dev
 *
 * Main Screen
 * Main Menu
 * 	- Kit Change
 * 		- List of kits, knob = scroll, press = ok
 *  - Metronome
 *  	- 3 text boxes with value/label.
 *  	- press switch from label/value
 *  		- BPM				(label/label)
 *  		- Volume			(label/label)
 *  		- Beat Count		(label/label)
 *  		- On/off 			(icon/label)
 *  - Audio Player
 *  	- Play/Pause 			(icon/label)
 *  	- Track Select 			(label/label)
 *  	- Play speed 			(label/label)
 *  	- Volume 				(label/label)
 *  - Training
 *  	- Timing Accuracy 		(icon/label)
 *  	- Timing W/o metronome	(icon/label)
 *  	- Warm-up				(icon/label)
 *  	- Rolls					(icon/label)
 *  - Kit Setup
 *  	- Instr. bind			(label/label)
 *  	- Pan					(icon/label)
 *  	- Reverb				(label/label)
 *  		- Delay				(label/label)
 *  		- Volume			(label/label)
 *  	- EQ					(icon/label)
 *  		- LOW Gain			(icon/label)
 *  		- LOW Freq			(label/label)
 *  		- MID Gain			(icon/label)
 *  		- MID Freq			(label/label)
 *  		- Hi Gain			(icon/label)
 *  		- Hi Freq			(label/label)
 *  - Global Setup
 *  	- Volume				(label/label)
 *  	- Reverb				(label/label)
 *  		- Delay				(label/label)
 *  		- Volume			(label/label)
 *  	- Equalizer
 *  		- LOW Gain			(icon/label)
 *  		- LOW Freq			(label/label)
 *  		- MID Gain			(icon/label)
 *  		- MID Freq			(label/label)
 *  		- Hi Gain			(icon/label)
 *  		- Hi Freq			(label/label)
 *  	- Trigger Setup
 *  		-
 *  	- Rotary shortcut		(icon/label)
 *  		- List of possible shortcuts
 *  	- Install Instr.		(icon/label)
 *  	- Advanced Setup
 *	  		- Backup/Restore	(icon/label)
 *	  		- Alsa Buffer Size	(label/label)
 *
 *  	Text input is made with knob : scroll current letter from
 *  		"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.()@*!# abcdefghijklmnopqrstuvwxyz<>~"
 *  		< and > moves carret left and right
 *  		~ delete the current character
 *
 */

#include <cstdlib>
#include <SDL/SDL.h>
#include <string>
#include <thread>

#include "soloud/soloud.h"
#include "soloud/soloud_wav.h"
#include "soloud/soloud_thread.h"
#include "Settings.h"
#include "Instrument.h"
#include "SerialHandle.h"
#include "ScreenDrawing.h"


const std::string piDrummerVersion="v0.1b";

using namespace std;
unsigned int lastTimeEvent;
unsigned int lastTimeScreenRefresh;
unsigned int ticksNow;

unsigned int labelScrollOffset;

// Song player related variables
SoLoud::Wav *songPlayer;
int songPlayerPointer;
bool songPlayerPaused;
bool done;

Settings myglobalSettings;
SDL_Surface* screen;
SoLoud::Soloud myAudioEngine;
ScreenDrawing myScreenDrawer;
SerialHandle *mySerialPort;
unsigned int HandleKeyEventRetVal;
SDLFontStore myFontstore;
Metronome myMetronome;

// function called by thread
void SerialThread(){

	unsigned int keyEvent;

	while(!done){
		// Check for serial port if bytes are available and handle them:
		keyEvent=mySerialPort->handleEvents(myglobalSettings.getCurrentDrumKit());
		if (keyEvent!=0){
			HandleKeyEventRetVal=myScreenDrawer.handleKeyPress(keyEvent);
		}
	}

	cerr <<  "SerialThread stopping."  << endl;

}
// Send a serial String (prototype):
void sendSerialString(std::string serialString);

// Main parameters are not used. Make them anonymous so the compiler won't complain.
//int main ( int argc, char** argv ){
int main ( int , char** ){
	// Global variables:

	HandleKeyEventRetVal=0;

	// initialize SDL video
	if ( SDL_Init( SDL_INIT_VIDEO ) < 0 ){
		cerr <<  "Unable to init SDL: " <<  SDL_GetError() << endl;
		return 1;
	}

	// Init the SDL_TTF part :
	if(TTF_Init() == -1){
		cerr <<  "Unable to init SDL_ttf: " <<  TTF_GetError() << endl;
		return 1;
	}

	// create a new window
	screen = SDL_SetVideoMode(320, 240, 24, SDL_HWSURFACE|SDL_DOUBLEBUF);
	if ( !screen )
	{
		cerr << "Unable to set video: " << SDL_GetError() << endl;
		return 1;
	}

	// Hide cursor !
	SDL_ShowCursor(SDL_DISABLE);


	if (!myglobalSettings.checkIfFileExists("res/arial.ttf")){
		cerr << "ERROR : Font file arial.ttf missing in .piDrummer/res folder." << endl;
		return 1;
	}

	// Draw the splash screen while we continue loading stuffs...
	myScreenDrawer.DrawSplashScreen();

	// Load the Global settings from file :
	myglobalSettings.LoadSettings();
	myglobalSettings.loadInstrumentList();
	myglobalSettings.loadDrumKitList();

	// We have loaded our default settings, if we have a last loaded drumKit, we should use it :
	string PowerOnKitName=myglobalSettings.getPowerOnKitName();
	DrumKit *PowerOnKit=myglobalSettings.GetDrumKitFromName(PowerOnKitName);
	if (PowerOnKit==NULL){
		// Get the first drumKit of the list if the list is not empty :
		if (myglobalSettings.getDrumKitList()->size()>0){
			PowerOnKit=(*myglobalSettings.getDrumKitList())[0];
		}
	}
	if (!myglobalSettings.loadDrumKit(PowerOnKit)){
		// We could'nt load the default drum kit, so init an empty one :
		cerr << "Could not load kit : " << myglobalSettings.getPowerOnKitName() << ". Generating a new one." << endl;
		myglobalSettings.setCurrentDrumKit(new DrumKit);
		myglobalSettings.getCurrentDrumKit()->setKitName("New Empty kit");
		myglobalSettings.getCurrentDrumKit()->setNumTriggerInput();
	}

	// Init serial port :
	mySerialPort = new SerialHandle(myglobalSettings.getSerialPort(), 115200);
	mySerialPort->initSerial();


	lastTimeEvent = SDL_GetTicks();


	// Instantiate the audio engine :
	myAudioEngine.init(1, 7,0, myglobalSettings.getAlsaBufferSize());		// clipping = roundoff
										// Backend = ALSA (7)
										// Sample rate = auto
										// BufferSize = 256

	// We're ready, disable splash screen after 3 seconds !
	while (SDL_GetTicks() <  lastTimeEvent + 1000 ){
		SDL_Delay(20);
		/// Wait...
	}

	done = false;

	// Run the SerialThread method in a different thread.
	std::thread t1(SerialThread);


	// Run the screen Drawer refresh in a separate thread :
	while (!done){

		// Play metronome:
		myMetronome.doClick();

		// Do all "graphic" events, including touch events
		SDL_Event event;
		while (SDL_PollEvent(&event)){
			// check for messages
			switch (event.type){
				// exit if the window is closed
			case SDL_QUIT:
				done = true;
				break;

				// check for keypresses
			case SDL_KEYDOWN:
				lastTimeEvent=SDL_GetTicks();

				HandleKeyEventRetVal=myScreenDrawer.handleKeyPress(event.key.keysym.sym);

				break;
			} // end switch
		} // end of message processing

		ticksNow=SDL_GetTicks();

		if (ticksNow > lastTimeScreenRefresh+30){
			// Increment the "label scroller offset"
			labelScrollOffset++;
			lastTimeScreenRefresh=ticksNow;

			// Call the function to draw the screen :
			myScreenDrawer.RefreshScreen();
		}

		// Do we need to poweroff because of delay ?
		if (myglobalSettings.getAutoPowerOffDelay() > 0){
			if (ticksNow > (lastTimeEvent+myglobalSettings.getAutoPowerOffDelay()*1000)){
				cerr << "The power-off delay is over !" << endl;
				HandleKeyEventRetVal=255;
			}
		}

		if (HandleKeyEventRetVal==255 || done==true){
			// Send a message to the Arduino to tell him we're going to shutdown,
			// So, it will blink the led and cut power in a few seconds...
			// TODO : send serial message to power off !

			// Display a GOODBYE screen :
			myScreenDrawer.DrawPowerOffScreen();

			// Stop the screen drawer :
			done=true;


			// Clean the SDL libs :
			TTF_Quit();
			SDL_Quit();

			//Set the current drumkit as the "power on" kit :
			myglobalSettings.setPowerOnKitName( myglobalSettings.getCurrentDrumKit()->getKitName() );

			// Saving settings
			cerr << "Saving global settings..." << endl;
			myglobalSettings.SaveSettings();

			cout << "Exited cleanly" << endl;
			// Wait for the SerialThread to exit.
			SDL_Delay(500);

			return 255;
		}

		SDL_Delay(30);


	} // end main loop

	return 0;
}

void sendSerialString(std::string serialString){
	if (mySerialPort !=NULL){
		mySerialPort->sendString(serialString);
	}
}

void sendSerialParameter(unsigned int TriggerNumber, string ParameterCode, int ParameterValue){
	/*
	 * Message format :
	 * TTPPVVVV
	 * TT = Trigger Number
	 * PP = Parameter Code
	 * VVVV = Parameter Value (variable length)
	 */

	if (mySerialPort !=NULL){
		mySerialPort->sendParameter(TriggerNumber, ParameterCode, ParameterValue);
	}
}
