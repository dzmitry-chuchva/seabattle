/*! Time-stamp: <@(#)main.cpp   27.03.2005 - 18:45:53   Dmitry Viktorovich Chuchva>
 *********************************************************************
 *  @file   : main.cpp
 *
 *  Project : Seabattle, a small game for player with cpu
 *
 *  Package : no
 *
 *  Company : Mitro Software Inc.
 *
 *  Author  : Dmitry Viktorovich Chuchva                              Date: 27.03.2005
 *
 *  Purpose : makes all work 
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  27.03.2005  BN : First Revision
 *
 *********************************************************************
 */

/*! debug switch - if set : cpu map is saved on random building 
    after placing each ship */
#define DEBUG 0

//! exclude MFC info from windows.h, use pure WINAPI
#define WIN32_LEAN_AND_MEAN
//! include windows stuff
#include <windows.h>
//! for file i/o
#include <stdio.h>
//! for srand, abs, rand, ...
#include <stdlib.h>
//! resource id's
#include "seabattle.h"

//! window class
#define WINDOWCLASS "GUNSNROSES_FOREVER"
//! window style - WS_OVERLAPPEDWINDOW without maximize button 
#define WINDOWSTYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | \
					 WS_VISIBLE)
//! window extension style
#define WINDOWEXSTYLE 0
//! window caption
#define WINDOWNAME "Seabattle v.0.1a"

//! if debug switch set, define name of debug file
#if (DEBUG)
 #define DEBUGFNAME "debug.deb"
#endif

//! game states defs
#define GAME_RUNNING 1
#define GAME_START 2
#define GAME_STOP 3

//! map size, it is strongly recommended not change this value :)
#define MAPSIZE 10
//! hutch side length, is equal to icons size 32x32
#define BUTTONS_SIDE 32
//! gap between player and cpu maps (horizontal)
#define MAPS_GAP 50
//! gap between text output line and maps (vertical)
#define TEXT_MAP_GAP 20

/*! calculated window width and height for current combination of map and 
	buttons sizes, gaps and others
 */
#define WINDOWWIDTH (BUTTONS_SIDE * MAPSIZE * 2 + MAPS_GAP + 20)
#define WINDOWHEIGHT (BUTTONS_SIDE * MAPSIZE + 40)

//! coords of message output point
#define STARTX_TEXT 10
#define STARTY_TEXT 10
//! additional "Remaining ships" message output point
#define STARTX_TEXTSHIPS (WINDOWWIDTH - 100)
#define STARTY_TEXTSHIPS (STARTY_TEXT)
//! coords of left - top corners player's and cpu's maps
#define STARTX_BUTTONS_PLAYER (STARTX_TEXT)
#define STARTY_BUTTONS_PLAYER (STARTY_TEXT + TEXT_MAP_GAP)
#define STARTX_BUTTONS_CPU ((STARTX_BUTTONS_PLAYER) + 10 * (BUTTONS_SIDE) \
							+ (MAPS_GAP))
#define STARTY_BUTTONS_CPU (STARTY_BUTTONS_PLAYER)

//! hutches ids start num
#define BUTTONS_BASE 8000
//! player's and cpu's map's hutches ids begin from
#define PLAYER_BIDS_BEGIN (BUTTONS_BASE)
//! cpu's hutches begin immediatly after player's
#define CPU_BIDS_BEGIN (PLAYER_BIDS_BEGIN + MAPSIZE * MAPSIZE)


//! hutch attribute: ship is placed here or nothing occupies this hutch
#define PLACE_NOTHING 0
#define PLACE_SHIP 1

//! direction of ship, part (deck) of which occupies current hutch 
#define DIR_UNDEF 0
#define DIR_HORIZ 1
#define DIR_VERT 2
//! for one - deck ship direction is not applicable, so set it to DIR_DEF
#define DIR_DEF 3

//! icons ids
#define SHIP_BODY_HOR 0
#define SHIP_BODY_VER 1
#define SHIP_BROKEN 2
#define SHIP_RIGHT 3
#define SHIP_LEFT 4
#define SHIP_TOP 5
#define SHIP_BOTTOM 6
#define WATER 7
#define SHIP_ONE 8
#define SHIP_BOUND 9

//! max length of message
#define MAX_MSG 100
//! max length of ship
#define MAX_SHIPLENGTH 4
/*! ships count - sum of numbers between 1 and MAX_SHIPLENGTH,  calculated
	as epsilon(1 .. n) = n * (n + 1) / 2
*/
#define SHIP_COUNT ((MAX_SHIPLENGTH * (MAX_SHIPLENGTH + 1)) / 2)

//! options for ship drawing function
#define OPTION_DRAW 1
#define OPTION_HIDDEN 2

//! used for managing free hutches in player map in cpu AI functions
#define REGISTER_FREEPL 1
#define FREEPL_NOTNEEDED 0

/*! count of different messages about one event; events : player misses, hits,
	or destroys ships
*/
#define MISS_MSGS 3
#define GET_MSGS 5
#define SHIPD_MSGS 3

/*! ship state struct,
	contains length of ship, count of destroyed decks and direction
 */
typedef struct {
	int length;
	int broken_length;
	int direction;
} shipstate_t;

/*! main hutch descr struct,
	contains place (PLACE_* defs, see above), direction (DIR_* defs),
	bid - hutch id, hButton - handle of hutch button.
*/
typedef struct {
	int place;
	int direction;
	int bid;
	HWND hButton;
} mapfield_t;

//! struct, descripting coord in map
typedef struct {
	int i,j;
} place_t;

//! list node struct; used for managing free hutches in player map
typedef struct tag_placelist_t {
	place_t pl;
	tag_placelist_t *next;
} placelist_t;

//! main window handle
HWND g_hWnd = NULL;
//! array of icons handles, corresponding to SHIP_* , WATER defs
HANDLE myicons[IDI_COUNT];
//! device context handle of window
HDC g_hdc = NULL;
//! font handle (used MS Sans Serif)
HFONT g_hFont = NULL;
//! accelerators table of application
HACCEL g_hAccel = NULL;
//! module instance
HINSTANCE g_hInstance = NULL;

#if (DEBUG)

//! debug file
FILE *f = fopen(DEBUGFNAME,"wt");

#endif


/*! event messages, add yours if you wish, 
	but dont remember update *_MSGS defs
*/
char *missmsgs[MISS_MSGS] = {
	"You dont get it!",
	"You miss it again!",
	"You miss it!"
};

char *getmsgs[GET_MSGS] = {
	"Good! Try to finish him!",
	"Yea-a-a-h! Find more decks!",
	"OK! Plus one more deck!",
	"Find next deck!",
	"Not bad!"
};

char *shipdonemsgs[SHIPD_MSGS] = {
	"O my god!!! You did it!!!!",
	"You are really digital g0d!! Find more ships!",
	"OK! Find next ship!"
};

//! maps
mapfield_t player_map[MAPSIZE][MAPSIZE],
	cpu_map[MAPSIZE][MAPSIZE];
//! initial game state
int game_state = GAME_START;
//! showed, when user chooses about menu
char *aboutstr = "Seabattle v.0.1a, by Dmitry Chuchva, (c) 2005\nAny comments to dmitry_chuchva@mail.ru";
//! client rectagle
RECT clrect;
//! screen geom, used for calculating window position on screen (center)
int screen_width = 0,screen_height = 0;
//! current message
char currmsg[MAX_MSG];
//! vars, used when player places ships himself
int curr_length_setting = 4, ships_of_curr_length_setted = 0, curr_decks_of_ship_setted = 0, 
		curr_direction = DIR_UNDEF;
//! var, used in cpu AI func
place_t prevpl = { -100, -100 };
//! if set, player map is filled by random, else player fills map himself
int random_fill = 0;
//! if set, then now will be players shoot, else - cpu's
int player_move;
//! if set, player begins, else it's choosen by random
int player_first = 0;
//! count of almost destroyed ships both sides
int player_broken_ships = 0, cpu_broken_ships = 0;
//! count of almost destroyed ships of player per one cpu shoot
int player_broken_session = 0;
//! used in message output. if set, then message is just showed
int flushed = 0;
//! last cpu shoot coord
place_t last_cpu_shoot = { -1, -1 };
//! random sequence of directions (east,west,south,nord), used in cpu AI func
place_t directions[4] = { { -1, 0 }, { 1, 0 }, { 0, -1}, { 0, 1 } };
//! curr direction of ship processing while cpu shoot, index in directions array
int cpu_curr_dir = 0;
//! list of free hutches of player map
placelist_t *freeplaces = NULL;
//! it length
int list_length = 0;

