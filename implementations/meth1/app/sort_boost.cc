/**
 * Test program. Takes gensort file as input and reads whole thing into memory,
 * sorting using C++ algorithm sort implementation.
 *
 * This version uses boost sorting algorithms.
 */
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <vector>

#include "exception.hh"
#include "util.hh"

#include <boost/sort/spreadsort/string_sort.hpp>

using namespace std;
using namespace boost::sort::spreadsort;

// sizes of records
static constexpr size_t KEY_BYTES = 10;
static constexpr size_t VAL_BYTES = 90;
static constexpr size_t REC_BYTES = KEY_BYTES + VAL_BYTES;

// large array for record values
static char * all_vals;
static char * cur_v;

// our record struct
struct Rec
{
  // Pulling the key inline with Rec improves sort performance by 2x. Appears
  // to be due to saving an indirection during sort.
  char key_[KEY_BYTES];
  char * val_;

  Rec() : val_{nullptr} {}

  inline void read( FILE *fdi )
  {
    val_ = cur_v;
    cur_v += VAL_BYTES;
    fread( key_, KEY_BYTES, 1, fdi );
    fread( val_, VAL_BYTES, 1, fdi );
  }

  inline size_t write( FILE *fdo ) const
  {
    size_t n = 0;
    n += fwrite( key_, KEY_BYTES, 1, fdo );
    n += fwrite( val_, VAL_BYTES, 1, fdo );
    return n;
  }

  inline bool operator<( const Rec & b ) const
  {
    return compare( b ) < 0 ? true : false;
  }

  inline int compare( const Rec & b ) const
  {
    return memcmp( key_, b.key_, KEY_BYTES );
  }

  inline const char * data( void ) const
  {
    return key_;
  }

  inline unsigned char operator[]( size_t offset ) const
  {
    return key_[offset];
  }

  inline size_t size( void ) const
  {
    return KEY_BYTES;
  }
} __attribute__((packed));

// All records (key + ptr to value)
static vector<Rec> all_recs{};

int run( char * argv[] )
{
  // startup
  FILE *fdi = fopen( argv[1], "r" );
  FILE *fdo = fopen( argv[2], "w" );

  // get filesize
  struct stat st;
  fstat( fileno( fdi ), &st );
  size_t nrecs = st.st_size / REC_BYTES;

  // setup space for data
  cur_v = all_vals = new char[ nrecs * VAL_BYTES ];
  all_recs = vector<Rec>( nrecs );

  auto t1 = chrono::high_resolution_clock::now();

  // read
  for ( uint64_t i = 0; i < nrecs; i++ ) {
    all_recs[i].read( fdi );
  }
  fclose( fdi );
  auto t2 = chrono::high_resolution_clock::now();

  // sort
  string_sort( all_recs.begin(), all_recs.end() );
  auto t3 = chrono::high_resolution_clock::now();

  // write
  for ( uint64_t i = 0; i < nrecs; i++ ) {
    all_recs[i].write( fdo );
  }
  fflush( fdo );
  fsync( fileno( fdo ) );
  fclose( fdo );
  auto t4 = chrono::high_resolution_clock::now();

  // stats
  auto t21 = chrono::duration_cast<chrono::milliseconds>( t2 - t1 ).count();
  auto t32 = chrono::duration_cast<chrono::milliseconds>( t3 - t2 ).count();
  auto t43 = chrono::duration_cast<chrono::milliseconds>( t4 - t3 ).count();
  auto t41 = chrono::duration_cast<chrono::milliseconds>( t4 - t1 ).count();

  cout << "Read  took " << t21 << "ms" << endl;
  cout << "Sort  took " << t32 << "ms" << endl;
  cout << "Write took " << t43 << "ms" << endl;
  cout << "Total took " << t41 << "ms" << endl;

  return EXIT_SUCCESS;
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file] [out]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( argv );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}


