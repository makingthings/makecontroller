#include "stdafx.h"
#include "timer.h"

// Constructeur
Timer::Timer(){}
// Destructeur
Timer::~Timer(){}

// Initialisation du Timer
void Timer::InitTimer(const HWND pHandle, const TIMERPROC ProcAd, const int TimerID)
{

	// Handle de la fenêtre parente
	_Handle = pHandle;
	// Identificateur du Timer
	_TimerID = TimerID;
	// Adresse de la procédure qui va recevoir le message WM_TIMER
	_ProcAd = ProcAd;

}

// Démarre le Timer
bool Timer::StartTimer()
{

	// Crée le Timer et le démarre
	return (SetTimer(_Handle, _TimerID, (UINT)_Interval, _ProcAd) == TRUE);

}

// Stop le Timer
bool Timer::StopTimer()
{

	// Détruit le Timer
	return (KillTimer(_Handle, _TimerID) == TRUE);

}

// Modifier l'interval du Timer
void Timer::Interval(const int Value)
{

	// Si le nouvelle interval(Value) est différent de l'ancien(_Interval)
	if (Value != _Interval)
	{

		// Sauvegarde le nouvelle
		_Interval = Value;

		// Stop(Détruit) le Timer
		StopTimer();
		// Recrée le Timer avec le nouvelle Interval
		StartTimer();

	}

}

// Connaître l'interval du Timer
int Timer::Interval()
{

	return _Interval;

}