//! functions prototypes
void ProcessClick(int);
void InitGame();
void PlaceCpuShips();
void PlacePlayerShips();
void errore(char*);
void errorwe(char*);
void (*Error)(char*);
void cleanup();
void ConsoleMsg(char*);
void ConsoleMsgBuffered(char*);
void FlushMsgs();
void DoMyPaint(HWND);
void ClearStructs();
void SetButtonState(mapfield_t&,unsigned char);
int FindPlaceForShip(mapfield_t (*map)[MAPSIZE][MAPSIZE],int length,int direction,int show);
int FindPlacesHoriz(mapfield_t (*map)[MAPSIZE][MAPSIZE],int length,place_t *places);
int FindPlacesVert(mapfield_t (*map)[MAPSIZE][MAPSIZE],int length,place_t *places);
void FindButton(int bid,place_t *pl);
int GoodFirstChoose(place_t &);
int CanPlaceShip(mapfield_t (*map)[MAPSIZE][MAPSIZE],place_t &,int length,int direction);
void RestartGame();
void ResetMaps();
int CpuShoot();
int PlayerShoot(int bid,place_t &pl);
void GetShipState(mapfield_t (*map)[MAPSIZE][MAPSIZE],place_t &,shipstate_t &);
void MarkShipBound(mapfield_t (*map)[MAPSIZE][MAPSIZE],place_t &,shipstate_t &,int);
void FillMapWithDots(mapfield_t (*map)[MAPSIZE][MAPSIZE]);
unsigned char GetButtonState(mapfield_t&);
void ResetDirections(place_t *directions);
void OppositeDir(place_t *directions,int &); 
void GetGoodRandomPlace(placelist_t *,int&,int&);
placelist_t *CreateFreePlacesListFor(mapfield_t (*map)[MAPSIZE][MAPSIZE]);
placelist_t *DeleteFromFreePlacesList(int,int);
placelist_t *CreatePlacesList(placelist_t*);
placelist_t *DeletePlacesList(placelist_t*);
placelist_t *AddPlace(placelist_t*,place_t&);
bool FreePlace(int i,int j);


#if (DEBUG)

void SaveMap(mapfield_t map[MAPSIZE][MAPSIZE]);

#endif




/*!
 * shows message and exits program with ExitProcess func
 *
 * @param *msg : error message
 *
 * @return void  : nothing
 */
void errore(char *msg)
{
	MessageBox(0,msg,"Error",MB_OK | MB_ICONSTOP);
	ExitProcess(0);
}


/*!
 * shows message and exits window cycle with following program exit
 *
 * @param *msg : error message
 *
 * @return void  : nothing
 */
void errorwe(char *msg)
{
	MessageBox(g_hWnd,msg,"Error",MB_OK | MB_ICONSTOP);
	PostQuitMessage(0);
}

//! standard window proc
LRESULT CALLBACK WndProc(HWND hWnd,UINT messg,WPARAM wParam,LPARAM lParam)
{
	int loword,hiword;

	switch (messg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		DoMyPaint(hWnd);
		return 0;
	case WM_KEYDOWN:
		//! ESC - quit
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		return 0;
	case WM_COMMAND:
		loword = (int)LOWORD(wParam);
		hiword = HIWORD(wParam);
		//! process menu and accelerator messages
		switch (loword)
		{
		case IDM_EXIT:
			PostQuitMessage(0);
			break;
		case IDA_EXIT:
			PostQuitMessage(0);
			break;
		case IDM_ABOUT:
			MessageBox(hWnd,aboutstr,"About",MB_OK | MB_ICONINFORMATION);
			break;
		case IDM_RESTART:
			RestartGame();
			break;
		case IDA_RESTART:
			RestartGame();
			break;
		case IDM_RANDOM:
			//! set/unset random fill switch for player
			random_fill = 1 - random_fill;
			CheckMenuItem(GetMenu(hWnd),IDM_RANDOM,(random_fill) ? MF_CHECKED : MF_UNCHECKED);
			if (MessageBox(hWnd,"Restart game?","Question",MB_YESNO | MB_ICONQUESTION) == IDYES)
				RestartGame();
			break;
		case IDM_PLAYERFIRST:
			//! set/unset "player always begins" switch
			player_first = 1 - player_first;
			CheckMenuItem(GetMenu(hWnd),IDM_PLAYERFIRST,(player_first) ? MF_CHECKED : MF_UNCHECKED);
			if (game_state == GAME_RUNNING)
				if (MessageBox(hWnd,"Restart game?","Question",MB_YESNO | MB_ICONQUESTION) == IDYES)
					RestartGame();
			break;
		}

		//! if it's a mouse "clicked" message and it's our map buttons
		if ((loword >= BUTTONS_BASE) && (hiword == BN_CLICKED))
		{
			ProcessClick(loword);
		}
		return 0;
	default:
		//! default message processing
		return DefWindowProc(hWnd,messg,wParam,lParam);
	}
}


//! program entry point - WinMain func
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{
	MSG msg;
	WNDCLASSEX wc;

	//! errors before window created
	Error = errore;

	//! fill window class params
	wc.hInstance = g_hInstance = hInstance;
	wc.cbSize = 48;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpfnWndProc = WndProc;
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.hCursor = LoadCursor(g_hInstance,MAKEINTRESOURCE(IDC_MYCURSOR));
	wc.hIcon = LoadIcon(g_hInstance,"windowicon");
	wc.hIconSm = wc.hIcon;
	wc.lpszMenuName = MAKEINTRESOURCE(MYMENU);
	wc.lpszClassName = WINDOWCLASS;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;

	if (!RegisterClassEx(&wc))
		Error("Invalid class");

	//! calc window pos
	screen_width = GetSystemMetrics(SM_CXSCREEN);
	screen_height = GetSystemMetrics(SM_CYSCREEN);
	clrect.left = (screen_width - WINDOWWIDTH) / 2;
	clrect.top = (screen_height - WINDOWHEIGHT) / 2;
	clrect.right = (screen_width + WINDOWWIDTH) / 2;
	clrect.bottom = (screen_height + WINDOWHEIGHT) / 2;

	//! take into account additional window element sizes, get full size of window
	if (!AdjustWindowRectEx(&clrect,WINDOWSTYLE,true,WINDOWEXSTYLE))
		Error("Cannot adjust");

	//! create window
	g_hWnd = CreateWindowEx(WINDOWEXSTYLE,WINDOWCLASS,WINDOWNAME,
		WINDOWSTYLE,clrect.left,clrect.top,clrect.right - clrect.left,clrect.bottom - clrect.top,
		NULL,LoadMenu(g_hInstance,MAKEINTRESOURCE(MYMENU)),g_hInstance,NULL);

	if (!g_hWnd)
		Error("Invalid window");

	//! window is created, process errors with corresponding function
	Error = errorwe;

	//! init game
	InitGame();

	//! message loop
	while (GetMessage(&msg,NULL,0,0))
	{
		if (!TranslateAccelerator(g_hWnd,g_hAccel,&msg))
			TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//! do deinitialization
	cleanup();

	return msg.wParam;
}


/*!
 * initializes game
 *
 * @param none
 *
 * @return void  : nothing
 */
void InitGame()
{
	int i,j;

	ClearStructs();

	//! init random seed
	srand(GetTickCount());
	//! load accels table
	g_hAccel = LoadAccelerators(g_hInstance,MAKEINTRESOURCE(MYACCELTABLE));

	//! load icons
	for (i = 0; i < IDI_COUNT; i++)
	{
		myicons[i] = LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_MYICONSBASE + i),
			IMAGE_ICON,0,0,LR_DEFAULTCOLOR);
		if (!myicons[i])
			Error("Cannot load icon");
	}
	
	//! init map structs, create buttons, set buttons icons
	for (i = 0; i < MAPSIZE; i++)
		for (j = 0; j < MAPSIZE; j++)
		{
			player_map[i][j].bid = PLAYER_BIDS_BEGIN + i * MAPSIZE + j;
			player_map[i][j].direction = DIR_UNDEF;
			player_map[i][j].place = PLACE_NOTHING;
			if (!(player_map[i][j].hButton = CreateWindowEx(0,"button","",
				WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_ICON,STARTX_BUTTONS_PLAYER + 
				j * BUTTONS_SIDE,STARTY_BUTTONS_PLAYER + i * BUTTONS_SIDE,
				BUTTONS_SIDE,BUTTONS_SIDE,g_hWnd,(HMENU)player_map[i][j].bid,
				g_hInstance,NULL)))
			{
				Error("Cannot create button");
			}
			SetButtonState(player_map[i][j],WATER);

			cpu_map[i][j].bid = CPU_BIDS_BEGIN + i * MAPSIZE + j;
			cpu_map[i][j].direction = DIR_UNDEF;
			cpu_map[i][j].place = PLACE_NOTHING;
			if (!(cpu_map[i][j].hButton = CreateWindowEx(0,"button","",
				WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_ICON,STARTX_BUTTONS_CPU + 
				j * BUTTONS_SIDE,STARTY_BUTTONS_CPU + i * BUTTONS_SIDE,
				BUTTONS_SIDE,BUTTONS_SIDE,g_hWnd,(HMENU)cpu_map[i][j].bid,g_hInstance,NULL)))
			{
				Error("Cannot create button");
			}
			SetButtonState(cpu_map[i][j],WATER);
		}

	//! set my cursor for all buttons of application
	SetClassLong(player_map[0][0].hButton,GCL_HCURSOR,(long)LoadCursor(g_hInstance,MAKEINTRESOURCE(IDC_MYCURSOR)));

	//! get device context
	g_hdc = GetDC(g_hWnd);
	//! select font MS Sans Serif
	g_hFont = CreateFont(-12,0,0,0,FW_THIN,0,0,0,ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY,FF_DONTCARE | DEFAULT_PITCH,"MS Sans Serif");
	SelectObject(g_hdc,g_hFont);
	SetBkMode(g_hdc,TRANSPARENT);
	//! place cpu ships
	PlaceCpuShips();
	//! after this, player begins to place his ships
	ConsoleMsg("Put your ships!!! First - four deck ship!");
}


