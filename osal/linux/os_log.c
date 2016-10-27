/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#if defined(__gnu_linux__)
#include <stdio.h>
#include <stdarg.h>
#include "os_log.h"

#define LINE_SZ 1024

void os_log(const char* tag, const char* msg, va_list list)
{
    char line[LINE_SZ] = {0};
    snprintf(line, sizeof(line), "%s: %s", tag, msg);
    vfprintf(stdout, line, list);
}

void os_err(const char* tag, const char* msg, va_list list)
{
    char line[LINE_SZ] = {0};
    snprintf(line, sizeof(line), "%s: %s", tag, msg);
    vfprintf(stderr, line, list);
}

#endif
