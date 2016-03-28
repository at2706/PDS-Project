#include <iostream>
#include <fstream>
using namespace std;

bool CreateDummy()
{
  ofstream out;
  out.open("Dummy.txt");
  // skip: test if open

  out<<"Some Header"<<endl;
  out<<"REPLACE1  12345678901234567890"<<endl;
  out<<"REPLACE2  12345678901234567890"<<endl;
  out<<"Now ~1 TB of data follows..."<<endl;

  out.close();

  return true;
}


int main()
{
  CreateDummy(); // skip: test if successful

  fstream inout;
  inout.open("Dummy.txt", ios::in | ios::out);
  // skip test if open

  bool FoundFirst = false;
  string FirstText = "REPLACE1";
  string FirstReplacement = "Replaced first!!!";

  bool FoundSecond = false;
  string SecondText = "REPLACE2";
  string SecondReplacement = "Replaced second!!!";

  string Line;
  size_t LastPos = inout.tellg();
  while (getline(inout, Line)) {
    if (FoundFirst == false && Line.compare(0, FirstText.size(), FirstText) == 0) {
      // skip: check if Line.size() >= FirstReplacement.size()
      while (FirstReplacement.size() < Line.size()) FirstReplacement += " ";
      FirstReplacement += '\n';

      inout.seekp(LastPos);
      inout.write(FirstReplacement.c_str(), FirstReplacement.size());
      FoundFirst = true;
    } else if (FoundSecond == false && Line.compare(0, SecondText.size(), SecondText) == 0) {
      // skip: check if Line.size() >= SecondReplacement.size()
      while (SecondReplacement.size() < Line.size()) SecondReplacement += " ";
      SecondReplacement += '\n';

      inout.seekp(LastPos);
      inout.write(SecondReplacement.c_str(), SecondReplacement.size());
      FoundSecond = true;
    }

    if (FoundFirst == true && FoundSecond == true) break;
    LastPos = inout.tellg();
  }
  inout.close();

  return 0;
}