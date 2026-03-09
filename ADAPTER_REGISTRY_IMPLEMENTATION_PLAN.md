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
- [x] Confirmed the current in-tree legacy-surface inventory after the realtime migration: `DataModule::GetData()` and `DataModule::ParseRawFrame(...)` have no remaining internal consumers, `SerialModule::SetDataCallback(...)` / `RequestData(GP_TOOL_DATA& ...)` have no in-tree consumers and now act as compatibility wrappers, while `DataModule::SaveDataRange(...)` remains the main live canonical persistence boundary used by `ChartManager`
- [x] Localized the live canonical export seam in `DataModule` by extracting `SaveDataRange(...)` record serialization into `WriteCanonicalDataRange(...)`, keeping the current `GP_TOOL_DATA` export format unchanged while making the remaining boundary explicit
- [x] Confirmed the current export contract is intentionally canonical and `.bin`-oriented: `ChartManager` proposes/saves a `.bin` file name, `SaveDataRange(...)` writes raw `GP_TOOL_DATA` records only, and existing per-record metadata is available but not yet consulted during export
- [x] Captured the remaining design blockers before any adapter-aware export step: decide whether exported ranges must remain homogeneous by `ToolDataKind`, whether export should preserve original container/layout vs canonical `.bin`, and whether mixed ranges should be rejected, normalized, or split
- [x] Added `DataModule::TryGetRangeMetadataSummary(...)` as a narrow metadata-level helper for range analysis, and refined it so homogeneity is evaluated against full `ToolRecordMetadata` rather than `ToolDataKind` alone; this gives future export-policy work a stricter signal without changing the current export behavior
- [x] Introduced `DataRangeMetadataSummary` plus `DataModule::TryDescribeRangeMetadata(...)` as the new descriptor-based range-analysis layer, while keeping `TryGetRangeMetadataSummary(...)` as a compatibility wrapper; this gives Phase 7 export-policy work a clearer place to evolve without touching the current `.bin` export contract
- [x] Added `DataRangeExportAnalysis` plus `DataModule::TryAnalyzeExportRange(...)` as a policy-ready export-analysis seam: it now explicitly reports valid-range status, metadata summary, whether current canonical `.bin` export is possible, and whether a mixed/non-homogeneous range would require an adapter-aware export policy decision
- [x] Wired `ChartManager` range export flow to `TryAnalyzeExportRange(...)` for diagnostics only: current `SaveDataRange(...)` behavior and `.bin` output remain unchanged, but successful export now explicitly labels mixed-range saves as canonical compatibility export
- [x] Localized `ChartManager` range-export UI details behind small helper functions for suggested file naming and success-message generation, so a future export-mode selector can be added in one place without changing the current export behavior
- [x] Localized canonical extraction inside the remaining GP-level wrappers by routing `DataModule::ParseRawFrame(GP_TOOL_DATA& ...)` and `SerialModule::RequestData(GP_TOOL_DATA& ...)` through dedicated helper functions; behavior is unchanged, but their compatibility-wrapper role is now more explicit in code
- [x] Localized the `SerialModule` parsed-to-canonical callback bridge behind a dedicated dispatch helper, so `SetDataCallback(...)` is now structurally clearer as a legacy compatibility path alongside `SetParsedDataCallback(...)`
- [x] Added a narrow `MeasurementView::GetSignature()` helper and reduced direct raw-record access in `CurveSelectionModule` legacy SMT calculation paths; the old computer-side SMT logic still uses `GP_TOOL_DATA` internally where necessary, but upper-loop flow now leans more explicitly on `MeasurementView` / `ToolDataAccessView`
- [x] Narrowed the legacy SMT computer-side computation one step further: `CurveSelectionModule` now uses `ToolDataAccessView` as the primary compute input for the legacy SMT phase reconstruction helper, leaving raw `GP_TOOL_DATA` only as a thin compatibility wrapper around that logic
- [x] Removed duplicated temperature extraction loops from `CurveSelectionModule` by reusing existing `DataModule::GetTemperatureData(...)` helpers for both full-range and zoomed paths; this narrows upper-layer data walking without changing behavior
- [x] Removed duplicated temperature iteration from `DataModule::CalculateSingleCurveData(...)` as well, so temperature curve calculation now consistently reuses `GetTemperatureData(...)` before applying Y-range filtering
- [x] Localized repeated active/silent calculation setup in `DataModule.cpp` behind small local helper structs/functions (`DataIndexWindow`, `ActiveCalculationContext`, `SilentCalculationContext`, and setup helpers), reducing copy-pasted range/frequency/transmitter preparation across `CalculateRatioData`, `CalculateDifferenceData`, `CalculateAmplitudeData`, `CalculatePhaseData`, `CalculateSilentAmplitudeData`, and `CalculateSilentPhaseDifferenceData`
- [x] Added `DataModule::TryGetTemperatureRange(...)` as a narrow helper for visible temperature min/max over an index window, and switched `ChartManager` temperature X-axis paths to reuse it instead of manually recomputing the visible range in UI-layer code
- [x] Localized duplicated nearest-temperature-to-index lookup in `ChartManager.cpp` behind a small helper, so both temperature/X-axis cursor conversion paths now share the same closest-index selection logic
- [x] Localized the remaining `ChartManager` temperature/X-axis cursor-to-index flow behind a single helper that owns temperature-data fetch, visible index window selection, and closest-index resolution, preserving the existing caller-specific fallback behavior
- [x] Localized `ChartManager` temperature/X-axis render setup behind a small helper as well, so the temperature render branch now reuses one preparation path for `temperatureData` plus visible `minXData/maxXData` instead of rebuilding that state inline
- [x] Localized duplicated temperature/X-axis `ChartModule::DrawDualAxisChart(...)` overload bodies behind a shared local helper, keeping both public overloads as thin wrappers that differ only in legend rendering mode
- [x] Added a shared `ChartModule` helper for visible temperature-axis range extraction (`startIdx/endIdx/minTemp/maxTemp`) and reused it across temperature-aware grid/data render paths instead of repeating the same `xAxisData` min/max block inline
- [x] Added a small `ChartModule` helper for temperature-to-screen-X conversion and reused it across temperature-aware draw paths, removing repeated inline `(temp - minTemp) / (maxTemp - minTemp)` pixel mapping formulas without changing geometry
- [x] Localized repeated `ChartManager` dual-axis `plotRect/dataRect` geometry calculation behind a shared local helper and reused it across screen-to-data conversion, hit-testing, crosshair, and note-marker paths to reduce future coordinate drift risk
- [x] Localized repeated `ChartManager` `xRatio -> dataX` resolution for both linear and temperature axes behind small helpers, preserving the existing caller-specific fallback behavior while reducing duplicated axis conversion branches
- [x] Localized `ChartManager::DrawNoteMarkers(...)` X-coordinate resolution behind a small helper that owns note-index validation plus linear/temperature axis X mapping, keeping the existing note placement behavior intact
- [x] Localized repeated left-axis display/draw Y-range preparation in `ChartManager` behind shared helpers, reusing the same `+/-5%` display padding and `2%` draw padding across draw, cursor, and note-marker paths without changing alignment behavior
- [x] Generalized the `ChartManager` draw-range helper to cover both left and right Y axes in coordinate conversion, removing the last inline right-axis `2%` draw-padding branch without changing dual-axis value mapping
- [x] Localized repeated `ChartManager` curve-data snapshot preparation (`CalculateCurveData(...)` + `GetDataRanges(...)`) behind a shared helper and reused it across draw, cursor, and note-marker paths without changing range semantics
- [x] Performed a small follow-up cleanup in `ChartManager`: removed an unnecessary `const_cast` from the shared curve snapshot helper and switched the remaining inline `ConvertScreenToDataCoords(...)` draw-range block to the same shared drawn-axis helper
- [x] Localized repeated frame-axis visible X-range preparation in `ChartManager` behind a small helper, reusing the same `zoom/dataSize -> [minX,maxX]` logic across draw and note-marker paths without changing frame-axis behavior
- [x] Localized repeated frame-axis `DrawDualAxisChart(...)` dispatch in `ChartManager::DrawChartWithCurves(...)` behind a small helper, keeping dual-axis and single-axis behavior unchanged while reducing duplicate render-call setup
- [x] Localized repeated full axis-value range extraction in `ChartModule` dual-axis render paths behind a shared helper, reusing the same padded `FindMinMaxValues(...)` semantics for left/right axes and proportional right-axis zoom without changing rendering behavior
- [x] Completed the `ChartModule` axis-range helper rollout across ordinary dual-axis grid and left/right axis rendering paths, removing the remaining duplicated raw min/max + `5%` padding blocks without changing existing dual-axis visuals
- [x] Narrowed the live polling path in `SerialModule` so parsed-frame handling owns the timer flow and the GP-level callback branch is reached through one localized compatibility helper instead of direct inline `canonicalData` access in `OnTimer()`
- [x] Localized `DataModule` parsed-to-canonical materialization a bit further by routing parsed-frame append paths through shared extraction/metadata helpers, narrowing one more compatibility boundary without changing storage behavior or public APIs
- [x] Separated legacy UI `5Tx` count from fallback transmitter-count semantics via a shared constant path reused by `UIModule` and `SchematicSelection`, making the remaining `5Tx` assumption slightly more explicit without changing UI behavior
- [x] Localized repeated SMT source-selection logic in `CurveSelectionModule` behind shared helpers, so the `FROM_COMPUTER` legacy branch versus adapter/sonde SMT branch is chosen in one place for both phase and rho paths without changing SMT behavior
- [x] Localized repeated full-range `DataModule` delegation setup in `CurveSelectionModule` behind a shared helper, reducing one more repeated compatibility-wrapper pattern for ordinary curve calculations without changing curve data semantics
- [x] Localized repeated SMT preset/checkbox binding logic in `SchematicModule` behind shared helpers, so SMT mode restoration, checkbox synchronization, and preset activation reuse one UI binding path without changing dialog behavior
- [x] Localized repeated SMT preset assembly in `CurveSelectionModule` behind a shared helper, so the sonde-default SMT preset construction path is expressed once for both rho and phase presets without changing preset semantics
- [x] Localized repeated silent preset preparation in `CurveSelectionModule` behind small helpers, so base preset config setup and silent-curve finalization are reused without changing silent preset semantics
- [x] Localized repeated selection-driven SMT curve assembly in `CurveSelectionModule` behind a shared helper, so `RHO_SMT_ONLY` and `PHASE_SMT_ONLY` now reuse one curve-list construction path without changing available-curve semantics

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
- The current codebase no longer uses `GetData()`, `DataModule::ParseRawFrame(...)`, `SetDataCallback(...)`, or `RequestData(GP_TOOL_DATA& ...)` internally, so these surfaces can be treated as compatibility-only unless a new in-tree consumer appears.
- `SaveDataRange(...)` is currently the only clearly live in-tree feature that still depends on serializing canonical `GP_TOOL_DATA` records directly for export.
- The internal write loop for `SaveDataRange(...)` is now localized behind `WriteCanonicalDataRange(...)`, so a future adapter-aware export design can be introduced at one boundary instead of inlining canonical writes inside the public method.
- `ChartManager` currently hardcodes the range-export UX around `.bin` naming/filtering and calls `SaveDataRange(...)` directly, so an adapter-aware export change would require both a persistence policy and a UI-contract decision.
- Per-record `ToolRecordMetadata` is already stored in `DataModule`, so the main blocker for adapter-aware export is no longer missing metadata, but the absence of an agreed export policy for homogeneous vs mixed-format ranges and canonical vs original-container output.
- `DataModule` now also exposes a narrow range-level metadata summary helper, with homogeneous-range detection based on full `ToolRecordMetadata`; this is enough to support a future policy layer for export decisions without widening raw-storage access again.
- `DataModule` now has an explicit descriptor-based range-analysis seam (`DataRangeMetadataSummary` / `TryDescribeRangeMetadata(...)`), so future export-policy work can grow around a dedicated range-analysis object instead of adding more ad hoc boolean/out-parameter helpers.
- `DataModule` now also exposes an export-oriented range descriptor (`DataRangeExportAnalysis` / `TryAnalyzeExportRange(...)`), so future policy work can decide how to handle mixed ranges separately from the current canonical `.bin` compatibility path.
- Current UI inventory shows a single range-export entry point in `ChartManager::HandleSaveRange(...)`: the save dialog, export analysis, success/error messaging, and `SaveDataRange(...)` call are all localized there, so any future user-facing export-mode choice can be introduced in one place without broad UI churn.
- `ChartManager` range export flow now also has small local UI helpers for file-name suggestion and success-message composition, reducing the amount of inline logic that a future export-mode prompt would need to share.
- `CurveSelectionModule`, `SchematicSelection`, and `UIModule` now expose the current 5Tx-centric contract more explicitly, but the UI contract itself is still fixed-width.
- `DataModule::GetData()` still exists as a public low-level compatibility API; current inspection shows no remaining upper-layer consumers, but the API surface is still public and therefore not yet formally downgraded.
- `DataModule` calculation and helper paths are less coupled to raw storage details now, and narrow single-record access is available via `TryGetRecord(...)` / `TryGetRecordMetadata(...)`; loading and the public low-level API still expose canonical storage directly where appropriate.
- SMT computer-side reconstruction still has an explicit 5-output semantic model, which may need a separate design decision if non-5Tx SMT support is introduced.
- After the export groundwork, the next safest Phase 7 focus is no longer UI/export expansion but formal compatibility-only classification of the remaining low-level GP surfaces with no internal consumers: `DataModule::GetData()`, `DataModule::ParseRawFrame(...)`, `SerialModule::RequestData(GP_TOOL_DATA& ...)`, and `SerialModule::SetDataCallback(...)`.
- Wrapper internals for the GP-level conversion surfaces are now localized, so the remaining work on these APIs is primarily classification/guidance rather than structural cleanup.
- Preferred replacements for remaining compatibility-only surfaces are now clear:
  - `DataModule::GetData()` -> prefer `TryGetRecord(...)`, `TryGetRecordMetadata(...)`, `GetDataView(...)`, or `GetMeasurementView(...)` depending on consumer needs.
  - `DataModule::ParseRawFrame(...)` -> prefer `ParseRawFrameDetailed(...)`.
  - `SerialModule::RequestData(GP_TOOL_DATA& ...)` -> prefer `RequestData(ParsedToolFrame& ...)`.
  - `SerialModule::SetDataCallback(...)` -> prefer `SetParsedDataCallback(...)`.
