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
    nk_uint          scroll;

    double           sim_time, fps;
    unsigned int     body_count;
    unsigned long long tick;

    float            plot_fps[SDC_MAX_PLOTS];
    int              plot_fps_n, plot_fps_head;

    int              visible, paused, step;
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

static const char *sdc__pfx[] = {"[DBG]","[INF]","[WRN]","[ERR]"};
static struct nk_color sdc__col[] = {
    {130,130,130,255}, {180,210,255,255}, {255,210,60,255}, {255,70,70,255}
};

void sdc_init(SDC *c, int n) {
    memset(c, 0, sizeof(*c));
    c->buf = (SdcEntry*)calloc(n, sizeof(SdcEntry));
    c->cap = n;
    c->show[0]=c->show[1]=c->show[2]=c->show[3]=1;
    c->autoscroll = 1;
    c->visible = 1;
}

void sdc_free(SDC *c) { free(c->buf); c->buf=NULL; }

void sdc_log(SDC *c, SdcLevel lvl, const char *fmt, ...) {
    va_list a;
    SdcEntry *e = &c->buf[c->head];
    e->level = lvl;
    va_start(a, fmt); vsnprintf(e->text, SDC_MAX_LINE, fmt, a); va_end(a);
    c->head = (c->head+1) % c->cap;
    if (c->count < c->cap) c->count++;
}

void sdc_clear(SDC *c) { c->head=c->count=0; memset(c->buf,0,c->cap*sizeof(SdcEntry)); }

void sdc_tick(SDC *c, double sim_time, double fps, unsigned int bodies, unsigned long long tick) {
    c->sim_time   = sim_time;
    c->fps        = fps;
    c->body_count = bodies;
    c->tick       = tick;
    c->plot_fps[c->plot_fps_head] = (float)fps;
    c->plot_fps_head = (c->plot_fps_head+1) % SDC_MAX_PLOTS;
    if (c->plot_fps_n < SDC_MAX_PLOTS) c->plot_fps_n++;
}

void sdc_toggle(SDC *c) { c->visible ^= 1; }
int  sdc_paused(SDC *c) { return c->paused; }
int  sdc_step  (SDC *c) { if(c->step){c->step=0;return 1;} return 0; }

void sdc_draw(SDC *c, struct nk_context *nk, float x, float y, float w, float h) {
    char tmp[128];
    int i;

    if (!c->visible) return;

    if (!nk_begin(nk, "##sdc",
            nk_rect(x,y,w,h),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_NO_SCROLLBAR))
    { nk_end(nk); return; }

    /* ---- status bar ---- */
    nk_layout_row_dynamic(nk, 16, 1);
    snprintf(tmp, sizeof(tmp),
        "t=%.2fs  tick=%llu  fps=%.0f  bodies=%u  [%s]",
        c->sim_time, c->tick, c->fps, c->body_count,
        c->paused ? "PAUSED" : "RUN");
    nk_text_colored(nk, tmp, (int)strlen(tmp), NK_TEXT_LEFT, nk_rgb(100,180,100));

    /* ---- controls ---- */
    nk_layout_row_begin(nk, NK_STATIC, 20, 8);
    nk_layout_row_push(nk,52);
    if (nk_button_label(nk, c->paused?"Resume":"Pause")) c->paused^=1;
    nk_layout_row_push(nk,40);
    if (c->paused && nk_button_label(nk,"Step")) c->step=1;
    nk_layout_row_push(nk,40);
    if (nk_button_label(nk,"Clear")) sdc_clear(c);
    nk_layout_row_push(nk,38); c->show[0]=nk_check_label(nk,"DBG",c->show[0]);
    nk_layout_row_push(nk,34); c->show[1]=nk_check_label(nk,"INF",c->show[1]);
    nk_layout_row_push(nk,38); c->show[2]=nk_check_label(nk,"WRN",c->show[2]);
    nk_layout_row_push(nk,38); c->show[3]=nk_check_label(nk,"ERR",c->show[3]);
    nk_layout_row_push(nk,20); c->autoscroll=nk_check_label(nk,"",c->autoscroll);
    nk_layout_row_end(nk);

    /* ---- filter ---- */
    nk_layout_row_dynamic(nk, 18, 1);
    nk_edit_string_zero_terminated(nk, NK_EDIT_FIELD, c->filter, sizeof(c->filter), nk_filter_default);

    /* ---- log scroll group ---- */
    nk_layout_row_dynamic(nk, h - 90.0f, 1);
    if (nk_group_scrolled_begin(nk, (struct nk_scroll*)&c->scroll, "##log",
            NK_WINDOW_BORDER)) {
        int start = (c->count < c->cap) ? 0 : c->head;
        for (i = 0; i < c->count; i++) {
            int idx = (start+i) % c->cap;
            SdcEntry *e = &c->buf[idx];
            if (!c->show[e->level]) continue;
            if (c->filter[0] && !strstr(e->text, c->filter)) continue;
            snprintf(tmp, sizeof(tmp), "%s %s", sdc__pfx[e->level], e->text);
            nk_layout_row_dynamic(nk, 15, 1);
            nk_text_colored(nk, tmp, (int)strlen(tmp), NK_TEXT_LEFT, sdc__col[e->level]);
        }
        nk_group_scrolled_end(nk);
    }

    nk_end(nk);
}

#endif
#endif