/*!
 * shows message
 *
 * @param *msg : message to show
 *
 * @return void  : nothing
 */
void ConsoleMsg(char *msg)
{
	strncpy(currmsg,msg,MAX_MSG);
	//! show instanly
	FlushMsgs();
}


/*!
 * adds message to message buffer, does not shows
 *
 * @param *msg : message to add
 *
 * @return void  : nothing
 */
void ConsoleMsgBuffered(char *msg)
{
	if (!flushed)
	{
		strcat(currmsg," ");	
		strncat(currmsg,msg,MAX_MSG);	
	} 
	else
		strncpy(currmsg,msg,MAX_MSG);
	flushed = 0;
}


/*!
 * shows message buffer
 *
 * @param none
 *
 * @return void  : nothing
 */
void FlushMsgs()
{
	RECT trect;
	TEXTMETRIC tm; 


	//! calc invalidating rect
	GetTextMetrics(g_hdc,&tm);
	trect.left = STARTX_TEXT;
	trect.top = STARTY_TEXT;
	trect.right = STARTX_TEXT + MAX_MSG * tm.tmAveCharWidth;
	trect.bottom = STARTY_TEXT + tm.tmHeight;
	//! and invalidate it
	InvalidateRect(g_hWnd,&trect,true);
	trect.left = STARTX_TEXTSHIPS;
	trect.top = STARTY_TEXTSHIPS;
	trect.right = WINDOWWIDTH - 1;
	trect.bottom = STARTY_TEXTSHIPS + tm.tmHeight;
	//! also second message output
	InvalidateRect(g_hWnd,&trect,true);
	//! flushed = yes
	flushed = 1;
}


/*!
 * redraws window
 *
 * @param hWnd : window handle to redraw
 *
 * @return void  : nothing
 */
void DoMyPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc;
	char buff[MAX_MSG];

	hdc = BeginPaint(hWnd,&ps);
	//! out message buffer
	TextOut(hdc,STARTX_TEXT,STARTY_TEXT,currmsg,strlen(currmsg));
	sprintf(buff,"Ships remaining: %d",SHIP_COUNT - cpu_broken_ships);
	//! out "remaining ... " message
	TextOut(hdc,STARTX_TEXTSHIPS,STARTY_TEXTSHIPS,buff,strlen(buff));
	EndPaint(hWnd,&ps);
}


/*!
 * does deinitialization after game end
 *
 * @param none
 *
 * @return void  : nothing
 */
void cleanup()
{
	int i,j;

	//! delete free hutches list
	DeletePlacesList(freeplaces);

	//! destroy buttons
	for (i = 0; i < MAPSIZE; i++)
		for (j = 0; j < MAPSIZE; j++)
		{
			if (player_map[i][j].hButton)
			{
				DestroyWindow(player_map[i][j].hButton);
				player_map[i][j].hButton = NULL;
			}
			if (cpu_map[i][j].hButton)
			{
				DestroyWindow(cpu_map[i][j].hButton);
				cpu_map[i][j].hButton = NULL;
			}
		}

	//! destroy icons
		for (i = 0; i < IDI_COUNT; i++)
		if (myicons[i])
		{
			DeleteObject(myicons[i]);
			myicons[i] = NULL;
		}

	//! destroy device context, font handle, accel table
	if (g_hdc)
	{
		ReleaseDC(g_hWnd,g_hdc);
		g_hdc = NULL;
	}
	if (g_hFont)
	{
		DeleteObject(SelectObject(GetDC(g_hWnd),g_hFont));
		g_hFont = NULL;
	}

	if (g_hAccel)
	{
		DestroyAcceleratorTable(g_hAccel);	// not obligatory
		g_hAccel = NULL;
	}

#if (DEBUG)

	//! close debug file
	fclose(f);

#endif

}


/*!
 * clears button and icon handles with NULLs
 *
 * @param none
 *
 * @return void  : nothing
 */
void ClearStructs()
{
	int i,j;

	for (i = 0; i < MAPSIZE; i++)
		for (j = 0; j < MAPSIZE; j++)
		{
			player_map[i][j].hButton = NULL;
			cpu_map[i][j].hButton = NULL;
		}

	for (i = 0; i < IDI_COUNT; i++)
		myicons[i] = NULL;
}


/*!
 * process user clicks
 *
 * @param bid : button id just clicked
 *
 * @return void  : nothing
 */
