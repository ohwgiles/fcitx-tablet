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
#include <fcitx/candidate.h>
#include <fcitx/context.h>
#include <fcitx/module.h>
#include "pen.h"
#include "ime.h"
#include "point.h"

// ime.c
// This file contains the fcitx input method object. It receives notifications
// from the tablet driver, calls the recognition code and updates candidates
// or commits the string as appropriate

typedef struct {
	// Location of the stored pen strokes, to pass through to the recogniser
	pt_t** stroke_buffer;
	FcitxInstance* fcitx;
} FcitxTabletIme;


INPUT_RETURN_VALUE FcitxTabletDoInput(void* arg, FcitxKeySym sym, unsigned int action) {
	// The event module uses VoidSymbol as a trigger, this means other input methods
	// will ignore it, actual actions will be passed in the action variable,
	// see ImeAction in ime.h
	if(sym == FcitxKey_VoidSymbol) {
		switch(action) {
		case IME_RECOGNISE:
			// call into recognition library, update candidate lists
			break;
		case IME_COMMIT:

			break;
		default:
			FcitxLog(ERROR, "IME asked to perform unknown action");
		}
	}
	return IRV_TO_PROCESS;
}


// TODO maybe not needed
boolean FcitxTabletInit(void* arg) {
	//FcitxTablet* tablet = (FcitxTablet*) arg;
	//FcitxLog(WARNING, "FcitxTabletInit, xkb layout: %s", tablet->conf.xkbLayout);
	//FcitxInstanceSetContext(tablet->fcitx, CONTEXT_IM_KEYBOARD_LAYOUT, tablet->conf.xkbLayout);
	return true;
}

// TODO maybe not needed
void FcitxTabletReset(void* arg) {
	FcitxLog(WARNING, "FcitxTabletReset");
}


// TODO maybe not needed, tablet input is one character at a time
INPUT_RETURN_VALUE FcitxTabletGetCandWords(void* arg) {
	FcitxLog(WARNING, "FcitxTabletGetCandWords");

	/*
	 FcitxTablet* chewing = (FcitxTablet*) arg;
	 FcitxInputState *input = FcitxInstanceGetInputState(chewing->owner);
	 FcitxMessages *msgPreedit = FcitxInputStateGetPreedit(input);
	 FcitxMessages *clientPreedit = FcitxInputStateGetClientPreedit(input);
	 TabletContext * ctx = chewing->context;
	 FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(chewing->owner);

	 chewing_set_candPerPage(ctx, config->iMaxCandWord);
	 FcitxCandidateWordSetPageSize(FcitxInputStateGetCandidateList(input), config->iMaxCandWord);

	 //clean up window asap
	 FcitxInstanceCleanInputWindow(chewing->owner);

	 char * buf_str = chewing_buffer_String(ctx);
	 char * zuin_str = chewing_zuin_String(ctx, NULL);
	 ConfigTablet(chewing);

	 FcitxLog(DEBUG, "%s %s", buf_str, zuin_str);

	 int index = 0;
	 // if not check done, so there is candidate word
	 if (!chewing_cand_CheckDone(ctx)) {
		  //get candidate word
		  chewing_cand_Enumerate(ctx);
		  while (chewing_cand_hasNext(ctx)) {
				char* str = chewing_cand_String(ctx);
				FcitxCandidateWord cw;
				TabletCandWord* w = (TabletCandWord*) fcitx_utils_malloc0(sizeof(TabletCandWord));
				w->index = index;
				cw.callback = FcitxTabletGetCandWord;
				cw.owner = chewing;
				cw.priv = w;
				cw.strExtra = NULL;
				cw.strWord = strdup(str);
				cw.wordType = MSG_OTHER;
				FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &cw);
				chewing_free(str);
				index ++;
		  }
	 }

	 // there is nothing
	 if (strlen(zuin_str) == 0 && strlen(buf_str) == 0 && index == 0)
		  return IRV_DISPLAY_CANDWORDS;

	 // setup cursor
	 FcitxInputStateSetShowCursor(input, true);
	 int cur = chewing_cursor_Current(ctx);
	 FcitxLog(DEBUG, "cur: %d", cur);
	 int rcur = FcitxTabletGetRawCursorPos(buf_str, cur);
	 FcitxInputStateSetCursorPos(input, rcur);
	 FcitxInputStateSetClientCursorPos(input, rcur);

	 // insert zuin in the middle
	 char * half1 = strndup(buf_str, rcur);
	 char * half2 = strdup(buf_str + rcur);
	 FcitxMessagesAddMessageAtLast(msgPreedit, MSG_INPUT, "%s%s%s", half1, zuin_str, half2);
	 FcitxMessagesAddMessageAtLast(clientPreedit, MSG_INPUT, "%s%s%s", half1, zuin_str, half2);
	 chewing_free(buf_str); chewing_free(zuin_str); free(half1); free(half2);
*/
	return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE FcitxTabletGetCandWord(void* arg, FcitxCandidateWord* candWord) {
/*
	 FcitxTablet* chewing = (FcitxTablet*) candWord->owner;
	 TabletCandWord* w = (TabletCandWord*) candWord->priv;
	 FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(chewing->owner);
	 FcitxInputState *input = FcitxInstanceGetInputState(chewing->owner);
	 int page = w->index / config->iMaxCandWord;
	 int off = w->index % config->iMaxCandWord;
	 if (page < 0 || page >= chewing_cand_TotalPage(chewing->context))
		  return IRV_TO_PROCESS;
	 int lastPage = chewing_cand_CurrentPage(chewing->context);
	 while (page != chewing_cand_CurrentPage(chewing->context)) {
		  if (page < chewing_cand_CurrentPage(chewing->context)) {
				chewing_handle_Left(chewing->context);
		  }
		  if (page > chewing_cand_CurrentPage(chewing->context)) {
				chewing_handle_Right(chewing->context);
		  }
		  // though useless, but take care if there is a bug cause freeze
		  if (lastPage == chewing_cand_CurrentPage(chewing->context)) {
				break;
		  }
		  lastPage = chewing_cand_CurrentPage(chewing->context);
	 }
	 chewing_handle_Default( chewing->context, selKey[off] );

	 if (chewing_keystroke_CheckAbsorb(chewing->context)) {
		  return IRV_DISPLAY_CANDWORDS;
	 } else if (chewing_keystroke_CheckIgnore(chewing->context)) {
		  return IRV_TO_PROCESS;
	 } else if (chewing_commit_Check(chewing->context)) {
		  char* str = chewing_commit_String(chewing->context);
		  strcpy(FcitxInputStateGetOutputString(input), str);
		  chewing_free(str);
		  return IRV_COMMIT_STRING;
	 } else
		  return IRV_DISPLAY_CANDWORDS;
		  */
	return IRV_COMMIT_STRING;

}

void FcitxTabletImeDestroy(void* arg)
{
}

void* FcitxTabletImeCreate(FcitxInstance* instance) {
	FcitxTabletIme* ime = fcitx_utils_new(FcitxTabletIme);
	FcitxInstanceRegisterIM(
				instance,
				ime, //userdata
				"Tablet",
				"Tablet",
				"Tablet",
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

	// Get the location of the stroke buffer from the tablet event module
	FcitxModuleFunctionArg args;
	ime->stroke_buffer = (pt_t**) InvokeFunction(instance, FCITX_TABLET, GETRESULTBUFFER, args);
	return ime;
}


FCITX_EXPORT_API FcitxIMClass ime = {
	FcitxTabletImeCreate,
	FcitxTabletImeDestroy
};
