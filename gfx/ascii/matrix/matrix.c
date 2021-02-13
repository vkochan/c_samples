/*
 * matrix.c - draw matrix codes rain effect
 */

#include <signal.h>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BANNER	"[THE MATRIX]"

#define USEC_IN_MSEC 1000

typedef struct mx_flow {
	int y;
	size_t len;
} mx_flow_t;

static volatile bool do_break = false;

static void sig_handler(int sig)
{
	do_break = sig == SIGINT;
}

#define CODE_COLOR 1

static mx_flow_t *flows;

static int max_x, max_y;

static void delay(void)
{
	usleep(USEC_IN_MSEC * 150);
}

static void code_draw(int y, int x, int color)
{
	attron(color);
	mvaddch(y, x, 33 + (random() % 60));
	attroff(color);
}

static void flow_draw(int x)
{
	mx_flow_t *f = &flows[x];
	int y;

	if (f->y + f->len > max_y)
		f->len--;
	else if (f->len < max_y)
		f->len++;

	if (f->y > max_y)
		memset(f, 0, sizeof(*f));

	for (y = 0; y < (f->y + f->len); y++) {
		int color = COLOR_PAIR(CODE_COLOR);

		if (y == f->y + f->len - 1)
			color = A_BOLD;
		else if (f->y + f->len - y < max_y / 4 && f->y + f->len < max_y)
			color |= A_BOLD;

		if (y < f->y)
			mvaddch(y, x, ' ');
		else
			code_draw(y, x, color);
	}

	if (f->len == max_y || f->y + f->len > max_y)
		f->y++;
}

static void init_flows(void)
{
	flows = realloc(flows, sizeof(mx_flow_t) * max_x);
	memset(flows, 0, sizeof(mx_flow_t) * max_x);
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

	attron(COLOR_PAIR(CODE_COLOR) | A_BOLD);
	mvaddstr(max_y / 2, max_x / 2 - strlen(BANNER) / 2, BANNER);
	attroff(COLOR_PAIR(CODE_COLOR) | A_BOLD);
	refresh();
	sleep(2);

	while (!do_break) {
		int x;

		for (x = 0; x < max_x; x++) {
			mx_flow_t *f = &flows[x];

			if (f->len)
				flow_draw(x);
		}

		flow_draw(random() % max_x);
		refresh();
		delay();
	}

	free(flows);
	endwin();
	return 0;
}
