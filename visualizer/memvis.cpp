#include "memvis.h"
#include <ncurses.h>
#include <locale.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

// ─── helpers ─────────────────────────────────────────────────
// ─── helpers ──────────────────────────────────────────────────

static void fmt_size(char* buf, size_t n, size_t bytes) {
    if (bytes >= 1024ULL * 1024 * 1024)
        snprintf(buf, n, "%.1fGB", bytes / (1024.0 * 1024 * 1024));
    else if (bytes >= 1024 * 1024)
        snprintf(buf, n, "%zuMB", bytes / (1024 * 1024));
    else if (bytes >= 1024)
        snprintf(buf, n, "%zuKB", bytes / 1024);
    else
        snprintf(buf, n, "%zuB", bytes);
}

static void draw_bar(WINDOW* w, int y, int x, int width, int pct) {
    int filled = (pct * width + 50) / 100;
    if (filled > width) filled = width;
    if (filled < 0)     filled = 0;

    char buf[256];
    int pos = 0;
    for (int i = 0; i < width; i++) {
        int rem = (int)sizeof(buf) - pos - 4;
        if (rem < 0) break;
        if (i < filled) {
            buf[pos++] = (char)0xE2; buf[pos++] = (char)0x96; buf[pos++] = (char)0x88; // █
        } else {
            buf[pos++] = (char)0xE2; buf[pos++] = (char)0x96; buf[pos++] = (char)0x91; // ░
        }
    }
    buf[pos] = '\0';
    mvwaddstr(w, y, x, buf);
}

// ─── shared memory reader ────────────────────────────────────
// ─── leitor de memoria compartilhada ─────────────────────────

int memvis_read_shm(int pid, BlockInfo* blocks, int max_blocks) {
    char path[256];
    snprintf(path, sizeof(path), "/tmp/meta-c-mem-%d.bin", pid);

    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;

    MetaCShmHeader header;
    ssize_t n = read(fd, &header, sizeof(header));
    if (n != (ssize_t)sizeof(header) || header.magic != META_C_SHM_MAGIC) {
        close(fd);
        return 0;
    }

    int count = (int)header.block_count;
    if (count > max_blocks) count = max_blocks;

    n = read(fd, blocks, (size_t)count * sizeof(BlockInfo));
    close(fd);
    return (n > 0) ? count : 0;
}

// ─── shared TUI render logic ─────────────────────────────────
// ─── logica de renderizacao TUI compartilhada ────────────────

enum { COL_NAME = 0, COL_SIZE = 14, COL_BAR = 24, BAR_W = 15,
       COL_PCT = COL_BAR + BAR_W + 2, COL_USED = COL_PCT + 6 };

static void render_title(int max_x, MemVisConfig cfg, const char* suffix) {
    attron(A_BOLD);
    mvprintw(0, 0, "  META-C Memory Visualizer%s", suffix ? suffix : "");
    attroff(A_BOLD);
    mvprintw(0, max_x - 22, "refresh: %dms", cfg.refresh_ms);
}

static void render_header(void) {
    attron(A_UNDERLINE);
    mvprintw(2, COL_NAME, "%-12s %8s  %s  %5s %8s",
             "Block", "Size", "Usage", "%", "Used");
    attroff(A_UNDERLINE);
}

static void render_blocks(BlockInfo* blocks, int count,
                          int selection, int max_y) {
    for (int i = 0; i < count && i < max_y - 6; i++) {
        int y = 3 + i;
        float pct = blocks[i].capacity > 0
            ? (float)blocks[i].used / blocks[i].capacity * 100.0f : 0.0f;

        if (i == selection) attron(A_REVERSE);

        if (pct > 80.0f && has_colors())
            attron(COLOR_PAIR(1));
        else if (pct > 60.0f && has_colors())
            attron(COLOR_PAIR(3));

        char sz[16], us[16];
        fmt_size(sz, sizeof(sz), blocks[i].capacity);
        fmt_size(us, sizeof(us), blocks[i].used);

        mvprintw(y, COL_NAME, "%-12s %8s ", blocks[i].name, sz);
        draw_bar(stdscr, y, COL_BAR, BAR_W, (int)pct);

        if (has_colors()) { attroff(COLOR_PAIR(1)); attroff(COLOR_PAIR(3)); }

        mvprintw(y, COL_PCT, "%3.0f%% ", pct);
        mvprintw(y, COL_USED, "%8s", us);

        if (pct > 80.0f) {
            if (has_colors()) attron(COLOR_PAIR(1) | A_BOLD);
            mvaddch(y, COL_USED + 9, ACS_DIAMOND);
            if (has_colors()) attroff(COLOR_PAIR(1) | A_BOLD);
        }

        if (i == selection) attroff(A_REVERSE);
    }
}

static void render_detail(BlockInfo* blocks, int count, int selection,
                          int max_y, int max_x) {
    int dy = 3 + count + 1;
    if (dy > max_y - 5) dy = max_y - 5;
    if (dy < 4)         dy = 4;

    mvhline(dy, 0, ACS_HLINE, max_x);
    dy++;

    if (count > 0 && selection >= 0 && selection < count) {
        const BlockInfo& s = blocks[selection];
        float pct = s.capacity > 0
            ? (float)s.used / s.capacity * 100.0f : 0.0f;

        if (has_colors()) attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(dy, 0, "  [%s]", s.name);
        if (has_colors()) attroff(COLOR_PAIR(4) | A_BOLD);

        char sz[16], pk[16], us[16];
        fmt_size(sz, sizeof(sz), s.capacity);
        fmt_size(pk, sizeof(pk), s.peak_used);
        fmt_size(us, sizeof(us), s.used);

        mvprintw(dy + 1, 2, "capacity:    %s  ", sz);
        mvprintw(dy + 1, 30, "used:    %s (%.0f%%)", us, pct);
        mvprintw(dy + 2, 2, "allocations: %zu", s.allocation_count);
        mvprintw(dy + 2, 30, "peak:    %s", pk);
        mvprintw(dy + 3, 2, "available:   %zuB", s.capacity - s.used);
    } else {
        mvprintw(dy, 2, "(no blocks)");
    }
}

