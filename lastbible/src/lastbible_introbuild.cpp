#include "util/TStringConversion.h"
#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TThingyTable.h"
#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include "sms/SmsPattern.h"
#include "lastbible/LastBibleScriptReader.h"
#include "lastbible/LastBibleLineWrapper.h"
#include "exception/TGenericException.h"
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>

using namespace std;
using namespace BlackT;
using namespace Sms;

TThingyTable table;
LastBibleLineWrapper::CharSizeTable sizeTable;
//vector<SmsPattern> font;
map<int, SmsPattern> font;
map<int, TGraphic> fontGraphics;

const static int charsPerRow = 16;
const static int baseOutputTile = 0x0B;
const static int tileOrMask = 0x0800;
const static int screenTileWidth = 20;
const static int screenVisibleX = 6;

const static int op_br   = 0x90;
const static int op_wait = 0x91;
const static int op_hero = 0x92;
const static int op_op93 = 0x93;
const static int op_terminator = 0xFF;

string getStringName(LastBibleScriptReader::ResultString result) {
//  int bankNum = result.srcOffset / 0x4000;
  return string("string_")
    + TStringConversion::intToString(result.srcOffset,
          TStringConversion::baseHex);
}

int getStringWidth(LastBibleScriptReader::ResultString result) {
  int width = 0;
  
  TBufStream ifs(0x10000);
  ifs.write(result.str.c_str(), result.str.size());
  ifs.clear();
  ifs.seek(0);
  
  while (!ifs.eof()) {
    TThingyTable::MatchResult result = table.matchId(ifs);
    if (result.id == -1) {
      throw TGenericException(T_SRCANDLINE,
                              "getStringWidth()",
                              "Unknown symbol at pos "
                                + TStringConversion::intToString(ifs.tell()));
    }
    
    width += sizeTable[result.id];
  }
  
  return width;
}

void composeStringGraphic(LastBibleScriptReader::ResultString result,
                          TGraphic& dst,
                          int extraOffset = 0) {
  // width of raw string in pixels
  int pixelWidth = getStringWidth(result) + extraOffset;
  // number of pixels from the left edge of the screen that the string would
  // have to be moved to center it
  int centerPixelOffset
    = ((screenTileWidth * SmsPattern::w) - pixelWidth) / 2;
  // number of pixels from the left edge of the tile at which the string
  // actually gets positioned that it must be moved in order to be centered 
  int subpixelOffset = (centerPixelOffset % SmsPattern::w);
  // the number of tiles required to contain the centered string
  int tileWidth = ((pixelWidth + subpixelOffset) / SmsPattern::w);
  if (((pixelWidth + subpixelOffset) % SmsPattern::w) != 0) ++tileWidth;
  
//  std::cerr << pixelWidth << std::endl;
//  int tileOffset = centerPixelOffset / SmsPattern::w;
//  if ((centerPixelOffset % SmsPattern::w) == 0) subpixelOffset = 4;
  
//  int tileWidth = ((pixelWidth + subpixelOffset) / SmsPattern::w) + 1;
//  if (subpixelOffset == 0) --tileWidth;

//  if (subpixelOffset != 0) ++tileWidth;

//  if ((pixelWidth % SmsPattern::w) == 0) --tileWidth;

  if (getStringWidth(result) == 0) {
    tileWidth = 0;
    centerPixelOffset = 0;
    subpixelOffset = 0;
  }

//  std::cerr << "  " << pixelWidth << " " << centerPixelOffset << " " << subpixelOffset << " " << tileWidth << std::endl;
  
  dst.resize(tileWidth * SmsPattern::w, SmsPattern::h);
  
  // "clear" with space character (index 0)
  for (int i = 0; i < tileWidth; i++) {
    font[0].toGraphic(dst, NULL,
                      i * SmsPattern::w, 0,
                      false, false, true);
  }
  
  
  TBufStream ifs(0x10000);
  ifs.write(result.str.c_str(), result.str.size());
  ifs.clear();
  ifs.seek(0);
  
  int pos = subpixelOffset + extraOffset;
  while (!ifs.eof()) {
    TThingyTable::MatchResult result = table.matchId(ifs);
    if (result.id == -1) {
      throw TGenericException(T_SRCANDLINE,
                              "composeStringGraphic()",
                              "Unknown symbol at pos "
                                + TStringConversion::intToString(ifs.tell()));
    }
    
    int charWidth = sizeTable[result.id];
    
    dst.copy(fontGraphics[result.id],
      TRect(pos, 0, sizeTable[result.id], SmsPattern::h),
      TRect(0, 0, 0, 0));
    
    pos += charWidth;
  }
}

