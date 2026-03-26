#define SDL_MAIN_HANDLED

#include <SDL3/SDL.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "launcher_font.h"

#ifdef _WIN32
#include <windows.h>
#define PATH_SEP '\\'
#define PATH_SEP_STR "\\"
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#define PATH_SEP '/'
#define PATH_SEP_STR "/"
#endif

/* ── Limits ─────────────────────────────────────────────────────────── */

#define LAUNCHER_MAX_MODS       128
#define LAUNCHER_MAX_PATH       1024
#define LAUNCHER_MAX_NAME       256
#define LAUNCHER_CONFIG_FILE    "launcher.cfg"

/* ── Colors ─────────────────────────────────────────────────────────── */

typedef struct { uint8_t r, g, b, a; } Color;

static const Color COL_BG        = {  30,  30,  46, 255 };
static const Color COL_PANEL     = {  45,  45,  65, 255 };
static const Color COL_PANEL_LIT = {  55,  55,  80, 255 };
static const Color COL_ACCENT    = {  86, 148, 242, 255 };
static const Color COL_ACCENT_HI = { 120, 175, 255, 255 };
static const Color COL_GREEN     = {  80, 200, 120, 255 };
static const Color COL_GREEN_HI  = { 110, 230, 150, 255 };
static const Color COL_RED       = { 220,  80,  80, 255 };
static const Color COL_TEXT      = { 205, 214, 244, 255 };
static const Color COL_TEXT_DIM  = { 140, 145, 170, 255 };
static const Color COL_CHECK     = {  80, 200, 120, 255 };
static const Color COL_BORDER    = {  70,  70, 100, 255 };
static const Color COL_SCROLL_BG = {  40,  40,  55, 255 };
static const Color COL_SCROLL_FG = {  80,  80, 115, 255 };

/* ── Layout constants ───────────────────────────────────────────────── */

#define WIN_W           640
#define WIN_H           520
#define MARGIN           16
#define FONT_SCALE        2
#define CHAR_W           (LAUNCHER_FONT_GLYPH_W * FONT_SCALE)
#define CHAR_H           (LAUNCHER_FONT_GLYPH_H * FONT_SCALE)
#define LINE_H           (CHAR_H + 4)
#define BTN_PAD_X        12
#define BTN_PAD_Y         6
#define MOD_ROW_H        28
#define SCROLL_W         10

/* ── Mod entry ──────────────────────────────────────────────────────── */

typedef struct
{
    char filename[LAUNCHER_MAX_NAME];
    char path[LAUNCHER_MAX_PATH];
    bool enabled;
} ModEntry;

/* ── App state ──────────────────────────────────────────────────────── */

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    bool running;

    /* ROM */
    char rom_path[LAUNCHER_MAX_PATH];

    /* Mods */
    char mods_dir[LAUNCHER_MAX_PATH];
    ModEntry mods[LAUNCHER_MAX_MODS];
    int mod_count;
    int mod_scroll;

    /* UI interaction */
    int mouse_x, mouse_y;
    bool mouse_down;
    bool mouse_clicked;

    /* File dialog pending */
    bool rom_dialog_pending;

    /* Game executable path (auto-detected) */
    char exe_path[LAUNCHER_MAX_PATH];
} LauncherState;

/* ── Drawing helpers ────────────────────────────────────────────────── */

