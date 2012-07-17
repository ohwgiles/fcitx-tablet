/**************************************************************************
 *
 *  fcitx-tablet : graphics tablet input for fcitx input method framework
 *  Copyright 2012  Oliver Giles
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 **************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcitx-utils/log.h>
#include <fcitx-config/hotkey.h>
#include <fcitx/keys.h>
#include <fcitx/candidate.h>
#include <fcitx/context.h>
#include <fcitx/module.h>
#include "pen.h"
#include "ime.h"
#include "point.h"
#include "recog.h"
#include "config.h"


// ime.c
// This file contains the fcitx input method object. It receives notifications
// from the tablet driver, calls the recognition code and updates candidates
// or commits the string as appropriate

INPUT_RETURN_VALUE FcitxTabletGetCandWords(void* arg);
INPUT_RETURN_VALUE FcitxTabletDoInput(void* arg, FcitxKeySym sym, unsigned int action) {
	// The event module uses VoidSymbol as a trigger, this means other input methods
	// will ignore it, actual actions will be passed in the action variable,
	// see ImeAction in ime.h
	FcitxTabletPen* tablet = (FcitxTabletPen*) arg;
	if(sym == FcitxKey_VoidSymbol) {
		switch(action) {
		case IME_RECOGNISE:
			tablet->recog->Process(tablet->recog_ud, tablet->strokes.buffer, (tablet->strokes.ptr - tablet->strokes.buffer));
			// call into recognition library, update candidate lists
			return IRV_DISPLAY_CANDWORDS;
			break;
		case IME_COMMIT:
			return IRV_COMMIT_STRING;
			break;
		default:
			FcitxLog(ERROR, "IME asked to perform unknown action");
		}
		return IRV_TO_PROCESS;
	}

	if(FcitxHotkeyIsHotKey(sym, action, FCITX_BACKSPACE)) {
		if(tablet->strokes.ptr != tablet->strokes.buffer)
			return IRV_CLEAN;
		else
			return IRV_TO_PROCESS;
	} else if(FcitxHotkeyIsHotKey(sym, action, FCITX_SPACE) || FcitxHotkeyIsHotKey(sym, action, FCITX_ENTER)) {
		if(tablet->strokes.ptr != tablet->strokes.buffer) {
			char str[] = "test";
			FcitxInputContext* ic = FcitxInstanceGetCurrentIC(tablet->fcitx);
			FcitxInstanceCommitString(tablet->fcitx, ic, str);
			return IRV_CLEAN;
		} else
			return IRV_TO_PROCESS;
	}
	return IRV_TO_PROCESS;
}


// TODO maybe not needed
boolean FcitxTabletInit(void* arg) {
	return true;
}

void FcitxTabletReset(void* arg) {
	FcitxTabletPen* d = (FcitxTabletPen*) arg;
	// reset stroke buffer
	d->strokes.ptr = d->strokes.buffer;
	d->strokes.n = 0;
	// get rid of the strokes covering the screen
	// TODO programmatically
	system("xrefresh");
}


INPUT_RETURN_VALUE FcitxTabletGetCandWord(void* arg, FcitxCandidateWord* c);

INPUT_RETURN_VALUE FcitxTabletGetCandWords(void* arg) {
	FcitxLog(WARNING, "FcitxTabletGetCandWords");
	FcitxTabletPen* d = (FcitxTabletPen*) arg;

	 FcitxInputState *input = FcitxInstanceGetInputState(d->fcitx);
	 FcitxInstanceCleanInputWindow(d->fcitx);
	 char* c = d->recog->GetCandidates(d->recog_ud);
	 int len = strlen(c);
	 int i = 0;
	 do {
		int n = mblen(&c[i], len);
		if(n <= 0) break;
		FcitxCandidateWord cw;
		cw.callback = FcitxTabletGetCandWord;
		cw.strExtra = NULL;
		cw.priv = NULL;
		cw.owner = d;
		cw.wordType = MSG_OTHER;
		cw.strWord = (char*) malloc(n+1);
		memcpy(cw.strWord, &c[i], n);
		cw.strWord[n] = 0;
		FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &cw);
		i += n;
	 } while(1);
	return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE FcitxTabletGetCandWord(void* arg, FcitxCandidateWord* candWord) {
	FcitxLog(WARNING, "FcitxTabletGetCandWord");
	FcitxTabletPen* d = (FcitxTabletPen*) candWord->owner;
	FcitxInputState *input = FcitxInstanceGetInputState(d->fcitx);
	strcpy(FcitxInputStateGetOutputString(input), candWord->strWord);
	return IRV_COMMIT_STRING;
}

void FcitxTabletImeDestroy(void* arg) {
}

void* FcitxTabletImeCreate(FcitxInstance* instance) {
	FcitxTabletPen* ud = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), FCITX_TABLET_NAME)->addonInstance;

	FcitxInstanceRegisterIM(
				instance,
				ud, //userdata
				"Tablet",
				"Tablet",
				"tablet",
				FcitxTabletInit,
				FcitxTabletReset,
				FcitxTabletDoInput,
				FcitxTabletGetCandWords,
				NULL,
				NULL,
				NULL,
				NULL,
				1,
				"zh_CN"
				);


	return ud;
}


FCITX_EXPORT_API FcitxIMClass ime = {
	FcitxTabletImeCreate,
	FcitxTabletImeDestroy
};
