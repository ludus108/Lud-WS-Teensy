// =====================================================================
// Lud-WS-Teensy -- Teensy 4.1 drum sampler + sequencer + LWS node
// ---------------------------------------------------------------------
// REV 13 (2026-10-01)
//   - songArr esteso a [16][4][256]: tipo, numero, kit, rev
//     Indici kit e rev diretti 0..15 (nessuna sentinella).
//     Ogni slot ha sempre kit e rev espliciti.
//   - SONGxx.BIN v0x02 (1029 B). Fallback v0x01 (517 B) con
//     upscale a kit=0, rev=0.
//   - advanceBlock() applica kit e rev dello slot al boundary.
//     Fix bug: setRev usa kitArr[9][kitNum], non kitNum-1.
//   - startStop() song mode: applica kit/rev dello slot 0.
//   - Nuove funzioni songBegin/songChunk/songEnd per ricezione
//     bulk song dal Display (da collegare in comunicazioni_mcu.h).
//   - Include tutti i fix rev 12.1 (swing, pan 9 voci, BD mono,
//     HH/OH condivisi).
//
// Layout righe seqArr / seqFillArr:
//   0=BD 1=SD 2=HH 3=OH 4=HH2 5=CLAP 6=PERC1 7=PERC2 8=PERC3
// Codifica celle: 0=silenzio, 1..3=velocity soft/medium/accent
//
// File SD:
//   PTN00.BIN .. PTN15.BIN           v0x02  9x64  581 B
//                                    v0x01  9x32  293 B (legacy)
//   PTN_FILL00.BIN .. PTN_FILL15.BIN v0x01  9x16  149 B
//   SONG00.BIN .. SONG15.BIN         v0x02  4x256 1029 B
//                                    v0x01  2x256 517 B (legacy)
//
// LWS: MCU_ID='T', Serial3 @ 1 Mbps (TX=14, RX=15)
// =====================================================================

#include <Arduino.h>

#define MCU_ID      'T'
#define LWS_SERIAL  Serial3
#define LWS_DEBUG   Serial
#define LWS_BAUD    1000000UL

#ifndef LWS_OUT_TARGET
#define LWS_OUT_TARGET 'R'
#endif

#include "serial_protocol.h"
#include "comunicazioni_mcu.h"

#include <Audio.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <EEPROM.h>

// ---------------------------------------------------------------------
// Pin map
// ---------------------------------------------------------------------
#define SERIALFLASH_CS   6
#define SD_CS            10
// LWS : Serial3 = TX14, RX15
// SPI : MISO=12, MOSI=11, SCK=13
// Serial6 (TX24/RX25) libero

// =====================================================================
// AUDIO GRAPH
// =====================================================================
AudioPlaySerialflashRaw soundBd, soundSd, soundHhC, soundHhO, soundHh2,
                         soundClap, soundPerc1, soundPerc2, soundPerc3;
AudioPlaySdWav          playSdWav, playSdWav2;
AudioAnalyzePeak        peak1;

AudioAmplifier          ampBd, ampSD, ampHh, ampOH, ampHH2,
                        ampClap, ampPerc1, ampPerc2, ampPerc3;

AudioEffectEnvelope     envHh, envOH;

AudioMixer4             mixOutL, mixOutR, mixDrum1L, mixDrum1R,
                        mixDrum2L, mixDrum2R, mixOutMono,
                        mixOutRev, mixOutDly, mixRevInt, mixHH;

AudioFilterStateVariable filter, filter2;
AudioEffectDelay         preDelay;
AudioEffectFreeverbStereo revInt;

AudioOutputI2SQuad      outQuad;
AudioControlSGTL5000    audioCtrl1, audioCtrl2;

// =====================================================================
// CONNESSIONI
// =====================================================================
AudioConnection bd0(soundBd, 0, ampBd, 0);
AudioConnection bd1(ampBd, 0, mixDrum1L, 0);
AudioConnection bd2(ampBd, 0, mixDrum1R, 0);

AudioConnection sd0(soundSd, 0, ampSD, 0);
AudioConnection sd1(ampSD, 0, mixDrum1L, 2);
AudioConnection sd2(ampSD, 0, mixDrum1R, 2);
AudioConnection sd3(ampSD, 0, mixRevInt, 0);
AudioConnection sd4(ampSD, 0, mixOutRev, 0);
AudioConnection sd5(ampSD, 0, mixOutDly, 0);

AudioConnection hhc0(soundHhC, 0, ampHh, 0);
AudioConnection hhc1(ampHh,   0, envHh, 0);
AudioConnection hhc2(envHh,   0, mixHH, 0);
AudioConnection hho0(soundHhO, 0, ampOH, 0);
AudioConnection hho1(ampOH,   0, envOH, 0);
AudioConnection hho2(envOH,   0, mixHH, 1);
AudioConnection hhm1(mixHH, 0, mixDrum1L, 1);
AudioConnection hhm2(mixHH, 0, mixDrum1R, 1);

AudioConnection cp0(soundClap, 0, ampClap, 0);
AudioConnection cp1(ampClap, 0, mixDrum1L, 3);
AudioConnection cp2(ampClap, 0, mixDrum1R, 3);
AudioConnection cp3(ampClap, 0, mixRevInt, 1);
AudioConnection cp4(ampClap, 0, mixOutRev, 1);
AudioConnection cp5(ampClap, 0, mixOutDly, 1);

AudioConnection hh20(soundHh2, 0, ampHH2, 0);
AudioConnection hh21(ampHH2, 0, mixDrum2L, 0);
AudioConnection hh22(ampHH2, 0, mixDrum2R, 0);

AudioConnection pe10(soundPerc1, 0, ampPerc1, 0);
AudioConnection pe11(ampPerc1, 0, mixDrum2L, 1);
AudioConnection pe12(ampPerc1, 0, mixDrum2R, 1);
AudioConnection pe13(ampPerc1, 0, mixRevInt, 2);

AudioConnection pe20(soundPerc2, 0, ampPerc2, 0);
AudioConnection pe21(ampPerc2, 0, mixDrum2L, 2);
AudioConnection pe22(ampPerc2, 0, mixDrum2R, 2);

AudioConnection pe30(soundPerc3, 0, ampPerc3, 0);
AudioConnection pe31(ampPerc3, 0, mixDrum2L, 3);
AudioConnection pe32(ampPerc3, 0, mixDrum2R, 3);
AudioConnection pe33(ampPerc3, 0, mixRevInt, 3);

AudioConnection pd(mixRevInt, 0, preDelay, 0);
AudioConnection ri(preDelay, 0, revInt, 0);
AudioConnection filt1(revInt, 0, filter, 0);
AudioConnection filt2(revInt, 1, filter2, 0);

