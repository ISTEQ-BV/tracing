// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Ilya Popov

#include "tracing.h"

#include <string>
#include <thread>
#include <vector>

int main()
{
    trace_init();
    trace_set_process_name("test_tracing_threads");
    TRACE_FUNC();

    std::vector<std::thread> threads;

    trace_begin("Create threads");
    for (int i = 0; i < 8; ++i)
    {
        TRACE_SCOPE("Create thread");
        threads.emplace_back([i](){
            TRACE_SCOPE("Worker thread");
            std::string name = "Worker thread " + std::to_string(i);
            trace_set_thread_name(name.c_str());
            for (int i = 0; i < 100; ++i) {
                TRACE_SCOPE("Empty scope with contention");
            }
        });
    }
    trace_end("Create threads");

    trace_begin("Wait for threads to finish");
    for (auto& thread : threads) {
        thread.join();
    }
    trace_end("Wait for threads to finish");

    trace_begin("Test performance");
    for (int i = 0; i < 1000; ++i) {
        TRACE_SCOPE("Empty scope");
    }
    trace_end("Test performance");
}
