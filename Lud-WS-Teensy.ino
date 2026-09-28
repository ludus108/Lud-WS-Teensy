// =====================================================================
// Lud-WS-Teensy -- Teensy 4.1 drum sampler + sequencer + LWS node
// ---------------------------------------------------------------------
//   				V 0.1.10 (2026-09-28) — tre livelli di velocity su tutte le voci drum,

//                      ultima modifica:
 /* 
 HH/OH con player, amp ed envelope separati,
                      seqArr esteso a 9 righe (OH in posizione 3). 
 */
//
//   - Motore audio completo (Teensy Audio Library + SerialFlash + SD)
//   - Protocollo LWS v1.1 (serial_protocol.h + comunicazioni_mcu.h)
//   - Mappa completa comandi LWS + echo bidirezionale
//   - Pin remap: LWS su Serial3 (TX=14, RX=15) per liberare Serial2
//     (pin 7,8) usato dal secondo codec I2S di AudioOutputI2SQuad
//
//   DRUM   : AudioPlaySerialflashRaw x9 (SPI Flash, CS=6)
//            BD, SD, HHc, HOo, HH2, CLAP, PERC1, PERC2, PERC3
//   TRACKS : AudioPlaySdWav x2 (SD, CS=10)
//   OUTPUT : AudioOutputI2SQuad + 2x SGTL5000
//
// Layout righe seqArr[16][9][32]:
//   0 = BD     1 = SD     2 = HH     3 = OH     4 = HH2
//   5 = CLAP   6 = PERC1  7 = PERC2  8 = PERC3
//
// Codifica celle:
//   0     = silenzio
//   1..3  = velocity soft / medium / accent
//
// LWS:
//   MCU_ID='T', Serial3 @ 1 Mbps (TX=14, RX=15)
//   Chiavi PARAM (target='T'): R,S,N,K,T,X,d,f,m,t,u,b,W,F,M,V,A,L,P,I,e,1..8,Z,C
// =====================================================================

#include <Arduino.h>

// ---------------------------------------------------------------------
// Config nodo LWS (PRIMA degli include LWS)
// ---------------------------------------------------------------------
#define MCU_ID      'T'
#define LWS_SERIAL  Serial3          // Teensy 4.1: TX=14, RX=15
#define LWS_DEBUG   Serial           // USB
#define LWS_BAUD    1000000UL        // 1 Mbps sul bus LWS

#include "serial_protocol.h"
#include "comunicazioni_mcu.h"

// ---------------------------------------------------------------------
// Librerie audio / storage
// ---------------------------------------------------------------------
#include <Audio.h>
#include <Wire.h>
#include <MIDI.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <EEPROM.h>

// ---------------------------------------------------------------------
// Pin map
// ---------------------------------------------------------------------
#define SERIALFLASH_CS   6
#define SD_CS            10
// LWS   : Serial3 = TX14, RX15
// MIDI  : Serial6 = TX24, RX25
// SPI   : MISO=12, MOSI=11, SCK=13 (condiviso SD + Flash)
// I2S   : da Teensy Audio Shield (pin 7,8,18,19,20,21,23)

// =====================================================================
// AUDIO GRAPH
// ---------------------------------------------------------------------
// Ogni voce drum passa per un AudioAmplifier dedicato, così ogni
// colpo può avere uno dei tre livelli di velocity (soft/medium/accent)
// senza agire sui mixer globali.
//
// HH e OH hanno player, amp ed envelope separati:
//   soundHhC -> ampHh -> envHh -\
//                                > mixHH -> mixDrum1L/R (canale 1)
//   soundHhO -> ampOH -> envOH -/
// L'envelope serve solo a un fade-out rapido (5 ms) per il choke
// pulito tra closed e open.
// =====================================================================

// --- Sorgenti drum (SerialFlash) -------------------------------------
AudioPlaySerialflashRaw soundBd, soundSd, soundHhC, soundHhO, soundHh2,
                         soundClap, soundPerc1, soundPerc2, soundPerc3;

// --- Sorgenti tracce WAV (SD) ----------------------------------------
AudioPlaySdWav          playSdWav, playSdWav2;

// --- Analizzatore ----------------------------------------------------
AudioAnalyzePeak        peak1;

// --- Amplificatori di velocity ---------------------------------------
AudioAmplifier          ampBd, ampSD, ampHh, ampOH, ampHH2,
                        ampClap, ampPerc1, ampPerc2, ampPerc3;

// --- Envelope di choke per HH/OH -------------------------------------
AudioEffectEnvelope     envHh, envOH;

// --- Mixer -----------------------------------------------------------
AudioMixer4             mixOutL, mixOutR, mixDrum1L, mixDrum1R,
                        mixDrum2L, mixDrum2R, mixOutMono,
                        mixOutRev, mixOutDly, mixRevInt, mixHH;

