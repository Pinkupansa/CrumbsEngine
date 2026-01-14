# TODO List

## Vulkan Layer
- Comments
- Raytracing ? 
- OS Support
- Gizmos support
- rewrite syncobjects which is currently tailored for the swapchain
- rewrite texture bundle to use texturedescriptor
- UI
- instancing
- dont re-record, only update uniform buffers (not possible in moltenVK?)
- implement actual AO
- investigate FPS loss

## Engine Layer
### Graphics Engine

- `Rigidbody`
- `Shader`
- `LogicScript`
- UI System 
- `ParticleSystem`
  
### Physics Engine 
- Gravity
- Collision
- Water ? 


### Audio Engine 

## Editor


CI/CD: add GitHub Actions / GitLab CI to build/test matrix (macOS/Linux/Windows). See CMakeLists.txt.
Automated tests: add unit tests (GoogleTest), integration tests and CTest integration in CMake. Entrypoint: main.
Code coverage & quality gates: coverage, clang-tidy, cppcheck, and failing-on-low-coverage in CI.
Sanitizers & ASAN/UBSAN/TSAN builds in CI for memory/thread safety.
Static analysis / security scans and dependency vulnerability checks.
Pre-commit hooks & format enforcement (clang-format already present: .clang-format, .editorconfig) — enforce in CI.
Linter rules and style guide + CONTRIBUTING.md / CODE_OF_CONDUCT.md; improve README.md and add CHANGELOG.md.
LICENSE file (none present) and third‑party license notices for vendored libs (e.g. stb_image.h, tiny_obj_loader.h).
API docs: Doxygen (or similar) for public headers and shipped API.
Packaging / release process + semantic versioning / tags and install targets in CMakeLists.txt.
Dependency management: pin versions, use submodules or package manager for third‑party libs (assimp, glfw, spirv-tools).
Robust logging (replace ad-hoc std::cout): structured logger (spdlog) and consistent error/severity handling (avoid mixing Debug::Log with std::cout).
Ownership / memory-safety audit: prefer smart pointers / clear ownership instead of raw pointers (e.g. Scene holding Crumb*, Crumb storing raw Scene*).
See Scene and Crumb.
Error handling policy and exceptions strategy (consistent propagation, no noexcept surprises).
Threading model and concurrency docs if multi-threading planned (thread-safety annotations).
Reproducible builds: pinned toolchain, lockfiles, containerized CI.
TODO → actionable issues: convert todo.md into tracked issues/roadmap.