#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include <jaus.h>
#include <nodeManagerInterface.h>

#if defined(WIN32)
	#undef MOUSE_MOVED	// conflict between PDCURSES and WIN32
	#include <curses.h>
	#include <windows.h>
	#define SLEEP_MS(x) Sleep(x)
	#define CLEAR "cls"
#elif defined(__linux) || defined(linux) || defined(__linux__)
	#include <ncurses.h>
	#include <termios.h>
	#include <unistd.h>
	#define CLEAR "clear"
	#define SLEEP_MS(x) usleep(x*1000)
#endif

#include "simulator.h"
#include "pd.h"
#include "gpos.h"
#include "vss.h"
#include "vehicleSim.h"
#include "wd.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DEFAULT_STRING_LENGTH 128
#define KEYBOARD_LOCK_TIMEOUT_SEC	60.0

static int mainRunning = FALSE;
static int verbose = TRUE;
static int keyboardLock = FALSE;
static FILE *logFile = NULL;
static char timeString[DEFAULT_STRING_LENGTH] = "";

// Operating specific console handles
#if defined(WIN32)
	static HANDLE handleStdin;
#elif defined(__linux) || defined(linux) || defined(__linux__)
	static struct termios newTermio;
	static struct termios storedTermio;
#endif


// Refresh screen in curses mode
void updateScreen(int keyboardLock, int keyPress)
{
	int row = 0;
	int col = 0;
	char string[256] = {0};
	PointLla vehiclePosLla;
	static int lastChoice = '1';
	
	if(!keyboardLock && keyPress != -1 && keyPress != 27 && keyPress != 12) //Magic Numbers: 27 = ESC, 12 = Ctrl+l
	{
		switch(keyPress)
		{
			case ' ':
				wdToggleRequestControl();
				break;
			
			case 'S':
				wdSetSpeed();
				break;
			
			case 'W':
				wdCreateWaypoint();
				break;
			
			default:
				lastChoice = keyPress;			
		}
	}

	clear();

	mvprintw(row,35,"Keyboard Lock:	%s", keyboardLock?"ON, Press ctrl+L to unlock":"OFF, Press ctrl+L to lock");
	
	mvprintw(row++,0,"+---------------------------+");
	mvprintw(row++,0,"|     Component Menu        |");
	mvprintw(row++,0,"|                           |");
	mvprintw(row++,0,"| 1. Vehicle Sim            |");
	mvprintw(row++,0,"| 2. Primitive Driver       |");
	mvprintw(row++,0,"| 3. GPOS / VSS             |");
	mvprintw(row++,0,"| 4. Waypoint Driver        |");
	mvprintw(row++,0,"|                           |");		
	mvprintw(row++,0,"| ESC to Exit               |");		
	mvprintw(row++,0,"+---------------------------+");

	row = 2;
	col = 40;
	switch(lastChoice)
	{
		case '1':
			mvprintw(row++,col,"Vehicle Simulator");	
			mvprintw(row++,col,"VS Update Rate:	%7.2f", vehicleSimGetUpdateRate());	
			mvprintw(row++,col,"VS Run/Pause:	%s", vehicleSimGetRunPause() == VEHICLE_SIM_PAUSE ? "Pause" : "Run");	
			row++;
			mvprintw(row++,col,"VS Vehicle X:\t%9.2f", vehicleSimGetX());	
			mvprintw(row++,col,"VS Vehicle Y:\t%9.2f", vehicleSimGetY());	
			mvprintw(row++,col,"VS Vehicle H:\t%9.2f", vehicleSimGetH());	
			mvprintw(row++,col,"VS Vehicle Speed: %7.2f", vehicleSimGetSpeed());	
		
			row++;
			mvprintw(row++,col,"VS Throttle:\t%9.2f", vehicleSimGetLinearEffortX());	
			mvprintw(row++,col,"VS Brake:\t%9.2f", vehicleSimGetResistiveEffortX());	
			mvprintw(row++,col,"VS Steering:\t%9.2f", vehicleSimGetRotationalEffort());	
		
			row++;	
			vehiclePosLla = vehicleSimGetPositionLla();
			mvprintw(row++,col,"VS Vehicle Latitude:  %+10.7f", vehiclePosLla? vehiclePosLla->latitudeRadians*DEG_PER_RAD : 0.0);
			mvprintw(row++,col,"VS Vehicle Longitude: %+10.7f", vehiclePosLla? vehiclePosLla->longitudeRadians*DEG_PER_RAD : 0.0);
			break;
	
		case '2':		
			mvprintw(row++,col,"Primitive Driver");	
			mvprintw(row++,col,"PD Update Rate:	%5.2f", pdGetUpdateRate());	
			jausAddressToString(pdGetAddress(), string );	
			mvprintw(row++,col,"PD Address:\t%s", string);	
			mvprintw(row++,col,"PD State:\t%s", jausStateGetString(pdGetState()));	
			
			row++;
			if(pdGetControllerStatus())
			{
				jausAddressToString(pdGetControllerAddress(), string);	
				mvprintw(row++,col,"PD Controller:	%s", string);	
			}
			else
			{
				mvprintw(row++,col,"PD Controller:	None");	
			}
			mvprintw(row++,col,"PD Controller SC:	%s", pdGetControllerScStatus()?"Active":"Inactive");	
			mvprintw(row++,col,"PD Controller State:	%s", jausStateGetString(pdGetControllerState()));	
			
			row++;
			mvprintw(row++,col,"PD Prop Effort X: %0.0lf", pdGetWrenchEffort()? pdGetWrenchEffort()->propulsiveLinearEffortXPercent:-1.0);	
			mvprintw(row++,col,"PD Rstv Effort X: %0.0lf", pdGetWrenchEffort()? pdGetWrenchEffort()->resistiveLinearEffortXPercent:-1.0);	
			mvprintw(row++,col,"PD Rtat Effort Z: %0.0lf", pdGetWrenchEffort()? pdGetWrenchEffort()->propulsiveRotationalEffortZPercent:-1.0);	
			break;
		
		case '3':
			mvprintw(row++,col,"Global Pose Sensor");	
			mvprintw(row++,col,"GPOS Update Rate: %7.2f", gposGetUpdateRate());	
			jausAddressToString(gposGetAddress(), string );	
			mvprintw(row++,col,"GPOS Address:\t    %s", string);	
			mvprintw(row++,col,"GPOS State:\t    %s", jausStateGetString(gposGetState()));	
			mvprintw(row++,col,"GPOS SC State:\t    %s", gposGetScActive()? "Active" : "Inactive");	
			
			row++;
			mvprintw(row++,col,"Velocity State Sensor");	
			mvprintw(row++,col,"VSS Update Rate:  %7.2f", vssGetUpdateRate());	
			jausAddressToString(vssGetAddress(), string );	
			mvprintw(row++,col,"VSS Address:\t    %s", string);	
			mvprintw(row++,col,"VSS State:\t    %s", jausStateGetString(vssGetState()));	
			mvprintw(row++,col,"VSS SC State:\t    %s", vssGetScActive()? "Active" : "Inactive");	
			break;
		
		case '4':
			mvprintw(row++,col,"Waypoint Driver");	
			mvprintw(row++,col,"WD Update Rate: %7.2f", wdGetUpdateRate());	

			jausAddressToString(wdGetAddress(), string);	
			mvprintw(row++,col,"WD Address:\t  %s", string);	
			mvprintw(row++,col,"WD State:\t  %s", jausStateGetString(wdGetState()));
			
			if(wdGetControllerAddress())
			{
				jausAddressToString(wdGetControllerAddress(), string);	
				mvprintw(row++,col,"WD Controller:\t  %s", string);	
			}
			else
			{
				mvprintw(row++,col,"WD Controller:\t  None");	
			}

			row = 11;
			col = 2;
			mvprintw(row++,col,"GPOS SC:\t    %s", wdGetGposScStatus()? "Active" : "Inactive");
			mvprintw(row++,col,"VSS SC:\t    %s", wdGetVssScStatus()? "Active" : "Inactive");
			mvprintw(row++,col,"PD Wrench SC:\t    %s", wdGetPdWrenchScStatus()? "Active" : "Inactive");
			mvprintw(row++,col,"PD State SC:\t    %s", wdGetPdStatusScStatus()? "Active" : "Inactive");
			row++;
			mvprintw(row++,col,"WD Request Control:\t%s", wdGetRequestControl()? "True" : "False");
			mvprintw(row++,col,"(Space to Toggle)");
			mvprintw(row++,col,"WD Control:\t\t%s", wdGetInControlStatus()? "True" : "False");
			mvprintw(row++,col,"PD State:\t\t%s", jausStateGetString(wdGetPdState()));
			
			row = 11;
			col = 40;
			if(wdGetGlobalWaypoint())
			{
				mvprintw(row++,col,"Global Waypoint: (%9.7lf,%9.7lf)", wdGetGlobalWaypoint()->latitudeDegrees, wdGetGlobalWaypoint()->longitudeDegrees);
			}
			else
			{
				mvprintw(row++,col,"Global Waypoint: None");
			}
					
			if(wdGetTravelSpeed())
			{
				mvprintw(row++,col,"Travel Speed: %7.2f", wdGetTravelSpeed()->speedMps);
			}
			else
			{
				mvprintw(row++,col,"Travel Speed: None");				
			}

			mvprintw(row++,col,"dSpeedMps: %7.2f", wdGetDesiredVehicleState()? wdGetDesiredVehicleState()->desiredSpeedMps : 0.0);
			mvprintw(row++,col,"dPhi:      %7.2f", wdGetDesiredVehicleState()? wdGetDesiredVehicleState()->desiredPhiEffort : 0.0);
			
			break;

		default:
			mvprintw(row++,col,"NONE.");	
			break;
	}
	
	move(24,0);
	refresh();
}