// --- Effetti ---------------------------------------------------------
AudioFilterStateVariable filter, filter2;
AudioEffectDelay         preDelay;
AudioEffectFreeverbStereo revInt;

// --- Output ----------------------------------------------------------
AudioOutputI2SQuad      outQuad;
AudioControlSGTL5000    audioCtrl1, audioCtrl2;

// =====================================================================
// CONNESSIONI
// =====================================================================

// --- BD (mixDrum1 ch 0) ----------------------------------------------
AudioConnection bd0(soundBd, 0, ampBd, 0);
AudioConnection bd1(ampBd, 0, mixDrum1L, 0);
AudioConnection bd2(ampBd, 0, mixDrum1R, 0);

// --- SD (mixDrum1 ch 2 + send wet) -----------------------------------
AudioConnection sd0(soundSd, 0, ampSD, 0);
AudioConnection sd1(ampSD, 0, mixDrum1L, 2);
AudioConnection sd2(ampSD, 0, mixDrum1R, 2);
AudioConnection sd3(ampSD, 0, mixRevInt, 0);
AudioConnection sd4(ampSD, 0, mixOutRev, 0);
AudioConnection sd5(ampSD, 0, mixOutDly, 0);

// --- HH / OH (due rami confluiscono in mixHH -> mixDrum1 ch 1) -------
AudioConnection hhc0(soundHhC, 0, ampHh, 0);
AudioConnection hhc1(ampHh,   0, envHh, 0);
AudioConnection hhc2(envHh,   0, mixHH, 0);

AudioConnection hho0(soundHhO, 0, ampOH, 0);
AudioConnection hho1(ampOH,   0, envOH, 0);
AudioConnection hho2(envOH,   0, mixHH, 1);

AudioConnection hhm1(mixHH, 0, mixDrum1L, 1);
AudioConnection hhm2(mixHH, 0, mixDrum1R, 1);

// --- CLAP (mixDrum1 ch 3 + send wet) ---------------------------------
AudioConnection cp0(soundClap, 0, ampClap, 0);
AudioConnection cp1(ampClap, 0, mixDrum1L, 3);
AudioConnection cp2(ampClap, 0, mixDrum1R, 3);
AudioConnection cp3(ampClap, 0, mixRevInt, 1);
AudioConnection cp4(ampClap, 0, mixOutRev, 1);
AudioConnection cp5(ampClap, 0, mixOutDly, 1);

// --- HH2 (mixDrum2 ch 0) ---------------------------------------------
AudioConnection hh20(soundHh2, 0, ampHH2, 0);
AudioConnection hh21(ampHH2, 0, mixDrum2L, 0);
AudioConnection hh22(ampHH2, 0, mixDrum2R, 0);

// --- PERC1 (mixDrum2 ch 1 + send riverbero) --------------------------
AudioConnection pe10(soundPerc1, 0, ampPerc1, 0);
AudioConnection pe11(ampPerc1, 0, mixDrum2L, 1);
AudioConnection pe12(ampPerc1, 0, mixDrum2R, 1);
AudioConnection pe13(ampPerc1, 0, mixRevInt, 2);

// --- PERC2 (mixDrum2 ch 2) -------------------------------------------
AudioConnection pe20(soundPerc2, 0, ampPerc2, 0);
AudioConnection pe21(ampPerc2, 0, mixDrum2L, 2);
AudioConnection pe22(ampPerc2, 0, mixDrum2R, 2);

// --- PERC3 (mixDrum2 ch 3 + send riverbero) --------------------------
AudioConnection pe30(soundPerc3, 0, ampPerc3, 0);
AudioConnection pe31(ampPerc3, 0, mixDrum2L, 3);
AudioConnection pe32(ampPerc3, 0, mixDrum2R, 3);
AudioConnection pe33(ampPerc3, 0, mixRevInt, 3);

// --- Catena riverbero ------------------------------------------------
AudioConnection pd(mixRevInt, 0, preDelay, 0);
AudioConnection ri(preDelay, 0, revInt, 0);
AudioConnection filt1(revInt, 0, filter, 0);
AudioConnection filt2(revInt, 1, filter2, 0);

// --- Tracce WAV ------------------------------------------------------
AudioConnection sd11(playSdWav, 0, peak1, 0);
AudioConnection sd12(playSdWav, 1, mixOutMono, 0);   // track1 bass
AudioConnection sd13(playSdWav2, 0, mixOutMono, 1);  // track2
AudioConnection sd14(playSdWav2, 1, mixOutMono, 2);  // track3

