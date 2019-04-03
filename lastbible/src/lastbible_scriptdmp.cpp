#include "util/TStringConversion.h"
#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TThingyTable.h"
#include "exception/TGenericException.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;
using namespace BlackT;

const static int op_clear      = 0xF4;
const static int op_wait       = 0xF1;
const static int op_wait2      = 0xF6;
const static int op_br         = 0xF2;
const static int op_scroll     = 0xF9;
const static int op_scroll2    = 0xFA;
const static int op_terminator = 0xFF;
const static int op_textspeed  = 0xF3;

const static int op_waitscroll = 0xF1F9;
const static int op_waitclear  = 0xF1F4;
const static int op_waitend    = 0xF1FF;

const static int op_brscroll   = 0xF2F9;
const static int op_brscroll2  = 0xF2FA;

//const static int op_wait2scroll = 0xF6F9;
const static int op_wait2scroll2 = 0xF6FA;
const static int op_wait2clear = 0xF6F4;
const static int op_wait2end   = 0xF6FF;

string as2bHex(int num) {
  string str = TStringConversion::intToString(num,
                  TStringConversion::baseHex).substr(2, string::npos);
  while (str.size() < 2) str = string("0") + str;
  
  return "<$" + str + ">";
}

void outputComment(std::ostream& ofs,
               string comment = "") {
  if (comment.size() > 0) {
    ofs << "//=======================================" << endl;
    ofs << "// " << comment << endl;
    ofs << "//=======================================" << endl;
    ofs << endl;
  }
}

void dumpString(TStream& ifs, std::ostream& ofs, const TThingyTable& table,
              int offset, int slot,
              string comment = "",
              bool screenStart = false) {
  ifs.seek(offset);
  
  std::ostringstream oss;
  
  if (comment.size() > 0)
    oss << "// " << comment << endl;
  
  if (screenStart) {
             // strings are frequently preceded by さ
/*      if ((result.id == 0x15)
             // numbers
             || (result.id < 0x0B) 
             // symbols
             || ((result.id >= 0x81) && (result.id <= 0x8B))
             || ((result.id >= 0x8D) && (result.id <= 0x8D))
             
             ) {
      oss << "omitting: " << resultStr << std::endl << std::endl;
      oss << "// ";
      ++offset;
      continue;
    }
    else {
      startScreened = true;
    } */
    
    // 0x15 is probably the standard "start dialogue" command.
    // search for it, and if it's not found, just do the whole thing.
    int start = ifs.tell();
    while (true) {
      TThingyTable::MatchResult result = table.matchId(ifs);
      if ((result.id == op_terminator)
          || (result.id == op_waitend)) {
//        oss << "// no 0x15 found: dumping whole string"
//          << std::endl << std::endl;
        ifs.seek(start);
        break;
      }
      else if ((result.id == 0x15)) {
//        oss << "// skipped " << ifs.tell() - start << " char(s)"
//          << std::endl << std::endl;
        offset = ifs.tell();
        break;
      }
    }
  }
  
  // comment out first line of original text
  oss << "// ";
  while (true) {
    
    TThingyTable::MatchResult result = table.matchId(ifs);
    if (result.id == -1) {
      throw TGenericException(T_SRCANDLINE,
                              "dumpString(TStream&, std::ostream&)",
                              string("At offset ")
                                + TStringConversion::intToString(
                                    ifs.tell(),
                                    TStringConversion::baseHex)
                                + ": unknown character '"
                                + TStringConversion::intToString(
                                    (unsigned char)ifs.peek(),
                                    TStringConversion::baseHex)
                                + "'");
    }
    
    string resultStr = table.getEntry(result.id);
    oss << resultStr;
    
    if ((result.id == op_terminator)
        || (result.id == op_waitend)
        || (result.id == op_wait2end)) {
//      oss << endl;
      oss << endl << endl;
      oss << resultStr;
      break;
    }
    else if ((result.id == op_br)
             || (result.id == op_brscroll)
             || (result.id == op_brscroll2)) {
      oss << endl;
      oss << "// ";
    }
    else if (result.id == op_clear) {
//      oss << endl;
//      oss << "// ";
      oss << endl << endl;
      oss << resultStr;
      oss << endl << endl;
      oss << "// ";
    }
    else if ((result.id == op_wait)
             || (result.id == op_waitclear)
             || (result.id == op_wait2clear)) {
      oss << endl << endl;
      oss << resultStr;
      oss << endl << endl;
      oss << "// ";
    }
    // do not output scroll commands to the preformatted text -- we have no
    // guarantee they will be valid for the translation
    else if ((result.id == op_waitscroll)) {
      oss << endl << endl;
      oss << table.getEntry(op_wait);
      oss << endl << endl;
      oss << "// ";
      continue;
    }
    else if ((result.id == op_wait2scroll2)) {
      oss << endl << endl;
      oss << table.getEntry(op_wait2);
      oss << endl << endl;
      oss << "// ";
      continue;
    }
    else if (result.id == op_textspeed) {
      oss << as2bHex(ifs.readu8());
    }
  }
  
  ofs << "#STARTMSG("
      // offset
      << TStringConversion::intToString(
          offset, TStringConversion::baseHex)
      << ", "
      // size
      << TStringConversion::intToString(
          ifs.tell() - offset, TStringConversion::baseDec)
      << ", "
      // slot num
      << TStringConversion::intToString(
          slot, TStringConversion::baseDec)
      << ")" << endl << endl;
  
  ofs << oss.str();
  
//  oss << endl;
  ofs << endl << endl;
//  ofs << "//   end pos: "
//      << TStringConversion::intToString(
//          ifs.tell(), TStringConversion::baseHex)
//      << endl;
//  ofs << "//   size: " << ifs.tell() - offset << endl;
/*      int answerTableAddr = ifs.tell();
      int answerTablePointer = (answerTableAddr % 0x4000) + 0x8000;
      int answerPointerHigh = (answerTablePointer & 0xFF00) >> 8;
      int answerPointerLow = (answerTablePointer & 0xFF);
      ofs << as2bHex(answerPointerLow) << as2bHex(answerPointerHigh) << endl; */
  ofs << endl;
  ofs << "#ENDMSG()";
  ofs << endl << endl;
}

void dumpStringSet(TStream& ifs, std::ostream& ofs, const TThingyTable& table,
               int startOffset, int slot,
               int numStrings,
               string comment = "",
               bool screenStart = false) {
  if (comment.size() > 0) {
    ofs << "//=======================================" << endl;
    ofs << "// " << comment << endl;
    ofs << "//=======================================" << endl;
    ofs << endl;
  }
  
  ifs.seek(startOffset);
  for (int i = 0; i < numStrings; i++) {
    ofs << "// substring " << i << endl;
    dumpString(ifs, ofs, table, ifs.tell(), slot, "", screenStart);
  }
}

void dumpTilemap(TStream& ifs, std::ostream& ofs, int offset, int slot,
              TThingyTable& tbl, int w, int h,
              bool isHalved = true,
              string comment = "") {
  ifs.seek(offset);
  
  std::ostringstream oss;
  
  if (comment.size() > 0)
    oss << "// " << comment << endl;
  
  // comment out first line of original text
  oss << "// ";
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
    
//      TThingyTable::MatchResult result = tbl.matchId(ifs);
      
      TByte next = ifs.get();
      if (!tbl.hasEntry(next)) {
        throw TGenericException(T_SRCANDLINE,
                                "dumpTilemap()",
                                string("At offset ")
                                  + TStringConversion::intToString(
                                      ifs.tell() - 1,
                                      TStringConversion::baseHex)
                                  + ": unknown character '"
                                  + TStringConversion::intToString(
                                      (unsigned char)next,
                                      TStringConversion::baseHex)
                                  + "'");
      }
      
//      string resultStr = tbl.getEntry(result.id);
      string resultStr = tbl.getEntry(next);
      oss << resultStr;
      
      if (!isHalved) ifs.get();
    }
    
    // end of line
    oss << endl;
    oss << "// ";
  }
  
//  oss << endl << endl << "[end]";
  
  ofs << "#STARTMSG("
      // offset
      << TStringConversion::intToString(
          offset, TStringConversion::baseHex)
      << ", "
      // size
      << TStringConversion::intToString(
          ifs.tell() - offset, TStringConversion::baseDec)
      << ", "
      // slot num
      << TStringConversion::intToString(
          slot, TStringConversion::baseDec)
      << ")" << endl << endl;
  
  ofs << oss.str();
  
//  oss << endl;
  ofs << endl << endl;
//  ofs << "//   end pos: "
//      << TStringConversion::intToString(
//          ifs.tell(), TStringConversion::baseHex)
//      << endl;
//  ofs << "//   size: " << ifs.tell() - offset << endl;
  ofs << endl;
  ofs << "#ENDMSG()";
  ofs << endl << endl;
}

void dumpTilemapSet(TStream& ifs, std::ostream& ofs, int startOffset, int slot,
               TThingyTable& tbl, int w, int h,
               int numTilemaps,
               bool isHalved = true,
               string comment = "") {
  if (comment.size() > 0) {
    ofs << "//=======================================" << endl;
    ofs << "// " << comment << endl;
    ofs << "//=======================================" << endl;
    ofs << endl;
  }
  
  ifs.seek(startOffset);
  for (int i = 0; i < numTilemaps; i++) {
    ofs << "// tilemap " << i << endl;
    dumpTilemap(ifs, ofs, ifs.tell(), slot, tbl, w, h, isHalved);
  }
}

void dumpMenu(TStream& ifs, std::ostream& ofs,
               TThingyTable& tbl,
               string comment = "") {
  int w = ifs.readu8();
  int numItems = ifs.readu8();
  int unknown1 = ifs.readu8();
  int unknown2 = ifs.readu8();
  
  int slot = (ifs.tell() / 0x4000);
  if (slot > 2) slot = 2;
  
  dumpTilemapSet(ifs, ofs, ifs.tell(), slot, tbl, w, 1, numItems, true, comment);
}

void dumpMenu(TStream& ifs, std::ostream& ofs, int offset,
               TThingyTable& tbl,
               string comment = "") {
  ifs.seek(offset);
  dumpMenu(ifs, ofs, tbl, comment);
}

void dumpMenuFull(TStream& ifs, std::ostream& ofs, int offset,
               TThingyTable& tbl,
               string comment = "") {
  ifs.seek(offset);
  
  int w = ifs.readu8();
  int h = ifs.readu8();
  int x = ifs.readu8();
  int y = ifs.readu8();
  
  int slot = (ifs.tell() / 0x4000);
  if (slot > 2) slot = 2;
  
  dumpMenu(ifs, ofs, tbl, comment);
}

void addComment(std::ostream& ofs, string comment) {
  ofs << "//===========================================================" << endl;
  ofs << "// " << comment << endl;
  ofs << "//===========================================================" << endl;
  ofs << endl;
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    cout << "Last Bible (Game Gear) script dumper" << endl;
    cout << "Usage: " << argv[0] << " [rom] [outprefix]" << endl;
    
    return 0;
  }
  
  string romName = string(argv[1]);
//  string tableName = string(argv[2]);
  string outPrefix = string(argv[2]);
  
  TBufStream ifs;
  ifs.open(romName.c_str());
  
/*  //==========================================
  // TEMPORARY -- dump text from old English ROM
  //==========================================
  TThingyTable tablestd;
  tablestd.readSjis(string("table/lastbible_old_en.tbl"));
//  TThingyTable tabletiles;
//  tabletiles.readSjis(string("table/lastbible_tiles.tbl"));
  
  {
    std::ofstream ofs((outPrefix + "system_old.txt").c_str(),
                  ios_base::binary);
    
    dumpTilemapSet(ifs, ofs, 0x1A43, 0, tablestd, 8, 1, 7, true, "status effects?");
    
    for (int i = 0; i < 91; i++) {
      dumpTilemapSet(ifs, ofs, 0x2BEA + (i * 9), 0, tablestd, 7, 1, 1, true,
        "spells/??? " + TStringConversion::intToString(i));
    }
    
    for (int i = 0; i < 105; i++) {
      dumpTilemapSet(ifs, ofs, 0x2F1D + (i * 18), 0, tablestd, 7, 1, 1, true,
        "item " + TStringConversion::intToString(i));
    }
    
//    dumpTilemapSet(ifs, ofs, 0x3B53, 2, tablestd, 6, 1, 5, true, "main menu");
//    dumpTilemapSet(ifs, ofs, 0x3B75, 2, tablestd, 5, 1, 3, true, "items menu");
    dumpMenu(ifs, ofs, 0x3B4F, tablestd, "main menu");
//    dumpMenu(ifs, ofs, 0x3B71, tablestd, "items menu");
//    dumpMenu(ifs, ofs, 0x3B84, tablestd, "?");
//    dumpMenu(ifs, ofs, 0x3BA6, tablestd, "?");
    dumpMenu(ifs, ofs, tablestd, "items menu");
    dumpMenu(ifs, ofs, tablestd, "formation menu");
    dumpMenu(ifs, ofs, tablestd, "system menu");
    dumpMenu(ifs, ofs, tablestd, "message speed menu");
    dumpMenu(ifs, ofs, tablestd, "yes/no 1");
    dumpMenu(ifs, ofs, tablestd, "save/load menu");
    dumpMenu(ifs, ofs, tablestd, "new game/continue");
    dumpMenu(ifs, ofs, tablestd, "battle menu");
    dumpMenu(ifs, ofs, tablestd, "?");
    dumpMenu(ifs, ofs, tablestd, "buy/sell");
    dumpMenu(ifs, ofs, tablestd, "yes/no 2");
    dumpMenu(ifs, ofs, tablestd, "fight menu 1");
    dumpMenu(ifs, ofs, tablestd, "fight menu 2");
    dumpMenu(ifs, ofs, tablestd, "?");
    dumpMenu(ifs, ofs, tablestd, "?");
    dumpMenu(ifs, ofs, tablestd, "?");
    
    dumpTilemapSet(ifs, ofs, 0x64EF, 1, tablestd, 7, 1, 8, true, "?");
    
    dumpMenu(ifs, ofs, 0x13C54, tablestd, "naming screen 1");
    dumpMenu(ifs, ofs, tablestd, "naming screen 2");
    dumpMenu(ifs, ofs, tablestd, "naming screen 3");
    dumpMenu(ifs, ofs, tablestd, "naming screen 4");
    dumpMenu(ifs, ofs, 0x13DD0, tablestd, "naming screen 5");
    dumpMenu(ifs, ofs, tablestd, "naming screen 6");
    dumpMenu(ifs, ofs, tablestd, "naming screen 7");
    
    // enemies
    for (int i = 0; i < 115; i++) {
      dumpTilemapSet(ifs, ofs, 0x14C32 + (i * 0x20), 2, tablestd, 7, 1, 1, true,
        "enemy " + TStringConversion::intToString(i));
    }
    
    dumpTilemapSet(ifs, ofs, 0x18192, 2, tablestd, 0x1E, 1, 1, true,
      "? not sure what this is, first few characters may be garbage");
    dumpTilemapSet(ifs, ofs, 0x18354, 2, tablestd, 2, 1, 1, true,
      "character 1 default name");
    dumpTilemapSet(ifs, ofs, 0x18357, 2, tablestd, 3, 1, 1, true,
      "character 2 default name");
    dumpTilemapSet(ifs, ofs, 0x1835B, 2, tablestd, 4, 1, 1, true,
      "character 3 default name");
    
    return 0;
  } */
  
  TThingyTable tablestd;
  tablestd.readSjis(string("table/lastbible.tbl"));
