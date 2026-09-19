// =====================================================================
// SDfunc.h -- Utility SD per Lud-WS-Teensy rev7
// =====================================================================
#pragma once

// ---------------------------------------------------------------------
// Lista ricorsiva file (debug)
// ---------------------------------------------------------------------
void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);
  File root = fs.open(dirname);
  if (!root)               { Serial.println("  Failed to open dir"); return; }
  if (!root.isDirectory()) { Serial.println("  Not a directory");    return; }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.printf("  DIR : %s\n", file.name());
      if (levels) listDir(fs, file.name(), levels - 1);
    } else {
      uint32_t sz = file.size();
      if (sz < 1000) Serial.printf("  FILE: %s  %u b\n",  file.name(), sz);
      else           Serial.printf("  FILE: %s  %u Kb\n", file.name(), sz / 1000);
    }
    file = root.openNextFile();
  }
}

// ---------------------------------------------------------------------
// Scrittura file (sovrascrive)
// ---------------------------------------------------------------------
void writeFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_WRITE);
  if (!file) { Serial.printf("writeFile: failed %s\n", path); return; }
  file.print(message);
  file.close();
}

// ---------------------------------------------------------------------
// Legge una riga CSV (terminata da '&') in un array 1D
// ---------------------------------------------------------------------
template <typename T>
void readCsvRow(fs::FS &fs, const char *path, T *arr, int maxLen) {
  File file = fs.open(path);
  if (!file) { Serial.printf("readCsvRow: failed %s\n", path); return; }
  String strIn;
  int pos = 0;
  while (file.available() && pos < maxLen) {
    char c = file.read();
    if (c == '&') break;
    if (c == ',') { arr[pos++] = (T)strIn.toInt(); strIn = ""; }
    else            strIn += c;
  }
  file.close();
}

// ---------------------------------------------------------------------
// Legge 32 valori in una riga di un array 2D [16][32]
// ---------------------------------------------------------------------
template <typename T>
void readPattern(fs::FS &fs, const char *path, T arr[16][32], int row) {
  File file = fs.open(path);
  if (!file) { Serial.printf("readPattern: failed %s\n", path); return; }
  String strIn;
  int pos = 0;
  while (file.available() && pos < 32) {
    char c = file.read();
    if (c == '&') break;
    if (c == ',') { arr[row][pos++] = (T)strIn.toInt(); strIn = ""; }
    else            strIn += c;
  }
  file.close();
}

// ---------------------------------------------------------------------
// Carica in RAM tutti i pattern dalla SD
// ---------------------------------------------------------------------
void loadMem(fs::FS &fs) {
  Serial.println("--- loadMem START ---");
  char path[64];

  readCsvRow(fs, "/Seq1/SEQ1Lung_arr.txt", seq1LungArr, 16);
  readCsvRow(fs, "/Seq2/SEQ2Lung_arr.txt", seq2LungArr, 16);

  for (int i = 0; i < 16; i++) {
    snprintf(path, sizeof(path), "/Seq1/note/PTN%d_note.txt",       i);
    readPattern(fs, path, seq1Arr, i);

    snprintf(path, sizeof(path), "/Seq1/slideFm/PTN%d_slideFm.txt", i);
    readPattern(fs, path, seq1SlideFmArr, i);

    snprintf(path, sizeof(path), "/Seq2/note/PTN%d_note.txt",       i);
    readPattern(fs, path, seq2Arr, i);

    snprintf(path, sizeof(path), "/Seq2/slideFm/PTN%d_slideFm.txt", i);
    readPattern(fs, path, seq2SlideFmArr, i);

    snprintf(path, sizeof(path), "/Seq2/wov/PTN%d_wov.txt",         i);
    readPattern(fs, path, seq2WovArr, i);
  }
  Serial.println("--- loadMem DONE ---");
}

// ---------------------------------------------------------------------
// Scrive su SD tutti i pattern dalla RAM
// ---------------------------------------------------------------------
void writeMem() {
  Serial.println("--- writeMem START ---");
  char buf[256];
  char path[64];
  int  pos;

  pos = 0;
  for (int i = 0; i < 16; i++)
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%d,", seq1LungArr[i]);
  snprintf(buf + pos, sizeof(buf) - pos, "&");
  writeFile(SD, "/Seq1/SEQ1Lung_arr.txt", buf);

  pos = 0;
  for (int i = 0; i < 16; i++)
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%d,", seq2LungArr[i]);
  snprintf(buf + pos, sizeof(buf) - pos, "&");
  writeFile(SD, "/Seq2/SEQ2Lung_arr.txt", buf);

  for (int w = 0; w < 16; w++) {
    pos = 0;
    for (int i = 0; i < 32; i++)
      pos += snprintf(buf + pos, sizeof(buf) - pos, "%d,", seq1Arr[w][i]);
    snprintf(buf + pos, sizeof(buf) - pos, "&");
    snprintf(path, sizeof(path), "/Seq1/note/PTN%d_note.txt", w);
    writeFile(SD, path, buf);

    pos = 0;
    for (int i = 0; i < 32; i++)
      pos += snprintf(buf + pos, sizeof(buf) - pos, "%d,", seq1SlideFmArr[w][i]);
    snprintf(buf + pos, sizeof(buf) - pos, "&");
    snprintf(path, sizeof(path), "/Seq1/slideFm/PTN%d_slideFm.txt", w);
    writeFile(SD, path, buf);

    pos = 0;
    for (int i = 0; i < 32; i++)
      pos += snprintf(buf + pos, sizeof(buf) - pos, "%d,", seq2Arr[w][i]);
    snprintf(buf + pos, sizeof(buf) - pos, "&");
    snprintf(path, sizeof(path), "/Seq2/note/PTN%d_note.txt", w);
    writeFile(SD, path, buf);

    pos = 0;
    for (int i = 0; i < 32; i++)
      pos += snprintf(buf + pos, sizeof(buf) - pos, "%d,", seq2SlideFmArr[w][i]);
    snprintf(buf + pos, sizeof(buf) - pos, "&");
    snprintf(path, sizeof(path), "/Seq2/slideFm/PTN%d_slideFm.txt", w);
    writeFile(SD, path, buf);

    pos = 0;
    for (int i = 0; i < 32; i++)
      pos += snprintf(buf + pos, sizeof(buf) - pos, "%d,", seq2WovArr[w][i]);
    snprintf(buf + pos, sizeof(buf) - pos, "&");
    snprintf(path, sizeof(path), "/Seq2/wov/PTN%d_wov.txt", w);
    writeFile(SD, path, buf);
  }
  Serial.println("--- writeMem DONE ---");
}

// ---------------------------------------------------------------------
// Info SD (debug)
// ---------------------------------------------------------------------
void SDinfo() {
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) { Serial.println("No SD card attached"); return; }

  Serial.print("SD Type: ");
  if      (cardType == CARD_MMC)  Serial.println("MMC");
  else if (cardType == CARD_SD)   Serial.println("SDSC");
  else if (cardType == CARD_SDHC) Serial.println("SDHC");
  else                            Serial.println("UNKNOWN");

  Serial.printf("SD size: %llu MB\n", SD.cardSize()   / (1024ULL * 1024ULL));
  Serial.printf("Total  : %llu MB\n", SD.totalBytes() / (1024ULL * 1024ULL));
  Serial.printf("Used   : %llu MB\n", SD.usedBytes()  / (1024ULL * 1024ULL));
  listDir(SD, "/", 0);
}