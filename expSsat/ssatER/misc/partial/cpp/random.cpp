#include<iostream>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

using namespace std;

void genRVars(FILE * file, int vars) {
  int number = (rand() % vars) / 10 + 3;
  for(int i = 0; i < number; ++i) {
    if (rand()%2 == 0)
       fprintf(file, "-");
    fprintf(file, "%d ", rand()%(vars/2) + (vars/2));
  }
}

int main(int argc, char ** argv) {
  int vars, clauses;
  FILE * file;
  string format;

  if (argc != 4) {
    cout << " > Need 4 arguments. " << endl;
    cout << " ./gen <sdimacs|ssat> <size> <name>" << endl;
    goto end;
  }
  cout << " > Generate family." << '\n';
  cout << " > Type " << argv[1] << '\n';
  cout << " > Size " << argv[2] << '\n';
  srand(time(NULL));
  file = fopen( argv[3] , "w" );
  vars = atoi(argv[2])*2;
  clauses = vars;

  if      ( !strcmp( argv[1] , "sdimacs" ) ) format = "sdimacs";
  else if ( !strcmp( argv[1] , "ssat"    ) ) format = "ssat";
  else {
    cout << " > Unknown format." << argv[1] << endl;
    return 1;
  }

  if ( format == "sdimacs" ) {
    fprintf( file, "p cnf %d %d\n", vars, clauses );
    fprintf( file, "e " );
    for ( int i = 1; i < vars/2+1; ++i )
      fprintf( file, "%d ", i );
    fprintf( file, "0\n" );

    fprintf( file, "r 0.5 " );
    for ( int i = vars/2+1; i < vars+1; ++i )
      fprintf( file, "%d ", i );
    fprintf( file, "0\n" );
  }
  else if ( format == "ssat" ) {
    fprintf( file, "%d\n%d\n", vars, clauses );
    for ( int i = 1; i < vars/2+1; ++i )
      fprintf( file, "%d x%d E\n", i, i );

    for ( int i = vars/2+1; i < vars+1; ++i )
      fprintf( file, "%d x%d R %f\n", i, i, 0.5 );
  }
  // write CNF formula.
  for(int i = 1; i < vars/2+1; ++i) {
    fprintf( file, "%d ", i );
    genRVars( file, vars );
    fprintf( file, "0\n");
    fprintf( file, "-%d ", i );
    genRVars( file, vars );
    fprintf( file, "0\n");
  }
  fclose( file );
end:
  return 0;
}
