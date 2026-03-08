# Adapter Registry Implementation Plan

## Goal

Migrate AMK-Horizon-New-Chart from the current mixed raw-frame parsing approach to a long-lived architecture based on:

- adapter registry
- strategy-based per-format processing
- capability-based feature checks
- explicit separation of AUTONOM_5Tx and LWD_4Tx logic
- future-safe extension path for new data structures

## Non-Negotiable Constraints

- Existing application functionality must remain intact.
- Current graph logic and user-visible behavior must not regress during migration.
- File and COM input must continue working during intermediate stages.
- Raw data formats must be explicitly separated instead of being mixed too early.
- This document must be kept up to date during implementation.
- Project currently targets `stdcpp14`, so the architecture must not depend on C++17-only language features.

## Architecture Target

### Core concepts to introduce

- `ToolDataKind`
- `ToolCapabilities`
- Typed raw frame structures per format
- `IToolDataAdapter`
- `ToolDataAdapterRegistry`
- Per-format adapters:
  - `Autonom5TxAdapter`
  - `Lwd4TxAdapter`
- Stable access layer for upper-level calculations and charting

### Long-term direction

The final architecture should follow this pipeline:

`File/COM -> signature detection -> adapter registry -> typed adapter -> typed raw frame -> stable data access/view -> chart/calculation logic`

## Current State

### Already in code

- Support for current AUTONOM_5Tx raw structure
- Temporary support for legacy LWD layout through direct parsing/conversion in `DataModule`
- Temporary automatic selection of raw layout by `signature/toolCode`
- Temporary COM parsing through `DataModule::ParseRawFrame(...)`
- New dedicated files introduced:
  - `ToolDataTypes.h`
  - `ToolDataAdapters.h`
  - `ToolDataAdapters.cpp`
- `DataModule` now routes raw parsing through `ToolDataAdapterRegistry`
- First dedicated adapters implemented for:
  - `AUTONOM_5Tx`
  - `LWD_4Tx`
- New adapter-layer files are added to the Visual Studio project file
- `SerialModule` now routes COM payload parsing through `ToolDataAdapterRegistry` directly
- New capability-aware access header introduced:
  - `ToolDataAccess.h`
- Main calculation paths in `DataModule` now use `ToolDataAccessView`
- `CurveSelectionModule` SMT/rho calculation paths now use `ToolDataAccessView`

### Current limitation

The logic is still separated only partially. The current implementation still converts formats too early into one canonical shape, which is acceptable as a temporary compatibility step, but not as the final architecture.

### Important implementation note

Because the project is configured for C++14, the migration is being built around:

- adapter registry
- strategy objects
- capability-based checks
- typed per-format raw structures

without requiring `std::variant` as a hard dependency.

## Work Plan

## Phase 1. Create architecture scaffolding

Status: Completed

Tasks:
- [x] Introduce shared architecture types in dedicated files
- [x] Define `ToolDataKind`
- [x] Define `ToolCapabilities`
- [x] Define typed raw frame models
- [x] Define `IToolDataAdapter`
- [x] Define `ToolDataAdapterRegistry`

## Phase 2. Move format-specific parsing into adapters

Status: In progress

Tasks:
- [x] Implement `Autonom5TxAdapter`
- [x] Implement `Lwd4TxAdapter`
- [x] Move raw parsing and validation entry-point in `DataModule` to registry-based dispatch
- [x] Move format-specific conversion rules into adapters
- [ ] Move the remaining format assumptions out of higher-level data handling code

## Phase 3. Integrate adapters into file loading

Status: In progress

Tasks:
- [x] Route file signature detection through registry
- [x] Let file loading select adapter instead of direct layout switch
- [ ] Preserve support for `.bin`, `.biv`, `.DEV`
- [ ] Keep existing user-visible file behavior stable

## Phase 4. Integrate adapters into COM flow

Status: Completed for current transport-parsing scope

Tasks:
- [x] Route COM payload parsing through registry
- [x] Keep raw payload recording unchanged
- [x] Keep polling and callbacks working
- [x] Avoid protocol regressions in the current 240-byte payload model

## Phase 5. Build stable upper-layer access

Status: In progress