void parseUserInput(char input)
{
	switch(input)
	{
		case 12: // 12 == 'ctrl + L'
			keyboardLock = !keyboardLock;
			break;
		
		case 27: // 27 
			if(!keyboardLock)
			{
				mainRunning = FALSE;
			}
			break;
		
		default:
			break;
	}
	return;
}

void parseCommandLine(int argCount, char **argString)
{
	int i = 0;
	int debugLevel = 0;
	char logFileStr[DEFAULT_STRING_LENGTH] = "";
	char debugLogicString[DEFAULT_STRING_LENGTH] = "";
	
	for(i=1; i<argCount; i++)
	{
		if(argString[i][0] == '-')
		{
			switch(argString[i][1])
			{
				case 'v':
					verbose = TRUE;
					//setLogVerbose(TRUE);
					break;
					
				case 'd':
					if(argString[i][2] == '+') 
					{
						//setDebugLogic(DEBUG_GREATER_THAN);
						sprintf(debugLogicString, "Greater than or equal to: ");
						if(argString[i][3] >= '0' && argString[i][3] <= '9')
						{
							debugLevel = atoi(&argString[i][3]);
						}
						else
						{
							printf("main: Incorrect use of arguments\n");
							break;
						}
					}
					else if(argString[i][2] == '-')
					{
						//setDebugLogic(DEBUG_LESS_THAN);
						sprintf(debugLogicString, "Less than or equal to: ");
						if(argString[i][3] >= '0' && argString[i][3] <= '9')
						{
							debugLevel = atoi(&argString[i][3]);
						}
						else
						{
							printf("main: Incorrect use of arguments\n");
							break;
						}
					}
					else if(argString[i][2] == '=')
					{
						//setDebugLogic(DEBUG_EQUAL_TO);
						if(argString[i][3] >= '0' && argString[i][3] <= '9')
						{
							debugLevel = atoi(&argString[i][3]);
						}
						else
						{
							printf("main: Incorrect use of arguments\n");
							break;
						}
					}
					else if(argString[i][2] >= '0' && argString[i][2] <= '9')
					{
						debugLevel = atoi(&argString[i][2]);
					}
					else
					{
						printf("main: Incorrect use of arguments\n");
						break;
					}
					printf("main: Switching to debug level: %s%d\n", debugLogicString, debugLevel);
					//setDebugLevel(debugLevel);
					break;
					
				case 'l':
					if(argCount > i+1 && argString[i+1][0] != '-')
					{
						logFile = fopen(argString[i+1], "w");
						if(logFile != NULL)
						{
							fprintf(logFile, "CIMAR %s Log -- %s\n", argString[0], timeString);
							//setLogFile(logFile);
						}
						else printf("main: Error creating log file, switching to default\n");
					}
					else
					{
						printf("main: Incorrect use of arguments\n");
					}
					break;

				default:
					printf("main: Incorrect use of arguments\n");
					break;
			}
		}
	}
}

