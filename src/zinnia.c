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
#include <limits.h>
#include <fcitx-utils/log.h>
#include <zinnia.h>
#include "recog.h"

// zinnia.c
// A recogniser using the Zinnia framework
// http://zinnia.sourceforge.net/

typedef struct {
	zinnia_recognizer_t* recog;
	zinnia_result_t* result;
} Zinnia;

void* ZinniaCreate(FcitxTabletConfig* cfg) {
	Zinnia* zn = fcitx_utils_new(Zinnia);
	zn->recog = zinnia_recognizer_new();
	// TODO load models
	/*
	if (!zinnia_recognizer_open(recognizer, "/usr/local/lib/zinnia/model/tomoe/handwriting-ja.model")) {
	  fprintf(stderr, "ERROR: %s\n", zinnia_recognizer_strerror(recognizer));
	  return -1;
	}*/
	zn->result = NULL;

	return zn;
}

void ZinniaDestroy(void* ud) {
	Zinnia* zn = (Zinnia*) ud;
	zinnia_result_destroy(zn->result);
	zinnia_recognizer_destroy(zn->recog);
}

void ZinniaProcess(void* ud, pt_t* points, int nPoints) {
	Zinnia* zn = (Zinnia*) ud;

	if(zn->result != NULL) {
		zinnia_result_destroy(zn->result);
		zn->result = NULL;
	}

	// get bounding box
	short xmin=SHRT_MAX,xmax=SHRT_MIN,ymin=SHRT_MAX,ymax=SHRT_MIN;
	for(int i=0; i<nPoints; ++i) {
		if(points[i].x > xmax) xmax = points[i].x;
		if(points[i].x < xmin) xmin = points[i].x;
		if(points[i].y > ymax) ymax = points[i].y;
		if(points[i].y < ymin) ymin = points[i].y;
	}
	// add some margin
	ymin -= 4;
	xmin -= 4;
	ymax += 4;
	xmax += 4;

	float sf;
	if(xmax - xmin > ymax - ymin) {
		sf = 300.0 / (xmax - xmin);
	} else {
		sf = 300.0 / (ymax - ymin);
	}

	size_t id = 0;
	zinnia_character_t *character = zinnia_character_new();
	zinnia_character_clear(character);
	zinnia_character_set_width(character, 300);
	zinnia_character_set_height(character, 300);
	// TODO strip points that are on the same slope
	for(int i=0; i<nPoints; ++i) {
		if(!PT_ISVALID(points[i])) {
			id++;
		} else {
			zinnia_character_add(character, id, sf * (points[i].x - xmin), sf * (points[i].y - ymin));
		}
	}

	zn->result = zinnia_recognizer_classify(zn->recog, character, 10);
	if (zn->result == NULL) {
		FcitxLog(ERROR, "Could not classify character");
		return;
	}

	zinnia_character_destroy(character);
}

char* ZinniaGetCandidates(void* ud) {
	Zinnia* zn = (Zinnia*) ud;
	for (int i = 0; i < zinnia_result_size(zn->result); ++i) {
		FcitxLog(WARNING, "%s\t%f\n", zinnia_result_value(zn->result, i), zinnia_result_score(zn->result, i));
	}
	return "under development";
}

FcitxTabletRecogniser zinnia = {
	ZinniaCreate,
	ZinniaProcess,
	ZinniaGetCandidates,
	ZinniaDestroy
};
