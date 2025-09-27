# Copilot Instructions for Power Controller

## Project Overview
This is an Arduino-based audio power controller that manages multi-zone audio system power sequencing, IR remote control, and refrigerator automation. The system controls power relays with precise timing sequences to prevent electrical issues.

## Architecture & State Machine
- **Simplified state-based design**: Two main states (`SYS_DOWN`, `SYS_UP`)
- **State transition functions**: `stateSysDown()` and `stateSysUp()` handle transitions
- **Central state manager**: `setSysState()` coordinates all transitions
- **Command processor**: `cmdSysPower()` handles power toggle commands

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
- Power outputs: `PIN_PWR_A` (9), `PIN_PWR_B` (10), `PIN_PWR_C` (11) 
- Control: `PIN_CMD_OUT` (3) for refrigerator
- Inputs: `PIN_PWR_SW` (7) for power switch, `PIN_IR_IN` (2) for IR receiver

## IR Communication
- Uses IRLib2 with NEC protocol
- **Critical**: `markExcess` compensation for IR repeater timing issues
- Commands: `CODE_SYSTEM_POWER` for system power, refrigerator on/off codes
- Always call `myReceiver.enableIRIn()` after processing commands

## Development Patterns
### Naming Conventions
- Constants: `ALL_CAPS` with descriptive prefixes (`PIN_`, `CODE_`, `SYS_`)
- Functions: camelCase with descriptive verb prefixes (`cmd`, `state`, `set`)
- Hardware states: `ON`/`OFF` macros instead of `HIGH`/`LOW` for clarity

### Debugging
- Serial output at 9600 baud with F() macro for flash string storage
- IR debug: prints protocol and hex value for all received codes
- State transitions announced via Serial

### Error Prevention
- Input pullups on switches to prevent floating inputs
- No switch debouncing on power switch (power sequencing time > bounce period)

## Key Integration Points
- **Refrigerator control**: `PIN_CMD_OUT` HIGH = off, LOW = on
- **IR communication**: NEC protocol with empirically-determined timing compensation
- **Power management**: Three-stage relay sequencing with specific timing requirements