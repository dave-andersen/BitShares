#include <bts/blockchain/block.hpp>
#include <bts/proof_of_work.hpp>
#include <bts/difficulty.hpp>
#include <bts/config.hpp>
#include <bts/small_hash.hpp>
#include <bts/momentum.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/variant.hpp>
namespace bts { namespace blockchain  {


  /**
   * Creates the gensis block and returns it.
  trx_block create_genesis_block()
  {
    try {
      trx_block b;
      b.version    = 0;
      b.prev       = block_id_type();
      b.block_num  = 0;
      b.timestamp  = fc::time_point::from_iso_string("20130730T054434");

      signed_transaction coinbase;
      coinbase.version = 0;
    //  coinbase.valid_after = 0;
    //  coinbase.valid_blocks = 0;

      // TODO: init from PTS here...
      coinbase.outputs.push_back( 
         trx_output( claim_by_signature_output( address("GmckPDdjQejZBP3t2gZqCqmEfi4") ), asset( 1000000., asset::bts) ) );
      
      b.trxs.emplace_back( std::move(coinbase) );
      b.trx_mroot   = b.calculate_merkle_root();

      return b;
    } FC_RETHROW_EXCEPTIONS( warn, "error creating gensis block" );
  }
  */

  trx_block::operator full_block()const
  {
    full_block b( (const block_header&)*this );
    b.trx_ids.reserve( trxs.size() );
    for( auto itr = trxs.begin(); itr != trxs.end(); ++itr )
    {
      b.trx_ids.push_back( itr->id() );
    }
    return b;
  }


  uint160 trx_block::calculate_merkle_root()const
  {
     if( trxs.size() == 0 ) return uint160();
     if( trxs.size() == 1 ) return trxs.front().id();

     std::vector<uint160> layer_one;
     for( auto itr = trxs.begin(); itr != trxs.end(); ++itr )
     {
       layer_one.push_back(itr->id());
     }
     std::vector<uint160> layer_two;
     while( layer_one.size() > 1 )
     {
        if( layer_one.size() % 2 == 1 )
        {
          layer_one.push_back( uint160() );
        }

        static_assert( sizeof(uint160[2]) == 40, "validate there is no padding between array items" );
        for( uint32_t i = 0; i < layer_one.size(); i += 2 )
        {
            layer_two.push_back(  small_hash( (char*)&layer_one[i], 2*sizeof(uint160) ) );
        }

        layer_one = std::move(layer_two);
     }
     return layer_one.front();
  }

  uint64_t block_header::get_required_difficulty(uint64_t prev_difficulty,uint64_t prev_avail_cdays)const
  {
      prev_avail_cdays /= BLOCKS_PER_YEAR;
      uint64_t cdd = total_cdd > prev_avail_cdays ? prev_avail_cdays : total_cdd;
      uint64_t factor = prev_avail_cdays - cdd ;
      factor += 1;
      // as CDD approaches the average CDD per block the factor approaches 0
      // as CDD approaches 0 factor approaches total_shares^2
      // if we have the threshold CDD then there is no difficulty penalty...
      // otherwise the difficulty goes up dramatically.
      return prev_difficulty * factor;
  }
  uint64_t block_header::get_difficulty()const
  {
     return bts::difficulty(id());
  }


  bool    block_header::validate_work()const
  {
     block_header tmp = *this;
     tmp.noncea = 0;
     tmp.nonceb = 0;
     auto tmp_id = tmp.id();
     auto seed = fc::sha256::hash( (char*)&tmp_id, sizeof(tmp_id) );
     return momentum_verify( seed, noncea, nonceb );
  }

  /**
   *  @return the digest of the block header used to evaluate the proof of work
   */
  block_id_type block_header::id()const
  {
     fc::sha512::encoder enc;
     fc::raw::pack( enc, *this );
     return small_hash( enc.result() );
  }

} } // bts::blockchain
