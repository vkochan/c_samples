/*
 * matrix.c - draw matrix codes rain effect
 */

#include <ctype.h>
#include <signal.h>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define USEC_IN_MSEC 1000

typedef struct mx_flow {
	int y, x;
	size_t len;
} mx_flow_t;

static volatile bool do_break = false;

static void sig_handler(int sig)
{
	do_break = sig == SIGINT;
}

#define CODE_COLOR 1

static size_t flows_count;
static mx_flow_t *flows;

static int max_x, max_y;

static void delay(void)
{
	usleep(USEC_IN_MSEC * 150);
}

static void flow_init(mx_flow_t *f, int x)
{
	f->x = x;
	f->y = 0;
	f->len = 0;
}

static void code_draw(int y, int x, int color)
{
	attron(color);
	mvaddch(y, x, 33 + (random() % 98));
	attroff(color);
}

static void flow_draw(mx_flow_t *f)
{
	int y;

	if (f->y + f->len > max_y)
		f->len--;
	else if (f->len < max_y)
		f->len++;

	if (f->y > max_y)
		flow_init(f, f->x);

	for (y = f->y; y < (f->y + f->len); y++) {
		int color;

		if (y == f->y + f->len - 1)
			color = A_BOLD;
		else
			color = COLOR_PAIR(CODE_COLOR);

		code_draw(y, f->x, color);
	}

	if (f->len == max_y || f->y + f->len > max_y)
		f->y++;
}

static void init_flows(void)
{
	int i;

	flows_count = max_x;
	flows = realloc(flows, sizeof(mx_flow_t) * flows_count);
        
	for (i = 0; i < flows_count; i++)
		flow_init(&flows[i], i);
}

int main(int argc, char **argv)
{
	initscr();
	noecho();
	curs_set(false);

	start_color();
	init_pair(CODE_COLOR, COLOR_GREEN, COLOR_BLACK);

	getmaxyx(stdscr, max_y, max_x);

	init_flows();

	while (!do_break) {
		int i;

		for (i = 0; i < flows_count; i++) {
			mx_flow_t *f = &flows[i];

			if (f->len)
				flow_draw(f);
		}

		flow_draw(&flows[random() % flows_count]);
		refresh();
		delay();
		clear();
	}

	free(flows);
	endwin();
	return 0;
}