// --- Mix finale ------------------------------------------------------
AudioConnection mxL1(mixOutMono, 0, mixOutL, 0);
AudioConnection mxL2(mixDrum1L, 0, mixOutL, 1);
AudioConnection mxL3(mixDrum2L, 0, mixOutL, 2);
AudioConnection mxL4(filter, 0, mixOutL, 3);

AudioConnection mxR1(mixOutMono, 0, mixOutR, 0);
AudioConnection mxR2(mixDrum1R, 0, mixOutR, 1);
AudioConnection mxR3(mixDrum2R, 0, mixOutR, 2);
AudioConnection mxR4(filter2, 0, mixOutR, 3);

AudioConnection o1(mixOutRev, 0, outQuad, 0);   // Rev L (SGTL5000 #1)
AudioConnection o2(mixOutDly, 0, outQuad, 1);   // Dly R
AudioConnection o3(mixOutL, 0, outQuad, 2);     // PCM L (SGTL5000 #2)
AudioConnection o4(mixOutR, 0, outQuad, 3);     // PCM R

// =====================================================================
// MIDI IN (opzionale, Serial6 = TX24/RX25)
// =====================================================================
MIDI_CREATE_INSTANCE(HardwareSerial, Serial6, MIDIext);

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
const String trackNameArr[5] = {"CompSl","track2","track3","track4","track5"};

// =====================================================================
// STATO SEQUENCER / MIXER
// =====================================================================
int step = 0;
int fineSeq = 15;
unsigned long seqTime = 120;
unsigned long swingTime = 30;
unsigned long swingMax = 0;
unsigned long preSeqTime = 120;

byte seqRunning = 0;
byte sync = 0;                    // 0=int, 1=SD track, 2=ext
long sdTotTime = 0;
int fillNum = 8;
int contaFillNum = 1;
byte togSdWave = 1;
byte togPeak = 0;

int bpm = 120;
int kitNum = 1;
int preKitNum = 1;

byte bdMute=0, sdMute=0, clapMute=0, hhMute=0, hh2Mute=0, percMute=1;

int track2exist = 0;
int ptnNum = 0;
int prePtnNum = 0;
byte autoStepZero = 1;

int voiceSel = 1;                 // 1..9 (BD,SD,HH,OH,HH2,CLAP,PRC1,PRC2,PRC3)
int tempRevPreset = 0;

elapsedMillis msecs, trigMsecs, trigOutMs;
File myFile;

#include "lista.h"
#include "mem.h"

// =====================================================================
// HELPERS
// =====================================================================
float mapFloat(float val, float daMin, float daMax, float aMin, float aMax) {
  return (val - daMin) * (aMax - aMin) / (daMax - daMin) + aMin;
}

void bpmToTime(int bpm2) {
  preSeqTime = floor((60000 / bpm2) / 4);
  swingMax = floor((preSeqTime / 2) - 2);
}

int soundPiuMeno(int val, byte Pm, int strum) {
  if (Pm == 1) val++;
  if (Pm == 0) val--;
  if (val > maxArr[strum]) val = maxArr[strum];
  if (val < 1) val = 0;
  return val;
}

// ---------------------------------------------------------------------
// Velocity: mappa 1..3 -> gain lineare
// ---------------------------------------------------------------------
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

void setPan(int id, int val) {
  int leftVal, rightVal;
  int tempLev = kitLevArr[id][preKitNum - 1];
  if (val > 50) { leftVal = tempLev; rightVal = tempLev; }
  else {
    leftVal  = map(val, 0, 20, 0, tempLev);
    rightVal = map(val, 0, 20, tempLev, 0);
  }
  AudioNoInterrupts();
  switch (id) {
    case 0: mixDrum1L.gain(0, levArr[leftVal]); mixDrum1R.gain(0, levArr[rightVal]); break;
    case 1: mixDrum1L.gain(2, levArr[leftVal]); mixDrum1R.gain(2, levArr[rightVal]); break;
    case 2: mixDrum1L.gain(1, levArr[leftVal]); mixDrum1R.gain(1, levArr[rightVal]); break;
    case 3: mixDrum2L.gain(0, levArr[leftVal]); mixDrum2R.gain(0, levArr[rightVal]); break;
    case 4: mixDrum1L.gain(3, levArr[leftVal]); mixDrum1R.gain(3, levArr[rightVal]); break;
    case 5: mixDrum2L.gain(1, levArr[leftVal]); mixDrum2R.gain(1, levArr[rightVal]); break;
    case 6: mixDrum2L.gain(2, levArr[leftVal]); mixDrum2R.gain(2, levArr[rightVal]); break;
    case 7: mixDrum2L.gain(3, levArr[leftVal]); mixDrum2R.gain(3, levArr[rightVal]); break;
  }
  AudioInterrupts();
}

