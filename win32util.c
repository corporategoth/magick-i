#ifdef WIN32
#include <windows.h>
#include <signal.h>
#include "win32util.h"

VOID CALLBACK MyTimerProc(
  

    HWND  hwnd,
    UINT  uMsg,
    UINT  idEvent,
    DWORD  dwTime
   )
{
	raise(SIGALRM);
}

void __cdecl alarm(int seconds)
{
	static UINT SetRetVal;
	if(seconds!=0)
	{
		SetRetVal=SetTimer(NULL,                /* handle of main window */ 
		0,               /* timer identifier      */ 
		seconds*1000,                     /* 5-second interval     */ 
		(TIMERPROC) MyTimerProc); /* timer callback        */
	}
	else
		KillTimer(NULL,SetRetVal);
}

#endif /*MSC_VER*/