static void set_color(SDL_Renderer *r, Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

static void draw_rect(SDL_Renderer *r, int x, int y, int w, int h, Color c)
{
    SDL_FRect rect = { (float)x, (float)y, (float)w, (float)h };
    set_color(r, c);
    SDL_RenderFillRect(r, &rect);
}

static void draw_rect_outline(SDL_Renderer *r, int x, int y, int w, int h, Color c)
{
    SDL_FRect rect = { (float)x, (float)y, (float)w, (float)h };
    set_color(r, c);
    SDL_RenderRect(r, &rect);
}

static void draw_char(SDL_Renderer *r, int x, int y, char ch, Color c)
{
    int index;
    const uint8_t *glyph;
    int row, col;

    if (ch < LAUNCHER_FONT_FIRST || ch > LAUNCHER_FONT_LAST)
        return;

    index = ch - LAUNCHER_FONT_FIRST;
    glyph = &launcher_font_data[index * LAUNCHER_FONT_GLYPH_H];
    set_color(r, c);

    for (row = 0; row < LAUNCHER_FONT_GLYPH_H; row++)
    {
        uint8_t bits = glyph[row];

        for (col = 0; col < LAUNCHER_FONT_GLYPH_W; col++)
        {
            if (bits & (0x80 >> col))
            {
                SDL_FRect px = {
                    (float)(x + col * FONT_SCALE),
                    (float)(y + row * FONT_SCALE),
                    (float)FONT_SCALE,
                    (float)FONT_SCALE
                };
                SDL_RenderFillRect(r, &px);
            }
        }
    }
}

static void draw_text(SDL_Renderer *r, int x, int y, const char *text, Color c)
{
    while (*text != '\0')
    {
        draw_char(r, x, y, *text, c);
        x += CHAR_W;
        text++;
    }
}

static int text_width(const char *text)
{
    return (int)strlen(text) * CHAR_W;
}

/* Truncated text that fits within max_w pixels, with "..." suffix if needed */
static void draw_text_clipped(SDL_Renderer *r, int x, int y, const char *text, int max_w, Color c)
{
    int len = (int)strlen(text);
    int max_chars = max_w / CHAR_W;

    if (max_chars <= 0)
        return;

    if (len <= max_chars)
    {
        draw_text(r, x, y, text, c);
    }
    else
    {
        int i;
        int show = max_chars > 3 ? max_chars - 3 : max_chars;

        for (i = 0; i < show && text[i]; i++)
            draw_char(r, x + i * CHAR_W, y, text[i], c);
        if (max_chars > 3)
            draw_text(r, x + show * CHAR_W, y, "...", c);
    }
}

static bool point_in_rect(int px, int py, int rx, int ry, int rw, int rh)
{
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}

/* Returns true if the button was clicked this frame */
static bool draw_button(LauncherState *s, int x, int y, int w, int h,
                         const char *label, Color bg, Color bg_hover)
{
    bool hovered = point_in_rect(s->mouse_x, s->mouse_y, x, y, w, h);
    bool clicked = hovered && s->mouse_clicked;
    Color col = hovered ? bg_hover : bg;
    int tx, ty;

    draw_rect(s->renderer, x, y, w, h, col);
    draw_rect_outline(s->renderer, x, y, w, h, COL_BORDER);

    tx = x + (w - text_width(label)) / 2;
    ty = y + (h - CHAR_H) / 2;
    draw_text(s->renderer, tx, ty, label, COL_TEXT);

    return clicked;
}

/* ── Path utilities ─────────────────────────────────────────────────── */

static const char *path_basename(const char *path)
{
    const char *slash = strrchr(path, '/');
    const char *bslash = strrchr(path, '\\');
    const char *last = NULL;

    if (slash && bslash)
        last = slash > bslash ? slash : bslash;
    else if (slash)
        last = slash;
    else if (bslash)
        last = bslash;

    return last ? last + 1 : path;
}

static void path_directory(char *out, size_t cap, const char *path)
{
    const char *base = path_basename(path);

    if (base == path)
    {
        snprintf(out, cap, ".");
    }
    else
    {
        size_t len = (size_t)(base - path);
        if (len >= cap)
            len = cap - 1;
        memcpy(out, path, len);
        /* Strip trailing separator */
        if (len > 0 && (out[len - 1] == '/' || out[len - 1] == '\\'))
            len--;
        out[len] = '\0';
    }
}

static bool file_exists(const char *path)
{
    FILE *fp = fopen(path, "rb");

    if (fp == NULL)
        return false;
    fclose(fp);
    return true;
}

static void maybe_seed_asset_root_from_exe(char *out_repo_root, size_t cap, const char *exe_path)
{
    char exe_dir[LAUNCHER_MAX_PATH];
    char repo_root[LAUNCHER_MAX_PATH];
    char bundle_path[LAUNCHER_MAX_PATH];
    const char *existing;

    if (out_repo_root != NULL && cap != 0u)
        out_repo_root[0] = '\0';

    existing = getenv("FIRERED_ASSET_ROOT");
    if (existing != NULL && existing[0] != '\0')
    {
        if (out_repo_root != NULL && cap != 0u)
            snprintf(out_repo_root, cap, "%s", existing);
        return;
    }

    path_directory(exe_dir, sizeof(exe_dir), exe_path);
    path_directory(repo_root, sizeof(repo_root), exe_dir);
    snprintf(bundle_path, sizeof(bundle_path), "%s" PATH_SEP_STR "cores" PATH_SEP_STR "firered" PATH_SEP_STR
        "portable" PATH_SEP_STR "data" PATH_SEP_STR "omega_map_layout_rom_companion_bundle.bin", repo_root);
    if (!file_exists(bundle_path))
        return;

#ifdef _WIN32
    SetEnvironmentVariableA("FIRERED_ASSET_ROOT", repo_root);
#else
    setenv("FIRERED_ASSET_ROOT", repo_root, 1);
#endif

    if (out_repo_root != NULL && cap != 0u)
        snprintf(out_repo_root, cap, "%s", repo_root);
}

/* FIRERED_TRACE_MOD_LAUNCH=1: log launch command + paths (stderr + sdl_mod_launch.log beside game exe). */
static int launcher_trace_mod_launch(void)
{
    static int tri = -1;
    const char *e;

    if (tri >= 0)
        return tri;
    e = getenv("FIRERED_TRACE_MOD_LAUNCH");
    tri = (e != NULL && e[0] != '\0' && strcmp(e, "0") != 0);
    return tri;
}

static FILE *launcher_trace_log_fp;

static void launcher_trace_open_log(const char *exe_path_for_dir)
{
    char exe_dir[LAUNCHER_MAX_PATH];
    char logpath[LAUNCHER_MAX_PATH];

    if (!launcher_trace_mod_launch())
        return;
    launcher_trace_log_fp = NULL;
    path_directory(exe_dir, sizeof(exe_dir), exe_path_for_dir);
    snprintf(logpath, sizeof(logpath), "%s" PATH_SEP_STR "sdl_mod_launch.log", exe_dir);
    launcher_trace_log_fp = fopen(logpath, "a");
}

static void launcher_trace_close_log(void)
{
    if (launcher_trace_log_fp != NULL)
    {
        fclose(launcher_trace_log_fp);
        launcher_trace_log_fp = NULL;
    }
}

static void launcher_tracef(const char *fmt, ...)
{
    va_list ap;
    char buf[1536];

    if (!launcher_trace_mod_launch())
        return;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fprintf(stderr, "[firered launcher] %s\n", buf);
    fflush(stderr);
    if (launcher_trace_log_fp != NULL)
    {
        fprintf(launcher_trace_log_fp, "[firered launcher] %s\n", buf);
        fflush(launcher_trace_log_fp);
    }
}

/* ── Mod scanning ───────────────────────────────────────────────────── */

static bool str_ends_with(const char *str, const char *suffix)
{
    size_t slen = strlen(str);
    size_t xlen = strlen(suffix);

    if (xlen > slen)
        return false;
    return strcmp(str + slen - xlen, suffix) == 0;
}

static bool launcher_patch_path_is_ups(const char *path)
{
    return str_ends_with(path, ".ups") || str_ends_with(path, ".UPS");
}

static void scan_mods(LauncherState *s)
{
    int old_count = s->mod_count;
    ModEntry old_mods[LAUNCHER_MAX_MODS];
    int i, j;

    /* Save old enabled states */
    memcpy(old_mods, s->mods, sizeof(old_mods));
    s->mod_count = 0;

    if (s->mods_dir[0] == '\0')
        return;

#ifdef _WIN32
    {
        WIN32_FIND_DATAA fdata;
        HANDLE hFind;
        char pattern[LAUNCHER_MAX_PATH];
        static const char *wild[] = { "*.bps", "*.ups" };
        int w;

        for (w = 0; w < 2; w++)
        {
            snprintf(pattern, sizeof(pattern), "%s\\%s", s->mods_dir, wild[w]);
            hFind = FindFirstFileA(pattern, &fdata);
            if (hFind == INVALID_HANDLE_VALUE)
                continue;

            do
            {
                if (s->mod_count >= LAUNCHER_MAX_MODS)
                    break;
                snprintf(s->mods[s->mod_count].filename, LAUNCHER_MAX_NAME, "%s", fdata.cFileName);
                snprintf(s->mods[s->mod_count].path, LAUNCHER_MAX_PATH, "%s\\%s", s->mods_dir, fdata.cFileName);
                s->mods[s->mod_count].enabled = false;
                s->mod_count++;
            } while (FindNextFileA(hFind, &fdata));

            FindClose(hFind);
        }
    }
#else
    {
        DIR *dir = opendir(s->mods_dir);
        struct dirent *ent;

        if (dir == NULL)
            return;

        while ((ent = readdir(dir)) != NULL)
        {
            if (s->mod_count >= LAUNCHER_MAX_MODS)
                break;
            if (ent->d_name[0] == '.')
                continue;
            if (!str_ends_with(ent->d_name, ".bps") && !str_ends_with(ent->d_name, ".ups"))
                continue;

            snprintf(s->mods[s->mod_count].filename, LAUNCHER_MAX_NAME, "%s", ent->d_name);
            snprintf(s->mods[s->mod_count].path, LAUNCHER_MAX_PATH, "%s/%s", s->mods_dir, ent->d_name);
            s->mods[s->mod_count].enabled = false;
            s->mod_count++;
        }

        closedir(dir);
    }
#endif

    /* Restore enabled state for mods that still exist */
    for (i = 0; i < s->mod_count; i++)
    {
        for (j = 0; j < old_count; j++)
        {
            if (strcmp(s->mods[i].filename, old_mods[j].filename) == 0)
            {
                s->mods[i].enabled = old_mods[j].enabled;
                break;
            }
        }
    }
}

/* ── Config persistence ─────────────────────────────────────────────── */

static void config_path(char *out, size_t cap, const char *exe_dir)
{
    snprintf(out, cap, "%s" PATH_SEP_STR "%s", exe_dir, LAUNCHER_CONFIG_FILE);
}

static void save_config(LauncherState *s)
{
    char cfg[LAUNCHER_MAX_PATH];
    char exe_dir[LAUNCHER_MAX_PATH];
    FILE *f;
    int i;

    path_directory(exe_dir, sizeof(exe_dir), s->exe_path);
    config_path(cfg, sizeof(cfg), exe_dir);

    f = fopen(cfg, "w");
    if (f == NULL)
    {
        /* Try writing next to launcher binary as fallback */
        f = fopen(LAUNCHER_CONFIG_FILE, "w");
        if (f == NULL)
            return;
    }

    fprintf(f, "rom=%s\n", s->rom_path);
    fprintf(f, "mods_dir=%s\n", s->mods_dir);

    for (i = 0; i < s->mod_count; i++)
    {
        if (s->mods[i].enabled)
            fprintf(f, "mod=%s\n", s->mods[i].filename);
    }

    fclose(f);
}

static void load_config(LauncherState *s)
{
    char cfg[LAUNCHER_MAX_PATH];
    char exe_dir[LAUNCHER_MAX_PATH];
    char line[LAUNCHER_MAX_PATH + 32];
    char enabled_mods[LAUNCHER_MAX_MODS][LAUNCHER_MAX_NAME];
    int enabled_count = 0;
    FILE *f;
    int i, j;

    path_directory(exe_dir, sizeof(exe_dir), s->exe_path);
    config_path(cfg, sizeof(cfg), exe_dir);

    f = fopen(cfg, "r");
    if (f == NULL)
    {
        f = fopen(LAUNCHER_CONFIG_FILE, "r");
        if (f == NULL)
            return;
    }

    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *nl = strchr(line, '\n');
        if (nl)
            *nl = '\0';
        nl = strchr(line, '\r');
        if (nl)
            *nl = '\0';

        if (strncmp(line, "rom=", 4) == 0)
        {
            snprintf(s->rom_path, sizeof(s->rom_path), "%s", line + 4);
        }
        else if (strncmp(line, "mods_dir=", 9) == 0)
        {
            snprintf(s->mods_dir, sizeof(s->mods_dir), "%s", line + 9);
        }
        else if (strncmp(line, "mod=", 4) == 0 && enabled_count < LAUNCHER_MAX_MODS)
        {
            snprintf(enabled_mods[enabled_count], LAUNCHER_MAX_NAME, "%s", line + 4);
            enabled_count++;
        }
    }

    fclose(f);

    /* Scan and restore enabled states */
    if (s->mods_dir[0] != '\0')
    {
        scan_mods(s);
        for (i = 0; i < s->mod_count; i++)
        {
            for (j = 0; j < enabled_count; j++)
            {
                if (strcmp(s->mods[i].filename, enabled_mods[j]) == 0)
                {
                    s->mods[i].enabled = true;
                    break;
                }
            }
        }
    }
}

