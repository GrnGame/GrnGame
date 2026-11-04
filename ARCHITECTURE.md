# GrnGame Architecture

This document describes the structure of the GrnGame repository and the
responsibility of each directory.

## Repository structure

### `/runtime`

Contains the runtime entry point. It starts the application and links the
GrnGame engine.

### `/tests`

Contains tests that verify the engine's behavior. Tests cover features such as
JSON, sound, controller input, text rendering, versioning and general engine
workflows.

### `/android-build`

Contains the Gradle project used to build the Android application. Android
resources, Java integration and application settings are stored here.

### `/ressources`

Contains the default project resources.

### `/scripts`

Contains automation scripts for project creation and updates, metadata
processing, testing, packaging and releases.

#### `/scripts/project`

Creates, updates and distributes GrnGame projects.

#### `/scripts/metadata`

Reads and processes project metadata.

#### `/scripts/test`

Runs the development and embedded test pipelines.

#### `/scripts/release`

Builds release packages and archives.

### `/pack`

Contains cmake rules used to package the project and create distributable
builds.

### `/cmake`

Contains the Lua modules used by the main cmake build.

### `/std`

Contains the Wren standard-library modules distributed with GrnGame.

### `/grngame`

Contains the engine source code. Each subdirectory should have one clear
responsibility.

#### `/grngame/core`

Contains the core engine lifecycle and shared systems, including application
initialization, windows, threads, messages and global engine data.

#### `/grngame/assets`

Loads and manages assets. It also contains the embedded-file manager used for
packaged resources.

#### `/grngame/audio`

Provides audio playback and audio resource management, including sounds,
filters, sound metadata and the sound manager.

#### `/grngame/bindings`

Connects native C code to Wren.

###### `/grngame/bindings/wren/modules`

Contains Wren-facing modules for input, events, windows, rendering, sound,
JSON, time and application control.

#### `/grngame/data`

Provides access to files and structured data, including JSON and SQLite.

#### `/grngame/dev`

Contains development tools.

#### `/grngame/embedded`

Contains the embedded-file logic.

#### `/grngame/input`

Manages keyboard, mouse, controller and text input, as well as event polling.

#### `/grngame/math`

Contains shared mathematical types and helper functions.

#### `/grngame/memory`

Contains engine memory utilities and allocators.

#### `/grngame/platform`

Provides operating-system abstractions for paths, directories, virtual memory
and platform or compiler checks.

#### `/grngame/renderer`

Contains rendering systems and resources.

#### `/grngame/text`

text logic system

#### `/grngame/utils`

Contains small reusable helpers.

#### `/grngame/web`

Contains interfaces and integration points for Web builds.

## Naming conventions

### Casing

- **Types:** `PascalCase`
- **Functions:** `PascalCase`
- **Macros:** `UPPER_SNAKE_CASE`
- **Variables, fields and other identifiers:** `snake_case`

Functions that operate on a structure or belong to a module should place the
subject or module name first. Examples:

- `WindowCreate`
- `PhysicsTick`
- `AudioLoad`

## Error handling

Always handle errors and check return values. If an error cannot be handled
immediately, add a precise `TODO: handle ...` comment explaining what remains.
Do not silently ignore failures.

## General guidelines

- Keep each `.c`/`.h` pair focused on one clear responsibility.
- Prefer functions shorter than 40 lines when this improves readability.
- Extract reusable logic into an appropriate public utility.
- Keep a single logical operation in one module whenever possible.
- Preserve clear ownership and cleanup for allocated resources.
- Add or update tests when changing observable engine behavior.
- These rules may be relaxed when doing so improves readability, portability or
  correctness.