- After this compatibility-only classification, the next safe in-tree narrowing step has started to shift upper-level legacy SMT flow away from direct `GP_TOOL_DATA*` checks where possible, using `MeasurementView` as the primary loop-level abstraction.
- That narrowing now extends into the legacy SMT helper itself: the compute core operates on `ToolDataAccessView`, so loop-level and helper-level code both depend less directly on raw canonical record access.
- The same narrowing pattern now also applies to temperature curves: `CurveSelectionModule` no longer duplicates temperature iteration logic and instead delegates that record walk back to `DataModule`.
- Inside `DataModule`, temperature handling is now also more internally consistent: `CalculateSingleCurveData(...)` reuses the dedicated temperature helper instead of maintaining a second manual iteration path.
- More broadly, `DataModule` calculation paths now share a localized preparation boundary for index-window and active/silent frequency/transmitter setup, making further narrowing work less likely to duplicate boilerplate.
- Temperature/X-axis handling is also a bit more centralized now: `ChartManager` still owns nearest-temperature-to-cursor/index selection, but visible temperature-range calculation has moved back into `DataModule`.
- Within `ChartManager`, the remaining temperature/X-axis cursor-to-index conversion logic is now also localized, reducing duplicate inline selection code while preserving the existing fallback behavior.
- The temperature/X-axis path in `ChartManager` is now effectively one localized compatibility helper instead of two partly duplicated call sites, which makes any future semantic move out of UI-layer code safer.
- The same is now true for the render-side setup: `ChartManager` no longer inlines temperature-axis data fetch/range preparation in its draw path, so temperature/X-axis concerns are more cleanly localized.
- In the lower chart-render layer, the two temperature-aware `DrawDualAxisChart(...)` overloads no longer duplicate the same render body; only the legend variant remains distinct at the wrapper level.
- `ChartModule` temperature render helpers now also share one visible temperature-range extraction path, which reduces repeated `xAxisData` window/min/max code across grid/data rendering functions.
- Temperature-aware chart drawing now also shares one local pixel-mapping helper for X coordinates, further narrowing repeated render math without changing the current plot geometry.
- In `ChartManager`, note/crosshair/coordinate paths now also depend on one localized dual-axis geometry helper instead of repeating the same margin math inline, which makes future note-alignment fixes less fragile.
- `ChartManager` axis conversion logic is now also more localized at the `xRatio -> dataX` level, so future temperature/linear cursor changes no longer need to edit multiple nearly identical branches.
- The same localization now extends into note-marker X placement, reducing one more duplicated linear-vs-temperature mapping branch near the note alignment code.
- Left-axis Y preparation in `ChartManager` is now also localized, which reduces one more source of potential drift between actual chart drawing, cursor conversion, and note marker placement.
- The remaining right-axis draw-range conversion branch now also reuses the same local helper, making dual-axis cursor/value conversion a bit more symmetric and less fragile.
- `ChartManager` now also shares one local curve-data snapshot builder, which narrows repeated data/range collection boilerplate around rendering, note placement, and coordinate conversion.
- The nearby cleanup pass also removed one avoidable constness escape and finished routing the remaining left-axis draw-range conversion block through the shared helper.
- Frame-axis visible X-range calculation in `ChartManager` is now also more localized, reducing one more repeated `zoom/dataSize` boilerplate block near rendering and note placement code.
- The remaining ordinary frame-axis draw dispatch in `DrawChartWithCurves(...)` is now also slightly narrower, so future draw-call argument changes no longer need to touch two nearly identical branches.
- `ChartModule` now also shares one helper for extracting full left/right axis value ranges, which narrows repeated dual-axis render preparation and keeps proportional right-axis zoom logic aligned across regular and temperature-aware paths.
- The same `ChartModule` helper now also feeds the ordinary dual-axis grid and axis-label render paths, so left/right render-range semantics are more consistent across the non-temperature chart layer as well.
- `SerialModule` live polling now also keeps parsed-frame flow slightly narrower: `OnTimer()` no longer inlines the GP callback conversion branch, so the remaining canonical compatibility callback path is more explicitly localized.
- Inside `DataModule`, parsed-frame append paths now also depend on one smaller helper layer for canonical extraction and metadata materialization, which makes the remaining low-level compatibility boundary a bit more explicit.
- The legacy `5Tx` UI contract is now also expressed through one shared constant path in the UI/schematic layer, which makes it easier to distinguish true UI constraints from separate runtime fallback semantics.
- `CurveSelectionModule` now also localizes the selection between legacy computer-side SMT reconstruction and direct adapter/sonde SMT access, reducing one more duplicated compatibility branch near the SMT calculation loops.
- Ordinary `CurveSelectionModule` calculation wrappers now also share one helper for preparing the full record range before delegating into `DataModule`, so the remaining upper-layer compatibility wrappers are a bit narrower and more consistent.
- `SchematicModule` now also routes SMT preset/checkbox UI state through one small binding layer, which reduces one more repeated legacy/UI-specific branch around SMT mode selection and preset activation.
- `CurveSelectionModule` now also shares one helper for constructing the default sonde-backed SMT presets, reducing one more repeated fixed-width preset-assembly branch tied to the current 5Tx UI contract.
- `CurveSelectionModule` silent preset branches now also share small helpers for preset base initialization and silent-curve finalization, reducing one more repeated special-case preset assembly path while preserving the current silent display semantics.
- `CurveSelectionModule` now also shares one helper for selection-driven SMT curve-list construction, reducing one more duplicated upper-layer SMT branch where the current mode is propagated into available curves.