void ProcessClick(int bid)
{
	int i,j;
	place_t pl;
	char buff[MAX_MSG];

	//! get coords of hutch, whose button is clicked
	FindButton(bid,&pl);
	//! kill this stupid frame on clicked button
	SendMessage(player_map[pl.i][pl.j].hButton,WM_KILLFOCUS,(WPARAM)g_hWnd,0);

	if (game_state == GAME_START)
	{
		//! do not let user to click cpu's map buttons
		if (bid >= CPU_BIDS_BEGIN)
			return;
		//! first click - def begin of ship
		if (curr_decks_of_ship_setted == 0)
		{
			//! check if it's possible to begin ship at this place
			if (!GoodFirstChoose(pl))
			{
				ConsoleMsg("Cannot begin ship of current length here! Try another place!");
				return;
			}
			//! set deck
			player_map[pl.i][pl.j].place = PLACE_SHIP;
			SetButtonState(player_map[pl.i][pl.j],SHIP_ONE);
			curr_decks_of_ship_setted++;
			if (curr_length_setting != 1)
				player_map[pl.i][pl.j].direction = DIR_UNDEF;
			else
				player_map[pl.i][pl.j].direction = DIR_DEF;
		}
		//! second click - complete ship in choosen direction
		else
		{
			//! it's possible now to define direction of ship
			//! direction - horizontal
			if ((prevpl.i == pl.i) && (prevpl.j != pl.j))
			{
				int dj = pl.j - prevpl.j;

				//! player must click near last clicked position to def direction
				if (abs(dj) != 1)
				{
					ConsoleMsg("Complete your ship! Just define direction of it (this means 'click near ship'), and i will complete it!");
					return;
				}
				//! check, if it's possible to complete ship in choosen dir
				if (dj < 0)
				{
					place_t tpl = { prevpl.i, prevpl.j - curr_length_setting + 1};
					if (!CanPlaceShip(&player_map,tpl,curr_length_setting,DIR_HORIZ))
					{
						ConsoleMsg("Cannot complete ship in this direction. Something prevents!");
						return;
					}
				} else
				{
					if (!CanPlaceShip(&player_map,prevpl,curr_length_setting,DIR_HORIZ))
					{
						ConsoleMsg("Cannot complete ship in this direction. Something prevents!");
						return;
					}
				}
				//! set dirs
				curr_direction = DIR_HORIZ;
				player_map[pl.i][pl.j].direction = curr_direction;
				player_map[prevpl.i][prevpl.j].direction = curr_direction;

				//! complete to right
				if (dj > 0)
				{
					//! form horizontal ship appearance
					SetButtonState(player_map[prevpl.i][prevpl.j],SHIP_LEFT);
					for (j = pl.j; j < pl.j + curr_length_setting - 2; j++)
					{
						player_map[pl.i][j].direction = curr_direction;
						player_map[pl.i][j].place = PLACE_SHIP;
						SetButtonState(player_map[pl.i][j],SHIP_BODY_HOR);
						curr_decks_of_ship_setted++;
					}
					player_map[pl.i][pl.j + curr_length_setting - 2].direction = curr_direction;
					player_map[pl.i][pl.j + curr_length_setting - 2].place = PLACE_SHIP;
					SetButtonState(player_map[pl.i][pl.j + curr_length_setting - 2],SHIP_RIGHT);
				}
				//! comlete to left
				else
				{
					//! form horizontal ship appearance
					SetButtonState(player_map[prevpl.i][prevpl.j],SHIP_RIGHT);
					for (j = pl.j; j > pl.j - curr_length_setting + 2; j--)
					{
						player_map[pl.i][j].direction = curr_direction;
						player_map[pl.i][j].place = PLACE_SHIP;
						SetButtonState(player_map[pl.i][j],SHIP_BODY_HOR);
						curr_decks_of_ship_setted++;
					}
					player_map[pl.i][pl.j - curr_length_setting + 2].direction = curr_direction;
					player_map[pl.i][pl.j - curr_length_setting + 2].place = PLACE_SHIP;
					SetButtonState(player_map[pl.i][pl.j - curr_length_setting + 2],SHIP_LEFT);
				}
				curr_decks_of_ship_setted++;
			}
			//! direction - vertical
			else if ((prevpl.j == pl.j) && (prevpl.i != pl.i))
			{
				int di = pl.i - prevpl.i;

				//! player must click near last clicked position to def direction
				if (abs(di) != 1)
				{
					ConsoleMsg("Complete your ship! Just define direction of it (this means 'click near ship'), and i will complete it!");
					return;
				}
				//! check, if it's possible to complete ship in choosen dir
				if (di < 0)
				{
					place_t tpl = { prevpl.i - curr_length_setting + 1, prevpl.j };
					if (!CanPlaceShip(&player_map,tpl,curr_length_setting,DIR_VERT))
					{
						ConsoleMsg("Cannot complete ship in this direction. Something prevents!");
						return;
					}
				} else
				{
					if (!CanPlaceShip(&player_map,prevpl,curr_length_setting,DIR_VERT))
					{
						ConsoleMsg("Cannot complete ship in this direction. Something prevents!");
						return;
					}
				}
				//! set dirs
				curr_direction = DIR_VERT;
				player_map[pl.i][pl.j].direction = curr_direction;
				player_map[prevpl.i][prevpl.j].direction = curr_direction;

				//! comlete to bottom
				if (di > 0)
				{
					//! form vertical ship appearance
					SetButtonState(player_map[prevpl.i][prevpl.j],SHIP_TOP);
					for (i = pl.i; i < pl.i + curr_length_setting - 2; i++)
					{
						player_map[i][pl.j].direction = curr_direction;
						player_map[i][pl.j].place = PLACE_SHIP;
						SetButtonState(player_map[i][pl.j],SHIP_BODY_VER);
						curr_decks_of_ship_setted++;
					}
					player_map[pl.i + curr_length_setting - 2][pl.j].direction = curr_direction;
					player_map[pl.i + curr_length_setting - 2][pl.j].place = PLACE_SHIP;
					SetButtonState(player_map[pl.i + curr_length_setting - 2][pl.j],SHIP_BOTTOM);
				}
				//! comlete to top
				else
				{
					//! form vertical ship appearance
					SetButtonState(player_map[prevpl.i][prevpl.j],SHIP_BOTTOM);
					for (i = pl.i; i > pl.i - curr_length_setting + 2; i--)
					{
						player_map[i][pl.j].direction = curr_direction;
						player_map[i][pl.j].place = PLACE_SHIP;
						SetButtonState(player_map[i][pl.j],SHIP_BODY_VER);
						curr_decks_of_ship_setted++;
					}
					player_map[pl.i - curr_length_setting + 2][pl.j].direction = curr_direction;
					player_map[pl.i - curr_length_setting + 2][pl.j].place = PLACE_SHIP;
					SetButtonState(player_map[pl.i - curr_length_setting + 2][pl.j],SHIP_TOP);
				}
				curr_decks_of_ship_setted++;
			}
			//! player clicks diagonal from begin - invalid dir
			else
			{
				ConsoleMsg("Position invalid. Ships are only vertical and horizontal");
				return;
			}
		}
		//! save last click
		memcpy(&prevpl,&pl,sizeof(place_t));
		ConsoleMsg("Continue, please!");
		//! if ship is done
		if (curr_decks_of_ship_setted == curr_length_setting)
		{
			//! update count of ship of curr length
			ships_of_curr_length_setted++;
			//! prepare to next ship
			curr_decks_of_ship_setted = 0;
			sprintf(buff,"Ship done. Now set another ship of %d length!",curr_length_setting);			
			ConsoleMsg(buff);
		}
		//! if all ships of current length done
		if (ships_of_curr_length_setted == MAX_SHIPLENGTH - curr_length_setting + 1)
		{
			/*! decrement current length (ships are set from MAX_SHIPLENGTH 
				decks downto 1)
			*/
			curr_length_setting--;
			//! if all ships of all lengths are set
			if (curr_length_setting == 0)
			{
				//! build list of free hutches for builded map
				freeplaces = CreateFreePlacesListFor(&player_map);
				//! start game
				game_state = GAME_RUNNING;
				//! define, who will begin game
				if (!player_first)
					player_move = rand() % 2;
				else
					player_move = 1;
				if (player_move)
					ConsoleMsg("Thanks. Now YOU begin the game.");
				else
				{
					ConsoleMsgBuffered("Thanks. Now CPU begins the game.");
					while (!(player_move = CpuShoot()));
				}
			}			
			//! go to next ship length
			else
			{
				sprintf(buff,"Ships of %d length done. Now begin to set ships with %d decks!",
					curr_length_setting + 1,curr_length_setting);
				ConsoleMsg(buff);
			}
			ships_of_curr_length_setted = 0;
			curr_direction = DIR_UNDEF;
		}
	}
	else
		//! game is running
		if (game_state == GAME_RUNNING)
		{
			//! player shoots
			if (player_move)
			{
				//! show msgs
				FlushMsgs();
				//! if player misses, do cpu shoot cycle
				if (!(player_move = PlayerShoot(bid,pl)))
					while (!(player_move = CpuShoot()));
			}
			//! possible, this code is unreachable, but for insurance ... 
			else
			{
				while (!(player_move = CpuShoot()));
			}
		}
		else
			//! game is stopped
			if (game_state == GAME_STOP)
			{
				ConsoleMsg("Game done. Choose File -> Restart if you want to play one more time.");
			}
}

