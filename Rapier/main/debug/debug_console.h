#ifndef SPACE_DEBUG_CONSOLE_H
#define SPACE_DEBUG_CONSOLE_H

#include <stdarg.h>
#include <stddef.h>

#define SDC_MAX_LINE  256
#define SDC_MAX_PLOTS 128

typedef enum { SDC_DEBUG=0, SDC_INFO, SDC_WARN, SDC_ERROR } SdcLevel;

typedef struct {
    SdcLevel level;
    char     text[SDC_MAX_LINE];
} SdcEntry;

typedef struct {
    SdcEntry        *buf;
    int              cap, head, count;
    int              show[4];
    char             filter[64];
    int              autoscroll;
    struct nk_scroll scroll;
    unsigned long long total, seen;   /* entries logged / entries already scrolled to */

    double           sim_time, fps;
    unsigned int     body_count;
    unsigned long long tick;

    float            plot_fps[SDC_MAX_PLOTS];
    int              plot_fps_n, plot_fps_head;

    int              visible, paused, step;

    char             input[128];
    int              focus;
} SDC;

#ifdef __cplusplus
extern "C" {
#endif

void sdc_init   (SDC *c, int max_lines);
void sdc_free   (SDC *c);
void sdc_log    (SDC *c, SdcLevel lvl, const char *fmt, ...);
void sdc_clear  (SDC *c);
void sdc_tick   (SDC *c, double sim_time, double fps, unsigned int bodies, unsigned long long tick);
void sdc_draw   (SDC *c, struct nk_context *nk, float x, float y, float w, float h);
void sdc_toggle (SDC *c);
int  sdc_paused (SDC *c);
int  sdc_step   (SDC *c);

#ifdef __cplusplus
}
#endif

#ifdef SPACE_DEBUG_CONSOLE_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Commands:
 *   help                      list commands
 *   clear                     clear the log
 *   pause | resume | step     control the simulation
 *   stats                     print t / tick / fps / bodies
 *   show <dbg|inf|wrn|err> <on|off>
 */

static struct nk_color sdc__col[] = {
    {130,130,130,255}, {215,215,215,255}, {255,210,60,255}, {255,90,90,255}
};

void sdc_init(SDC *c, int n) {
    memset(c, 0, sizeof(*c));
    c->buf = (SdcEntry*)calloc(n, sizeof(SdcEntry));
    c->cap = n;
    c->show[0]=c->show[1]=c->show[2]=c->show[3]=1;
    c->autoscroll = 1;
    c->visible = 1;
    c->focus = 1;
}

void sdc_free(SDC *c) { free(c->buf); c->buf=NULL; }

void sdc_log(SDC *c, SdcLevel lvl, const char *fmt, ...) {
    va_list a;
    SdcEntry *e = &c->buf[c->head];
    e->level = lvl;
    va_start(a, fmt); vsnprintf(e->text, SDC_MAX_LINE, fmt, a); va_end(a);
    c->head = (c->head+1) % c->cap;
    if (c->count < c->cap) c->count++;
    c->total++;
}

void sdc_clear(SDC *c) {
    c->head = c->count = 0;
    memset(c->buf, 0, c->cap*sizeof(SdcEntry));
}

void sdc_tick(SDC *c, double sim_time, double fps, unsigned int bodies, unsigned long long tick) {
    c->sim_time   = sim_time;
    c->fps        = fps;
    c->body_count = bodies;
    c->tick       = tick;
    c->plot_fps[c->plot_fps_head] = (float)fps;
    c->plot_fps_head = (c->plot_fps_head+1) % SDC_MAX_PLOTS;
    if (c->plot_fps_n < SDC_MAX_PLOTS) c->plot_fps_n++;
}

void sdc_toggle(SDC *c) { c->visible ^= 1; if (c->visible) c->focus = 1; }
int  sdc_paused(SDC *c) { return c->paused; }
int  sdc_step  (SDC *c) { if(c->step){c->step=0;return 1;} return 0; }

static int sdc__on(const char *s) { return s && (!strcmp(s,"on") || !strcmp(s,"1")); }

