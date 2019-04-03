#include "util/TStringConversion.h"
#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TThingyTable.h"
#include "lastbible/LastBibleScriptReader.h"
#include "lastbible/LastBibleLineWrapper.h"
#include "exception/TGenericException.h"
#include <string>
#include <map>
#include <fstream>
#include <iostream>

using namespace std;
using namespace BlackT;
using namespace Sms;

TThingyTable table;

const static int hashMask = 0x0FFF;

const static int op_tilebr     = 0xEE;
const static int op_br         = 0xFE;
const static int op_terminator = 0xFF;

string getStringName(LastBibleScriptReader::ResultString result) {
//  int bankNum = result.srcOffset / 0x4000;
  return string("string_")
    + TStringConversion::intToString(result.srcOffset,
          TStringConversion::baseHex);
}

void exportRawResults(LastBibleScriptReader::ResultCollection& results,
                      std::string filename) {
  TBufStream ofs(0x10000);
  for (int i = 0; i < results.size(); i++) {
    ofs.write(results[i].str.c_str(), results[i].str.size());
  }
  ofs.save((filename).c_str());
}

void exportRawResults(TStream& ifs,
                      std::string filename) {
  LastBibleScriptReader::ResultCollection results;
  LastBibleScriptReader(ifs, results, table)();
  exportRawResults(results, filename);
}

void exportTabledResults(TStream& ifs,
                         std::string binFilename,
                         LastBibleScriptReader::ResultCollection& results,
                         TBufStream& ofs) {
  int offset = 0;
  for (int i = 0; i < results.size(); i++) {
    ofs.writeu16le(offset + (results.size() * 2));
    offset += results[i].str.size();
  }
  
  for (int i = 0; i < results.size(); i++) {
    ofs.write(results[i].str.c_str(), results[i].str.size());
  }
  
  ofs.save((binFilename).c_str());
}

void exportTabledResults(TStream& ifs,
                         std::string binFilename) {
  LastBibleScriptReader::ResultCollection results;
  LastBibleScriptReader(ifs, results, table)();
  
//  std::ofstream incofs(incFilename.c_str());
  TBufStream ofs(0x10000);
  exportTabledResults(ifs, binFilename, results, ofs);
}

void exportSizeTabledResults(TStream& ifs,
                         std::string binFilename) {
  LastBibleScriptReader::ResultCollection results;
  LastBibleScriptReader(ifs, results, table)();
  
//  std::ofstream incofs(incFilename.c_str());
  TBufStream ofs(0x10000);
  ofs.writeu8(results.size());
  exportTabledResults(ifs, binFilename, results, ofs);
}

