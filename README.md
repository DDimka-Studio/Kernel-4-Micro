# Kernel 4 Micro (optimized)

Kernel 4 Micro is a lightweight, optimized kernel designed to transform your Arduino (e.g., Nano, Uno) into a versatile I/O controller. By running this sketch, you can execute hardware commands directly via Serial port (RealPort) without the need for recompiling or reflashing the firmware for every new task. It essentially turns your Arduino into a remote-controlled peripheral.

The project is specifically optimized for memory-constrained AVR microcontrollers (like the ATmega328P) by avoiding `String` usage and dynamic memory allocation.

## Features

*   **Pin Control:** Digital/analog reading and digital/PWM writing.
*   **Flexible Modes:** Supports `IN`, `OUT`, and `INPUT_PULLUP`.
*   **Intel HEX:** Capability to process data in standard `.hex` format to quickly set states for multiple pins.
*   **Command Chaining:** Execute multiple commands in a single line (sequentially or "batched").
*   **Efficiency:** Command parsing is performed directly in `char` buffers without dynamic allocation.

## Command List

| Command | Description | Example |
| :--- | :--- | :--- |
| `RS <pin>` | Read digital pin state | `RS D11` |
| `RA <pin>` | Read analog value (0-1023) | `RA A0` |
| `WD <pin> <0\|1>` | Write digital pin | `WD D13 1` |
| `WA <pin> <0-255>` | Write PWM (pins 3, 5, 6, 9, 10, 11) | `WA D9 128` |
| `MODE <pin> <m>` | Set pin mode (`IN`, `OUT`, `PU`) | `MODE D2 PU` |
| `HEX :LLAAAATTDDCC` | Write via Intel HEX format | `HEX :01000D00FFF3` |
| `DUS <us>` | Blocking delay (microseconds) | `DUS 40` |
| `DMS <ms>` | Blocking delay (milliseconds) | `DMS 2` |
| `HELP` | Show help message | `HELP` |

**Pin Format:** `D0`-`D13` (digital), `A0`-`A7` (analog).

## Command Chaining

The sketch supports grouping commands in a single line:

1.  **Sequential Execution (`:`)**: Commands are executed one after another, and the response for each is printed immediately.
    *   Example: `WD D13 1 : DUS 1000 : WD D13 0`
2.  **Batched ("Parallel") Execution (`x`)**: All hardware actions are executed consecutively as fast as possible without intermediate serial output. A final summary report is printed after all operations finish.
    *   Example: `WA D9 100 x WA D10 100 x WA D11 100`

## Technical Details

*   **RAM Optimization:** No `String` class usage. Parsing is implemented using pointers within a fixed `char` buffer.
*   **Limitations:** Maximum number of chained segments is 28.
*   **Intel HEX:** Supports standard format `:LLAAAATTDD..DDCC`. Type `0x01` (EOF) terminates processing, `0x00` writes data to pins starting from address `AAAA`.
