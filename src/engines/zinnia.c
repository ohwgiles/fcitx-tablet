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
#include "engine.h"
#include "line.h"
// zinnia.c
// A recogniser using the Zinnia framework
// http://zinnia.sourceforge.net/

typedef struct {
	zinnia_recognizer_t* recog;
	zinnia_result_t* result;
	char* candidates;
} Zinnia;

void* ZinniaCreate(FcitxTabletConfig* cfg) {
	Zinnia* zn = fcitx_utils_new(Zinnia);
	zn->recog = zinnia_recognizer_new();
	// create the engine
	if (!zinnia_recognizer_open(zn->recog, cfg->ZinniaModel)) {
		FcitxLog(ERROR, "Could not create Zinnia engine: %s", zinnia_recognizer_strerror(zn->recog));
		return NULL;
	}
	zn->result = NULL;
	// The output string. 30 chars should be plenty.
	zn->candidates = (char*) malloc(30 * sizeof(char));
	return zn;
}

void ZinniaDestroy(void* ud) {
	Zinnia* zn = (Zinnia*) ud;
	zinnia_result_destroy(zn->result);
	zinnia_recognizer_destroy(zn->recog);
	free(zn->candidates);
}

void ZinniaProcess(void* ud, pt_t* points, int nPoints) {
	Zinnia* zn = (Zinnia*) ud;
	// clear the last result
	if(zn->result != NULL) {
		zinnia_result_destroy(zn->result);
		zn->result = NULL;
	}
	// get bounding box
	short xmin=SHRT_MAX,xmax=SHRT_MIN,ymin=SHRT_MAX,ymax=SHRT_MIN;
	for(int i=0; i<nPoints; ++i) {
		if(PT_ISVALID(points[i])) {
			if(points[i].x > xmax) xmax = points[i].x;
			if(points[i].x < xmin) xmin = points[i].x;
			if(points[i].y > ymax) ymax = points[i].y;
			if(points[i].y < ymin) ymin = points[i].y;
		}
	}
	// add some margin
	ymin -= 4;
	xmin -= 4;
	ymax += 4;
	xmax += 4;
	// determine scaling factors
	float sf;
	if(xmax - xmin > ymax - ymin) {
		sf = 300.0 / (xmax - xmin);
		ymin = 0;
	} else {
		sf = 300.0 / (ymax - ymin);
		xmin = 0;
	}
	// the stroke index
	size_t id = 0;
	// create a new character
	zinnia_character_t *character = zinnia_character_new();
	zinnia_character_clear(character);
	zinnia_character_set_width(character, 300);
	zinnia_character_set_height(character, 300);
	// the index in the point array from which this stroke begins
	int from = 0;
	for(int i=0; i<nPoints; ++i) {
		// If we get to a stroke break, do the magic
		if(!PT_ISVALID(points[i])) {
			pt_t* simplified = 0;
			// Simplify the line we have so far. An epsilon of 6 is a guess,
			// it probably should be determined based on xmax,xmin,ymax,ymin
			int n = SimplifyLine(&points[from],i-from,&simplified,6);
			// Add the scaled stroke to the zinnia character
			for(int j=0;j<n;++j)
				zinnia_character_add(character, id, sf * (simplified[j].x - xmin), sf * (simplified[j].y - ymin));
			// clear the simplified stroke memory
			free(simplified);
			// advance to the next stroke
			id++;
			from = i+1;
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
	char* p = zn->candidates;
	for (int i = 0; i < zinnia_result_size(zn->result); ++i)
		p = stpcpy(p, zinnia_result_value(zn->result, i));
	return zn->candidates;
}

TabletEngine engZinnia = {
	ZinniaCreate,
	ZinniaProcess,
	ZinniaGetCandidates,
	ZinniaDestroy
};
