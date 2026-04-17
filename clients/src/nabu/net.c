#include "nabu_lobby.h"

/* Download buffers */
static uint8_t tnfs_rx_buf[DOWNLOAD_BUF_SIZE];
static uint8_t tnfs_tx_buf[DOWNLOAD_BUF_SIZE];

/*
 * TNFS helpers.
 *
 * The TNFS download path in this file was shaped by the packet flow
 * and session handling used in GTAMP's NABU TNFS client code under:
 * C:\NABU-LIB\nabufuji\tnfs\v0.6
 */
static void close_local_delete(uint8_t local_fh, const char *name)
{
    rn_fileHandleClose(local_fh);
    remove(name);
}

static bool tnfs_parse_url(const char *url, char *host, uint8_t host_len, char *path, uint8_t path_len)
{
    const char *start;
    const char *slash;
    uint8_t i = 0;

    if (strncmp(url, "tnfs://", 7) != 0 && strncmp(url, "TNFS://", 7) != 0)
        return false;

    start = url + 7;
    slash = strchr(start, '/');
    if (!slash)
        return false;

    while (start + i < slash && i + 1 < host_len) {
        host[i] = start[i];
        i++;
    }
    host[i] = 0;

    if (host[0] == 0)
        return false;

    copy_text(path, slash, path_len);
    return path[0] == '/';
}

static bool tnfs_xchg(uint8_t tcp_handle, uint16_t *session_id, uint8_t *seq, uint8_t cmd, uint16_t pay_len)
{
    uint16_t total_tx;
    int32_t got;

    /*
     * Packet layout follows the same general TNFS request and reply shape used
     * by the GTAMP NABU TNFS client reference.
     */
    tnfs_tx_buf[0] = (uint8_t)(*session_id & 0xFF);
    tnfs_tx_buf[1] = (uint8_t)((*session_id >> 8) & 0xFF);
    tnfs_tx_buf[2] = *seq;
    tnfs_tx_buf[3] = cmd;
    total_tx = (uint16_t)(4 + pay_len);

    if (rn_TCPHandleWrite(tcp_handle, 0, total_tx, tnfs_tx_buf) != (int32_t)total_tx)
        return false;

    while (rn_TCPHandleSize(tcp_handle) < 5) {}

    got = rn_TCPHandleRead(tcp_handle, tnfs_rx_buf, 0, DOWNLOAD_BUF_SIZE);
    if (got < 5 || tnfs_rx_buf[2] != *seq) {
        (*seq)++;
        return false;
    }

    *session_id = (uint16_t)tnfs_rx_buf[0] | ((uint16_t)tnfs_rx_buf[1] << 8);
    (*seq)++;
    return true;
}

