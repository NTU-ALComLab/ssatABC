#include<iostream>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

using namespace std;

void genRVars(FILE * file1, FILE * file2, int vars, int end) {
  for(int i = 0; i < end; ++i) {
    fprintf(file1, "%d ", vars/3+i+1);
    fprintf(file2, "%d ", vars/3+i+1);
  }
}

int main(int argc, char ** argv) {
  int vars, clauses;
  FILE * file1, * file2;
  char name[100];
  string format;

  if (argc != 3) {
    cout << " > Need 3 arguments. " << endl;
    cout << " ./gen3 <size> <name>" << endl;
    goto end;
  }
  cout << " > Generate partial family." << '\n';
  cout << " > Size " << argv[1] << '\n';
  srand(time(NULL));

  sprintf(name, "%s.sdimacs", argv[2]);
  file1 = fopen( name, "w" );
  sprintf(name, "%s.ssat", argv[2]);
  file2 = fopen( name, "w" );

  vars = atoi(argv[1])*3;
  clauses = vars;

  fprintf( file1, "p cnf %d %d\n", vars, clauses );
  fprintf( file1, "e " );
  for ( int i = 1; i < vars/3+1; ++i )
    fprintf( file1, "%d ", i );
  fprintf( file1, "0\n" );

  fprintf( file1, "r 0.5 " );
  for ( int i = vars/3+1; i < vars+1; ++i )
    fprintf( file1, "%d ", i );
  fprintf( file1, "0\n" );

  fprintf( file2, "%d\n%d\n", vars, clauses );
  for ( int i = 1; i < vars/3+1; ++i )
    fprintf( file2, "%d x%d E\n", i, i );

  for ( int i = vars/3+1; i < vars+1; ++i )
    fprintf( file2, "%d x%d R %f\n", i, i, 0.5 );

  // write CNF formula.
  for(int i = 1; i < vars/3+1; ++i) {
    fprintf( file1, "%d ", i );
    fprintf( file2, "%d ", i );
    genRVars( file1, file2, vars, i );
    fprintf( file1, "0\n");
    fprintf( file2, "0\n");
  }
  for(int i = 1; i < vars/3+1; ++i) {
    fprintf( file1, "-%d ", i );
    fprintf( file2, "-%d ", i );
    genRVars( file1, file2, vars, i+vars/3 );
    fprintf( file1, "0\n");
    fprintf( file2, "0\n");
  }
  fclose( file1 );
  fclose( file2 );
end:
  return 0;
}
