#include<fstream>
#include<cstring>
#include<stdio.h>

using namespace std;

int main(int argc, char ** argv) {
  fstream file_in, file_out;
  string line;
  int vars, clauses, rlength = 0, index;
  double probability;
  char type;

  if(argc != 3)
    return 1;
  else {
    file_in.open(argv[1], ios::in);
    file_out.open(argv[2], ios::out);
    // file_out << "c ssat file from sdimacs\n";
    while(getline(file_in, line)) {
      if(line[0] == 'p') {
        sscanf(line.c_str(), "p cnf %d %d", &vars, &clauses);
        getline(file_in, line);
        
        file_in >> type;
        file_in >> probability;
        for(;;) {
          file_in >> index;
          if(index == 0) break;
          ++rlength;
        }
        getline(file_in, line);

        file_out << vars << '\n' << clauses << '\n';
        for(int i = rlength; i < vars; ++i) {
          file_out << i+1 << " n" << i+1 << " E" << '\n';
        }
        for(int i = 0; i < rlength; ++i) {
          file_out << i+1 << " n" << i+1 << " R " << probability << '\n';
        }
      }
      else if(line[0] != 'c'){
        file_out << line << '\n';
      }
    }
    file_in.close();
    file_out.close();
  }
  return 0;
}
