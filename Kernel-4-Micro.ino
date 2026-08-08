// ==== Kernel 4 Micro (optimized) ====
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

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (bufPos > 0) {
        lineBuf[bufPos] = '\0';
        handleLine(lineBuf);
        bufPos = 0;
      }
    } else if (bufPos < BUF_LEN - 1) {
      lineBuf[bufPos++] = c;
    }
  }
}

int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  c = toupper(c);
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

int hexByte(const char* s) {
  if (!s || !s[0] || !s[1]) return -1;
  int hi = hexNibble(s[0]);
  int lo = hexNibble(s[1]);
  if (hi < 0 || lo < 0) return -1;
  return (hi << 4) | lo;
}

long parsePin(char* s) {
  if (s[0] == 'D' || s[0] == 'd') return atol(s + 1);
  if (s[0] == 'A' || s[0] == 'a') return A0 + atol(s + 1);
  return atol(s);
}

uint8_t splitWords(char* line, char* words[], uint8_t maxWords) {
  uint8_t n = 0;
  char* p = line;
  while (*p) {
    while (*p == ' ') p++;
    if (!*p) break;
    if (n >= maxWords) break;
    words[n++] = p;
    while (*p && *p != ' ') p++;
    if (*p) *p++ = '\0';
  }
  return n;
}

void toUpperStr(char* s) {
  for (; *s; s++) *s = toupper(*s);
}

void delayUsLong(unsigned long us) {
  while (us > 16000UL) {
    delay(16);
    us -= 16000UL;
  }
  delayMicroseconds((unsigned int)us);
}

void doAction(char* words[], uint8_t n, char* reply) {
  reply[0] = '\0';
  if (n == 0) return;

  char cmd[8];
  strncpy(cmd, words[0], sizeof(cmd) - 1);
  cmd[sizeof(cmd) - 1] = '\0';
  toUpperStr(cmd);

  if (strcmp(cmd, "HELP") == 0) {
    strcpy(reply, "HELP");
  }
  else if (strcmp(cmd, "RS") == 0 && n >= 2) {
    int pin = parsePin(words[1]);
    snprintf(reply, REPLY_LEN, "PIN %s = %d", words[1], digitalRead(pin));
  }
  else if (strcmp(cmd, "RA") == 0 && n >= 2) {
    int pin = parsePin(words[1]);
    snprintf(reply, REPLY_LEN, "PIN %s = %d", words[1], analogRead(pin));
  }
  else if (strcmp(cmd, "WD") == 0 && n >= 3) {
    int pin = parsePin(words[1]);
    int val = atoi(words[2]);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, val ? HIGH : LOW);
    strcpy(reply, "OK");
  }
  else if (strcmp(cmd, "WA") == 0 && n >= 3) {
    int pin = parsePin(words[1]);
    int val = constrain(atoi(words[2]), 0, 255);
    pinMode(pin, OUTPUT);
    analogWrite(pin, val);
    strcpy(reply, "OK");
  }
  else if (strcmp(cmd, "MODE") == 0 && n >= 3) {
    int pin = parsePin(words[1]);
    toUpperStr(words[2]);
    if (strcmp(words[2], "OUT") == 0) pinMode(pin, OUTPUT);
    else if (strcmp(words[2], "IN") == 0) pinMode(pin, INPUT);
    else if (strcmp(words[2], "PU") == 0) pinMode(pin, INPUT_PULLUP);
    else { strcpy(reply, "ERR mode"); return; }
    strcpy(reply, "OK");
  }
  else if (strcmp(cmd, "HEX") == 0 && n >= 2) {
    char* rec = words[1];
    if (rec[0] != ':') { strcpy(reply, "ERR hex: must start with ':'"); return; }
    rec++;

    int len = hexByte(rec);
    if (len < 0) { strcpy(reply, "ERR hex: bad length"); return; }
    rec += 2;

    int addrHi = hexByte(rec);
    if (addrHi < 0) { strcpy(reply, "ERR hex: bad address"); return; }
    rec += 2;
    int addrLo = hexByte(rec);
    if (addrLo < 0) { strcpy(reply, "ERR hex: bad address"); return; }
    rec += 2;

    int type = hexByte(rec);
    if (type < 0) { strcpy(reply, "ERR hex: bad record type"); return; }
    rec += 2;

    if (len > 32) { strcpy(reply, "ERR hex: record too long (max 32)"); return; }
    uint8_t data[32];
    int sum = len + addrHi + addrLo + type;
    for (int i = 0; i < len; i++) {
      int b = hexByte(rec);
      if (b < 0) { strcpy(reply, "ERR hex: bad data byte"); return; }
      data[i] = (uint8_t)b;
      sum += b;
      rec += 2;
    }

    int cksum = hexByte(rec);
    if (cksum < 0) { strcpy(reply, "ERR hex: bad checksum"); return; }
    sum += cksum;
    if ((sum & 0xFF) != 0) { strcpy(reply, "ERR hex: checksum mismatch"); return; }

    int addr = (addrHi << 8) | addrLo;

    if (type == 0x01) { strcpy(reply, "OK HEX EOF"); return; }
    if (type != 0x00) {
      snprintf(reply, REPLY_LEN, "OK HEX type=%02X ignored", type);
      return;
    }

    for (int i = 0; i < len; i++) {
      int pin = addr + i;
      int val = data[i];
      pinMode(pin, OUTPUT);
      if (val == 0 || val == 1) digitalWrite(pin, val);
      else analogWrite(pin, val);
    }
    snprintf(reply, REPLY_LEN, "OK HEX addr=%02X len=%d", addr, len);
  }
  else if (strcmp(cmd, "DUS") == 0 && n >= 2) {
    delayUsLong(atol(words[1]));
    strcpy(reply, "OK");
  }
  else if (strcmp(cmd, "DMS") == 0 && n >= 2) {
    delay(atol(words[1]));
    strcpy(reply, "OK");
  }
  else {
    strcpy(reply, "ERR unknown/format. Type HELP.");
  }
}