void generateSection(TStream& ifs, const std::string& outPrefix,
                     int extraCenterTileOffset = 0,
                     int extraCenterPixelOffset = 0) {
  // intro text
  vector<TBufStream> outputTilemaps;
//  vector<SmsPattern> outputPatterns;
  TBufStream outputPatterns(0x10000);
  int outTileNum = baseOutputTile;
  {
    LastBibleScriptReader::ResultCollection results;
    LastBibleScriptReader(ifs, results, table)();
    
    for (unsigned int i = 0; i < results.size(); i++) {
//      std::cerr << "string " << i << std::endl;
//      cout << getStringWidth(results[i]) << endl;
      TGraphic stringGraphic;
      composeStringGraphic(results[i], stringGraphic, extraCenterPixelOffset);
//      TPngConversion::graphicToRGBAPng("test_" + TStringConversion::intToString(i) + ".png",
//                                       stringGraphic);
      
      TBufStream tilemapOfs(0x10000);
      int tileW = stringGraphic.w() / SmsPattern::w;
      // number of tiles
      tilemapOfs.writeu8(tileW);
      // centering offset
      int centerOffset = ((screenTileWidth - tileW) / 2) + extraCenterTileOffset;
      // fuck
//      if ((getStringWidth(results[i]) % SmsPattern::w) == 0) ++centerOffset;
      tilemapOfs.writeu8(centerOffset);
      for (int j = 0; j < tileW; j++) {
        SmsPattern pattern;
        pattern.fromGrayscaleGraphic(stringGraphic, j * SmsPattern::w, 0);
//        outputPatterns.push_back(pattern);
        pattern.write(outputPatterns);
        
        int tileId = outTileNum++ | tileOrMask;
        tilemapOfs.writeu16le(tileId);
        
        // HACK: ban tiles from 0x7B to 0x9F and 0xB0 to 0xEF
//        while (((outTileNum >= 0x7B) && (outTileNum < 0xA0))
//               || ((outTileNum >= 0xB0) && (outTileNum < 0xF0))) {
        while (((outTileNum >= 0x7B) && (outTileNum < 0xF0))) {
//          SmsPattern pattern;
//          pattern.write(outputPatterns);
          ++outTileNum;
        }
      }
      
      outputTilemaps.push_back(tilemapOfs);
    }
    
  }
  
  outputPatterns.save((outPrefix + "grp.bin").c_str());
  
  TBufStream outputTilemapTable(0x10000);
  outputTilemapTable.writeu8(outputTilemaps.size());
/*  {
    int offset = (outputTilemaps.size() * 2);
    for (unsigned int i = 0; i < outputTilemaps.size(); i++) {
      outputTilemapTable.writeu16le(offset);
      offset += outputTilemaps[i].size();
    }
  } */
  
  for (unsigned int i = 0; i < outputTilemaps.size(); i++) {
    TBufStream& ofs = outputTilemaps[i];
    ofs.seek(0);
    outputTilemapTable.writeFrom(ofs, ofs.size());
  }
  
  outputTilemapTable.save((outPrefix + "tilemaps.bin").c_str());
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cout << "LastBible intro builder" << endl;
    cout << "Usage: " << argv[0] << " [inprefix] [thingy] [outprefix]"
      << endl;
    
    return 0;
  }
  
  string inPrefix = string(argv[1]);
  string tableName = string(argv[2]);
  string outPrefix = string(argv[3]);
  
  table.readSjis(tableName);
  
  // read size table
  {
    TBufStream ifs;
    ifs.open("out/font/sizetable.bin");
    int pos = 0;
    while (!ifs.eof()) {
      sizeTable[pos++] = ifs.readu8();
    }
  }
  
  int numChars = sizeTable.size();
  
  // font graphics
  TGraphic g;
  TPngConversion::RGBAPngToGraphic("rsrc/font_vwf/font.png", g);
  for (int i = 0; i < numChars; i++) {
    int x = (i % charsPerRow) * SmsPattern::w;
    int y = (i / charsPerRow) * SmsPattern::h;
  
    SmsPattern pattern;
    pattern.fromGrayscaleGraphic(g, x, y);
    
//    font.push_back(pattern);
    font[i] = pattern;
    TGraphic patternGraphic(SmsPattern::w, SmsPattern::h);
    patternGraphic.copy(g,
           TRect(0, 0, 0, 0),
           TRect(x, y, SmsPattern::w, SmsPattern::h));
    fontGraphics[i] = patternGraphic;
  }
  
  {
    TBufStream ifs;
    ifs.open((inPrefix + "scroll1.txt").c_str());
    generateSection(ifs, outPrefix + "scroll1/");
  }
  
  {
    TBufStream ifs;
    ifs.open((inPrefix + "scroll2.txt").c_str());
    generateSection(ifs, outPrefix + "scroll2/");
  }
  
  {
    TBufStream ifs;
    ifs.open((inPrefix + "credits.txt").c_str());
    generateSection(ifs, outPrefix + "credits/", 2, 6);
  }
  