AudioConnection sd11(playSdWav, 0, peak1, 0);
AudioConnection sd12(playSdWav, 1, mixOutMono, 0);
AudioConnection sd13(playSdWav2, 0, mixOutMono, 1);
AudioConnection sd14(playSdWav2, 1, mixOutMono, 2);

AudioConnection mxL1(mixOutMono, 0, mixOutL, 0);
AudioConnection mxL2(mixDrum1L, 0, mixOutL, 1);
AudioConnection mxL3(mixDrum2L, 0, mixOutL, 2);
AudioConnection mxL4(filter, 0, mixOutL, 3);
AudioConnection mxR1(mixOutMono, 0, mixOutR, 0);
AudioConnection mxR2(mixDrum1R, 0, mixOutR, 1);
AudioConnection mxR3(mixDrum2R, 0, mixOutR, 2);
AudioConnection mxR4(filter2, 0, mixOutR, 3);

AudioConnection o1(mixOutRev, 0, outQuad, 0);
AudioConnection o2(mixOutDly, 0, outQuad, 1);
AudioConnection o3(mixOutL, 0, outQuad, 2);
AudioConnection o4(mixOutR, 0, outQuad, 3);

// =====================================================================
// COSTANTI TRACCE
// =====================================================================
const int trackMaxNum = 5;
const char* trackSdArr[5]  = {"TGTRACK1.wav","TGTRACK2.wav","TGTRACK3.wav",
                              "TGTRACK4.wav","TGTRACK5.wav"};
const char* trackSdBArr[5] = {"TGTRACK1B.wav","TGTRACK2B.wav","TGTRACK3B.wav",
                              "TGTRACK4B.wav","TGTRACK5B.wav"};
int trackTempoArr[5] = {0,0,0,0,0};
int trackSdNum = 0;

// =====================================================================
// STATO SEQUENCER / MIXER
// =====================================================================
unsigned long seqTime    = 120;
unsigned long swingTime  = 30;
unsigned long swingMax   = 0;
unsigned long preSeqTime = 120;

byte seqRunning = 0;
byte sync       = 0;
long sdTotTime  = 0;
byte togSdWave  = 1;
byte togPeak    = 0;

int bpm       = 120;
int kitNum    = 0;
int preKitNum = 0;

byte bdMute=0, sdMute=0, clapMute=0, hhMute=0, hh2Mute=0, percMute=1;

int track2exist = 0;
int ptnNum      = 0;
int prePtnNum   = 0;
byte autoStepZero = 1;

int voiceSel = 1;
int tempRevPreset = 0;

// ---- Playback state -------------------------------------------------
byte  playMode   = 0;
byte  curSection = 0;
byte  preSection = 0;
byte  fillNum    = 0;
byte  preFillNum = 0;
bool  playingFill = false;
bool  fillPending = false;
int   songNum    = 0;
int   songPos    = 0;
int   curSubStep = 0;
byte  curRev     = 0;              // preset REV attivo (indice 0..15)
byte* curBlock[9];

// Ricezione song dal Display (bulk)
bool  songRxActive = false;
int   songRxNum    = 0;
int   songRxLen    = 0;
int   songRxOffset = 0;

elapsedMillis msecs, trigMsecs, trigOutMs;
File myFile;

#include "lista.h"
#include "mem.h"

// =====================================================================
// HELPERS
// =====================================================================
void bpmToTime(int bpm2) {
  preSeqTime = floor((60000 / bpm2) / 4);
  swingMax   = floor((preSeqTime / 2) - 2);
  if (swingTime > swingMax) swingTime = swingMax;
}

void cpuUsage() {
  LWS_DEBUG.printf("CPU=%0.1f%% max=%0.1f%% mem=%u max=%u\n",
                   AudioProcessorUsage(), AudioProcessorUsageMax(),
                   AudioMemoryUsage(), AudioMemoryUsageMax());
}

void getTempoTrack(int num) {
  int ms = trackTempoArr[num];
  int sec = ms / 1000;
  int min = sec / 60;
  sec %= 60;
  LWS_DEBUG.printf("track %d: %d:%02d\n", num, min, sec);
}

const float velGain[4] = {0.0f, 0.4f, 0.7f, 1.0f};

inline void setVel(AudioAmplifier &amp, int v) {
  if (v < 1) v = 1;
  if (v > 3) v = 3;
  amp.gain(velGain[v]);
}

void fxOutLev(float lev) {
  AudioNoInterrupts();
  mixOutL.gain(3, lev);
  mixOutR.gain(3, lev);
  AudioInterrupts();
}

void drumLev(float lev) {
  AudioNoInterrupts();
  mixOutL.gain(1, lev); mixOutL.gain(2, lev);
  mixOutR.gain(1, lev); mixOutR.gain(2, lev);
  AudioInterrupts();
}

// Applica il pan alla voce indicata.
//   id = 0..8 (0=BD, 1=SD, 2=HH, 3=OH, 4=HH2, 5=CLAP, 6=PERC1, 7=PERC2, 8=PERC3)
//   val = 0..100 (0=hard right, 10=center, 20=hard left, 51..100=mono)
// Casi speciali:
//   - id 0 (BD): sempre mono, val ignorato.
//   - id 3 (OH): reindirizzato a id 2 (HH). OH condivide il canale.
void setPan(int id, int val) {
  if (id == 0) {
    int lvl = kitLevArr[0][kitNum];
    AudioNoInterrupts();
    mixDrum1L.gain(0, levArr[lvl]);
    mixDrum1R.gain(0, levArr[lvl]);
    AudioInterrupts();
    return;
  }
  if (id == 3) id = 2;

  int leftVal, rightVal;
  int tempLev = kitLevArr[id][kitNum];

  if (val > 50) {
    leftVal = tempLev;
    rightVal = tempLev;
  } else {
    if (val > 20) val = 20;
    if (val < 0)  val = 0;
    leftVal  = map(val, 0, 20, 0, tempLev);
    rightVal = map(val, 0, 20, tempLev, 0);
  }

  AudioNoInterrupts();
  switch (id) {
    case 1: mixDrum1L.gain(2, levArr[leftVal]); mixDrum1R.gain(2, levArr[rightVal]); break;
    case 2: mixDrum1L.gain(1, levArr[leftVal]); mixDrum1R.gain(1, levArr[rightVal]); break;
    case 4: mixDrum2L.gain(0, levArr[leftVal]); mixDrum2R.gain(0, levArr[rightVal]); break;
    case 5: mixDrum1L.gain(3, levArr[leftVal]); mixDrum1R.gain(3, levArr[rightVal]); break;
    case 6: mixDrum2L.gain(1, levArr[leftVal]); mixDrum2R.gain(1, levArr[rightVal]); break;
    case 7: mixDrum2L.gain(2, levArr[leftVal]); mixDrum2R.gain(2, levArr[rightVal]); break;
    case 8: mixDrum2L.gain(3, levArr[leftVal]); mixDrum2R.gain(3, levArr[rightVal]); break;
  }
  AudioInterrupts();
}

