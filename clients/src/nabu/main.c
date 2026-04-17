#include "nabu_lobby.h"

/* Shared state */
uint8_t response_buf[RESPONSE_BUF_SIZE];
ServerInfo servers[MAX_SERVERS];
uint8_t server_count = 0;
uint8_t selected = 0;
char player_name[16];
char line_buf[96];
char fallback_game[] = "?";
char fallback_server[] = "--";
char endpoint[96] = DEFAULT_ENDPOINT;
char platform_name[16] = DEFAULT_PLATFORM;
char request_url[160];
char request_source[160];
char status_line[80];
char download_name[16];
char tnfs_host_override[64];
uint8_t page_size = DEFAULT_PAGE_SIZE;
uint16_t page_offset = 0;
uint16_t tnfs_port = TNFS_PORT;
uint16_t response_size = 0;
bool more_pages = false;

/* Main entry */
void main(void)
{
    int key;

    initNABULib();

    load_config();
    prompt_player_name();
    write_debug_file();

    vt_clearScreen();
    printf(PRODUCT_NAME " " VERSION_STRING "\n");
    printf("Cfg  : %s\n", CFG_FILE);
    printf("Name : %s\n", player_name);
    printf("Plat : %s\n", platform_name);
    printf("Size : %u\n", (unsigned)page_size);
    if (request_source[0])
        printf("Src  : %.30s\n", request_source);
    else
        printf("Src  : %.30s\n", endpoint);
    printf("\nFetching...\n");

    refresh_lobby();

    while (1) {
        key = getch();

        if (key == KEY_QUIT_1 || key == KEY_QUIT_2)
            break;

        if ((key == KEY_MOVE_UP_1 || key == KEY_MOVE_UP_2) && server_count > 0) {
            if (selected > 0)
                selected--;
            draw_screen("");
        } else if ((key == KEY_MOVE_DOWN_1 || key == KEY_MOVE_DOWN_2) && server_count > 0) {
            if (selected + 1 < server_count)
                selected++;
            draw_screen("");
        } else if (key == KEY_PAGE_PREV_1 || key == KEY_PAGE_PREV_2) {
            if (page_offset >= page_size)
                page_offset = (uint16_t)(page_offset - page_size);
            else
                page_offset = 0;
            selected = 0;
            refresh_lobby();
        } else if (key == KEY_PAGE_NEXT_1 || key == KEY_PAGE_NEXT_2) {
            if (more_pages) {
                page_offset = (uint16_t)(page_offset + page_size);
                selected = 0;
                refresh_lobby();
            } else {
                draw_screen("No next page");
            }
        } else if (key == KEY_REFRESH_1 || key == KEY_REFRESH_2) {
            refresh_lobby();
        } else if ((key == KEY_GET_1 || key == KEY_GET_2) && server_count > 0) {
            if (download_selected_client())
                draw_screen(status_line);
            else
                draw_screen(status_line);
        } else if ((key == KEY_SELECT_1 || key == KEY_SELECT_2) && server_count > 0) {
            save_selection();
            draw_screen("Selection saved to LOBBYSEL.DAT");
        }
    }
}

/*
 * NABULIB headers pull in implementation bodies, so the local NABU client is
 * compiled as a single translation unit even though the code is split across
 * several source files for readability.
 */
#include "config.c"
#include "net.c"
#include "ui.c"