/*  // intro text
  vector<TBufStream> outputTilemaps;
//  vector<SmsPattern> outputPatterns;
  TBufStream outputPatterns(0x10000);
  int outTileNum = baseOutputTile;
  {
    TBufStream ifs;
//    ifs.open((inPrefix + "script.txt").c_str());
    ifs.open((inPrefix + "intro.txt").c_str());
    
    LastBibleScriptReader::ResultCollection results;
    LastBibleScriptReader(ifs, results, table)();
    
    for (unsigned int i = 0; i < results.size(); i++) {
//      std::cerr << "string " << i << std::endl;
//      cout << getStringWidth(results[i]) << endl;
      TGraphic stringGraphic;
      composeStringGraphic(results[i], stringGraphic);
//      TPngConversion::graphicToRGBAPng("test_" + TStringConversion::intToString(i) + ".png",
//                                       stringGraphic);
      
      TBufStream tilemapOfs(0x10000);
      int tileW = stringGraphic.w() / SmsPattern::w;
      // number of tiles
      tilemapOfs.writeu8(tileW);
      // centering offset
      int centerOffset = ((screenTileWidth - tileW) / 2);
      tilemapOfs.writeu8(centerOffset + screenVisibleX);
      for (int j = 0; j < tileW; j++) {
        SmsPattern pattern;
        pattern.fromGrayscaleGraphic(stringGraphic, j * SmsPattern::w, 0);
//        outputPatterns.push_back(pattern);
        pattern.write(outputPatterns);
        
        int tileId = outTileNum++ | tileOrMask;
        tilemapOfs.writeu16le(tileId);
      }
      
      outputTilemaps.push_back(tilemapOfs);
    }
    
  }
  
  outputPatterns.save((outPrefix + "intro/grp.bin").c_str());
  
  TBufStream outputTilemapTable(0x10000);
  outputTilemapTable.writeu8(outputTilemaps.size());
  
  for (unsigned int i = 0; i < outputTilemaps.size(); i++) {
    TBufStream& ofs = outputTilemaps[i];
    ofs.seek(0);
    outputTilemapTable.writeFrom(ofs, ofs.size());
  }
  
  outputTilemapTable.save((outPrefix + "/intro/tilemaps.bin").c_str()); */
  
  return 0;
}