void setRev(int id) {
  if (id < 0)  id = 0;
  if (id > 15) id = 15;
  AudioNoInterrupts();
  revInt.roomsize(levArr[revPresetArr[0][id]]);
  revInt.damping(levArr[revPresetArr[1][id]]);
  filter.frequency(cutArr[revPresetArr[2][id]]);
  filter.resonance(resArr[revPresetArr[3][id]]);
  preDelay.delay(0, delayArr[revPresetArr[4][id]]);
  if (revPresetArr[7][id] == 1) {
    filter2.frequency(cutArr[revPresetArr[5][id]]);
    filter2.resonance(resArr[revPresetArr[6][id]]);
  }
  if (revPresetArr[7][id] == 0) {
    filter2.frequency(cutArr[revPresetArr[2][id]]);
    filter2.resonance(resArr[revPresetArr[3][id]]);
  }
  AudioInterrupts();
  curRev = id;
}

// Applica pan/level di tutte le voci per il kit corrente.
// OH viene saltato perché condivide out e pan con HH.
void mixLev() {
  AudioNoInterrupts();
  for (int i = 0; i < 9; i++) {
    if (i == 3) continue;
    setPan(i, kitPanArr[i][kitNum]);
  }
  AudioInterrupts();
}

// =====================================================================
// SD I/O — Pattern / Fill / Song
// =====================================================================

static const uint8_t PTN_MAGIC    = 0x50;
static const uint8_t PTN_V2       = 0x02;
static const uint8_t PTN_V1       = 0x01;
static const uint8_t PTN_ROWS     = 9;
static const uint8_t PTN_COLS_V2  = 64;
static const uint8_t PTN_COLS_V1  = 32;

static const uint8_t FILL_MAGIC   = 0x46;
static const uint8_t FILL_VERSION = 0x01;
static const uint8_t FILL_ROWS    = 9;
static const uint8_t FILL_COLS    = 16;

static const uint8_t SONG_MAGIC   = 0x53;
static const uint8_t SONG_V1      = 0x01;
static const uint8_t SONG_V2      = 0x02;
static const int     SONG_SLOTS   = 256;

static uint8_t crc8_atm(const uint8_t *data, size_t len) {
  uint8_t crc = 0x00;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x80) ? ((crc << 1) ^ 0x07) : (crc << 1);
  }
  return crc;
}

// ---------------------------------------------------------------------
// Pattern
// ---------------------------------------------------------------------
static bool leggiPtnBin(int w) {
  char fname[13];
  snprintf(fname, sizeof(fname), "PTN%02d.BIN", w);

  File f = SD.open(fname, FILE_READ);
  if (!f) return false;

  size_t sz = f.size();
  size_t expectedV2 = 4 + PTN_ROWS * PTN_COLS_V2 + 1;
  size_t expectedV1 = 4 + PTN_ROWS * PTN_COLS_V1 + 1;
  if (sz != expectedV2 && sz != expectedV1) { f.close(); return false; }

  uint8_t buf[600];
  if ((size_t)f.read(buf, sz) != sz) { f.close(); return false; }
  f.close();

  if (buf[0] != PTN_MAGIC || buf[2] != PTN_ROWS) return false;
  if (crc8_atm(buf, sz - 1) != buf[sz - 1]) return false;

  if (buf[1] == PTN_V2 && buf[3] == PTN_COLS_V2) {
    memcpy(&seqArr[w][0][0], buf + 4, PTN_ROWS * PTN_COLS_V2);
    return true;
  }
  if (buf[1] == PTN_V1 && buf[3] == PTN_COLS_V1) {
    memset(&seqArr[w][0][0], 0, sizeof(seqArr[w]));
    for (int v = 0; v < PTN_ROWS; v++)
      memcpy(&seqArr[w][v][0], buf + 4 + v * PTN_COLS_V1, PTN_COLS_V1);
    return true;
  }
  return false;
}

static bool scriviPtnBin(int w) {
  size_t total = 4 + PTN_ROWS * PTN_COLS_V2 + 1;
  uint8_t buf[600];
  buf[0] = PTN_MAGIC;
  buf[1] = PTN_V2;
  buf[2] = PTN_ROWS;
  buf[3] = PTN_COLS_V2;
  memcpy(buf + 4, &seqArr[w][0][0], PTN_ROWS * PTN_COLS_V2);
  buf[total - 1] = crc8_atm(buf, total - 1);

  char fname[13];
  snprintf(fname, sizeof(fname), "PTN%02d.BIN", w);
  SD.remove(fname);
  File f = SD.open(fname, FILE_WRITE);
  if (!f) return false;
  size_t wr = f.write(buf, total);
  f.close();
  return (wr == total);
}

// ---------------------------------------------------------------------
// Fill
// ---------------------------------------------------------------------
static bool leggiFillBin(int w) {
  char fname[20];
  snprintf(fname, sizeof(fname), "PTN_FILL%02d.BIN", w);

  File f = SD.open(fname, FILE_READ);
  if (!f) return false;

  size_t total = 4 + FILL_ROWS * FILL_COLS + 1;
  if (f.size() != total) { f.close(); return false; }

  uint8_t buf[160];
  if ((size_t)f.read(buf, total) != total) { f.close(); return false; }
  f.close();

  if (buf[0] != FILL_MAGIC || buf[1] != FILL_VERSION) return false;
  if (buf[2] != FILL_ROWS  || buf[3] != FILL_COLS)    return false;
  if (crc8_atm(buf, total - 1) != buf[total - 1])     return false;

  memcpy(&seqFillArr[w][0][0], buf + 4, FILL_ROWS * FILL_COLS);
  return true;
}

static bool scriviFillBin(int w) {
  size_t total = 4 + FILL_ROWS * FILL_COLS + 1;
  uint8_t buf[160];
  buf[0] = FILL_MAGIC;
  buf[1] = FILL_VERSION;
  buf[2] = FILL_ROWS;
  buf[3] = FILL_COLS;
  memcpy(buf + 4, &seqFillArr[w][0][0], FILL_ROWS * FILL_COLS);
  buf[total - 1] = crc8_atm(buf, total - 1);

  char fname[20];
  snprintf(fname, sizeof(fname), "PTN_FILL%02d.BIN", w);
  SD.remove(fname);
  File f = SD.open(fname, FILE_WRITE);
  if (!f) return false;
  size_t wr = f.write(buf, total);
  f.close();
  return (wr == total);
}