Tasks:
- [x] Introduce initial stable adapter-driven access layer via `ToolDataAccessView`
- [x] Reduce part of the direct dependency on fixed `[2][5]` arrays in `DataModule`
- [x] Reduce part of the direct dependency on fixed `[2][5]` arrays in `CurveSelectionModule`
- [x] Reduce direct field access in `DataModule` helper methods to `ToolDataAccessView`
- [x] Centralize current UI transmitter-count assumptions in `CurveSelectionModule`
- [x] Store per-record `ToolRecordMetadata` alongside canonical `GP_TOOL_DATA`
- [x] Expose `DataModule::GetDataView(...)` and `TryGetRecordMetadata(...)` for metadata-backed access
- [ ] Prepare for future non-5Tx formats in upper chart-generation logic
- [x] Decide immediate next storage step: cached capabilities per record before fuller `MeasurementView`
- [x] Design and introduce an initial `MeasurementView` API for upper modules

## Phase 6. Migrate calculations gradually

Status: In progress

Tasks:
- [x] Migrate main `DataModule` calculation helpers to stable access layer
- [x] Migrate `CurveSelectionModule` SMT/rho calculations to stable access layer
- [ ] Keep all existing curve behavior stable during migration
- [x] Reduce hardcoded transmitter-count assumptions inside `DataModule`
- [x] Reduce hardcoded 5Tx loops in `CurveSelectionModule` curve-generation paths
- [x] Start consuming metadata-backed `GetDataView(...)` in `CurveSelectionModule`
- [x] Start consuming `MeasurementView` in `CurveSelectionModule` temperature/SMT paths
- [x] Centralize explicit UI transmitter-count constants across `UIModule`, `SchematicSelection`, and `CurveSelectionModule`
- [x] Start consuming `MeasurementView` in `ChartManager` temperature X-axis paths
- [x] Remove raw `GetData()` dependency from `CurveSelectionModule` SMT helpers where record views are enough
- [x] Complete current safe upper-layer `MeasurementView` migration pass for remaining non-adapter consumers
- [x] Centralize legacy fallback transmitter-count for data/access compatibility paths
- [x] Isolate legacy computer-side SMT reconstruction behind named constants and helper types
- [x] Add minimal semantic helpers to `MeasurementView` where they simplify existing upper-layer consumers
- [x] Align `DataModule` temperature-oriented internal paths with `MeasurementView`
- [x] Localize repeated `SchematicModule` UI-contract restoration logic behind local helpers
- [x] Route legacy SMT output indexing through a checked helper instead of direct array access
- [x] Localize receiver-side `SchematicModule` UI-contract checks behind local helpers and named thresholds
- [x] Localize repeated `SchematicModule` selection bounds checks and selected-state reads behind local helpers
- [x] Reduce direct raw-storage coupling inside `DataModule` calculation paths by using `HasData()` / `GetDataSize()`
- [x] Add a narrow single-record raw accessor in `DataModule` as a safer compatibility alternative to whole-vector access
- [x] Repoint `GetMeasurementView(...)` to narrow metadata access instead of direct metadata-vector indexing
- [x] Repoint `SaveDataRange(...)` and range validation helpers to narrow/raw-size access where appropriate
- [x] Repoint `GetDataIndexRange(...)` to `HasData()` / `GetDataSize()` for helper-layer consistency
- [x] Repoint `DataModule` load-result checks to `HasData()` for state-api consistency
- [x] Localize repeated fixed-width transmitter-state updates in `UIModule` behind local helpers
- [x] Localize `ChartModule` default transmitter-state, channel palette, and legend loops behind the shared UI transmitter count
- [x] Localize repeated `CurveSelectionModule` transmitter bounds checks and preset loops behind existing UI helpers
- [x] Formally classify `DataModule::GetData()` as a legacy/compatibility API now that no internal upper-layer call sites remain
- [x] Decide that computer-side SMT remains an explicit legacy 5-output algorithm boundary scoped to `CurveSelectionModule` / `FROM_COMPUTER`
- [x] Inventory the remaining intentional UI `5Tx` assumptions by category so future refactors can distinguish real UI contract from legacy shortcut
- [x] Localize `SchematicModule` element labels, tooltips, and fixed visual slot order behind local helpers / named constants without changing the current 5-slot UI contract
- [x] Localize `CurveSelectionModule` preset titles and repeated display-name construction behind shared helpers without changing the current `T1..T5` UI naming contract
- [x] Inventory the remaining `DataModule` mixed parsing / forced canonical-conversion paths before Phase 7 cleanup
- [x] Localize `DataModule` container-specific frame reading and parsed-frame append behind local helpers without changing canonical `GP_TOOL_DATA` loading
- [ ] Finish identifying and removing remaining 5Tx-centric UI contract assumptions where they are still intentionally fixed

## Phase 7. Remove temporary compatibility shortcuts

Status: In Progress

Tasks:
- [x] Remove duplicated container-specific frame loops from `DataModule` loading path
- [ ] Remove early forced canonical conversion where no longer needed
- [ ] Leave only adapter-based routing

