#include "../pokefirered_core/include/firered_runtime.h"
#include "../include/gba/io_reg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

#define MAX_FRAMES 20000
#define SUCCESS_SETTLE_FRAMES 180

struct MarkerState {
    int saw_main_menu;
    int saw_new_game_scene;
    int saw_new_game;
    int saw_overworld;
};

static int read_file_bytes(const char *path, unsigned char **out_data, size_t *out_size)
{
    FILE *file = fopen(path, "rb");
    unsigned char *data;
    size_t size;

    if (file == NULL)
        return 0;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return 0;
    }

    size = (size_t)ftell(file);
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return 0;
    }

    data = (unsigned char *)malloc(size + 1);
    if (data == NULL)
    {
        fclose(file);
        return 0;
    }

    if (size != 0 && fread(data, 1, size, file) != size)
    {
        free(data);
        fclose(file);
        return 0;
    }

    data[size] = '\0';
    fclose(file);
    *out_data = data;
    *out_size = size;
    return 1;
}

static void build_log_path(char *buffer, size_t buffer_size)
{
#ifdef _WIN32
    const char *temp_dir = getenv("TEMP");

    snprintf(buffer, buffer_size, "%s%s%s", temp_dir != NULL ? temp_dir : ".", PATH_SEP, "pokefirered_gdextension.log");
#else
    snprintf(buffer, buffer_size, "%s", "/tmp/pokefirered_gdextension.log");
#endif
}

static void clear_log_file(const char *path)
{
    FILE *file = fopen(path, "wb");

    if (file != NULL)
        fclose(file);
}

static int file_contains_term(const char *path, const char *term)
{
    unsigned char *data;
    size_t size;
    int found;

    if (!read_file_bytes(path, &data, &size))
        return 0;

    (void)size;
    found = strstr((const char *)data, term) != NULL;
    free(data);
    return found;
}

static void refresh_markers(const char *log_path, struct MarkerState *markers)
{
    if (!markers->saw_main_menu && file_contains_term(log_path, "CB2_MainMenu:"))
    {
        markers->saw_main_menu = 1;
        printf("marker: main menu\n");
    }

    if (!markers->saw_new_game_scene && file_contains_term(log_path, "StartNewGameScene"))
    {
        markers->saw_new_game_scene = 1;
        printf("marker: new game scene\n");
    }

    if (!markers->saw_new_game && file_contains_term(log_path, "CB2_NewGame: enter"))
    {
        markers->saw_new_game = 1;
        printf("marker: cb2 new game\n");
    }

    if (!markers->saw_overworld && file_contains_term(log_path, "CB2_Overworld: enter"))
    {
        markers->saw_overworld = 1;
        printf("marker: overworld\n");
    }
}

static uint16_t choose_buttons(int frame, const struct MarkerState *markers)
{
    int pulse;

    if (!markers->saw_main_menu)
    {
        if (frame > 1500)
        {
            pulse = (frame - 1500) % 90;
            if (pulse < 5)
                return START_BUTTON;
        }
        return 0;
    }

    if (!markers->saw_new_game_scene)
    {
        pulse = frame % 45;
        if (pulse < 5)
            return A_BUTTON;
        return 0;
    }

    if (!markers->saw_new_game)
    {
        pulse = frame % 30;
        if (pulse < 5)
            return A_BUTTON;
        return 0;
    }

    return 0;
}

int main(int argc, char **argv)
{
    const char *rom_path = argc > 1 ? argv[1] : "baserom.gba";
    unsigned char *rom_data;
    size_t rom_size;
    char log_path[512];
    struct MarkerState markers = {0};
    int frame;
    int success_frame = -1;

    if (!read_file_bytes(rom_path, &rom_data, &rom_size))
    {
        fprintf(stderr, "failed to read ROM: %s\n", rom_path);
        return 1;
    }

    build_log_path(log_path, sizeof(log_path));
    clear_log_file(log_path);

    printf("rom: %s (%lu bytes)\n", rom_path, (unsigned long)rom_size);
    printf("log: %s\n", log_path);

    if (!firered_init(rom_data, rom_size))
    {
        fprintf(stderr, "firered_init failed\n");
        free(rom_data);
        return 1;
    }

    for (frame = 0; frame < MAX_FRAMES; frame++)
    {
        uint16_t buttons;

        if ((frame % 15) == 0)
            refresh_markers(log_path, &markers);

        buttons = choose_buttons(frame, &markers);
        firered_set_input(buttons);
        firered_run_frame();

        if ((frame % 120) == 0)
            printf("frame=%d buttons=0x%04X\n", frame, buttons);

        if (markers.saw_overworld)
        {
            if (success_frame < 0)
                success_frame = frame;
            else if (frame - success_frame >= SUCCESS_SETTLE_FRAMES)
                break;
        }
    }

    refresh_markers(log_path, &markers);
    firered_shutdown();
    free(rom_data);

    if (!markers.saw_main_menu)
    {
        fprintf(stderr, "never reached main menu\n");
        return 2;
    }

    if (!markers.saw_new_game_scene)
    {
        fprintf(stderr, "never started new game scene\n");
        return 3;
    }

    if (!markers.saw_new_game)
    {
        fprintf(stderr, "never reached CB2_NewGame\n");
        return 4;
    }

    if (!markers.saw_overworld)
    {
        fprintf(stderr, "never reached overworld\n");
        return 5;
    }

    printf("success: reached overworld without crashing\n");
    return 0;
}