void PlaceCpuShips()
{
	if (!FindPlaceForShip(&cpu_map,4,rand() % 2 + 1,OPTION_HIDDEN))
		return;
#if (DEBUG)

	SaveMap(cpu_map);

#endif
	if (!FindPlaceForShip(&cpu_map,3,rand() % 2 + 1,OPTION_HIDDEN))
		return;
#if (DEBUG)

	SaveMap(cpu_map);

#endif
	if (!FindPlaceForShip(&cpu_map,3,rand() % 2 + 1,OPTION_HIDDEN))
		return;
#if (DEBUG)

	SaveMap(cpu_map);

#endif
	if (!FindPlaceForShip(&cpu_map,2,rand() % 2 + 1,OPTION_HIDDEN))
		return;
#if (DEBUG)

	SaveMap(cpu_map);

#endif

	if (!FindPlaceForShip(&cpu_map,2,rand() % 2 + 1,OPTION_HIDDEN))
		return;
#if (DEBUG)

	SaveMap(cpu_map);

#endif
	if (!FindPlaceForShip(&cpu_map,2,rand() % 2 + 1,OPTION_HIDDEN))
		return;
#if (DEBUG)

	SaveMap(cpu_map);

#endif
	if (!FindPlaceForShip(&cpu_map,1,rand() % 2 + 1,OPTION_HIDDEN))
		return;
#if (DEBUG)

	SaveMap(cpu_map);

#endif
	if (!FindPlaceForShip(&cpu_map,1,rand() % 2 + 1,OPTION_HIDDEN))
		return;
#if (DEBUG)

	SaveMap(cpu_map);

#endif
	if (!FindPlaceForShip(&cpu_map,1,rand() % 2 + 1,OPTION_HIDDEN))
		return;
#if (DEBUG)

	SaveMap(cpu_map);

#endif
	if (!FindPlaceForShip(&cpu_map,1,rand() % 2 + 1,OPTION_HIDDEN))
		return;
#if (DEBUG)

	SaveMap(cpu_map);

#endif
}

void SetButtonState(mapfield_t &field,unsigned char icon_ind)
{
	if (field.hButton)
		SendMessage(field.hButton,BM_SETIMAGE,IMAGE_ICON,(LPARAM)myicons[icon_ind]);
}

int FindPlaceForShip(mapfield_t (*map)[MAPSIZE][MAPSIZE],int length,int direction,int show)
{
	place_t goodplaces[MAPSIZE * MAPSIZE];
	int i,j; int numpl;
	int pl;

	switch (direction)
	{
	case DIR_HORIZ:
		numpl = FindPlacesHoriz(map,length,goodplaces);
		break;
	case DIR_VERT:
		numpl = FindPlacesVert(map,length,goodplaces);
		break;
	}

	if (!numpl)
	{
		Error("Cannot place cpu ships");
		return 0;
	}

	pl = rand() % numpl;
	if (direction == DIR_HORIZ)
	{
		if (length == 1)
			direction = DIR_DEF;
		i = goodplaces[pl].i;
		for (j = goodplaces[pl].j; j < length + goodplaces[pl].j; j++)
		{
			(*map)[i][j].place =  PLACE_SHIP;
			(*map)[i][j].direction = direction;
			if (show == OPTION_DRAW)
				SetButtonState((*map)[i][j],SHIP_BODY_HOR);
		}
		if (show == OPTION_DRAW)
		{
			if (length == 1)
				SetButtonState((*map)[goodplaces[pl].i][goodplaces[pl].j],SHIP_ONE);
			else
			{
				SetButtonState((*map)[goodplaces[pl].i][goodplaces[pl].j],SHIP_LEFT);
				SetButtonState((*map)[goodplaces[pl].i][goodplaces[pl].j + length - 1],SHIP_RIGHT);
			}
		}
	}
	else
	{
		if (length == 1)
			direction = DIR_DEF;
		j = goodplaces[pl].j;
		for (i = goodplaces[pl].i; i < length + goodplaces[pl].i; i++)
		{
			(*map)[i][j].place =  PLACE_SHIP;
			(*map)[i][j].direction = direction;
			if (show == OPTION_DRAW)
				SetButtonState((*map)[i][j],SHIP_BODY_VER);
		}
		if (show == OPTION_DRAW)
		{
			if (length == 1)
				SetButtonState((*map)[goodplaces[pl].i][goodplaces[pl].j],SHIP_ONE);
			else
			{
				SetButtonState((*map)[goodplaces[pl].i][goodplaces[pl].j],SHIP_TOP);
				SetButtonState((*map)[goodplaces[pl].i + length - 1][goodplaces[pl].j],SHIP_BOTTOM);
			}
		}
	}
	return 1;
}

int FindPlacesHoriz(mapfield_t (*map)[MAPSIZE][MAPSIZE],int length,place_t *places)
{
	int i,j; int res = 0;
	
	for (i = 0; i < MAPSIZE; i++)
		for (j = 0; j < MAPSIZE; j++)
		{
			place_t pl = { i, j };
			
			if (CanPlaceShip(map,pl,length,DIR_HORIZ))
			{
				places[res].i = i;
				places[res].j = j;
				res++;
			}
		}
	return res;
}

int FindPlacesVert(mapfield_t (*map)[MAPSIZE][MAPSIZE],int length,place_t *places)
{
	int i,j; int res = 0;
	
	for (j = 0; j < MAPSIZE; j++)
		for (i = 0; i < MAPSIZE; i++)
		{
			place_t pl = { i, j };

			if (CanPlaceShip(map,pl,length,DIR_VERT))
			{
				places[res].i = i;
				places[res].j = j;
				res++;
			}
		}
	return res;
}

#if (DEBUG)

void SaveMap(mapfield_t map[MAPSIZE][MAPSIZE])
{
	fprintf(f,"\n");
	for (int i = 0; i < MAPSIZE; i++)
	{
		for (int j = 0; j < MAPSIZE; j++)
			fprintf(f,"%2d",map[i][j].place);
		fprintf(f,"\n");
	}
}

#endif

void FindButton(int bid,place_t *pl)
{
	if (bid < CPU_BIDS_BEGIN)
	{
		pl -> i = (bid - BUTTONS_BASE) / MAPSIZE;
		pl -> j = (bid - BUTTONS_BASE) % MAPSIZE;
	}
	else
	{
		pl -> i = (bid - CPU_BIDS_BEGIN) / MAPSIZE;
		pl -> j = (bid - CPU_BIDS_BEGIN) % MAPSIZE;
	}
}

int GoodFirstChoose(place_t &pl)
{
	place_t tplv = { pl.i - curr_length_setting + 1, pl.j };
	place_t tplh = { pl.i, pl.j - curr_length_setting + 1 };

	int fl1 = CanPlaceShip(&player_map,pl,curr_length_setting,DIR_HORIZ),
		fl2 = CanPlaceShip(&player_map,pl,curr_length_setting,DIR_VERT),
		fl3 = CanPlaceShip(&player_map,tplh,curr_length_setting,DIR_HORIZ),
		fl4 = CanPlaceShip(&player_map,tplv,curr_length_setting,DIR_VERT);
	
	if ((fl1) || (fl2) || (fl3) || (fl4))
				return 1;
	return 0;
}