//  TThingyTable tabletiles;
//  tabletiles.readSjis(string("table/lastbible_tiles.tbl"));
  
  {
    std::ofstream ofs((outPrefix + "system.txt").c_str(),
                  ios_base::binary);
    
    dumpTilemapSet(ifs, ofs, 0x1A43, 0, tablestd, 8, 1, 7, true, "status effects?");
    
    for (int i = 0; i < 91; i++) {
      dumpTilemapSet(ifs, ofs, 0x2BEA + (i * 9), 0, tablestd, 7, 1, 1, true,
        "spells/??? " + TStringConversion::intToString(i));
    }
    
    for (int i = 0; i < 105; i++) {
      dumpTilemapSet(ifs, ofs, 0x2F1D + (i * 18), 0, tablestd, 7, 1, 1, true,
        "item " + TStringConversion::intToString(i));
    }
    
/*//    dumpTilemapSet(ifs, ofs, 0x3B53, 2, tablestd, 6, 1, 5, true, "main menu");
//    dumpTilemapSet(ifs, ofs, 0x3B75, 2, tablestd, 5, 1, 3, true, "items menu");
    dumpMenu(ifs, ofs, 0x3B4F, tablestd, "main menu");
//    dumpMenu(ifs, ofs, 0x3B71, tablestd, "items menu");
//    dumpMenu(ifs, ofs, 0x3B84, tablestd, "?");
//    dumpMenu(ifs, ofs, 0x3BA6, tablestd, "?");
    dumpMenu(ifs, ofs, tablestd, "items menu");
    dumpMenu(ifs, ofs, tablestd, "formation menu");
    dumpMenu(ifs, ofs, tablestd, "system menu");
    dumpMenu(ifs, ofs, tablestd, "message speed menu");
    dumpMenu(ifs, ofs, tablestd, "yes/no 1");
    dumpMenu(ifs, ofs, tablestd, "save/load menu");
    dumpMenu(ifs, ofs, tablestd, "new game/continue");
    dumpMenu(ifs, ofs, tablestd, "battle menu");
    dumpMenu(ifs, ofs, tablestd, "?");
    dumpMenu(ifs, ofs, tablestd, "buy/sell");
    dumpMenu(ifs, ofs, tablestd, "yes/no 2");
    dumpMenu(ifs, ofs, tablestd, "fight menu 1");
    dumpMenu(ifs, ofs, tablestd, "fight menu 2");
    dumpMenu(ifs, ofs, tablestd, "?");
    dumpMenu(ifs, ofs, tablestd, "?");
    dumpMenu(ifs, ofs, tablestd, "?"); */
    
    dumpTilemapSet(ifs, ofs, 0x64EF, 1, tablestd, 7, 1, 8, true, "?");
    
/*    dumpMenu(ifs, ofs, 0x13C54, tablestd, "naming screen 1");
    dumpMenu(ifs, ofs, tablestd, "naming screen 2");
    dumpMenu(ifs, ofs, tablestd, "naming screen 3");
    dumpMenu(ifs, ofs, tablestd, "naming screen 4");
    dumpMenu(ifs, ofs, 0x13DD0, tablestd, "naming screen 5");
    dumpMenu(ifs, ofs, tablestd, "naming screen 6");
    dumpMenu(ifs, ofs, tablestd, "naming screen 7"); */
    
    // enemies
    for (int i = 0; i < 115; i++) {
      dumpTilemapSet(ifs, ofs, 0x14C32 + (i * 0x20), 2, tablestd, 7, 1, 1, true,
        "enemy " + TStringConversion::intToString(i));
    }
    
    dumpTilemapSet(ifs, ofs, 0x18192, 2, tablestd, 0x1E, 1, 1, true,
      "? not sure what this is, first few characters may be garbage");
    dumpTilemapSet(ifs, ofs, 0x18354, 2, tablestd, 2, 1, 1, true,
      "character 1 default name");
    dumpTilemapSet(ifs, ofs, 0x18357, 2, tablestd, 3, 1, 1, true,
      "character 2 default name");
    dumpTilemapSet(ifs, ofs, 0x1835B, 2, tablestd, 4, 1, 1, true,
      "character 3 default name");
  }
  
  // remappable menus
  {
    std::ofstream ofs((outPrefix + "menus.txt").c_str(),
                  ios_base::binary);
    
    dumpMenu(ifs, ofs, 0x3B4F, tablestd, "main menu");
    dumpMenu(ifs, ofs, tablestd, "items menu");
    dumpMenu(ifs, ofs, tablestd, "formation menu");
    dumpMenu(ifs, ofs, tablestd, "system menu");
    dumpMenu(ifs, ofs, tablestd, "message speed menu");
    dumpMenu(ifs, ofs, tablestd, "yes/no 1");
    dumpMenu(ifs, ofs, tablestd, "save/load menu");
    dumpMenu(ifs, ofs, tablestd, "new game/continue");
    dumpMenu(ifs, ofs, tablestd, "battle menu");
    dumpMenu(ifs, ofs, tablestd, "?");
    dumpMenu(ifs, ofs, tablestd, "buy/sell");
    dumpMenu(ifs, ofs, tablestd, "yes/no 2");
    dumpMenu(ifs, ofs, tablestd, "fight menu 1");
    dumpMenu(ifs, ofs, tablestd, "fight menu 2");
    dumpMenu(ifs, ofs, tablestd, "?");
    dumpMenu(ifs, ofs, tablestd, "?");
    dumpMenu(ifs, ofs, tablestd, "?");
    
    dumpMenu(ifs, ofs, 0x13C54, tablestd, "naming screen 1");
    dumpMenu(ifs, ofs, tablestd, "naming screen 2");
    dumpMenu(ifs, ofs, tablestd, "naming screen 3");
    dumpMenu(ifs, ofs, tablestd, "naming screen 4");
    dumpMenu(ifs, ofs, 0x13DD0, tablestd, "naming screen 5");
    dumpMenu(ifs, ofs, tablestd, "naming screen 6");
    dumpMenu(ifs, ofs, tablestd, "naming screen 7");
  }
  
  // dialogue
  {
    std::ofstream ofs((outPrefix + "dialogue.txt").c_str(),
                  ios_base::binary);
    
    addComment(ofs, "common messages");
    dumpStringSet(ifs, ofs, tablestd, 0x1BC2, 0, 1, "got item", false);
    dumpStringSet(ifs, ofs, tablestd, 0x1BE9, 0, 1, "used item", false);
    dumpStringSet(ifs, ofs, tablestd, 0x1C0F, 0, 1, "transferred item", false);
    dumpStringSet(ifs, ofs, tablestd, 0x1C82, 0, 1, "got ally 1?", false);
    dumpStringSet(ifs, ofs, tablestd, 0x247A, 0, 1, "got ally 2?", false);
    dumpStringSet(ifs, ofs, tablestd, 0x2647, 0, 1, "?", false);
    dumpStringSet(ifs, ofs, tablestd, 0x26AF, 0, 1, "learned spell", false);
    
    {
      addComment(ofs, "some story text?");
      dumpStringSet(ifs, ofs, tablestd, 0x8CD7 + 0, 2, 1, "auto-generated string 25", true);
      dumpStringSet(ifs, ofs, tablestd, 0x8D34 + 0, 2, 1, "auto-generated string 26", true);
      dumpStringSet(ifs, ofs, tablestd, 0x8D90 + 0, 2, 1, "auto-generated string 27", true);
      dumpStringSet(ifs, ofs, tablestd, 0x8F48 + 0, 2, 1, "auto-generated string 28", true);
      dumpStringSet(ifs, ofs, tablestd, 0x8FB7 + 0, 2, 1, "auto-generated string 29", true);
      dumpStringSet(ifs, ofs, tablestd, 0x8FD8 + 0, 2, 1, "auto-generated string 30", true);
      dumpStringSet(ifs, ofs, tablestd, 0x9038 + 0, 2, 1, "auto-generated string 31", true);
      dumpStringSet(ifs, ofs, tablestd, 0x907D + 0, 2, 1, "auto-generated string 32", true);
      dumpStringSet(ifs, ofs, tablestd, 0x90C0 + 0, 2, 1, "auto-generated string 33", true);
      dumpStringSet(ifs, ofs, tablestd, 0x90DA + 0, 2, 1, "auto-generated string 34", true);
      dumpStringSet(ifs, ofs, tablestd, 0x90F7 + 0, 2, 1, "auto-generated string 35", true);
      dumpStringSet(ifs, ofs, tablestd, 0x916B + 0, 2, 1, "auto-generated string 36", true);
      dumpStringSet(ifs, ofs, tablestd, 0x91AD + 0, 2, 1, "auto-generated string 37", true);
      dumpStringSet(ifs, ofs, tablestd, 0x91CB + 0, 2, 1, "auto-generated string 38", true);
      dumpStringSet(ifs, ofs, tablestd, 0x91ED + 0, 2, 1, "auto-generated string 39", true);
      dumpStringSet(ifs, ofs, tablestd, 0x9277 + 0, 2, 1, "auto-generated string 40", true);
      dumpStringSet(ifs, ofs, tablestd, 0x9291 + 0, 2, 1, "auto-generated string 41", true);
      dumpStringSet(ifs, ofs, tablestd, 0x93F9 + 0, 2, 1, "auto-generated string 42", true);
      dumpStringSet(ifs, ofs, tablestd, 0x9435 + 0, 2, 1, "auto-generated string 43", true);
      dumpStringSet(ifs, ofs, tablestd, 0x946A + 0, 2, 1, "auto-generated string 44", true);
      dumpStringSet(ifs, ofs, tablestd, 0x9543 + 0, 2, 1, "auto-generated string 45", true);
      dumpStringSet(ifs, ofs, tablestd, 0x957F + 0, 2, 1, "auto-generated string 46", true);
      dumpStringSet(ifs, ofs, tablestd, 0x959D + 0, 2, 1, "auto-generated string 47", true);
      dumpStringSet(ifs, ofs, tablestd, 0x95BF + 0, 2, 1, "auto-generated string 48", true);
      dumpStringSet(ifs, ofs, tablestd, 0x96A5 + 0, 2, 1, "auto-generated string 49", true);
      dumpStringSet(ifs, ofs, tablestd, 0x971B + 0, 2, 1, "auto-generated string 50", true);
      dumpStringSet(ifs, ofs, tablestd, 0x97BA + 0, 2, 1, "auto-generated string 51", true);
//      dumpStringSet(ifs, ofs, tablestd, 0x9A01 + 0, 2, 1, "auto-generated string 52", true);
      dumpStringSet(ifs, ofs, tablestd, 0x9A54 + 0, 2, 1, "auto-generated string 53", true);
      dumpStringSet(ifs, ofs, tablestd, 0x9A83 + 0, 2, 1, "auto-generated string 54", true);
      dumpStringSet(ifs, ofs, tablestd, 0x9D28 + 0, 2, 1, "auto-generated string 59", true);
      dumpStringSet(ifs, ofs, tablestd, 0x9FC0 + 0, 2, 1, "auto-generated string 60", true);
      dumpStringSet(ifs, ofs, tablestd, 0x9FE1 + 0, 2, 1, "auto-generated string 61", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA000 + 0, 2, 1, "auto-generated string 62", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA094 + 0, 2, 1, "auto-generated string 63", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA18D + 0, 2, 1, "auto-generated string 64", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA1BB + 0, 2, 1, "auto-generated string 65", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA22D + 0, 2, 1, "auto-generated string 66", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA239 + 0, 2, 1, "auto-generated string 67", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA259 + 0, 2, 1, "auto-generated string 68", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA27C + 0, 2, 1, "auto-generated string 69", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA28B + 0, 2, 1, "auto-generated string 70", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA2C9 + 0, 2, 1, "auto-generated string 71", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA2D8 + 0, 2, 1, "auto-generated string 72", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA305 + 0, 2, 1, "auto-generated string 73", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA328 + 0, 2, 1, "auto-generated string 74", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA354 + 0, 2, 1, "auto-generated string 75", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA399 + 0, 2, 1, "auto-generated string 76", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA3A5 + 0, 2, 1, "auto-generated string 77", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA74D + 0, 2, 1, "auto-generated string 78", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA79B + 0, 2, 1, "auto-generated string 79", true);
      dumpStringSet(ifs, ofs, tablestd, 0xA9C9 + 0, 2, 1, "auto-generated string 81", true);
      dumpStringSet(ifs, ofs, tablestd, 0xAA70 + 0, 2, 1, "auto-generated string 82", true);
      dumpStringSet(ifs, ofs, tablestd, 0xAC9A + 0, 2, 1, "auto-generated string 83", true);
      dumpStringSet(ifs, ofs, tablestd, 0xACBB + 0, 2, 1, "auto-generated string 84", true);
//      dumpStringSet(ifs, ofs, tablestd, 0xAE50 + 0, 2, 1, "auto-generated string 85", true);
      dumpStringSet(ifs, ofs, tablestd, 0xAE5E + 0, 2, 1, "auto-generated string 86", true);
      dumpStringSet(ifs, ofs, tablestd, 0xAED8 + 0, 2, 1, "auto-generated string 87", true);
      dumpStringSet(ifs, ofs, tablestd, 0xAF6B + 0, 2, 1, "auto-generated string 89", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB090 + 0, 2, 1, "auto-generated string 90", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB0DE + 0, 2, 1, "auto-generated string 92", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB1C8 + 0, 2, 1, "auto-generated string 93", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB297 + 0, 2, 1, "auto-generated string 95", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB342 + 0, 2, 1, "auto-generated string 96", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB3AD + 0, 2, 1, "auto-generated string 98", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB41B + 0, 2, 1, "auto-generated string 99", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB4B8 + 0, 2, 1, "auto-generated string 101", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB579 + 0, 2, 1, "auto-generated string 102", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB5E6 + 0, 2, 1, "auto-generated string 104", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB6A2 + 0, 2, 1, "auto-generated string 105", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB71A + 0, 2, 1, "auto-generated string 107", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB804 + 0, 2, 1, "auto-generated string 108", true);
      dumpStringSet(ifs, ofs, tablestd, 0xB8EE + 0, 2, 1, "auto-generated string 110", true);
      dumpStringSet(ifs, ofs, tablestd, 0xBB0F + 0, 2, 1, "auto-generated string 111", true);
      dumpStringSet(ifs, ofs, tablestd, 0xBE01 + 0, 2, 1, "auto-generated string 115", true);
    }
    
    // TODO: check
    {
      addComment(ofs, "various system/battle messages");
//      dumpStringSet(ifs, ofs, tablestd, 0x10149 + 0, 2, 1, "auto-generated string 0", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10352 + 0, 2, 1, "auto-generated string 1", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1035E + 0, 2, 1, "auto-generated string 2", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10374 + 0, 2, 1, "auto-generated string 3", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1037D + 0, 2, 1, "auto-generated string 4", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1038C + 0, 2, 1, "auto-generated string 5", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1039D + 0, 2, 1, "auto-generated string 6", false);
      dumpStringSet(ifs, ofs, tablestd, 0x103AF + 0, 2, 1, "auto-generated string 7", false);
      dumpStringSet(ifs, ofs, tablestd, 0x103BA + 0, 2, 1, "auto-generated string 8", false);
      dumpStringSet(ifs, ofs, tablestd, 0x103D3 + 0, 2, 1, "auto-generated string 9", false);
      dumpStringSet(ifs, ofs, tablestd, 0x103E4 + 0, 2, 1, "auto-generated string 10", false);
      dumpStringSet(ifs, ofs, tablestd, 0x103EF + 0, 2, 1, "auto-generated string 11", false);
      dumpStringSet(ifs, ofs, tablestd, 0x103FB + 0, 2, 1, "auto-generated string 12", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1040C + 0, 2, 1, "auto-generated string 13", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1041F + 0, 2, 1, "auto-generated string 14", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10435 + 0, 2, 1, "auto-generated string 15", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10444 + 0, 2, 1, "auto-generated string 16", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10453 + 0, 2, 1, "auto-generated string 17", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1045F + 0, 2, 1, "auto-generated string 18", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10473 + 0, 2, 1, "auto-generated string 19", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1048B + 0, 2, 1, "auto-generated string 20", false);
      dumpStringSet(ifs, ofs, tablestd, 0x104A5 + 0, 2, 1, "auto-generated string 21", false);
      dumpStringSet(ifs, ofs, tablestd, 0x104B5 + 0, 2, 1, "auto-generated string 22", false);
      dumpStringSet(ifs, ofs, tablestd, 0x104C8 + 0, 2, 1, "auto-generated string 23", false);
      dumpStringSet(ifs, ofs, tablestd, 0x104DC + 0, 2, 1, "auto-generated string 24", false);
      dumpStringSet(ifs, ofs, tablestd, 0x104E9 + 0, 2, 1, "auto-generated string 25", false);
      dumpStringSet(ifs, ofs, tablestd, 0x104F5 + 0, 2, 1, "auto-generated string 26", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10506 + 0, 2, 1, "auto-generated string 27", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10521 + 0, 2, 1, "auto-generated string 28", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10537 + 0, 2, 1, "auto-generated string 29", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10544 + 0, 2, 1, "auto-generated string 30", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10558 + 0, 2, 1, "auto-generated string 31", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1056E + 0, 2, 1, "auto-generated string 32", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10586 + 0, 2, 1, "auto-generated string 33", false);
      dumpStringSet(ifs, ofs, tablestd, 0x105A4 + 0, 2, 1, "auto-generated string 34", false);
      dumpStringSet(ifs, ofs, tablestd, 0x105C3 + 0, 2, 1, "auto-generated string 35", false);
      dumpStringSet(ifs, ofs, tablestd, 0x105DF + 0, 2, 1, "auto-generated string 36", false);
      dumpStringSet(ifs, ofs, tablestd, 0x105FC + 0, 2, 1, "auto-generated string 37", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1060E + 0, 2, 1, "auto-generated string 38", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1061C + 0, 2, 1, "auto-generated string 39", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10622 + 0, 2, 1, "auto-generated string 40", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10624 + 0, 2, 1, "auto-generated string 41", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10629 + 0, 2, 1, "auto-generated string 42", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1062E + 0, 2, 1, "auto-generated string 43", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1063D + 0, 2, 1, "auto-generated string 44", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1064D + 0, 2, 1, "auto-generated string 45", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1065B + 0, 2, 1, "auto-generated string 46", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10665 + 0, 2, 1, "auto-generated string 47", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1066A + 0, 2, 1, "auto-generated string 48", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10676 + 0, 2, 1, "auto-generated string 49", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10686 + 0, 2, 1, "auto-generated string 50", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10690 + 0, 2, 1, "auto-generated string 51", false);
      dumpStringSet(ifs, ofs, tablestd, 0x106A1 + 0, 2, 1, "auto-generated string 52", false);
      dumpStringSet(ifs, ofs, tablestd, 0x106A6 + 0, 2, 1, "auto-generated string 53", false);
      dumpStringSet(ifs, ofs, tablestd, 0x106B6 + 0, 2, 1, "auto-generated string 54", false);
      dumpStringSet(ifs, ofs, tablestd, 0x106BC + 0, 2, 1, "auto-generated string 55", false);
      dumpStringSet(ifs, ofs, tablestd, 0x106C7 + 0, 2, 1, "auto-generated string 56", false);
      dumpStringSet(ifs, ofs, tablestd, 0x106DC + 0, 2, 1, "auto-generated string 57", false);
      dumpStringSet(ifs, ofs, tablestd, 0x106E6 + 0, 2, 1, "auto-generated string 58", false);
      dumpStringSet(ifs, ofs, tablestd, 0x106F4 + 0, 2, 1, "auto-generated string 59", false);
      dumpStringSet(ifs, ofs, tablestd, 0x106FA + 0, 2, 1, "auto-generated string 60", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10709 + 0, 2, 1, "auto-generated string 61", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10719 + 0, 2, 1, "auto-generated string 62", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10725 + 0, 2, 1, "auto-generated string 63", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1072A + 0, 2, 1, "auto-generated string 64", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10738 + 0, 2, 1, "auto-generated string 65", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1073C + 0, 2, 1, "auto-generated string 66", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1075E + 0, 2, 1, "auto-generated string 67", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10768 + 0, 2, 1, "auto-generated string 68", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1076C + 0, 2, 1, "auto-generated string 69", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10770 + 0, 2, 1, "auto-generated string 70", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10788 + 0, 2, 1, "auto-generated string 71", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10792 + 0, 2, 1, "auto-generated string 72", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1079F + 0, 2, 1, "auto-generated string 73", false);
      dumpStringSet(ifs, ofs, tablestd, 0x107A7 + 0, 2, 1, "auto-generated string 74", false);
      dumpStringSet(ifs, ofs, tablestd, 0x107AF + 0, 2, 1, "auto-generated string 75", false);
      dumpStringSet(ifs, ofs, tablestd, 0x107B7 + 0, 2, 1, "auto-generated string 76", false);
      dumpStringSet(ifs, ofs, tablestd, 0x107BF + 0, 2, 1, "auto-generated string 77", false);
      dumpStringSet(ifs, ofs, tablestd, 0x107C7 + 0, 2, 1, "auto-generated string 78", false);
      dumpStringSet(ifs, ofs, tablestd, 0x107CF + 0, 2, 1, "auto-generated string 79", false);
      dumpStringSet(ifs, ofs, tablestd, 0x107D7 + 0, 2, 1, "auto-generated string 80", false);
      dumpStringSet(ifs, ofs, tablestd, 0x107E1 + 0, 2, 1, "auto-generated string 81", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10806 + 0, 2, 1, "auto-generated string 82", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10813 + 0, 2, 1, "auto-generated string 83", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10835 + 0, 2, 1, "auto-generated string 84", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1084B + 0, 2, 1, "auto-generated string 85", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1085B + 0, 2, 1, "auto-generated string 86", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10871 + 0, 2, 1, "auto-generated string 87", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1088E + 0, 2, 1, "auto-generated string 88", false);
      dumpStringSet(ifs, ofs, tablestd, 0x108AA + 0, 2, 1, "auto-generated string 89", false);
      dumpStringSet(ifs, ofs, tablestd, 0x108C2 + 0, 2, 1, "auto-generated string 90", false);
      dumpStringSet(ifs, ofs, tablestd, 0x108E3 + 0, 2, 1, "auto-generated string 91", false);
      dumpStringSet(ifs, ofs, tablestd, 0x108F2 + 0, 2, 1, "auto-generated string 92", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10907 + 0, 2, 1, "auto-generated string 93", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1091F + 0, 2, 1, "auto-generated string 94", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1092A + 0, 2, 1, "auto-generated string 95", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10935 + 0, 2, 1, "auto-generated string 96", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10948 + 0, 2, 1, "auto-generated string 97", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10960 + 0, 2, 1, "auto-generated string 98", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1098D + 0, 2, 1, "auto-generated string 99", false);
      dumpStringSet(ifs, ofs, tablestd, 0x109BB + 0, 2, 1, "auto-generated string 100", false);
      dumpStringSet(ifs, ofs, tablestd, 0x109CD + 0, 2, 1, "auto-generated string 101", false);
      dumpStringSet(ifs, ofs, tablestd, 0x109DD + 0, 2, 1, "auto-generated string 102", false);
      dumpStringSet(ifs, ofs, tablestd, 0x109E6 + 0, 2, 1, "auto-generated string 103", false);
      dumpStringSet(ifs, ofs, tablestd, 0x109F8 + 0, 2, 1, "auto-generated string 104", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10A0A + 0, 2, 1, "auto-generated string 105", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10A1A + 0, 2, 1, "auto-generated string 106", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10A28 + 0, 2, 1, "auto-generated string 107", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10A36 + 0, 2, 1, "auto-generated string 108", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10A41 + 0, 2, 1, "auto-generated string 109", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10A43 + 0, 2, 1, "auto-generated string 110", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10A52 + 0, 2, 1, "auto-generated string 111", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10A61 + 0, 2, 1, "auto-generated string 112", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10A6F + 0, 2, 1, "auto-generated string 113", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10A7D + 0, 2, 1, "auto-generated string 114", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10A8B + 0, 2, 1, "auto-generated string 115", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10A99 + 0, 2, 1, "auto-generated string 116", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10AA7 + 0, 2, 1, "auto-generated string 117", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10ABF + 0, 2, 1, "auto-generated string 118", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10AD9 + 0, 2, 1, "auto-generated string 119", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10AEB + 0, 2, 1, "auto-generated string 120", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10B04 + 0, 2, 1, "auto-generated string 121", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10B13 + 0, 2, 1, "auto-generated string 122", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10B22 + 0, 2, 1, "auto-generated string 123", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10B2C + 0, 2, 1, "auto-generated string 124", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10B3D + 0, 2, 1, "auto-generated string 125", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10B4F + 0, 2, 1, "auto-generated string 126", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10B5D + 0, 2, 1, "auto-generated string 127", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10B6A + 0, 2, 1, "auto-generated string 128", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10B8F + 0, 2, 1, "auto-generated string 129", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10B9E + 0, 2, 1, "auto-generated string 130", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10BAE + 0, 2, 1, "auto-generated string 131", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10BBF + 0, 2, 1, "auto-generated string 132", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10BD2 + 0, 2, 1, "auto-generated string 133", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10BDD + 0, 2, 1, "auto-generated string 134", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10BF1 + 0, 2, 1, "auto-generated string 135", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10C05 + 0, 2, 1, "auto-generated string 136", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10C17 + 0, 2, 1, "auto-generated string 137", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10C28 + 0, 2, 1, "auto-generated string 138", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10C37 + 0, 2, 1, "auto-generated string 139", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10C47 + 0, 2, 1, "auto-generated string 140", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10C68 + 0, 2, 1, "auto-generated string 141", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10C9B + 0, 2, 1, "auto-generated string 142", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10CA8 + 0, 2, 1, "auto-generated string 143", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10CB3 + 0, 2, 1, "auto-generated string 144", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10CC0 + 0, 2, 1, "auto-generated string 145", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10CCD + 0, 2, 1, "auto-generated string 146", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10CD9 + 0, 2, 1, "auto-generated string 147", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10CE5 + 0, 2, 1, "auto-generated string 148", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10CF2 + 0, 2, 1, "auto-generated string 149", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10D0F + 0, 2, 1, "auto-generated string 150", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10D1A + 0, 2, 1, "auto-generated string 151", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10D2B + 0, 2, 1, "auto-generated string 152", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10D3D + 0, 2, 1, "auto-generated string 153", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10D52 + 0, 2, 1, "auto-generated string 154", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10D68 + 0, 2, 1, "auto-generated string 155", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10D84 + 0, 2, 1, "auto-generated string 156", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10D97 + 0, 2, 1, "auto-generated string 157", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10DAA + 0, 2, 1, "auto-generated string 158", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10DBD + 0, 2, 1, "auto-generated string 159", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10DD7 + 0, 2, 1, "auto-generated string 160", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10DE9 + 0, 2, 1, "auto-generated string 161", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10DFF + 0, 2, 1, "auto-generated string 162", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10E1E + 0, 2, 1, "auto-generated string 163", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10E2F + 0, 2, 1, "auto-generated string 164", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10E44 + 0, 2, 1, "auto-generated string 165", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10E4A + 0, 2, 1, "auto-generated string 166", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10E4E + 0, 2, 1, "auto-generated string 167", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10E58 + 0, 2, 1, "auto-generated string 168", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10E67 + 0, 2, 1, "auto-generated string 169", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10E77 + 0, 2, 1, "auto-generated string 170", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10E8B + 0, 2, 1, "auto-generated string 171", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10EA0 + 0, 2, 1, "auto-generated string 172", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10EB5 + 0, 2, 1, "auto-generated string 173", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10EC5 + 0, 2, 1, "auto-generated string 174", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10ED9 + 0, 2, 1, "auto-generated string 175", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10EE7 + 0, 2, 1, "auto-generated string 176", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10EF1 + 0, 2, 1, "auto-generated string 177", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10EFD + 0, 2, 1, "auto-generated string 178", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10F0C + 0, 2, 1, "auto-generated string 179", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10F16 + 0, 2, 1, "auto-generated string 180", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10F22 + 0, 2, 1, "auto-generated string 181", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10F2E + 0, 2, 1, "auto-generated string 182", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10F3A + 0, 2, 1, "auto-generated string 183", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10F4D + 0, 2, 1, "auto-generated string 184", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10F60 + 0, 2, 1, "auto-generated string 185", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10F73 + 0, 2, 1, "auto-generated string 186", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10F83 + 0, 2, 1, "auto-generated string 187", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10F9A + 0, 2, 1, "auto-generated string 188", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10FAF + 0, 2, 1, "auto-generated string 189", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10FC4 + 0, 2, 1, "auto-generated string 190", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10FDA + 0, 2, 1, "auto-generated string 191", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10FE7 + 0, 2, 1, "auto-generated string 192", false);
      dumpStringSet(ifs, ofs, tablestd, 0x10FF7 + 0, 2, 1, "auto-generated string 193", false);
      addComment(ofs, "labels displayed when a new map is entered?");
      dumpStringSet(ifs, ofs, tablestd, 0x11003 + 0, 2, 1, "auto-generated string 194", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11011 + 0, 2, 1, "auto-generated string 195", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1101E + 0, 2, 1, "auto-generated string 196", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1102D + 0, 2, 1, "auto-generated string 197", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1103C + 0, 2, 1, "auto-generated string 198", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1104B + 0, 2, 1, "auto-generated string 199", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1105B + 0, 2, 1, "auto-generated string 200", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11066 + 0, 2, 1, "auto-generated string 201", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11072 + 0, 2, 1, "auto-generated string 202", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11080 + 0, 2, 1, "auto-generated string 203", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11091 + 0, 2, 1, "auto-generated string 204", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1109D + 0, 2, 1, "auto-generated string 205", false);
      dumpStringSet(ifs, ofs, tablestd, 0x110AA + 0, 2, 1, "auto-generated string 206", false);
      dumpStringSet(ifs, ofs, tablestd, 0x110B7 + 0, 2, 1, "auto-generated string 207", false);
      dumpStringSet(ifs, ofs, tablestd, 0x110C3 + 0, 2, 1, "auto-generated string 208", false);
      dumpStringSet(ifs, ofs, tablestd, 0x110D0 + 0, 2, 1, "auto-generated string 209", false);
      dumpStringSet(ifs, ofs, tablestd, 0x110DD + 0, 2, 1, "auto-generated string 210", false);
      dumpStringSet(ifs, ofs, tablestd, 0x110E8 + 0, 2, 1, "auto-generated string 211", false);
      dumpStringSet(ifs, ofs, tablestd, 0x110F4 + 0, 2, 1, "auto-generated string 212", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11101 + 0, 2, 1, "auto-generated string 213", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1110D + 0, 2, 1, "auto-generated string 214", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1111A + 0, 2, 1, "auto-generated string 215", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11126 + 0, 2, 1, "auto-generated string 216", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11134 + 0, 2, 1, "auto-generated string 217", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11142 + 0, 2, 1, "auto-generated string 218", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1114F + 0, 2, 1, "auto-generated string 219", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1115B + 0, 2, 1, "auto-generated string 220", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11169 + 0, 2, 1, "auto-generated string 221", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11175 + 0, 2, 1, "auto-generated string 222", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11182 + 0, 2, 1, "auto-generated string 223", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1118E + 0, 2, 1, "auto-generated string 224", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1119B + 0, 2, 1, "auto-generated string 225", false);
      dumpStringSet(ifs, ofs, tablestd, 0x111A8 + 0, 2, 1, "auto-generated string 226", false);
      dumpStringSet(ifs, ofs, tablestd, 0x111B5 + 0, 2, 1, "auto-generated string 227", false);
      dumpStringSet(ifs, ofs, tablestd, 0x111C1 + 0, 2, 1, "auto-generated string 228", false);
      dumpStringSet(ifs, ofs, tablestd, 0x111CC + 0, 2, 1, "auto-generated string 229", false);
      dumpStringSet(ifs, ofs, tablestd, 0x111D7 + 0, 2, 1, "auto-generated string 230", false);
      dumpStringSet(ifs, ofs, tablestd, 0x111E2 + 0, 2, 1, "auto-generated string 231", false);
      dumpStringSet(ifs, ofs, tablestd, 0x111ED + 0, 2, 1, "auto-generated string 232", false);
      dumpStringSet(ifs, ofs, tablestd, 0x111FB + 0, 2, 1, "auto-generated string 233", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1120A + 0, 2, 1, "auto-generated string 234", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11215 + 0, 2, 1, "auto-generated string 235", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11222 + 0, 2, 1, "auto-generated string 236", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11231 + 0, 2, 1, "auto-generated string 237", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11240 + 0, 2, 1, "auto-generated string 238", false);
      addComment(ofs, "more system messages");
      dumpStringSet(ifs, ofs, tablestd, 0x1124F + 0, 2, 1, "auto-generated string 239", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11251 + 0, 2, 1, "auto-generated string 240", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11262 + 0, 2, 1, "auto-generated string 241", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1126E + 0, 2, 1, "auto-generated string 242", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1127D + 0, 2, 1, "auto-generated string 243", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11288 + 0, 2, 1, "auto-generated string 244", false);
      dumpStringSet(ifs, ofs, tablestd, 0x112A5 + 0, 2, 1, "auto-generated string 245", false);
      dumpStringSet(ifs, ofs, tablestd, 0x112B3 + 0, 2, 1, "auto-generated string 246", false);
      dumpStringSet(ifs, ofs, tablestd, 0x112C3 + 0, 2, 1, "auto-generated string 247", false);
      dumpStringSet(ifs, ofs, tablestd, 0x112D2 + 0, 2, 1, "auto-generated string 248", false);
      dumpStringSet(ifs, ofs, tablestd, 0x112D8 + 0, 2, 1, "auto-generated string 249", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1130B + 0, 2, 1, "auto-generated string 250", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11328 + 0, 2, 1, "auto-generated string 251", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1133C + 0, 2, 1, "auto-generated string 252", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11341 + 0, 2, 1, "auto-generated string 253", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11346 + 0, 2, 1, "auto-generated string 254", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1134F + 0, 2, 1, "auto-generated string 255", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11361 + 0, 2, 1, "auto-generated string 256", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1136D + 0, 2, 1, "auto-generated string 257", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1138C + 0, 2, 1, "auto-generated string 258", false);
      dumpStringSet(ifs, ofs, tablestd, 0x113AD + 0, 2, 1, "auto-generated string 259", false);
      dumpStringSet(ifs, ofs, tablestd, 0x113C2 + 0, 2, 1, "auto-generated string 260", false);
      dumpStringSet(ifs, ofs, tablestd, 0x113CF + 0, 2, 1, "auto-generated string 261", false);
      dumpStringSet(ifs, ofs, tablestd, 0x113E2 + 0, 2, 1, "auto-generated string 262", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1141B + 0, 2, 1, "auto-generated string 263", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11436 + 0, 2, 1, "auto-generated string 264", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11442 + 0, 2, 1, "auto-generated string 265", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1145D + 0, 2, 1, "auto-generated string 266", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11472 + 0, 2, 1, "auto-generated string 267", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11484 + 0, 2, 1, "auto-generated string 268", false);
      dumpStringSet(ifs, ofs, tablestd, 0x114BC + 0, 2, 1, "auto-generated string 269", false);
      dumpStringSet(ifs, ofs, tablestd, 0x114C8 + 0, 2, 1, "auto-generated string 270", false);
      dumpStringSet(ifs, ofs, tablestd, 0x114DC + 0, 2, 1, "auto-generated string 271", false);
      dumpStringSet(ifs, ofs, tablestd, 0x114F8 + 0, 2, 1, "auto-generated string 272", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11502 + 0, 2, 1, "auto-generated string 273", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1150F + 0, 2, 1, "auto-generated string 274", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11518 + 0, 2, 1, "auto-generated string 275", false);
      addComment(ofs, "more battle messages?");
      dumpStringSet(ifs, ofs, tablestd, 0x11539 + 0, 2, 1, "auto-generated string 276", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11551 + 0, 2, 1, "auto-generated string 277", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11561 + 0, 2, 1, "auto-generated string 278", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1156D + 0, 2, 1, "auto-generated string 279", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1157F + 0, 2, 1, "auto-generated string 280", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1158B + 0, 2, 1, "auto-generated string 281", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11598 + 0, 2, 1, "auto-generated string 282", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1159A + 0, 2, 1, "auto-generated string 283", false);
      dumpStringSet(ifs, ofs, tablestd, 0x115A8 + 0, 2, 1, "auto-generated string 284", false);
      dumpStringSet(ifs, ofs, tablestd, 0x115BE + 0, 2, 1, "auto-generated string 285", false);
      dumpStringSet(ifs, ofs, tablestd, 0x115CE + 0, 2, 1, "auto-generated string 286", false);
      dumpStringSet(ifs, ofs, tablestd, 0x115DE + 0, 2, 1, "auto-generated string 287", false);
      dumpStringSet(ifs, ofs, tablestd, 0x115F0 + 0, 2, 1, "auto-generated string 288", false);
      addComment(ofs, "?");
      dumpStringSet(ifs, ofs, tablestd, 0x11607 + 0, 2, 1, "auto-generated string 289", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11630 + 0, 2, 1, "auto-generated string 290", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1164C + 0, 2, 1, "auto-generated string 291", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1166B + 0, 2, 1, "auto-generated string 292", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1168B + 0, 2, 1, "auto-generated string 293", false);
      dumpStringSet(ifs, ofs, tablestd, 0x116B5 + 0, 2, 1, "auto-generated string 294", false);
      dumpStringSet(ifs, ofs, tablestd, 0x116D0 + 0, 2, 1, "auto-generated string 295", false);
      dumpStringSet(ifs, ofs, tablestd, 0x116E8 + 0, 2, 1, "auto-generated string 296", false);
      dumpStringSet(ifs, ofs, tablestd, 0x116FD + 0, 2, 1, "auto-generated string 297", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11709 + 0, 2, 1, "auto-generated string 298", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11730 + 0, 2, 1, "auto-generated string 299", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1174A + 0, 2, 1, "auto-generated string 300", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11770 + 0, 2, 1, "auto-generated string 301", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11794 + 0, 2, 1, "auto-generated string 302", false);
      dumpStringSet(ifs, ofs, tablestd, 0x117B6 + 0, 2, 1, "auto-generated string 303", false);
      dumpStringSet(ifs, ofs, tablestd, 0x117CC + 0, 2, 1, "auto-generated string 304", false);
      dumpStringSet(ifs, ofs, tablestd, 0x117E4 + 0, 2, 1, "auto-generated string 305", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11807 + 0, 2, 1, "auto-generated string 306", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1183C + 0, 2, 1, "auto-generated string 307", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11867 + 0, 2, 1, "auto-generated string 308", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11889 + 0, 2, 1, "auto-generated string 309", false);
      dumpStringSet(ifs, ofs, tablestd, 0x118A8 + 0, 2, 1, "auto-generated string 310", false);
      dumpStringSet(ifs, ofs, tablestd, 0x118D4 + 0, 2, 1, "auto-generated string 311", false);
      dumpStringSet(ifs, ofs, tablestd, 0x118F1 + 0, 2, 1, "auto-generated string 312", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11921 + 0, 2, 1, "auto-generated string 313", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11951 + 0, 2, 1, "auto-generated string 314", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11970 + 0, 2, 1, "auto-generated string 315", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11988 + 0, 2, 1, "auto-generated string 316", false);
      dumpStringSet(ifs, ofs, tablestd, 0x119AA + 0, 2, 1, "auto-generated string 317", false);
      dumpStringSet(ifs, ofs, tablestd, 0x119BE + 0, 2, 1, "auto-generated string 318", false);
      dumpStringSet(ifs, ofs, tablestd, 0x119E0 + 0, 2, 1, "auto-generated string 319", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11A0E + 0, 2, 1, "auto-generated string 320", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11A28 + 0, 2, 1, "auto-generated string 321", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11A3B + 0, 2, 1, "auto-generated string 322", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11A64 + 0, 2, 1, "auto-generated string 323", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11A78 + 0, 2, 1, "auto-generated string 324", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11A8B + 0, 2, 1, "auto-generated string 325", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11AA4 + 0, 2, 1, "auto-generated string 326", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11AC1 + 0, 2, 1, "auto-generated string 327", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11AE3 + 0, 2, 1, "auto-generated string 328", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11AF6 + 0, 2, 1, "auto-generated string 329", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11B06 + 0, 2, 1, "auto-generated string 330", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11B30 + 0, 2, 1, "auto-generated string 331", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11B55 + 0, 2, 1, "auto-generated string 332", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11B78 + 0, 2, 1, "auto-generated string 333", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11B97 + 0, 2, 1, "auto-generated string 334", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11BBD + 0, 2, 1, "auto-generated string 335", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11BE2 + 0, 2, 1, "auto-generated string 336", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11BF9 + 0, 2, 1, "auto-generated string 337", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11C27 + 0, 2, 1, "auto-generated string 338", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11C40 + 0, 2, 1, "auto-generated string 339", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11C52 + 0, 2, 1, "auto-generated string 340", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11C75 + 0, 2, 1, "auto-generated string 341", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11C89 + 0, 2, 1, "auto-generated string 342", false);
      addComment(ofs, "more battle stuff?");
      dumpStringSet(ifs, ofs, tablestd, 0x11CAC + 0, 2, 1, "auto-generated string 343", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11CBB + 0, 2, 1, "auto-generated string 344", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11CCD + 0, 2, 1, "auto-generated string 345", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11D1A + 0, 2, 1, "auto-generated string 346", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11D87 + 0, 2, 1, "auto-generated string 347", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11DEC + 0, 2, 1, "auto-generated string 348", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11DFE + 0, 2, 1, "auto-generated string 349", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11E11 + 0, 2, 1, "auto-generated string 350", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11E1C + 0, 2, 1, "auto-generated string 351", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11E2D + 0, 2, 1, "auto-generated string 352", false);
      addComment(ofs, "intro");
      dumpStringSet(ifs, ofs, tablestd, 0x11E3D + 0, 2, 1, "auto-generated string 353", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11EBC + 0, 2, 1, "auto-generated string 354", false);
      addComment(ofs, "displayed after waiting on \"the end\" screen for 5 minutes");
      dumpStringSet(ifs, ofs, tablestd, 0x11EE5 + 0, 2, 1, "auto-generated string 355", false);
      addComment(ofs, "ending");
      dumpStringSet(ifs, ofs, tablestd, 0x11EF2 + 0, 2, 1, "auto-generated string 356", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11F13 + 0, 2, 1, "auto-generated string 357", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11F7A + 0, 2, 1, "auto-generated string 358", false);
      dumpStringSet(ifs, ofs, tablestd, 0x11FAD + 0, 2, 1, "auto-generated string 359", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12020 + 0, 2, 1, "auto-generated string 360", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12039 + 0, 2, 1, "auto-generated string 361", false);
      dumpStringSet(ifs, ofs, tablestd, 0x120D6 + 0, 2, 1, "auto-generated string 362", false);
      addComment(ofs, "track titles for the sound test");
      dumpStringSet(ifs, ofs, tablestd, 0x122C4 + 0, 2, 1, "auto-generated string 363", false);
      dumpStringSet(ifs, ofs, tablestd, 0x122CB + 0, 2, 1, "auto-generated string 364", false);
      dumpStringSet(ifs, ofs, tablestd, 0x122D0 + 0, 2, 1, "auto-generated string 365", false);
      dumpStringSet(ifs, ofs, tablestd, 0x122D6 + 0, 2, 1, "auto-generated string 366", false);
      dumpStringSet(ifs, ofs, tablestd, 0x122DC + 0, 2, 1, "auto-generated string 367", false);
      dumpStringSet(ifs, ofs, tablestd, 0x122E0 + 0, 2, 1, "auto-generated string 368", false);
      dumpStringSet(ifs, ofs, tablestd, 0x122E7 + 0, 2, 1, "auto-generated string 369", false);
      dumpStringSet(ifs, ofs, tablestd, 0x122EB + 0, 2, 1, "auto-generated string 370", false);
      dumpStringSet(ifs, ofs, tablestd, 0x122EF + 0, 2, 1, "auto-generated string 371", false);
      dumpStringSet(ifs, ofs, tablestd, 0x122F5 + 0, 2, 1, "auto-generated string 372", false);
      dumpStringSet(ifs, ofs, tablestd, 0x122FA + 0, 2, 1, "auto-generated string 373", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12303 + 0, 2, 1, "auto-generated string 374", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12306 + 0, 2, 1, "auto-generated string 375", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1230E + 0, 2, 1, "auto-generated string 376", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12312 + 0, 2, 1, "auto-generated string 377", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12317 + 0, 2, 1, "auto-generated string 378", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12320 + 0, 2, 1, "auto-generated string 379", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12328 + 0, 2, 1, "auto-generated string 380", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1232F + 0, 2, 1, "auto-generated string 381", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12333 + 0, 2, 1, "auto-generated string 382", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12339 + 0, 2, 1, "auto-generated string 383", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1233E + 0, 2, 1, "auto-generated string 384", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12347 + 0, 2, 1, "auto-generated string 385", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1234C + 0, 2, 1, "auto-generated string 386", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12353 + 0, 2, 1, "auto-generated string 387", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12357 + 0, 2, 1, "auto-generated string 388", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12360 + 0, 2, 1, "auto-generated string 389", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12364 + 0, 2, 1, "auto-generated string 390", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12367 + 0, 2, 1, "auto-generated string 391", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1236E + 0, 2, 1, "auto-generated string 392", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12376 + 0, 2, 1, "auto-generated string 393", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12379 + 0, 2, 1, "auto-generated string 394", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1237E + 0, 2, 1, "auto-generated string 395", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12382 + 0, 2, 1, "auto-generated string 396", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1238A + 0, 2, 1, "auto-generated string 397", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12390 + 0, 2, 1, "auto-generated string 398", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12397 + 0, 2, 1, "auto-generated string 399", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1239B + 0, 2, 1, "auto-generated string 400", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1239E + 0, 2, 1, "auto-generated string 401", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123A3 + 0, 2, 1, "auto-generated string 402", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123A8 + 0, 2, 1, "auto-generated string 403", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123AE + 0, 2, 1, "auto-generated string 404", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123B3 + 0, 2, 1, "auto-generated string 405", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123BA + 0, 2, 1, "auto-generated string 406", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123BF + 0, 2, 1, "auto-generated string 407", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123C3 + 0, 2, 1, "auto-generated string 408", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123C9 + 0, 2, 1, "auto-generated string 409", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123D1 + 0, 2, 1, "auto-generated string 410", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123D9 + 0, 2, 1, "auto-generated string 411", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123E1 + 0, 2, 1, "auto-generated string 412", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123E7 + 0, 2, 1, "auto-generated string 413", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123EF + 0, 2, 1, "auto-generated string 414", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123F4 + 0, 2, 1, "auto-generated string 415", false);
      dumpStringSet(ifs, ofs, tablestd, 0x123F9 + 0, 2, 1, "auto-generated string 416", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12401 + 0, 2, 1, "auto-generated string 417", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12409 + 0, 2, 1, "auto-generated string 418", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12412 + 0, 2, 1, "auto-generated string 419", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12418 + 0, 2, 1, "auto-generated string 420", false);
/*      dumpStringSet(ifs, ofs, tablestd, 0x12445 + 0, 2, 1, "auto-generated string 421", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12473 + 0, 2, 1, "auto-generated string 422", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12478 + 0, 2, 1, "auto-generated string 423", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1247C + 0, 2, 1, "auto-generated string 424", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12481 + 0, 2, 1, "auto-generated string 425", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12484 + 0, 2, 1, "auto-generated string 426", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12488 + 0, 2, 1, "auto-generated string 427", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1248D + 0, 2, 1, "auto-generated string 428", false);
      dumpStringSet(ifs, ofs, tablestd, 0x12490 + 0, 2, 1, "auto-generated string 429", false);
      dumpStringSet(ifs, ofs, tablestd, 0x1249C + 0, 2, 1, "auto-generated string 430", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124A1 + 0, 2, 1, "auto-generated string 431", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124A4 + 0, 2, 1, "auto-generated string 432", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124B2 + 0, 2, 1, "auto-generated string 433", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124B6 + 0, 2, 1, "auto-generated string 434", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124BA + 0, 2, 1, "auto-generated string 435", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124BE + 0, 2, 1, "auto-generated string 436", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124C2 + 0, 2, 1, "auto-generated string 437", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124CA + 0, 2, 1, "auto-generated string 438", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124CE + 0, 2, 1, "auto-generated string 439", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124D2 + 0, 2, 1, "auto-generated string 440", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124D7 + 0, 2, 1, "auto-generated string 441", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124E6 + 0, 2, 1, "auto-generated string 442", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124E9 + 0, 2, 1, "auto-generated string 443", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124ED + 0, 2, 1, "auto-generated string 444", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124F2 + 0, 2, 1, "auto-generated string 445", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124F8 + 0, 2, 1, "auto-generated string 446", false);
      dumpStringSet(ifs, ofs, tablestd, 0x124FD + 0, 2, 1, "auto-generated string 447", false); */
    }
  
    dumpStringSet(ifs, ofs, tablestd, 0x1B36A, 1, 1, "intro text scroll 2", false);
    dumpStringSet(ifs, ofs, tablestd, 0x1B49C, 1, 1, "intro?", false);
    dumpStringSet(ifs, ofs, tablestd, 0x1B503, 1, 1, "intro?", false);
    dumpStringSet(ifs, ofs, tablestd, 0x1B5A2, 1, 1, "intro?", false);
    dumpStringSet(ifs, ofs, tablestd, 0x1B628, 1, 1,
      "intro text scroll 1 -- [charX] = X chars of spaces", false);
    dumpStringSet(ifs, ofs, tablestd, 0x1B71D, 1, 1, "credits", false);
    
    {
      addComment(ofs, "story text");
    //  dumpStringSet(ifs, ofs, tablestd, 0x6C000 + 0, 2, 1, "auto-generated string 0", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6C014 + 0, 2, 1, "auto-generated string 1", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C036 + 2, 2, 1, "auto-generated string 2", false);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6C055 + 0, 2, 1, "auto-generated string 3", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C0F0 + 0, 2, 1, "auto-generated string 4", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C11E + 0, 2, 1, "auto-generated string 5", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C14A + 0, 2, 1, "auto-generated string 6", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6C198 + 0, 2, 1, "auto-generated string 7", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6C1C4 + 0, 2, 1, "auto-generated string 8", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C1E1 + 7, 2, 1, "auto-generated string 9", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6C1F5 + 0, 2, 1, "auto-generated string 10", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C247 + 0, 2, 1, "auto-generated string 11", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C2EA + 0, 2, 1, "auto-generated string 12", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C317 + 0, 2, 1, "auto-generated string 13", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C334 + 7, 2, 1, "auto-generated string 14", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6C348 + 0, 2, 1, "auto-generated string 15", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C3A1 + 0, 2, 1, "auto-generated string 16", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C426 + 0, 2, 1, "auto-generated string 17", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C462 + 0, 2, 1, "auto-generated string 18", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C4D0 + 0, 2, 1, "auto-generated string 19", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C50C + 0, 2, 1, "auto-generated string 20", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C579 + 0, 2, 1, "auto-generated string 21", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C5A0 + 0, 2, 1, "auto-generated string 22", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C5DC + 0, 2, 1, "auto-generated string 23", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C71F + 0, 2, 1, "auto-generated string 24", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C760 + 0, 2, 1, "auto-generated string 25", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C7C6 + 0, 2, 1, "auto-generated string 26", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C807 + 0, 2, 1, "auto-generated string 27", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C86D + 0, 2, 1, "auto-generated string 28", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C895 + 0, 2, 1, "auto-generated string 29", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C8BB + 0, 2, 1, "auto-generated string 30", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C8EA + 0, 2, 1, "auto-generated string 31", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C918 + 0, 2, 1, "auto-generated string 32", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C940 + 20, 2, 1, "auto-generated string 33", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C960 + 0, 2, 1, "auto-generated string 34", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6C988 + 0, 2, 1, "auto-generated string 35", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C99F + 13, 2, 1, "auto-generated string 36", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6C9D1 + 2, 2, 1, "auto-generated string 37", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6C9E4 + 5, 2, 1, "auto-generated string 38", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6C9F5 + 6, 2, 1, "auto-generated string 39", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6CA0D + 5, 2, 1, "auto-generated string 40", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6CA30 + 8, 2, 1, "auto-generated string 41", false);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6CA49 + 0, 2, 1, "auto-generated string 42", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CA52 + 8, 2, 1, "auto-generated string 43", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6CAC3 + 0, 2, 1, "auto-generated string 44", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CAD2 + 0, 2, 1, "auto-generated string 45", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CADD + 0, 2, 1, "auto-generated string 46", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CAEC + 0, 2, 1, "auto-generated string 47", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6CB1D + 0, 2, 1, "auto-generated string 48", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CB4B + 0, 2, 1, "auto-generated string 49", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CB67 + 0, 2, 1, "auto-generated string 50", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CBEA + 0, 2, 1, "auto-generated string 51", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CC3B + 1, 2, 1, "auto-generated string 52", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CC66 + 0, 2, 1, "auto-generated string 53", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CC8E + 0, 2, 1, "auto-generated string 54", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CC98 + 0, 2, 1, "auto-generated string 55", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CCB5 + 2, 2, 1, "auto-generated string 56", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6CCD7 + 0, 2, 1, "auto-generated string 57", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CCF7 + 3, 2, 1, "auto-generated string 58", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6CD12 + 0, 2, 1, "auto-generated string 59", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6CD27 + 0, 2, 1, "auto-generated string 60", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6CDD1 + 7, 2, 1, "auto-generated string 61", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6CE5C + 4, 2, 1, "auto-generated string 62", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6CEEA + 7, 2, 1, "auto-generated string 63", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6CF6F + 10, 2, 1, "auto-generated string 64", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6D002 + 5, 2, 1, "auto-generated string 65", false);
      dumpStringSet(ifs, ofs, tablestd, 0x6D06A + 2, 2, 1, "auto-generated string 66", false);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6D0ED + 0, 2, 1, "auto-generated string 67", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D0FF + 0, 2, 1, "auto-generated string 68", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D124 + 0, 2, 1, "auto-generated string 69", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D1ED + 0, 2, 1, "auto-generated string 70", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D200 + 0, 2, 1, "auto-generated string 71", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D242 + 0, 2, 1, "auto-generated string 72", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D263 + 0, 2, 1, "auto-generated string 73", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D2B6 + 0, 2, 1, "auto-generated string 74", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D2DA + 0, 2, 1, "auto-generated string 75", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D2ED + 0, 2, 1, "auto-generated string 76", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D319 + 0, 2, 1, "auto-generated string 77", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D345 + 0, 2, 1, "auto-generated string 78", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D367 + 0, 2, 1, "auto-generated string 79", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D398 + 0, 2, 1, "auto-generated string 80", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D3DA + 0, 2, 1, "auto-generated string 81", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D3FC + 0, 2, 1, "auto-generated string 82", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D44F + 0, 2, 1, "auto-generated string 83", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D461 + 0, 2, 1, "auto-generated string 84", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D46E + 0, 2, 1, "auto-generated string 85", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D480 + 0, 2, 1, "auto-generated string 86", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D4C5 + 0, 2, 1, "auto-generated string 87", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D4EE + 0, 2, 1, "auto-generated string 88", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D541 + 0, 2, 1, "auto-generated string 89", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D598 + 0, 2, 1, "auto-generated string 90", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D65A + 0, 2, 1, "auto-generated string 91", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D66F + 0, 2, 1, "auto-generated string 92", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D696 + 0, 2, 1, "auto-generated string 93", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D6C0 + 0, 2, 1, "auto-generated string 94", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D6E9 + 0, 2, 1, "auto-generated string 95", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D707 + 0, 2, 1, "auto-generated string 96", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D724 + 0, 2, 1, "auto-generated string 97", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D74F + 0, 2, 1, "auto-generated string 98", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D7B0 + 0, 2, 1, "auto-generated string 99", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D804 + 0, 2, 1, "auto-generated string 100", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D82B + 0, 2, 1, "auto-generated string 101", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D855 + 0, 2, 1, "auto-generated string 102", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D87E + 0, 2, 1, "auto-generated string 103", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D89C + 0, 2, 1, "auto-generated string 104", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D8B5 + 0, 2, 1, "auto-generated string 105", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D8DE + 0, 2, 1, "auto-generated string 106", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D932 + 0, 2, 1, "auto-generated string 107", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D959 + 0, 2, 1, "auto-generated string 108", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D988 + 0, 2, 1, "auto-generated string 109", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D9B7 + 0, 2, 1, "auto-generated string 110", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6D9EF + 0, 2, 1, "auto-generated string 111", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DA10 + 0, 2, 1, "auto-generated string 112", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DA2E + 0, 2, 1, "auto-generated string 113", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DA77 + 0, 2, 1, "auto-generated string 114", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6DABB + 0, 2, 1, "auto-generated string 115", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DABD + 0, 2, 1, "auto-generated string 116", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DB00 + 0, 2, 1, "auto-generated string 117", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DBA9 + 0, 2, 1, "auto-generated string 118", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DBBF + 0, 2, 1, "auto-generated string 119", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DC02 + 0, 2, 1, "auto-generated string 120", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DC1F + 0, 2, 1, "auto-generated string 121", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DC45 + 0, 2, 1, "auto-generated string 122", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DC61 + 0, 2, 1, "auto-generated string 123", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DC8B + 0, 2, 1, "auto-generated string 124", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DCC2 + 0, 2, 1, "auto-generated string 125", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DCEC + 0, 2, 1, "auto-generated string 126", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DD02 + 0, 2, 1, "auto-generated string 127", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DD45 + 0, 2, 1, "auto-generated string 128", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DD62 + 0, 2, 1, "auto-generated string 129", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DD88 + 0, 2, 1, "auto-generated string 130", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DDA4 + 0, 2, 1, "auto-generated string 131", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DDCE + 0, 2, 1, "auto-generated string 132", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DE05 + 0, 2, 1, "auto-generated string 133", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DE2D + 0, 2, 1, "auto-generated string 134", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DE6E + 0, 2, 1, "auto-generated string 135", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DF27 + 0, 2, 1, "auto-generated string 136", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DF79 + 0, 2, 1, "auto-generated string 137", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DF9C + 0, 2, 1, "auto-generated string 138", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6DFDC + 0, 2, 1, "auto-generated string 139", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E007 + 0, 2, 1, "auto-generated string 140", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E022 + 0, 2, 1, "auto-generated string 141", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E11A + 0, 2, 1, "auto-generated string 142", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E149 + 0, 2, 1, "auto-generated string 143", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E161 + 0, 2, 1, "auto-generated string 144", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E17E + 0, 2, 1, "auto-generated string 145", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E193 + 0, 2, 1, "auto-generated string 146", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E1AB + 0, 2, 1, "auto-generated string 147", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E1BE + 0, 2, 1, "auto-generated string 148", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E1CE + 0, 2, 1, "auto-generated string 149", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E1F5 + 0, 2, 1, "auto-generated string 150", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E21F + 0, 2, 1, "auto-generated string 151", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E23E + 0, 2, 1, "auto-generated string 152", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E256 + 0, 2, 1, "auto-generated string 153", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E27A + 0, 2, 1, "auto-generated string 154", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E291 + 0, 2, 1, "auto-generated string 155", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E2B7 + 0, 2, 1, "auto-generated string 156", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E2CA + 0, 2, 1, "auto-generated string 157", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E2EB + 0, 2, 1, "auto-generated string 158", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E309 + 0, 2, 1, "auto-generated string 159", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E333 + 0, 2, 1, "auto-generated string 160", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E357 + 0, 2, 1, "auto-generated string 161", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E376 + 0, 2, 1, "auto-generated string 162", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E394 + 0, 2, 1, "auto-generated string 163", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E40A + 0, 2, 1, "auto-generated string 164", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E427 + 0, 2, 1, "auto-generated string 165", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E43E + 0, 2, 1, "auto-generated string 166", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E462 + 0, 2, 1, "auto-generated string 167", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E475 + 0, 2, 1, "auto-generated string 168", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E4AF + 0, 2, 1, "auto-generated string 169", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E4FB + 0, 2, 1, "auto-generated string 170", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E525 + 0, 2, 1, "auto-generated string 171", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E53A + 0, 2, 1, "auto-generated string 172", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E54E + 0, 2, 1, "auto-generated string 173", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E55E + 0, 2, 1, "auto-generated string 174", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E598 + 0, 2, 1, "auto-generated string 175", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E5E4 + 0, 2, 1, "auto-generated string 176", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E60F + 0, 2, 1, "auto-generated string 177", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E66C + 0, 2, 1, "auto-generated string 178", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E68F + 0, 2, 1, "auto-generated string 179", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E6C2 + 0, 2, 1, "auto-generated string 180", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E6FB + 0, 2, 1, "auto-generated string 181", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E74A + 0, 2, 1, "auto-generated string 182", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E76B + 0, 2, 1, "auto-generated string 183", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E780 + 0, 2, 1, "auto-generated string 184", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E7A7 + 0, 2, 1, "auto-generated string 185", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6E7DA + 0, 2, 1, "auto-generated string 186", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E837 + 0, 2, 1, "auto-generated string 187", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E859 + 0, 2, 1, "auto-generated string 188", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E871 + 0, 2, 1, "auto-generated string 189", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E8AB + 0, 2, 1, "auto-generated string 190", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E8E9 + 0, 2, 1, "auto-generated string 191", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E901 + 0, 2, 1, "auto-generated string 192", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E934 + 0, 2, 1, "auto-generated string 193", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E96D + 0, 2, 1, "auto-generated string 194", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E9BE + 0, 2, 1, "auto-generated string 195", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E9D6 + 0, 2, 1, "auto-generated string 196", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6E9F7 + 0, 2, 1, "auto-generated string 197", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EA0F + 0, 2, 1, "auto-generated string 198", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EA49 + 0, 2, 1, "auto-generated string 199", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EA86 + 0, 2, 1, "auto-generated string 200", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EA9E + 0, 2, 1, "auto-generated string 201", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6EAD3 + 0, 2, 1, "auto-generated string 202", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6EADB + 0, 2, 1, "auto-generated string 203", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6EB05 + 0, 2, 1, "auto-generated string 204", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6EB15 + 0, 2, 1, "auto-generated string 205", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EB88 + 0, 2, 1, "auto-generated string 206", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EBF0 + 0, 2, 1, "auto-generated string 207", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EC1F + 0, 2, 1, "auto-generated string 208", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EC36 + 0, 2, 1, "auto-generated string 209", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EC63 + 0, 2, 1, "auto-generated string 210", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EC83 + 0, 2, 1, "auto-generated string 211", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6ECA3 + 0, 2, 1, "auto-generated string 212", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6ECC3 + 0, 2, 1, "auto-generated string 213", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6ED13 + 0, 2, 1, "auto-generated string 214", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6ED49 + 0, 2, 1, "auto-generated string 215", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EDA1 + 0, 2, 1, "auto-generated string 216", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EDD3 + 0, 2, 1, "auto-generated string 217", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EDE7 + 0, 2, 1, "auto-generated string 218", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EE14 + 0, 2, 1, "auto-generated string 219", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EE32 + 0, 2, 1, "auto-generated string 220", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EE52 + 0, 2, 1, "auto-generated string 221", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EE72 + 0, 2, 1, "auto-generated string 222", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EEA3 + 0, 2, 1, "auto-generated string 223", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EED5 + 0, 2, 1, "auto-generated string 224", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EEE9 + 0, 2, 1, "auto-generated string 225", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EF16 + 0, 2, 1, "auto-generated string 226", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EF34 + 0, 2, 1, "auto-generated string 227", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EF54 + 0, 2, 1, "auto-generated string 228", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EF74 + 0, 2, 1, "auto-generated string 229", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EFA3 + 0, 2, 1, "auto-generated string 230", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EFBC + 0, 2, 1, "auto-generated string 231", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6EFE7 + 0, 2, 1, "auto-generated string 232", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F005 + 0, 2, 1, "auto-generated string 233", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F025 + 0, 2, 1, "auto-generated string 234", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6F045 + 0, 2, 1, "auto-generated string 235", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F0C4 + 0, 2, 1, "auto-generated string 236", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F0DA + 0, 2, 1, "auto-generated string 237", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F0ED + 0, 2, 1, "auto-generated string 238", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F124 + 0, 2, 1, "auto-generated string 239", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F147 + 0, 2, 1, "auto-generated string 240", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F160 + 0, 2, 1, "auto-generated string 241", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F189 + 0, 2, 1, "auto-generated string 242", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F19C + 0, 2, 1, "auto-generated string 243", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F1C5 + 0, 2, 1, "auto-generated string 244", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F22C + 0, 2, 1, "auto-generated string 245", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F263 + 0, 2, 1, "auto-generated string 246", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F27A + 0, 2, 1, "auto-generated string 247", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F292 + 0, 2, 1, "auto-generated string 248", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F2C1 + 0, 2, 1, "auto-generated string 249", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F2E7 + 0, 2, 1, "auto-generated string 250", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F2FC + 0, 2, 1, "auto-generated string 251", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F325 + 0, 2, 1, "auto-generated string 252", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F35C + 0, 2, 1, "auto-generated string 253", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F375 + 0, 2, 1, "auto-generated string 254", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F388 + 0, 2, 1, "auto-generated string 255", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F3B1 + 0, 2, 1, "auto-generated string 256", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F3C6 + 0, 2, 1, "auto-generated string 257", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F421 + 0, 2, 1, "auto-generated string 258", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F470 + 0, 2, 1, "auto-generated string 259", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F4E8 + 0, 2, 1, "auto-generated string 260", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F526 + 0, 2, 1, "auto-generated string 261", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F545 + 0, 2, 1, "auto-generated string 262", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F57B + 0, 2, 1, "auto-generated string 263", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F598 + 0, 2, 1, "auto-generated string 264", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F5BB + 0, 2, 1, "auto-generated string 265", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F5E6 + 0, 2, 1, "auto-generated string 266", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F608 + 0, 2, 1, "auto-generated string 267", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F644 + 0, 2, 1, "auto-generated string 268", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F67A + 0, 2, 1, "auto-generated string 269", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F68F + 0, 2, 1, "auto-generated string 270", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F6DB + 0, 2, 1, "auto-generated string 271", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F71B + 0, 2, 1, "auto-generated string 272", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F75D + 0, 2, 1, "auto-generated string 273", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F79C + 0, 2, 1, "auto-generated string 274", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F7BC + 0, 2, 1, "auto-generated string 275", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F7EC + 0, 2, 1, "auto-generated string 276", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F828 + 0, 2, 1, "auto-generated string 277", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F85E + 0, 2, 1, "auto-generated string 278", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F86C + 0, 2, 1, "auto-generated string 279", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F8BE + 0, 2, 1, "auto-generated string 280", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F8FE + 0, 2, 1, "auto-generated string 281", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F93E + 0, 2, 1, "auto-generated string 282", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F97F + 0, 2, 1, "auto-generated string 283", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F9A9 + 0, 2, 1, "auto-generated string 284", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6F9CF + 0, 2, 1, "auto-generated string 285", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FA0B + 0, 2, 1, "auto-generated string 286", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FA41 + 0, 2, 1, "auto-generated string 287", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FA4F + 0, 2, 1, "auto-generated string 288", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FAA3 + 0, 2, 1, "auto-generated string 289", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FAE1 + 0, 2, 1, "auto-generated string 290", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FB21 + 0, 2, 1, "auto-generated string 291", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x6FB62 + 0, 2, 1, "auto-generated string 292", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FBBD + 0, 2, 1, "auto-generated string 293", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FBDD + 0, 2, 1, "auto-generated string 294", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FBF6 + 0, 2, 1, "auto-generated string 295", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FC19 + 0, 2, 1, "auto-generated string 296", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FC32 + 0, 2, 1, "auto-generated string 297", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FC82 + 0, 2, 1, "auto-generated string 298", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FC92 + 0, 2, 1, "auto-generated string 299", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FCB5 + 0, 2, 1, "auto-generated string 300", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FCCE + 0, 2, 1, "auto-generated string 301", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FD33 + 0, 2, 1, "auto-generated string 302", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FD47 + 0, 2, 1, "auto-generated string 303", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FDAD + 0, 2, 1, "auto-generated string 304", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FDD3 + 0, 2, 1, "auto-generated string 305", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FE21 + 0, 2, 1, "auto-generated string 306", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FE36 + 0, 2, 1, "auto-generated string 307", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FE4B + 0, 2, 1, "auto-generated string 308", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FE74 + 0, 2, 1, "auto-generated string 309", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FE87 + 0, 2, 1, "auto-generated string 310", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FEC2 + 0, 2, 1, "auto-generated string 311", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FEE6 + 0, 2, 1, "auto-generated string 312", true);
      dumpStringSet(ifs, ofs, tablestd, 0x6FF29 + 0, 2, 1, "auto-generated string 313", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x70000 + 0, 2, 1, "auto-generated string 314", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70050 + 0, 2, 1, "auto-generated string 315", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70100 + 0, 2, 1, "auto-generated string 316", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70158 + 0, 2, 1, "auto-generated string 317", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7019F + 0, 2, 1, "auto-generated string 318", true);
      dumpStringSet(ifs, ofs, tablestd, 0x701C4 + 0, 2, 1, "auto-generated string 319", true);
      dumpStringSet(ifs, ofs, tablestd, 0x701E7 + 0, 2, 1, "auto-generated string 320", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7020C + 0, 2, 1, "auto-generated string 321", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7025A + 0, 2, 1, "auto-generated string 322", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70288 + 0, 2, 1, "auto-generated string 323", true);
      dumpStringSet(ifs, ofs, tablestd, 0x702EB + 0, 2, 1, "auto-generated string 324", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7033C + 0, 2, 1, "auto-generated string 325", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7036A + 0, 2, 1, "auto-generated string 326", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7038B + 0, 2, 1, "auto-generated string 327", true);
      dumpStringSet(ifs, ofs, tablestd, 0x703A1 + 0, 2, 1, "auto-generated string 328", true);
      dumpStringSet(ifs, ofs, tablestd, 0x703F2 + 0, 2, 1, "auto-generated string 329", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70402 + 0, 2, 1, "auto-generated string 330", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70418 + 0, 2, 1, "auto-generated string 331", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70467 + 0, 2, 1, "auto-generated string 332", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70477 + 0, 2, 1, "auto-generated string 333", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70493 + 0, 2, 1, "auto-generated string 334", true);
      dumpStringSet(ifs, ofs, tablestd, 0x704E7 + 0, 2, 1, "auto-generated string 335", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7053D + 0, 2, 1, "auto-generated string 336", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x705B5 + 0, 2, 1, "auto-generated string 337", true);
      dumpStringSet(ifs, ofs, tablestd, 0x705C2 + 0, 2, 1, "auto-generated string 338", true);
      dumpStringSet(ifs, ofs, tablestd, 0x705EB + 0, 2, 1, "auto-generated string 339", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70625 + 0, 2, 1, "auto-generated string 340", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7064B + 0, 2, 1, "auto-generated string 341", true);
      dumpStringSet(ifs, ofs, tablestd, 0x706AA + 0, 2, 1, "auto-generated string 342", true);
      dumpStringSet(ifs, ofs, tablestd, 0x706D0 + 0, 2, 1, "auto-generated string 343", true);
      dumpStringSet(ifs, ofs, tablestd, 0x706F1 + 0, 2, 1, "auto-generated string 344", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70738 + 0, 2, 1, "auto-generated string 345", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7078D + 0, 2, 1, "auto-generated string 346", true);
      dumpStringSet(ifs, ofs, tablestd, 0x707B7 + 0, 2, 1, "auto-generated string 347", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70834 + 0, 2, 1, "auto-generated string 348", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70856 + 0, 2, 1, "auto-generated string 349", true);
      dumpStringSet(ifs, ofs, tablestd, 0x708B3 + 0, 2, 1, "auto-generated string 350", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70923 + 0, 2, 1, "auto-generated string 351", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70937 + 0, 2, 1, "auto-generated string 352", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70951 + 0, 2, 1, "auto-generated string 353", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7097F + 0, 2, 1, "auto-generated string 354", true);
      dumpStringSet(ifs, ofs, tablestd, 0x709C5 + 0, 2, 1, "auto-generated string 355", true);
      dumpStringSet(ifs, ofs, tablestd, 0x709DB + 0, 2, 1, "auto-generated string 356", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70A09 + 0, 2, 1, "auto-generated string 357", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70A2D + 0, 2, 1, "auto-generated string 358", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x70A44 + 0, 2, 1, "auto-generated string 359", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70A83 + 0, 2, 1, "auto-generated string 360", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70AAB + 0, 2, 1, "auto-generated string 361", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70AC8 + 0, 2, 1, "auto-generated string 362", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70B04 + 0, 2, 1, "auto-generated string 363", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70B26 + 0, 2, 1, "auto-generated string 364", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70B46 + 0, 2, 1, "auto-generated string 365", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70B82 + 0, 2, 1, "auto-generated string 366", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70BA4 + 0, 2, 1, "auto-generated string 367", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x70BCC + 0, 2, 1, "auto-generated string 368", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70C0A + 0, 2, 1, "auto-generated string 369", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70C3D + 0, 2, 1, "auto-generated string 370", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70C6B + 0, 2, 1, "auto-generated string 371", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70C98 + 0, 2, 1, "auto-generated string 372", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70CC6 + 0, 2, 1, "auto-generated string 373", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70CD6 + 0, 2, 1, "auto-generated string 374", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70CE6 + 0, 2, 1, "auto-generated string 375", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70CF6 + 0, 2, 1, "auto-generated string 376", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x70D26 + 0, 2, 1, "auto-generated string 377", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70D31 + 0, 2, 1, "auto-generated string 378", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70D68 + 0, 2, 1, "auto-generated string 379", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70DC9 + 0, 2, 1, "auto-generated string 380", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70E17 + 0, 2, 1, "auto-generated string 381", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x70E55 + 0, 2, 1, "auto-generated string 382", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70EAF + 0, 2, 1, "auto-generated string 383", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70EC3 + 0, 2, 1, "auto-generated string 384", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70EE1 + 0, 2, 1, "auto-generated string 385", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70F22 + 0, 2, 1, "auto-generated string 386", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70F5E + 0, 2, 1, "auto-generated string 387", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70F7B + 0, 2, 1, "auto-generated string 388", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x70FB1 + 0, 2, 1, "auto-generated string 389", true);
      dumpStringSet(ifs, ofs, tablestd, 0x70FC9 + 0, 2, 1, "auto-generated string 390", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71042 + 0, 2, 1, "auto-generated string 391", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7105E + 0, 2, 1, "auto-generated string 392", true);
      dumpStringSet(ifs, ofs, tablestd, 0x710F0 + 0, 2, 1, "auto-generated string 393", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71100 + 0, 2, 1, "auto-generated string 394", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7110C + 0, 2, 1, "auto-generated string 395", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7116B + 0, 2, 1, "auto-generated string 396", true);
      dumpStringSet(ifs, ofs, tablestd, 0x711AF + 0, 2, 1, "auto-generated string 397", true);
      dumpStringSet(ifs, ofs, tablestd, 0x711D8 + 0, 2, 1, "auto-generated string 398", true);
      dumpStringSet(ifs, ofs, tablestd, 0x711E3 + 0, 2, 1, "auto-generated string 399", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71202 + 0, 2, 1, "auto-generated string 400", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7124C + 0, 2, 1, "auto-generated string 401", true);
      dumpStringSet(ifs, ofs, tablestd, 0x712A5 + 0, 2, 1, "auto-generated string 402", true);
      dumpStringSet(ifs, ofs, tablestd, 0x712FB + 0, 2, 1, "auto-generated string 403", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71313 + 0, 2, 1, "auto-generated string 404", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71366 + 0, 2, 1, "auto-generated string 405", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7139D + 0, 2, 1, "auto-generated string 406", true);
      dumpStringSet(ifs, ofs, tablestd, 0x713C7 + 0, 2, 1, "auto-generated string 407", true);
      dumpStringSet(ifs, ofs, tablestd, 0x713FE + 0, 2, 1, "auto-generated string 408", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71428 + 0, 2, 1, "auto-generated string 409", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7145F + 0, 2, 1, "auto-generated string 410", true);
      dumpStringSet(ifs, ofs, tablestd, 0x714C8 + 0, 2, 1, "auto-generated string 411", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7156D + 0, 2, 1, "auto-generated string 412", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7158E + 0, 2, 1, "auto-generated string 413", true);
      dumpStringSet(ifs, ofs, tablestd, 0x715AC + 0, 2, 1, "auto-generated string 414", true);
      dumpStringSet(ifs, ofs, tablestd, 0x715E8 + 0, 2, 1, "auto-generated string 415", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7161C + 0, 2, 1, "auto-generated string 416", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7163A + 0, 2, 1, "auto-generated string 417", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71658 + 0, 2, 1, "auto-generated string 418", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7166E + 0, 2, 1, "auto-generated string 419", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7169E + 0, 2, 1, "auto-generated string 420", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7174A + 0, 2, 1, "auto-generated string 421", true);
      dumpStringSet(ifs, ofs, tablestd, 0x717C5 + 0, 2, 1, "auto-generated string 422", true);
      dumpStringSet(ifs, ofs, tablestd, 0x717EF + 0, 2, 1, "auto-generated string 423", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7184C + 0, 2, 1, "auto-generated string 424", true);
      dumpStringSet(ifs, ofs, tablestd, 0x718A4 + 0, 2, 1, "auto-generated string 425", true);
      dumpStringSet(ifs, ofs, tablestd, 0x718C2 + 0, 2, 1, "auto-generated string 426", true);
      dumpStringSet(ifs, ofs, tablestd, 0x718DE + 0, 2, 1, "auto-generated string 427", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7193D + 0, 2, 1, "auto-generated string 428", true);
      // begins with clear?
      dumpStringSet(ifs, ofs, tablestd, 0x71967 + 0, 2, 1, "auto-generated string 429", true);
      dumpStringSet(ifs, ofs, tablestd, 0x719D3 + 0, 2, 1, "auto-generated string 430", true);
      dumpStringSet(ifs, ofs, tablestd, 0x719F9 + 0, 2, 1, "auto-generated string 431", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71A57 + 0, 2, 1, "auto-generated string 432", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71AB8 + 0, 2, 1, "auto-generated string 433", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71AE5 + 0, 2, 1, "auto-generated string 434", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71B05 + 0, 2, 1, "auto-generated string 435", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71B37 + 0, 2, 1, "auto-generated string 436", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71B55 + 0, 2, 1, "auto-generated string 437", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71BA4 + 0, 2, 1, "auto-generated string 438", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71BD2 + 0, 2, 1, "auto-generated string 439", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71C06 + 0, 2, 1, "auto-generated string 440", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71C38 + 0, 2, 1, "auto-generated string 441", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71C7C + 0, 2, 1, "auto-generated string 442", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71CC1 + 0, 2, 1, "auto-generated string 443", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x71CF8 + 0, 2, 1, "auto-generated string 444", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71CFF + 0, 2, 1, "auto-generated string 445", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71D36 + 0, 2, 1, "auto-generated string 446", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71D8C + 0, 2, 1, "auto-generated string 447", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71DA9 + 0, 2, 1, "auto-generated string 448", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x71DD2 + 0, 2, 1, "auto-generated string 449", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71DD5 + 0, 2, 1, "auto-generated string 450", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71E58 + 0, 2, 1, "auto-generated string 451", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71E7C + 0, 2, 1, "auto-generated string 452", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71EFD + 0, 2, 1, "auto-generated string 453", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71F30 + 0, 2, 1, "auto-generated string 454", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71F4C + 0, 2, 1, "auto-generated string 455", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71FA6 + 0, 2, 1, "auto-generated string 456", true);
      dumpStringSet(ifs, ofs, tablestd, 0x71FDA + 0, 2, 1, "auto-generated string 457", true);
      dumpStringSet(ifs, ofs, tablestd, 0x720CA + 0, 2, 1, "auto-generated string 458", true);
      dumpStringSet(ifs, ofs, tablestd, 0x720EC + 0, 2, 1, "auto-generated string 459", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x72181 + 0, 2, 1, "auto-generated string 460", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72184 + 0, 2, 1, "auto-generated string 461", true);
      dumpStringSet(ifs, ofs, tablestd, 0x721B4 + 0, 2, 1, "auto-generated string 462", true);
      dumpStringSet(ifs, ofs, tablestd, 0x721D2 + 0, 2, 1, "auto-generated string 463", true);
      dumpStringSet(ifs, ofs, tablestd, 0x721F2 + 0, 2, 1, "auto-generated string 464", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x72210 + 0, 2, 1, "auto-generated string 465", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72251 + 0, 2, 1, "auto-generated string 466", true);
      dumpStringSet(ifs, ofs, tablestd, 0x722B7 + 0, 2, 1, "auto-generated string 467", true);
      dumpStringSet(ifs, ofs, tablestd, 0x722F2 + 0, 2, 1, "auto-generated string 468", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72337 + 0, 2, 1, "auto-generated string 469", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72360 + 0, 2, 1, "auto-generated string 470", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72392 + 0, 2, 1, "auto-generated string 471", true);
      dumpStringSet(ifs, ofs, tablestd, 0x723CE + 0, 2, 1, "auto-generated string 472", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72402 + 0, 2, 1, "auto-generated string 473", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7242C + 0, 2, 1, "auto-generated string 474", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72450 + 0, 2, 1, "auto-generated string 475", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72477 + 0, 2, 1, "auto-generated string 476", true);
      dumpStringSet(ifs, ofs, tablestd, 0x724BF + 0, 2, 1, "auto-generated string 477", true);
      dumpStringSet(ifs, ofs, tablestd, 0x724FA + 0, 2, 1, "auto-generated string 478", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x72526 + 0, 2, 1, "auto-generated string 479", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x72581 + 0, 2, 1, "auto-generated string 480", true);
      dumpStringSet(ifs, ofs, tablestd, 0x725A4 + 0, 2, 1, "auto-generated string 481", true);
      dumpStringSet(ifs, ofs, tablestd, 0x725B7 + 0, 2, 1, "auto-generated string 482", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72625 + 0, 2, 1, "auto-generated string 483", true);
      dumpStringSet(ifs, ofs, tablestd, 0x726AB + 0, 2, 1, "auto-generated string 484", true);
      dumpStringSet(ifs, ofs, tablestd, 0x726F3 + 0, 2, 1, "auto-generated string 485", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7273F + 0, 2, 1, "auto-generated string 486", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7276B + 0, 2, 1, "auto-generated string 487", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7277C + 0, 2, 1, "auto-generated string 488", true);
      dumpStringSet(ifs, ofs, tablestd, 0x727A7 + 0, 2, 1, "auto-generated string 489", true);
      dumpStringSet(ifs, ofs, tablestd, 0x727BA + 0, 2, 1, "auto-generated string 490", true);
      dumpStringSet(ifs, ofs, tablestd, 0x727CB + 0, 2, 1, "auto-generated string 491", true);
      dumpStringSet(ifs, ofs, tablestd, 0x727F9 + 0, 2, 1, "auto-generated string 492", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7284A + 0, 2, 1, "auto-generated string 493", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7287F + 0, 2, 1, "auto-generated string 494", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72897 + 0, 2, 1, "auto-generated string 495", true);
      dumpStringSet(ifs, ofs, tablestd, 0x728A3 + 0, 2, 1, "auto-generated string 496", true);
      dumpStringSet(ifs, ofs, tablestd, 0x728FF + 0, 2, 1, "auto-generated string 497", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72917 + 0, 2, 1, "auto-generated string 498", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72928 + 0, 2, 1, "auto-generated string 499", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72959 + 0, 2, 1, "auto-generated string 500", true);
      dumpStringSet(ifs, ofs, tablestd, 0x729A4 + 0, 2, 1, "auto-generated string 501", true);
      dumpStringSet(ifs, ofs, tablestd, 0x729C2 + 0, 2, 1, "auto-generated string 502", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72A34 + 0, 2, 1, "auto-generated string 503", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72A60 + 0, 2, 1, "auto-generated string 504", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x72A91 + 0, 2, 1, "auto-generated string 505", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72AA9 + 0, 2, 1, "auto-generated string 506", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72AD9 + 0, 2, 1, "auto-generated string 507", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72B3C + 0, 2, 1, "auto-generated string 508", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72B69 + 0, 2, 1, "auto-generated string 509", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72C26 + 0, 2, 1, "auto-generated string 510", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72C50 + 0, 2, 1, "auto-generated string 511", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72D00 + 0, 2, 1, "auto-generated string 512", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72D49 + 0, 2, 1, "auto-generated string 513", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x72D79 + 0, 2, 1, "auto-generated string 514", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72D8E + 0, 2, 1, "auto-generated string 515", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72DC2 + 0, 2, 1, "auto-generated string 516", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72DD8 + 0, 2, 1, "auto-generated string 517", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x72EAB + 0, 2, 1, "auto-generated string 518", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72EBD + 0, 2, 1, "auto-generated string 519", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72EDA + 0, 2, 1, "auto-generated string 520", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72EF5 + 0, 2, 1, "auto-generated string 521", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72F0D + 0, 2, 1, "auto-generated string 522", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72F44 + 0, 2, 1, "auto-generated string 523", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72F5D + 0, 2, 1, "auto-generated string 524", true);
      dumpStringSet(ifs, ofs, tablestd, 0x72FDC + 0, 2, 1, "auto-generated string 525", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73030 + 0, 2, 1, "auto-generated string 526", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7305B + 0, 2, 1, "auto-generated string 527", true);
      dumpStringSet(ifs, ofs, tablestd, 0x730F0 + 0, 2, 1, "auto-generated string 528", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73123 + 0, 2, 1, "auto-generated string 529", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x7315E + 0, 2, 1, "auto-generated string 530", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73165 + 0, 2, 1, "auto-generated string 531", true);
      dumpStringSet(ifs, ofs, tablestd, 0x731BA + 0, 2, 1, "auto-generated string 532", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x731F6 + 0, 2, 1, "auto-generated string 533", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7321E + 0, 2, 1, "auto-generated string 534", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x73268 + 0, 2, 1, "auto-generated string 535", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7328C + 0, 2, 1, "auto-generated string 536", true);
      dumpStringSet(ifs, ofs, tablestd, 0x732B9 + 0, 2, 1, "auto-generated string 537", true);
      dumpStringSet(ifs, ofs, tablestd, 0x732E9 + 0, 2, 1, "auto-generated string 538", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x73312 + 0, 2, 1, "auto-generated string 539", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73391 + 0, 2, 1, "auto-generated string 540", true);
      dumpStringSet(ifs, ofs, tablestd, 0x733B9 + 0, 2, 1, "auto-generated string 541", true);
      dumpStringSet(ifs, ofs, tablestd, 0x733E5 + 0, 2, 1, "auto-generated string 542", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73424 + 0, 2, 1, "auto-generated string 543", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73435 + 0, 2, 1, "auto-generated string 544", true);
      dumpStringSet(ifs, ofs, tablestd, 0x734A3 + 0, 2, 1, "auto-generated string 545", true);
      dumpStringSet(ifs, ofs, tablestd, 0x734B8 + 0, 2, 1, "auto-generated string 546", true);
      dumpStringSet(ifs, ofs, tablestd, 0x734FB + 0, 2, 1, "auto-generated string 547", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7351F + 0, 2, 1, "auto-generated string 548", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73541 + 0, 2, 1, "auto-generated string 549", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7354F + 0, 2, 1, "auto-generated string 550", true);
      dumpStringSet(ifs, ofs, tablestd, 0x735A9 + 0, 2, 1, "auto-generated string 551", true);
      dumpStringSet(ifs, ofs, tablestd, 0x735BA + 0, 2, 1, "auto-generated string 552", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73623 + 0, 2, 1, "auto-generated string 553", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7368F + 0, 2, 1, "auto-generated string 554", true);
      dumpStringSet(ifs, ofs, tablestd, 0x736C6 + 0, 2, 1, "auto-generated string 555", true);
      dumpStringSet(ifs, ofs, tablestd, 0x736ED + 0, 2, 1, "auto-generated string 556", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73718 + 0, 2, 1, "auto-generated string 557", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7373D + 0, 2, 1, "auto-generated string 558", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73778 + 0, 2, 1, "auto-generated string 559", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73785 + 0, 2, 1, "auto-generated string 560", true);
      dumpStringSet(ifs, ofs, tablestd, 0x737DD + 0, 2, 1, "auto-generated string 561", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7381E + 0, 2, 1, "auto-generated string 562", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7386A + 0, 2, 1, "auto-generated string 563", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73896 + 0, 2, 1, "auto-generated string 564", true);
      dumpStringSet(ifs, ofs, tablestd, 0x738BA + 0, 2, 1, "auto-generated string 565", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x73902 + 0, 2, 1, "auto-generated string 566", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73923 + 0, 2, 1, "auto-generated string 567", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73968 + 0, 2, 1, "auto-generated string 568", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73983 + 0, 2, 1, "auto-generated string 569", true);
      dumpStringSet(ifs, ofs, tablestd, 0x739C0 + 0, 2, 1, "auto-generated string 570", true);
      dumpStringSet(ifs, ofs, tablestd, 0x739F2 + 0, 2, 1, "auto-generated string 571", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73A0D + 0, 2, 1, "auto-generated string 572", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73A6A + 0, 2, 1, "auto-generated string 573", true);
//      dumpStringSet(ifs, ofs, tablestd, 0x73AC9 + 0, 2, 1, "auto-generated string 574", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73AFF + 0, 2, 1, "auto-generated string 575", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73B5C + 0, 2, 1, "auto-generated string 576", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73B9A + 0, 2, 1, "auto-generated string 577", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73BD7 + 0, 2, 1, "auto-generated string 578", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73BFA + 0, 2, 1, "auto-generated string 579", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73C54 + 0, 2, 1, "auto-generated string 580", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73C91 + 0, 2, 1, "auto-generated string 581", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73CD1 + 0, 2, 1, "auto-generated string 582", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73D25 + 0, 2, 1, "auto-generated string 583", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73DA9 + 0, 2, 1, "auto-generated string 584", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x73DD3 + 0, 2, 1, "auto-generated string 585", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73DF0 + 0, 2, 1, "auto-generated string 586", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73E17 + 0, 2, 1, "auto-generated string 587", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73E7F + 0, 2, 1, "auto-generated string 588", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73EA9 + 0, 2, 1, "auto-generated string 589", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73EEF + 0, 2, 1, "auto-generated string 590", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73F14 + 0, 2, 1, "auto-generated string 591", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73F39 + 0, 2, 1, "auto-generated string 592", true);
      dumpStringSet(ifs, ofs, tablestd, 0x73F6C + 0, 2, 1, "auto-generated string 593", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74018 + 0, 2, 1, "auto-generated string 594", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x74067 + 0, 2, 1, "auto-generated string 595", true);
      dumpStringSet(ifs, ofs, tablestd, 0x740CB + 0, 2, 1, "auto-generated string 596", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7410C + 0, 2, 1, "auto-generated string 597", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7413C + 0, 2, 1, "auto-generated string 598", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74168 + 0, 2, 1, "auto-generated string 599", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74197 + 0, 2, 1, "auto-generated string 600", true);
      dumpStringSet(ifs, ofs, tablestd, 0x741D0 + 0, 2, 1, "auto-generated string 601", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74209 + 0, 2, 1, "auto-generated string 602", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74271 + 0, 2, 1, "auto-generated string 603", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7429C + 0, 2, 1, "auto-generated string 604", true);
      dumpStringSet(ifs, ofs, tablestd, 0x742CC + 0, 2, 1, "auto-generated string 605", true);
      dumpStringSet(ifs, ofs, tablestd, 0x742F2 + 0, 2, 1, "auto-generated string 606", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74329 + 0, 2, 1, "auto-generated string 607", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74368 + 0, 2, 1, "auto-generated string 608", true);
      dumpStringSet(ifs, ofs, tablestd, 0x743F9 + 0, 2, 1, "auto-generated string 609", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7442B + 0, 2, 1, "auto-generated string 610", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74473 + 0, 2, 1, "auto-generated string 611", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7449B + 0, 2, 1, "auto-generated string 612", true);
      dumpStringSet(ifs, ofs, tablestd, 0x744D1 + 0, 2, 1, "auto-generated string 613", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74532 + 0, 2, 1, "auto-generated string 614", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7454B + 0, 2, 1, "auto-generated string 615", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74560 + 0, 2, 1, "auto-generated string 616", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74594 + 0, 2, 1, "auto-generated string 617", true);
      dumpStringSet(ifs, ofs, tablestd, 0x745BC + 0, 2, 1, "auto-generated string 618", true);
      dumpStringSet(ifs, ofs, tablestd, 0x745F2 + 0, 2, 1, "auto-generated string 619", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74653 + 0, 2, 1, "auto-generated string 620", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7466C + 0, 2, 1, "auto-generated string 621", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74681 + 0, 2, 1, "auto-generated string 622", true);
      dumpStringSet(ifs, ofs, tablestd, 0x746AA + 0, 2, 1, "auto-generated string 623", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74726 + 0, 2, 1, "auto-generated string 624", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74757 + 0, 2, 1, "auto-generated string 625", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74781 + 0, 2, 1, "auto-generated string 626", true);
      dumpStringSet(ifs, ofs, tablestd, 0x747BB + 0, 2, 1, "auto-generated string 627", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x74818 + 0, 2, 1, "auto-generated string 628", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7484D + 0, 2, 1, "auto-generated string 629", true);
      dumpStringSet(ifs, ofs, tablestd, 0x748BD + 0, 2, 1, "auto-generated string 630", true);
      dumpStringSet(ifs, ofs, tablestd, 0x748ED + 0, 2, 1, "auto-generated string 631", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x74927 + 0, 2, 1, "auto-generated string 632", true);
      dumpStringSet(ifs, ofs, tablestd, 0x749FF + 0, 2, 1, "auto-generated string 633", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74A0E + 0, 2, 1, "auto-generated string 634", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74A68 + 0, 2, 1, "auto-generated string 635", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74A88 + 0, 2, 1, "auto-generated string 636", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74AB8 + 0, 2, 1, "auto-generated string 637", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74B1C + 0, 2, 1, "auto-generated string 638", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74B6B + 0, 2, 1, "auto-generated string 639", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74B75 + 0, 2, 1, "auto-generated string 640", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74C14 + 0, 2, 1, "auto-generated string 641", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74C23 + 0, 2, 1, "auto-generated string 642", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74C7D + 0, 2, 1, "auto-generated string 643", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74C9D + 0, 2, 1, "auto-generated string 644", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74CCD + 0, 2, 1, "auto-generated string 645", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74D31 + 0, 2, 1, "auto-generated string 646", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74D80 + 0, 2, 1, "auto-generated string 647", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74D99 + 0, 2, 1, "auto-generated string 648", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74DAD + 0, 2, 1, "auto-generated string 649", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74E4E + 0, 2, 1, "auto-generated string 650", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74E5D + 0, 2, 1, "auto-generated string 651", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74EB7 + 0, 2, 1, "auto-generated string 652", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74ED7 + 0, 2, 1, "auto-generated string 653", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74F07 + 0, 2, 1, "auto-generated string 654", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74F6B + 0, 2, 1, "auto-generated string 655", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74FBA + 0, 2, 1, "auto-generated string 656", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74FC5 + 0, 2, 1, "auto-generated string 657", true);
      dumpStringSet(ifs, ofs, tablestd, 0x74FEC + 0, 2, 1, "auto-generated string 658", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75004 + 0, 2, 1, "auto-generated string 659", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7502B + 0, 2, 1, "auto-generated string 660", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75072 + 0, 2, 1, "auto-generated string 661", true);
      dumpStringSet(ifs, ofs, tablestd, 0x750B5 + 0, 2, 1, "auto-generated string 662", true);
      dumpStringSet(ifs, ofs, tablestd, 0x750D6 + 0, 2, 1, "auto-generated string 663", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75151 + 0, 2, 1, "auto-generated string 664", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75175 + 0, 2, 1, "auto-generated string 665", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7519E + 0, 2, 1, "auto-generated string 666", true);
      dumpStringSet(ifs, ofs, tablestd, 0x751C9 + 0, 2, 1, "auto-generated string 667", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x7523F + 0, 2, 1, "auto-generated string 668", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75255 + 0, 2, 1, "auto-generated string 669", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7529F + 0, 2, 1, "auto-generated string 670", true);
      dumpStringSet(ifs, ofs, tablestd, 0x752E7 + 0, 2, 1, "auto-generated string 671", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7530B + 0, 2, 1, "auto-generated string 672", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75334 + 0, 2, 1, "auto-generated string 673", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75363 + 0, 2, 1, "auto-generated string 674", true);
    //  dumpStringSet(ifs, ofs, tablestd, 0x756B9 + 0, 2, 1, "auto-generated string 675", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75831 + 0, 2, 1, "auto-generated string 676", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75852 + 0, 2, 1, "auto-generated string 677", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7585E + 0, 2, 1, "auto-generated string 678", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7587F + 0, 2, 1, "auto-generated string 679", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7588C + 0, 2, 1, "auto-generated string 680", true);
      dumpStringSet(ifs, ofs, tablestd, 0x758AD + 0, 2, 1, "auto-generated string 681", true);
      dumpStringSet(ifs, ofs, tablestd, 0x758BF + 0, 2, 1, "auto-generated string 682", true);
      dumpStringSet(ifs, ofs, tablestd, 0x758DA + 0, 2, 1, "auto-generated string 683", true);
      dumpStringSet(ifs, ofs, tablestd, 0x758E6 + 0, 2, 1, "auto-generated string 684", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75920 + 0, 2, 1, "auto-generated string 685", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7593B + 0, 2, 1, "auto-generated string 686", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75947 + 0, 2, 1, "auto-generated string 687", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75982 + 0, 2, 1, "auto-generated string 688", true);
      dumpStringSet(ifs, ofs, tablestd, 0x7599D + 0, 2, 1, "auto-generated string 689", true);
      dumpStringSet(ifs, ofs, tablestd, 0x759A9 + 0, 2, 1, "auto-generated string 690", true);
      dumpStringSet(ifs, ofs, tablestd, 0x759DE + 0, 2, 1, "auto-generated string 691", true);
      dumpStringSet(ifs, ofs, tablestd, 0x759EE + 0, 2, 1, "auto-generated string 692", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75A07 + 0, 2, 1, "auto-generated string 693", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75A14 + 0, 2, 1, "auto-generated string 694", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75A31 + 0, 2, 1, "auto-generated string 695", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75A3E + 0, 2, 1, "auto-generated string 696", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75A5C + 0, 2, 1, "auto-generated string 697", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75A69 + 0, 2, 1, "auto-generated string 698", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75A81 + 0, 2, 1, "auto-generated string 699", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75A90 + 0, 2, 1, "auto-generated string 700", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75AAC + 0, 2, 1, "auto-generated string 701", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75AB9 + 0, 2, 1, "auto-generated string 702", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75AD8 + 0, 2, 1, "auto-generated string 703", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75AE5 + 0, 2, 1, "auto-generated string 704", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75B02 + 0, 2, 1, "auto-generated string 705", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75B1D + 0, 2, 1, "auto-generated string 706", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75B2C + 0, 2, 1, "auto-generated string 707", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75B47 + 0, 2, 1, "auto-generated string 708", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75B53 + 0, 2, 1, "auto-generated string 709", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75B6E + 0, 2, 1, "auto-generated string 710", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75B80 + 0, 2, 1, "auto-generated string 711", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75B9B + 0, 2, 1, "auto-generated string 712", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75BA8 + 0, 2, 1, "auto-generated string 713", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75BC3 + 0, 2, 1, "auto-generated string 714", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75BD2 + 0, 2, 1, "auto-generated string 715", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75BED + 0, 2, 1, "auto-generated string 716", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75BFC + 0, 2, 1, "auto-generated string 717", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75C17 + 0, 2, 1, "auto-generated string 718", true);
      dumpStringSet(ifs, ofs, tablestd, 0x75C24 + 0, 2, 1, "auto-generated string 719", true);
  //    dumpStringSet(ifs, ofs, tablestd, 0x75F29 + 0, 2, 1, "auto-generated string 720", true);
  //    dumpStringSet(ifs, ofs, tablestd, 0x75F43 + 0, 2, 1, "auto-generated string 721", true);
  //    dumpStringSet(ifs, ofs, tablestd, 0x75F54 + 0, 2, 1, "auto-generated string 722", true);
  //    dumpStringSet(ifs, ofs, tablestd, 0x75FA2 + 0, 2, 1, "auto-generated string 723", true);
  //    dumpStringSet(ifs, ofs, tablestd, 0x75FAE + 0, 2, 1, "auto-generated string 724", true);
  //    dumpStringSet(ifs, ofs, tablestd, 0x75FBD + 0, 2, 1, "auto-generated string 725", true);
  //    dumpStringSet(ifs, ofs, tablestd, 0x76059 + 0, 2, 1, "auto-generated string 726", true);
  //    dumpStringSet(ifs, ofs, tablestd, 0x760DF + 0, 2, 1, "auto-generated string 727", true);
  //    dumpStringSet(ifs, ofs, tablestd, 0x760E2 + 0, 2, 1, "auto-generated string 728", true);
  //    dumpStringSet(ifs, ofs, tablestd, 0x760F3 + 0, 2, 1, "auto-generated string 729", true);
  //    dumpStringSet(ifs, ofs, tablestd, 0x76119 + 0, 2, 1, "auto-generated string 730", true);
    }


  }
  
  // text strings