/* ── File dialog callback ───────────────────────────────────────────── */

static void rom_dialog_callback(void *userdata, const char * const *filelist, int filter)
{
    LauncherState *s = (LauncherState *)userdata;

    (void)filter;
    s->rom_dialog_pending = false;

    if (filelist == NULL || filelist[0] == NULL)
        return;

    snprintf(s->rom_path, sizeof(s->rom_path), "%s", filelist[0]);

    /* Auto-set mods directory to "mods" next to the ROM */
    if (s->mods_dir[0] == '\0')
    {
        char rom_dir[LAUNCHER_MAX_PATH];
        path_directory(rom_dir, sizeof(rom_dir), s->rom_path);
        snprintf(s->mods_dir, sizeof(s->mods_dir), "%s" PATH_SEP_STR "mods", rom_dir);
    }

    scan_mods(s);
    save_config(s);
}

/* ── Auto-detect game executable ────────────────────────────────────── */

static void detect_exe(LauncherState *s)
{
    const char *candidates[] = {
#ifdef _WIN32
        "decomp_engine_sdl.exe",
        "build\\decomp_engine_sdl.exe",
        "..\\build\\decomp_engine_sdl.exe",
#else
        "decomp_engine_sdl",
        "build/decomp_engine_sdl",
        "../build/decomp_engine_sdl",
        /* When launcher is in build/ alongside the game */
        "./decomp_engine_sdl",
#endif
        NULL
    };
    int i;

    for (i = 0; candidates[i] != NULL; i++)
    {
        FILE *f = fopen(candidates[i], "rb");
        if (f != NULL)
        {
            fclose(f);
            snprintf(s->exe_path, sizeof(s->exe_path), "%s", candidates[i]);
            return;
        }
    }

    /* Fallback */
#ifdef _WIN32
    snprintf(s->exe_path, sizeof(s->exe_path), "decomp_engine_sdl.exe");
#else
    snprintf(s->exe_path, sizeof(s->exe_path), "decomp_engine_sdl");
#endif
}

