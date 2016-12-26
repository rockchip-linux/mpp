/*
 * Copyright 2017 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>

#include "mpp_parse_cfg.h"

#include "mpp_log.h"
#include "rk_type.h"

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(x[0]))

enum CONFIG_TYPE {
    CONFIG_TYPE_OPTION,
    CONFIG_TYPE_EVENT,

    OPT_TYPE_INDEX_TYPE,
    OPT_TYPE_LOOP_NUM,

    IDX_TYPE_FRM_NUM,
    IDX_TYPE_MSEC,
};

struct options_table {
    int type;
    const char *type_str;
};

/* table for converting string to type */
static struct options_table op_tbl[] = {
    {CONFIG_TYPE_OPTION,    "[CONFIG]"},
    {CONFIG_TYPE_EVENT,     "[EVENT]"},

    {OPT_TYPE_INDEX_TYPE,   "index"},
    {OPT_TYPE_LOOP_NUM,     "loop"},

    {IDX_TYPE_FRM_NUM,      "frm"},
    {IDX_TYPE_MSEC,         "msec"},
};

struct cfg_file {
    FILE *file;
    char cache[128];
    RK_S32 cache_on;
};

/* get rid of leading and following space from string */
static char *string_trim(char *string)
{
    char *p = string;
    long len;

    if (string == NULL)
        return NULL;

    while (*p == ' ')
        p++;

    len = strlen(p);

    while (p[len - 1] == ' ')
        len--;
    p[len] = '\0';

    return p;
}

static char *read_cfg_line(struct cfg_file *cfg)
{
    int ch;
    int i;

    while (!cfg->cache_on) {
        i = 0;

        while (1) {
            ch = fgetc(cfg->file);
            if (i == 0 && ch == EOF)
                return NULL;
            else if (ch == EOF || feof(cfg->file) || ch == '\n')
                break;

            cfg->cache[i++] = ch;
        }
        cfg->cache[i] = '\0';

        /* a note, ignore and get the next line */
        if (cfg->cache[0] != '#')
            break;
    }

    cfg->cache_on = 1;
    return string_trim(cfg->cache);
}

static inline void invalid_cfg_cache(struct cfg_file *cfg)
{
    cfg->cache_on = 0;
}

static char *get_opt_value(char *line)
{
    size_t i;

    for (i = 0; i < strlen(line); ++i) {
        if (line[i] == ':')
            return string_trim(&line[i + 1]);
    }

    return NULL;
}

/* convert string to index by look up map table */
static int lookup_opt_type(char *line)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(op_tbl); ++i) {
        if (!strncmp(op_tbl[i].type_str, line,
                     strlen(op_tbl[i].type_str))) {
            mpp_log("option type %s find\n", op_tbl[i].type_str);
            return op_tbl[i].type;
        }
    }

    return -1;
}

static int scan_event_line(struct cfg_file *cfg, struct rc_event *event)
{
    char *line = read_cfg_line(cfg);

    if (line != NULL && line[0] != '\n' && line[0] != '[') {
        sscanf(line, "%d\t%d\t%f",
               &event->idx, &event->bps, &event->fps);
        mpp_log("idx: %d, bps %u, fps: %f\n",
                event->idx, event->bps, event->fps);
        invalid_cfg_cache(cfg);
    } else {
        return -1;
    }

    return 0;
}

static int parse_events(struct cfg_file *cfg, struct rc_test_config *ea)
{
    int ret;
    int i;

    for (i = 0; i < 128; ++i) {
        ret = scan_event_line(cfg, &ea->event[i]);
        if (ret < 0) {
            ea->event_cnt = i;
            break;
        }
    }

    return 0;
}

static int parse_options(struct cfg_file *cfg, struct rc_test_config *ea)
{
    char *opt;
    int type;

    while (1) {
        opt = read_cfg_line(cfg);

        if (opt && opt[0] != '\n' && opt[0] != '[') {
            type = lookup_opt_type(opt);

            switch (type) {
            case OPT_TYPE_INDEX_TYPE:
                ea->idx_type = lookup_opt_type(get_opt_value(opt));
                break;
            case OPT_TYPE_LOOP_NUM:
                sscanf(get_opt_value(opt), "%d", &ea->loop);
                mpp_log("loop num: %d\n", ea->loop);
                break;
            default:
                break;
            }

            invalid_cfg_cache(cfg);
        } else {
            break;
        }
    }
    return 0;
}

int mpp_parse_config(char *cfg_url, struct rc_test_config *ea)
{
    struct cfg_file cfg;

    if (cfg_url == NULL || strlen(cfg_url) == 0) {
        mpp_err("invalid input config url\n");
        return -1;
    }

    cfg.file = fopen(cfg_url, "rb");
    if (cfg.file == NULL) {
        mpp_err("fopen %s failed\n", cfg_url);
        return -1;
    }
    cfg.cache_on = 0;

    while (1) {
        char *line = read_cfg_line(&cfg);

        if (!line)
            break;

        invalid_cfg_cache(&cfg);
        if (line[0] == '[') {
            int type = lookup_opt_type(line);

            switch (type) {
            case CONFIG_TYPE_EVENT:
                parse_events(&cfg, ea);
                break;
            case CONFIG_TYPE_OPTION:
                parse_options(&cfg, ea);
                break;
            default:
                mpp_err("invalid config type find\n");
                fclose(cfg.file);
                return -1;
            }
        }
    }

    fclose(cfg.file);

    return 0;
}

#ifdef PARSE_CONFIG_TEST
int main(int argc, char **argv)
{
    struct rc_test_config event_array;

    if (argc < 2) {
        mpp_err("invalid input argument\n");
        return -1;
    }

    mpp_parse_config(argv[1], &event_array);

    return 0;
}
#endif
