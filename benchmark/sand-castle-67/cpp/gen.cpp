#include <iostream>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>

using namespace std;

int main( int argc , char ** argv )
{
  int frames;
  int vars, clauses;
  char name[128];
  double pro[5] = { 0.5 , 0.67 , 0.25 , 0.75 , 0.5 };
  FILE * file;

  if( argc != 2 ) {
    cout << "./gen <# of frames>" << '\n';
    return 1;
  }

  frames = atoi(argv[1]);
  cout << "  > Get # of frames = " << frames << '\n';

  if( frames < 1 ) {
    cout << "  > # of frames must be greater than 0." << '\n';
    return 1;
  }

  vars = 2 + 9 * frames;
  clauses = 3 + 18 * frames;

  sprintf( name , "SC-%d.sdimacs" , frames );
  file = fopen( name , "w" );

  fprintf( file , "c SAND-CASTLE-67 SDIMACS genereted by sand-castle-67.cpp\n" );
  fprintf( file , "p cnf %d %d\n" , vars , clauses );

  fprintf( file , "e " );
  for ( int i = 0 ; i < frames ; ++i )
    fprintf( file , "%d %d " , i * 9 + 3 , i * 9 + 4 );
  fprintf( file , "0\n" );
  
  for ( int i = 0 ; i < frames ; ++i )
    for ( int j = 0 ; j < 5 ; ++j )
      fprintf( file , "r %f %d 0\n" , pro[j] , i * 9 + 5 + j );

  fprintf( file , "e 1 2 " );
  for ( int i = 0 ; i < frames ; ++i )
    fprintf( file , "%d %d " , i * 9 + 10 , i * 9 + 11 );
  fprintf( file , "0\n" );

  // Initial conditions & Goal.
  fprintf( file , "-1 0\n-2 0\n%d 0\n", vars );
  for ( int i = 0 ; i < frames ; ++i ) {
    fprintf( file , "%d %d 0\n" , i * 9 + 3 , i * 9 + 4 );
    fprintf( file , "-%d -%d 0\n" , i * 9 + 3 , i * 9 + 4 );
    fprintf( file , "-%d -%d %d 0\n" , i * 9 + 3 , i * 9 + 1 , i * 9 + 10 );
    fprintf( file , "-%d %d -%d %d 0\n" , i * 9 + 3 , i * 9 + 1 , i * 9 + 5 , i * 9 + 10 );
    fprintf( file , "-%d %d %d -%d 0\n" , i * 9 + 3 , i * 9 + 1 , i * 9 + 5 , i * 9 + 10 );
    fprintf( file , "-%d -%d -%d -%d %d 0\n" , i * 9 + 4 , i * 9 + 1 , i * 9 + 2 , i * 9 + 8 , i * 9 + 10 );
    fprintf( file , "-%d -%d -%d %d -%d 0\n" , i * 9 + 4 , i * 9 + 1 , i * 9 + 2 , i * 9 + 8 , i * 9 + 10 );
    fprintf( file , "-%d -%d %d -%d %d 0\n" , i * 9 + 4 , i * 9 + 1 , i * 9 + 2 , i * 9 + 11 , i * 9 + 10 );
    fprintf( file , "-%d -%d %d %d -%d %d 0\n" , i * 9 + 4 , i * 9 + 1 , i * 9 + 2 , i * 9 + 11 , i * 9 + 9 , i * 9 + 10 );
    fprintf( file , "-%d -%d %d %d %d -%d 0\n" , i * 9 + 4 , i * 9 + 1 , i * 9 + 2 , i * 9 + 11 , i * 9 + 9 , i * 9 + 10 );
    fprintf( file , "-%d %d -%d 0\n" , i * 9 + 4 , i * 9 + 1 , i * 9 + 10 );
    fprintf( file , "-%d -%d %d 0\n" , i * 9 + 3 , i * 9 + 2 , i * 9 + 11 );
    fprintf( file , "-%d %d -%d 0\n" , i * 9 + 3 , i * 9 + 2 , i * 9 + 11 );
    fprintf( file , "-%d -%d %d 0\n" , i * 9 + 4 , i * 9 + 2 , i * 9 + 11 );
    fprintf( file , "-%d %d -%d -%d %d 0\n" , i * 9 + 4 , i * 9 + 2 , i * 9 + 1 , i * 9 + 6 , i * 9 + 11 );
    fprintf( file , "-%d %d -%d %d -%d 0\n" , i * 9 + 4 , i * 9 + 2 , i * 9 + 1 , i * 9 + 6 , i * 9 + 11 );
    fprintf( file , "-%d %d %d -%d %d 0\n" , i * 9 + 4 , i * 9 + 2 , i * 9 + 1 , i * 9 + 7 , i * 9 + 11 );
    fprintf( file , "-%d %d %d %d -%d 0\n" , i * 9 + 4 , i * 9 + 2 , i * 9 + 1 , i * 9 + 7 , i * 9 + 11 );
  }

  fclose( file );
  return 0;
}
