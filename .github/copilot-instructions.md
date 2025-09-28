# Copilot Instructions for Power Controller

## Project Overview
This is an Arduino-based audio power controller that manages audio system power sequencing and refrigerator automation. The system controls power relays with precise timing sequences to prevent electrical issues and includes serial command interface and physical switch inputs.

## Architecture & State Machine
- **Simplified state-based design**: Two main states (`SYS_DOWN`, `SYS_UP`)
- **State transition functions**: `setSystemState()` and `setRefrigState()` handle transitions
- **Refrigerator control**: Dedicated `setRefrigState()` function with override protection

## Critical Hardware Patterns
### Power Sequencing
```cpp
// Always use delays between relay activations
digitalWrite(PIN_PWR_A, ON);
delay(3*1000);
digitalWrite(PIN_PWR_B, ON);
delay(1*1000);
digitalWrite(PIN_PWR_C, ON);
```
**Why**: Hardware interactions between controller power supply and relay picking require sequential activation.

### Pin Definitions
- Power outputs: `PIN_PWR_A` (9), `PIN_PWR_B` (10), `PIN_PWR_C` (11), `PIN_PILOT` (LED_BUILTIN)
- Control: `PIN_CMD_OUT` (3) for refrigerator (HIGH = off, LOW = on)
- Inputs: `PIN_PWR_SW` (7) for power switch, `PIN_REFRIG_OVERRIDE` (6) for refrigerator override

## Serial Command Interface
- **Control Commands**: Require "set " prefix (e.g., `set system on`, `set refrigerator off`)
- **Status Commands**: `status system` or `status refrigerator` return "on"/"off"
- **Response Format**: "OK" for success, "on"/"off" for status, "ERR <message>" for errors
- **Processing**: `processSerialCommands()` delegates to `processSetCommand()` and `processStatusCommand()`
- **Action Parsing**: `parseOnOffAction()` converts "on"/"off" strings to numeric values
- **Action Parsing**: `parseOnOffAction()` converts "on"/"off" strings to numeric values

## Refrigerator Override Logic
- **Override Switch**: `PIN_REFRIG_OVERRIDE` (LOW = active/override on)
- **Behavior**: When override is active, ignore requests to turn refrigerator off
- **Transition Handling**: Override activation (LOW) turns refrigerator on if it was off; deactivation (HIGH) leaves state unchanged
- **Monitoring**: Continuous check in `loop()` with `prevOverrideState` tracking

## Development Patterns
### Naming Conventions
- Constants: `ALL_CAPS` with descriptive prefixes (`PIN_`, `CODE_`, `SYS_`)
- Functions: camelCase with descriptive verb prefixes (`cmd`, `state`, `set`)
- Hardware states: `ON`/`OFF` macros instead of `HIGH`/`LOW` for clarity

### Debugging
- Serial output at 9600 baud with F() macro for flash string storage
- Command responses provide feedback for all operations
- Silent operation except for command responses

### Error Prevention
- Input pullups on switches to prevent floating inputs
- No switch debouncing on power switch (power sequencing time > bounce period)
- Override switch prevents unwanted refrigerator power-off

### Coding Style
- If a block only contains a single statement, remove the "{}" braces
- Put the opening curly bracket of a function on the same line as the function declaration

## Key Integration Points
- **Refrigerator control**: `PIN_CMD_OUT` with state tracking in `currentRefrigState`
- **Serial communication**: Command parsing with space-separated tokens via `processSerialCommands()`, `processSetCommand()`, and `processStatusCommand()`
- **Power management**: Three-stage relay sequencing with specific timing requirements
- **Switch inputs**: Physical switches for manual control with state change detection
- **Refrigerator control**: `PIN_CMD_OUT` HIGH = off, LOW = on
- **Power management**: Three-stage relay sequencing with specific timing requirements