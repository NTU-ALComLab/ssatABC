#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string.h>
#include <math.h>
#include <vector>
using namespace std;

bool
increBinary( vector<bool> & phase )
{
   for ( int i = 0 ; i < phase.size() ; ++i ) {
      if ( !phase[i] ) {
         phase[i] = true;
         for ( int j = 0 ; j < i ; ++j ) phase[j] = false;
         return true;
      }
   }
   return false;
}

int
main( int argc , char ** argv )
{
   char fName[128];
   bool sdimacs = strcmp( argv[1] , "sdimacs" ) ? 0 : 1;
   int nVar = atoi(argv[2]);
   int nCla = pow(2,nVar)-1;
   FILE * out;
   vector<bool> phase( nVar , false );
   if ( sdimacs ) {
      sprintf( fName , "minterm_%d.sdimacs" , nVar );
      out = fopen( fName , "w" );
      fprintf( out , "c SSAT minterm family with %d variables\n" , nVar );
      fprintf( out , "p cnf %d %d\n" , 2*nVar , nCla );
      fprintf( out , "e" );
      for ( int i = 1 ; i <= nVar ; ++i ) fprintf( out , " %d" , i );
      fprintf( out , " 0\nr 0.5" );
      for ( int i = 1 ; i <= nVar ; ++i ) fprintf( out , " %d" , nVar+i );
      fprintf( out , " 0\n" );
   }
   else {
      sprintf( fName , "minterm_%d.ssat" , nVar );
      out = fopen( fName , "w" );
      fprintf( out , "%d\n%d\n" , 2*nVar , nVar );
      for ( int i = 1 ; i <= nVar ; ++i ) fprintf( out , "%d x%d E\n" , i , i );
      for ( int i = 1 ; i <= nVar ; ++i ) fprintf( out , "%d x%d R 0.5\n" , nVar+i , nVar+i );
   }
   do {
      bool stop = true;
      for ( int i = 0 ; i < phase.size() ; ++i ) 
         if ( !phase[i] ) { stop = false; break; }
      if ( !stop ) {
         for ( int i = 0 ; i < phase.size() ; ++i )
            fprintf( out , "%s%d " , phase[i] ? "-":"" , i+1 );
         for ( int i = 0 ; i < phase.size() ; ++i )
            fprintf( out , "%s%d " , phase[i] ? "-":"" , nVar+i+1 );
         fprintf( out , "0\n" );
      }
   } while ( increBinary(phase) );
   fclose( out );
   return 0;
}
