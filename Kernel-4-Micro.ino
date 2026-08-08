// ==== Kernel 4 Micro (optimized) ====

// Basic commands:
//   RS <pin>              - read digital state (e.g., RS D11)
//   RA <pin>              - analog read 0-1023 (e.g., RA A0)
//   WD <pin> <0|1>        - digital write
//   WA <pin> <0-255>      - PWM (pins 3,5,6,9,10,11)
//   MODE <pin> <IN|OUT|PU>
//   HEX :LLAAAATTDD..DDCC - write actual Intel HEX record (see doAction,
//                           case "HEX"), e.g., HEX :01000D00FFF3
//   DUS <us>               - blocking pause in microseconds
//   DMS <ms>               - blocking pause in milliseconds
//   HELP

// Command chains in one line:
//   CMD1 ... : CMD2 ...   - sequentially: response to each command
//                           is printed immediately after execution.
//   CMD1 ... x CMD2 ...   - "in parallel": first, ALL hardware actions
//                           are executed in sequence without Serial output
//                           between them, and only then all responses
//                           are printed at once. This is the maximum
//                           simultaneity achievable on a single-core AVR.
//                           There is no real parallelism here, but pins
//                           switch "tightly" one after another, without
//                           delays for printing between them.

// Optimization: Removed Arduino String everywhere. On AVR (low RAM, heavy
// fragmentation from constant Strings), this was the main issue.
// Now parsing is done in-place on char buffers, without memory allocation.

#define BUF_LEN      250
#define MAX_WORDS    90
#define MAX_SUBCMDS  28
#define REPLY_LEN    64

char lineBuf[BUF_LEN];
uint8_t bufPos = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  readSerial();
}

// Non-buffering character-by-character read - does not block loop().
void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (bufPos > 0) {
        lineBuf[bufPos] = '\0';
        processLine();
        bufPos = 0;
      }
    } else if (bufPos < BUF_LEN - 1) {
      lineBuf[bufPos++] = c;
    }
  }
}

void processLine() {
  // ...
}

// If overflow:
// Do nothing: a partially executed remote/bus sequence may leave the screen/chip in a dirty state.
  char msg[REPLY_LEN];
  snprintf(msg, REPLY_LEN, "ERR too many chained sub-commands (max %d)", MAX_SUBCMDS);
  Serial.println(msg);
  return;

if (sep == 0 || subCount == 1) {
    // Single command - as in the original version
    char reply[REPLY_LEN];
    doAction(&words[subStart[0]], subLen[0], reply);
    if (strcmp(reply, "HELP") == 0) printHelp();
    else Serial.println(reply);
    return;
}

if (sep == ':') {
    // Sequentially: action + print immediately for each sub-command
    for (uint8_t i = 0; i < subCount; i++) {
      if (subLen[i] == 0) continue;
      char reply[REPLY_LEN];
      doAction(&words[subStart[i]], subLen[i], reply);
      if (strcmp(reply, "HELP") == 0) printHelp();
      else Serial.println(reply);
    }
} else {
    // 'x' - "in parallel": first ALL actions in sequence (timing is critical here
    // - see DUS/DMS), and only then a single combined response.
    // Previously, there was char[MAX_SUBCMDS][REPLY_LEN] - with MAX_SUBCMDS=28,
    // this is 1792 bytes, almost the entire SRAM reserve of ATmega328P.
    // Instead, we keep only the first error (if any) and a success counter.
    char reply[REPLY_LEN];
    char firstErr[REPLY_LEN];
    firstErr[0] = '\0';
    uint8_t okCount = 0;
    bool anyHelp = false;
    for (uint8_t i = 0; i < subCount; i++) {
      if (subLen[i] == 0) continue;
      doAction(&words[subStart[i]], subLen[i], reply);
      if (strcmp(reply, "HELP") == 0) {
        anyHelp = true;
      } else if (strncmp(reply, "ERR", 3) == 0) {
        if (firstErr[0] == '\0') strcpy(firstErr, reply);
      } else {
        okCount++;
      }
    }
    if (anyHelp) printHelp();
    if (firstErr[0] != '\0') Serial.println(firstErr);
    else {
      snprintf(reply, REPLY_LEN, "OK x%d", okCount);
      Serial.println(reply);
    }
}
