#ifndef POINT_H
#define POINT_H

typedef struct {
	short x;
	short y;
} pt_t;

#define PT_INVALID { -1, -1 }
#define PT_ISVALID(p) (p.x != -1 && p.y != -1)

#endif // POINT_H
