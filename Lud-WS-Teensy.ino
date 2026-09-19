// =====================================================================
// Lud-WS-Teensy -- Teensy 4.1 drum sampler + sequencer (LWS node)
// ---------------------------------------------------------------------
// REV 7 (allineata al Router v0.0.4)
//   - LWS_BAUD = 1 Mbps su UART hardware (Serial2)
//   - Pin Serial2 Teensy 4.1: pin 7 = RX, pin 8 = TX
//   - Partner: Pico 2 Router su Serial1 (UART0 HW, GP0/GP1)
//
// MAPPA CHIAVI PARAM (payload [target='T'][key][value]):
//   'R' run          'S' stop          'Y' clock src (0=int, 1=ext)
//   'b' bpm (20..250) 'N' pattern (0..15)
//   'd' vol drum      'm' vol mono      's' vol stereo   (0..255)
//   'Z' status query (echo locale su USB, non sul bus LWS)
//
// PIN MAP (Teensy 4.1):
//   CS_CV             -> 2
//   CS_LFO_           -> 15     (mantenuto, non pilotato)
//   CLK_TEENSY_IN_    -> 34
//   TRIG1_OUT_        -> 22
//   LEV_SHIFT_OE_     -> 33
//   SD_CS_            -> 21
//   LWS UART          -> Serial2 (RX=7, TX=8)
// =====================================================================

#include <Arduino.h>

// ---------------------------------------------------------------------
// Config nodo (PRIMA degli include LWS)
// ---------------------------------------------------------------------
#define MCU_ID      'T'
#define LWS_SERIAL  Serial2          // Teensy 4.1: pin 7=RX, 8=TX
#define LWS_DEBUG   Serial           // USB

#define LWS_BAUD    1000000UL        // 1 Mbps — bus LWS

// ---------------------------------------------------------------------
// Include protocollo condiviso
// ---------------------------------------------------------------------
#include "serial_protocol.h"         // CMD_*, CRC-8/ATM, parser, helpers i32
#include "comunicazioni_mcu.h"       // callbacks, pending ACK, lws_mcu_poll()

#include "FS.h"
#include "SD.h"
#include "SPI.h"

// ---------------------------------------------------------------------
// Pin map
// ---------------------------------------------------------------------
#define CS_CV            2
#define CS_LFO_          15
#define CLK_TEENSY_IN_   34
#define TRIG1_OUT_       22
#define LEV_SHIFT_OE_    33
#define SD_CS_           21

// Destinatario degli echo di stato
#ifndef LWS_OUT_TARGET
#define LWS_OUT_TARGET   ID_DISPLAY
#endif

// ---------------------------------------------------------------------
// Tipi / stato globale
// ---------------------------------------------------------------------
enum Channel { CHANNEL_A = 0, CHANNEL_B = 1 };

unsigned long seqTime     = 1200;
unsigned long clockTime   = 0;
unsigned long trigEndTime = 0;

byte seq1Running = 0;
int  contaSeq    = 0;
int  contaSeq2   = 0;
int  seq1Ptn     = 0;
int  seq2Ptn     = 0;
byte seq1Sync    = 0;       // 0 = int, 1 = Teensy ext clock

byte drumLev  = 200;
byte trkMLev  = 200;
byte trkSTLev = 200;

boolean bufferVref = true;
boolean gain2x     = false;

// ---------------------------------------------------------------------
// Debug helper (USB)
// ---------------------------------------------------------------------
void dbg(const String &msg) { LWS_DEBUG.println("[T] " + msg); }

// Forward decl
void setBpm(byte bpm);
void dumpStatus();

// ---------------------------------------------------------------------
// DAC MCP49x1 (SPI)
// ---------------------------------------------------------------------
void dacOut(unsigned short data, Channel chan, int ssIn) {
  data &= 0x3ff;
  digitalWrite(ssIn, LOW);
  uint16_t out = (chan << 15) | (bufferVref << 14) |
                 ((gain2x) << 13) | (1 << 12) | (data << 2);
  SPI.transfer((out & 0xff00) >> 8);
  SPI.transfer(out & 0xff);
  digitalWrite(ssIn, HIGH);
}

