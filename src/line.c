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
#include "line.h"
#include <math.h>
#include <stdlib.h>

// This algorithm operates on the line of points from input[start] to input[end]
// e is epsilon, the tolerance. The indicies of saved points (distance from line > e) are
// stored in the output array.
static void DouglasPeucker(const pt_t* input, const int start, const int end, const int e, int* output, int*nout) {
	float dmax = 0; // the maximum distance away from the given line
	int idx = 0; // the index of the most-distant point
	// form line equation y = mx + c
	float m = (float) ((float)input[end].y - input[start].y) / (float) (input[end].x - input[start].x);
	float c = input[start].y;
	// Loop over line, finding maximum perpendicular distance from line
	for(int i=start+1; i<end; ++i) {
		float d = fabs(m*input[i].x - input[i].y + c) / sqrt(pow(c,2)+1);
		if(d > dmax) {
			idx = i;
			dmax = d;
		}
	}
	// If we have a maximum point, bisect the line and recurse
	if(dmax > e) {
		// Save the index of this point
		output[(*nout)++] = idx;
		DouglasPeucker(input, start, idx, e, output, nout);
		DouglasPeucker(input, idx, end, e, output, nout);
	}
}

// Small comparison function for integers, for use in quicksort
static int intComparator(const void* _a, const void*_b) {
	const int *a = _a;
	const int *b = _b;
	if(*a == *b) return 0;
	if(*a > *b) return 1;
	return -1;
}

int SimplifyLine(pt_t* input, int nInput, pt_t** output, int epsilon) {
	if(epsilon < 0) return -1; // A negative epsilon is unstable
	// Create memory to hold the maximum number of output points,
	// which is the same as the number of input points.
	int* retainedIndicies = (int*) malloc(nInput * sizeof(int));
	int nRetainedPoints = 0;
	// Run the recursive algorithm. The algorithm requires the first and
	// last point are retained, but does not store them in retainedIndicies.
	DouglasPeucker(input, 0, nInput-1, epsilon, retainedIndicies, &nRetainedPoints);
	// The results will be in an indeterminate order. Sort them here
	// so we don't have to search through the array of saved indices
	// when generating the output array.
	qsort(retainedIndicies, nRetainedPoints, sizeof(int), &intComparator);
	// Create the output array. Plus two for the first and last
	// points in the input array (not included in nRetainedPoints)
	pt_t* o = (pt_t*) malloc(sizeof(pt_t) * (nRetainedPoints+2));
	*output = o; // Set the output pointer
	*o++ = input[0]; // First store the first point
	// incrementing the pointer means we don't have to use offset
	// arithmetic in this for loop
	for(int i = 0; i < nRetainedPoints; ++i)
		o[i] = input[retainedIndicies[i]];
	// Also save the last point of the input array
	o[nRetainedPoints] = input[nInput-1];
	// Clean up the original array of indices
	free(retainedIndicies);
	// Return the number of points in the output array
	return nRetainedPoints + 2;
}
