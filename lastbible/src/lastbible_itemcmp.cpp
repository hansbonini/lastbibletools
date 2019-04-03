#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TGraphic.h"
#include "util/TStringConversion.h"
#include "util/TThingyTable.h"
#include "util/TPngConversion.h"
#include "util/TCsv.h"
#include "util/TSoundFile.h"
#include <vector>
#include <string>
#include <iostream>

using namespace std;
using namespace BlackT;
//using namespace Sms;

TThingyTable table;

string as2bHex(int num) {
  string str = TStringConversion::intToString(num,
                  TStringConversion::baseHex).substr(2, string::npos);
  while (str.size() < 2) str = string("0") + str;
  
//  return "<$" + str + ">";
  return str;
}

string as2bHexPrefix(int num) {
  return "$" + as2bHex(num) + "";
}

int main(int argc, char* argv[]) {
  if (argc < 5) {
    cout << "Last Bible item info printer" << endl;
    cout << "Usage: " << argv[0] << " [rom] [offset] [table] [count]" << endl;
  }
  
  // game boy = 0x355c
  // game gear = 0x2f1d
  // gbc = 0x40402
  // gbc us = 0x405c4 (names are 12b)
  
  TIfstream ifs(argv[1], ios_base::binary);
  ifs.seek(TStringConversion::stringToInt(string(argv[2])));
  table.readSjis(string(argv[3]));
  int numItems = TStringConversion::stringToInt(string(argv[4]));
  
  for (int i = 0; i < numItems; i++) {
    
    string name;
    for (int j = 0; j < 7; j++) {
      name += table.getEntry(table.matchId(ifs).id);
    }
    
    cout << ";=====" << endl;
    cout << "; item " << i << endl;
    cout << ";=====" << endl;
    
    cout << "name: " << name << endl;
    for (int i = 0; i < 11; i++) {
      cout << "field" + TStringConversion::intToString(i)
        << ": " << TStringConversion::intToString(ifs.readu8(),
                    TStringConversion::baseHex) << endl;
    }
  }
  
  return 0;
}