## Current Stage Snapshot

- Current stage: late Phase 7 / compatibility-boundary narrowing and upper-layer render-path consolidation.
- Rough completion estimate for the currently scoped migration plan: about 91-95%.
- Main remaining work is no longer broad adapter plumbing, but finishing the highest-value compatibility/documentation boundaries and the last meaningful upper-layer/localization steps without changing behavior.

## Draft Phase 7 Export Policy

- Default compatibility behavior:
  - Keep the current `ChartManager -> SaveDataRange(...) -> canonical .bin` flow unchanged as the default export path.
  - Treat canonical `.bin` export as the explicit compatibility contract until a new UI choice or API is introduced.
- Homogeneous range policy:
  - If `TryAnalyzeExportRange(...)` reports a homogeneous range, the current canonical `.bin` export remains acceptable as the safe default.
  - A future adapter-aware export mode may optionally offer an adapter-specific/original-container export for homogeneous ranges, but only as an additive path.
- Mixed range policy:
  - If `TryAnalyzeExportRange(...)` reports a mixed range, do not silently invent a new container/layout.
  - Preferred safe future behavior is either:
    - reject adapter-aware export for the mixed range with a clear message, or
    - keep allowing canonical `.bin` compatibility export while labeling it explicitly as normalized canonical output.
  - Do not auto-split mixed ranges into multiple files unless the UI explicitly asks for that behavior.