int CanPlaceShip(mapfield_t (*map)[MAPSIZE][MAPSIZE],place_t &pl,int length,int direction)
{
	int k;
	int i = pl.i, j = pl.j; int fl = 0;
	int limit;

	if ((pl.i < 0) || (pl.i > MAPSIZE - 1) || (pl.j < 0) || (pl.j > MAPSIZE - 1))
		return 0;

	switch (direction)
	{
	case DIR_HORIZ:
		if ((*map)[i][j].direction != DIR_UNDEF)
				return 0;
		if (i != MAPSIZE - 1)
			if ((*map)[i + 1][j].direction != DIR_UNDEF)
				return 0;
		if (i > 0)
			if ((*map)[i - 1][j].direction != DIR_UNDEF)
				return 0;
		if (j > 0)
		{
			if ((*map)[i][j - 1].direction != DIR_UNDEF)
				return 0;
			if (i > 0)
				if ((*map)[i - 1][j - 1].direction != DIR_UNDEF)
					return 0;
			if (i != MAPSIZE - 1)
				if ((*map)[i + 1][j - 1].direction != DIR_UNDEF)
					return 0;
		}
		if (MAPSIZE - j < length)
			return 0;
		if (j + length != MAPSIZE)
		{
			if (i > 0)
				if ((*map)[i - 1][j + length].direction != DIR_UNDEF)
					return 0;
			if (i != MAPSIZE - 1)
				if ((*map)[i + 1][j + length].direction != DIR_UNDEF)
					return 0;
			limit = length + j + 1;
		}
		else
			limit = length + j;
		for (k = j + 1; k < limit; k++)
		{
			if ((*map)[i][k].direction != DIR_UNDEF)
			{
				fl = 1;
				break;
			}
			if (i != MAPSIZE - 1)
				if ((*map)[i + 1][k].direction != DIR_UNDEF)
				{
					fl = 1;
					break;
				}
			if (i > 0)
				if ((*map)[i - 1][k].direction != DIR_UNDEF)
				{
					fl = 1;
					break;
				}
		}
		if (fl)
			return 0;
		break;
	case DIR_VERT:
		if ((*map)[i][j].direction != DIR_UNDEF)
			return 0;
		if (j != MAPSIZE - 1)
			if ((*map)[i][j + 1].direction != DIR_UNDEF)
				return 0;
		if (j > 0)
			if ((*map)[i][j - 1].direction != DIR_UNDEF)
				return 0;
		if (i > 0)
		{
			if ((*map)[i - 1][j].direction != DIR_UNDEF)
				return 0;
			if (j > 0)
				if ((*map)[i - 1][j - 1].direction != DIR_UNDEF)
					return 0;
			if (j != MAPSIZE - 1)
				if ((*map)[i - 1][j + 1].direction != DIR_UNDEF)
					return 0;
		}
		if (MAPSIZE - i < length)
			return 0;
		if (i + length != MAPSIZE)
		{
			if (j > 0)
				if ((*map)[i + length][j - 1].direction != DIR_UNDEF)
					return 0;
			if (j != MAPSIZE - 1)
				if ((*map)[i + length][j + 1].direction != DIR_UNDEF)
					return 0;
			limit = length + i + 1;
		}
		else
			limit = length + i;
		
		for (k = i + 1; k < limit; k++)
		{
			if ((*map)[k][j].direction != DIR_UNDEF)
			{
				fl = 1;
				break;
			}
			if (j != MAPSIZE - 1)
				if ((*map)[k][j + 1].direction != DIR_UNDEF)
				{
					fl = 1;
					break;
				}
			if (j > 0)
				if ((*map)[k][j - 1].direction != DIR_UNDEF)
				{
					fl = 1;
					break;
				}
		}
		if (fl)
			return 0;
		break;
	}

	return 1;
}

void RestartGame()
{
	ResetMaps();
	PlaceCpuShips();
	if (random_fill)
	{
		PlacePlayerShips();
		freeplaces = CreateFreePlacesListFor(&player_map);
		if (!player_first)
			player_move = rand() % 2;
		else
			player_move = 1;
		if (player_move)
			ConsoleMsg("Game restarted. Map filled by random for you. YOU begin the game!");
		else
		{
			FlushMsgs();
			ConsoleMsgBuffered("Game restarted. Map filled by random for you. CPU begins the game!");
			while (!(player_move = CpuShoot()));
		}
		game_state = GAME_RUNNING;
	} else
	{
		ConsoleMsg("Game restarted. Put your ships!!! First - four deck ship!");
		game_state = GAME_START;
	}
}

void ResetMaps()
{
	int i,j;

	for (i = 0; i < MAPSIZE; i++)
		for (j = 0; j < MAPSIZE; j++)
		{
			cpu_map[i][j].direction = DIR_UNDEF;
			cpu_map[i][j].place = PLACE_NOTHING;
			player_map[i][j].direction = DIR_UNDEF;
			player_map[i][j].place = PLACE_NOTHING;
			SetButtonState(cpu_map[i][j],WATER);
			SetButtonState(player_map[i][j],WATER);
		}

	curr_length_setting = 4;
	ships_of_curr_length_setted = 0;
	curr_decks_of_ship_setted = 0; 
	curr_direction = DIR_UNDEF;
    prevpl.i = -100;
	prevpl.j = -100;
	cpu_broken_ships = 0;
	player_broken_ships = 0;
	player_broken_session = 0;
	last_cpu_shoot.i = -1;
	last_cpu_shoot.j = -1;
	ResetDirections(directions);
	cpu_curr_dir = 0;
}

void PlacePlayerShips()
{
	int i,j;

	for (i = MAX_SHIPLENGTH; i > 0; i--)
		for (j = 0; j < MAX_SHIPLENGTH - i + 1; j++)
			FindPlaceForShip(&player_map,i,rand() % 2 + 1,OPTION_DRAW);
}

int CpuShoot()
{
	int i,j;
	shipstate_t shipstate;

	if (last_cpu_shoot.i < 0)
	{
		GetGoodRandomPlace(freeplaces,i,j);
		if (player_map[i][j].place != PLACE_SHIP)
		{
			char buff[MAX_MSG];

			if (player_broken_session > 0)
			{
				sprintf(buff,"Cpu get %d of your ships!! Be carefull! Your shoot!!",
					player_broken_session);
				ConsoleMsgBuffered(buff);
			}
			else
				ConsoleMsgBuffered("Cpu doesnt get you! Your shoot!");
			SetButtonState(player_map[i][j],SHIP_BOUND);
			DeleteFromFreePlacesList(i,j);
			return 1;
		}
		else
		{
			SetButtonState(player_map[i][j],SHIP_BROKEN);
			last_cpu_shoot.i = i;
			last_cpu_shoot.j = j;
			GetShipState(&player_map,last_cpu_shoot,shipstate);
			if (shipstate.broken_length + 1 == shipstate.length)		// check for one-deck ships
			{
				player_map[i][j].direction = DIR_UNDEF;
				DeleteFromFreePlacesList(i,j);
				MarkShipBound(&player_map,last_cpu_shoot,shipstate,REGISTER_FREEPL);
				player_broken_ships++; player_broken_session++;
				if (player_broken_ships == SHIP_COUNT)
				{
					ConsoleMsg("YOU LOOSE. Press Ctrl - R for restart!");
					FillMapWithDots(&player_map);
					game_state = GAME_STOP;
					MessageBox(g_hWnd,"You loose. Game stopped.","Info",MB_OK | MB_ICONASTERISK);
					return 1;
				}
				last_cpu_shoot.i = -1;
				last_cpu_shoot.j = -1;
			}
			return 0;
		}
	}
	else
	{
		place_t lcs_backup = { last_cpu_shoot.i, last_cpu_shoot.j };

		while (!FreePlace(last_cpu_shoot.i + directions[cpu_curr_dir].i,
						last_cpu_shoot.j + directions[cpu_curr_dir].j))
				cpu_curr_dir++;
		
		if (cpu_curr_dir > 3)
			cpu_curr_dir = 0;

		if (player_map[last_cpu_shoot.i + directions[cpu_curr_dir].i][last_cpu_shoot.j + directions[cpu_curr_dir].j].place
				== PLACE_SHIP)
		{
			do
			{
				last_cpu_shoot.i += directions[cpu_curr_dir].i;
				last_cpu_shoot.j += directions[cpu_curr_dir].j;
				SetButtonState(player_map[last_cpu_shoot.i][last_cpu_shoot.j],SHIP_BROKEN);
				player_map[last_cpu_shoot.i][last_cpu_shoot.j].direction = DIR_UNDEF;
				DeleteFromFreePlacesList(last_cpu_shoot.i,last_cpu_shoot.j);
			}
			while (FreePlace(last_cpu_shoot.i + directions[cpu_curr_dir].i,
							last_cpu_shoot.j + directions[cpu_curr_dir].j) && 
				(player_map[last_cpu_shoot.i + directions[cpu_curr_dir].i][last_cpu_shoot.j + directions[cpu_curr_dir].j].place
					== PLACE_SHIP));
			GetShipState(&player_map,lcs_backup,shipstate);
			if (shipstate.broken_length + 1 == shipstate.length)
			{
				player_map[lcs_backup.i][lcs_backup.j].direction = DIR_UNDEF;
				DeleteFromFreePlacesList(lcs_backup.i,lcs_backup.j);
				MarkShipBound(&player_map,lcs_backup,shipstate,REGISTER_FREEPL);
				player_broken_ships++; player_broken_session++;
				if (player_broken_ships == SHIP_COUNT)
				{
					ConsoleMsg("YOU LOOSE. Press Ctrl - R for restart!");
					game_state = GAME_STOP;
					MessageBox(g_hWnd,"You loose. Game stopped.","Info",MB_OK | MB_ICONASTERISK);
					return 1;
				}
				last_cpu_shoot.i = -1;
				last_cpu_shoot.j = -1;
				cpu_curr_dir = 0;
				return 0;
			}
			else
			{
				if (FreePlace(last_cpu_shoot.i + directions[cpu_curr_dir].i,
							last_cpu_shoot.j + directions[cpu_curr_dir].j))
				{
					SetButtonState(player_map[last_cpu_shoot.i + directions[cpu_curr_dir].i][last_cpu_shoot.j + directions[cpu_curr_dir].j],
						SHIP_BOUND);
					DeleteFromFreePlacesList(last_cpu_shoot.i + directions[cpu_curr_dir].i,
						last_cpu_shoot.j + directions[cpu_curr_dir].j);
				}
				OppositeDir(directions,cpu_curr_dir);
				last_cpu_shoot.i = lcs_backup.i;
				last_cpu_shoot.j = lcs_backup.j;

				char buff[MAX_MSG];

				if (player_broken_session > 0)
				{
					sprintf(buff,"Cpu get %d of your ships!! Be carefull! Your shoot!!",
						player_broken_session);
					ConsoleMsgBuffered(buff);
				}
				else
					ConsoleMsgBuffered("Cpu damaged one of your ships! Your shoot!");
				return 1;
			}
		}
		else
		{
			SetButtonState(player_map[last_cpu_shoot.i + directions[cpu_curr_dir].i][last_cpu_shoot.j + directions[cpu_curr_dir].j],
				SHIP_BOUND);
			DeleteFromFreePlacesList(last_cpu_shoot.i + directions[cpu_curr_dir].i,
						last_cpu_shoot.j + directions[cpu_curr_dir].j);			
			cpu_curr_dir++;

			char buff[MAX_MSG];

			if (player_broken_session > 0)
			{
				sprintf(buff,"Cpu get %d of your ships!! Be carefull! Your shoot!!",
					player_broken_session);
				ConsoleMsgBuffered(buff);
			}
			else
				ConsoleMsgBuffered("Cpu doesn't get you. Your shoot!");
			return 1;
		}
	}
}