void setupTerminal()
{
	if(verbose)
	{
#if defined(__linux) || defined(linux) || defined(__linux__)
		tcgetattr(0,&storedTermio);
		memcpy(&newTermio,&storedTermio,sizeof(struct termios));
		
		// Disable canonical mode, and set buffer size to 0 byte(s)
		newTermio.c_lflag &= (~ICANON);
		newTermio.c_lflag &= (~ECHO);
		newTermio.c_cc[VTIME] = 0;
		newTermio.c_cc[VMIN] = 0;
		tcsetattr(0,TCSANOW,&newTermio);
#elif defined(WIN32)
		// Setup the console window's input handle
		handleStdin = GetStdHandle(STD_INPUT_HANDLE); 
#endif
	}
	else
	{	
		// Start up Curses window
		initscr();
		cbreak();
		noecho();
		nodelay(stdscr, 1);	// Don't wait at the getch() function if the user hasn't hit a key
		keypad(stdscr, 1); // Allow Function key input and arrow key input
	}
}

void cleanupConsole()
{
	if(verbose)
	{
#if defined(__linux) || defined(linux) || defined(__linux__)
		tcsetattr(0,TCSANOW,&storedTermio);
#endif
	}
	else
	{
		// Stop Curses
		clear();
		endwin();
	}
}

