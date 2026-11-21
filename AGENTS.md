# AllegroScore Build & Repo Rules
- Always configure CMake with Ninja (`cmake -B build -G Ninja`) and build in parallel (`cmake --build build -j$(nproc)`).
- Never run `rm -rf build` unless CMake cache is broken, as rebuilding Verovio from scratch is expensive.
- Run headless Qt tests using `xvfb-run -a ./build/AllegroScore -platform offscreen` or `xvfb-run ctest --test-dir build --output-on-failure`.

## Packaging & Release Rules (Mandatory)
- **Target Desktop:** The user runs **CachyOS (Arch Linux)** and does NOT compile manually on their desktop.
- **Definition of Done:** Every milestone or feature release MUST:
  1. Compile cleanly with Ninja (`cmake --build build -j$(nproc)`).
  2. Pass headless smoke tests (`xvfb-run -a ./build/AllegroScore -platform offscreen`).
  3. Rebuild the self-contained Linux AppImage via `./scripts/package_appimage.sh` so `./dist/AllegroScore-x86_64.AppImage` (gitignored) always reflects the latest build.
  4. Commit and push to `origin/main` so `.github/workflows/build.yml` automatically builds and publishes updated release artifacts.
- **AI Backend Standard:** Always keep `AIService` compatible with configurable OpenAI `/v1/chat/completions` endpoints (`baseUrl`, `model`, `apiKey`) so it works seamlessly with local `FreeLLMAPI` as well as cloud providers.

## Strict Engineering & Verification Rules
- **No silent fallbacks:** Never mask failed commands with `|| echo ...` or `|| true` in build/packaging scripts; let errors fail loudly and fix the root cause.
- **Verify every artifact on disk:** Never claim a file, binary, or AppImage was created without running `ls -lh <path>` and testing it directly.
- **Small, incremental edits:** Implement one feature or fix per turn, compile (`cmake --build build -j$(nproc)`), run tests, and commit before moving on.
- **Read before editing:** Always inspect existing files (`CMakeLists.txt`, headers) before modifying them rather than guessing paths or flags.