void printHelp() {
  Serial.println(F("=== Kernel - commands ==="));
  Serial.println(F("RS <pin>          read digital pin state, e.g. RS D11"));
  Serial.println(F("RA <pin>          read analog value 0-1023, e.g. RA A0"));
  Serial.println(F("WD <pin> <0|1>    write digital pin, e.g. WD D13 1"));
  Serial.println(F("WA <pin> <0-255>  write PWM (pins 3,5,6,9,10,11), e.g. WA D9 128"));
  Serial.println(F("MODE <pin> <m>    set pin mode: IN, OUT, PU, e.g. MODE D2 PU"));
  Serial.println(F("HEX :LLAAAATTDDCC real Intel HEX record, e.g. HEX :01000D00FFF3"));
  Serial.println(F("DUS <us>          blocking delay, microseconds, e.g. DUS 40"));
  Serial.println(F("DMS <ms>          blocking delay, milliseconds, e.g. DMS 2"));
  Serial.println(F("HELP              show this message"));
  Serial.println(F("Pin format: Dn digital (D0-D13), An analog (A0-A7)"));
  Serial.println(F("Chaining: CMD1 : CMD2  = sequential"));
  Serial.println(F("          CMD1 x CMD2  = batched / best-effort parallel"));
}

void handleLine(char* line) {
  char* words[MAX_WORDS];
  uint8_t total = splitWords(line, words, MAX_WORDS);
  if (total == 0) return;

  uint8_t subStart[MAX_SUBCMDS];
  uint8_t subLen[MAX_SUBCMDS];
  uint8_t subCount = 0;
  char sep = 0;      
  bool overflow = false; 

  uint8_t curStart = 0;
  for (uint8_t i = 0; i < total; i++) {
    bool isSep = false;
    char c = 0;
    if (strcmp(words[i], ":") == 0) { c = ':'; isSep = true; }
    else if ((words[i][0] == 'x' || words[i][0] == 'X') && words[i][1] == '\0') {
      c = 'x'; isSep = true;
    }

    if (isSep) {
      if (sep != 0 && sep != c) {
        Serial.println(F("ERR mixed ':' and 'x' separators in one line"));
        return;
      }
      sep = c;
      if (subCount < MAX_SUBCMDS) {
        subStart[subCount] = curStart;
        subLen[subCount] = i - curStart;
        subCount++;
      } else {
        overflow = true;
      }
      curStart = i + 1;
    }
  }
  if (subCount < MAX_SUBCMDS) {
    subStart[subCount] = curStart;
    subLen[subCount] = total - curStart;
    subCount++;
  } else {
    overflow = true;
  }

  if (overflow) {
    char msg[REPLY_LEN];
    snprintf(msg, REPLY_LEN, "ERR too many chained sub-commands (max %d)", MAX_SUBCMDS);
    Serial.println(msg);
    return;
  }

  if (sep == 0 || subCount == 1) {
    char reply[REPLY_LEN];
    doAction(&words[subStart[0]], subLen[0], reply);
    if (strcmp(reply, "HELP") == 0) printHelp();
    else Serial.println(reply);
    return;
  }

  if (sep == ':') {
    for (uint8_t i = 0; i < subCount; i++) {
      if (subLen[i] == 0) continue;
      char reply[REPLY_LEN];
      doAction(&words[subStart[i]], subLen[i], reply);
      if (strcmp(reply, "HELP") == 0) printHelp();
      else Serial.println(reply);
    }
  } else {
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
    else { snprintf(reply, REPLY_LEN, "OK x%d", okCount); Serial.println(reply); }
  }
}
