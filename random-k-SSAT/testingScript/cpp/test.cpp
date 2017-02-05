#include <fstream>
#include <stdio.h>
#include <iostream>

#define TIMEOUT 600

using namespace std;

int main(int argc, char ** argv) {
 
  char cmdTest[256];
  double prob = 0.5;
  int counter = 0;

  for(int i = 3; i < 10; i = i + 4) {
    for(int j = 30; j < 70; j = j + 10) {
      for(int k = 20; k < 300; k = k + 20) {
        for(int l = 20; l < j  ; l = l + 10   ) {
          sprintf(cmdTest, "echo \"=====\n\" >> test.log &&\
             echo \"Using %d %d %d %d %f %d\n\" >> test.log &&\
              ./gen-comp.sh %d %d %d %d %f %d >> test.log", i, j, k, l, prob, TIMEOUT, i, j, k, l, prob, TIMEOUT);
          if(system(cmdTest)) {
            cout << "  Error! Problems with testing...\n";
            return 1;
          }
          counter++;
          if(counter % 20 == 0)
            cout << "  > Test " << counter << " instances." << endl;
        }
      }
    }
  }

  return 0;
}
