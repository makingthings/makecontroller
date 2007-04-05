// SAMBADLL.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "SAMBADLL.h"
#include "locale.h"
#include "resource.h"

// http://wiki.tcl.tk/1687
#   ifdef USE_TCL_STUBS
       // Mark this .obj as needing tcl's Stubs library.
#       pragma comment(lib, "tclstub" \
           STRINGIFY(JOIN(TCL_MAJOR_VERSION,TCL_MINOR_VERSION)) ".lib")
#       if !defined(_MT) || !defined(_DLL) || defined(_DEBUG)
           // This fixes a bug with how the Stubs library was compiled.
           // The requirement for msvcrt.lib from tclstubXX.lib should
           // be removed.
#           pragma comment(linker, "-nodefaultlib:msvcrt.lib")
#       endif
#   else
   // Mark this .obj needing the import library
#   pragma comment(lib, "tcl" \
       STRINGIFY(JOIN(TCL_MAJOR_VERSION,TCL_MINOR_VERSION)) ".lib")
#   endif

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

extern "C"{
/* =========================================================================== */
typedef struct {
	char *name;     /* Name of command. */
	Tcl_CmdProc *cmdProc;
} AEcmds;


#define TCL_ARGUMENTS \
	ClientData clientData, \
	Tcl_Interp *interp, \
	int argc, \
	CONST84 char *argv[]

// -------- general pupose  commands --------
extern SAMBADLL_API int c_look_for_usb_periph _ANSI_ARGS_((TCL_ARGUMENTS));
// -------- UART management commands --------
extern SAMBADLL_API int c_open_uart _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_reopen_uart _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_close_uart _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_config_uart _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int write_short _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int write_byte _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int write_int _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int write_data _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int read_short _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int read_byte _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int read_int _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int read_data _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_donnes_init _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_donnes_get _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_go _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_open_BulkUSB _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_close_BulkUSB _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_PCMonitorProtocol _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_TermProtocol _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_wait _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_compare _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int c_test_processus _ANSI_ARGS_((TCL_ARGUMENTS));
extern SAMBADLL_API int get_version _ANSI_ARGS_((TCL_ARGUMENTS));
char *get_script(int);

static AEcmds commands[] = {
	// --------    management commands   --------
	{"c_look_for_usb_periph",     c_look_for_usb_periph},
	{"c_open_uart",               c_open_uart},
	{"c_reopen_uart",             c_reopen_uart},
	{"c_close_uart",              c_close_uart},
	{"c_config_uart",             c_config_uart},
	{"write_short",				  write_short},
	{"write_byte",				  write_byte},
	{"write_int",				  write_int},
	{"write_data",				  write_data},
	{"read_short",				  read_short},
	{"read_byte",				  read_byte},
	{"read_int",				  read_int},
	{"read_data",				  read_data},
	{"c_compare",				  c_compare},
	{"c_go",					  c_go},
	{"c_open_BulkUSB",			  c_open_BulkUSB},
	{"c_close_BulkUSB",			  c_close_BulkUSB},
	{"c_PCMonitorProtocol",		  c_PCMonitorProtocol},
	{"c_TermProtocol",		      c_TermProtocol},
	{"c_wait",					  c_wait},
	{"c_test_processus",		  c_test_processus},
	{"get_version",				  get_version},
	{(char *) NULL,               NULL}
};

SAMBADLL_API int Samba_Init _ANSI_ARGS_((Tcl_Interp *interp))
{
	AEcmds     *cmdPtr;

	if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
		return TCL_ERROR;
    }

    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */   
    for (cmdPtr = commands; cmdPtr->name; cmdPtr++) {
		if(! Tcl_CreateCommand(interp, cmdPtr->name, cmdPtr->cmdProc,
			(ClientData ) NULL, (Tcl_CmdDeleteProc *)NULL))
			return TCL_ERROR;
	}
	return TCL_OK;

}


} // end extern C

