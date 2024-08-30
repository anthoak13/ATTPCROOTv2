#include <fstream>
#include <iostream>
#include <string>
#include <vector>

void test2()
{
   TH1F *hTest = new TH1F("hTest", "hTest", 100, 0, 100);

   for (int i = 0; i < 100; i++) {
      // hTest->Fill(50);
      hTest->SetBinContent(i, i);
   }

   hTest->Fit("pol1");

   hTest->Draw();
}

void test()
{

   std::string fileName = "test.csv";
   std::ifstream file(fileName);

   if (!file.is_open())
      throw std::invalid_argument("file does not exist " + fileName);

   // Clear the header
   // std::string header;
   // std::getline(file, header);

   for (auto &row : CSVRange<int>(file)) {
      int row0 = row[0];
      int row1 = row[1];

      std::cout << row0 << " " << row1 << std::endl;
   }

   std::cout << "Hello, World!" << std::endl;
}