## Current Session Focus

- Create this tracking document
- Introduce the adapter-registry scaffolding in code
- Start extracting raw-format routing out of `DataModule`
- Keep the migration safe and incremental

## Done in This Session

- [x] Created persistent implementation plan document
- [x] Introduced `ToolDataTypes.h`
- [x] Introduced `ToolDataAdapters.h`
- [x] Introduced `ToolDataAdapters.cpp`
- [x] Added new adapter-layer files to `.vcxproj`
- [x] Implemented `Autonom5TxAdapter`
- [x] Implemented `Lwd4TxAdapter`
- [x] Rewired `DataModule` parsing to use `ToolDataAdapterRegistry`
- [x] Rewired `SerialModule` parsing to use `ToolDataAdapterRegistry` directly
- [x] Introduced `ToolDataAccess.h`
- [x] Migrated main `DataModule` calculation paths to `ToolDataAccessView`
- [x] Migrated `CurveSelectionModule` SMT/rho access to `ToolDataAccessView`
- [x] Made `DataModule` derive active transmitter count from current dataset capabilities
- [x] Replaced remaining direct `DataModule` temperature/rho helper access with `ToolDataAccessView`
- [x] Centralized current UI transmitter-count usage in `CurveSelectionModule`
- [x] Replaced scattered `for (... < 5)` curve-generation loops with a shared UI transmitter-count helper
- [x] Added `ToolRecordMetadata` and per-record metadata storage in `DataModule`
- [x] Updated file-loading and real-time append paths to populate record metadata
- [x] Switched `DataModule` access paths to metadata-backed `GetDataView(...)`
- [x] Switched `CurveSelectionModule` temperature/SMT access to metadata-backed `GetDataView(...)`
- [x] Introduced `MeasurementView` in `ToolDataAccess.h`
- [x] Exposed `DataModule::GetMeasurementView(...)`
- [x] Switched initial `CurveSelectionModule` upper-layer paths to `MeasurementView`
- [x] Added explicit UI-count constants to `SchematicSelection` and `UIModule`
- [x] Repointed `CurveSelectionModule` color/preset/UI fallback logic to `SchematicSelection::UI_TRANSMITTER_COUNT`
- [x] Replaced raw `ChartManager` temperature-axis access to `GP_TOOL_DATA::temperature` with `MeasurementView`
- [x] Replaced `CurveSelectionModule` SMT helper raw record-vector access with `GetMeasurementView()` + `GetDataSize()`
- [x] Verified no remaining upper-layer raw `GetData()` consumers outside `DataModule` itself
- [x] Introduced shared `kLegacyFallbackTransmitterCount` and switched `ToolDataAccessView`/`DataModule` fallback paths to it
- [x] Isolated legacy `simmetry_5` SMT computer-side reconstruction in `CurveSelectionModule` behind `LegacySmtPhaseOutputs` and named input/output count constants
- [x] Added `MeasurementView::GetTemperature()` and repointed migrated temperature-oriented consumers in `CurveSelectionModule` and `ChartManager`
- [x] Repointed `DataModule` temperature filtering and X-axis temperature helpers to `MeasurementView::GetTemperature()` for internal consistency
- [x] Replaced repeated `SchematicModule` curve-restoration checks for "all UI transmitters" / transmitter restoration / silent restoration with local helper functions
- [x] Added `TryGetLegacyComputedPhaseSmt(...)` so legacy SMT 5-output access is range-checked and isolated from call sites
- [x] Replaced repeated `SchematicModule` receiver-restoration assignments and the dual-receiver curve threshold literal with local helpers / a named constant
- [x] Replaced repeated `SchematicModule` element index bounds checks / selected-state sync logic with shared local helpers
- [x] Repointed `DataModule` calculation-oriented emptiness/size checks to `HasData()` / `GetDataSize()` where raw storage access was unnecessary
- [x] Added `DataModule::TryGetRecord(...)` and repointed `GetMeasurementView(...)` to use narrow single-record access instead of direct raw-vector indexing
- [x] Repointed `GetMeasurementView(...)` to `TryGetRecordMetadata(...)` instead of direct metadata-vector indexing
- [x] Repointed `SaveDataRange(...)` / `IsValidDataRange(...)` to `TryGetRecord(...)` / `GetDataSize()` where that preserved behavior
- [x] Repointed `GetDataIndexRange(...)` to `HasData()` / `GetDataSize()` instead of direct raw-vector sizing
- [x] Repointed `DataModule` load-result validation / success checks to `HasData()` instead of direct `data.empty()`
- [x] Replaced repeated `UIModule` transmitter-state bounds checks / assignments with shared local helpers
- [x] Replaced `ChartModule` hardcoded transmitter-array sizes, default enabled vectors, legend loops, and modulo-5 color selection with `UIModule::UI_TRANSMITTER_COUNT` + local helpers
- [x] Replaced repeated `CurveSelectionModule` transmitter bounds checks / preset loops with `IsUiTransmitterIndex(...)` and `GetUiTransmitterCount(...)`
- [x] Confirmed there are no remaining internal `.GetData()` call sites outside `DataModule`, so `GetData()` can now be treated as a public legacy/compatibility-only surface while `GetDataSize()` / `TryGetRecord(...)` / `GetMeasurementView(...)` remain the preferred access paths
- [x] Confirmed `ComputePhaseSmtFromComputer(...)` / `TryGetLegacyComputedPhaseSmt(...)` are only used inside `CurveSelectionModule` computer-side SMT branches, and that metrology inversion still assumes transmitter-indexed access compatible with exactly five SMT outputs
- [x] Enumerated the remaining intentional UI `5Tx` assumptions more explicitly: `UIModule` fixed-size enabled state, `ChartModule` `T1..T5` legend/palette naming, `CurveSelectionModule` preset/display naming and transmitter palettes, and `SchematicModule` fixed 5Tx visual layout/order (`T5,T3,T1,R1,R2,T2,T4` plus two silent slots)
- [x] Replaced repeated `SchematicModule` label / tooltip / slot-order string-building with local helper functions and named layout constants, while keeping the existing `T1..T5`, `R1..R2`, and silent-slot UI semantics intact
- [x] Replaced repeated `CurveSelectionModule` preset-title assignments and manual transmitter/silent display-name string-building with `GetPresetDisplayName(...)` and existing `GenerateDisplayName(...)`, while keeping the current localized `T1..T5` / silent naming semantics intact
- [x] Confirmed the main remaining Phase 7 `DataModule` compatibility layer is container-specific file loading / early canonicalization rather than calculation-path parsing; adapter-backed frame parsing itself already routes through `ToolDataAdapterRegistry`
- [x] Replaced repeated `DataModule` file-container frame seeking / raw-frame reading / metadata append logic with local helpers (`GetPhysicalFrameSize(...)`, `ReadParsedFrameAt(...)`, `AppendParsedFrame(...)`) while preserving current canonical `GP_TOOL_DATA` storage
- [x] Unified `DataModule::LoadDataFromFile(...)` `.bin/.biv/.dev` branches into one adapter-backed parsed-frame loop via `LoadParsedFrames(...)`, while preserving the current canonical `GP_TOOL_DATA` + `ToolRecordMetadata` storage contract
- [x] Centralized the canonical append boundary in `DataModule` so both parsed-frame loading and realtime `AddData(...)` / `AddDataBatch(...)` flow through `AppendCanonicalRecord(...)`
- [x] Confirmed the remaining eager canonical-materialization boundary is now concentrated in `DataModule` public/storage seams (`ParseRawFrame(...)`, `LoadDataFromFile(...)`, `AddData(...)`, `AddDataBatch(...)`, `GetData()`, `SaveDataRange(...)`) rather than spread across multiple parsing branches
- [x] Routed realtime ingest through `ParsedToolFrame` / metadata by adding `DataModule::AddParsedData(...)`, `SerialModule` parsed request/callback APIs, and switching `ChartManager` live ingestion to the parsed callback while preserving legacy `GP_TOOL_DATA` compatibility callbacks