void generateHashTable(string infile, string outPrefix, string outName) {
  TBufStream ifs;
//    ifs.open((inPrefix + "script.txt").c_str());
//  ifs.open((outPrefix + "script_wrapped.txt").c_str());
  ifs.open(infile.c_str());
  
  LastBibleScriptReader::ResultCollection results;
  LastBibleScriptReader(ifs, results, table)();
  
//    TBufStream ofs(0x20000);
//    for (unsigned int i = 0; i < results.size(); i++) {
//      ofs.write(results[i].str.c_str(), results[i].str.size());
//    }
//    ofs.save((outPrefix + "script.bin").c_str());
  
  // create:
  // * an individual .bin file for each compiled string
  // * a .inc containing, for each string, one superfree section with an
  //   incbin that includes the corresponding string's .bin
  // * a .inc containing the hash bucket arrays for the remapped strings.
  //   table keys are (orig_pointer & 0x1FFF).
  //   the generated bucket sets go in a single superfree section.
  //   each bucket set is an array of the following structure (terminate
  //   arrays with FF so we can detect missed entries):
  //       struct Bucket {
  //       u8 origBank
  //       u16 origPointer  // respects original slotting!
  //       u8 newBank
  //       u16 newPointer
  //     }
  // * a .inc containing the bucket array start pointers (keys are 16-bit
  //   and range from 0x0000-0x1FFF, so this gets its own bank)
  
  std::ofstream strIncOfs(
    (outPrefix + "strings" + outName + ".inc").c_str());
  std::map<int, LastBibleScriptReader::ResultCollection>
    mappedStringBuckets;
  for (unsigned int i = 0; i < results.size(); i++) {
    std::string stringName = getStringName(results[i]) + outName;
    
    // write string to file
    TBufStream ofs(0x10000);
    ofs.write(results[i].str.c_str(), results[i].str.size());
    ofs.save((outPrefix + "strings/" + stringName + ".bin").c_str());
    
    // add string binary to generated includes
    strIncOfs << ".slot 2" << endl;
    strIncOfs << ".section \"string include " << outName << " "
      << i << "\" superfree"
      << endl;
    strIncOfs << "  " << stringName << ":" << endl;
    strIncOfs << "    " << ".incbin \""
      << outPrefix << "strings/" << stringName << ".bin"
      << "\"" << endl;
    strIncOfs << ".ends" << endl;
    
    // add to map
    mappedStringBuckets[results[i].srcOffset & hashMask]
      .push_back(results[i]);
  }
  
  // generate bucket arrays
  std::ofstream stringHashOfs(
    (outPrefix + "string_bucketarrays" + outName + ".inc").c_str());
  stringHashOfs << ".include \""
    << outPrefix + "strings" + outName + ".inc\""
    << endl;
  stringHashOfs << ".section \"string hash buckets " << outName
    << "\" superfree" << endl;
  stringHashOfs << "  stringHashBuckets" + outName + ":" << endl;
  for (std::map<int, LastBibleScriptReader::ResultCollection>::iterator it
         = mappedStringBuckets.begin();
       it != mappedStringBuckets.end();
       ++it) {
    int key = it->first;
    LastBibleScriptReader::ResultCollection& results = it->second;
    
    stringHashOfs << "  hashBucketArray_"
      << outName
      << TStringConversion::intToString(key,
            TStringConversion::baseHex)
      << ":" << endl;
    
    for (unsigned int i = 0; i < results.size(); i++) {
      LastBibleScriptReader::ResultString result = results[i];
      string stringName = getStringName(result) + outName;
      
      // original bank
      stringHashOfs << "    .db " << result.srcOffset / 0x4000 << endl;
      // original pointer (respecting slotting)
      stringHashOfs << "    .dw "
        << (result.srcOffset & 0x3FFF) + (0x4000 * result.srcSlot)
        << endl;
      // new bank
      stringHashOfs << "    .db :" << stringName << endl;
      // new pointer
      stringHashOfs << "    .dw " << stringName << endl;
    }
    
    // array terminator
    stringHashOfs << "  .db $FF " << endl;
  }
  stringHashOfs << ".ends" << endl;
  
  // generate bucket array hash table
  std::ofstream bucketHashOfs(
    (outPrefix + "string_bucket_hashtable" + outName + ".inc").c_str());
  bucketHashOfs << ".include \""
    << outPrefix + "string_bucketarrays" + outName + ".inc\""
    << endl;
  bucketHashOfs
    << ".section \"bucket array hash table " << outName
      << "\" size $4000 align $4000 superfree"
    << endl;
  bucketHashOfs << "  bucketArrayHashTable" << outName << ":" << endl;
  for (int i = 0; i < hashMask; i++) {
    std::map<int, LastBibleScriptReader::ResultCollection>::iterator findIt
      = mappedStringBuckets.find(i);
    if (findIt != mappedStringBuckets.end()) {
      int key = findIt->first;
      // bucket bank
      bucketHashOfs << "    .db :hashBucketArray_" + outName
        << TStringConversion::intToString(key,
              TStringConversion::baseHex)
        << endl;
      // bucket pointer
      bucketHashOfs << "    .dw hashBucketArray_" + outName
        << TStringConversion::intToString(key,
              TStringConversion::baseHex)
        << endl;
      // reserved
      bucketHashOfs << "    .db $FF"
        << endl;
    }
    else {
      // no array
      bucketHashOfs << "    .db $FF,$FF,$FF,$FF" << endl;
    }
  }
  bucketHashOfs << ".ends" << endl;
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cout << "Last Bible (Game Gear) script builder" << endl;
    cout << "Usage: " << argv[0] << " [inprefix] [thingy] [outprefix]"
      << endl;
    
    return 0;
  }
  
  string inPrefix = string(argv[1]);
  string tableName = string(argv[2]);
  string outPrefix = string(argv[3]);
  
  table.readSjis(tableName);
  
  // wrap script
  {
    // read size table
    LastBibleLineWrapper::CharSizeTable sizeTable;
    {
      TBufStream ifs;
      ifs.open("out/font/sizetable.bin");
      int pos = 0;
      while (!ifs.eof()) {
        sizeTable[pos++] = ifs.readu8();
      }
    }
    
    {
      TBufStream ifs;
//      ifs.open((inPrefix + "dialogue.txt").c_str());
      ifs.open(("out/script/dialogue_all.txt"));
      
      TLineWrapper::ResultCollection results;
      LastBibleLineWrapper(ifs, results, table, sizeTable)();
      
      if (results.size() > 0) {
        TOfstream ofs((outPrefix + "dialogue_wrapped.txt").c_str());
        ofs.write(results[0].str.c_str(), results[0].str.size());
      }
    }
  }
  
  generateHashTable((outPrefix + "dialogue_wrapped.txt"),
                    outPrefix,
                    "dialogue");
  
  {
    TBufStream ifs;
    ifs.open((inPrefix + "enemies.txt").c_str());
    exportTabledResults(ifs, outPrefix + "enemies.bin");
  }
    
  {
    TBufStream ifs;
    ifs.open((inPrefix + "enemies_plural.txt").c_str());
    exportTabledResults(ifs, outPrefix + "enemies_plural.bin");
  }
    
  {
    TBufStream ifs;
    ifs.open((inPrefix + "ordinal_numbers.txt").c_str());
    exportTabledResults(ifs, outPrefix + "ordinal_numbers.bin");
  }
  
  {
    TBufStream ifs;
    ifs.open((inPrefix + "new.txt").c_str());
    
    exportRawResults(ifs, outPrefix + "enemy_appeared_plural.bin");
    exportRawResults(ifs, outPrefix + "theend.bin");
    exportRawResults(ifs, outPrefix + "new_system_menu.bin");
    exportRawResults(ifs, outPrefix + "speech_settings_menu.bin");
    exportRawResults(ifs, outPrefix + "speech_settings_explanation.bin");
    exportRawResults(ifs, outPrefix + "speech_settings_menulabel.bin");
  }
  
  {
    TBufStream ifs;
    ifs.open((inPrefix + "nameentry_table.txt").c_str());
    
    exportRawResults(ifs, outPrefix + "nameentry_table.bin");
  }
  
  {
    TBufStream ifs;
    ifs.open((inPrefix + "charnames_default.txt").c_str());
    
    exportRawResults(ifs, outPrefix + "charnames_default_char0.bin");
    exportRawResults(ifs, outPrefix + "charnames_default_char1.bin");
    exportRawResults(ifs, outPrefix + "charnames_default_char2.bin");
  }
  
  {
    TBufStream ifs;
    ifs.open((inPrefix + "charnames_cheat.txt").c_str());
    
    exportRawResults(ifs, outPrefix + "charnames_cheat_char0.bin");
    exportRawResults(ifs, outPrefix + "charnames_cheat_char1.bin");
    exportRawResults(ifs, outPrefix + "charnames_cheat_char2.bin");
    exportRawResults(ifs, outPrefix + "charnames_cheat_soundtest.bin");
  }
  
  // tilemaps/new
/*  {
    TBufStream ifs;
    ifs.open((inPrefix + "tilemaps.txt").c_str());
    
    exportRawResults(ifs, outPrefix + "roulette_right.bin");
    exportRawResults(ifs, outPrefix + "roulette_wrong.bin");
    exportRawResults(ifs, outPrefix + "roulette_timeup.bin");
    exportRawResults(ifs, outPrefix + "roulette_perfect.bin");
    exportRawResults(ifs, outPrefix + "roulette_blank.bin");
    
    exportRawResults(ifs, outPrefix + "mainmenu_help.bin");
    
    exportSizeTabledResults(ifs, outPrefix + "credits.bin");
  } */
  
  return 0;
}