void setRev(int id) {
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
  if (revPresetArr[7][kitArr[9][kitNum - 1]] == 0) {
    filter2.frequency(cutArr[revPresetArr[2][id]]);
    filter2.resonance(resArr[revPresetArr[3][id]]);
  }
  AudioInterrupts();
}

void mixLev() {
  AudioNoInterrupts();
  for (int i = 0; i < 8; i++) setPan(i, kitPanArr[i][preKitNum - 1]);
  AudioInterrupts();
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

// =====================================================================
// SD PATTERN (leggiSd / scriviSd)
// ---------------------------------------------------------------------
// File PTNxx.TXT: 9 righe, una per voce, 32 valori separati da virgola.
// Layout righe:
//   0=BD 1=SD 2=HH 3=OH 4=HH2 5=CLAP 6=PERC1 7=PERC2 8=PERC3
//
// Compatibilità: se il file ha solo 8 righe (formato storico con OH
// dentro HH), la riga OH resta 0 e i valori 4..6 eventualmente presenti
// sulla riga HH vengono tradotti in velocity 1..3 sulla riga OH.
// =====================================================================

void leggiSd() {
  String bufferT, filename;
  char filenameCA[10];
  int strIndex[2];
  int maxIndex, indexArr, indexF;
  String charTemp;

  for (int w = 0; w < 16; w++) {
    indexF = 0;
    filename = "PTN" + (String)w + ".TXT";
    filename.toCharArray(filenameCA, 10);
    myFile = SD.open(filenameCA);

    if (!myFile) {
      // File assente: azzera pattern per sicurezza
      memset(seqArr[w], 0, sizeof(seqArr[w]));
      continue;
    }

    while (myFile.available()) {
      bufferT = myFile.readStringUntil('\n');
      strIndex[0] = 0;
      strIndex[1] = -1;
      maxIndex = bufferT.length() - 1;
      indexArr = 0;

      for (int i = 0; i <= maxIndex; i++) {
        if (bufferT.charAt(i) == ',' || i == maxIndex) {
          strIndex[0] = strIndex[1] + 1;
          strIndex[1] = (i == maxIndex) ? i + 1 : i;
          charTemp = bufferT.substring(strIndex[0], strIndex[1]);

          int val = charTemp.toInt();
          if (indexF < 9 && indexArr < 32) {
            seqArr[w][indexF][indexArr] = (byte)val;

            // Shim compatibilità: vecchi open (4..6) dentro HH
            if (indexF == 2 && val >= 4 && val <= 6) {
              seqArr[w][3][indexArr] = (byte)(val - 3);   // OH = 1..3
              seqArr[w][2][indexArr] = 0;                 // HH azzerato
            } else if (indexF == 2 && val > 3) {
              seqArr[w][2][indexArr] = 3;                 // clamp di sicurezza
            }
          }
          indexArr++;
        }
      }
      indexF++;
      if (indexF >= 9) break;   // oltre 9 righe: ignora il resto
    }
    myFile.close();
  }
}

void scriviSd() {
  String filename;
  char filenameCA[10];
  String txtOut = "";
  String suffTxt = "";

  for (int w = 0; w < 16; w++) {
    filename = "PTN" + (String)w + ".TXT";
    filename.toCharArray(filenameCA, 10);
    SD.remove(filenameCA);
    myFile = SD.open(filenameCA, FILE_WRITE);

    for (int y = 0; y < 9; y++) {
      for (int i = 0; i < 32; i++) {
        suffTxt = (i == 31) ? "" : ",";
        txtOut += (String)seqArr[w][y][i] + suffTxt;
      }
      myFile.println(txtOut);
      txtOut = "";
    }
    myFile.close();
    delay(10);
  }
}

// =====================================================================
// SEQUENCER
// ---------------------------------------------------------------------
// Layout righe seqArr[16][9][32]:
//   0=BD  1=SD  2=HH  3=OH  4=HH2  5=CLAP  6=PERC1  7=PERC2  8=PERC3
//
// Celle:
//   0     = silenzio
//   1..3  = velocity soft / medium / accent
//
// HH e OH hanno player, amp ed envelope separati. L'envelope chiude
// in 5 ms quando parte l'altro ramo (choke pulito). Se sullo stesso
// step entrambe le righe sono >0, vince HH.
// =====================================================================
void seqRun() {
  int numR = 0;

  // -------------------------------------------------------------------
  // Voce 0 — BD
  // -------------------------------------------------------------------
  if (seqArr[ptnNum][0][step] > 0 && bdMute == 0) {
    setVel(ampBd, seqArr[ptnNum][0][step]);
    numR = random(1, bdArrSize[kitArr[0][kitNum - 1]]);
    soundBd.play(bdArr[kitArr[0][kitNum - 1]][numR - 1]);
  }

  // -------------------------------------------------------------------
  // Voce 1 — SD
  // -------------------------------------------------------------------
  if (seqArr[ptnNum][1][step] > 0 && sdMute == 0) {
    setVel(ampSD, seqArr[ptnNum][1][step]);
    numR = random(1, sdArrSize[kitArr[1][kitNum - 1]]);
    soundSd.play(sdArr[kitArr[1][kitNum - 1]][numR - 1]);
  }

  // -------------------------------------------------------------------
  // Voci 2 + 3 — HH (closed) e OH (open) con choke reciproco pulito
  // -------------------------------------------------------------------
  if (hhMute == 0) {
    uint8_t hhVel = seqArr[ptnNum][2][step];
    uint8_t ohVel = seqArr[ptnNum][3][step];

    if (hhVel > 0) {
      // CLOSED: vel 1..3; choke dell'open via envelope
      setVel(ampHh, hhVel);
      envOH.noteOff();                          // fade-out 5 ms sull'open
      numR = random(1, hhArrSize[kitArr[2][kitNum - 1]]);
      soundHhC.play(hhArr[kitArr[2][kitNum - 1]][numR - 1]);
      envHh.noteOn();
    } else if (ohVel > 0) {
      // OPEN: vel 1..3; choke del closed via envelope
      setVel(ampOH, ohVel);
      envHh.noteOff();                          // fade-out 5 ms sul closed
      numR = random(1, ohArrSize[kitArr[3][kitNum - 1]]);
      soundHhO.play(ohArr[kitArr[3][kitNum - 1]][numR - 1]);
      envOH.noteOn();
    }
  }

  // -------------------------------------------------------------------
  // Voce 4 — HH2
  // -------------------------------------------------------------------
  if (seqArr[ptnNum][4][step] > 0 && hh2Mute == 0) {
    setVel(ampHH2, seqArr[ptnNum][4][step]);
    numR = random(1, hhArrSize[kitArr[4][kitNum - 1]]);
    soundHh2.play(hhArr[kitArr[4][kitNum - 1]][numR - 1]);
  }

  // -------------------------------------------------------------------
  // Voce 5 — CLAP
  // -------------------------------------------------------------------
  if (seqArr[ptnNum][5][step] > 0 && clapMute == 0) {
    setVel(ampClap, seqArr[ptnNum][5][step]);
    numR = random(1, clapArrSize[kitArr[5][kitNum - 1]]);
    soundClap.play(clapArr[kitArr[5][kitNum - 1]][numR - 1]);
  }

  // -------------------------------------------------------------------
  // Voce 6 — PERC1
  // -------------------------------------------------------------------
  if (seqArr[ptnNum][6][step] > 0 && percMute == 0) {
    setVel(ampPerc1, seqArr[ptnNum][6][step]);
    numR = random(1, perc1ArrSize[kitArr[6][kitNum - 1]]);
    soundPerc1.play(perc1Arr[kitArr[6][kitNum - 1]][numR - 1]);
  }

  // -------------------------------------------------------------------
  // Voce 7 — PERC2
  // -------------------------------------------------------------------
  if (seqArr[ptnNum][7][step] > 0 && percMute == 0) {
    setVel(ampPerc2, seqArr[ptnNum][7][step]);
    numR = random(1, perc2ArrSize[kitArr[7][kitNum - 1]]);
    soundPerc2.play(perc2Arr[kitArr[7][kitNum - 1]][numR - 1]);
  }

  // -------------------------------------------------------------------
  // Voce 8 — PERC3
  // -------------------------------------------------------------------
  if (seqArr[ptnNum][8][step] > 0 && percMute == 0) {
    setVel(ampPerc3, seqArr[ptnNum][8][step]);
    numR = random(1, perc3ArrSize[kitArr[8][kitNum - 1]]);
    soundPerc3.play(perc3Arr[kitArr[8][kitNum - 1]][numR - 1]);
  }

  // -------------------------------------------------------------------
  // Swing
  // -------------------------------------------------------------------
  if (step % 2 == 0) seqTime = preSeqTime + swingTime;
  else               seqTime = preSeqTime - swingTime;

  step++;

  // -------------------------------------------------------------------
  // Fine pattern / cambio kit / pattern pendenti
  // -------------------------------------------------------------------
  if (step > fineSeq) {
    contaFillNum++;

    if (sync == 0) fillNum = fillArr[ptnNum];
    if (sync == 1) fillNum = trackFillArr[trackSdNum];

    if (contaFillNum < fillNum) {
      step = 0;
      fineSeq = 15;
    } else {
      contaFillNum = 0;
      step = 16;
      fineSeq = 31;
    }

    if (preKitNum != kitNum) {
      kitNum = preKitNum;
      mixLev();
      setRev(kitNum - 1);
    }
    if (prePtnNum != ptnNum) {
      ptnNum = prePtnNum;
    }
  }
}

void startStop() {
  if (sync == 0) {
    if (seqRunning == 0) {
      seqRunning = 1; togSdWave = 1; step = 0;
    } else {
      seqRunning = 0; contaFillNum = 1;
      togSdWave = 1; step = 0; swingTime = 30;
    }
  }
  if (sync == 1) {
    if (seqRunning == 0) {
      seqRunning = 1;
      kitNum = preKitNum;
      swingTime = 0;
      percMute = 1;
      togSdWave = 0;
      step = 0;
    } else {
      seqRunning = 0;
      sdTotTime = 0;
      if (track2exist == 1) playSdWav2.stop();
      playSdWav.stop();
      percMute = 1;
      togSdWave = 1;
      step = 0;
      contaFillNum = 1;
    }
  }
}

// =====================================================================
// LWS — EDIT KIT / FX helpers
// =====================================================================
int voiceToRevIdx(int voice) {
  switch (voice) {
    case 2: return 0;   // SD
    case 6: return 1;   // CLAP
    case 7: return 2;   // PERC1
    case 9: return 3;   // PERC3
    default: return -1;
  }
}

void applyFxParam(int key, int value) {
  tempRevPreset = kitArr[9][kitNum - 1];
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
              setRev(tempRevPreset); break;
  }
  AudioInterrupts();
}