#include "table.h"
#include "SDfunc.h"

// =====================================================================
// CALLBACKS LWS
// =====================================================================
void on_param(char target, char key, uint8_t value) {
  if (target != MCU_ID) return;   // non è per noi

  LWS_DEBUG.printf("[LWS] PARAM key=%c val=%u\n", key, value);

  switch (key) {

    // -------- trasporto --------------------------------------------
    case 'R':
      seq1Running = 1;
      break;
    case 'S':
      seq1Running = 0;
      contaSeq = contaSeq2 = 0;
      break;

    // -------- sorgente clock ---------------------------------------
    case 'Y':
      seq1Sync = (value != 0) ? 1 : 0;
      break;

    // -------- BPM (con echo del valore effettivo post-clamp) -------
    case 'b': {
      setBpm(value);
      byte actualBpm = (byte)(240000UL / seqTime);
      lws_send_param(LWS_OUT_TARGET, 'b', actualBpm);
      break;
    }

    // -------- pattern ----------------------------------------------
    case 'N': {
      if (value > 15) value = 0;
      seq1Ptn = seq2Ptn = value;
      lws_send_param(LWS_OUT_TARGET, 'N', value);
      break;
    }

    // -------- volumi -----------------------------------------------
    case 'd':
      drumLev = value;
      lws_send_param(LWS_OUT_TARGET, 'd', drumLev);
      break;
    case 'm':
      trkMLev = value;
      lws_send_param(LWS_OUT_TARGET, 'm', trkMLev);
      break;
    case 's':
      trkSTLev = value;
      lws_send_param(LWS_OUT_TARGET, 's', trkSTLev);
      break;

    // -------- status query (solo su USB) ---------------------------
    case 'Z':
      dumpStatus();
      break;

    default:
      LWS_DEBUG.printf("[LWS] key sconosciuta: %c\n", key);
      break;
  }
}

void on_error(const char *msg) {
  LWS_DEBUG.printf("[LWS] ERROR: %s\n", msg);
}

// Stub MIDI (il Router attualmente NON inoltra MIDI al Teensy)
void on_cc(uint8_t cc, uint8_t val) {
  LWS_DEBUG.printf("[LWS] CC %u=%u\n", cc, val);
}
void on_note(uint8_t onoff, uint8_t pitch, uint8_t vel) {
  LWS_DEBUG.printf("[LWS] NOTE %s p=%u v=%u\n",
                   onoff ? "ON" : "OFF", pitch, vel);
}
void on_bend(int32_t bend) {
  LWS_DEBUG.printf("[LWS] BEND %ld\n", (long)bend);
}

// =====================================================================
// SETUP
// =====================================================================
void setup() {
  LWS_DEBUG.begin(115200);
  LWS_DEBUG.println("=== Lud-WS-Teensy boot (rev7, LWS node) ===");
  LWS_DEBUG.printf("MCU_ID = '%c', LWS_BAUD = %lu\n",
                   MCU_ID, (unsigned long)LWS_BAUD);

  LWS_SERIAL.begin(LWS_BAUD);        // 1 Mbps sul bus LWS

  // Callbacks LWS
  lws_set_callbacks(on_param, on_error);
  lws_set_midi_callbacks(on_cc, on_note, on_bend);

  // SPI + DAC
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);

  pinMode(LEV_SHIFT_OE_, OUTPUT); digitalWrite(LEV_SHIFT_OE_, LOW);
  pinMode(TRIG1_OUT_,    OUTPUT); digitalWrite(TRIG1_OUT_,    HIGH);
  pinMode(CS_CV,         OUTPUT);
  pinMode(CS_LFO_,       OUTPUT);
  pinMode(CLK_TEENSY_IN_,INPUT);

  // SD (opzionale: se non monta, si usano i pattern hard-coded in table.h)
  if (!SD.begin(SD_CS_)) dbg("SD mount FAILED");
  else                   dbg("SD mount OK");
  // listDir(SD, "/", 0);   // DBG
  // SDinfo();              // DBG
  // loadMem(SD);           // carica pattern da SD
  // writeMem();            // scrive pattern su SD (demo)

  delay(500);
  digitalWrite(LEV_SHIFT_OE_, HIGH);
  setBpm(120);
  dbg("Setup completo, in ascolto sul bus LWS a 1 Mbps");
}

