//Simmons
const int SIMMSDarrs = 4, SIMMHTarrs = 3, SIMMMTarrs = 3, SIMMLTarrs = 4, SIMMRIMarrs = 2;
const char* SIMMSDarr[SIMMSDarrs] = {"SIMMSD1.raw", "SIMMSD2.raw", "SIMMSD3.raw", "SIMMSD4.raw"};
const char* SIMMHTarr[SIMMHTarrs] = {"SIMMHT1.raw", "SIMMHT2.raw", "SIMMHT3.raw"};
const char* SIMMMTarr[SIMMMTarrs] = {"SIMMMT1.raw", "SIMMMT2.raw", "SIMMMT3.raw"};
const char* SIMMLTarr[SIMMLTarrs] = {"SIMLT1.raw", "SIMLT2.raw", "SIMLT3.raw", "SIMLT4.raw"};
const char* SIMMRIMarr[SIMMRIMarrs] = {"SIMMRIM1.raw", "SIMMRIM2.raw"};
//CR8
const int CR78BDarrs = 3, CR78BLO1arrs = 3, CR78BLO2arrs = 3, CR78CLAParrs = 2, CR78COWarrs = 2, CR78GUIarrs = 6;
const char* CR78BDarr[CR78BDarrs] = {"CR78BD1.raw", "CR78BD2.raw", "CR78BD3.raw"};
const char* CR78BLO1arr[CR78BLO1arrs] = {"CR78BLO1.raw", "CR78BLO2.raw", "CR78BLO3.raw"};
const char* CR78BLO2arr[CR78BLO2arrs] = {"CR78BL~1.raw", "CR78BL~2.raw", "CR78BL~3.raw"};
const char* CR78CLAParr[CR78CLAParrs] = {"CR78CLP1.raw", "CR78CLP2.raw"};
const char* CR78COWarr[CR78COWarrs] = {"CR78COW1.raw", "CR78COW2.raw"};
const char* CR78GUIarr[CR78GUIarrs] = {"CR78GUI1.raw", "CR78GUI2.raw", "CR78GUI3.raw", "CR78GUI4.raw", "CR78GUI5.raw", "CR78GUI6.raw"};
const int CR78HHarrs = 4, CR78HOarrs = 4, CR78MEarrs = 4, CR78RIMarrs = 4, CR78SDarrs = 4, CR78TAMarrs = 4;
const char* CR78HHarr[CR78HHarrs] = {"CR78HH1.raw", "CR78HH2.raw", "CR78HH3.raw", "CR78HH4.raw"};
const char* CR78HOarr[CR78HOarrs] = {"CR78HO1.raw", "CR78HO2.raw", "CR78HO3.raw", "CR78HO4.raw"};
const char* CR78MEarr[CR78MEarrs] = {"CR78ME1.raw", "CR78ME2.raw", "CR78ME3.raw", "CR78ME4.raw"};
const char* CR78RIMarr[CR78RIMarrs] = {"CR78RIM1.raw", "CR78RIM2.raw", "CR78RIM3.raw", "CR78RIM4.raw"};
const char* CR78SDarr[CR78SDarrs] = {"CR78SD1.raw", "CR78SD2.raw", "CR78SD3.raw", "CR78SD4.raw"};
const char* CR78TAMarr[CR78TAMarrs] = {"CR78TAM1.raw", "CR78TAM2.raw", "CR78TAM3.raw", "CR78TAM4.raw"};
//CR77
const int CR77BDs = 1, CR77HHarrs = 8, CR77BKarrs = 4, TR76HOarrs = 4, TR76RIMarrs = 4, TR76PER1Carrs = 4, TR76PER2Carrs = 4, TR76PER3Carrs = 5;
const char* CR77BD[1] = {"CR77Bd.raw"};
const char* CR77HHarr[8] = {"CR77Hh1.raw", "Cr77Hh2.raw", "Cr77Hh3.raw", "Cr77Hh4.raw", "Cr77Hh5.raw", "Cr77Hh6.raw", "Cr77Hh7.raw", "Cr77Hh8.raw"};
const char* CR77BKarr[4] = {"CR77Bk1.raw", "CR77Bk2.raw", "CR77Bk3.raw", "CR77Bk4.raw"};
//tr76
const char* TR76HOarr[4] = {"TR76HO1.raw", "TR76HO2.raw", "TR76HO3.raw", "TR76HO4.raw"};
const char* TR76RIMarr[4] = {"TR76RIM1.raw", "TR76RIM2.raw", "TR76RIM3.raw", "TR76RIM4.raw"};
const char* TR76PER1Carr[4] = {"TR76PE1.raw", "TR76PE2.raw", "TR76PE3.raw", "TR76PE4.raw"};
const char* TR76PER2Carr[4] = {"TR76PER5.raw", "TR76PER6.raw", "TR76PER7.raw", "TR76PER8.raw"};
const char* TR76PER3Carr[5] = 	{"TR76PE9.raw", "TR76PE~1.raw", "TR76PE11.raw", "TR76PE12.raw", "TR76PER13.raw"};
const int TR76BDarrs = 8,  TR76HHarrs = 8, TR76SDarrs = 8;
const char* TR76BDarr[8] = {"TR76BD1.raw", "TR76BD2.raw", "TR76BD3.raw", "TR76BD4.raw", "TR76BD5.raw", "TR76BD8.raw", "TR76BD9.raw", "TR76BD10.raw"};
const char* TR76HHarr[8] = {"TR76HH1.raw", "TR76HH2.raw", "TR76HH3.raw", "TR76HH4.raw", "TR76HH5.raw", "TR76HH6.raw", "TR76HH7.raw", "TR76HH8.raw"};
const char* TR76SDarr[8] = {"TR76SD1.raw", "TR76SD2.raw", "TR76SD3.raw", "TR76SD4.raw", "TR76SD5.raw", "TR76SD6.raw", "TR76SD7.raw", "TR76SD8.raw"};
//hAMMOND
const int HAMBDarrs = 5, HAMHOBarrs = 5, HAMHHarrs = 6, HAMSDAarrs = 7, HAMSDBarrs = 4;
const char* HAMBDarr[5] = {"HAMBD1.raw", "HAMBD2.raw", "HAMBD3.raw", "HAMBD4.raw", "HAMBD5.raw"};
const char* HAMHOBarr[5] = {"HAMHOB1.raw", "HAMHOB2.raw", "HAMHOB3.raw", "HAMHOB4.raw", "HAMHOB5.raw"};
const char* HAMHHarr[6] = {"HAMHH1.raw", "HAMHH2.raw", "HAMHH3.raw", "HAMHH4.raw", "HAMHH5.raw", "HAMHH6.raw"};
const char* HAMSDAarr[7] = {"HAMSDA1.raw", "HAMSDA2.raw", "HAMSDA3.raw", "HAMSDA4.raw", "HAMSDA5.raw", "HAMSDA6.raw", "HAMSDA7.raw"};
const char* HAMSDBarr[4] = {"HAMSDB1.raw", "HAMSDB2.raw", "HAMSDB3.raw", "HAMSDB4.raw"};
//minopop
const int MPOPBDarrs = 4, MPOMXLarrs = 2, MPOPCLarrs = 4, MPOPCONarrs = 4, MPORIMarrs = 4, MPOPHHarrs = 6, MPOPHOarrs = 2, MPOPSDarrs = 12;
const char* MPOPBDarr[4] = {"MPOPBD1.raw", "MPOPBD2.raw", "MPOPBD3.raw", "MPOPBD4.raw"};
const char* MPOMXLarr[2] = {"MPOPMX1.raw", "MPOPMX2.raw"};
const char* MPOPCLarr[4] = {"MPOPCL1.raw", "MPOPCL2.raw", "MPOPCL5.raw", "MPOPCL6.raw"};
const char* MPOPCONarr[4] = {"MPOPCON1.raw", "MPOPCON2.raw", "MPOPCON3.raw", "MPOPCON4.raw"};
const char* MPORIMarr[4] = {"MPORIM1.raw", "MPORIM2.raw", "MPORIM3.raw", "MPORIM4.raw"};
const char* MPOPHHarr[6] = {"MPOPHH1.raw", "MPOPHH2.raw", "MPOPHH3.raw", "MPOPHH4.raw", "MPOPHH5.raw", "MPOPHH6.raw"};
const char* MPOPHOarr[2] = {"MPOPHO1.raw", "MPOPHO2.raw"};
const char* MPOPSDarr[12] = {"MPOPSD1.raw", "MPOPSD2.raw", "MPOPSD3.raw", "MPOPSD4.raw", "MPOPSD5.raw", "MPOPSD6.raw", "MPOPSD8.raw", "MPOPSD9.raw", "MPOPSD10.raw", "MPOPSD11.raw", "MPOPSD12.raw", "MPOPSD13.raw"};
//808
const int BD808arrs = 8, BD808Larrs = 8, CLAP808arrs = 8, CLAV808arrs = 8, CH808arrs = 8, SD808arrs = 8, SD808Barrs = 8, RS808arrs = 8, RS808Barrs = 8;
const char* BD808arr[8] = {"BD808A1.raw", "BD808A2.raw", "BD808A3.raw", "BD808A4.raw", "BD808A5.raw", "BD808A6.raw", "BD808A7.raw", "BD808A8.raw"};
const char* BD808Larr[8] = {"BD808B1.raw", "BD808B2.raw", "BD808B3.raw", "BD808B4.raw", "BD808B5.raw", "BD808B6.raw", "BD808B7.raw", "BD808B8.raw"};
const char* CLAP808arr[8] = {"clap8081.raw", "clap8082.raw", "clap8083.raw", "clap8084.raw", "clap8085.raw", "clap8086.raw", "clap8087.raw", "clap8088.raw"};
const char* CLAV808arr[8] = {"CLAV8081.raw", "CLAV8082.raw", "CLAV8083.raw", "CLAV8084.raw", "CLAV8085.raw", "CLAV8086.raw", "CLAV8087.raw", "CLAV8088.raw"};
const char* CH808arr[8] = {"CH8081.raw", "CH8082.raw", "CH8083.raw", "CH8084.raw", "CH8085.raw", "CH8086.raw", "CH8087.raw", "CH8088.raw"};
const char* SD808arr[8] = {"sd808A1.raw", "sd808A2.raw", "sd808A3.raw", "sd808A4.raw", "sd808A5.raw", "sd808A6.raw", "sd808A7.raw", "sd808A8.raw"};
const char* SD808Barr[8] = {"sd808B1.raw", "sd808B2.raw", "sd808B3.raw", "sd808B4.raw", "sd808B5.raw", "sd808B6.raw", "sd808B7.raw", "sd808B8.raw"};
const char* RS808arr[8] = {"RS8081.raw", "RS8082.raw", "RS8083.raw", "RS8084.raw", "RS8085.raw", "RS8086.raw", "RS8087.raw", "RS8088.raw"};
const char* RS808Barr[8] = {"RS808B1.raw", "RS808B2.raw", "RS808B3.raw", "RS808B4.raw", "RS808B5.raw", "RS808B6.raw", "RS808B7.raw", "RS808B8.raw"};
const int OH808arrs = 8, HCA808arrs = 4, HCB808arrs = 4, HCC808arrs = 4, HCD808arrs = 4, HCE808arrs = 4, OH808Barrs = 8, MAR808arrs = 8, CYM808Aarrs = 4, CYM808Barrs = 4;
const char* OH808arr[8] = {"OH808A1.raw", "OH808A2.raw", "OH808A3.raw", "OH808A4.raw", "OH808A5.raw", "OH808A6.raw", "OH808A7.raw", "OH808A8.raw"};
const char* HCA808arr[4] = {"HC808A1.raw", "HC808A2.raw", "HC808A3.raw", "HC808A4.raw"};
const char* HCB808arr[4] = {"HC808B1.raw", "HC808B2.raw", "HC808B3.raw", "HC808B4.raw"};
const char* HCC808arr[4] = {"HC808C1.raw", "HC808C2.raw", "HC808C3.raw", "HC808C4.raw"};
const char* HCD808arr[4] = {"HC808D1.raw", "HC808D2.raw", "HC808D3.raw", "HC808D4.raw"};
const char* HCE808arr[4] = {"HC808E1.raw", "HC808E2.raw", "HC808E3.raw", "HC808E4.raw"};
const char* OH808Barr[8] = {"OH808B1.raw", "OH808B2.raw", "OH808B3.raw", "OH808B4.raw", "OH808B5.raw", "OH808B6.raw", "OH808B7.raw", "OH808B8.raw"};
const char* MAR808arr[8] = {"MAR8081.raw", "MAR8082.raw", "MAR8083.raw", "MAR8084.raw", "MAR8085.raw", "MAR8086.raw", "MAR8087.raw", "MAR8088.raw"};
const char* CYM808Aarr[4] = {"CYM808A1.raw", "CYM808A2.raw", "CYM808A3.raw", "CYM808A4.raw"};
const char* CYM808Barr[4] = {"CYM808B1.raw", "CYM808B2.raw", "CYM808B3.raw", "CYM808B4.raw"};
const int HT808Aarrs = 4, HT808Barrs = 4, MT808Aarrs = 4, MT808Barrs = 4, MT808Carrs = 4, LT808Aarrs = 4, LT808Barrs = 4, LC808Aarrs = 4, LC808Barrs = 4, LC808Carrs = 4, LC808Darrs = 4, MC808Aarrs = 4, MC808Barrs = 4, MC808Carrs = 4;
const char* HT808Aarr[4] = {"HT808A1.raw", "HT808A2.raw", "HT808A3.raw", "HT808A4.raw"};
const char* HT808Barr[4] = {"HT808B1.raw", "HT808B2.raw", "HT808B3.raw", "HT808B4.raw"};
const char* MT808Aarr[4] = {"MT808A1.raw", "MT808A2.raw", "MT808A3.raw", "MT808A4.raw"};
const char* MT808Barr[4] = {"MT808B1.raw", "MT808B2.raw", "MT808B3.raw", "MT808B4.raw"};
const char* MT808Carr[4] = {"MT808C1.raw", "MT808C2.raw", "MT808C3.raw", "MT808C4.raw"};
const char* LT808Aarr[4] = {"LT808A1.raw", "LT808A2.raw", "LT808A3.raw", "LT808A4.raw"};
const char* LT808Barr[4] = {"LT808B1.raw", "LT808B2.raw", "LT808B3.raw", "LT808B4.raw"};
const char* LC808Aarr[4] = {"LC808A1.raw", "LC808A2.raw", "LC808A3.raw", "LC808A4.raw"};
const char* LC808Barr[4] = {"LC808B1.raw", "LC808B2.raw", "LC808B3.raw", "LC808B4.raw"};
const char* LC808Carr[4] = {"LC808C1.raw", "LC808C2.raw", "LC808C3.raw", "LC808C4.raw"};
const char* LC808Darr[4] = {"LC808D1.raw", "LC808D2.raw", "LC808D3.raw", "LC808D4.raw"};
const char* MC808Aarr[4] = {"MC808A1.raw", "MC808A2.raw", "MC808A3.raw", "MC808A4.raw"};
const char* MC808Barr[4] = {"MC808B1.raw", "MC808B2.raw", "MC808B3.raw", "MC808B4.raw"};
const char* MC808Carr[4] = {"MC808C1.raw", "MC808C2.raw", "MC808C3.raw", "MC808C4.raw"};
//LINN
const int BDLINNarrs = 3, BNGLINNarrs = 2, HHLINNarrs = 3, LINNHHBs = 1, LINNCRASHs = 1, OHBLINNarrs = 2, TMBLINNarrs = 2, OHLINNarrs = 3, RIMLINNarrs = 4, RIDLINNarrs = 3, SDLINNarrs = 3;
const char* BDLINNarr[3] = {"LINNBD1.raw", "LINNBD2.raw", "LINNBD3.raw"};
const char* BNGLINNarr[2] = {"LINNBNG1.raw", "LINNBNG2.raw"};
const char* HHLINNarr[3] = {"LINNHH1.raw", "LINNHH2.raw", "LINNHH3.raw"};
const char* LINNHHB[1] = {"LINNHHB1.raw"};
const char* LINNCRASH[1] = {"LINNCRSH.raw"};
const char* OHBLINNarr[2] = {"LINNOHB1.raw", "LINNOHB2.raw"};
const char* TMBLINNarr[2] = {"LINNTMB1.raw", "LINNTMB2.raw"};
const char* OHLINNarr[3] = {"LINNOH1.raw", "LINNOH2.raw", "LINNOH3.raw"};
const char* RIMLINNarr[4] = {"LINNRIM1.raw", "LINNRIM2.raw", "LINNRIM3.raw", "LINNRIM4.raw"};
const char* RIDLINNarr[3] = {"LINNRID1.raw", "LINNRID2.raw", "LINNRID3.raw"};
const char* SDLINNarr[3] = {"LINNSD1.raw", "LINNSD2.raw", "LINNSD3.raw"};
//const char* CONGALINNarr[6] = {"LINNCGLL.raw", "LINNCOGM.raw","LINNCGLM.raw", "LINNCGLLMM.raw", "LINNCGMH.raw", "LINNCGH.raw"};
const int CONGLLINNarrs = 2, CONGMLINNarrs = 2, CONGHLINNarrs = 2, TOMLINNarrs = 6, CABLINNarrs = 2, COWLINNarrs = 2, CLAPLINNarrs = 3, OH909Aarrs = 6;
const char* CONGLLINNarr[2] = {"LINNCGLL.raw", "LINNCGLM.raw"};
const char* CONGMLINNarr[2] = {"LINNCGLM.raw", "LINNCGMM.raw"};
const char* CONGHLINNarr[2] = {"LINNCGMH.raw", "LINNCGH.raw"};
const char* TOMLINNarr[6] = {"LINNTOLL.raw", "LINNTOL.raw", "LINNTOLM.raw", "LINNTOM.raw", "LINNTOMH.raw", "LINNTOH.raw"};
const char* CABLINNarr[2] = {"LINNCAB1.raw", "LINNCAB2.raw"};
const char* COWLINNarr[2] = {"LINNCOW1.raw", "LINNCOW2.raw"};
const char* CLAPLINNarr[3] = {"LINNCLP1.raw", "LINNCLP2.raw", "LINNCLP3.raw"};
//
const char* OH909Aarr[6] = {"OH909A1.raw", "OH909A2.raw", "OH909A3.raw", "OH909A4.raw", "OH909A5.raw", "OH909A6.raw"};
const int KANOBLarrs = 8, KANOBRarrs = 8, KANOarrs = 8;
const char* KANOBLarr[8] = {"kanoBl1.raw", "kanoBl2.raw", "kanoBl3.raw", "kanoBl4.raw", "kanoBl5.raw", "kanoBl6.raw", "kanoBl7.raw", "kanoBl8.raw"};
const char* KANOBRarr[8] = {"kanoBr1.raw", "kanoBr2.raw", "kanoBr3.raw", "kanoBr4.raw", "kanoBr5.raw", "kanoBr6.raw", "kanoBr7.raw", "kanoBr8.raw"};
const char* KANOarr[8] = {"kanoA1.raw", "kanoA2.raw", "kanoA3.raw", "kanoA4.raw", "kanoA5.raw", "kanoA6.raw", "kanoA7.raw", "kanoA8.raw"};
//const char* RIMarr[6] = {"RIM1.raw","RIM2.raw","RIM3.raw","RIM4.raw","RIM5.raw","RIM6.raw"};
const int LUDBDs = 1, BDBANs = 1, BDMOS1s = 1, BDMOS2s = 1, SDKRIarrs = 8, OHKRIarrs = 8, LUDSDs = 1, LUDOHs = 1, LUDHHarrs = 6;
const char* LUDBD[1] = {"BD1.raw"};
const char* BDBAN[1] = {"BDban1.raw"};
const char* BDMOS1[1] = {"BDmos1.raw"};
const char* BDMOS2[1] = {"BDmos2.raw"};
const char* SDKRIarr[8] = {"SDkri1.raw", "SDkri2.raw", "SDkri3.raw", "SDkri4.raw", "SDkri5.raw", "SDkri6.raw", "SDkri7.raw", "SDkri8.raw"};
const char* OHKRIarr[8] = {"OHkri1.raw", "OHkri2.raw", "OHkr3.raw", "OHkri4.raw", "OHkri5.raw", "OHkri6.raw", "OHkri7.raw", "OHkri8.raw"};
const char* LUDSD[1] = {"SD1.raw"};
const char* LUDOH[1] = {"OH1.raw"};
const char* LUDHHarr[6] = {"HH1.raw", "HH2.raw", "HH3.raw", "HH4.raw", "HH5.raw", "HH6.raw"};
//const char* MARKRIarr[8] = {"MARKRI1.raw", "MARKRI2.raw", "MARKR3.raw", "MARKRI4.raw", "MARKRI5.raw", "MARKRI6.raw", "MARKRI7.raw", "MARKRI8.raw"};
//const char* MAR909Aarr[6] = {"MAR909A1.raw", "MAR909A2.raw", "MARKR3.raw", "MAR909A4.raw", "MAR909A5.raw", "MAR909A6.raw"};