- Recommended next implementation step:
  - First wire `ChartManager` export flow to `TryAnalyzeExportRange(...)` for diagnostics / messaging only, without changing the actual file format or successful export behavior.
  - Status: completed. `ChartManager` now uses export analysis to annotate mixed-range saves while preserving the current canonical `.bin` flow.
  - Only after that introduce a user-visible choice between compatibility canonical export and any future adapter-aware export mode.
  - Current UI inventory indicates that this choice can likely be localized to `ChartManager::HandleSaveRange(...)` as either a pre-save mode selector or a small follow-up prompt, without changing unrelated modules.
  - Supporting cleanup is now also done: suggested file-name generation and success-message composition are localized helpers, so a future selector can reuse them without duplicating UI strings/flow glue.
  - Current decision: keep diagnostics-only behavior for now and defer any user-visible export-mode selector until there is an explicit second export path to offer. Adding a selector earlier would change UX without yet providing real new functionality.

## Next Safe Phase 7 Focus

- Primary target:
  - Formally treat the remaining GP-level low-level APIs with no in-tree consumers as compatibility-only surfaces.
- Candidate surfaces:
  - `DataModule::GetData()`
  - `DataModule::ParseRawFrame(...)`
  - `SerialModule::RequestData(GP_TOOL_DATA& ...)`
  - `SerialModule::SetDataCallback(...)`
- Safe approach:
  - Prefer documentation, API-shape inventory, and local helper-based guidance over behavioral changes.
  - Do not remove or break these surfaces while they remain public compatibility entry points.
  - Only introduce stronger deprecation mechanics if they can be done without noisy build regressions for the current Visual Studio workflow.
  - Status: structural cleanup for `ParseRawFrame(GP_TOOL_DATA& ...)`, `RequestData(GP_TOOL_DATA& ...)`, and the `SetDataCallback(...)` dispatch path is complete; remaining work is to keep these surfaces explicitly compatibility-only in plan/API guidance while leaving behavior unchanged.

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
