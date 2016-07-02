/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * This file is the starting point of the program and implements
 * the main loop
 *
 * =======================================================================
 */

#include <fcntl.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "../../common/header/common.h"
#include "header/unix.h"


time_t modifiedTime(const char* filename){
    struct stat st;
    uint32_t res = stat(filename, &st);
    if(res != 0){
        fprintf(stderr, "could not open file: %s\n", filename);
    }
    return st.st_mtime;
}

int main(int argc, char **argv){

	/* register signal handler */
	registerHandler();

	/* Prevent running Quake II as root. Only very mad
	   minded or stupid people even think about it. :) */
	if (getuid() == 0)
	{
		printf("Quake II shouldn't be run as root! Backing out to save your ass. If\n");
		printf("you really know what you're doing, edit src/unix/main.c and remove\n");
		printf("this check. But don't complain if Quake II eats your dog afterwards!\n");

		return 1;
	}

	/* Enforce the real UID to
	   prevent setuid crap */
	if (getuid() != geteuid())
	{
		printf("The effective UID is not the real UID! Your binary is probably marked\n");
		printf("'setuid'. That is not good idea, please fix it :) If you really know\n");
		printf("what you're doing edit src/unix/main.c and remove this check. Don't\n");
		printf("complain if Quake II eats your dog afterwards!\n");

		return 1;
	}

	/* enforce C locale */
	setenv("LC_ALL", "C", 1);

	const char* versionString = va("Yamagi Quake II v%s", YQ2VERSION);
	int verLen = strlen(versionString);
	printf("\n%s\n", versionString);
	for(int i = 0; i < verLen; ++i)
	{
		putc('=', stdout);
	}
	puts("\n");

#ifndef DEDICATED_ONLY
	printf("Client build options:\n");
#ifdef SDL2
	printf(" + SDL2\n");
#else
	printf(" - SDL2 (using 1.2)\n");
#endif

#ifdef CDA
	printf(" + CD audio\n");
#else
	printf(" - CD audio\n");
#endif
#ifdef OGG
	printf(" + OGG/Vorbis\n");
#else
	printf(" - OGG/Vorbis\n");
#endif
#ifdef USE_OPENAL
	printf(" + OpenAL audio\n");
#else
	printf(" - OpenAL audio\n");
#endif
#ifdef ZIP
	printf(" + Zip file support\n");
#else
	printf(" - Zip file support\n");
#endif
#endif

	printf("Platform: %s\n", BUILDSTRING);
	printf("Architecture: %s\n", CPUSTRING);

	/* Seed PRNG */
	randk_seed();

	/* Initialze the game */
	Qcommon_Init(argc, argv);

	/* Do not delay reads on stdin*/
	fcntl(fileno(stdin), F_SETFL, fcntl(fileno(stdin), F_GETFL, NULL) | FNDELAY);

	int time, newtime;
	int oldtime = Sys_Milliseconds();

    time_t mtime = modifiedTime(argv[0]);
    int checktime = 0;

	/* The legendary Quake II mainloop */
	while (1){
		/* find time spent rendering last frame */
		do{
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
		} while (time < 1);

        checktime += time;
        if(checktime >= 1000){
            if(mtime != modifiedTime(argv[0])){
                return 0;
            }
            checktime = 0;
        }
		Qcommon_Frame(time);
		oldtime = newtime;
	}
	return 0;
}

