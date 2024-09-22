// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Ilya Popov

// This is needed to get gettid() function
#define _GNU_SOURCE

#include "tracing.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <time.h>
#include <unistd.h>

static FILE* tracing_file = NULL;
static long tracing_started_us = 0;
static int tracing_pid = -1;
static __thread int tracing_tid = -1;

static long time_us()
{
    struct timespec t;

    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) {
        return 0;
    }

    return t.tv_sec * 1000000 + (t.tv_nsec + 500) / 1000;
}

static int get_tid()
{
    if (tracing_tid == -1) {
        tracing_tid = gettid();
    }
    return tracing_tid;
}

static void write_event(const char *name, const char *cat, char ph, long ts, const char *extra)
{
    fprintf(tracing_file, "{\"name\": \"%s\", \"cat\": \"%s\", \"ph\": \"%c\", \"pid\": %d, \"tid\": %d, \"ts\": %ld%s},\n",
            name, cat, ph, tracing_pid, get_tid(), ts, extra);
}
static void write_event_no_comma(const char *name, const char *cat, char ph, long ts, const char *extra)
{
    fprintf(tracing_file, "{\"name\": \"%s\", \"cat\": \"%s\", \"ph\": \"%c\", \"pid\": %d, \"tid\": %d, \"ts\": %ld%s}\n",
            name, cat, ph, tracing_pid, get_tid(), ts, extra);
}

void trace_begin(const char *name)
{
    if (!tracing_file) {
        return;
    }
    write_event(name, "", 'B', time_us() - tracing_started_us, "");
}

void trace_end(const char *name)
{
    if (!tracing_file) {
        return;
    }
    write_event(name, "", 'E', time_us() - tracing_started_us, "");
}

void trace_init()
{
    tracing_started_us = time_us();
    tracing_pid = getpid();

    char buffer[64];

    const char* filename = getenv("TRACING_FILE_NAME");
    if (filename == NULL) {
        if (getenv("TRACING_ENABLE") == NULL) {
            return;
        }
        unsigned random = 0;
        int len = getrandom(&random, sizeof(random), 0);
        if (len != sizeof(random)) {
            random = 0;
        }
        len = snprintf(buffer, 64, "tracing-%d-%.8x.json", tracing_pid, random);
        if (len >= 64) {
            filename = "";
        }
        filename = buffer;
    }

    tracing_file = fopen(filename, "w");
    if (!tracing_file) {
        fprintf(stderr, "Warning: could not open tracing file: '%s': %s\n", filename, strerror(errno));
        fprintf(stderr, "Warning: tracing is disabled\n");
        return;
    }
    // disable buffering
    setvbuf(tracing_file, NULL, _IONBF, 0);

    fprintf(tracing_file, "[\n");
    write_event("tracing", "", 'B', time_us() - tracing_started_us, "");
    atexit(trace_close);
}

void trace_close()
{
    if (!tracing_file) {
        return;
    }
    write_event_no_comma("tracing", "", 'E', time_us() - tracing_started_us, "");
    fprintf(tracing_file, "]\n");
    fclose(tracing_file);
    tracing_file = NULL;
}

static void write_metadata_event(const char* event_name, const char* argument, const char* value)
{
    char buffer[256];
    int len = snprintf(buffer, 256, ", \"args\": {\"%s\": \"%s\"}", argument, value);
    if (len >= 256) {
        // output truncated, just use empty string
        buffer[0] = '\0';
    }
    write_event(event_name, "", 'M', time_us() - tracing_started_us, buffer);
}

void trace_set_process_name(const char *name)
{
    if (!tracing_file) {
        return;
    }
    write_metadata_event("process_name", "name", name);
}

void trace_set_thread_name(const char *name)
{
    if (!tracing_file) {
        return;
    }
    write_metadata_event("thread_name", "name", name);
}