/* ── Launch game ────────────────────────────────────────────────────── */

static void launch_game(LauncherState *s)
{
    char cmd[4096];
    char exe_dir[LAUNCHER_MAX_PATH];
    char seeded_asset_root[LAUNCHER_MAX_PATH];
    int enabled_count = 0;
    const char *patch_path = NULL;
    int i;
    int sys_ret;

    if (s->rom_path[0] == '\0')
        return;

    launcher_trace_open_log(s->exe_path);
    path_directory(exe_dir, sizeof(exe_dir), s->exe_path);
    maybe_seed_asset_root_from_exe(seeded_asset_root, sizeof(seeded_asset_root), s->exe_path);

    /* Find first enabled mod (SDL shell supports one patch at a time) */
    for (i = 0; i < s->mod_count; i++)
    {
        if (s->mods[i].enabled)
        {
            if (patch_path == NULL)
                patch_path = s->mods[i].path;
            enabled_count++;
        }
    }

    save_config(s);

    if (launcher_trace_mod_launch())
    {
        launcher_tracef("game_exe=%s", s->exe_path);
        launcher_tracef("rom_path=%s", s->rom_path);
        launcher_tracef("mods_dir=%s", s->mods_dir[0] != '\0' ? s->mods_dir : "(unset)");
        launcher_tracef("enabled_mod_count=%d", enabled_count);
        launcher_tracef("launch_workdir=%s", exe_dir);
        launcher_tracef("FIRERED_ASSET_ROOT=%s", seeded_asset_root[0] != '\0' ? seeded_asset_root : "(unchanged)");
        for (i = 0; i < s->mod_count; i++)
        {
            if (s->mods[i].enabled)
                launcher_tracef("  enabled_mod: %s", s->mods[i].path);
        }
        launcher_tracef("selected_patch_for_shell=%s", patch_path != NULL ? patch_path : "(none)");
    }

    if (patch_path != NULL)
    {
        const char *flag = launcher_patch_path_is_ups(patch_path) ? "--ups" : "--bps";

        if (launcher_trace_mod_launch())
            launcher_tracef("launch patch_flag=%s (UPS if .ups/.UPS else BPS) path=%s", flag, patch_path);

        snprintf(cmd, sizeof(cmd),
#ifdef _WIN32
                 "start \"\" /D \"%s\" \"%s\" \"%s\" %s \"%s\"",
#else
                 "\"%s\" \"%s\" %s \"%s\" &",
#endif
                 exe_dir, s->exe_path, s->rom_path, flag, patch_path);
    }
    else
    {
        snprintf(cmd, sizeof(cmd),
#ifdef _WIN32
                 "start \"\" /D \"%s\" \"%s\" \"%s\"",
#else
                 "\"%s\" \"%s\" &",
#endif
                 exe_dir, s->exe_path, s->rom_path);
    }

    if (enabled_count > 1)
        fprintf(stderr, "Launcher: Warning: %d mods enabled but only 1 patch supported at a time. Using: %s\n",
                enabled_count, path_basename(patch_path));

    fprintf(stderr, "Launcher: %s\n", cmd);
    if (launcher_trace_mod_launch())
        launcher_tracef("system() cmdline: %s", cmd);

    sys_ret = system(cmd);
    if (launcher_trace_mod_launch())
    {
        launcher_tracef("system() returned %d (0 often OK; Windows+start may be non-zero)", sys_ret);
        launcher_trace_close_log();
    }
    else
        launcher_trace_close_log();
}