// ---------------------------------------------------------------------
// Song — v0x02 (4 campi) con fallback v0x01 (2 campi)
// ---------------------------------------------------------------------
static bool leggiSongBin(int s) {
  char fname[13];
  snprintf(fname, sizeof(fname), "SONG%02d.BIN", s);

  File f = SD.open(fname, FILE_READ);
  if (!f) return false;

  size_t sz = f.size();
  size_t totalV1 = 4 + SONG_SLOTS * 2 + 1;   // 517
  size_t totalV2 = 4 + SONG_SLOTS * 4 + 1;   // 1029
  if (sz != totalV1 && sz != totalV2) { f.close(); return false; }

  uint8_t buf[1040];
  if ((size_t)f.read(buf, sz) != sz) { f.close(); return false; }
  f.close();

  if (buf[0] != SONG_MAGIC) return false;
  if (crc8_atm(buf, sz - 1) != buf[sz - 1]) return false;

  songLen[s] = buf[2];
  if (songLen[s] > 255) songLen[s] = 255;

  memset(songArr[s], 0, sizeof(songArr[s]));

  if (buf[1] == SONG_V1 && sz == totalV1) {
    for (int i = 0; i < SONG_SLOTS; i++) {
      songArr[s][0][i] = buf[4 + i * 2];
      songArr[s][1][i] = buf[4 + i * 2 + 1];
      // kit = 0, rev = 0 (già azzerati)
    }
  } else if (buf[1] == SONG_V2 && sz == totalV2) {
    for (int i = 0; i < SONG_SLOTS; i++) {
      songArr[s][0][i] = buf[4 + i * 4];
      songArr[s][1][i] = buf[4 + i * 4 + 1];
      songArr[s][2][i] = buf[4 + i * 4 + 2];
      songArr[s][3][i] = buf[4 + i * 4 + 3];
    }
  } else {
    return false;
  }
  return true;
}

static bool scriviSongBin(int s) {
  size_t total = 4 + SONG_SLOTS * 4 + 1;
  uint8_t buf[1040];
  buf[0] = SONG_MAGIC;
  buf[1] = SONG_V2;
  buf[2] = songLen[s];
  buf[3] = 0;
  for (int i = 0; i < SONG_SLOTS; i++) {
    buf[4 + i * 4]     = songArr[s][0][i];
    buf[4 + i * 4 + 1] = songArr[s][1][i];
    buf[4 + i * 4 + 2] = songArr[s][2][i];
    buf[4 + i * 4 + 3] = songArr[s][3][i];
  }
  buf[total - 1] = crc8_atm(buf, total - 1);

  char fname[13];
  snprintf(fname, sizeof(fname), "SONG%02d.BIN", s);
  SD.remove(fname);
  File f = SD.open(fname, FILE_WRITE);
  if (!f) return false;
  size_t wr = f.write(buf, total);
  f.close();
  return (wr == total);
}

// ---------------------------------------------------------------------
// Orchestrazione
// ---------------------------------------------------------------------
void leggiSd() {
  memset(seqArr,     0, sizeof(seqArr));
  memset(seqFillArr, 0, sizeof(seqFillArr));
  memset(songArr,    0, sizeof(songArr));
  memset(songLen,    0, sizeof(songLen));

  int okP = 0, okF = 0, okS = 0;
  for (int w = 0; w < 16; w++) if (leggiPtnBin (w)) okP++;
  for (int w = 0; w < 16; w++) if (leggiFillBin(w)) okF++;
  for (int s = 0; s < 16; s++) if (leggiSongBin(s)) okS++;
  LWS_DEBUG.printf("[SD] PTN:%d/16 FILL:%d/16 SONG:%d/16\n", okP, okF, okS);
}

void scriviSd() {
  int okP = 0, okF = 0, okS = 0;
  for (int w = 0; w < 16; w++) if (scriviPtnBin (w)) okP++;
  for (int w = 0; w < 16; w++) if (scriviFillBin(w)) okF++;
  for (int s = 0; s < 16; s++) if (scriviSongBin(s)) okS++;
  LWS_DEBUG.printf("[SD] scritti PTN:%d FILL:%d SONG:%d\n", okP, okF, okS);
}

// =====================================================================
// RICEZIONE SONG DAL DISPLAY (bulk)
// ---------------------------------------------------------------------
// Queste funzioni sono chiamate da comunicazioni_mcu.h quando arrivano
// i comandi CMD_SONG_BEGIN / CMD_SONG_CHUNK / CMD_SONG_END.
//
// Protocollo:
//   CMD_SONG_BEGIN  'q'  [song][len_lo][len_hi]
//   CMD_SONG_CHUNK  'x'  [off_lo][off_hi][4 byte per slot] x N
//   CMD_SONG_END    'k'  [crc8]
//
// Gli offset sono in slot (0..255), ogni chunk invia N slot completi
// (4 byte ciascuno). Il CRC è calcolato su tutti i 1024 byte del
// payload (256 x 4) nell'ordine tipo,numero,kit,rev.
// =====================================================================

void songBegin(int song, int len) {
  if (song < 0 || song > 15) return;
  if (len < 0 || len > 255) return;
  songRxActive = true;
  songRxNum    = song;
  songRxLen    = len;
  songRxOffset = 0;
  memset(songArr[song], 0, sizeof(songArr[song]));
  LWS_DEBUG.printf("[SONG] begin song=%d len=%d\n", song, len);
}

void songChunk(int off, const uint8_t *data, int numSlots) {
  if (!songRxActive) return;
  if (off < 0 || off + numSlots > SONG_SLOTS) return;
  for (int i = 0; i < numSlots; i++) {
    int idx = off + i;
    songArr[songRxNum][0][idx] = data[i * 4];
    songArr[songRxNum][1][idx] = data[i * 4 + 1];
    songArr[songRxNum][2][idx] = data[i * 4 + 2];
    songArr[songRxNum][3][idx] = data[i * 4 + 3];
  }
  songRxOffset = off + numSlots;
}

