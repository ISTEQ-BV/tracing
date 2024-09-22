Tracing
=======

Minimal tracing library in C for Linux generating traces in 
[Chrome Trace Event format](https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview).

Example
-------

*(Example program is in C++ for ease of thread creation. The library itself is C.)*

```cpp
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
            //std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
```

API
---

Synopsys:

    void trace_init();
    void trace_close();

    void trace_set_process_name(const char* name);
    void trace_set_thread_name(const char* name);

    void trace_begin(const char* name);
    void trace_end(const char* name);

    // Macros:
    TRACE_SCOPE()
    TRACE_FUNC()

Description:

    void trace_init();
   
Initialize the tracing library. 

This must be called before other calls to the library and before creating the threads.
Opens the trace json file (either specified in the `TRACE_FILE_NAME` environment
variable, or, if that is not set, with format `trace-<pid>-<random>.json`).
It also registers `trace_close()` to be called at exit (with `atexit()`).

    void trace_close();
   
Close trace file.

Must be called after threads have finished.

    void trace_begin(const char* name);
    
Begin event with name `name`. 
This emits event 'B' in the Chrome Event format.

    void trace_end(const char* name);

End event with name `name`. Name must match a previous begin event. 
This emits event 'E' in the Chrome Event format.

    void trace_set_process_name(const char* name);
    void trace_set_thread_name(const char* name);

Set current process name and set current thread name. 
This emits event 'M' (metadata) in the Chrome Event format.

    TRACE_SCOPE(name);
    
This macro makes a call to `trace_begin(name)` and registers `trace_end(name)`
to be called at scope end, using `__attribute__((cleanup))`. 
This attribute is supported by GCC and Clang (and supposedly Intel). 
String `name` must still be available at the time `trace_end` is called (scope exit).

    TRACE_FUNC();

The same as above but uses current function name as scope name (using `__func__`)

Environment variables
---------------------

    TRACING_FILE_NAME
    
Specify trace file name (this also enables tracing). 

If file name is not specified, but 

    TRACING_ENABLE
    
is set instead, the filename is generated automatically in the form

    trace-<pid>-<random>.json

Performance
-----------

- To preserve maximum information in a crash, the library writes events immediately to the file.
This also allows watching the trace file while the program still runs.
- Writing to the file is synchronised by virtue of using C functions, such as `fprintf`.
- The library makes two syscalls for every event: one `clock_gettime` and one `write`.

Therefore:
- Do not use this library in the inner loops. 
Only use it to trace some larger sections of the code.
- Be especially careful when using it from many threads.

Overhead was measured using the example program above.
Overhead on one event call is approximately 4 us (microseconds).
Overhead of an empty scope is ~ 8 us.

Benchmark system: Ubuntu 23.10, GCC 13.2, AMD Ryzen 5700G

Viewing traces
--------------

Chrome Trace Event format was chosen as one of the most widely supported trace formats.

To view the file, options are:

- **QtCreator** To open use menu item "Analyze" -> "Chrome Trace Format Viewer" -> "Load JSON file".
  Does not show event names by default, use a toolbar button to enable showing information on hover without a click.
- **[Speedscope](https://www.speedscope.app/)** The most friendly visualization, but only shows one thread at a time.
- **[Perfetto](https://ui.perfetto.dev/)**

Viewers that do not work:
- [Firefox profiler](https://profiler.firefox.com/) - cannot load generic Trace Event file. 
https://github.com/firefox-devtools/profiler/issues/4915

Note: Some viewers can have issues with traces longer than ~30 min, seemingly due to integer overflow (time is saved in the file as integer microseconds).

Compatibility and requirements
------------------------------

Scoped macros require a GCC or Clang compiler for `__attribute__((cleanup))`

The library uses non-standard calls `gettid()` and `getrandom()`
and therefore is Linux-only.

License
-------

MIT license

Copyright (c) 2024 Ilya Popov