/* ── Main UI ────────────────────────────────────────────────────────── */

static void draw_ui(LauncherState *s)
{
    int y = MARGIN;
    int content_w = WIN_W - 2 * MARGIN;
    int btn_w, btn_h;
    int rom_label_w;
    int rom_display_w;
    int panel_y, panel_h, list_y, list_h;
    int visible_rows, max_scroll, i;
    int play_w, play_h, play_x, play_y;
    int enabled_count = 0;
    char status[128];

    /* ── Title bar ── */
    draw_rect(s->renderer, 0, 0, WIN_W, MARGIN + CHAR_H + MARGIN, COL_PANEL);
    draw_text(s->renderer, MARGIN, MARGIN, "NativeCart Launcher", COL_ACCENT);
    y = MARGIN + CHAR_H + MARGIN + 8;

    /* ── ROM selection ── */
    draw_text(s->renderer, MARGIN, y, "ROM:", COL_TEXT);
    rom_label_w = text_width("ROM:") + 8;

    btn_w = text_width("Browse") + 2 * BTN_PAD_X;
    btn_h = CHAR_H + 2 * BTN_PAD_Y;
    rom_display_w = content_w - rom_label_w - btn_w - 8;

    /* ROM path display box */
    draw_rect(s->renderer, MARGIN + rom_label_w, y - 4, rom_display_w, btn_h, COL_PANEL);
    draw_rect_outline(s->renderer, MARGIN + rom_label_w, y - 4, rom_display_w, btn_h, COL_BORDER);

    if (s->rom_path[0] != '\0')
        draw_text_clipped(s->renderer, MARGIN + rom_label_w + 4, y, path_basename(s->rom_path), rom_display_w - 8, COL_TEXT);
    else
        draw_text_clipped(s->renderer, MARGIN + rom_label_w + 4, y, "No ROM selected", rom_display_w - 8, COL_TEXT_DIM);

    /* Browse button */
    if (draw_button(s, MARGIN + rom_label_w + rom_display_w + 8, y - 4, btn_w, btn_h,
                    "Browse", COL_ACCENT, COL_ACCENT_HI))
    {
        if (!s->rom_dialog_pending)
        {
            static const SDL_DialogFileFilter filters[] = {
                { "GBA ROMs", "gba;bin" },
                { "All files", "*" },
            };
            s->rom_dialog_pending = true;
            SDL_ShowOpenFileDialog(rom_dialog_callback, s, s->window, filters, 2, NULL, false);
        }
    }

    y += btn_h + 16;

    /* ── Mods section ── */
    draw_text(s->renderer, MARGIN, y, "Mods", COL_TEXT);

    /* Refresh button */
    {
        int ref_w = text_width("Refresh") + 2 * BTN_PAD_X;
        int ref_x = WIN_W - MARGIN - ref_w;

        if (draw_button(s, ref_x, y - 4, ref_w, btn_h, "Refresh", COL_PANEL_LIT, COL_ACCENT))
            scan_mods(s);
    }

    y += CHAR_H + 12;

    /* Mods directory display */
    draw_text(s->renderer, MARGIN, y, "Folder:", COL_TEXT_DIM);
    {
        int folder_x = MARGIN + text_width("Folder:") + 8;
        int folder_w = content_w - text_width("Folder:") - 8;

        if (s->mods_dir[0] != '\0')
            draw_text_clipped(s->renderer, folder_x, y, s->mods_dir, folder_w, COL_TEXT_DIM);
        else
            draw_text_clipped(s->renderer, folder_x, y, "(auto: <rom_dir>/mods/)", folder_w, COL_TEXT_DIM);
    }
    y += LINE_H + 4;

    /* ── Mod list panel ── */
    panel_y = y;
    panel_h = WIN_H - panel_y - MARGIN - 60; /* Leave room for Play button */
    list_y = panel_y + 4;
    list_h = panel_h - 8;
    visible_rows = list_h / MOD_ROW_H;

    draw_rect(s->renderer, MARGIN, panel_y, content_w, panel_h, COL_PANEL);
    draw_rect_outline(s->renderer, MARGIN, panel_y, content_w, panel_h, COL_BORDER);

    if (s->mod_count == 0)
    {
        const char *msg = s->mods_dir[0] != '\0'
            ? "No .bps / .ups mods found"
            : "Select a ROM to scan for mods";
        int msg_x = MARGIN + (content_w - text_width(msg)) / 2;
        int msg_y = panel_y + (panel_h - CHAR_H) / 2;
        draw_text(s->renderer, msg_x, msg_y, msg, COL_TEXT_DIM);
    }
    else
    {
        /* Clamp scroll */
        max_scroll = s->mod_count - visible_rows;
        if (max_scroll < 0)
            max_scroll = 0;
        if (s->mod_scroll > max_scroll)
            s->mod_scroll = max_scroll;
        if (s->mod_scroll < 0)
            s->mod_scroll = 0;

        /* Set clip rect for mod list */
        {
            SDL_Rect clip = { MARGIN + 4, list_y, content_w - 8 - SCROLL_W, list_h };
            SDL_SetRenderClipRect(s->renderer, &clip);
        }

        for (i = s->mod_scroll; i < s->mod_count && (i - s->mod_scroll) < visible_rows; i++)
        {
            int row_y = list_y + (i - s->mod_scroll) * MOD_ROW_H;
            int check_x = MARGIN + 12;
            int check_y = row_y + (MOD_ROW_H - 14) / 2;
            int label_x = check_x + 22;
            bool row_hovered = point_in_rect(s->mouse_x, s->mouse_y,
                                              MARGIN + 4, row_y,
                                              content_w - 8 - SCROLL_W, MOD_ROW_H);

            /* Row highlight */
            if (row_hovered)
                draw_rect(s->renderer, MARGIN + 4, row_y, content_w - 8 - SCROLL_W, MOD_ROW_H, COL_PANEL_LIT);

            /* Checkbox */
            draw_rect_outline(s->renderer, check_x, check_y, 14, 14, COL_BORDER);
            if (s->mods[i].enabled)
                draw_rect(s->renderer, check_x + 2, check_y + 2, 10, 10, COL_CHECK);

            /* Filename */
            draw_text_clipped(s->renderer, label_x, row_y + (MOD_ROW_H - CHAR_H) / 2,
                              s->mods[i].filename,
                              content_w - 50 - SCROLL_W,
                              s->mods[i].enabled ? COL_TEXT : COL_TEXT_DIM);

            /* Click to toggle */
            if (row_hovered && s->mouse_clicked)
                s->mods[i].enabled = !s->mods[i].enabled;
        }

        SDL_SetRenderClipRect(s->renderer, NULL);

        /* Scrollbar */
        if (s->mod_count > visible_rows)
        {
            int sb_x = MARGIN + content_w - 4 - SCROLL_W;
            int sb_h = list_h;
            int thumb_h = (visible_rows * sb_h) / s->mod_count;
            int thumb_y;

            if (thumb_h < 20)
                thumb_h = 20;
            thumb_y = list_y + (s->mod_scroll * (sb_h - thumb_h)) / max_scroll;

            draw_rect(s->renderer, sb_x, list_y, SCROLL_W, sb_h, COL_SCROLL_BG);
            draw_rect(s->renderer, sb_x, thumb_y, SCROLL_W, thumb_h, COL_SCROLL_FG);
        }
    }

    /* ── Status + Play button ── */
    for (i = 0; i < s->mod_count; i++)
    {
        if (s->mods[i].enabled)
            enabled_count++;
    }

    if (enabled_count > 1)
        snprintf(status, sizeof(status), "%d mods enabled (only first patch will be applied)", enabled_count);
    else if (enabled_count == 1)
        snprintf(status, sizeof(status), "1 mod enabled");
    else
        snprintf(status, sizeof(status), "No mods enabled (vanilla)");

    {
        int status_y = WIN_H - MARGIN - 10;
        draw_text(s->renderer, MARGIN, status_y, status,
                  enabled_count > 1 ? COL_RED : COL_TEXT_DIM);
    }

    /* Play button */
    play_w = text_width("  PLAY  ") + 2 * BTN_PAD_X;
    play_h = CHAR_H + 2 * BTN_PAD_Y + 8;
    play_x = WIN_W - MARGIN - play_w;
    play_y = WIN_H - MARGIN - play_h;

    if (s->rom_path[0] != '\0')
    {
        if (draw_button(s, play_x, play_y, play_w, play_h, "  PLAY  ", COL_GREEN, COL_GREEN_HI))
            launch_game(s);
    }
    else
    {
        draw_rect(s->renderer, play_x, play_y, play_w, play_h, COL_PANEL);
        draw_rect_outline(s->renderer, play_x, play_y, play_w, play_h, COL_BORDER);
        {
            int tx = play_x + (play_w - text_width("  PLAY  ")) / 2;
            int ty = play_y + (play_h - CHAR_H) / 2;
            draw_text(s->renderer, tx, ty, "  PLAY  ", COL_TEXT_DIM);
        }
    }
}

