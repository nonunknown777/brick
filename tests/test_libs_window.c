#include "../runtime/libs/window/window.h"
#include "../runtime/block_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <X11/Xlib.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void name()
#define RUN(name) do { \
    name(); \
    printf("[PASS] %s\n", #name); \
    tests_passed++; \
} while(0)

#define FAIL(msg) do { \
    printf("[FAIL] %s\n", msg); \
    tests_failed++; \
    return; \
} while(0)

static BlockCtx* test_block = NULL;

static int has_display(void) {
    const char* d = getenv("DISPLAY");
    return d && d[0] != '\0';
}

static int x11_error_occurred = 0;

static int x11_error_handler(Display* dpy, XErrorEvent* ev) {
    (void)dpy; (void)ev;
    x11_error_occurred = 1;
    return 0;
}

static MetaWindow* create_hidden(const char* title, int w, int h, uint32_t extra_flags) {
    return meta_window_create(test_block, title, w, h, META_WINDOW_HIDDEN | extra_flags);
}

TEST(test_create_destroy) {
    MetaWindow* win = create_hidden("test_create_destroy", 640, 480, 0);
    if (!win) FAIL("test_create_destroy: meta_window_create returned NULL");
    meta_window_destroy(win);
}

TEST(test_create_null_title) {
    MetaWindow* win = create_hidden(NULL, 800, 600, 0);
    if (!win) FAIL("test_create_null_title: meta_window_create returned NULL");
    meta_window_destroy(win);
}

TEST(test_create_min_size) {
    x11_error_occurred = 0;
    MetaWindow* win = create_hidden("min", 1, 1, 0);
    if (x11_error_occurred) FAIL("test_create_min_size: X11 error during creation");
    if (!win) FAIL("test_create_min_size: meta_window_create returned NULL");
    meta_window_destroy(win);
}

TEST(test_should_close_initial) {
    MetaWindow* win = create_hidden("should_close", 400, 300, 0);
    if (!win) FAIL("test_should_close_initial: create failed");
    int sc = meta_window_should_close(win);
    meta_window_destroy(win);
    if (sc != 0) FAIL("test_should_close_initial: expected 0");
}

TEST(test_get_size) {
    MetaWindow* win = create_hidden("get_size", 1024, 768, 0);
    if (!win) FAIL("test_get_size: create failed");
    int w = 0, h = 0;
    meta_window_get_size(win, &w, &h);
    meta_window_destroy(win);
    if (w != 1024) FAIL("test_get_size: width mismatch");
    if (h != 768) FAIL("test_get_size: height mismatch");
}

TEST(test_get_size_null_output) {
    MetaWindow* win = create_hidden("null_out", 100, 200, 0);
    if (!win) FAIL("test_get_size_null_output: create failed");
    meta_window_get_size(win, NULL, NULL);
    meta_window_destroy(win);
}

TEST(test_set_title) {
    MetaWindow* win = create_hidden("original", 800, 600, 0);
    if (!win) FAIL("test_set_title: create failed");
    meta_window_set_title(win, "new title");
    meta_window_destroy(win);
}

TEST(test_set_title_null) {
    MetaWindow* win = create_hidden("title_null", 800, 600, 0);
    if (!win) FAIL("test_set_title_null: create failed");
    meta_window_set_title(win, NULL);
    meta_window_destroy(win);
}

TEST(test_poll_events_hidden) {
    MetaWindow* win = create_hidden("poll_hidden", 800, 600, 0);
    if (!win) FAIL("test_poll_events_hidden: create failed");
    meta_window_poll_events(win);
    meta_window_destroy(win);
}

TEST(test_next_event_empty) {
    MetaWindow* win = create_hidden("next_empty", 800, 600, 0);
    if (!win) FAIL("test_next_event_empty: create failed");
    meta_window_poll_events(win);
    MetaEvent e;
    int ret = meta_window_next_event(win, &e);
    meta_window_destroy(win);
    if (ret != 0) FAIL("test_next_event_empty: expected 0");
}

TEST(test_swap_buffers) {
    MetaWindow* win = create_hidden("swap", 800, 600, 0);
    if (!win) FAIL("test_swap_buffers: create failed");
    meta_window_swap_buffers(win);
    meta_window_destroy(win);
}

TEST(test_native_handle) {
    MetaWindow* win = create_hidden("native", 800, 600, 0);
    if (!win) FAIL("test_native_handle: create failed");
    void* handle = meta_window_native_handle(win);
    meta_window_destroy(win);
    if (handle == NULL) FAIL("test_native_handle: expected non-NULL");
}

TEST(test_fullscreen_toggle) {
    MetaWindow* win = create_hidden("fullscreen", 800, 600, 0);
    if (!win) FAIL("test_fullscreen_toggle: create failed");
    meta_window_set_fullscreen(win, 1);
    meta_window_set_fullscreen(win, 0);
    meta_window_destroy(win);
}

TEST(test_multiple_windows) {
    MetaWindow* wins[5];
    for (int i = 0; i < 5; i++) {
        char title[32];
        snprintf(title, sizeof(title), "multi_%d", i);
        wins[i] = create_hidden(title, 200 + i * 50, 200 + i * 50, 0);
        if (!wins[i]) {
            for (int j = 0; j < i; j++) meta_window_destroy(wins[j]);
            FAIL("test_multiple_windows: create failed");
        }
    }
    for (int i = 0; i < 5; i++) {
        meta_window_poll_events(wins[i]);
        meta_window_swap_buffers(wins[i]);
    }
    for (int i = 0; i < 5; i++) {
        meta_window_destroy(wins[i]);
    }
}

TEST(test_null_safety) {
    meta_window_destroy(NULL);
    int r1 = meta_window_poll_events(NULL);
    if (r1 != 0) FAIL("test_null_safety: poll_events(NULL) != 0");
    MetaEvent e;
    int r2 = meta_window_next_event(NULL, &e);
    if (r2 != 0) FAIL("test_null_safety: next_event(NULL) != 0");
    meta_window_swap_buffers(NULL);
    int sc = meta_window_should_close(NULL);
    if (sc != 1) FAIL("test_null_safety: should_close(NULL) != 1");
    meta_window_set_title(NULL, "test");
    meta_window_set_fullscreen(NULL, 1);
    void* nh = meta_window_native_handle(NULL);
    if (nh != NULL) FAIL("test_null_safety: native_handle(NULL) != NULL");
}

int main() {
    if (!has_display()) {
        printf("[SKIP] No X display available (set $DISPLAY to run)\n");
        return 77;
    }

    XSetErrorHandler(x11_error_handler);

    test_block = block_create_bytes(262144);
    if (!test_block) {
        printf("[FAIL] Failed to create test block\n");
        return 1;
    }

    printf("=== Window Library Tests (Linux/X11) ===\n\n");

    RUN(test_create_destroy);
    RUN(test_create_null_title);
    RUN(test_create_min_size);
    RUN(test_should_close_initial);
    RUN(test_get_size);
    RUN(test_get_size_null_output);
    RUN(test_set_title);
    RUN(test_set_title_null);
    RUN(test_poll_events_hidden);
    RUN(test_next_event_empty);
    RUN(test_swap_buffers);
    RUN(test_native_handle);
    RUN(test_fullscreen_toggle);
    RUN(test_multiple_windows);
    RUN(test_null_safety);

    block_destroy(test_block);

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