bool songEnd(uint8_t crc) {
  if (!songRxActive) return false;

  // Calcola CRC sui 1024 byte effettivi
  uint8_t calc = 0x00;
  for (int i = 0; i < SONG_SLOTS; i++) {
    uint8_t b[4] = {
      songArr[songRxNum][0][i],
      songArr[songRxNum][1][i],
      songArr[songRxNum][2][i],
      songArr[songRxNum][3][i]
    };
    for (int k = 0; k < 4; k++) {
      calc ^= b[k];
      for (uint8_t bit = 0; bit < 8; bit++)
        calc = (calc & 0x80) ? ((calc << 1) ^ 0x07) : (calc << 1);
    }
  }

  bool ok = (calc == crc);
  if (ok) {
    songLen[songRxNum] = songRxLen;
    LWS_DEBUG.printf("[SONG] end song=%d ok\n", songRxNum);
  } else {
    LWS_DEBUG.printf("[SONG] end CRC mismatch (calc=%02X recv=%02X)\n",
                     calc, crc);
  }
  songRxActive = false;
  return ok;
}

// =====================================================================
// PLAYBACK — gestione blocchi
// =====================================================================
void setCurrentBlock(byte type, byte idx) {
  for (int v = 0; v < 9; v++) {
    if (type <= 3) curBlock[v] = &seqArr[idx][v][type * 16];
    else           curBlock[v] = &seqFillArr[idx][v][0];
  }
}

void advanceBlock() {
  if (playMode == 0) {
    fillNum = preFillNum;

    if (playingFill) {
      playingFill = false;
      curSection  = preSection;
      setCurrentBlock(curSection, ptnNum);
      lws_send_param(LWS_OUT_TARGET, 'h', 0);
    } else if (fillPending) {
      fillPending = false;
      playingFill = true;
      setCurrentBlock(4, fillNum);
      lws_send_param(LWS_OUT_TARGET, 'h', 1);
    } else {
      curSection = preSection;
      setCurrentBlock(curSection, ptnNum);
    }

    if (prePtnNum != ptnNum) {
      ptnNum = prePtnNum;
      if (!playingFill) setCurrentBlock(curSection, ptnNum);
    }

  } else {
    // SONG MODE
    if (songLen[songNum] == 0) { seqRunning = 0; return; }
    songPos++;
    if (songPos >= songLen[songNum]) songPos = 0;

    byte t = songArr[songNum][0][songPos];
    byte n = songArr[songNum][1][songPos];
    byte k = songArr[songNum][2][songPos];
    byte r = songArr[songNum][3][songPos];
    if (t > 4) t = 0;
    if (k > 15) k = 0;
    if (r > 15) r = 0;

    setCurrentBlock(t, n);

    // Kit: applica se diverso dal corrente
    if (k != kitNum) {
      kitNum = k;
      preKitNum = k;
      mixLev();
    }
    // Rev: applica se diverso dal corrente
    if (r != curRev) {
      setRev(r);
    }
  }

  // Pattern mode: applica cambi kit/rev pendenti da comandi LWS
  if (playMode == 0) {
    if (preKitNum != kitNum) {
      kitNum = preKitNum;
      mixLev();
    }
  }
}

// =====================================================================
// SEQUENCER
// =====================================================================
void seqRun() {
  int numR = 0;

  if (swingTime > swingMax) swingTime = swingMax;

  // Voce 0 — BD
  if (curBlock[0][curSubStep] > 0 && bdMute == 0) {
    setVel(ampBd, curBlock[0][curSubStep]);
    numR = random(1, bdArrSize[kitArr[0][kitNum]]);
    soundBd.play(bdArr[kitArr[0][kitNum]][numR - 1]);
  }

  // Voce 1 — SD
  if (curBlock[1][curSubStep] > 0 && sdMute == 0) {
    setVel(ampSD, curBlock[1][curSubStep]);
    numR = random(1, sdArrSize[kitArr[1][kitNum]]);
    soundSd.play(sdArr[kitArr[1][kitNum]][numR - 1]);
  }

  // Voci 2+3 — HH e OH con choke
  if (hhMute == 0) {
    uint8_t hhVel = curBlock[2][curSubStep];
    uint8_t ohVel = curBlock[3][curSubStep];
    if (hhVel > 0) {
      setVel(ampHh, hhVel);
      envOH.noteOff();
      numR = random(1, hhArrSize[kitArr[2][kitNum]]);
      soundHhC.play(hhArr[kitArr[2][kitNum]][numR - 1]);
      envHh.noteOn();
    } else if (ohVel > 0) {
      setVel(ampOH, ohVel);
      envHh.noteOff();
      numR = random(1, ohArrSize[kitArr[3][kitNum]]);
      soundHhO.play(ohArr[kitArr[3][kitNum]][numR - 1]);
      envOH.noteOn();
    }
  }

  // Voce 4 — HH2
  if (curBlock[4][curSubStep] > 0 && hh2Mute == 0) {
    setVel(ampHH2, curBlock[4][curSubStep]);
    numR = random(1, hhArrSize[kitArr[4][kitNum]]);
    soundHh2.play(hhArr[kitArr[4][kitNum]][numR - 1]);
  }

  // Voce 5 — CLAP
  if (curBlock[5][curSubStep] > 0 && clapMute == 0) {
    setVel(ampClap, curBlock[5][curSubStep]);
    numR = random(1, clapArrSize[kitArr[5][kitNum]]);
    soundClap.play(clapArr[kitArr[5][kitNum]][numR - 1]);
  }

  // Voce 6 — PERC1
  if (curBlock[6][curSubStep] > 0 && percMute == 0) {
    setVel(ampPerc1, curBlock[6][curSubStep]);
    numR = random(1, perc1ArrSize[kitArr[6][kitNum]]);
    soundPerc1.play(perc1Arr[kitArr[6][kitNum]][numR - 1]);
  }

  // Voce 7 — PERC2
  if (curBlock[7][curSubStep] > 0 && percMute == 0) {
    setVel(ampPerc2, curBlock[7][curSubStep]);
    numR = random(1, perc2ArrSize[kitArr[7][kitNum]]);
    soundPerc2.play(perc2Arr[kitArr[7][kitNum]][numR - 1]);
  }

  // Voce 8 — PERC3
  if (curBlock[8][curSubStep] > 0 && percMute == 0) {
    setVel(ampPerc3, curBlock[8][curSubStep]);
    numR = random(1, perc3ArrSize[kitArr[8][kitNum]]);
    soundPerc3.play(perc3Arr[kitArr[8][kitNum]][numR - 1]);
  }

  // Swing
  if (curSubStep % 2 == 0) seqTime = preSeqTime + swingTime;
  else                     seqTime = preSeqTime - swingTime;

  curSubStep++;
  if (curSubStep >= 16) {
    curSubStep = 0;
    advanceBlock();
  }
}

