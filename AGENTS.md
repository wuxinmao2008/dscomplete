# DsComplete Agent Instructions

These instructions apply to all files under `src/plugins/dscomplete/`.

## Scope

DsComplete is an independently built Qt Creator plugin that provides DeepSeek FIM inline code completion.

Keep the plugin limited to code completion. Do not add chat, Agent behavior, repository indexing, cross-file retrieval, autonomous tool execution, or unrelated AI features.

All implementation changes must remain inside this directory unless the user explicitly approves changes elsewhere.

## External Plugin Boundary

- Build against the installed Qt Creator SDK through `QtCreatorConfig.cmake`.
- Use public exported Qt Creator APIs only.
- Do not include private headers, `_p.h` headers, or depend on another plugin's `Internal` namespace.
- Do not copy implementation code from Copilot or other Qt Creator plugins.
- Copilot sources may be consulted as read-only reference material.
- Do not add this plugin to the repository-level `src/plugins/CMakeLists.txt` or `src/plugins/plugins.qbs` unless explicitly requested.
- Do not install or copy the plugin into `C:\Qt\Tools\qtcreator` without explicit confirmation.

The installed Qt Creator SDK does not currently provide the QtTaskTree development headers required by ProjectExplorer public headers. Do not add a ProjectExplorer dependency or project-level settings until a matching SDK is available.

## Build-System Synchronization

This repository maintains CMake and qbs descriptions in parallel.

Whenever source files, dependencies, tests, or target properties change in `CMakeLists.txt`, make the corresponding change in `dscomplete.qbs`, and vice versa.

`Qt6::TaskTree` is declared as an imported interface target only to satisfy the installed Qt Creator SDK's transitive target metadata. Do not call TaskTree APIs unless the matching headers and import library are available.

## Architecture

Keep responsibilities separated:

- `dscompleteplugin.cpp`: plugin lifecycle, actions, status-bar UI, and candidate cycling.
- `dscompletesettings.*`: global settings and secret storage.
- `dscompleteclient.*`: editor lifecycle, debounce, network requests, cancellation, stale-response checks, and suggestion insertion.
- `dscompleteprotocol.*`: context extraction, endpoint normalization, payload generation, endpoint validation, and response parsing.
- `tst_dscompleteprotocol.cpp`: protocol-level unit tests.
- `package.ps1`: existing binary verification and single-library ZIP packaging.

Keep protocol helpers independent from editor and network objects so they remain easy to test.

## Completion Behavior

- Use DeepSeek FIM requests with `prompt` and `suffix`.
- Keep requests non-streaming unless streaming is explicitly requested.
- Never issue parallel requests merely to synthesize multiple candidates.
- Maintain at most one active request per editor.
- Abort obsolete requests when the cursor, document, editor, or enabled state changes.
- Validate request ID, document revision, cursor position, and current suggestion state before displaying a response.
- Do not overwrite a suggestion displayed by another provider.
- Use `TextEditor::TextSuggestion::Data`, `TextEditor::CyclicSuggestion`, and `TextEditorWidget::insertSuggestion()` rather than implementing custom ghost-text painting.
- Reuse TextEditor's existing apply, apply-word, and apply-line actions.

## Security and Privacy

- Store API keys with `Core::SecretAspect`.
- Never log API keys, Authorization headers, complete request headers, or raw secrets.
- Do not include source context in diagnostic logs.
- Require HTTPS for remote endpoints.
- Permit HTTP only for localhost or loopback addresses.
- Do not ignore TLS errors.
- Keep a bounded response size and a finite transfer timeout.
- Do not automatically retry authentication errors or rate-limited requests.
- Validate all network data before constructing editor suggestions.

## Qt Creator Style

Follow the repository-level instructions in `CLAUDE.md`.

- Qualify free Utils functions with `Utils::`.
- Use `QTC_ASSERT`, `QTC_CHECK`, or `QTC_GUARD` instead of `Q_ASSERT`.
- Use `Utils::creatorColor()` or palette colors for UI colors.
- Use `Utils::StyleHelper::uiFont()` for custom fonts.
- Use `Utils::SpacingTokens` for custom margins and spacing.
- Avoid hard-coded UI dimensions.
- Add comments only when the code cannot express the reason itself, and keep them short.
- Do not introduce abstractions for one-off operations.

## Testing

Configure and build with an MSVC x64 developer environment:

```powershell
. "C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1" `
    -Arch amd64 -HostArch amd64

cmake -S . -B build-ninja-msvc -G Ninja `
    -DCMAKE_BUILD_TYPE=RelWithDebInfo `
    -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64" `
    -DQtCreator_DIR="C:/Qt/Tools/qtcreator/lib/cmake/QtCreator" `
    -DWITH_TESTS=ON

cmake --build build-ninja-msvc
ctest --test-dir build-ninja-msvc --output-on-failure -C RelWithDebInfo
```

For protocol changes, add or update tests covering both valid and malformed responses. Network/editor behavior should use a local fake HTTP service or an injectable transport rather than the real DeepSeek service.

Before completing a change:

1. Build the plugin in `RelWithDebInfo` mode.
2. Run all DsComplete tests.
3. Check editor diagnostics.
4. Verify plugin metadata with `qtplugininfo` when metadata or dependencies change.
5. Confirm Git changes remain inside `src/plugins/dscomplete/`.
6. Confirm API keys and source context are absent from logs and committed fixtures.

For release packaging, run `./package.ps1` after building. The script must not invoke a build, and must continue to reject Debug binaries by default, verify embedded metadata with `qtplugininfo`, keep `DsComplete.dll` at the ZIP root, and emit a SHA-256 checksum.

## Documentation

Keep `ReadMe.md` synchronized with user-visible settings, build commands, supported behavior, security constraints, and known limitations.

Do not create additional Markdown documentation unless explicitly requested.