static bool tnfs_download_selected(void)
{
    char host[64];
    char remote_path[128];
    uint8_t tcp_handle;
    uint8_t local_fh;
    uint8_t remote_fh;
    uint8_t seq = 0;
    uint16_t session_id = 0;
    uint16_t read_len;
    uint16_t total = 0;

    if (!tnfs_parse_url(servers[selected].client_url, host, sizeof(host), remote_path, sizeof(remote_path))) {
        strcpy(status_line, "Bad TNFS URL");
        append_debug_line("DOWNLOAD", "TNFS_BAD_URL");
        return false;
    }

    if (tnfs_host_override[0])
        copy_text(host, tnfs_host_override, sizeof(host));

    save_selection();
    append_selection_download_info("TNFS", download_name);
    append_debug_line("TNFS_HOST", host);
    append_debug_line("TNFS_PATH", remote_path);

    sprintf(status_line, "TNFS %s", download_name);
    draw_screen(status_line);

    tcp_handle = rn_TCPOpen((uint8_t)strlen(host), (uint8_t *)host, tnfs_port, 0xFF);
    if (tcp_handle == 0xFF) {
        sprintf(status_line, "TNFS host timeout %.18s", host);
        append_debug_line("DOWNLOAD", "TNFS_TCP_OPEN_FAILED");
        return false;
    }

    /*
     * The mount, open, read and close sequence follows the same TNFS
     * command order already proven in the GTAMP NABU client.
     */
    tnfs_tx_buf[4] = 0x02;
    tnfs_tx_buf[5] = 0x01;
    strcpy((char *)&tnfs_tx_buf[6], "/");
    tnfs_tx_buf[8] = 0x00;
    tnfs_tx_buf[9] = 0x00;
    if (!tnfs_xchg(tcp_handle, &session_id, &seq, TNFS_CMD_MOUNT, 6) || tnfs_rx_buf[4] != 0x00) {
        rn_TCPHandleClose(tcp_handle);
        sprintf(status_line, "TNFS mount err %u", (unsigned)tnfs_rx_buf[4]);
        append_debug_line("DOWNLOAD", "TNFS_MOUNT_FAILED");
        return false;
    }

    local_fh = rn_fileOpen((uint8_t)strlen(download_name),
                           (uint8_t *)download_name,
                           OPEN_FILE_FLAG_READWRITE,
                           0xFF);
    if (local_fh == 0xFF) {
        rn_TCPHandleClose(tcp_handle);
        strcpy(status_line, "Local open failed");
        append_debug_line("DOWNLOAD", "TNFS_LOCAL_OPEN_FAILED");
        return false;
    }
    rn_fileHandleEmptyFile(local_fh);

    tnfs_tx_buf[4] = 0x01;
    tnfs_tx_buf[5] = 0x00;
    strcpy((char *)&tnfs_tx_buf[6], remote_path);
    if (!tnfs_xchg(tcp_handle, &session_id, &seq, TNFS_CMD_OPENFILE, (uint16_t)strlen(remote_path) + 3) ||
        tnfs_rx_buf[4] != 0x00) {
        close_local_delete(local_fh, download_name);
        rn_TCPHandleClose(tcp_handle);
        sprintf(status_line, "TNFS open err %u", (unsigned)tnfs_rx_buf[4]);
        append_debug_line("DOWNLOAD", "TNFS_OPENFILE_FAILED");
        return false;
    }

    remote_fh = tnfs_rx_buf[5];

    while (1) {
        tnfs_tx_buf[4] = remote_fh;
        tnfs_tx_buf[5] = 0x00;
        tnfs_tx_buf[6] = 0x02;

        if (!tnfs_xchg(tcp_handle, &session_id, &seq, TNFS_CMD_READFILE, 3) || tnfs_rx_buf[4] != 0x00)
            break;

        read_len = (uint16_t)tnfs_rx_buf[5] | ((uint16_t)tnfs_rx_buf[6] << 8);
        if (read_len == 0)
            break;

        rn_fileHandleAppend(local_fh, 0, read_len, &tnfs_rx_buf[7]);
        total = (uint16_t)(total + read_len);
    }

    tnfs_tx_buf[4] = remote_fh;
    tnfs_xchg(tcp_handle, &session_id, &seq, TNFS_CMD_CLOSEFILE, 1);
    rn_fileHandleClose(local_fh);
    rn_TCPHandleClose(tcp_handle);

    if (total == 0) {
        remove(download_name);
        strcpy(status_line, "TNFS empty file");
        append_debug_line("DOWNLOAD", "TNFS_ZERO_BYTES");
        return false;
    }

    sprintf(status_line, "Saved %s %u bytes", download_name, (unsigned)total);
    append_debug_line("DOWNLOAD", "TNFS_OK");
    return true;
}