## Open Risks

- Upper layers still assume fixed active transmitter count of 5 in many places.
- The current graph/calculation code directly accesses canonical arrays and will need gradual migration.
- COM protocol currently assumes fixed 240-byte payload size; new structures must remain compatible with this transport or require protocol-specific branching later.
- Legacy documentation contains historical drift and must be verified against current code during each migration step.
- The current public API still exposes `GP_TOOL_DATA`, so upper-level separation of format-specific behavior is not yet complete.
- `ToolDataAccessView` still supports fallback derivation from `signature`, but the preferred path is now metadata-backed access via `DataModule`.
- `MeasurementView` now includes a minimal semantic helper for temperature, but any further growth should still be justified by repeated upper-layer usage patterns.
- `DataModule` loading still stores canonical `GP_TOOL_DATA` eagerly after adapter parsing; container-specific control flow is now localized, but adapter-only routing is not yet the sole public/storage model.
- `DataModule` no longer duplicates per-format frame loops, but eager canonical materialization still remains the main compatibility boundary to revisit in later Phase 7 steps.
- `DataModule` now has a single canonical append path, but public APIs and persistence still fundamentally serialize / expose `GP_TOOL_DATA`, so any deeper Phase 7 change will require an explicit decision about storage/API compatibility.
- Realtime polling now carries `ParsedToolFrame` through `SerialModule -> ChartManager -> DataModule`, but file persistence, `RequestData(GP_TOOL_DATA&)`, `ParseRawFrame(...)`, and public raw-storage APIs still preserve the canonical compatibility boundary.
- `CurveSelectionModule`, `SchematicSelection`, and `UIModule` now expose the current 5Tx-centric contract more explicitly, but the UI contract itself is still fixed-width.
- `DataModule::GetData()` still exists as a public low-level compatibility API; current inspection shows no remaining upper-layer consumers, but the API surface is still public and therefore not yet formally downgraded.
- `DataModule` calculation and helper paths are less coupled to raw storage details now, and narrow single-record access is available via `TryGetRecord(...)` / `TryGetRecordMetadata(...)`; loading and the public low-level API still expose canonical storage directly where appropriate.
- SMT computer-side reconstruction still has an explicit 5-output semantic model, which may need a separate design decision if non-5Tx SMT support is introduced.

