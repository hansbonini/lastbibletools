#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TGraphic.h"
#include "util/TStringConversion.h"
#include "util/TPngConversion.h"
#include "util/TCsv.h"
#include "util/TSoundFile.h"
#include <vector>
#include <string>
#include <iostream>

using namespace std;
using namespace BlackT;
//using namespace Sms;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cout << "Script text scraper" << endl;
    cout << "Usage: " << argv[0] << " [file]" << endl;
    return 0;
  }
  
  std::ifstream ifs(argv[1], ios_base::binary);
  
  while (ifs.good()) {
    ifs.get();
    if (!ifs.good()) break;
    ifs.unget();
    
    std::string line;
    std::getline(ifs, line);
    
    if ((line.size() > 0)
        && (line[0] == '#')) {
      cout << line << endl;
    }
  }
  
  return 0;
}