int PlayerShoot(int bid, place_t &pl)
{
	shipstate_t shipstate;

	if (bid < CPU_BIDS_BEGIN)
	{
		ConsoleMsg("Shoot at right map! Try again!");
		return 1;
	}

	player_broken_session = 0;

	if ((cpu_map[pl.i][pl.j].direction != DIR_UNDEF) 
			&& (cpu_map[pl.i][pl.j].place == PLACE_SHIP))
	{
		SetButtonState(cpu_map[pl.i][pl.j],SHIP_BROKEN);
		GetShipState(&cpu_map,pl,shipstate);
		cpu_map[pl.i][pl.j].direction = DIR_UNDEF;
		shipstate.broken_length++;
		if (shipstate.length == shipstate.broken_length)
		{
			cpu_broken_ships++;
			if (cpu_broken_ships == SHIP_COUNT)
			{
				FillMapWithDots(&cpu_map);
				ConsoleMsg("YOU WIN!!!! Congratulation! Restart game if you want play one more time.");
				game_state = GAME_STOP;
				MessageBox(g_hWnd,"You win. Game stopped.","Info",MB_OK | MB_ICONASTERISK);
			}
			else
			{
				ConsoleMsg(shipdonemsgs[rand() % SHIPD_MSGS]);
				MarkShipBound(&cpu_map,pl,shipstate,FREEPL_NOTNEEDED);
			}
		}
		else
			ConsoleMsg(getmsgs[rand() % GET_MSGS]);
		return 1;
	} else
	{
		ConsoleMsgBuffered(missmsgs[rand() % MISS_MSGS]);
		if (GetButtonState(cpu_map[pl.i][pl.j]) != SHIP_BROKEN)
			SetButtonState(cpu_map[pl.i][pl.j],SHIP_BOUND);
		return 0;
	}
}

void GetShipState(mapfield_t (*map)[MAPSIZE][MAPSIZE],place_t &pl,shipstate_t &shipst)
{
	int i,j;

	shipst.length = 0;
	shipst.broken_length = 0;
	shipst.direction = (*map)[pl.i][pl.j].direction;

	switch ((*map)[pl.i][pl.j].direction)
	{
	case DIR_HORIZ:
		i = pl.i; j = pl.j + 1;
		if (j < MAPSIZE)
			while ((*map)[i][j].place != PLACE_NOTHING)
			{
				shipst.length++;
				if ((*map)[i][j].direction == DIR_UNDEF)
					shipst.broken_length++;
				j++;
				if (j == MAPSIZE)
					break;
			}
		j = pl.j;
		while ((*map)[i][j].place != PLACE_NOTHING)
		{
			shipst.length++;
			if ((*map)[i][j].direction == DIR_UNDEF)
				shipst.broken_length++;
			j--;
			if (j < 0)
				break;
		}
		break;
	case DIR_VERT:
		i = pl.i + 1; j = pl.j;
		if (i < MAPSIZE)
			while ((*map)[i][j].place != PLACE_NOTHING)
			{
				shipst.length++;
				if ((*map)[i][j].direction == DIR_UNDEF)
					shipst.broken_length++;
				i++;
				if (i == MAPSIZE)
					break;
			}
		i = pl.i;
		while ((*map)[i][j].place != PLACE_NOTHING)
		{
			shipst.length++;
			if ((*map)[i][j].direction == DIR_UNDEF)
				shipst.broken_length++;
			i--;
			if (i < 0)
				break;
		}
		break;
	case DIR_DEF:
		shipst.broken_length = 0;
		shipst.length = 1;
		break;
	}
}