// =====================================================================
// SEQUENCER
// =====================================================================
void seq1Run() {
  dacOut((int)seq1Arr[seq1Ptn][contaSeq], CHANNEL_A, CS_CV);
  contaSeq++;
  if (contaSeq > seq1LungArr[seq1Ptn]) contaSeq = 0;
}

void seq2Run() {
  dacOut((int)seq2Arr[seq2Ptn][contaSeq2], CHANNEL_B, CS_CV);
  contaSeq2++;
  if (contaSeq2 > seq2LungArr[seq2Ptn]) contaSeq2 = 0;
}

// =====================================================================
// BPM
// =====================================================================
void setBpm(byte bpm) {
  if (bpm < 20)  bpm = 20;
  if (bpm > 250) bpm = 250;
  seqTime = 240000UL / bpm;
  dbg("setBpm " + String(bpm) + " -> seqTime=" + String(seqTime));
}

// =====================================================================
// DUMP STATO (log locale su USB)
// =====================================================================
void dumpStatus() {
  LWS_DEBUG.print  ("seq1Run=");  LWS_DEBUG.print(seq1Running);
  LWS_DEBUG.print  (" seq1Ptn="); LWS_DEBUG.print(seq1Ptn);
  LWS_DEBUG.print  (" seq2Ptn="); LWS_DEBUG.print(seq2Ptn);
  LWS_DEBUG.print  (" seq1Sync=");LWS_DEBUG.print(seq1Sync);
  LWS_DEBUG.print  (" seqTime="); LWS_DEBUG.print(seqTime);
  LWS_DEBUG.print  (" drum=");    LWS_DEBUG.print(drumLev);
  LWS_DEBUG.print  (" trkM=");    LWS_DEBUG.print(trkMLev);
  LWS_DEBUG.print  (" trkST=");   LWS_DEBUG.println(trkSTLev);
}

// =====================================================================
// CLOCK
// =====================================================================
byte togTeensyClk = 0;
byte togTrigOut   = 0;

void clockFunc() {
  // ----- clock esterno (Teensy) ------------------------------------
  if (digitalRead(CLK_TEENSY_IN_) == HIGH && seq1Sync == 1 &&
      togTeensyClk == 0 && seq1Running == 1) {
    digitalWrite(TRIG1_OUT_, LOW);
    seq1Run();
    seq2Run();
    togTeensyClk = 1;
  }
  if (digitalRead(CLK_TEENSY_IN_) == LOW && seq1Sync == 1 && togTeensyClk == 1) {
    digitalWrite(TRIG1_OUT_, HIGH);
    togTeensyClk = 0;
  }

  // ----- clock interno ---------------------------------------------
  if (millis() >= clockTime && seq1Running == 1 && togTrigOut == 0) {
    togTrigOut  = 1;
    trigEndTime = millis() + (seqTime / 32);
    if (seq1Sync == 0) {
      digitalWrite(TRIG1_OUT_, LOW);
      seq1Run();
      seq2Run();
    }
    clockTime = millis() + (seqTime / 16);
  }
  if (millis() >= trigEndTime && togTrigOut == 1) {
    if (seq1Sync == 0) digitalWrite(TRIG1_OUT_, HIGH);
    togTrigOut = 0;
  }
}

// =====================================================================
// LOOP
// =====================================================================
void loop() {
  lws_mcu_poll();   // RX + retry pending ACK (da comunicazioni_mcu.h)
  clockFunc();
}