# AllegroScore Build & Repo Rules
- Always configure CMake with Ninja (`cmake -B build -G Ninja`) and build in parallel (`cmake --build build -j$(nproc)`).
- Never run `rm -rf build` unless CMake cache is broken, as rebuilding Verovio from scratch is expensive.
- Run headless Qt tests using `xvfb-run ctest --test-dir build --output-on-failure`.