## Remaining 5Tx Assumptions by Category

### UI contract boundaries

- State / toggles:
  - `UIModule` still stores fixed-size enabled transmitter state for two charts (`chart1EnabledTransmitters` / `chart2EnabledTransmitters`) sized by `UI_TRANSMITTER_COUNT`.
  - `SchematicModule` restoration logic still interprets "all transmitters selected" relative to the current UI-width transmitter set.
- Legend / palette:
  - `ChartModule` still exposes a fixed `T1..T5` legend/palette via `CHANNEL_NAMES` / `CHANNEL_COLORS`, even though the loops now route through the shared UI transmitter count.
  - `CurveSelectionModule` still keeps axis palettes whose first five semantic slots are intentionally mapped to `T1..T5`.
- Naming / presets / labels:
  - `CurveSelectionModule` naming is now routed through shared helpers (`GetPresetDisplayName(...)`, `GenerateDisplayName(...)`), but it still intentionally emits display names such as `T1 Разность фаз` / `T1 Амплитуда R1` and preset families that mean "all UI transmitters".
  - `SchematicModule` label / tooltip generation is now localized behind helper functions, but it still intentionally exposes explicit `T1..T5`, `R1..R2`, and silent `R1/R2` labels.
- Layout / geometry:
  - `SchematicSelection` and `SchematicModule` still model a fixed-width device schematic with `T1..T5`, `R1..R2`, and two silent channels.
  - `SchematicModule` slot-order/layout logic is now localized behind named constants/helpers, but it still intentionally uses the current visual ordering of the seven framed slots as `T5, T3, T1, R1, R2, T2, T4`, followed by two silent slots outside the frame.

### Algorithm / domain-specific boundaries

- `CurveSelectionModule::ComputePhaseSmtFromComputer(...)` is now explicitly isolated as a legacy helper, but it still implements `simmetry_5` coefficients and produces exactly five SMT outputs.
- Metrology-driven SMT inversion still assumes transmitter-indexed access compatible with the current 5-output SMT model, although direct legacy output indexing is now wrapped by `TryGetLegacyComputedPhaseSmt(...)`.
- Decision: keep computer-side SMT as an explicit legacy `4 delta inputs -> 5 outputs` boundary inside `CurveSelectionModule` `FROM_COMPUTER` mode for now, rather than promoting it into a generic capability-aware algorithm API. Revisit only if a non-5Tx SMT source or adapter-backed generalized SMT contract appears.

### Compatibility fallbacks

- `ToolDataAccessView::GetTransmitterCount()` and `DataModule::GetAvailableTransmitterCount()` now share a single compatibility fallback constant rather than separate hardcoded `5` values.
- `DataModule::GetData()` remains publicly available as a legacy low-level API, but current inspection shows no remaining internal upper-layer consumers; preferred internal access is now `GetDataSize()` / `TryGetRecord(...)` / `GetMeasurementView(...)`.

## Next Immediate Actions

1. Decide which remaining 5Tx assumptions are true UI constraints versus temporary implementation shortcuts.
2. If non-5Tx SMT support appears later, introduce a dedicated generalized SMT boundary instead of widening the current legacy `simmetry_5` helper.
3. Decide whether `MeasurementView` should grow beyond the current minimal semantic helpers for chart/UI consumers.
4. Keep `DataModule::GetData()` public only as a legacy/compatibility surface unless a real caller justifies broader usage again.
5. Keep this document updated after each meaningful step.
