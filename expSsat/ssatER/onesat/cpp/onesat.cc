#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string.h>
using namespace std;

int
main( int argc , char ** argv )
{
   char fName[128];
   bool sdimacs = strcmp( argv[1] , "sdimacs" ) ? 0 : 1;
   int nVar = atoi(argv[2]);
   FILE * out;
   if ( sdimacs ) {
      sprintf( fName , "onesat_%d.sdimacs" , nVar );
      out = fopen( fName , "w" );
      fprintf( out , "c SSAT onesat family with %d variables\n" , nVar );
      fprintf( out , "p cnf %d %d\n" , 2*nVar , nVar );
      fprintf( out , "e" );
      for ( int i = 1 ; i <= nVar ; ++i ) fprintf( out , " %d" , i );
      fprintf( out , " 0\nr 0.5" );
      for ( int i = 1 ; i <= nVar ; ++i ) fprintf( out , " %d" , nVar+i );
      fprintf( out , " 0\n" );
   }
   else {
      sprintf( fName , "onesat_%d.ssat" , nVar );
      out = fopen( fName , "w" );
      fprintf( out , "%d\n%d\n" , 2*nVar , nVar );
      for ( int i = 1 ; i <= nVar ; ++i ) fprintf( out , "%d x%d E\n" , i , i );
      for ( int i = 1 ; i <= nVar ; ++i ) fprintf( out , "%d x%d R 0.5\n" , nVar+i , nVar+i );
   }
   for ( int i = 1 ; i <= nVar ; ++i ) {
      for ( int j = 1 ; j <= i-1 ; ++j )
         fprintf( out , "-%d " , j );
      fprintf( out , "%d" , i );
      for ( int j = 1 ; j <= i ; ++j )
         fprintf( out , " %d" , nVar+j );
      fprintf( out , " 0\n" );
   }
   fclose( out );
   return 0;
}
