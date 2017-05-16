#include<iostream>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<set>

using namespace std;

void genRVars(FILE * file1, FILE * file2, int vars, int index, bool sign) {
  int number = (rand() % vars) / 10 + 4;
  int value;
  set<int> vars_pool;
  set<int>::iterator it;
  int values[64], count = 0;
  while(count != 2*number) {
    do {
      value = rand()%(vars/2) + (vars/2);
    } while (vars_pool.find(value) != vars_pool.end());
    vars_pool.insert(value);
    ++count;
  }
  for(it = vars_pool.begin(); it != vars_pool.end(); it++) {
    cout << *it << ' ';
  }
  cout << endl;
  it = vars_pool.begin();
  for(int i = 0; i < 2*number; ++i) {
    if (rand()%2 == 0)
      values[i] = -1 * (*(it));
    else
      values[i] = *it;
    it++;
  }
  if (!sign) {
    fprintf( file1, "%d ", index );
    fprintf( file2, "%d ", index );
  }
  else {
    fprintf( file1, "-%d ", index );
    fprintf( file2, "-%d ", index );
  }
  for(int i = 0; i < number; ++i) {
    fprintf( file1, "%d ", values[i] );
    fprintf( file2, "%d ", values[i] );
  }
  fprintf( file1, "0\n");
  fprintf( file2, "0\n");
  if (!sign) {
    fprintf( file1, "%d ", index+1 );
    fprintf( file2, "%d ", index+1 );
  }
  else {
    fprintf( file1, "-%d ", index+1 );
    fprintf( file2, "-%d ", index+1 );
  }
  for(int i = 0; i < 2*number; ++i) {
    fprintf( file1, "%d ", values[i] );
    fprintf( file2, "%d ", values[i] );
  }
  fprintf( file1, "0\n");
  fprintf( file2, "0\n");
}

int main(int argc, char ** argv) {
  int vars, clauses;
  FILE * file1, * file2;
  char name[100];
  string format;

  if (argc != 4) {
    cout << " > Need 4 arguments. " << endl;
    cout << " ./gen <size> <name> <seed>" << endl;
    goto end;
  }
  cout << " > Generate family." << '\n';
  cout << " > Size " << argv[1] << '\n';
  srand(time(NULL)*(unsigned)atoi(argv[3]));

  sprintf(name, "%s.sdimacs", argv[2]);
  file1 = fopen( name, "w" );
  sprintf(name, "%s.ssat", argv[2]);
  file2 = fopen( name, "w" );

  vars = atoi(argv[1])*2;
  clauses = vars;

  fprintf( file1, "p cnf %d %d\n", vars, clauses );
  fprintf( file1, "e " );
  for ( int i = 1; i < vars/2+1; ++i )
    fprintf( file1, "%d ", i );
  fprintf( file1, "0\n" );

  fprintf( file1, "r 0.5 " );
  for ( int i = vars/2+1; i < vars+1; ++i )
    fprintf( file1, "%d ", i );
  fprintf( file1, "0\n" );

  fprintf( file2, "%d\n%d\n", vars, clauses );
  for ( int i = 1; i < vars/2+1; ++i )
    fprintf( file2, "%d x%d E\n", i, i );

  for ( int i = vars/2+1; i < vars+1; ++i )
    fprintf( file2, "%d x%d R %f\n", i, i, 0.5 );

  // write CNF formula.
  for(int i = 1; i < vars/2+1; i=i+2) {
    genRVars( file1, file2, vars, i, true);
    genRVars( file1, file2, vars, i, false);
  }
  fclose( file1 );
  fclose( file2 );
end:
  return 0;
}