static void derive_download_name(const char *url)
{
    const char *name;
    const char *query;
    const char *dot;
    uint8_t base_len = 0;
    uint8_t ext_len = 0;
    uint8_t i;

    name = strrchr(url, '/');
    if (name)
        name++;
    else
        name = url;

    query = strchr(name, '?');
    if (!query)
        query = name + strlen(name);

    dot = NULL;
    for (i = 0; name + i < query; i++) {
        if (name[i] == '.')
            dot = &name[i];
    }

    memset(download_name, 0, sizeof(download_name));

    if (dot && dot > name) {
        while (name + base_len < dot && base_len < 8) {
            char c = name[base_len];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                download_name[base_len++] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
            else
                download_name[base_len++] = '_';
        }

        if (base_len == 0)
            strcpy(download_name, "CLIENT.BIN");
        else
            download_name[base_len++] = '.';

        while (download_name[0] != 0 && dot + 1 + ext_len < query && ext_len < 3) {
            char c = dot[1 + ext_len];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                download_name[base_len + ext_len] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
            else
                download_name[base_len + ext_len] = '_';
            ext_len++;
        }
        if (download_name[0] != 0)
            download_name[base_len + ext_len] = 0;
    } else {
        while (name + base_len < query && base_len < 8) {
            char c = name[base_len];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                download_name[base_len++] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
            else
                download_name[base_len++] = '_';
        }
        if (base_len == 0)
            strcpy(download_name, "CLIENT.BIN");
        else
            strcpy(download_name + base_len, ".BIN");
    }

    if (download_name[0] == 0)
        strcpy(download_name, "CLIENT.BIN");
}

/* Request and parse helpers */
static void build_request_url(void)
{
    if (request_source[0]) {
        copy_text(request_url, request_source, sizeof(request_url));
    } else {
        sprintf(request_url,
                "%s?bin=1&platform=%s&pagesize=%u&offset=%u",
                endpoint,
                platform_name,
                (unsigned)page_size,
                (unsigned)page_offset);
    }
}

/* Lobby response parsing */
void copy_fixed_string(char *dst, const uint8_t *src, uint8_t len)
{
    uint8_t i;
    uint8_t out = 0;

    for (i = 0; i < len; i++) {
        uint8_t c = src[i];
        if (c == 0)
            break;
        dst[out++] = (char)c;
    }

    while (out > 0 && dst[out - 1] == ' ')
        out--;

    dst[out] = 0;
}

bool fetch_lobby_binary(void)
{
    uint8_t fh;
    uint16_t got;
    uint16_t total = 0;
    int32_t remote_size;

    memset(response_buf, 0, sizeof(response_buf));
    build_request_url();
    write_debug_file();

    fh = rn_fileOpen((uint8_t)strlen(request_url), (uint8_t *)request_url, OPEN_FILE_FLAG_READONLY, 0xFF);
    if (fh == 0xFF) {
        sprintf(status_line, "Open failed for %s", platform_name);
        append_debug_line("FETCH", "OPEN_FAILED");
        return false;
    }

    remote_size = rn_fileHandleSize(fh);

    while (total < sizeof(response_buf)) {
        got = rn_fileHandleReadSeq(fh, response_buf, total, (uint16_t)(sizeof(response_buf) - total));
        if (got == 0)
            break;
        total = (uint16_t)(total + got);
    }

    rn_fileHandleClose(fh);
    response_size = total;

    if (total < RESPONSE_HEADER) {
        sprintf(status_line, "Short reply (%u bytes)", (unsigned)total);
        append_debug_line("FETCH", "SHORT_REPLY");
        return false;
    }

    sprintf(status_line,
            "Fetched %u bytes (size %ld) p%u off%u",
            (unsigned)total,
            (long)remote_size,
            (unsigned)((page_offset / page_size) + 1),
            (unsigned)page_offset);
    append_debug_line("FETCH", "OK");

    return true;
}

bool parse_lobby_binary(void)
{
    uint8_t i;
    uint16_t offset = RESPONSE_HEADER;
    uint8_t reported = response_buf[0];
    uint8_t max_records;

    server_count = 0;
    more_pages = false;
    memset(servers, 0, sizeof(servers));

    max_records = (uint8_t)((response_size - RESPONSE_HEADER) / SERVER_RECORD_SIZE);
    if (reported > max_records)
        reported = max_records;
    if (reported > page_size)
        reported = page_size;
    if (reported > MAX_SERVERS)
        reported = MAX_SERVERS;

    for (i = 0; i < reported; i++) {
        if ((uint16_t)(offset + SERVER_RECORD_SIZE) > response_size)
            break;

        servers[i].game_type = response_buf[offset];
        copy_fixed_string(servers[i].game, &response_buf[offset + 1], 16);
        copy_fixed_string(servers[i].server, &response_buf[offset + 18], 32);
        copy_fixed_string(servers[i].url, &response_buf[offset + 51], 64);
        copy_fixed_string(servers[i].client_url, &response_buf[offset + 116], 64);
        copy_fixed_string(servers[i].region, &response_buf[offset + 181], 2);
        servers[i].online = response_buf[offset + 184];
        servers[i].players = response_buf[offset + 185];
        servers[i].max_players = response_buf[offset + 186];
        servers[i].ping_age = (uint16_t)response_buf[offset + 187] |
                              ((uint16_t)response_buf[offset + 188] << 8);

        server_count++;
        offset = (uint16_t)(offset + SERVER_RECORD_SIZE);
    }

    if (selected >= server_count && server_count > 0)
        selected = 0;
    if (server_count == page_size)
        more_pages = true;

    return server_count > 0;
}

