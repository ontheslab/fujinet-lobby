#include "nabu_lobby.h"

/* Player setup */
void prompt_player_name(void)
{
    while (player_name[0] == 0) {
        vt_clearScreen();
        printf(PRODUCT_NAME " " VERSION_STRING "\n");
        printf("------------------------------\n");
        printf("Enter player name: ");
        fflush(stdout);

        if (fgets(player_name, sizeof(player_name), stdin) == NULL)
            player_name[0] = 0;

        trim_newline(player_name);

        if (strlen(player_name) < 2) {
            printf("\nName too short. Press a key.\n");
            getch();
            player_name[0] = 0;
        }
    }

    save_config();
}

/* Selection and data files */
void save_selection(void)
{
    uint8_t fh;
    uint8_t len;

    if (server_count == 0)
        return;

    fh = rn_fileOpen((uint8_t)strlen(SEL_FILE), (uint8_t *)SEL_FILE, OPEN_FILE_FLAG_READWRITE, 0xFF);
    if (fh == 0xFF)
        return;

    rn_fileHandleEmptyFile(fh);

    sprintf(line_buf, "PLAYER=%s\n", player_name);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "TYPE=%u\n", servers[selected].game_type);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "GAME=%s\n", servers[selected].game);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "SERVER=%s\n", servers[selected].server);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "URL=%s\n", servers[selected].url);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "CLIENT_URL=%s\n", servers[selected].client_url);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "REGION=%s\n", servers[selected].region);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "PLATFORM=%s\n", platform_name);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    rn_fileHandleClose(fh);
}

void append_selection_download_info(const char *mode, const char *file_name)
{
    uint8_t fh;
    uint8_t len;

    fh = rn_fileOpen((uint8_t)strlen(SEL_FILE), (uint8_t *)SEL_FILE, OPEN_FILE_FLAG_READWRITE, 0xFF);
    if (fh == 0xFF)
        return;

    sprintf(line_buf, "DOWNLOAD_MODE=%s\n", mode);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "DOWNLOAD_FILE=%s\n", file_name);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "DOWNLOAD_URL=%s\n", servers[selected].client_url);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    rn_fileHandleClose(fh);
}

/* Screen drawing */
void draw_screen(const char *status)
{
    uint8_t i;
    char *game_name;
    char *server_name;

    vt_clearScreen();
    printf(PRODUCT_NAME " " VERSION_STRING "\n");
    printf("Player: %s  Plat: %s  Count: %u\n",
           player_name,
           platform_name,
           (unsigned)server_count);
    printf("Page: %u  Offset: %u  Size: %u\n",
           (unsigned)((page_offset / page_size) + 1),
           (unsigned)page_offset,
           (unsigned)page_size);
    if (request_source[0])
        printf("Src : %.33s\n", request_source);
    else
        printf("Src : %.33s\n", endpoint);
    printf("---------------------------------------\n");

    if (server_count == 0) {
        printf("No servers loaded.\n");
    } else {
        for (i = 0; i < server_count; i++) {
            if (servers[i].game[0])
                game_name = servers[i].game;
            else
                game_name = fallback_game;

            if (servers[i].server[0])
                server_name = servers[i].server;
            else if (servers[i].region[0])
                server_name = servers[i].region;
            else
                server_name = fallback_server;

            printf("%c %-11s %-15s %3u/%-3u\n",
                   i == selected ? '>' : ' ',
                   game_name,
                   server_name,
                   (unsigned)servers[i].players,
                   (unsigned)servers[i].max_players);
        }
    }

    printf("---------------------------------------\n");
    if (server_count > 0) {
        printf("Game : %s\n", servers[selected].game);
        printf("Server: %s\n", servers[selected].server[0] ? servers[selected].server : fallback_server);
        printf("Regn : %s  On: %u\n", servers[selected].region[0] ? servers[selected].region : "--",
               (unsigned)servers[selected].online);
        printf("URL  : %.39s\n", servers[selected].url);
        printf("Client: %.39s\n", servers[selected].client_url);
    } else {
        printf("Game :\n");
        printf("Server:\n");
        printf("Regn :\n");
        printf("URL  :\n");
        printf("Client:\n");
    }

    printf("---------------------------------------\n");
    printf("W/S Move A/D Page Ent Save G Get Q Qt\n");
    if (status && status[0])
        printf("%.39s\n", status);
}
