/*
 * File: pathfind.c
 * Purpose: Pathfinding algorithm
 *
 * Copyright (c) 2004 Christophe Cavalaria, Leon Marrick
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */
#include "angband.h"


/*** Constants ***/

/* Maximum size around the player to consider in the pathfinder */
#define MAX_PF_RADIUS 50

/* Maximum distance to consider in the pathfinder */
#define MAX_PF_LENGTH 250


/*** Globals ***/

static int terrain[MAX_PF_RADIUS][MAX_PF_RADIUS];
char pf_result[MAX_PF_LENGTH];
int pf_result_index;

static int ox, oy, ex, ey;


/*** Pathfinding code ***/

static bool is_valid_pf(int y, int x)
{
	/* Unvisited means allowed */
	if (!(cave_info[y][x] & (CAVE_MARK))) return (TRUE);

	/* Require open space */
	return (cave_floor_bold(y, x));
}

static void fill_terrain_info(void)
{
	int i, j;

	ox = MAX(p_ptr->px - MAX_PF_RADIUS / 2, 0);
	oy = MAX(p_ptr->py - MAX_PF_RADIUS / 2, 0);

	ex = MIN(p_ptr->px + MAX_PF_RADIUS / 2 - 1, DUNGEON_WID);
	ey = MIN(p_ptr->py + MAX_PF_RADIUS / 2 - 1, DUNGEON_HGT);

	for (i = 0; i < MAX_PF_RADIUS * MAX_PF_RADIUS; i++)
		terrain[0][i] = -1;

	for (j = oy; j < ey; j++)
		for (i = ox; i < ex; i++)
			if (is_valid_pf(j, i))
				terrain[j - oy][i - ox] = MAX_PF_LENGTH;

	terrain[p_ptr->py - oy][p_ptr->px - ox] = 1;
}

#define MARK_DISTANCE(c,d) if ((c <= MAX_PF_LENGTH) && (c > d)) { c = d; try_again = (TRUE); }

bool findpath(int y, int x)
{
	int i, j, dir;
	bool try_again;
	int cur_distance;

	fill_terrain_info();

	terrain[p_ptr->py - oy][p_ptr->px - ox] = 1;

	if ((x >= ox) && (x < ex) && (y >= oy) && (y < ey))
	{
		if ((cave_m_idx[y][x] > 0) && (mon_list[cave_m_idx[y][x]].ml))
		{
			terrain[y - oy][x - ox] = MAX_PF_LENGTH;
		}

#if 0
		else if (terrain[y-oy][x-ox] != MAX_PF_LENGTH)
		{
		   bell("Target blocked");
		   return (FALSE);
		}
#endif

		terrain[y - oy][x - ox] = MAX_PF_LENGTH;
	}
	else
	{
		bell("Target out of range.");
		return (FALSE);
	}

	if (terrain[y - oy][x - ox] == -1)
	{
		bell("Target space forbidden");
		return (FALSE);
	}


	/* 
	 * And now starts the very naive and very 
	 * inefficient pathfinding algorithm
	 */
	do
	{
		try_again = FALSE;

		for (j = oy + 1; j < ey - 1; j++)
		{
			for (i = ox + 1; i < ex - 1; i++)
			{
				cur_distance = terrain[j - oy][i - ox] + 1;

				if ((cur_distance > 0) && (cur_distance < MAX_PF_LENGTH))
				{
					for (dir = 1; dir < 10; dir++)
					{
						if (dir == 5)
							dir++;

						MARK_DISTANCE(terrain[j - oy + ddy[dir]][i - ox + ddx[dir]], cur_distance);
					}
				}
			}
		}

		if (terrain[y - oy][x - ox] < MAX_PF_LENGTH)
			try_again = (FALSE);

	}
	while (try_again);

	/* Failure */
	if (terrain[y - oy][x - ox] == MAX_PF_LENGTH)
	{
		bell("Target space unreachable.");
		return (FALSE);
	}

	/* Success */
	i = x;
	j = y;

	pf_result_index = 0;

	while ((i != p_ptr->px) || (j != p_ptr->py))
	{
		cur_distance = terrain[j - oy][i - ox] - 1;
		for (dir = 1; dir < 10; dir++)
		{
			if (terrain[j - oy + ddy[dir]][i - ox + ddx[dir]] == cur_distance)
				break;
		}

		/* Should never happend */
		if (dir == 10)
		{
			bell("Wtf ?");
			return (FALSE);
		}

		else if (dir == 5)
		{
			bell("Heyyy !");
			return (FALSE);
		}

		pf_result[pf_result_index++] = '0' + (char)(10 - dir);
		i += ddx[dir];
		j += ddy[dir];
	}

	pf_result_index--;

	return (TRUE);
}