const char** bdArr[12] = {LUDBD, BDBAN, BDMOS1, BDMOS2, BDLINNarr, BD808arr, BD808Larr, CR78BDarr, CR77BD, TR76BDarr, MPOPBDarr, HAMBDarr};
const int bdArrSize[12] = {LUDBDs, BDBANs, BDMOS1s, BDMOS2s, BDLINNarrs, BD808arrs, BD808Larrs, CR78BDarrs, CR77BDs, TR76BDarrs, MPOPBDarrs, HAMBDarrs};

const char** sdArr[12] = {LUDSD, SDKRIarr, SD808arr, SD808Barr, SDLINNarr, MPOPSDarr, TR76SDarr, CR78SDarr, SIMMSDarr, HAMSDAarr, HAMSDBarr, SIMMRIMarr};
const int sdArrSize[12] = {LUDSDs, SDKRIarrs, SD808arrs, SD808Barrs, SDLINNarrs, MPOPSDarrs, TR76SDarrs, CR78SDarrs, SIMMSDarrs, HAMSDAarrs, HAMSDBarrs, SIMMRIMarrs};

const char** hhArr[14] = {LUDHHarr, HHLINNarr, LINNHHB, CH808arr, MPOPHHarr, HAMHHarr, TR76HHarr, CR77HHarr, CR78HHarr, MAR808arr, CABLINNarr, CYM808Aarr, CYM808Barr, RIDLINNarr};
const int hhArrSize[14] = {LUDHHarrs, HHLINNarrs, LINNHHBs, CH808arrs, MPOPHHarrs, HAMHHarrs, TR76HHarrs, CR77HHarrs, CR78HHarrs, MAR808arrs, CABLINNarrs, CYM808Aarrs, CYM808Barrs, RIDLINNarrs};

