#ifndef METH1_NODE_HH
#define METH1_NODE_HH

#include <mutex>
#include <condition_variable>

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include "boost_d_ary_heap.hh"
#endif

#include "buffered_io.hh"
#include "file.hh"
#include "socket.hh"

#include "implementation.hh"
#include "priority_queue.hh"
#include "record.hh"

#define OVERLAP_IO 1

/**
 * Stratergy 1.
 * - No upfront work.
 * - Full linear scan for each record.
 */
namespace meth1
{

/**
 * Node defines the backend running on each node with a subset of the data.
 */
class Node : public Implementation
{
private:
  File data_;
#if OVERLAP_IO == 0
  BufferedIO bio_;
#endif
  std::string port_;
  Record last_;
  uint64_t fpos_;
  uint64_t max_mem_;

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
  using PQ = boost::heap::d_ary_heap<Record, boost::heap::arity<6>>;
  // using PQ = mystl::priority_queue<Record>;
#else
  using PQ = mystl::priority_queue<Record>;
#endif

public:
  Node( std::string file, std::string port, uint64_t max_memory );

  /* No copy */
  Node( const Node & n ) = delete;
  Node & operator=( const Node & n ) = delete;

  /* Allow move */
  Node( Node && n ) = delete;
  Node & operator=( Node && n ) = delete;

  /* Run the node - list and respond to RPCs */
  void Run( void );

private:
  void DoInitialize( void );
  std::vector<Record> DoRead( uint64_t pos, uint64_t size );
  uint64_t DoSize( void );

  std::vector<Record> rec_sort( PQ recs );
  Record seek( uint64_t pos );

  PQ linear_scan( const Record & after, uint64_t size = 1 );

  void RPC_Read( BufferedIO_O<TCPSocket> & client );
  void RPC_Size( BufferedIO_O<TCPSocket> & client );
};
}

#endif /* METH1_NODE_HH */