/* ── Main ───────────────────────────────────────────────────────────── */

int main(int argc, char **argv)
{
    LauncherState state;

    (void)argc;
    (void)argv;

    memset(&state, 0, sizeof(state));
    state.running = true;

    detect_exe(&state);
    load_config(&state);

    /* If a mods dir was configured, rescan */
    if (state.mods_dir[0] != '\0')
        scan_mods(&state);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    state.window = SDL_CreateWindow("NativeCart Launcher", WIN_W, WIN_H, 0);
    if (state.window == NULL)
    {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    state.renderer = SDL_CreateRenderer(state.window, NULL);
    if (state.renderer == NULL)
    {
        state.renderer = SDL_CreateRenderer(state.window, SDL_SOFTWARE_RENDERER);
    }
    if (state.renderer == NULL)
    {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(state.window);
        SDL_Quit();
        return 1;
    }

    while (state.running)
    {
        SDL_Event event;

        state.mouse_clicked = false;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                state.running = false;
                break;
            }

            if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                state.mouse_x = (int)event.motion.x;
                state.mouse_y = (int)event.motion.y;
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                state.mouse_down = true;
                state.mouse_clicked = true;
                state.mouse_x = (int)event.button.x;
                state.mouse_y = (int)event.button.y;
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT)
            {
                state.mouse_down = false;
            }

            if (event.type == SDL_EVENT_MOUSE_WHEEL)
            {
                state.mod_scroll -= (int)event.wheel.y;
            }

            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
            {
                if (event.key.key == SDLK_ESCAPE)
                    state.running = false;

                if (event.key.key == SDLK_RETURN && state.rom_path[0] != '\0')
                    launch_game(&state);
            }
        }

        set_color(state.renderer, COL_BG);
        SDL_RenderClear(state.renderer);

        draw_ui(&state);

        SDL_RenderPresent(state.renderer);
        SDL_Delay(16); /* ~60 FPS */
    }

    save_config(&state);

    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();

    return 0;
}