const char** ohArr[11] = {LUDOH, OHKRIarr, OH808arr, OH808Barr, OH909Aarr, OHLINNarr, OHBLINNarr, MPOPHOarr, HAMHOBarr, TR76HOarr, CR78HOarr};
const int ohArrSize[11] = {LUDOHs, OHKRIarrs, OH808arrs, OH808Barrs, OH909Aarrs, OHLINNarrs, OHBLINNarrs, MPOPHOarrs, HAMHOBarrs, TR76HOarrs, CR78HOarrs};

const char** clapArr[5] = {KANOarr, KANOBRarr, KANOBLarr, CLAPLINNarr, CLAP808arr};
const int clapArrSize[5] = {KANOarrs, KANOBRarrs, KANOBLarrs, CLAPLINNarrs, CLAP808arrs};

const char** perc1Arr[12] = {LC808Aarr, LC808Barr, LC808Carr, LC808Darr, LT808Aarr, LT808Barr, TOMLINNarr, SIMMLTarr, CONGLLINNarr, MPOMXLarr, TR76PER1Carr, CR78RIMarr};
const int perc1ArrSize[12] = {LC808Aarrs, LC808Barrs, LC808Carrs, LC808Darrs, LT808Aarrs, LT808Barrs, TOMLINNarrs, SIMMLTarrs, CONGLLINNarrs, MPOMXLarrs, TR76PER1Carrs, CR78RIMarrs};