static void render_help(int max_x, int max_y, bool demo) {
    mvhline(max_y - 1, 0, ' ', max_x);
    attron(A_REVERSE);
    if (demo)
        mvprintw(max_y - 1, 0, " 'r' reset  'q' quit ");
    else
        mvprintw(max_y - 1, 0, " 'q' quit ");
    mvprintw(max_y - 1, max_x - 18, "Meta-C v0.1");
    attroff(A_REVERSE);
}

static void setup_ncurses(int timeout_ms) {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(timeout_ms);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_RED,   COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW,COLOR_BLACK);
        init_pair(4, COLOR_CYAN,  COLOR_BLACK);
    }
}

// ─── embedded mode (reads from runtime registry) ─────────────
// ─── modo embutido (le do registro runtime) ──────────────────

void memvis_run(MemVisConfig config) {
    setup_ncurses(config.refresh_ms);

    BlockInfo blocks[META_C_MAX_BLOCKS];
    int block_count = (int)block_snapshot(blocks, META_C_MAX_BLOCKS);

    // fallback: create demo blocks if nothing registered
    // fallback: cria blocos de demonstracao se nada registrado
    BlockCtx* demos[META_C_MAX_BLOCKS];
    int ndemos = 0;
    bool demo = (block_count == 0);

    if (demo) {
        static const char* names[] = {"global", "game", "temp", "assets"};
        static const size_t sizes[] = {256, 64, 32, 128};
        ndemos = 4;

        for (int i = 0; i < ndemos; i++) {
            demos[i] = block_create(sizes[i]);
            block_register(demos[i], names[i]);
            int n = rand() % 50 + 10;
            for (int j = 0; j < n; j++)
                block_alloc(demos[i], (size_t)(rand() % 4096 + 1));
        }
        block_count = (int)block_snapshot(blocks, META_C_MAX_BLOCKS);
    }

    bool running = true;
    int sel = 0;
    char title_sfx[64] = {0};
    if (demo) snprintf(title_sfx, sizeof(title_sfx), "  [demo]");

    while (running) {
        block_count = (int)block_snapshot(blocks, META_C_MAX_BLOCKS);

        if (sel >= block_count) sel = block_count - 1;
        if (sel < 0)            sel = 0;

        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        clear();

        render_title(max_x, config, title_sfx);
        mvhline(1, 0, ACS_HLINE, max_x);
        render_header();
        render_blocks(blocks, block_count, sel, max_y);
        render_detail(blocks, block_count, sel, max_y, max_x);
        render_help(max_x, max_y, demo);

        refresh();

        int ch = getch();
        switch (ch) {
            case 'q': case 'Q': running = false; break;
            case KEY_UP:    if (sel > 0) sel--; break;
            case KEY_DOWN:  if (sel < block_count - 1) sel++; break;
            case 'r': case 'R':
                if (demo) {
                    BlockCtx* c = block_find(blocks[sel].name);
                    if (c) block_reset(c);
                }
                break;
        }
    }

    if (demo) {
        for (int i = 0; i < ndemos; i++) {
            block_unregister(demos[i]);
            block_destroy(demos[i]);
        }
    }

    endwin();
}

// ─── attach mode (reads /tmp/meta-c-mem-<pid>.bin) ──────────
// ─── modo attach (le /tmp/meta-c-mem-<pid>.bin) ──────────────

void memvis_attach(int pid, MemVisConfig config) {
    setup_ncurses(config.refresh_ms);

    BlockInfo blocks[META_C_MAX_BLOCKS];
    bool running = true;
    int sel = 0;
    int block_count = 0;
    bool connected = false;
    char title_sfx[64];
    snprintf(title_sfx, sizeof(title_sfx), "  [attached to PID %d]", pid);

    while (running) {
        block_count = memvis_read_shm(pid, blocks, META_C_MAX_BLOCKS);
        connected = (block_count > 0);

        if (sel >= block_count) sel = block_count - 1;
        if (sel < 0)            sel = 0;

        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        clear();

        render_title(max_x, config, title_sfx);
        mvhline(1, 0, ACS_HLINE, max_x);

        if (!connected) {
            attron(A_BOLD);
            mvprintw(max_y / 2, (max_x - 28) / 2,
                     "Waiting for PID %d...", pid);
            attroff(A_BOLD);
            refresh();
            napms(200);
            int ch = getch();
            if (ch == 'q' || ch == 'Q') running = false;
            continue;
        }

        render_header();
        render_blocks(blocks, block_count, sel, max_y);
        render_detail(blocks, block_count, sel, max_y, max_x);
        render_help(max_x, max_y, false);

        refresh();

        int ch = getch();
        switch (ch) {
            case 'q': case 'Q': running = false; break;
            case KEY_UP:    if (sel > 0) sel--; break;
            case KEY_DOWN:  if (sel < block_count - 1) sel++; break;
        }
    }

    endwin();
}