// =====================================================================
// START / STOP
// =====================================================================
void startStop() {
  if (sync == 0) {
    if (seqRunning == 0) {
      seqRunning  = 1;
      togSdWave   = 1;
      curSubStep  = 0;
      playingFill = false;
      fillPending = false;
      fillNum     = preFillNum;

      if (playMode == 0) {
        curSection = preSection = 0;
        setCurrentBlock(0, ptnNum);
      } else {
        if (songLen[songNum] == 0) { seqRunning = 0; return; }
        songPos = 0;
        byte t = songArr[songNum][0][0];
        byte n = songArr[songNum][1][0];
        byte k = songArr[songNum][2][0];
        byte r = songArr[songNum][3][0];
        if (t > 4) t = 0;
        if (k > 15) k = 0;
        if (r > 15) r = 0;
        setCurrentBlock(t, n);
        // Applica kit e rev dello slot 0 subito
        if (k != kitNum) { kitNum = preKitNum = k; mixLev(); }
        if (r != curRev) { setRev(r); }
      }
    } else {
      seqRunning  = 0;
      togSdWave   = 1;
      swingTime   = min((unsigned long)30, swingMax);
      playingFill = false;
      fillPending = false;
      curSubStep  = 0;
    }
  }

  if (sync == 1) {
    if (seqRunning == 0) {
      seqRunning = 1;
      kitNum     = preKitNum;
      swingTime  = 0;
      percMute   = 1;
      togSdWave  = 0;
      curSubStep = 0;
    } else {
      seqRunning = 0;
      sdTotTime  = 0;
      if (track2exist == 1) playSdWav2.stop();
      playSdWav.stop();
      percMute   = 1;
      togSdWave  = 1;
      curSubStep = 0;
    }
  }
}

// =====================================================================
// LWS — EDIT KIT / FX helpers
// =====================================================================
int voiceToRevIdx(int voice) {
  switch (voice) {
    case 2: return 0;
    case 6: return 1;
    case 7: return 2;
    case 9: return 3;
    default: return -1;
  }
}

void applyFxParam(int key, int value) {
  tempRevPreset = kitArr[9][kitNum];
  if (value > 20) value = 20;
  if (value < 0)  value = 0;

  AudioNoInterrupts();
  switch (key) {
    case '1': revPresetArr[0][tempRevPreset] = value;
              revInt.roomsize(levArr[value]); break;
    case '2': revPresetArr[1][tempRevPreset] = value;
              revInt.damping(levArr[value]); break;
    case '3': revPresetArr[4][tempRevPreset] = value;
              preDelay.delay(0, delayArr[value]); break;
    case '4': revPresetArr[2][tempRevPreset] = value;
              filter.frequency(cutArr[value]);
              if (revPresetArr[7][tempRevPreset] == 0)
                  filter2.frequency(cutArr[value]);
              break;
    case '5': revPresetArr[3][tempRevPreset] = value;
              filter.resonance(resArr[value]);
              if (revPresetArr[7][tempRevPreset] == 0)
                  filter2.resonance(resArr[value]);
              break;
    case '6': revPresetArr[5][tempRevPreset] = value;
              filter2.frequency(cutArr[value]); break;
    case '7': revPresetArr[6][tempRevPreset] = value;
              filter2.resonance(resArr[value]); break;
    case '8': revPresetArr[7][tempRevPreset] = value ? 1 : 0;
              setRev(curRev); break;
  }
  AudioInterrupts();
}

void applyVoiceParam(int key, int value) {
  int vi = voiceSel - 1;
  int ki = preKitNum;

  switch (key) {
    case 'A':
      if (value > maxArr[vi]) value = maxArr[vi];
      if (value < 0) value = 0;
      kitArr[vi][ki] = value;
      break;

    case 'L':
      if (value > 20) value = 20;
      if (value < 0)  value = 0;
      kitLevArr[vi][ki] = value;
      setPan(vi, kitPanArr[vi][ki]);
      break;

    case 'P': {
      if (value > 100) value = 100;
      if (value < 0)   value = 0;

      if (voiceSel == 1) {
        kitPanArr[0][ki] = value;   // BD, memorizza ma non applica
      } else if (voiceSel == 4) {
        kitPanArr[2][ki] = value;
        kitPanArr[3][ki] = value;
        setPan(2, value);
      } else {
        kitPanArr[vi][ki] = value;
        setPan(vi, value);
      }
      break;
    }

    case 'I': {
      int ri = voiceToRevIdx(voiceSel);
      if (ri >= 0) {
        if (value > 20) value = 20;
        if (value < 0)  value = 0;
        intRevLevArr[ri][ki] = value;
        AudioNoInterrupts();
        mixRevInt.gain(ri, levArr[value]);
        AudioInterrupts();
      }
      break;
    }
  }
}