/*
 * Accept values for y and x (considered as the endpoints of lines) between
 * 0 and 40, and return an angle in degrees (divided by two).  -LM-
 *
 * This table's input and output need some processing:
 *
 * Because this table gives degrees for a whole circle, up to radius 20, its
 * origin is at (x,y) = (20, 20).  Therefore, the input code needs to find
 * the origin grid (where the lines being compared come from), and then map
 * it to table grid 20,20.  Do not, however, actually try to compare the
 * angle of a line that begins and ends at the origin with any other line -
 * it is impossible mathematically, and the table will return the value "255".
 *
 * The output of this table also needs to be massaged, in order to avoid the
 * discontinuity at 0/180 degrees.  This can be done by:
 *   rotate = 90 - first value
 *   this rotates the first input to the 90 degree line)
 *   tmp = ABS(second value + rotate) % 180
 *   diff = ABS(90 - tmp) = the angular difference (divided by two) between
 *   the first and second values.
 *
 * Note that grids diagonal to the origin have unique angles.
 */
byte get_angle_to_grid[41][41] =
{
  {  68,  67,  66,  65,  64,  63,  62,  62,  60,  59,  58,  57,  56,  55,  53,  52,  51,  49,  48,  46,  45,  44,  42,  41,  39,  38,  37,  35,  34,  33,  32,  31,  30,  28,  28,  27,  26,  25,  24,  24,  23 },
  {  69,  68,  67,  66,  65,  64,  63,  62,  61,  60,  59,  58,  56,  55,  54,  52,  51,  49,  48,  47,  45,  43,  42,  41,  39,  38,  36,  35,  34,  32,  31,  30,  29,  28,  27,  26,  25,  24,  24,  23,  22 },
  {  69,  69,  68,  67,  66,  65,  64,  63,  62,  61,  60,  58,  57,  56,  54,  53,  51,  50,  48,  47,  45,  43,  42,  40,  39,  37,  36,  34,  33,  32,  30,  29,  28,  27,  26,  25,  24,  24,  23,  22,  21 },
  {  70,  69,  69,  68,  67,  66,  65,  64,  63,  61,  60,  59,  58,  56,  55,  53,  52,  50,  48,  47,  45,  43,  42,  40,  38,  37,  35,  34,  32,  31,  30,  29,  27,  26,  25,  24,  24,  23,  22,  21,  20 },
  {  71,  70,  69,  69,  68,  67,  66,  65,  63,  62,  61,  60,  58,  57,  55,  54,  52,  50,  49,  47,  45,  43,  41,  40,  38,  36,  35,  33,  32,  30,  29,  28,  27,  25,  24,  24,  23,  22,  21,  20,  19 },
  {  72,  71,  70,  69,  69,  68,  67,  65,  64,  63,  62,  60,  59,  58,  56,  54,  52,  51,  49,  47,  45,  43,  41,  39,  38,  36,  34,  32,  31,  30,  28,  27,  26,  25,  24,  23,  22,  21,  20,  19,  18 },
  {  73,  72,  71,  70,  69,  69,  68,  66,  65,  64,  63,  61,  60,  58,  57,  55,  53,  51,  49,  47,  45,  43,  41,  39,  37,  35,  33,  32,  30,  29,  27,  26,  25,  24,  23,  22,  21,  20,  19,  18,  17 },
  {  73,  73,  72,  71,  70,  70,  69,  68,  66,  65,  64,  62,  61,  59,  57,  56,  54,  51,  49,  47,  45,  43,  41,  39,  36,  34,  33,  31,  29,  28,  26,  25,  24,  23,  21,  20,  20,  19,  18,  17,  17 },
  {  75,  74,  73,  72,  72,  71,  70,  69,  68,  66,  65,  63,  62,  60,  58,  56,  54,  52,  50,  47,  45,  43,  40,  38,  36,  34,  32,  30,  28,  27,  25,  24,  23,  21,  20,  19,  18,  18,  17,  16,  15 },
  {  76,  75,  74,  74,  73,  72,  71,  70,  69,  68,  66,  65,  63,  61,  59,  57,  55,  53,  50,  48,  45,  42,  40,  37,  35,  33,  31,  29,  27,  25,  24,  23,  21,  20,  19,  18,  17,  16,  16,  15,  14 },
  {  77,  76,  75,  75,  74,  73,  72,  71,  70,  69,  68,  66,  64,  62,  60,  58,  56,  53,  51,  48,  45,  42,  39,  37,  34,  32,  30,  28,  26,  24,  23,  21,  20,  19,  18,  17,  16,  15,  15,  14,  13 },
  {  78,  77,  77,  76,  75,  75,  74,  73,  72,  70,  69,  68,  66,  64,  62,  60,  57,  54,  51,  48,  45,  42,  39,  36,  33,  30,  28,  26,  24,  23,  21,  20,  18,  17,  16,  15,  15,  14,  13,  13,  12 },
  {  79,  79,  78,  77,  77,  76,  75,  74,  73,  72,  71,  69,  68,  66,  63,  61,  58,  55,  52,  49,  45,  41,  38,  35,  32,  29,  27,  24,  23,  21,  19,  18,  17,  16,  15,  14,  13,  13,  12,  11,  11 },
  {  80,  80,  79,  79,  78,  77,  77,  76,  75,  74,  73,  71,  69,  68,  65,  63,  60,  57,  53,  49,  45,  41,  37,  33,  30,  27,  25,  23,  21,  19,  17,  16,  15,  14,  13,  13,  12,  11,  11,  10,  10 },
  {  82,  81,  81,  80,  80,  79,  78,  78,  77,  76,  75,  73,  72,  70,  68,  65,  62,  58,  54,  50,  45,  40,  36,  32,  28,  25,  23,  20,  18,  17,  15,  14,  13,  12,  12,  11,  10,  10,   9,   9,   8 },
  {  83,  83,  82,  82,  81,  81,  80,  79,  79,  78,  77,  75,  74,  72,  70,  68,  64,  60,  56,  51,  45,  39,  34,  30,  26,  23,  20,  18,  16,  15,  13,  12,  11,  11,  10,   9,   9,   8,   8,   7,   7 },
  {  84,  84,  84,  83,  83,  83,  82,  81,  81,  80,  79,  78,  77,  75,  73,  71,  68,  63,  58,  52,  45,  38,  32,  27,  23,  19,  17,  15,  13,  12,  11,  10,   9,   9,   8,   7,   7,   7,   6,   6,   6 },
  {  86,  86,  85,  85,  85,  84,  84,  84,  83,  82,  82,  81,  80,  78,  77,  75,  72,  68,  62,  54,  45,  36,  28,  23,  18,  15,  13,  12,  10,   9,   8,   8,   7,   6,   6,   6,   5,   5,   5,   4,   4 },
  {  87,  87,  87,  87,  86,  86,  86,  86,  85,  85,  84,  84,  83,  82,  81,  79,  77,  73,  68,  58,  45,  32,  23,  17,  13,  11,   9,   8,   7,   6,   6,   5,   5,   4,   4,   4,   4,   3,   3,   3,   3 },
  {  89,  88,  88,  88,  88,  88,  88,  88,  88,  87,  87,  87,  86,  86,  85,  84,  83,  81,  77,  68,  45,  23,  13,   9,   7,   6,   5,   4,   4,   3,   3,   3,   2,   2,   2,   2,   2,   2,   2,   2,   1 },
  {  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  {  91,  92,  92,  92,  92,  92,  92,  92,  92,  93,  93,  93,  94,  94,  95,  96,  97,  99, 103, 113, 135, 158, 167, 171, 173, 174, 175, 176, 176, 177, 177, 177, 178, 178, 178, 178, 178, 178, 178, 178, 179 },
  {  93,  93,  93,  93,  94,  94,  94,  94,  95,  95,  96,  96,  97,  98,  99, 101, 103, 107, 113, 122, 135, 148, 158, 163, 167, 169, 171, 172, 173, 174, 174, 175, 175, 176, 176, 176, 176, 177, 177, 177, 177 },
  {  94,  94,  95,  95,  95,  96,  96,  96,  97,  98,  98,  99, 100, 102, 103, 105, 108, 113, 118, 126, 135, 144, 152, 158, 162, 165, 167, 168, 170, 171, 172, 172, 173, 174, 174, 174, 175, 175, 175, 176, 176 },
  {  96,  96,  96,  97,  97,  97,  98,  99,  99, 100, 101, 102, 103, 105, 107, 109, 113, 117, 122, 128, 135, 142, 148, 153, 158, 161, 163, 165, 167, 168, 169, 170, 171, 171, 172, 173, 173, 173, 174, 174, 174 },
  {  97,  97,  98,  98,  99,  99, 100, 101, 101, 102, 103, 105, 106, 108, 110, 113, 116, 120, 124, 129, 135, 141, 146, 150, 154, 158, 160, 162, 164, 165, 167, 168, 169, 169, 170, 171, 171, 172, 172, 173, 173 },
  {  98,  99,  99, 100, 100, 101, 102, 102, 103, 104, 105, 107, 108, 110, 113, 115, 118, 122, 126, 130, 135, 140, 144, 148, 152, 155, 158, 160, 162, 163, 165, 166, 167, 168, 168, 169, 170, 170, 171, 171, 172 },
  { 100, 100, 101, 101, 102, 103, 103, 104, 105, 106, 107, 109, 111, 113, 115, 117, 120, 123, 127, 131, 135, 139, 143, 147, 150, 153, 155, 158, 159, 161, 163, 164, 165, 166, 167, 167, 168, 169, 169, 170, 170 },
  { 101, 101, 102, 103, 103, 104, 105, 106, 107, 108, 109, 111, 113, 114, 117, 119, 122, 125, 128, 131, 135, 139, 142, 145, 148, 151, 153, 156, 158, 159, 161, 162, 163, 164, 165, 166, 167, 167, 168, 169, 169 },
  { 102, 103, 103, 104, 105, 105, 106, 107, 108, 110, 111, 113, 114, 116, 118, 120, 123, 126, 129, 132, 135, 138, 141, 144, 147, 150, 152, 154, 156, 158, 159, 160, 162, 163, 164, 165, 165, 166, 167, 167, 168 },
  { 103, 104, 105, 105, 106, 107, 108, 109, 110, 111, 113, 114, 116, 118, 120, 122, 124, 127, 129, 132, 135, 138, 141, 143, 146, 148, 150, 152, 154, 156, 158, 159, 160, 161, 162, 163, 164, 165, 165, 166, 167 },
  { 104, 105, 106, 106, 107, 108, 109, 110, 111, 113, 114, 115, 117, 119, 121, 123, 125, 127, 130, 132, 135, 138, 140, 143, 145, 147, 149, 151, 153, 155, 156, 158, 159, 160, 161, 162, 163, 164, 164, 165, 166 },
  { 105, 106, 107, 108, 108, 109, 110, 111, 113, 114, 115, 117, 118, 120, 122, 124, 126, 128, 130, 133, 135, 137, 140, 142, 144, 146, 148, 150, 152, 153, 155, 156, 158, 159, 160, 161, 162, 162, 163, 164, 165 },
  { 107, 107, 108, 109, 110, 110, 111, 113, 114, 115, 116, 118, 119, 121, 123, 124, 126, 129, 131, 133, 135, 137, 139, 141, 144, 146, 147, 149, 151, 152, 154, 155, 156, 158, 159, 160, 160, 161, 162, 163, 163 },
  { 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 119, 120, 122, 123, 125, 127, 129, 131, 133, 135, 137, 139, 141, 143, 145, 147, 148, 150, 151, 153, 154, 155, 156, 158, 159, 159, 160, 161, 162, 163 },
  { 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 120, 121, 122, 124, 126, 128, 129, 131, 133, 135, 137, 139, 141, 142, 144, 146, 148, 149, 150, 152, 153, 154, 155, 157, 158, 159, 159, 160, 161, 162 },
  { 109, 110, 111, 112, 113, 114, 114, 115, 117, 118, 119, 120, 122, 123, 125, 126, 128, 130, 131, 133, 135, 137, 139, 140, 142, 144, 145, 147, 148, 150, 151, 152, 153, 155, 156, 157, 158, 159, 159, 160, 161 },
  { 110, 111, 112, 113, 114, 114, 115, 116, 117, 119, 120, 121, 122, 124, 125, 127, 128, 130, 132, 133, 135, 137, 138, 140, 142, 143, 145, 146, 148, 149, 150, 151, 153, 154, 155, 156, 157, 158, 159, 159, 160 },
  { 111, 112, 113, 114, 114, 115, 116, 117, 118, 119, 120, 122, 123, 124, 126, 127, 129, 130, 132, 133, 135, 137, 138, 140, 141, 143, 144, 146, 147, 148, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 159 },
  { 112, 113, 114, 114, 115, 116, 117, 118, 119, 120, 121, 122, 124, 125, 126, 128, 129, 131, 132, 133, 135, 137, 138, 139, 141, 142, 144, 145, 146, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159 },
  { 113, 114, 114, 115, 116, 117, 118, 118, 120, 121, 122, 123, 124, 125, 127, 128, 129, 131, 132, 134, 135, 136, 138, 139, 141, 142, 143, 145, 146, 147, 148, 149, 150, 152, 152, 153, 154, 155, 156, 157, 158 }
};



/*
 * Calculates and returns the angle to the target or in the given
 * direction.
 *
 * Note:  If a compass direction is supplied, we ignore any target.
 * Note:  We supply the angle divided by 2.
 */
int get_angle_to_target(int y0, int x0, int y1, int x1, int dir)
{
	int ny, nx;
	int dist_conv;

	/* No valid compass direction given */
	if ((dir == 0) || (dir == 5) || (dir > 9))
	{
		/* Check for a valid target */
		if ((y1) && (x1))
		{
			/* Get absolute distance between source and target */
			int dy = ABS(y1 - y0);
			int dx = ABS(x1 - x0);

			/* Calculate distance conversion factor */
			if ((dy > 20) || (dx > 20))
			{
				/* Must shrink the distance to avoid illegal table access */
				if (dy > dx) dist_conv = 1 + (10 * dy / 20);
				else         dist_conv = 1 + (10 * dx / 20);
			}
			else
			{
				dist_conv = 10;
			}
			/* Convert and reorient grid for table access */
			ny = 20 + 10 * (y1 - y0) / dist_conv;
			nx = 20 + 10 * (x1 - x0) / dist_conv;

			/* Illegal table access is bad */
			if ((ny < 0) || (ny > 40) || (nx < 0) || (nx > 40))
			{
				/* Note error */
				return (-1);
			}
		}

		/* No compass direction and no target --> note error */
		else
		{
			return (-1);
		}
	}

	/* We have a valid compass direction */
	else
	{
		/* Step in that direction a bunch of times, get target */
		y1 = y0 + (ddy_ddd[dir] * 10);
		x1 = x0 + (ddx_ddd[dir] * 10);

		/* Convert to table grids */
		ny = 20 + (y1 - y0);
		nx = 20 + (x1 - x0);
	}

	/* Get angle to target. */
	return (get_angle_to_grid[ny][nx]);
}

/*
 * Using the angle given, find a grid that is in that direction from the
 * origin.
 *
 * Note:  This function does not yield very good results when the
 * character is adjacent to the outer wall of the dungeon and the projection
 * heads towards it.
 */
void get_grid_using_angle(int angle, int y0, int x0, int *ty, int *tx)
{
	int y, x;
	int best_y = 0, best_x = 0;

	int diff;
	int this_angle;
	int fudge = 180;


	/* Angle must be legal */
	if ((angle < 0) || (angle >= 180)) return;

	/* Scan the table, get as good a match as possible */
	for (y = 0; y < 41; y++)
	{
		for (x = 0; x < 41; x++)
		{
			/* Corresponding grid in dungeon must be fully in bounds  XXX */
			if (!in_bounds_fully(y0 - 20 + y, x0 - 20 + x)) continue;

			/* Check this table grid */
			this_angle = get_angle_to_grid[y][x];

			/* Get inaccuracy of this angle */
			diff = ABS(angle - this_angle);

			/* Inaccuracy is lower than previous best */
			if (diff < fudge)
			{
				/* Note coordinates */
				best_y = y;
				best_x = x;

				/* Save inaccuracy as a new best */
				fudge = diff;

				/* Note perfection */
				if (fudge == 0) break;
			}
		}

		/* Note perfection */
		if (fudge == 0) break;
	}

	/* We have an unacceptably large fudge factor */
	if (fudge >= 30)
	{
		/* Set target to original grid */
		*ty = y0;
		*tx = x0;
	}

	/* Usual case */
	else
	{
		/* Set target */
		*ty = y0 - 20 + best_y;
		*tx = x0 - 20 + best_x;
	}
}

