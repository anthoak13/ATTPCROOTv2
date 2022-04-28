#include <fstream>
#include <iostream>

#include "/mnt/simulations/attpcroot/adam/ATTPCROOTv2/AtTools/AtCSVReader.h"
using namespace std;
void formatCalibration()
{
   ifstream calFile("output/filteredBaseline/calibration.csv");
   if (!calFile.is_open()) {
      cout << "Can't open input calibration file" << endl;
   }

   ofstream outFile("output/filteredBaseline/calibrationFormated.txt");
   if (!outFile.is_open()) {
      cout << "Can't open output calibration file" << endl;
   }
   if (!calFile || !outFile)
      return;
   for (auto &row : CSVRange<double>(calFile)) {
      int padNum = row[0];
      double b = row[1];
      double gain = row[2];
      if (gain < 0.4)
         gain *= 3.663;
      outFile << padNum << "\t" << 0 << "\t" << gain << endl;
   }
}
