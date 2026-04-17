#include "nabu_lobby.h"

/* Config helpers */
static void set_defaults(void)
{
    copy_text(endpoint, DEFAULT_ENDPOINT, sizeof(endpoint));
    copy_text(platform_name, DEFAULT_PLATFORM, sizeof(platform_name));
    request_source[0] = 0;
    tnfs_host_override[0] = 0;
    page_size = DEFAULT_PAGE_SIZE;
    page_offset = 0;
    tnfs_port = TNFS_PORT;
    more_pages = false;
    status_line[0] = 0;
}

void trim_newline(char *s)
{
    uint8_t i = 0;
    while (s[i]) {
        if (s[i] == '\r' || s[i] == '\n') {
            s[i] = 0;
            return;
        }
        i++;
    }
}

void copy_text(char *dst, const char *src, uint8_t max_len)
{
    uint8_t i = 0;

    while (src[i] && i + 1 < max_len) {
        dst[i] = src[i];
        i++;
    }

    dst[i] = 0;
}

static void apply_config_line(char *line)
{
    char *value;

    trim_newline(line);
    if (line[0] == 0)
        return;
    if (line[0] == '#' || line[0] == ';')
        return;

    if (!strchr(line, '=')) {
        copy_text(player_name, line, sizeof(player_name));
        return;
    }

    value = strchr(line, '=');
    *value++ = 0;

    if (strcmp(line, "NAME") == 0) {
        copy_text(player_name, value, sizeof(player_name));
    } else if (strcmp(line, "ENDPOINT") == 0) {
        copy_text(endpoint, value, sizeof(endpoint));
    } else if (strcmp(line, "PLATFORM") == 0) {
        copy_text(platform_name, value, sizeof(platform_name));
    } else if (strcmp(line, "PAGESIZE") == 0) {
        uint8_t n = (uint8_t)atoi(value);
        if (n > 0 && n <= MAX_SERVERS)
            page_size = n;
    } else if (strcmp(line, "REQUEST") == 0) {
        copy_text(request_source, value, sizeof(request_source));
    } else if (strcmp(line, "TNFSHOST") == 0) {
        copy_text(tnfs_host_override, value, sizeof(tnfs_host_override));
    } else if (strcmp(line, "TNFSPORT") == 0) {
        uint16_t n = (uint16_t)atoi(value);
        if (n > 0)
            tnfs_port = n;
    }
}

/* Config load and save */
void load_config(void)
{
    uint8_t fh;
    uint16_t got;
    uint16_t pos = 0;
    uint8_t line_len = 0;
    uint8_t i;

    set_defaults();
    player_name[0] = 0;

    fh = rn_fileOpen((uint8_t)strlen(CFG_FILE), (uint8_t *)CFG_FILE, OPEN_FILE_FLAG_READONLY, 0xFF);
    if (fh == 0xFF)
        return;

    while (pos < sizeof(response_buf)) {
        got = rn_fileHandleReadSeq(fh, response_buf, pos, 64);
        if (got == 0)
            break;
        pos = (uint16_t)(pos + got);
        if (pos + 64 > sizeof(response_buf))
            break;
    }

    rn_fileHandleClose(fh);

    for (i = 0; i < pos; i++) {
        char c = (char)response_buf[i];
        if (c == '\r')
            continue;

        if (c == '\n') {
            line_buf[line_len] = 0;
            apply_config_line(line_buf);
            line_len = 0;
        } else if (line_len + 1 < sizeof(line_buf)) {
            line_buf[line_len++] = c;
        }
    }

    if (line_len > 0) {
        line_buf[line_len] = 0;
        apply_config_line(line_buf);
    }
}

static void append_cfg_line(uint8_t fh, const char *key, const char *value)
{
    uint8_t len;

    sprintf(line_buf, "%s=%s\n", key, value);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);
}

void save_config(void)
{
    uint8_t fh;
    char small_num[8];

    fh = rn_fileOpen((uint8_t)strlen(CFG_FILE), (uint8_t *)CFG_FILE, OPEN_FILE_FLAG_READWRITE, 0xFF);
    if (fh == 0xFF)
        return;

    rn_fileHandleEmptyFile(fh);
    append_cfg_line(fh, "NAME", player_name);
    append_cfg_line(fh, "ENDPOINT", endpoint);
    append_cfg_line(fh, "PLATFORM", platform_name);
    if (request_source[0])
        append_cfg_line(fh, "REQUEST", request_source);
    if (tnfs_host_override[0])
        append_cfg_line(fh, "TNFSHOST", tnfs_host_override);
    sprintf(small_num, "%u", (unsigned)page_size);
    append_cfg_line(fh, "PAGESIZE", small_num);
    sprintf(small_num, "%u", (unsigned)tnfs_port);
    append_cfg_line(fh, "TNFSPORT", small_num);
    rn_fileHandleClose(fh);
}

/* Debug output */
void write_debug_file(void)
{
    uint8_t fh;
    uint8_t len;

    fh = rn_fileOpen((uint8_t)strlen(DBG_FILE), (uint8_t *)DBG_FILE, OPEN_FILE_FLAG_READWRITE, 0xFF);
    if (fh == 0xFF)
        return;

    rn_fileHandleEmptyFile(fh);

    sprintf(line_buf, "VERSION=%s\n", VERSION_STRING);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "NAME=%s\n", player_name);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "ENDPOINT=%s\n", endpoint);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "PLATFORM=%s\n", platform_name);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "PAGESIZE=%u\n", (unsigned)page_size);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "OFFSET=%u\n", (unsigned)page_offset);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "REQUESTSRC=%s\n", request_source[0] ? request_source : "(generated)");
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "TNFSHOST=%s\n", tnfs_host_override[0] ? tnfs_host_override : "(url host)");
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "TNFSPORT=%u\n", (unsigned)tnfs_port);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    sprintf(line_buf, "REQUESTURL=%s\n", request_url);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);

    rn_fileHandleClose(fh);
}

void append_debug_line(const char *key, const char *value)
{
    uint8_t fh;
    uint8_t len;

    fh = rn_fileOpen((uint8_t)strlen(DBG_FILE), (uint8_t *)DBG_FILE, OPEN_FILE_FLAG_READWRITE, 0xFF);
    if (fh == 0xFF)
        return;

    sprintf(line_buf, "%s=%s\n", key, value);
    len = (uint8_t)strlen(line_buf);
    rn_fileHandleAppend(fh, 0, len, (uint8_t *)line_buf);
    rn_fileHandleClose(fh);
}
