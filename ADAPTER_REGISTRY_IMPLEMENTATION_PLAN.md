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
- [ ] Finish identifying and removing remaining 5Tx-centric UI contract assumptions where they are still intentionally fixed

## Phase 7. Remove temporary compatibility shortcuts

Status: Pending

Tasks:
- [ ] Remove direct mixed parsing from `DataModule`
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

## Open Risks

- Upper layers still assume fixed active transmitter count of 5 in many places.
- The current graph/calculation code directly accesses canonical arrays and will need gradual migration.
- COM protocol currently assumes fixed 240-byte payload size; new structures must remain compatible with this transport or require protocol-specific branching later.
- Legacy documentation contains historical drift and must be verified against current code during each migration step.
- The current public API still exposes `GP_TOOL_DATA`, so upper-level separation of format-specific behavior is not yet complete.
- `ToolDataAccessView` still supports fallback derivation from `signature`, but the preferred path is now metadata-backed access via `DataModule`.
- `MeasurementView` is intentionally still thin; more semantic methods may be needed before wider upper-layer migration.
- `CurveSelectionModule`, `SchematicSelection`, and `UIModule` now expose the current 5Tx-centric contract more explicitly, but the UI contract itself is still fixed-width.
- `DataModule::GetData()` still exists as a public low-level compatibility API, even though current upper-layer safe migration targets no longer depend on it.
- SMT computer-side reconstruction still has an explicit 5-output semantic model, which may need a separate design decision if non-5Tx SMT support is introduced.

## Remaining 5Tx Assumptions by Category

### UI contract boundaries

- `UIModule` keeps fixed-size transmitter visibility state for chart/UI interaction.
- `SchematicSelection` and `SchematicModule` still model a fixed-width device schematic with `T1..T5`, `R1..R2`, and two silent channels.
- `CurveSelectionModule` preset generation, transmitter color palettes, and transmitter naming still intentionally map to the current 5-slot UI contract.
- Curve restoration logic in `SchematicModule` still interprets "all transmitters selected" in terms of the current UI-width transmitter set.

### Algorithm / domain-specific boundaries

- `CurveSelectionModule::ComputePhaseSmtFromComputer(...)` still implements legacy `simmetry_5` coefficients and produces exactly five SMT outputs.
- Metrology-driven SMT inversion still assumes transmitter-indexed access compatible with the current 5-output SMT model.

### Compatibility fallbacks

- `ToolDataAccessView::GetTransmitterCount()` and `DataModule::GetAvailableTransmitterCount()` now share a single compatibility fallback constant rather than separate hardcoded `5` values.
- `DataModule::GetData()` remains publicly available as a legacy low-level API even though current upper-layer migration targets no longer require it.

## Next Immediate Actions

1. Decide which remaining 5Tx assumptions are true UI constraints versus temporary implementation shortcuts.
2. Decide whether SMT computer-side reconstruction should remain explicitly 5-output or be isolated behind a dedicated UI/algorithm boundary.
3. Decide whether `MeasurementView` should stay thin or grow semantic helper methods for chart/UI consumers.
4. Decide when `DataModule::GetData()` can be downgraded to a legacy/compatibility-only API.
5. Keep this document updated after each meaningful step.
