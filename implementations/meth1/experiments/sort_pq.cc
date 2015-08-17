/**
 * Test performance of priority_queue's.
 *
 * - Uses C file IO.
 * - Uses own Record struct.
 * - Use C++ std::priority_queue + std::vector.
 */
#include <sys/stat.h>
#include <iostream>

#include "record.hh"
#include "timestamp.hh" 

#include "config.h"
#include "pq.hh"

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include <boost/sort/spreadsort/string_sort.hpp>
using namespace boost::sort::spreadsort;
#endif

using namespace std;

using RR = RecordS;

vector<RR> scan( char * buf, size_t nrecs, size_t size, const RR & after )
{
  auto t0 = time_now();
  size_t cmps = 0, pushes = 0, pops = 0;
  mystl::priority_queue<RR> pq{nrecs / 10 + 1};

  for ( uint64_t i = 0; i < nrecs; i++ ) {
    const char * r = buf + Rec::SIZE * i;
    RecordPtr next( r, i );
    cmps++;
    if ( next > after ) {
      if ( pq.size() < size ) {
        pushes++;
        pq.emplace( r, i );
      } else if ( next < pq.top() ) {
        cmps++; pushes++; pops++;
        pq.emplace( r, i );
        pq.pop();
      } else {
        cmps++;
      }
    }
  }
  auto t1 = time_now();

  vector<RR> vrecs = move( pq.container() );
#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
  string_sort( vrecs.begin(), vrecs.end() );
#else
  sort( vrecs.begin(), vrecs.end() );
#endif

  cout << "pq, " << time_diff<ms>( t1, t0 ) << endl;
  cout << "* cmps   , " << cmps << endl;
  cout << "* push   , " << pushes << endl;
  cout << "* pops   , " << pops << endl;
  cout << endl;

  return vrecs;
}

void run( char * fin )
{
  // open file
  FILE *fdi = fopen( fin, "r" );
  struct stat st;
  fstat( fileno( fdi ), &st );
  size_t nrecs = st.st_size / Rec::SIZE;

  // read file into memory
  auto t0 = time_now();
  char * buf = new char[st.st_size];
  for ( int nr = 0; nr < st.st_size; ) {
    auto r = fread( buf, st.st_size - nr, 1, fdi );
    if ( r <= 0 ) {
      break;
    }
    nr += r;
  }
  cout << "read , " << time_diff<ms>( t0 ) << endl;
  cout << endl;
  
  // stats
  cout << "size, " << nrecs << endl;
  cout << "cunk, " << nrecs / 10 << endl;
  cout << endl;

  // scan file
  auto t1 = time_now();
  RR after( Rec::MIN );
  for ( uint64_t i = 0; i < 10; i++ ) {
    auto recs = scan( buf, nrecs, nrecs / 10, after );
    after = recs.back();
    cout << "last: " << recs[recs.size()-1] << endl;
  }
  cout << endl << "total: " << time_diff<ms>( t1 ) << endl;
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 2 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( argv[1] );
  } catch ( const exception & e ) {
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}