char getUserInput()
{
	char retVal = FALSE;
	int choice;
	int i = 0;

	if(verbose)
	{
	#if defined(WIN32)
    INPUT_RECORD inputEvents[128];
	DWORD eventCount;

		// See how many events are waiting for us, this prevents blocking if none
		GetNumberOfConsoleInputEvents(handleStdin, &eventCount);
		
		if(eventCount > 0)
		{
			// Check for user input here
			ReadConsoleInput( 
					handleStdin,		// input buffer handle 
					inputEvents,		// buffer to read into 
					128,				// size of read buffer 
					&eventCount);		// number of records read 
		}
 
	    // Parse console input events 
        for (i = 0; i < (int) eventCount; i++) 
        {
            switch(inputEvents[i].EventType) 
            { 
				case KEY_EVENT: // keyboard input 
					parseUserInput(inputEvents[i].Event.KeyEvent.uChar.AsciiChar);
					retVal = TRUE;
					break;
				
				default:
					break;
			}
		}
	#elif defined(__linux) || defined(linux) || defined(__linux__)
		choice = getc(stdin);
		if(choice > -1)
		{
			parseUserInput(choice);
			retVal = TRUE;
		}
	#endif
	}
	else
	{
		choice = getch(); // Get the key that the user has selected
		updateScreen(keyboardLock, choice);
		if(choice > -1)
		{
			parseUserInput(choice);
			retVal = TRUE;
		}
	}

	return retVal;
}


int main(int argCount, char **argString)
{
	int i = 0;
	char keyPressed = FALSE;
	int keyboardLock = FALSE;
	double keyboardLockTime = getTimeSeconds() + KEYBOARD_LOCK_TIMEOUT_SEC;
	time_t timeStamp;
	
	//Get and Format Time String
	time(&timeStamp);
	strftime(timeString, DEFAULT_STRING_LENGTH-1, "%m-%d-%Y %X", localtime(&timeStamp));

	system(CLEAR);

	printf("main: CIMAR Core Executable: %s\n", timeString);

//	// Make directory and set permisions on CIMAR Directory to fully read, write, execute
//	if(mkdir("/var/log/CIMAR/", 0777) < 0) 
//	{
//		if(errno != EEXIST)
//		{
//			perror("main: Error");
//			return -1;
//		}
//	}


//	if(logFile == NULL)
//	{
//		i = strlen(argString[0]) - 1;
//		while(i>0 && argString[0][i-1] != '/') i--;
//				
//		sprintf(logFileStr, "/var/log/CIMAR/%s.log", &argString[0][i]);
//		printf("main: Creating log: %s\n", logFileStr);
//		logFile = fopen(logFileStr, "w");
//		if(logFile != NULL)
//		{
//			fprintf(logFile, "CIMAR %s Log -- %s\n", argString[0], timeString);
//			//setLogFile(logFile);
//		}
//		else printf("main: ERROR: Could not create log file\n");
//		// BUG: Else pError here
//	}
			
	//cDebug(1, "main: Starting Up %s Node Software\n", simulatorGetName());
	if(simulatorStartup())
	{
		printf("main: %s Node Startup failed\n", simulatorGetName());
		//cDebug(1, "main: Exiting %s Node Software\n", simulatorGetName());
#if defined(WIN32)
		system("pause");
#else
		printf("Press ENTER to exit\n");
		getch();
#endif
		return 0;
	}

	setupTerminal();

	mainRunning = TRUE;
	
	while(mainRunning)
	{
		keyPressed = getUserInput();

		if(keyPressed)
		{
			keyboardLockTime = getTimeSeconds() + KEYBOARD_LOCK_TIMEOUT_SEC;		
		}
		else if(getTimeSeconds() > keyboardLockTime)
		{
				keyboardLock = TRUE;
		}

		//if(verbose)
		//{
		//	choice = getc(stdin);
		//}
		//else // Not in verbose mode
		//{
		//	choice = getch(); // Get the key that the user has selected
		//	updateScreen(keyboardLock, choice);		
		//}
						
		SLEEP_MS(1000);
	}

	cleanupConsole();
	
	//cDebug(1, "main: Shutting Down %s Node Software\n", simulatorGetName());
	simulatorShutdown();
	
	if(logFile != NULL)
	{
		fclose(logFile);
	}
	
	return 0;
}
