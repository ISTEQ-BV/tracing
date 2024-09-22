// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Ilya Popov

#ifndef TRACING_H

#ifdef __cplusplus
extern "C" {
#endif

void trace_init();
void trace_close();

void trace_set_process_name(const char* name);
void trace_set_thread_name(const char* name);

void trace_begin(const char* name);
void trace_end(const char* name);

#ifdef __GNUC__

inline void trace_scope_end(const char** pname)
{
    trace_end(*pname);
}

#define TRACE_SCOPE(name) \
__attribute__((cleanup(trace_scope_end))) const char* trace_scope_name = (name); \
trace_begin(trace_scope_name);

#define TRACE_FUNC() TRACE_SCOPE(__func__)

#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
