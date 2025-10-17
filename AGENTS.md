# Repository Guidelines

## Project Structure & Module Organization
- Root CMake project; build config in `CMakeLists.txt`; C++ entry `main.cpp`.
- Library sources under `sources/` mirrored by headers in `headers/`; keep new pairs aligned and update CMake targets.
- Third-party logging lives in `third_party/spdlog`; avoid modifying vendored code, prefer wrapper helpers.

## Build, Test, and Development Commands
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` configures an out-of-tree build; re-run when dependencies or options change.
- `cmake --build build` compiles the server; add `--config Release` on MSVC generators.
- `./build/reactorHttp` (Linux) or `build\\Release\\reactorHttp.exe` (Windows) runs the binary; set the working directory so static files resolve.
- `ctest --test-dir build` is reserved for automated tests; wire it up alongside new unit or integration suites.

## Coding Style & Naming Conventions
- Target C++17; prefer 4-space indent, brace-on-same-line for function definitions, and namespace comments only when behavior is non-obvious.
- Classes use `PascalCase`, member functions `camelCase`, and member fields end with `_` (e.g., `threadId_`); keep filenames paired (`Buffer.cpp`/`Buffer.h`).
- Group includes as standard / third-party / project headers, using relative paths (`../headers`); maintain platform guards for socket primitives.

## Testing Guidelines
- Manual smoke test: build, run, and hit `http://localhost:20500` to verify directory listing and file downloads.
- Add future tests under `tests/`, register them in `CMakeLists.txt`, and keep ports deterministic; focus on buffers, dispatchers, and HTTP parsing coverage.

## Commit & Pull Request Guidelines
- Write concise, present-tense commits (e.g., `Add epoll edge-trigger guard`); existing history mixes English and Chinese—clarity matters more than language.
- Reference impacted modules in the message body and call out platform-specific logic where reviewers should retest.
- Pull requests should summarize changes, list verification steps (`cmake --build`, runtime smoke), link issues, and add screenshots for HTML output tweaks.

## Platform Configuration Tips
- Adjust defaults in `main.cpp` (`port`, `threadNum`, working directory`) before committing and document the intent in PR notes.
- Tame logging during automated runs via `spdlog::set_level` in initialization code to keep CI output readable.
