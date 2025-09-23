# Design Document

## Overview

This design addresses two critical visualization issues in the AMK-Horizon-Chart application:
1. Graph flickering during zoom selection caused by excessive full window redraws
2. Incorrect Y-axis range calculation that causes data lines to merge with axes

The solution involves optimizing mouse event handling and implementing proper Y-axis padding calculations.

## Architecture

### Current Architecture Issues
- `WM_MOUSEMOVE` handler triggers full window invalidation on every mouse movement
- `FindMinMaxValues` method uses exact data bounds without visual padding
- No distinction between lightweight selection drawing and full chart redraws

### Proposed Architecture Changes
- Separate lightweight selection rectangle drawing from full chart redraws
- Implement Y-axis padding calculation in the data range computation
- Optimize mouse event handling to minimize unnecessary operations

## Components and Interfaces

### 1. Mouse Event Handling (ChartManager)

**Current Implementation:**
```cpp
void HandleMouseMove(HWND hWnd, int x, int y) {
    // Updates selection coordinates
    // Calls InvalidateRect(hWnd, NULL, FALSE) - causes flickering
}
```

**New Implementation:**
```cpp
void HandleMouseMove(HWND hWnd, int x, int y) {
    // Updates selection coordinates
    // Directly draws selection rectangle without full invalidation
    // Uses optimized drawing techniques
}
```

### 2. Y-Axis Range Calculation (ChartModule)

**Current Implementation:**
```cpp
void FindMinMaxValues(..., float& minVal, float& maxVal) {
    // Finds exact min/max from data
    // No padding applied
}
```

**New Implementation:**
```cpp
void FindMinMaxValues(..., float& minVal, float& maxVal) {
    // Finds exact min/max from data
    // Applies 5% padding to both ends
    // Handles edge cases (identical values, very small ranges)
}
```

### 3. Selection Rectangle Drawing (ChartModule)

**New Method:**
```cpp
void DrawZoomRectangleOptimized(HDC hdc, const RECT& zoomRect, const RECT& chartArea) {
    // Draws selection rectangle using optimized techniques
    // Minimizes screen updates
    // Preserves underlying chart content
}
```

## Data Models

### ZoomRange Structure (Enhanced)
```cpp
struct ZoomRange {
    double minX, maxX, minY, maxY;
    bool isZoomed;
    
    // New: Track if range has padding applied
    bool hasPadding;
    double originalMinY, originalMaxY;  // Store original bounds
};
```

### Y-Axis Padding Configuration
```cpp
struct YAxisConfig {
    static constexpr double PADDING_PERCENTAGE = 0.05;  // 5%
    static constexpr double MIN_RANGE = 2.0;            // Minimum range for identical values
};
```

## Error Handling

### Mouse Event Error Handling
- Validate chart boundaries before processing mouse events
- Handle cases where mouse moves outside chart areas
- Gracefully handle rapid mouse movements

### Y-Axis Calculation Error Handling
- Handle division by zero when all values are identical
- Manage extremely large or small data ranges
- Validate padding calculations don't create invalid ranges

## Testing Strategy

### Unit Tests
1. **Y-Axis Padding Tests:**
   - Test normal data ranges with 5% padding
   - Test identical values creating artificial range
   - Test very small ranges expanding to minimum
   - Test very large ranges maintaining proportional padding

2. **Mouse Event Tests:**
   - Test selection rectangle drawing without full redraws
   - Test boundary conditions (selection outside chart area)
   - Test rapid mouse movements

### Integration Tests
1. **End-to-End Zoom Tests:**
   - Test complete zoom workflow without flickering
   - Test zoom on both charts independently
   - Test zoom reset functionality

2. **Performance Tests:**
   - Measure frame rate during selection operations
   - Verify no performance regression in chart drawing
   - Test with large datasets

### Visual Tests
1. **Flickering Verification:**
   - Manual testing of smooth selection rectangle drawing
   - Verification that chart background remains stable during selection

2. **Y-Axis Padding Verification:**
   - Visual confirmation of adequate spacing around data lines
   - Testing with various data ranges and zoom levels

## Implementation Approach

### Phase 1: Y-Axis Padding Implementation
1. Modify `FindMinMaxValues` method to apply padding
2. Update all callers to use padded values
3. Test with various data scenarios

### Phase 2: Mouse Event Optimization
1. Modify `HandleMouseMove` to avoid full invalidation
2. Implement optimized selection rectangle drawing
3. Update `HandleMouseUp` to trigger single redraw

### Phase 3: Integration and Testing
1. Integrate both fixes
2. Comprehensive testing across all scenarios
3. Performance validation

## Performance Considerations

### Memory Usage
- Minimal additional memory for padding calculations
- No significant impact on existing data structures

### CPU Usage
- Reduced CPU usage due to fewer full window redraws
- Slight increase in Y-axis calculation complexity (negligible)

### Rendering Performance
- Significant improvement in selection smoothness
- Maintained chart rendering performance
- Optimized drawing operations for selection rectangle