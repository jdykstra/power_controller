# Copilot Instructions for Power Controller

## Project Overview
This is an Arduino-based audio power controller that manages multi-zone audio system power sequencing, IR remote control, and refrigerator automation. The system controls power relays with precise timing sequences to prevent electrical issues.

## Architecture & State Machine
- **State-based design**: Three main states (`SYS_DOWN`, `SYS_LIVING_ROOM_UP`, `SYS_OFFICE_UP`)
- **State transition functions**: Each state has its own handler function (`stateSysDown()`, `stateLivingRoomUp()`, `stateOfficeUp()`)
- **Central state manager**: `setSysState()` coordinates all transitions
- **Command processors**: `cmdLivingRoomPower()` and `cmdOfficePower()` handle input commands

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
- Control: `PIN_CMD_OUT` (3) for refrigerator, `PIN_SPKR_SW` (12) for speaker switching
- Inputs: `PIN_PWR_SW` (7), `PIN_OVER_SW` (8), `PIN_IR_IN` (2)

## IR Communication
- Uses IRLib2 with NEC protocol
- **Critical**: `markExcess` compensation for IR repeater timing issues
- Commands defined as hex constants (`CODE_LIVING_ROOM_POWER`, etc.)
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
- Separate delay sequences for different state transitions

## Legacy Code Notes
The file contains commented legacy office system code that's no longer active but preserved for reference. When modifying:
- Focus on living room power functions and refrigerator control
- Office power functions are maintained for compatibility but may not be used
- Speaker switch (`PIN_SPKR_SW`) is currently normally closed (N.C.)

## Key Integration Points
- **Refrigerator control**: `PIN_CMD_OUT` HIGH = off, LOW = on
- **IR communication**: NEC protocol with empirically-determined timing compensation
- **Power management**: Three-stage relay sequencing with specific timing requirements