// =====================================================================
// LWS CALLBACKS
// =====================================================================
void on_param(char target, char key, uint8_t value) {
  if (target != MCU_ID) return;
  LWS_DEBUG.printf("[LWS] key=%c val=%u\n", key, value);

  int v = value;

  switch (key) {
    case 'R': if (!seqRunning) startStop(); break;
    case 'S': if ( seqRunning) startStop(); break;

    case 'N':
      prePtnNum = constrain(v, 0, 15);
      lws_send_param(LWS_OUT_TARGET, 'N', (uint8_t)prePtnNum);
      break;
    case 'K':
      preKitNum = constrain(v, 0, 15);
      lws_send_param(LWS_OUT_TARGET, 'K', (uint8_t)preKitNum);
      break;
    case 'T':
      if (sync == 1) {
        trackSdNum = constrain(v, 0, trackMaxNum - 1);
        getTempoTrack(trackSdNum);
        track2exist = SD.exists(trackSdBArr[trackSdNum]) ? 1 : 0;
      }
      lws_send_param(LWS_OUT_TARGET, 'T', (uint8_t)trackSdNum);
      lws_send_param(LWS_OUT_TARGET, '2', (uint8_t)track2exist);
      break;
    case 'X':
      sync = constrain(v, 0, 2);
      lws_send_param(LWS_OUT_TARGET, 'X', sync);
      break;

    case 'd': {
      uint8_t n = constrain(v, 0, 20);
      levPtnArr[1][ptnNum] = n;
      drumLev(levArr[n]);
      lws_send_param(LWS_OUT_TARGET, 'd', n);
      break;
    }
    case 'f': {
      uint8_t n = constrain(v, 0, 20);
      levPtnArr[0][ptnNum] = n;
      fxOutLev(levArr[n]);
      lws_send_param(LWS_OUT_TARGET, 'f', n);
      break;
    }
    case 'm': {
      uint8_t n = constrain(v, 0, 20);
      trackLevArr[trackSdNum] = n;
      AudioNoInterrupts(); mixOutMono.gain(0, levArr[n]); AudioInterrupts();
      lws_send_param(LWS_OUT_TARGET, 'm', n);
      break;
    }
    case 't': {
      uint8_t n = constrain(v, 0, 20);
      track2LevArr[trackSdNum] = n;
      AudioNoInterrupts(); mixOutMono.gain(1, levArr[n]); AudioInterrupts();
      lws_send_param(LWS_OUT_TARGET, 't', n);
      break;
    }
    case 'u': {
      uint8_t n = constrain(v, 0, 20);
      track3LevArr[trackSdNum] = n;
      AudioNoInterrupts(); mixOutMono.gain(2, levArr[n]); AudioInterrupts();
      lws_send_param(LWS_OUT_TARGET, 'u', n);
      break;
    }

    case 'b':
      bpm = constrain(v, 60, 250);
      bpmToTime(bpm);
      lws_send_param(LWS_OUT_TARGET, 'b', (uint8_t)bpm);
      lws_send_param(LWS_OUT_TARGET, 'W', (uint8_t)swingTime);
      break;

    case 'W': {
      uint8_t w = (uint8_t)constrain(v, 0, (int)swingMax);
      swingTime = w;
      lws_send_param(LWS_OUT_TARGET, 'W', w);
      break;
    }

    case 'F': break;   // legacy, ignorato

    case 'M':
      bdMute   = (value & 0x01) ? 1 : 0;
      sdMute   = (value & 0x02) ? 1 : 0;
      hhMute   = (value & 0x04) ? 1 : 0;
      hh2Mute  = (value & 0x08) ? 1 : 0;
      clapMute = (value & 0x10) ? 1 : 0;
      percMute = (value & 0x20) ? 1 : 0;
      lws_send_param(LWS_OUT_TARGET, 'M', value);
      break;

    case 'V':
      voiceSel = constrain(v, 1, 9);
      lws_send_param(LWS_OUT_TARGET, 'V', (uint8_t)voiceSel);
      break;
    case 'A': case 'L': case 'P': case 'I':
      applyVoiceParam(key, v);
      lws_send_param(LWS_OUT_TARGET, key, value);
      break;

    case 'e':
      tempRevPreset = constrain(v, 0, 15);
      kitArr[9][kitNum] = tempRevPreset;
      setRev(tempRevPreset);
      lws_send_param(LWS_OUT_TARGET, 'e', (uint8_t)tempRevPreset);
      break;
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8':
      applyFxParam(key, v);
      lws_send_param(LWS_OUT_TARGET, key, value);
      break;

    case 'y':
      if (!seqRunning) {
        playMode = constrain(v, 0, 1);
        lws_send_param(LWS_OUT_TARGET, 'y', playMode);
      }
      break;

    case 's':
      songNum = constrain(v, 0, 15);
      if (playMode == 1 && seqRunning && songLen[songNum] > 0) {
        songPos = 0;
        byte t = songArr[songNum][0][0];
        byte n = songArr[songNum][1][0];
        byte k = songArr[songNum][2][0];
        byte r = songArr[songNum][3][0];
        if (t > 4) t = 0;
        if (k > 15) k = 0;
        if (r > 15) r = 0;
        setCurrentBlock(t, n);
        if (k != kitNum) { kitNum = preKitNum = k; mixLev(); }
        if (r != curRev) { setRev(r); }
        curSubStep = 0;
      }
      lws_send_param(LWS_OUT_TARGET, 's', songNum);
      break;

    case 'o':
      if (playMode == 0) {
        preSection = constrain(v, 0, 3);
        lws_send_param(LWS_OUT_TARGET, 'o', preSection);
      }
      break;

    case 'i':
      preFillNum = constrain(v, 0, 15);
      lws_send_param(LWS_OUT_TARGET, 'i', preFillNum);
      break;

    case 'h':
      if (playMode == 0 && seqRunning
          && !playingFill && !fillPending) {
        fillPending = true;
        lws_send_param(LWS_OUT_TARGET, 'h', 2);
      }
      break;

    case 'Z': dumpStatus(); break;
    case 'C': cpuUsage();  break;

    default:
      LWS_DEBUG.printf("[LWS] key sconosciuta: %c\n", key);
      break;
  }
}

void on_error(const char *msg) {
  LWS_DEBUG.printf("[LWS] ERROR: %s\n", msg);
}
void on_cc(uint8_t cc, uint8_t val) {
  LWS_DEBUG.printf("[LWS] CC %u=%u\n", cc, val);
}
void on_note(uint8_t on, uint8_t pitch, uint8_t vel) {
  LWS_DEBUG.printf("[LWS] NOTE %s p=%u v=%u\n", on ? "ON" : "OFF", pitch, vel);
}
void on_bend(int32_t b) {
  LWS_DEBUG.printf("[LWS] BEND %ld\n", (long)b);
}

// =====================================================================
// DUMP STATO
// =====================================================================
void dumpStatus() {
  LWS_DEBUG.printf("run=%u sync=%u bpm=%d mode=%u ptn=%d sec=%u fill=%u "
                   "song=%d pos=%d len=%u substep=%d rev=%u\n",
                   seqRunning, sync, bpm, playMode,
                   ptnNum, curSection, fillNum,
                   songNum, songPos, songLen[songNum], curSubStep, curRev);
  LWS_DEBUG.printf("fillTrig=%u preSec=%u preFill=%u prePtn=%d preKit=%d\n",
                   playingFill ? 1 : (fillPending ? 2 : 0),
                   preSection, preFillNum, prePtnNum, preKitNum);
  LWS_DEBUG.printf("kit=%d track=%d swing=%lu/%lu drum=%d fx=%d "
                   "mono=%d tk2=%d tk3=%d\n",
                   kitNum, trackSdNum, swingTime, swingMax,
                   levPtnArr[1][ptnNum], levPtnArr[0][ptnNum],
                   trackLevArr[trackSdNum], track2LevArr[trackSdNum],
                   track3LevArr[trackSdNum]);
  LWS_DEBUG.printf("mute BD=%u SD=%u HH+OH=%u HH2=%u CLAP=%u PERC=%u\n",
                   bdMute, sdMute, hhMute, hh2Mute, clapMute, percMute);
  LWS_DEBUG.printf("voiceSel=%d snd=%d lev=%d pan=%d\n",
                   voiceSel,
                   kitArr[voiceSel - 1][kitNum],
                   kitLevArr[voiceSel - 1][kitNum],
                   kitPanArr[voiceSel - 1][kitNum]);
  LWS_DEBUG.printf("fxPreset=%d revSize=%d revDamp=%d preDly=%d "
                   "cut=%d res=%d cut2=%d res2=%d mode=%d\n",
                   kitArr[9][kitNum],
                   revPresetArr[0][kitArr[9][kitNum]],
                   revPresetArr[1][kitArr[9][kitNum]],
                   revPresetArr[4][kitArr[9][kitNum]],
                   revPresetArr[2][kitArr[9][kitNum]],
                   revPresetArr[3][kitArr[9][kitNum]],
                   revPresetArr[5][kitArr[9][kitNum]],
                   revPresetArr[6][kitArr[9][kitNum]],
                   revPresetArr[7][kitArr[9][kitNum]]);
}

