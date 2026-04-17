/*
 * Local NABU lobby integration header.
 *
 * This is an add-only client that lives inside the FujiNet lobby tree without
 * changing the upstream shared lobby files.
 */

#ifndef NABU_LOBBY_H
#define NABU_LOBBY_H

#define BIN_TYPE BIN_CPM
#define DISABLE_VDP
#define DISABLE_KEYBOARD_INT

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include "../../../../../NABULIB/NABU-LIB.h"
#include "../../../../../NABULIB/RetroNET-FileStore.h"
#include "vars.h"

/* Product identity and file names */
#define PRODUCT_NAME        "NABU Lobby"
#define VERSION_STRING      "1.10.00"
#define CFG_FILE            "NLOBBY.CFG"
#define SEL_FILE            "LOBBYSEL.DAT"
#define DBG_FILE            "LOBBYDBG.TXT"
#define DEFAULT_ENDPOINT    "https://lobby.fujinet.online/view"
#define DEFAULT_PLATFORM    "atari"
#define DEFAULT_PAGE_SIZE   10
#define MAX_SERVERS         16
#define SERVER_RECORD_SIZE  189
#define RESPONSE_HEADER     3
#define RESPONSE_BUF_SIZE   (RESPONSE_HEADER + (MAX_SERVERS * SERVER_RECORD_SIZE))
#define DOWNLOAD_BUF_SIZE   512
#define TNFS_PORT           16384
#define TNFS_CMD_MOUNT      0x00
#define TNFS_CMD_OPENFILE   0x20
#define TNFS_CMD_READFILE   0x21
#define TNFS_CMD_CLOSEFILE  0x23

/* Lobby entry record */
typedef struct ServerInfoTag {
    uint8_t game_type;
    char game[17];
    char server[33];
    char url[65];
    char client_url[65];
    char region[3];
    uint8_t online;
    uint8_t players;
    uint8_t max_players;
    uint16_t ping_age;
} ServerInfo;

/* Shared state */
extern uint8_t response_buf[RESPONSE_BUF_SIZE];
extern ServerInfo servers[MAX_SERVERS];
extern uint8_t server_count;
extern uint8_t selected;
extern char player_name[16];
extern char line_buf[96];
extern char fallback_game[];
extern char fallback_server[];
extern char endpoint[96];
extern char platform_name[16];
extern char request_url[160];
extern char request_source[160];
extern char status_line[80];
extern char download_name[16];
extern char tnfs_host_override[64];
extern uint8_t page_size;
extern uint16_t page_offset;
extern uint16_t tnfs_port;
extern uint16_t response_size;
extern bool more_pages;

/* Shared helpers */
void trim_newline(char *s);
void copy_fixed_string(char *dst, const uint8_t *src, uint8_t len);
void copy_text(char *dst, const char *src, uint8_t max_len);

void load_config(void);
void save_config(void);
void write_debug_file(void);
void append_debug_line(const char *key, const char *value);

bool fetch_lobby_binary(void);
bool parse_lobby_binary(void);
void refresh_lobby(void);
bool download_selected_client(void);

/* User interface */
void prompt_player_name(void);
void save_selection(void);
void append_selection_download_info(const char *mode, const char *file_name);
void draw_screen(const char *status);

#endif