void applyVoiceParam(int key, int value) {
  int vi = voiceSel - 1;
  int ki = preKitNum - 1;

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

    case 'P':
      if (value > 100) value = 100;
      if (value < 0)   value = 0;
      kitPanArr[vi][ki] = value;
      setPan(vi, value);
      break;

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
    // Trasporto
    case 'R': if (!seqRunning) startStop(); break;
    case 'S': if ( seqRunning) startStop(); break;

    // Selezione
    case 'N':
      prePtnNum = constrain(v, 0, 15);
      lws_send_param(LWS_OUT_TARGET, 'N', (uint8_t)prePtnNum);
      break;
    case 'K':
      preKitNum = constrain(v, 1, 16);
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

    // Livelli master
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
      AudioNoInterrupts();
      mixOutMono.gain(0, levArr[n]);
      AudioInterrupts();
      lws_send_param(LWS_OUT_TARGET, 'm', n);
      break;
    }
    case 't': {
      uint8_t n = constrain(v, 0, 20);
      track2LevArr[trackSdNum] = n;
      AudioNoInterrupts();
      mixOutMono.gain(1, levArr[n]);
      AudioInterrupts();
      lws_send_param(LWS_OUT_TARGET, 't', n);
      break;
    }
    case 'u': {
      uint8_t n = constrain(v, 0, 20);
      track3LevArr[trackSdNum] = n;
      AudioNoInterrupts();
      mixOutMono.gain(2, levArr[n]);
      AudioInterrupts();
      lws_send_param(LWS_OUT_TARGET, 'u', n);
      break;
    }

    // Timing
    case 'b':
      bpm = constrain(v, 60, 250);
      bpmToTime(bpm);
      lws_send_param(LWS_OUT_TARGET, 'b', (uint8_t)bpm);
      break;
    case 'W':
      swingTime = (unsigned long)constrain(v, 0, (int)swingMax);
      lws_send_param(LWS_OUT_TARGET, 'W', (uint8_t)swingTime);
      break;
    case 'F': {
      uint8_t n = constrain(v, 2, 32);
      fillArr[ptnNum] = n;
      lws_send_param(LWS_OUT_TARGET, 'F', n);
      break;
    }

    // Mute (bitmask) — bit2 = HH+OH, bit3 = HH2
    case 'M':
      bdMute   = (value & 0x01) ? 1 : 0;
      sdMute   = (value & 0x02) ? 1 : 0;
      hhMute   = (value & 0x04) ? 1 : 0;   // HH + OH insieme
      hh2Mute  = (value & 0x08) ? 1 : 0;
      clapMute = (value & 0x10) ? 1 : 0;
      percMute = (value & 0x20) ? 1 : 0;
      lws_send_param(LWS_OUT_TARGET, 'M', value);
      break;

    // Edit kit
    case 'V':
      voiceSel = constrain(v, 1, 9);
      lws_send_param(LWS_OUT_TARGET, 'V', (uint8_t)voiceSel);
      break;
    case 'A': case 'L': case 'P': case 'I':
      applyVoiceParam(key, v);
      lws_send_param(LWS_OUT_TARGET, key, value);
      break;

    // Edit fx
    case 'e':
      tempRevPreset = constrain(v, 0, 15);
      kitArr[9][kitNum - 1] = tempRevPreset;
      setRev(tempRevPreset);
      lws_send_param(LWS_OUT_TARGET, 'e', (uint8_t)tempRevPreset);
      break;
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8':
      applyFxParam(key, v);
      lws_send_param(LWS_OUT_TARGET, key, value);
      break;

    // Sistema
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
// MIDI IN callbacks
// =====================================================================
void handleNoteOn(byte ch, byte pitch, byte vel) {
  LWS_DEBUG.printf("[MIDI] NoteOn ch=%u p=%u v=%u\n", ch, pitch, vel);
}
void handleNoteOff(byte ch, byte pitch, byte vel) {
  LWS_DEBUG.printf("[MIDI] NoteOff ch=%u p=%u\n", ch, pitch);
}
void handleCC(byte ch, byte num, byte val) {
  LWS_DEBUG.printf("[MIDI] CC ch=%u n=%u v=%u\n", ch, num, val);
}
void handlePB(byte ch, int bend) {
  LWS_DEBUG.printf("[MIDI] PB ch=%u b=%d\n", ch, bend);
}