void refresh_lobby(void)
{
    if (!fetch_lobby_binary()) {
        server_count = 0;
        append_debug_line("STATUS", status_line);
        draw_screen(status_line);
        return;
    }

    if (!parse_lobby_binary()) {
        if (response_buf[0] == 0)
            sprintf(status_line, "No entries for %s", platform_name);
        else
            sprintf(status_line, "Parse failed for %s", platform_name);
        append_debug_line("STATUS", status_line);
        draw_screen(status_line);
        return;
    }

    append_debug_line("STATUS", status_line);
    draw_screen(status_line);
}

bool download_selected_client(void)
{
    uint8_t remote_fh;
    uint8_t local_fh;
    uint16_t got;
    uint16_t total = 0;
    char download_buf[DOWNLOAD_BUF_SIZE];

    if (server_count == 0) {
        strcpy(status_line, "No selection");
        return false;
    }

    if (servers[selected].client_url[0] == 0) {
        strcpy(status_line, "No client URL");
        return false;
    }

    if (strncmp(servers[selected].client_url, "tnfs://", 7) == 0 ||
        strncmp(servers[selected].client_url, "TNFS://", 7) == 0) {
        derive_download_name(servers[selected].client_url);
        append_debug_line("DOWNLOAD_URL", servers[selected].client_url);
        append_debug_line("DOWNLOAD_NAME", download_name);
        return tnfs_download_selected();
    }

    derive_download_name(servers[selected].client_url);
    sprintf(status_line, "DL %s", download_name);
    draw_screen(status_line);
    save_selection();
    append_selection_download_info("HTTP", download_name);
    append_debug_line("DOWNLOAD_URL", servers[selected].client_url);
    append_debug_line("DOWNLOAD_NAME", download_name);

    remote_fh = rn_fileOpen((uint8_t)strlen(servers[selected].client_url),
                            (uint8_t *)servers[selected].client_url,
                            OPEN_FILE_FLAG_READONLY,
                            0xFF);
    if (remote_fh == 0xFF) {
        strcpy(status_line, "DL open failed");
        append_debug_line("DOWNLOAD", "OPEN_FAILED");
        return false;
    }

    local_fh = rn_fileOpen((uint8_t)strlen(download_name),
                           (uint8_t *)download_name,
                           OPEN_FILE_FLAG_READWRITE,
                           0xFF);
    if (local_fh == 0xFF) {
        rn_fileHandleClose(remote_fh);
        strcpy(status_line, "Local open failed");
        append_debug_line("DOWNLOAD", "LOCAL_OPEN_FAILED");
        return false;
    }

    rn_fileHandleEmptyFile(local_fh);

    while (1) {
        got = rn_fileHandleReadSeq(remote_fh, (uint8_t *)download_buf, 0, DOWNLOAD_BUF_SIZE);
        if (got == 0)
            break;

        rn_fileHandleAppend(local_fh, 0, got, (uint8_t *)download_buf);
        total = (uint16_t)(total + got);
    }

    rn_fileHandleClose(local_fh);
    rn_fileHandleClose(remote_fh);

    if (total == 0) {
        remove(download_name);
        strcpy(status_line, "HTTP empty file");
        append_debug_line("DOWNLOAD", "HTTP_ZERO_BYTES");
        return false;
    }

    sprintf(status_line, "Saved %s %u bytes", download_name, (unsigned)total);
    append_debug_line("DOWNLOAD", "OK");
    return true;
}