/*  {
    std::ofstream ofs((outPrefix + "script.txt").c_str(),
                  ios_base::binary);
  
    for (int i = 0; i < 60; i++) {
      outputComment(ofs,
        "quiz world standard question "
        + TStringConversion::intToString(i));
        
      ofs << "#SETSIZE(112, 7)" << endl;
      ofs << "#SETBREAKMODE(SINGLE)" << endl;
      
      ifs.seek(0xCD85 + (i * 2));
      int ptr = ifs.readu16le();
      int addr = (ptr % 0x4000) + 0xC000;
      
      // dump question
      dumpStringSet(ifs, ofs, tablestd, addr, 2, 1,
        "");
      
      
      
      int answerTableAddr = ifs.tell();
      int answerTablePointer = (answerTableAddr % 0x4000) + 0x8000;
      int answerPointerHigh = (answerTablePointer & 0xFF00) >> 8;
      int answerPointerLow = (answerTablePointer & 0xFF);
//      ofs << as2bHex(answerPointerLow) << as2bHex(answerPointerHigh) << endl;
        
      ofs << "#SETSIZE(88, 2)" << endl;
      ofs << "#SETBREAKMODE(SINGLE)" << endl;
      
      // question string is followed by table of pointers to answers
      ifs.seek(answerTableAddr);
      for (int j = 0; j < 3; j++) {
        int ptr = ifs.readu16le();
        int addr = (ptr % 0x4000) + 0xC000;
        
        int pos = ifs.tell();
        
        // dump answer
        dumpStringSet(ifs, ofs, tablestd, addr, 2, 1,
          "");
        
        ifs.seek(pos);
      }
    }
  
    dumpStringSet(ifs, ofs, tablestd, 0xDA4B, 2, 8,
      "quiz world picture questions");
  
    dumpStringSet(ifs, ofs, tablestd, 0xDB63, 2, 2,
      "endings");
    
    dumpStringSet(ifs, ofs, tablestd, 0xDBE7, 2, 9,
      "minigame menu");
    
    dumpStringSet(ifs, ofs, tablestd, 0x23E49, 2, 15,
      "find tuxedo mask");
    
    dumpStringSet(ifs, ofs, tablestd, 0x3C028, 2, 17,
      "minigame intro strings");
  } */
  
  // tilemaps
/*  {
    std::ofstream ofs((outPrefix + "tilemaps.txt").c_str(),
                  ios_base::binary);
    
    dumpTilemapSet(ifs, ofs, 0x73A1, 2,
                   tablestd, 4, 1,
                   1, false, "sailor team roulette 1");
    dumpTilemapSet(ifs, ofs, 0x73AB, 2,
                   tablestd, 4, 2,
                   1, false, "sailor team roulette 2");
    dumpTilemapSet(ifs, ofs, 0x73BD, 2,
                   tablestd, 8, 2,
                   1, false, "sailor team roulette 3");
    dumpTilemapSet(ifs, ofs, 0x73DF, 2,
                   tablestd, 9, 1,
                   1, false, "sailor team roulette 4");
    dumpTilemapSet(ifs, ofs, 0x73F3, 2,
                   tablestd, 4, 2,
                   1, false, "sailor team roulette 5");
    
    dumpTilemapSet(ifs, ofs, 0x7C9E6, 2,
                   tablestd, 0x14, 0x12,
                   1, false, "sound test");
    
    outputComment(ofs, "main menu @ 0x10042");
    
    outputComment(ofs, "credits are an object sprite state, "
      "save your sanity and just dump them manually");
  } */
  
  
  return 0;
} 