// =====================================================================
// DUMP STATO
// =====================================================================
void dumpStatus() {
  LWS_DEBUG.printf("run=%u sync=%u bpm=%d ptn=%d kit=%d step=%d track=%d\n",
                   seqRunning, sync, bpm, ptnNum, preKitNum, step, trackSdNum);
  LWS_DEBUG.printf("drum=%d fx=%d mono=%d tk2=%d tk3=%d fill=%d swing=%lu\n",
                   levPtnArr[1][ptnNum], levPtnArr[0][ptnNum],
                   trackLevArr[trackSdNum], track2LevArr[trackSdNum],
                   track3LevArr[trackSdNum], fillArr[ptnNum], swingTime);
  LWS_DEBUG.printf("mute BD=%u SD=%u HH+OH=%u HH2=%u CLAP=%u PERC=%u\n",
                   bdMute, sdMute, hhMute, hh2Mute, clapMute, percMute);
  LWS_DEBUG.printf("voiceSel=%d snd=%d lev=%d pan=%d\n",
                   voiceSel,
                   kitArr[voiceSel - 1][preKitNum - 1],
                   kitLevArr[voiceSel - 1][preKitNum - 1],
                   kitPanArr[voiceSel - 1][preKitNum - 1]);
  LWS_DEBUG.printf("fxPreset=%d revSize=%d revDamp=%d preDly=%d "
                   "cut=%d res=%d cut2=%d res2=%d mode=%d\n",
                   kitArr[9][kitNum - 1],
                   revPresetArr[0][kitArr[9][kitNum - 1]],
                   revPresetArr[1][kitArr[9][kitNum - 1]],
                   revPresetArr[4][kitArr[9][kitNum - 1]],
                   revPresetArr[2][kitArr[9][kitNum - 1]],
                   revPresetArr[3][kitArr[9][kitNum - 1]],
                   revPresetArr[5][kitArr[9][kitNum - 1]],
                   revPresetArr[6][kitArr[9][kitNum - 1]],
                   revPresetArr[7][kitArr[9][kitNum - 1]]);
}