void MarkShipBound(mapfield_t (*map)[MAPSIZE][MAPSIZE],place_t &pl,shipstate_t &shipstate,int freepl_f)
{
	int start,limit;
	int i,j;

	switch (shipstate.direction)
	{
	case DIR_HORIZ:
		if (pl.j > 0)
			while ((*map)[pl.i][pl.j - 1].place == PLACE_SHIP)
			{
				pl.j--;
				if (pl.j == 0)
					break;
			}
		if (pl.j + shipstate.length < MAPSIZE)
			limit = pl.j + shipstate.length;
		else
			limit = pl.j + shipstate.length - 1;
		if (pl.j > 0)
			start = pl.j - 1;
		else
			start = pl.j;
		for (j = start; j <= limit; j++)
		{
			if (pl.i > 0)
			{
				SetButtonState((*map)[pl.i - 1][j],SHIP_BOUND);
				if (freepl_f == REGISTER_FREEPL)
					DeleteFromFreePlacesList(pl.i - 1,j);
			}
			if (pl.i != MAPSIZE - 1)
			{
				SetButtonState((*map)[pl.i + 1][j],SHIP_BOUND);
				if (freepl_f == REGISTER_FREEPL)
					DeleteFromFreePlacesList(pl.i + 1,j);
			}
		}
		if (pl.j > 0)
		{
			SetButtonState((*map)[pl.i][pl.j - 1],SHIP_BOUND);
			if (freepl_f == REGISTER_FREEPL)
				DeleteFromFreePlacesList(pl.i,pl.j - 1);
		}
		if ((*map)[pl.i][limit].place == PLACE_NOTHING)
		{
			SetButtonState((*map)[pl.i][limit],SHIP_BOUND);
			if (freepl_f == REGISTER_FREEPL)
				DeleteFromFreePlacesList(pl.i,limit);
		}
		break;
	case DIR_VERT:
		if (pl.i > 0)
			while ((*map)[pl.i - 1][pl.j].place == PLACE_SHIP)
			{
				pl.i--;
				if (pl.i == 0)
					break;
			}
		if (pl.i + shipstate.length < MAPSIZE)
			limit = pl.i + shipstate.length;
		else
			limit = pl.i + shipstate.length - 1;
		if (pl.i > 0)
			start = pl.i - 1;
		else
			start = pl.i;
		for (i = start; i <= limit; i++)
		{
			if (pl.j > 0)
			{
				SetButtonState((*map)[i][pl.j - 1],SHIP_BOUND);
				if (freepl_f == REGISTER_FREEPL)
					DeleteFromFreePlacesList(i,pl.j - 1);
			}
			if (pl.j != MAPSIZE - 1)
			{
				SetButtonState((*map)[i][pl.j + 1],SHIP_BOUND);
				if (freepl_f == REGISTER_FREEPL)
					DeleteFromFreePlacesList(i,pl.j + 1);
			}
		}
		if (pl.i > 0)
		{
			SetButtonState((*map)[pl.i - 1][pl.j],SHIP_BOUND);
			if (freepl_f == REGISTER_FREEPL)
				DeleteFromFreePlacesList(pl.i - 1,pl.j);
		}
		if ((*map)[limit][pl.j].place == PLACE_NOTHING)
		{
			SetButtonState((*map)[limit][pl.j],SHIP_BOUND);
			if (freepl_f == REGISTER_FREEPL)
				DeleteFromFreePlacesList(limit,pl.j);
		}
		break;
	case DIR_DEF:
		if (pl.i > 0)
		{
			if (pl.j > 0)
			{
				SetButtonState((*map)[pl.i - 1][pl.j - 1],SHIP_BOUND);
				if (freepl_f == REGISTER_FREEPL)
					DeleteFromFreePlacesList(pl.i - 1,pl.j - 1);
			}
			if (pl.j != MAPSIZE - 1)
			{
				SetButtonState((*map)[pl.i - 1][pl.j + 1],SHIP_BOUND);
				if (freepl_f == REGISTER_FREEPL)
					DeleteFromFreePlacesList(pl.i - 1,pl.j + 1);
			}
			SetButtonState((*map)[pl.i - 1][pl.j],SHIP_BOUND);
			if (freepl_f == REGISTER_FREEPL)
					DeleteFromFreePlacesList(pl.i - 1,pl.j);
		}
		if (pl.i != MAPSIZE - 1)
		{
			if (pl.j > 0)
			{
				SetButtonState((*map)[pl.i + 1][pl.j - 1],SHIP_BOUND);
				if (freepl_f == REGISTER_FREEPL)
					DeleteFromFreePlacesList(pl.i + 1,pl.j - 1);
			}
			if (pl.j != MAPSIZE - 1)
			{
				SetButtonState((*map)[pl.i + 1][pl.j + 1],SHIP_BOUND);
				if (freepl_f == REGISTER_FREEPL)
					DeleteFromFreePlacesList(pl.i + 1,pl.j + 1);
			}
			SetButtonState((*map)[pl.i + 1][pl.j],SHIP_BOUND);
			if (freepl_f == REGISTER_FREEPL)
					DeleteFromFreePlacesList(pl.i + 1,pl.j);
		}
		if (pl.j > 0)
		{
				SetButtonState((*map)[pl.i][pl.j - 1],SHIP_BOUND);
				if (freepl_f == REGISTER_FREEPL)
					DeleteFromFreePlacesList(pl.i,pl.j - 1);
		}
		if (pl.j != MAPSIZE - 1)
		{
				SetButtonState((*map)[pl.i][pl.j + 1],SHIP_BOUND);
				if (freepl_f == REGISTER_FREEPL)
					DeleteFromFreePlacesList(pl.i,pl.j + 1);
		}
		break;
	}	
}

void FillMapWithDots(mapfield_t (*map)[MAPSIZE][MAPSIZE])
{
	HANDLE h;

	for (int i = 0; i < MAPSIZE; i++)
		for (int j = 0; j < MAPSIZE; j++)
		{
			if ((*map)[i][j].place == PLACE_NOTHING)
			{
				h = (HANDLE)SendMessage((*map)[i][j].hButton,BM_GETIMAGE,(WPARAM)IMAGE_ICON,0);			
				if (h != myicons[SHIP_BOUND])
					SetButtonState((*map)[i][j],SHIP_BOUND);
			}
		}
}

unsigned char GetButtonState(mapfield_t& fl)
{
	HANDLE h;
	unsigned char i;

	h = (HANDLE)SendMessage(fl.hButton,BM_GETIMAGE,IMAGE_ICON,0);
	for (i = 0; i < IDI_COUNT; i++)
		if (h == myicons[i])
			break;
	return i;
}

void ResetDirections(place_t *directions)
{
	int seedd = rand() % 3 - 1;		/// seedd = { -1 , 0 , 1 } 

	directions[0].i = seedd;
	directions[0].j = abs(seedd) - 1;

	directions[1].i = - seedd;
	directions[1].j = - abs(seedd) + 1;

	directions[2].i = abs(seedd) - 1;
	directions[2].j = seedd;

	directions[3].i = - abs(seedd) + 1;
	directions[3].j = - seedd;
}

void OppositeDir(place_t *directions,int &cpd)
{
	place_t opp = { - directions[cpd].i, - directions[cpd].j  };

	int i = 0;

	while (!((directions[i].i == opp.i) && (directions[i].j == opp.j)))
		i++;

	cpd = i;
}

void GetGoodRandomPlace(placelist_t *list,int &i,int &j)
{
	int num, k = 0;
	placelist_t *tlist = freeplaces;

	if ((list_length == 0) || (!freeplaces))
	{
		MessageBox(g_hWnd,"Error! freeplaces is null or empty!","Fatal error",MB_OK);
		PostQuitMessage(0);
	}

	num = rand() % list_length;
	while (k < num)
	{
		tlist = tlist -> next;		
		k++;
	}
	i = tlist -> pl.i;
	j = tlist -> pl.j;
}

placelist_t *CreatePlacesList(placelist_t* list)
{
	if (list)
		list = DeletePlacesList(list);
	
	list = new placelist_t;
	list -> next = NULL;
	return list;
}

placelist_t *DeletePlacesList(placelist_t* list)
{
	placelist_t *tlist;

	while (list)
	{
		tlist = list;
		list = list -> next;
		delete tlist;
	}

	list = NULL;
	return NULL;
}

placelist_t *AddPlace(placelist_t* list,place_t& pl)
{
	placelist_t *tlist = list;

	while (tlist -> next)
	{
		tlist = tlist -> next;
	}
	tlist -> pl.i = pl.i;
	tlist -> pl.j = pl.j;
	tlist -> next = new placelist_t;
	tlist -> next -> next = NULL;
	return tlist;
}

placelist_t *CreateFreePlacesListFor(mapfield_t (*map)[MAPSIZE][MAPSIZE])
{
	if (freeplaces)
		freeplaces = DeletePlacesList(freeplaces);

	freeplaces = CreatePlacesList(freeplaces);
	list_length = 0;
	for (int i = 0; i < MAPSIZE; i++)
		for (int j = 0; j < MAPSIZE; j++)
		{
			place_t pl = { i, j };
			AddPlace(freeplaces,pl);
			list_length++;
		}

	return freeplaces;
}

placelist_t *DeleteFromFreePlacesList(int i, int j)
{
	placelist_t *tlist = freeplaces, *prev = freeplaces;

	while (tlist)
	{
		if ((tlist -> pl.i == i) && (tlist -> pl.j == j))
		{
			if (tlist == freeplaces)
				freeplaces = tlist -> next;
			else
				prev -> next = tlist -> next;
			delete tlist;
			list_length--;
			break;
		}
		prev = tlist;
		tlist = tlist -> next;
	}
	return freeplaces;
}

bool FreePlace(int i,int j)
{
	placelist_t *tlist = freeplaces;

	while (tlist)
	{
		if ((tlist -> pl.i == i) && (tlist -> pl.j == j))
			return true;
		tlist = tlist -> next;
	}
	return false;
}
