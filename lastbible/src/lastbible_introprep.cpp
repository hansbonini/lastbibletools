#include "sms/SmsPattern.h"
#include "sms/GodzillaCmp.h"
#include "sms/OneBppCmp.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TBufStream.h"
#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include "util/TStringConversion.h"
#include "util/TOpt.h"
#include <string>
#include <iostream>

using namespace std;
using namespace BlackT;
using namespace Sms;

const int sectionTileW = 2;
const int sectionTileH = 2;
const int sectionW = sectionTileW * SmsPattern::w;
const int sectionH = sectionTileH * SmsPattern::h;

TGraphic grp;
int mapCounter = 0;
string outPrefix;

void generateMap(int tileX, int tileY,
                 int w = sectionTileW, int h = sectionTileH) {
  int realW = w * SmsPattern::w;
  int realH = h * SmsPattern::h;
  TGraphic g(realW, realH);
  
  int baseX = tileX * SmsPattern::w;
  int baseY = tileY * SmsPattern::h;
  g.copy(grp, TRect(0, 0, 0, 0), TRect(baseX, baseY, realW, realH));
/*  for (int j = 0; j < sectionTileH; j++) {
    for (int i = 0; i < sectionTileW; i++) {
      int x = baseX + (SmsPattern::w * i);
      int y = baseY + (SmsPattern::h * j);
      
    }
  } */
  
  string outName = outPrefix + "map_"
    + TStringConversion::intToString(mapCounter++)
    + ".png";
  TPngConversion::graphicToRGBAPng(outName, g);
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    cout << "Last Bible (Game Gear) intro prep tool"
      << endl;
    cout << "Usage: " << argv[0] << " <infile> <outprefix>"
      << endl;
    return 0;
  }
  
  TPngConversion::RGBAPngToGraphic(string(argv[1]), grp);
  outPrefix = string(argv[2]);
  
/*  for (int i = 0; i < 5; i++) {
    generateMap(1 + (i * sectionTileW), 2);
  }
  
  for (int i = 0; i < 6; i++) {
    generateMap(2 + (i * sectionTileW), 4);
  }
  generateMap(14, 4, 3, 2);
  
  for (int i = 0; i < 9; i++) {
    generateMap(2 + (i * sectionTileW), 6);
  }
  
  for (int i = 0; i < 5; i++) {
    generateMap(8 + (i * sectionTileW), 8);
  } */
  
  // new layout to allow "megami tensei gaiden" to go on one line.
  // note that this has only 25 sections vs. the original's 26.
  
  for (int i = 0; i < 5; i++) {
    generateMap(1 + (i * sectionTileW), 2);
  }
  
  for (int i = 0; i < 6; i++) {
    generateMap(2 + (i * sectionTileW), 4);
  }
  generateMap(14, 4, 3, 2);
  
  for (int i = 0; i < 9; i++) {
    generateMap(2 + (i * sectionTileW), 6);
  }
  
  for (int i = 0; i < 1; i++) {
    generateMap(8 + (i * sectionTileW), 7);
  }
  
  for (int i = 5; i < 8; i++) {
    generateMap(1 + (i * sectionTileW), 2);
  }
  
/*  for (int i = 0; i < mapCounter; i++) {
    string numstr
      = TStringConversion::intToString(i + 3, TStringConversion::baseHex)
          .substr(2, string::npos);
    while (numstr.size() < 2) numstr = "0" + numstr;
    cout << "[Tilemap" << numstr << "]" << endl;
    cout << "source=" << outPrefix << "map_"
      << TStringConversion::intToString(i) << ".png" << endl;
    cout << "dest=out/maps/intro_burn_"
      << TStringConversion::intToString(i) << ".bin" << endl;
    cout << "halfwidth=1" << endl;
    cout << endl;
  } */
  
//  ofs.save(argv[2]);
  
  return 0;
}