// =====================================================================
// SETUP
// =====================================================================
void setup() {
  LWS_DEBUG.begin(115200);
  LWS_DEBUG.println("=== Lud-WS-Teensy boot (rev10, sampler+LWS+3vel) ===");
  LWS_DEBUG.printf("MCU_ID='%c' LWS_BAUD=%lu\n", MCU_ID, (unsigned long)LWS_BAUD);

  LWS_SERIAL.begin(LWS_BAUD);           // Serial3 @ 1 Mbps (TX=14, RX=15)

  lws_set_callbacks(on_param, on_error);
  lws_set_midi_callbacks(on_cc, on_note, on_bend);

  // MIDI IN (opzionale, Serial6)
  MIDIext.begin(MIDI_CHANNEL_OMNI);
  MIDIext.turnThruOff();
  MIDIext.setHandleNoteOn(handleNoteOn);
  MIDIext.setHandleNoteOff(handleNoteOff);
  MIDIext.setHandleControlChange(handleCC);
  MIDIext.setHandlePitchBend(handlePB);

  // Audio
  AudioMemory(260);                     // aumentato per i nuovi amp+env
  audioCtrl1.enable(); audioCtrl1.volume(0.7);
  audioCtrl2.enable(); audioCtrl2.volume(0.7);

  // SerialFlash (drum samples)
  if (!SerialFlash.begin(SERIALFLASH_CS)) {
    LWS_DEBUG.println("SerialFlash FAILED");
  } else {
    LWS_DEBUG.println("SerialFlash OK");
  }

  // SD (tracce WAV + pattern TXT)
  if (!SD.begin(SD_CS)) {
    LWS_DEBUG.println("SD FAILED");
  } else {
    LWS_DEBUG.println("SD OK");
  }

  // --- Gain mixer drum ---------------------------------------------
  mixDrum1L.gain(0,0.7); mixDrum1R.gain(0,0.7); // bd
  mixDrum1L.gain(1,0.7); mixDrum1R.gain(1,0.7); // hh+oh
  mixDrum1L.gain(2,0.7); mixDrum1R.gain(2,0.7); // sd
  mixDrum1L.gain(3,0.7); mixDrum1R.gain(3,0.7); // clap
  mixDrum2L.gain(0,0.7); mixDrum2R.gain(0,0.7); // hh2
  mixDrum2L.gain(1,0.7); mixDrum2R.gain(1,0.7); // perc1
  mixDrum2L.gain(2,0.7); mixDrum2R.gain(2,0.7); // perc2
  mixDrum2L.gain(3,0.7); mixDrum2R.gain(3,0.7); // perc3

  // --- Sottomixer HH/OH: entrambi i rami a guadagno unitario -------
  mixHH.gain(0, 1.0f);
  mixHH.gain(1, 1.0f);
  mixHH.gain(2, 0.0f);
  mixHH.gain(3, 0.0f);

  // --- Envelope choke (fade-out 5 ms, per il resto trasparenti) ----
  envHh.attack(0.5f);  envHh.hold(0.0f); envHh.decay(0.0f);
  envHh.sustain(1.0f); envHh.release(5.0f);
  envOH.attack(0.5f);  envOH.hold(0.0f); envOH.decay(0.0f);
  envOH.sustain(1.0f); envOH.release(5.0f);

  // --- Gain iniziali degli amplificatori di velocity ---------------
  // Partono a 1.0; verranno ricalcolati a ogni colpo da setVel().
  ampBd.gain(1.0f);
  ampSD.gain(1.0f);
  ampHh.gain(1.0f);
  ampOH.gain(1.0f);
  ampHH2.gain(1.0f);
  ampClap.gain(1.0f);
  ampPerc1.gain(1.0f);
  ampPerc2.gain(1.0f);
  ampPerc3.gain(1.0f);

  // --- Mixer master -------------------------------------------------
  mixOutL.gain(0,1.0); mixOutL.gain(1,0.7); mixOutL.gain(2,0.7);
  mixOutR.gain(0,1.0); mixOutR.gain(1,0.7); mixOutR.gain(2,0.7);

  fxOutLev(levArr[levPtnArr[0][0]]);
  mixRevInt.gain(0, levArr[intRevLevArr[0][preKitNum - 1]]);
  mixRevInt.gain(1, levArr[intRevLevArr[1][preKitNum - 1]]);
  mixRevInt.gain(2, levArr[intRevLevArr[2][preKitNum - 1]]);
  mixRevInt.gain(3, levArr[intRevLevArr[3][preKitNum - 1]]);

  mixOutRev.gain(0,0.7); mixOutRev.gain(1,0.7);
  mixOutDly.gain(0,0.7); mixOutDly.gain(1,0.7);

  mixOutMono.gain(0,0.7); mixOutMono.gain(1,0.7); mixOutMono.gain(2,0.7);

  preDelay.disable(7); preDelay.disable(6); preDelay.disable(5);
  preDelay.disable(4); preDelay.disable(3); preDelay.disable(2);
  preDelay.disable(1);

  setRev(0);

  // Carica pattern da SD + calcola durate tracce
  leggiSd();
  bpmToTime(bpm);

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
  // LWS
  lws_mcu_poll();

  // MIDI
  MIDIext.read();

  // Sequencer interno
  if (msecs > seqTime && seqRunning == 1 && sync == 0) {
    msecs = 0;
    seqRun();
  }

  // SD track play (sync == 1)
  if (togSdWave == 0 && sync == 1 && seqRunning == 1) {
    kitNum = trackKitArr[trackSdNum];
    preKitNum = kitNum;
    fillNum = trackFillArr[trackSdNum];
    togSdWave = 1;
    step = 0;
    if (track2exist == 1) playSdWav2.play(trackSdBArr[trackSdNum]);
    playSdWav.play(trackSdArr[trackSdNum]);
    delay(2);
    sdTotTime = playSdWav.lengthMillis();
  }

  // Peak-based sequencer (sync == 1)
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
          if (step != 0 && step < 16) step = 0;
          if (step >= 16 && step != 16) step = 16;
        }
      }
      trigMsecs = 0;
    }
  }
}