const char** perc2Arr[13] = {MC808Aarr, MC808Barr, MC808Carr, MT808Aarr, MT808Barr, MT808Carr, SIMMMTarr, CONGMLINNarr, BNGLINNarr, MPOPCONarr, TR76PER2Carr, CR78BLO1arr, CR78GUIarr};
const int perc2ArrSize[13] = {MC808Aarrs, MC808Barrs, MC808Carrs, MT808Aarrs, MT808Barrs, MT808Carrs, SIMMMTarrs, CONGMLINNarrs, BNGLINNarrs, MPOPCONarrs, TR76PER2Carrs, CR78BLO1arrs, CR78GUIarrs};

const char** perc3Arr[13] = {HCA808arr, HCB808arr, HCC808arr, HCD808arr, HCE808arr, HT808Aarr, HT808Barr, SIMMHTarr, CONGHLINNarr, MPOPCLarr, TR76PER3Carr, CR78BLO2arr, CR78COWarr};
const int perc3ArrSize[13] = {HCA808arrs, HCB808arrs, HCC808arrs, HCD808arrs, HCE808arrs, HT808Aarrs, HT808Barrs, SIMMHTarrs, CONGHLINNarrs, MPOPCLarrs, TR76PER3Carrs, CR78BLO2arrs, CR78COWarrs};
int maxArr[9] = {11, 11, 13, 10, 13, 4, 11, 12, 12};
/*KITS:
  1 = Lud1
  2 = Lud2
  3 = Lud3
  4 = 808
  5 = 909
  6 = Linn
  7 = miniPop
  8 = TR76
  9 = CR77
  10 = CR78
  11 = HAMMOND
*/
