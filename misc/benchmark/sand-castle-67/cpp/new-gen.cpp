#include <iostream>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <string.h>

using namespace std;

int main( int argc , char ** argv )
{
  bool sdimacs = false , blif = false , multi = false;;
  int frames;
  int vars, clauses;
  char name[128];
  double pro[5] = { 0.5 , 0.67 , 0.25 , 0.75 , 0.5 };
  FILE * file;

  if( argc != 4 ) {
    cout << "./gen <format> <# of frames> <prefix type>" << '\n';
    return 1;
  }
  if      ( !strcmp( argv[1] , "sdimacs" ) ) sdimacs = true;
  else if ( !strcmp( argv[1] , "ssat"    ) ) sdimacs = false;
  else if ( !strcmp( argv[1] , "blif"    ) ) blif = true;
  else {
     cout << "Unknown format " << argv[1] << endl;
     return 1;
  }
  frames = atoi(argv[2]);
  if      ( !strcmp( argv[3] , "multi"  ) ) multi = true;
  else if ( !strcmp( argv[3] , "single" ) ) multi = false;
  else {
     cout << "Unknown prefix type " << argv[3] << endl;
     return 1;
  }
  cout << "  > Get # of frames = " << frames << " , format = " << ( blif ? "blif" : (sdimacs ? "sdimacs" : "ssat") ) 
       << " , multi = " << (multi ? "true" : "false") << endl;

  if( frames < 1 ) {
    cout << "  > # of frames must be greater than 0." << '\n';
    return 1;
  }

  sprintf( name , "SC-%d_%s.%s" , frames , (multi ? "m" : "s") , blif ? "blif" : (sdimacs ? "sdimacs" : "ssat" ) );
  file = fopen( name , "w" );
  
  if( blif ) {
     fprintf( file , "c primary inputs quantification\n" );
     for ( int i = 0 ; i < frames ; ++i ) {
        fprintf( file , "c input CHOOSE_%d E\n", i );
        fprintf( file , "c input D1_%d R %f\n" , i , pro[0] );
        fprintf( file , "c input E1_%d R %f\n" , i , pro[1] );
        fprintf( file , "c input E2_%d R %f\n" , i , pro[2] );
        fprintf( file , "c input E3_%d R %f\n" , i , pro[3] );
        fprintf( file , "c input E4_%d R %f\n" , i , pro[4] );
     }
    fprintf( file , ".model SC%d\n" , frames );
    fprintf( file , ".inputs" );
    if ( multi ) {
       for( int i = 0 ; i < frames ; ++i ) {
         fprintf( file , " CHOOSE_%d", i );
         fprintf( file , " D1_%d E1_%d E2_%d E3_%d E4_%d", i , i , i , i , i );
       }
    }
    else {
       for( int i = 0 ; i < frames ; ++i )
          fprintf( file , " CHOOSE_%d", i );
       for( int i = 0 ; i < frames ; ++i ) 
         fprintf( file , " D1_%d E1_%d E2_%d E3_%d E4_%d", i , i , i , i , i );
    }
    fprintf( file , "\n.outputs CAS_%d\n" , frames );
    for( int i = 0 ; i < frames ; ++i ) {
      fprintf( file , ".subckt SC0 D1=D1_%d E1=E1_%d E2=E2_%d E3=E3_%d E4=E4_%d " , i , i , i , i , i ); 
      fprintf( file , "CAS_pre=CAS_%d MOA_pre=MOA_%d CHOOSE=CHOOSE_%d CAS_nex=CAS_%d MOA_nex=MOA_%d\n" , i , i , i , i + 1 , i + 1 ); 
    }
    fprintf( file , ".names CONST0\n" );
    fprintf( file , ".names CONST1\n" );
    fprintf( file , "1\n" );
    fprintf( file , ".names CONST0 MOA_0\n" );
    fprintf( file , "1 1\n" );
    fprintf( file , ".names CONST0 CAS_0\n" );
    fprintf( file , "1 1\n" );
    fprintf( file , ".end\n\n" );
    fprintf( file , ".model SC0\n" );
    fprintf( file , ".inputs D1 E1 E2 E3 E4 CAS_pre MOA_pre CHOOSE\n" );
    fprintf( file , ".outputs CAS_nex MOA_nex\n" );
    fprintf( file , ".subckt MUX21 A=CAS_pre B=OR1   Control=CHOOSE  COUT=CAS_nex \n" );
    fprintf( file , ".names CAS_pre MUX1 OR1\n" );
    fprintf( file , "00 0\n" );
    fprintf( file , ".subckt MUX21 A=E1      B=E2    Control=MOA_pre COUT=MUX1\n" );
    fprintf( file , ".subckt MUX21 A=OR2     B=AND1  Control=CHOOSE  COUT=MOA_nex\n" );
    fprintf( file , ".names MOA_pre D1 OR2\n" );
    fprintf( file , "00 0\n" );
    fprintf( file , ".names MOA_pre MUX2 AND1\n" );
    fprintf( file , "11 1\n" );
    fprintf( file , ".subckt MUX21 A=E3      B=OR3   Control=CAS_pre COUT=MUX2\n" );
    fprintf( file , ".names CAS_nex E4 OR3\n" );
    fprintf( file , "00 0\n" );
    fprintf( file , ".end\n\n" );
    fprintf( file , ".model MUX21\n" );
    fprintf( file , ".inputs A B Control\n" );
    fprintf( file , ".outputs COUT\n" );
    fprintf( file , ".names A Control o1\n" );
    fprintf( file , "11 1\n" );
    fprintf( file , ".names Control B o2\n" );
    fprintf( file , "01 1\n" );
    fprintf( file , ".names o1 o2 COUT\n" );
    fprintf( file , "00 0\n" );
    fprintf( file , ".end\n" );
  }
  else {
    vars = 2 + 9 * frames;
    clauses = 3 + 18 * frames;

    if ( sdimacs ) fprintf( file , "p cnf %d %d\n" , vars , clauses );
    else           fprintf( file , "%d\n%d\n" , vars , clauses );

    if ( sdimacs ) {
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
    }
    else {
      // with multi frames ordering !
      for ( int i = 0 ; i < frames ; ++i )
      { // <--
        fprintf( file , "%d x%d E\n%d x%d E\n" , i*9+3 , i*9+3 , i*9+4 , i*9+4 );

      // for ( int i = 0 ; i < frames ; ++i )
        for ( int j = 0 ; j < 5 ; ++j )
          fprintf( file , "%d x%d R %f\n" , i * 9 + 5 + j , i*9+5+j , pro[j] );
      } // <--

      fprintf( file , "1 x1 E\n2 x2 E\n" );
      for ( int i = 0 ; i < frames ; ++i )
        fprintf( file , "%d x%d E\n%d x%d E\n" , i * 9 + 10 , i*9+10, i * 9 + 11 , i*9+11 );
    }
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
  }
  fclose( file );
  return 0;
}