static void sdc__exec(SDC *c, char *line) {
    char *cmd, *a1, *a2;
    sdc_log(c, SDC_INFO, "> %s", line);

    cmd = strtok(line, " \t");
    if (!cmd) return;
    a1 = strtok(NULL, " \t");
    a2 = strtok(NULL, " \t");

    if (!strcmp(cmd, "help")) {
        sdc_log(c, SDC_INFO, "help, hide (or press `), clear, pause, resume, step, stats");
        sdc_log(c, SDC_INFO, "filter [text], show <dbg|inf|wrn|err> <on|off>, autoscroll <on|off>");
    } else if (!strcmp(cmd, "hide")) {
        c->visible = 0;
    } else if (!strcmp(cmd, "clear")) {
        sdc_clear(c);
    } else if (!strcmp(cmd, "pause")) {
        c->paused = 1;
    } else if (!strcmp(cmd, "resume")) {
        c->paused = 0;
    } else if (!strcmp(cmd, "step")) {
        if (c->paused) c->step = 1; else sdc_log(c, SDC_WARN, "step: pause first");
    } else if (!strcmp(cmd, "stats")) {
        sdc_log(c, SDC_INFO, "t=%.2fs tick=%llu fps=%.0f bodies=%u %s",
                c->sim_time, c->tick, c->fps, c->body_count, c->paused ? "PAUSED" : "RUN");
    } else if (!strcmp(cmd, "show")) {
        int lvl = -1;
        if (a1 && !strcmp(a1,"dbg")) lvl = 0;
        else if (a1 && !strcmp(a1,"inf")) lvl = 1;
        else if (a1 && !strcmp(a1,"wrn")) lvl = 2;
        else if (a1 && !strcmp(a1,"err")) lvl = 3;
        if (lvl < 0 || !a2) sdc_log(c, SDC_WARN, "usage: show <dbg|inf|wrn|err> <on|off>");
        else c->show[lvl] = sdc__on(a2);
    } else {
        sdc_log(c, SDC_ERROR, "unknown command: %s", cmd);
    }
}

// keep the toggle key out of the input field 
static int sdc__filter(const struct nk_text_edit *box, nk_rune r) {
    if (r == '`' || r == '~') return 0;
    return nk_filter_default(box, r);
}

void sdc_draw(SDC *c, struct nk_context *nk, float x, float y, float w, float h) {
    char tmp[SDC_MAX_LINE + 8];
    nk_flags ev;
    float log_h;
    int i;

    if (!c->visible) return;

    if (!nk_begin(nk, "##sdc",
            nk_rect(x,y,w,h),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_NO_SCROLLBAR))
    { nk_end(nk); return; }

    // log
    log_h = nk_window_get_content_region_size(nk).y - 38.0f;
    if (log_h < 30.0f) log_h = 30.0f;

    nk_layout_row_dynamic(nk, log_h, 1);
    if (nk_group_scrolled_begin(nk, &c->scroll, "##log", NK_WINDOW_BORDER)) {
        int start = (c->count < c->cap) ? 0 : c->head;
        nk_layout_row_dynamic(nk, 15, 1);
        for (i = 0; i < c->count; i++) {
            SdcEntry *e = &c->buf[(start+i) % c->cap];
            if (!c->show[e->level]) continue;
            if (c->filter[0] && !strstr(e->text, c->filter)) continue;
            if (e->level == SDC_WARN)       snprintf(tmp, sizeof(tmp), "[WARN] %s", e->text);
            else if (e->level == SDC_ERROR) snprintf(tmp, sizeof(tmp), "[ERR] %s", e->text);
            else                            snprintf(tmp, sizeof(tmp), "%s", e->text);
            nk_text_colored(nk, tmp, (int)strlen(tmp), NK_TEXT_LEFT, sdc__col[e->level]);
        }
        if (c->autoscroll && c->seen != c->total) c->scroll.y = 1000000;
        c->seen = c->total;
        nk_group_scrolled_end(nk);
    }

    //input
    nk_layout_row_dynamic(nk, 26, 1);
    if (c->focus) { nk_edit_focus(nk, 0); c->focus = 0; }
    ev = nk_edit_string_zero_terminated(nk, NK_EDIT_FIELD|NK_EDIT_SIG_ENTER,
                                        c->input, sizeof(c->input), sdc__filter);
    if ((ev & NK_EDIT_COMMITED) && c->input[0]) {
        char line[sizeof(c->input)];
        memcpy(line, c->input, sizeof(line));
        c->input[0] = 0;
        c->focus = 1;
        sdc__exec(c, line);
    }

    nk_end(nk);
}

#endif
#endif