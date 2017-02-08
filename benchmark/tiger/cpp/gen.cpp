#include <iostream>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <string.h>

using namespace std;

int main( int argc , char ** argv )
{
  int frames;
  int vars, clauses;
  char name[128];
  double pro[2] = { 0.85 , 0.15 }; 
  FILE * file;
  string format;

  if( argc != 4 ) {
    cout << "./gen <format> <# of frames> <two|mul>" << '\n';
    return 1;
  }

  if      ( !strcmp( argv[1] , "sdimacs" ) ) format = "sdimacs";
  else if ( !strcmp( argv[1] , "ssat"    ) ) format = "ssat";
  else if ( !strcmp( argv[1] , "blif"    ) ) format = "blif";
  else {
     cout << "Unknown format " << argv[1] << endl;
     return 1;
  }

  frames = atoi(argv[2]);
  
  cout << "  > Get # of frames = " << frames << " format " << format << '\n';
  if( frames < 1 ) {
    cout << "  > # of frames must be greater than 0." << '\n';
    return 1;
  }
  sprintf( name , "SC-%d.%s" , frames , format.c_str() );
  file = fopen( name , "w" );

  if( format == "blif" ) {
  }
  else {
    vars = 3 + 9 * frames;
    clauses = 4 + 30 * frames;

    if ( format == "sdimacs" ) {
      fprintf( file , "p cnf %d %d\n" , vars , clauses );
      fprintf( file , "e " );
      for ( int i = 0 ; i < frames ; ++i )
        fprintf( file , "%d %d %d " , i * 9 + 4 , i * 9 + 5 , i * 9 + 6 );
      fprintf( file , "0\n" );

      fprintf( file , "r 0.5 3 0" );
      for ( int i = 0 ; i < frames ; ++i )
        for ( int j = 0 ; j < 2 ; ++j )
          fprintf( file , "r %f %d 0\n" , pro[j] , i * 9 + 7 + j );

      fprintf( file , "e 1 2 " );
      for ( int i = 0 ; i < frames ; ++i )
        fprintf( file , "%d %d %d %d " , i * 9 + 9 , i * 9 + 10 , i * 9 + 11 , i * 9 + 12 );
      fprintf( file , "0\n" );
    }
    else if ( format == "ssat" ) {
      fprintf( file , "%d\n%d\n" , vars , clauses );
      for ( int i = 0 ; i < frames ; ++i )
        fprintf( file , "%d x%d E\n%d x%d E\n%d x%d E\n" , i*9+4 , i*9+4 , i*9+5 , i*9+5 , i*9+6 , i*9+6 );

      fprintf( file , "3 x3 R 0.5\n" );
      for ( int i = 0 ; i < frames ; ++i )
        for ( int j = 0 ; j < 2 ; ++j )
          fprintf( file , "%d x%d R %f\n" , i*9+7+j , i*9+7+j , pro[j] );
      
      fprintf( file , "1 x1 E\n2 x2 E\n" );
      for ( int i = 0 ; i < frames ; ++i )
        fprintf( file , "%d x%d E\n%d x%d E\n%d x%d E\n%d x%d E\n" , i*9+9, i*9+9, i*9+10, i*9+10, i*9+11, i*9+11, i*9+12, i*9+12 );
    }
    // Initial conditions & Goal.
    fprintf( file , "-1 0\n-2 0\n-%d 0\n%d 0\n", vars-1, vars );
      fprintf( file , "%d %d %d 0\n" , 0 * 9 + 4 , 0 * 9 + 5 , 0 * 9 + 6 );
      fprintf( file , "-%d -%d 0\n" , 0 * 9 + 4 , 0 * 9 + 5 );
      fprintf( file , "-%d -%d 0\n" , 0 * 9 + 4 , 0 * 9 + 6 );
      fprintf( file , "-%d -%d 0\n" , 0 * 9 + 5 , 0 * 9 + 6 );
      fprintf( file , "-%d -%d -%d %d 0\n" , 0 * 9 + 4 , 0 * 9 + 3 , 0 * 9 + 7 , 0 * 9 + 10 );
      fprintf( file , "-%d -%d %d -%d 0\n" , 0 * 9 + 4 , 0 * 9 + 3 , 0 * 9 + 7 , 0 * 9 + 10 );
      fprintf( file , "-%d %d -%d %d 0\n"  , 0 * 9 + 4 , 0 * 9 + 3 , 0 * 9 + 8 , 0 * 9 + 10 );
      fprintf( file , "-%d %d %d -%d 0\n"  , 0 * 9 + 4 , 0 * 9 + 3 , 0 * 9 + 8 , 0 * 9 + 10 );
      fprintf( file , "-%d -%d 0\n" , 0 * 9 + 5 , 0 * 9 + 10 );
      fprintf( file , "-%d -%d 0\n" , 0 * 9 + 6 , 0 * 9 + 10 );
      fprintf( file , "-%d -%d %d 0\n" , 0 * 9 + 4 , 0 * 9 + 3 , 0 * 9 + 9 );
      fprintf( file , "-%d %d -%d 0\n" , 0 * 9 + 4 , 0 * 9 + 3 , 0 * 9 + 9 );
      fprintf( file , "-%d -%d %d 0\n" , 0 * 9 + 5 , 0 * 9 + 3 , 0 * 9 + 9 );
      fprintf( file , "-%d %d -%d 0\n" , 0 * 9 + 5 , 0 * 9 + 3 , 0 * 9 + 9 );
      fprintf( file , "-%d -%d %d 0\n" , 0 * 9 + 6 , 0 * 9 + 3 , 0 * 9 + 9 );
      fprintf( file , "-%d %d -%d 0\n" , 0 * 9 + 6 , 0 * 9 + 3 , 0 * 9 + 9 );
      fprintf( file , "-%d -%d %d 0\n" , 0 * 9 + 4 , 0 * 9 + 1 , 0 * 9 + 11 );
      fprintf( file , "-%d %d -%d 0\n" , 0 * 9 + 4 , 0 * 9 + 1 , 0 * 9 + 11 );
      fprintf( file , "-%d -%d %d 0\n" , 0 * 9 + 5 , 0 * 9 + 1 , 0 * 9 + 11 );
      fprintf( file , "-%d %d -%d %d 0\n" , 0 * 9 + 5 , 0 * 9 + 1 , 0 * 9 + 3 , 0 * 9 + 11 );
      fprintf( file , "-%d %d %d -%d 0\n" , 0 * 9 + 5 , 0 * 9 + 1 , 0 * 9 + 3 , 0 * 9 + 11 );
      fprintf( file , "-%d -%d %d 0\n" , 0 * 9 + 6 , 0 * 9 + 1 , 0 * 9 + 11 );
      fprintf( file , "-%d %d -%d -%d 0\n" , 0 * 9 + 6 , 0 * 9 + 1 , 0 * 9 + 3 , 0 * 9 + 11 );
      fprintf( file , "-%d %d %d %d 0\n" , 0 * 9 + 6 , 0 * 9 + 1 , 0 * 9 + 3 , 0 * 9 + 11 );
      fprintf( file , "-%d -%d %d 0\n" , 0 * 9 + 4 , 0 * 9 + 2 , 0 * 9 + 12 );
      fprintf( file , "-%d %d -%d 0\n" , 0 * 9 + 4 , 0 * 9 + 2 , 0 * 9 + 12 );
      fprintf( file , "-%d -%d -%d 0\n" , 0 * 9 + 5 , 0 * 9 + 3 , 0 * 9 + 12 );
      fprintf( file , "-%d %d %d 0\n" , 0 * 9 + 5 , 0 * 9 + 3 , 0 * 9 + 12 );
      fprintf( file , "-%d -%d %d 0\n" , 0 * 9 + 6 , 0 * 9 + 3 , 0 * 9 + 12 );
      fprintf( file , "-%d %d -%d 0\n" , 0 * 9 + 6 , 0 * 9 + 3 , 0 * 9 + 12 );
    for ( int i = 1 ; i < frames ; ++i ) {
      fprintf( file , "%d %d %d 0\n" , i * 9 + 4 , i * 9 + 5 , i * 9 + 6 );
      fprintf( file , "-%d -%d 0\n" , i * 9 + 4 , i * 9 + 5 );
      fprintf( file , "-%d -%d 0\n" , i * 9 + 4 , i * 9 + 6 );
      fprintf( file , "-%d -%d 0\n" , i * 9 + 5 , i * 9 + 6 );
      fprintf( file , "-%d -%d -%d %d 0\n" , i * 9 + 4 , i * 9 , i * 9 + 7 , i * 9 + 10 );
      fprintf( file , "-%d -%d %d -%d 0\n" , i * 9 + 4 , i * 9 , i * 9 + 7 , i * 9 + 10 );
      fprintf( file , "-%d %d -%d %d 0\n"  , i * 9 + 4 , i * 9 , i * 9 + 8 , i * 9 + 10 );
      fprintf( file , "-%d %d %d -%d 0\n"  , i * 9 + 4 , i * 9 , i * 9 + 8 , i * 9 + 10 );
      fprintf( file , "-%d -%d 0\n" , i * 9 + 5 , i * 9 + 10 );
      fprintf( file , "-%d -%d 0\n" , i * 9 + 6 , i * 9 + 10 );
      fprintf( file , "-%d -%d %d 0\n" , i * 9 + 4 , i * 9 , i * 9 + 9 );
      fprintf( file , "-%d %d -%d 0\n" , i * 9 + 4 , i * 9 , i * 9 + 9 );
      fprintf( file , "-%d -%d %d 0\n" , i * 9 + 5 , i * 9 , i * 9 + 9 );
      fprintf( file , "-%d %d -%d 0\n" , i * 9 + 5 , i * 9 , i * 9 + 9 );
      fprintf( file , "-%d -%d %d 0\n" , i * 9 + 6 , i * 9 , i * 9 + 9 );
      fprintf( file , "-%d %d -%d 0\n" , i * 9 + 6 , i * 9 , i * 9 + 9 );
      fprintf( file , "-%d -%d %d 0\n" , i * 9 + 4 , i * 9 + 2 , i * 9 + 11 );
      fprintf( file , "-%d %d -%d 0\n" , i * 9 + 4 , i * 9 + 2 , i * 9 + 11 );
      fprintf( file , "-%d -%d %d 0\n" , i * 9 + 5 , i * 9 + 2 , i * 9 + 11 );
      fprintf( file , "-%d %d -%d %d 0\n" , i * 9 + 5 , i * 9 + 2 , i * 9 , i * 9 + 11 );
      fprintf( file , "-%d %d %d -%d 0\n" , i * 9 + 5 , i * 9 + 2 , i * 9 , i * 9 + 11 );
      fprintf( file , "-%d -%d %d 0\n" , i * 9 + 6 , i * 9 + 2 , i * 9 + 11 );
      fprintf( file , "-%d %d -%d -%d 0\n" , i * 9 + 6 , i * 9 + 2 , i * 9 , i * 9 + 11 );
      fprintf( file , "-%d %d %d %d 0\n" , i * 9 + 6 , i * 9 + 2 , i * 9 , i * 9 + 11 );
      fprintf( file , "-%d -%d %d 0\n" , i * 9 + 4 , i * 9 + 3 , i * 9 + 12 );
      fprintf( file , "-%d %d -%d 0\n" , i * 9 + 4 , i * 9 + 3 , i * 9 + 12 );
      fprintf( file , "-%d -%d -%d 0\n" , i * 9 + 5 , i * 9 , i * 9 + 12 );
      fprintf( file , "-%d %d %d 0\n" , i * 9 + 5 , i * 9 , i * 9 + 12 );
      fprintf( file , "-%d -%d %d 0\n" , i * 9 + 6 , i * 9 , i * 9 + 12 );
      fprintf( file , "-%d %d -%d 0\n" , i * 9 + 6 , i * 9 , i * 9 + 12 );
    }
  }

  fclose( file );
  return 0;
}