// =====================================================================
// SETUP
// =====================================================================
void setup() {
  LWS_DEBUG.begin(115200);
  LWS_DEBUG.println("=== Lud-WS-Teensy boot (rev13) ===");
  LWS_DEBUG.printf("MCU_ID='%c' LWS_BAUD=%lu\n", MCU_ID, (unsigned long)LWS_BAUD);

  LWS_SERIAL.begin(LWS_BAUD);

  lws_set_callbacks(on_param, on_error);
  lws_set_midi_callbacks(on_cc, on_note, on_bend);

  AudioMemory(260);
  audioCtrl1.enable(); audioCtrl1.volume(0.7);
  audioCtrl2.enable(); audioCtrl2.volume(0.7);

  if (!SerialFlash.begin(SERIALFLASH_CS)) LWS_DEBUG.println("SerialFlash FAILED");
  else                                     LWS_DEBUG.println("SerialFlash OK");
  if (!SD.begin(SD_CS))                    LWS_DEBUG.println("SD FAILED");
  else                                     LWS_DEBUG.println("SD OK");

  mixDrum1L.gain(0,0.7); mixDrum1R.gain(0,0.7);
  mixDrum1L.gain(1,0.7); mixDrum1R.gain(1,0.7);
  mixDrum1L.gain(2,0.7); mixDrum1R.gain(2,0.7);
  mixDrum1L.gain(3,0.7); mixDrum1R.gain(3,0.7);
  mixDrum2L.gain(0,0.7); mixDrum2R.gain(0,0.7);
  mixDrum2L.gain(1,0.7); mixDrum2R.gain(1,0.7);
  mixDrum2L.gain(2,0.7); mixDrum2R.gain(2,0.7);
  mixDrum2L.gain(3,0.7); mixDrum2R.gain(3,0.7);

  mixHH.gain(0, 1.0f);
  mixHH.gain(1, 1.0f);
  mixHH.gain(2, 0.0f);
  mixHH.gain(3, 0.0f);

  envHh.attack(0.5f);  envHh.hold(0.0f); envHh.decay(0.0f);
  envHh.sustain(1.0f); envHh.release(5.0f);
  envOH.attack(0.5f);  envOH.hold(0.0f); envOH.decay(0.0f);
  envOH.sustain(1.0f); envOH.release(5.0f);

  ampBd.gain(1.0f);
  ampSD.gain(1.0f);
  ampHh.gain(1.0f);
  ampOH.gain(1.0f);
  ampHH2.gain(1.0f);
  ampClap.gain(1.0f);
  ampPerc1.gain(1.0f);
  ampPerc2.gain(1.0f);
  ampPerc3.gain(1.0f);

  mixOutL.gain(0,1.0); mixOutL.gain(1,0.7); mixOutL.gain(2,0.7);
  mixOutR.gain(0,1.0); mixOutR.gain(1,0.7); mixOutR.gain(2,0.7);

  fxOutLev(levArr[levPtnArr[0][0]]);
  mixRevInt.gain(0, levArr[intRevLevArr[0][preKitNum]]);
  mixRevInt.gain(1, levArr[intRevLevArr[1][preKitNum]]);
  mixRevInt.gain(2, levArr[intRevLevArr[2][preKitNum]]);
  mixRevInt.gain(3, levArr[intRevLevArr[3][preKitNum]]);

  mixOutRev.gain(0,0.7); mixOutRev.gain(1,0.7);
  mixOutDly.gain(0,0.7); mixOutDly.gain(1,0.7);
  mixOutMono.gain(0,0.7); mixOutMono.gain(1,0.7); mixOutMono.gain(2,0.7);

  preDelay.disable(7); preDelay.disable(6); preDelay.disable(5);
  preDelay.disable(4); preDelay.disable(3); preDelay.disable(2);
  preDelay.disable(1);

  kitNum    = kitArr[9][0];   // default kit 0
  preKitNum = kitNum;
  setRev(kitArr[9][kitNum]);

  leggiSd();
  bpmToTime(bpm);

  playMode    = 0;
  curSection  = preSection = 0;
  ptnNum      = prePtnNum  = 0;
  fillNum     = preFillNum = 0;
  playingFill = false;
  fillPending = false;
  songNum     = 0;
  songPos     = 0;
  curSubStep  = 0;
  setCurrentBlock(0, 0);

  for (int i = 0; i < trackMaxNum; i++) {
    delay(50);
    playSdWav.play(trackSdArr[i]);
    delay(5);
    trackTempoArr[i] = (int)playSdWav.lengthMillis();
    playSdWav.stop();
  }

  LWS_DEBUG.println("Setup completo, in ascolto LWS a 1 Mbps");
}

// =====================================================================
// LOOP
// =====================================================================
void loop() {
  lws_mcu_poll();

  if (msecs > seqTime && seqRunning == 1 && sync == 0) {
    msecs = 0;
    seqRun();
  }

  if (togSdWave == 0 && sync == 1 && seqRunning == 1) {
    kitNum    = trackKitArr[trackSdNum];
    preKitNum = kitNum;
    togSdWave = 1;
    curSubStep = 0;
    if (track2exist == 1) playSdWav2.play(trackSdBArr[trackSdNum]);
    playSdWav.play(trackSdArr[trackSdNum]);
    delay(2);
    sdTotTime = playSdWav.lengthMillis();
  }

  if (peak1.available() && sync == 1 && seqRunning == 1) {
    float leftNumber = peak1.read();
    int leftPeak = leftNumber * 30.0;
    if (leftPeak > 1 && togPeak == 0) {
      seqRun();
      togPeak = 1;
      trigMsecs = 0;
    }
    if (leftPeak == 0) {
      togPeak = 0;
      if (trigMsecs > 30) {
        if (autoStepZero == 1) {
          if (curSubStep != 0) curSubStep = 0;
        }
      }
      trigMsecs = 0;
    }
  }
}