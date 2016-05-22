/*
 * fire.c - draw fire animation
 */

#include <signal.h>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define USEC_IN_MSEC 1000

static volatile bool do_break = false;

static void sig_handler(int sig)
{
	do_break = sig == SIGINT;
}

#define LOWER_COLOR  3
#define MIDDLE_COLOR 2
#define UPPER_COLOR  1

static int max_x, max_y;

static void delay(void)
{
	usleep(USEC_IN_MSEC * 150);
}

static void draw_flame(void)
{
	int x;

	for (x = 0; x < max_x; x++) {
		int y0 = random() % (max_y / 2);
		int color;
		int y;

		for (y = 0; y <= max_y; y++) {
			if (y < y0) {
				mvaddch(y, x, ' ');
				continue;
			}

			for (color = UPPER_COLOR; color <= LOWER_COLOR; color++) {
				if (y <= y0 + ((max_y - y0) / 3) * color)
					break;
			}

			attron(COLOR_PAIR(color) | A_BOLD);
			mvaddch(y, x, 'X');
			attroff(COLOR_PAIR(color) | A_BOLD);
		}
	}
}

int main(int argc, char **argv)
{
	initscr();
	noecho();
	curs_set(false);

	start_color();
	init_pair(LOWER_COLOR + 1, COLOR_WHITE, COLOR_BLACK);
	init_pair(LOWER_COLOR, COLOR_WHITE, COLOR_BLACK);
	init_pair(MIDDLE_COLOR, COLOR_YELLOW, COLOR_BLACK);
	init_pair(UPPER_COLOR, COLOR_RED, COLOR_BLACK);

	getmaxyx(stdscr, max_y, max_x);

	while (!do_break) {
		draw_flame();
		refresh();
		delay();
	}

	endwin();
	return 0;
}
