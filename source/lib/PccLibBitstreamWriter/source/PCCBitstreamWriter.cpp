/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2017, ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "PCCBitstreamCommon.h"
#include "PCCBitstream.h"
#include "PCCHighLevelSyntax.h"
#include "PCCAtlasAdaptationParameterSetRbsp.h"

#include "PCCBitstreamWriter.h"

using namespace std;
using namespace pcc;

PCCBitstreamWriter::PCCBitstreamWriter()  = default;
PCCBitstreamWriter::~PCCBitstreamWriter() = default;

int32_t PCCBitstreamWriter::write( SampleStreamNalUnit& ssnu, PCCBitstream& bitstream ) {
  TRACE_BITSTREAM( "PCCBitstreamXXcoder: SampleStream Nal Unit start \n" );
  ssnu.setSizePrecisionBytesMinus1( 3 );
  sampleStreamNalHeader( bitstream, ssnu );
  // sampleStreamNalUnit( bitstream, ssnu, ssnu.getNalUnit() );
  //  for(auto& nalUnit : ssnu.getNalUnit() ){
  //  sampleStreamNalUnit( bitstream, ssnu, ssnu.getNalUnit() );
  //  }
  TRACE_BITSTREAM( "PCCBitstreamXXcoder: SampleStream Nal Unit start done \n" );
  return 0;
}

size_t PCCBitstreamWriter::write( SampleStreamV3CUnit& ssvu, PCCBitstream& bitstream ) {
  TRACE_BITSTREAM( "PCCBitstreamXXcoder: SampleStream Vpcc Unit start \n" );
  size_t headerSize = 0;
  // Calculating the precision of the unit size
  uint32_t maxUnitSize = 0;
  for ( auto& v3cUnit : ssvu.getV3CUnit() ) {
    if ( maxUnitSize < v3cUnit.getSize() ) { maxUnitSize = static_cast<uint32_t>( v3cUnit.getSize() ); }
  }
  TRACE_BITSTREAM( "maxUnitSize = %u \n", maxUnitSize );
  uint32_t precision = static_cast<uint32_t>(
      min( max( static_cast<int>( ceil( static_cast<double>( ceilLog2( maxUnitSize ) ) / 8.0 ) ), 1 ), 8 ) - 1 );
  TRACE_BITSTREAM( " => SsvhUnitSizePrecisionBytesMinus1 = %u \n", precision );
  ssvu.setSsvhUnitSizePrecisionBytesMinus1( precision );
  sampleStreamV3CHeader( bitstream, ssvu );
  headerSize += 1;
  TRACE_BITSTREAM( "UnitSizePrecisionBytesMinus1 %d <=> %d / 8 - 1\n", precision, ceilLog2( maxUnitSize ) );
  size_t unitCount = 0;
  for ( auto& v3cUnit : ssvu.getV3CUnit() ) {
    sampleStreamV3CUnit( bitstream, ssvu, v3cUnit );
    TRACE_BITSTREAM( "V3C Unit Size(unit type:%zu, %zuth/%zu)  = %zu \n", unitCount, (size_t)v3cUnit.getType(),
                     ssvu.getV3CUnitCount(), v3cUnit.getSize() );
    unitCount++;
    headerSize += ssvu.getSsvhUnitSizePrecisionBytesMinus1() + 1;
  }
  TRACE_BITSTREAM( "PCCBitstreamXXcoder: SampleStream Vpcc Unit start done \n" );
  return headerSize;
}

// B.2 Sample stream V3C unit syntax and semantics
// B.2.1 Sample stream V3C header syntax
void PCCBitstreamWriter::sampleStreamV3CHeader( PCCBitstream& bitstream, SampleStreamV3CUnit& ssvu ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( ssvu.getSsvhUnitSizePrecisionBytesMinus1(), 3 );  // u(3)
  bitstream.write( 0, 5 );                                           // u(5)
}

// B.2.2 Sample stream V3C unit syntax
void PCCBitstreamWriter::sampleStreamV3CUnit( PCCBitstream& bitstream, SampleStreamV3CUnit& ssvu, V3CUnit& v3cUnit ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( v3cUnit.getSize(),
                   8 * ( ssvu.getSsvhUnitSizePrecisionBytesMinus1() + 1 ) );  // u(v)
  TRACE_BITSTREAM( "V3CUnitType: %hhu V3CUnitSize: %zu\n", (uint8_t)v3cUnit.getType(), v3cUnit.getSize() );
  bitstream.copyFrom( v3cUnit.getBitstream(), 0, v3cUnit.getSize() );
}

int PCCBitstreamWriter::encode( PCCHighLevelSyntax& syntax, SampleStreamV3CUnit& ssvu ) {
  auto& vuhGVD = syntax.getV3CUnitHeaderGVD();
  auto& vuhAD  = syntax.getV3CUnitHeaderAD();
  auto& vuhAVD = syntax.getV3CUnitHeaderAVD();
  auto& vuhOVD = syntax.getV3CUnitHeaderOVD();
  auto& sps    = syntax.getVps();
  auto& asps   = syntax.getAtlasSequenceParameterSet( 0 );  // TODO: get correct value
  vuhGVD.setV3CParameterSetId( sps.getV3CParameterSetId() );
  vuhAD.setV3CParameterSetId( sps.getV3CParameterSetId() );
  vuhAVD.setV3CParameterSetId( sps.getV3CParameterSetId() );
  vuhOVD.setV3CParameterSetId( sps.getV3CParameterSetId() );
  syntax.getBitstreamStat().newGOF();
  // encode the SPS
  PCCBitstream bitstreamVPS;
#ifdef BITSTREAM_TRACE
  bitstreamVPS.setTrace( true );
  bitstreamVPS.setTraceFile( traceFile_ );
  bitstreamVPS.trace( "PCCBitstream::(V3C_VPS)\n" );
#endif
  v3cUnit( syntax, bitstreamVPS, V3C_VPS );
  ssvu.addV3CUnit().setBitstream( std::move( bitstreamVPS ), V3C_VPS );
  for ( uint8_t atlasIdx = 0; atlasIdx < sps.getAtlasCountMinus1() + 1; atlasIdx++ ) {
    vuhAD.setAtlasId( atlasIdx );
    vuhOVD.setAtlasId( atlasIdx );
    vuhGVD.setAtlasId( atlasIdx );
    vuhAVD.setAtlasId( atlasIdx );
    // encode the AD
    PCCBitstream bitstreamAD;
#ifdef BITSTREAM_TRACE
    bitstreamAD.setTrace( true );
    bitstreamAD.setTraceFile( traceFile_ );
    bitstreamAD.trace( "PCCBitstream::(V3C_AD)\n" );
#endif
    v3cUnit( syntax, bitstreamAD, V3C_AD );
    ssvu.addV3CUnit().setBitstream( std::move( bitstreamAD ), V3C_AD );
    // encode OVD
    PCCBitstream bitstreamOVD;
#ifdef BITSTREAM_TRACE
    bitstreamOVD.setTrace( true );
    bitstreamOVD.setTraceFile( traceFile_ );
    bitstreamOVD.trace( "PCCBitstream::(V3C_OVD)\n" );
#endif
    v3cUnit( syntax, bitstreamOVD, V3C_OVD );
    ssvu.addV3CUnit().setBitstream( std::move( bitstreamOVD ), V3C_OVD );
    // encode GVD
    if ( sps.getMapCountMinus1( atlasIdx ) > 0 && sps.getMultipleMapStreamsPresentFlag( atlasIdx ) ) {
      for ( uint32_t mapIdx = 0; mapIdx < sps.getMapCountMinus1( atlasIdx ) + 1; mapIdx++ ) {
        PCCBitstream bitstreamGVD;
#ifdef BITSTREAM_TRACE
        bitstreamGVD.setTrace( true );
        bitstreamGVD.setTraceFile( traceFile_ );
        bitstreamGVD.trace( "PCCBitstream::(V3C_GVD)\n" );
#endif
        // encode D(mapIdx)
        vuhGVD.setMapIndex( mapIdx );
        vuhGVD.setAuxiliaryVideoFlag( false );
        v3cUnit( syntax, bitstreamGVD, V3C_GVD );
        ssvu.addV3CUnit().setBitstream( std::move( bitstreamGVD ), V3C_GVD );
      }
    } else {
      PCCBitstream bitstreamGVD;
#ifdef BITSTREAM_TRACE
      bitstreamGVD.setTrace( true );
      bitstreamGVD.setTraceFile( traceFile_ );
      bitstreamGVD.trace( "PCCBitstream::(V3C_GVD)\n" );
#endif
      // encode D=D(0)|D(1)|...|D(N)
      vuhGVD.setMapIndex( 0 );
      vuhGVD.setAuxiliaryVideoFlag( false );
      v3cUnit( syntax, bitstreamGVD, V3C_GVD );
      ssvu.addV3CUnit().setBitstream( std::move( bitstreamGVD ), V3C_GVD );
    }
    if ( asps.getRawPatchEnabledFlag() && sps.getAuxiliaryVideoPresentFlag( atlasIdx ) ) {
      PCCBitstream bitstreamGVD;
#ifdef BITSTREAM_TRACE
      bitstreamGVD.setTrace( true );
      bitstreamGVD.setTraceFile( traceFile_ );
      bitstreamGVD.trace( "PCCBitstream::(V3C_GVD)\n" );
#endif
      // encode RAW
      vuhGVD.setMapIndex( 0 );
      vuhGVD.setAuxiliaryVideoFlag( true );
      v3cUnit( syntax, bitstreamGVD, V3C_GVD );
      ssvu.addV3CUnit().setBitstream( std::move( bitstreamGVD ), V3C_GVD );
    }
    for ( int attIdx = 0; attIdx < sps.getAttributeInformation( atlasIdx ).getAttributeCount(); attIdx++ ) {
      vuhAVD.setAttributeIndex( attIdx );
      for ( int attDim = 0;
            attDim < sps.getAttributeInformation( atlasIdx ).getAttributeDimensionPartitionsMinus1( attIdx ) + 1;
            attDim++ ) {
        vuhAVD.setAttributeDimensionIndex( attDim );
        // encode AVD
        if ( sps.getMapCountMinus1( atlasIdx ) > 0 && sps.getMultipleMapStreamsPresentFlag( atlasIdx ) ) {
          for ( uint32_t mapIdx = 0; mapIdx < sps.getMapCountMinus1( atlasIdx ) + 1; mapIdx++ ) {
            PCCBitstream bitstreamAVD;
#ifdef BITSTREAM_TRACE
            bitstreamAVD.setTrace( true );
            bitstreamAVD.setTraceFile( traceFile_ );
            bitstreamAVD.trace( "PCCBitstream::(V3C_AVD)\n" );
#endif
            // encode D(mapIdx)
            vuhAVD.setAttributeIndex( attIdx );
            vuhAVD.setAttributeDimensionIndex( attDim );
            vuhAVD.setMapIndex( mapIdx );
            vuhAVD.setAuxiliaryVideoFlag( false );
            v3cUnit( syntax, bitstreamAVD, V3C_AVD );
            ssvu.addV3CUnit().setBitstream( std::move( bitstreamAVD ), V3C_AVD );
          }
        } else {
          PCCBitstream bitstreamAVD;
#ifdef BITSTREAM_TRACE
          bitstreamAVD.setTrace( true );
          bitstreamAVD.setTraceFile( traceFile_ );
          bitstreamAVD.trace( "PCCBitstream::(V3C_AVD)\n" );
#endif
          // encode D=D(0)|D(1)|...|D(N)
          vuhAVD.setAttributeIndex( attIdx );
          vuhAVD.setAttributeDimensionIndex( attDim );
          vuhAVD.setMapIndex( 0 );
          vuhAVD.setAuxiliaryVideoFlag( false );
          v3cUnit( syntax, bitstreamAVD, V3C_AVD );
          ssvu.addV3CUnit().setBitstream( std::move( bitstreamAVD ), V3C_AVD );
        }
        if ( asps.getRawPatchEnabledFlag() && sps.getAuxiliaryVideoPresentFlag( atlasIdx ) ) {
          PCCBitstream bitstreamAVD;
#ifdef BITSTREAM_TRACE
          bitstreamAVD.setTrace( true );
          bitstreamAVD.setTraceFile( traceFile_ );
          bitstreamAVD.trace( "PCCBitstream::(V3C_AVD)\n" );
#endif
          // encode RAW
          vuhAVD.setAttributeIndex( attIdx );
          vuhAVD.setAttributeDimensionIndex( attDim );
          vuhAVD.setMapIndex( 0 );
          vuhAVD.setAuxiliaryVideoFlag( true );
          v3cUnit( syntax, bitstreamAVD, V3C_AVD );
          ssvu.addV3CUnit().setBitstream( std::move( bitstreamAVD ), V3C_AVD );
        }
      }
    }
  }
  return 0;
}

void PCCBitstreamWriter::videoSubStream( PCCHighLevelSyntax& syntax,
                                         PCCBitstream&       bitstream,
                                         V3CUnitType         V3CUnitType ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto&  sps        = syntax.getVps();
  auto&  vuh        = syntax.getV3CUnitHeader( static_cast<int>( V3CUnitType ) - 1 );
  size_t atlasIndex = vuh.getAtlasId();
  if ( V3CUnitType == V3C_OVD ) {
    TRACE_BITSTREAM( "OccupancyMap \n" );
    bitstream.write( syntax.getVideoBitstream( VIDEO_OCCUPANCY ) );
    syntax.getBitstreamStat().setVideoBinSize( VIDEO_OCCUPANCY, syntax.getVideoBitstream( VIDEO_OCCUPANCY ).size() );
  } else if ( V3CUnitType == V3C_GVD ) {
    if ( vuh.getAuxiliaryVideoFlag() ) {
      TRACE_BITSTREAM( "Geometry raw\n" );
      bitstream.write( syntax.getVideoBitstream( VIDEO_GEOMETRY_RAW ) );
      syntax.getBitstreamStat().setVideoBinSize( VIDEO_GEOMETRY_RAW,
                                                 syntax.getVideoBitstream( VIDEO_GEOMETRY_RAW ).size() );
    } else {
      if ( sps.getMapCountMinus1( atlasIndex ) > 0 && sps.getMultipleMapStreamsPresentFlag( atlasIndex ) ) {
        auto geometryIndex = static_cast<PCCVideoType>( VIDEO_GEOMETRY_D0 + vuh.getMapIndex() );
        TRACE_BITSTREAM( "Geometry MAP: %d\n", vuh.getMapIndex() );
        bitstream.write( syntax.getVideoBitstream( geometryIndex ) );
        syntax.getBitstreamStat().setVideoBinSize( geometryIndex, syntax.getVideoBitstream( geometryIndex ).size() );
      } else {
        TRACE_BITSTREAM( "Geometry \n" );
        bitstream.write( syntax.getVideoBitstream( VIDEO_GEOMETRY ) );
        syntax.getBitstreamStat().setVideoBinSize( VIDEO_GEOMETRY, syntax.getVideoBitstream( VIDEO_GEOMETRY ).size() );
      }
    }
  } else if ( V3CUnitType == V3C_AVD ) {
    if ( sps.getAttributeInformation( atlasIndex ).getAttributeCount() > 0 ) {
      if ( vuh.getAuxiliaryVideoFlag() ) {
        auto textureIndex = static_cast<PCCVideoType>( VIDEO_TEXTURE_RAW + vuh.getAttributeDimensionIndex() );
        TRACE_BITSTREAM( "Texture raw, PARTITION: %d\n", vuh.getAttributeDimensionIndex() );
        bitstream.write( syntax.getVideoBitstream( textureIndex ) );
        syntax.getBitstreamStat().setVideoBinSize( textureIndex, syntax.getVideoBitstream( textureIndex ).size() );
      } else {
        if ( sps.getMapCountMinus1( atlasIndex ) > 0 && sps.getMultipleMapStreamsPresentFlag( atlasIndex ) ) {
          auto textureIndex = static_cast<PCCVideoType>(
              VIDEO_TEXTURE_T0 + vuh.getMapIndex() * MAX_NUM_ATTR_PARTITIONS + vuh.getAttributeDimensionIndex() );
          TRACE_BITSTREAM( "Texture MAP: %d, PARTITION: %d\n", vuh.getMapIndex(), vuh.getAttributeDimensionIndex() );
          bitstream.write( syntax.getVideoBitstream( textureIndex ) );
          syntax.getBitstreamStat().setVideoBinSize( textureIndex, syntax.getVideoBitstream( textureIndex ).size() );
        } else {
          auto textureIndex = static_cast<PCCVideoType>( VIDEO_TEXTURE + vuh.getAttributeDimensionIndex() );
          TRACE_BITSTREAM( "Texture PARTITION: %d\n", vuh.getAttributeDimensionIndex() );
          bitstream.write( syntax.getVideoBitstream( textureIndex ) );
          syntax.getBitstreamStat().setVideoBinSize( textureIndex, syntax.getVideoBitstream( textureIndex ).size() );
        }
      }
    }
  }
}

// 7.3.2.1 General V3C unit syntax
void PCCBitstreamWriter::v3cUnit( PCCHighLevelSyntax& syntax, PCCBitstream& bitstream, V3CUnitType V3CUnitType ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto position = static_cast<int32_t>( bitstream.size() );
  v3cUnitHeader( syntax, bitstream, V3CUnitType );
  v3cUnitPayload( syntax, bitstream, V3CUnitType );
  syntax.getBitstreamStat().setV3CUnitSize( V3CUnitType, static_cast<int32_t>( bitstream.size() ) - position );
  TRACE_BITSTREAM( "v3cUnit: V3CUnitType = %d(%s) \n", V3CUnitType, toString( V3CUnitType ).c_str() );
  TRACE_BITSTREAM( "v3cUnit: size [%d ~ %d] \n", position, bitstream.size() );
  TRACE_BITSTREAM( "%s done\n", __func__ );
}

// 7.3.2.2 V3C unit header syntax
void PCCBitstreamWriter::v3cUnitHeader( PCCHighLevelSyntax& syntax, PCCBitstream& bitstream, V3CUnitType v3cUnitType ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( v3cUnitType, 5 );  // u(5)
  if ( v3cUnitType == V3C_AVD || v3cUnitType == V3C_GVD || v3cUnitType == V3C_OVD || v3cUnitType == V3C_AD ) {
    auto& vpcc = syntax.getV3CUnitHeader( static_cast<int>( v3cUnitType ) - 1 );
    bitstream.write( vpcc.getV3CParameterSetId(), 4 );  // u(4)
    bitstream.write( vpcc.getAtlasId(), 6 );            // u(6)
  }
  if ( v3cUnitType == V3C_AVD ) {
    auto& vpcc = syntax.getV3CUnitHeaderAVD();
    bitstream.write( vpcc.getAttributeIndex(), 7 );           // u(7)
    bitstream.write( vpcc.getAttributeDimensionIndex(), 5 );  // u(5)
    bitstream.write( vpcc.getMapIndex(), 4 );                 // u(4)
    bitstream.write( vpcc.getAuxiliaryVideoFlag(), 1 );       // u(1)
  } else if ( v3cUnitType == V3C_GVD ) {
    auto& vpcc = syntax.getV3CUnitHeaderGVD();
    bitstream.write( vpcc.getMapIndex(), 4 );            // u(4)
    bitstream.write( vpcc.getAuxiliaryVideoFlag(), 1 );  // u(1)
    bitstream.write( 0, 12 );                            // u(12)
  } else if ( v3cUnitType == V3C_OVD || v3cUnitType == V3C_AD ) {
    bitstream.write( 0, 17 );  // u(17)
  } else {
    bitstream.write( 0, 27 );  // u(27)
  }
}

// 7.3.2.3 V3C unit payload syntax
void PCCBitstreamWriter::v3cUnitPayload( PCCHighLevelSyntax& syntax,
                                         PCCBitstream&       bitstream,
                                         V3CUnitType         v3cUnitType ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sps = syntax.getVps();
  TRACE_BITSTREAM( "v3cUnitType = %d \n", (int32_t)v3cUnitType );
  if ( v3cUnitType == V3C_VPS ) {
    v3cParameterSet( sps, syntax, bitstream );
  } else if ( v3cUnitType == V3C_AD ) {
    atlasSubStream( syntax, bitstream );
  } else if ( v3cUnitType == V3C_OVD || v3cUnitType == V3C_GVD || v3cUnitType == V3C_AVD ) {
    videoSubStream( syntax, bitstream, v3cUnitType );
  }
}

// 7.3.3 Byte alignment syntax
void PCCBitstreamWriter::byteAlignment( PCCBitstream& bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( 1, 1 );  // f(1): equal to 1
  while ( !bitstream.byteAligned() ) {
    bitstream.write( 0, 1 );  // f(1): equal to 0
  }
}

// 7.3.4.1 General V3C parameter set syntax
void PCCBitstreamWriter::v3cParameterSet( V3CParameterSet& vps, PCCHighLevelSyntax& syntax, PCCBitstream& bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  profileTierLevel( vps.getProfileTierLevel(), bitstream );
  bitstream.write( vps.getV3CParameterSetId(), 4 );  // u(4)
  bitstream.write( 0, 8 );                           // u(8)
  bitstream.write( vps.getAtlasCountMinus1(), 6 );   // u(6)
  for ( uint32_t j = 0; j < vps.getAtlasCountMinus1() + 1; j++ ) {
    TRACE_BITSTREAM( "Atlas = %zu \n", j );
    bitstream.write( vps.getAtlasId( j ), 16 );        // u(16)
    bitstream.write( vps.getFrameWidth( j ), 16 );     // u(16)
    bitstream.write( vps.getFrameHeight( j ), 16 );    // u(16)
    bitstream.write( vps.getMapCountMinus1( j ), 4 );  // u(4)
    TRACE_BITSTREAM( " MapCountMinus1 = %zu \n", vps.getMapCountMinus1( j ) );
    if ( vps.getMapCountMinus1( j ) > 0 ) {
      bitstream.write( vps.getMultipleMapStreamsPresentFlag( j ), 1 );  // u(1)
      TRACE_BITSTREAM( "MultipleMapStreamsPresentFlag = %zu \n", vps.getMultipleMapStreamsPresentFlag( j ) );
    }
    for ( size_t i = 1; i <= vps.getMapCountMinus1( j ); i++ ) {
      if ( vps.getMultipleMapStreamsPresentFlag( j ) ) {
        bitstream.write( vps.getMapAbsoluteCodingEnableFlag( j, i ), 1 );  // u(1)
      }
      TRACE_BITSTREAM( " AbsoluteCoding Map%zu = %zu \n", i, vps.getMapAbsoluteCodingEnableFlag( j, i ) );
      if ( static_cast<int>( vps.getMapAbsoluteCodingEnableFlag( j, i ) ) == 0 ) {
        bitstream.writeUvlc( vps.getMapPredictorIndexDiff( j, i ) );
        TRACE_BITSTREAM( " PredictorIndex L%zu = %zu \n", i, vps.getMapPredictorIndexDiff( j, i ) );
      }
    }
    bitstream.write( vps.getAuxiliaryVideoPresentFlag( j ), 1 );  // u(1)
    bitstream.write( vps.getOccupancyVideoPresentFlag( j ), 1 );  // u(1)
    bitstream.write( vps.getGeometryVideoPresentFlag( j ), 1 );   // u(1)
    bitstream.write( vps.getAttributeVideoPresentFlag( j ), 1 );  // u(1)
#ifdef BITSTREAM_TRACE
    for ( size_t i = 0; i < vps.getMapCountMinus1( j ) + 1; i++ ) {
      TRACE_BITSTREAM( " AbsoluteCoding L%zu = %zu \n", i, vps.getMapAbsoluteCodingEnableFlag( j, i ) );
    }
#endif
    if ( vps.getOccupancyVideoPresentFlag( j ) ) {
      occupancyInformation( vps.getOccupancyInformation( j ), bitstream );
    }
    if ( vps.getGeometryVideoPresentFlag( j ) ) {
      geometryInformation( vps.getGeometryInformation( j ), vps, bitstream );
    }
    if ( vps.getAttributeVideoPresentFlag( j ) ) {
      attributeInformation( vps.getAttributeInformation( j ), vps, bitstream );
    }
  }
  bitstream.write( vps.getExtensionPresentFlag(), 1 );  // u(1)
  if ( vps.getExtensionPresentFlag() ) {
    bitstream.write( vps.getVpccExtensionFlag(), 1 );  // u(1)
    bitstream.write( vps.getMivExtensionFlag(), 1 );   // u(1)
    bitstream.write( vps.getExtension6Bits(), 6 );     // u(6)
  }
  if ( vps.getVpccExtensionFlag() ) { vpsVpccExtension( bitstream, vps.getVpsVpccExtension() ); }
  if ( vps.getMivExtensionFlag() ) { vpsMivExtension( bitstream ); }
  if ( vps.getExtension6Bits() ) {
    bitstream.writeUvlc( vps.getExtensionLengthMinus1() );  // ue(v)
    for ( size_t i = 0; i < vps.getExtensionLengthMinus1() + 1; i++ ) {
      bitstream.write( vps.getExtensionDataByte( i ), 8 );  // u(8)
    }
  }
  byteAlignment( bitstream );
}

// 7.3.4.2 Profile, tier, and level syntax
void PCCBitstreamWriter::profileTierLevel( ProfileTierLevel& ptl, PCCBitstream& bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( ptl.getTierFlag(), 1 );                  // u(1)
  bitstream.write( ptl.getProfileCodecGroupIdc(), 7 );      // u(7)
  bitstream.write( ptl.getProfileToolsetIdc(), 8 );         // u(8)
  bitstream.write( ptl.getProfileReconctructionIdc(), 8 );  // u(8)
  bitstream.write( 0, 32 );                                 // u(32)
  bitstream.write( ptl.getLevelIdc(), 8 );                  // u(8)
  bitstream.write( ptl.getNumSubProfiles(), 6 );            // u(6)
  bitstream.write( ptl.getExtendedSubProfileFlag(), 1 );    // u(1)
  for ( size_t i = 0; i < ptl.getNumSubProfiles(); i++ ) {
    size_t v = ptl.getExtendedSubProfileFlag() == 0 ? 32 : 64;
    bitstream.write( ptl.getSubProfileIdc( i ), v );  // u(v)
  }
  bitstream.write( ptl.getToolConstraintsPresentFlag(), 1 );  // u(1)
  if ( ptl.getToolConstraintsPresentFlag() ) {
    profileToolsetConstraintsInformation( ptl.getProfileToolsetConstraintsInformation(), bitstream );
  }
}

// 7.3.4.3 Occupancy parameter set syntax
void PCCBitstreamWriter::occupancyInformation( OccupancyInformation& oi, PCCBitstream& bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( oi.getOccupancyCodecId(), 8 );                       // u(8)
  bitstream.write( oi.getLossyOccupancyMapCompressionThreshold(), 8 );  // u(8)
  bitstream.write( oi.getOccupancyNominal2DBitdepthMinus1(), 5 );       // u(5)
  bitstream.write( oi.getOccupancyMSBAlignFlag(), 1 );                  // u(1)
  TRACE_BITSTREAM( "  OccupancyLossyThreshold = %d  \n", oi.getLossyOccupancyMapCompressionThreshold() );
}

// 7.3.4.4 Geometry parameter set syntax
void PCCBitstreamWriter::geometryInformation( GeometryInformation& gi, V3CParameterSet& vps, PCCBitstream& bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  size_t atlasIndex = 0;
  bitstream.write( gi.getGeometryCodecId(), 8 );                      // u(8)
  bitstream.write( gi.getGeometryNominal2dBitdepthMinus1(), 5 );      // u(5)
  bitstream.write( gi.getGeometryMSBAlignFlag(), 1 );                 // u(1)
  bitstream.write( gi.getGeometry3dCoordinatesBitdepthMinus1(), 5 );  // u(5)
  if ( vps.getAuxiliaryVideoPresentFlag( atlasIndex ) ) {
    bitstream.write( gi.getAuxiliaryGeometryCodecId(), 8 );  // u(8)
  }
}

// 7.3.4.5 Attribute information
void PCCBitstreamWriter::attributeInformation( AttributeInformation& ai,
                                               V3CParameterSet&      sps,
                                               PCCBitstream&         bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  size_t atlasIndex = 0;
  bitstream.write( ai.getAttributeCount(), 7 );  // u(7)
  TRACE_BITSTREAM( "AttributeCount = %u  \n", ai.getAttributeCount() );
  for ( uint32_t i = 0; i < ai.getAttributeCount(); i++ ) {
    bitstream.write( ai.getAttributeTypeId( i ), 4 );   // u(4)
    bitstream.write( ai.getAttributeCodecId( i ), 8 );  // u(8)
    if ( sps.getAuxiliaryVideoPresentFlag( atlasIndex ) ) {
      bitstream.write( ai.getAuxiliaryAttributeCodecId( i ), 8 );  // u(8)
    }
    if ( sps.getMapCountMinus1( atlasIndex ) > 0 ) {
      bitstream.write( ai.getAttributeMapAbsoluteCodingPersistenceFlag( i ),
                       1 );  // u(1)
    }
    bitstream.write( ai.getAttributeDimensionMinus1( i ), 6 );  // u(6)
    if ( ai.getAttributeDimensionMinus1( i ) > 0 ) {
      bitstream.write( ai.getAttributeDimensionPartitionsMinus1( i ), 6 );  // u(6)
      int32_t remainingDimensions = ai.getAttributeDimensionMinus1( i );
      int32_t k                   = ai.getAttributeDimensionPartitionsMinus1( i );
      for ( int32_t j = 0; j < k; j++ ) {
        if ( k - j != remainingDimensions ) {
          bitstream.writeUvlc( static_cast<uint32_t>( ai.getAttributePartitionChannelsMinus1( i, j ) ) );  // ue(v)
        }
        remainingDimensions -= ai.getAttributePartitionChannelsMinus1( i, j ) + 1;
      }
    }
    bitstream.write( ai.getAttributeNominal2dBitdepthMinus1( i ), 5 );  // u(5)
    bitstream.write( ai.getAttributeMSBAlignFlag( i ), 1 );             // u(1)
  }
}

// 7.3.4.6	Profile toolset constraints information syntax
void PCCBitstreamWriter::profileToolsetConstraintsInformation( ProfileToolsetConstraintsInformation& ptci,
                                                               PCCBitstream&                         bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( ptci.getOneFrameOnlyFlag(), 1 );                           // u(1)
  bitstream.write( ptci.getEnhancedOccupancyMapForDepthContraintFlag(), 1 );  // u(1)
  bitstream.write( ptci.getMaxMapCountMinus1(), 4 );                          // u(4)
  bitstream.write( ptci.getMaxAtlasCountMinus1(), 4 );                        // u(4)
  bitstream.write( ptci.getMultipleMapStreamsConstraintFlag(), 1 );           // u(1)
  bitstream.write( ptci.getPointLocalReconstructionConstraintFlag(), 1 );     // u(1)
  bitstream.write( ptci.getAttributeMaxDimensionMinus1(), 6 );                // u(6)
  bitstream.write( ptci.getAttributeMaxDimensionPartitionsMinus1(), 6 );      // u(6)
  bitstream.write( ptci.getNoEightOrientationsConstraintFlag(), 1 );          // u(1)
  bitstream.write( ptci.getNo45degreeProjectionPatchConstraintFlag(), 1 );    // u(1)
  bitstream.write( 0, 6 );                                                    // u(6)
  bitstream.write( ptci.getNumReservedConstraintByte(), 8 );                  // u(8)
  for ( size_t i = 0; i < ptci.getNumReservedConstraintByte(); i++ ) {
    bitstream.write( ptci.getReservedConstraintByte( i ), 8 );  // u(8)
  }
}

// 7.3.5 NAL unit syntax
// 7.3.5.1 General NAL unit syntax
void PCCBitstreamWriter::nalUnit( PCCBitstream& bitstream, NalUnit& nalUnit ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  nalUnitHeader( bitstream, nalUnit );
  for ( size_t i = 2; i < nalUnit.getSize(); i++ ) {
    bitstream.write( nalUnit.getData( i ), 8 );  // b(8)
  }
}

// 7.3.5.2 NAL unit header syntax
void PCCBitstreamWriter::nalUnitHeader( PCCBitstream& bitstream, NalUnit& nalUnit ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( 0, 1 );                              // f(1)
  bitstream.write( nalUnit.getType(), 6 );              // u(6)
  bitstream.write( nalUnit.getLayerId(), 6 );           // u(6)
  bitstream.write( nalUnit.getTemporalyIdPlus1(), 3 );  // u(3)
  TRACE_BITSTREAM( " NalUnitSize      = %zu \n", nalUnit.getSize() );
  TRACE_BITSTREAM( " NalUnitType      = %hhu \n", nalUnit.getType() );
  TRACE_BITSTREAM( " LayerId          = %hhu \n", nalUnit.getLayerId() );
  TRACE_BITSTREAM( " TemporalyIdPlus1 = %hhu \n", nalUnit.getTemporalyIdPlus1() );
}

// 7.3.6.1 Atlas sequence parameter set Rbsp
void PCCBitstreamWriter::atlasSequenceParameterSetRbsp( AtlasSequenceParameterSetRbsp& asps,
                                                        PCCHighLevelSyntax&            syntax,
                                                        PCCBitstream&                  bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.writeUvlc( static_cast<uint32_t>( asps.getAtlasSequenceParameterSetId() ) );         // ue(v)
  bitstream.write( asps.getFrameWidth(), 16 );                                                   // u(16)
  bitstream.write( asps.getFrameHeight(), 16 );                                                  // u(16)
  bitstream.writeUvlc( static_cast<uint32_t>( asps.getLog2MaxAtlasFrameOrderCntLsbMinus4() ) );  // ue(v)
  bitstream.writeUvlc( static_cast<uint32_t>( asps.getMaxDecAtlasFrameBufferingMinus1() ) );     // ue(v)
  bitstream.write( asps.getLongTermRefAtlasFramesFlag(), 1 );                                    // u(1)
  bitstream.writeUvlc( static_cast<uint32_t>( asps.getNumRefAtlasFrameListsInAsps() ) );         // ue(v)
  for ( size_t i = 0; i < asps.getNumRefAtlasFrameListsInAsps(); i++ ) {
    refListStruct( asps.getRefListStruct( i ), asps, bitstream );
  }
  bitstream.write( asps.getUseEightOrientationsFlag(), 1 );       // u(1)
  bitstream.write( asps.getExtendedProjectionEnabledFlag(), 1 );  // u(1)
  if ( asps.getExtendedProjectionEnabledFlag() ) {
    bitstream.writeUvlc( static_cast<uint32_t>( asps.getMaxNumberProjectionsMinus1() ) );  // ue(v)
  }
  bitstream.write( asps.getNormalAxisLimitsQuantizationEnabledFlag(), 1 );  // u(1)
  bitstream.write( asps.getNormalAxisMaxDeltaValueEnabledFlag(), 1 );       // u(1)
  bitstream.write( asps.getPatchPrecedenceOrderFlag(), 1 );                 // u(1)
  bitstream.write( asps.getLog2PatchPackingBlockSize(), 3 );                // u(3)
  TRACE_BITSTREAM( "Log2PatchPackingBlockSize = %u \n", asps.getLog2PatchPackingBlockSize() );
  bitstream.write( asps.getPatchSizeQuantizerPresentFlag(), 1 );  // u(1)
  bitstream.write( asps.getMapCountMinus1(), 4 );                 // u(4)
  bitstream.write( asps.getPixelDeinterleavingFlag(), 1 );        // u(1)
  if ( asps.getPixelDeinterleavingFlag() ) {
    for ( size_t i = 0; i < asps.getMapCountMinus1() + 1; i++ ) {
      bitstream.write( asps.getxeDeinterleavingMapFlag( i ), 1 );  // u(1)
    }
  }
  bitstream.write( asps.getEomPatchEnabledFlag(), 1 );  // u(1)
  if ( asps.getEomPatchEnabledFlag() && asps.getMapCountMinus1() == 0 ) {
    bitstream.write( asps.getEomFixBitCountMinus1(), 4 );  // u(4)
  }
  bitstream.write( asps.getRawPatchEnabledFlag(), 1 );  // u(1)
  if ( asps.getRawPatchEnabledFlag() || asps.getEomPatchEnabledFlag() ) {
    bitstream.write( asps.getAuxiliaryVideoEnabledFlag(), 1 );  // u(1)
  }
  bitstream.write( asps.getPointLocalReconstructionEnabledFlag(), 1 );  // u(1)
  if ( asps.getPointLocalReconstructionEnabledFlag() ) {
    pointLocalReconstructionInformation( asps, syntax, bitstream );
  }
  bitstream.write( asps.getVuiParametersPresentFlag(), 1 );  // u(1)
  if ( asps.getVuiParametersPresentFlag() ) { vuiParameters( bitstream, asps.getVuiParameters() ); }

  bitstream.write( asps.getExtensionFlag(), 1 );  // u(1)
  if ( asps.getExtensionFlag() ) {
    bitstream.write( asps.getVpccExtensionFlag(), 1 );  // u(1)
    bitstream.write( asps.getMivExtensionFlag(), 1 );   // u(1)
    bitstream.write( asps.getExtension6Bits(), 6 );     // u(6)
  }
  if ( asps.getVpccExtensionFlag() ) { aspsVpccExtension( bitstream, asps, asps.getAspsVpccExtension() ); }
  if ( asps.getMivExtensionFlag() ) { aspsMivExtension( bitstream ); }
  if ( asps.getExtension6Bits() ) {
    while ( moreRbspData( bitstream ) ) {
      bitstream.write( 0, 1 );  // u(1)
    }
  }
  rbspTrailingBits( bitstream );
}

// 7.3.4.6 Point local reconstruction information syntax
void PCCBitstreamWriter::pointLocalReconstructionInformation( AtlasSequenceParameterSetRbsp& asps,
                                                              PCCHighLevelSyntax&            syntax,
                                                              PCCBitstream&                  bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  TRACE_BITSTREAM( "  MapCountMinus1() = %u \n", asps.getMapCountMinus1() );
  for ( size_t j = 0; j < asps.getMapCountMinus1() + 1; j++ ) {
    TRACE_BITSTREAM( "  PLR info map %zu \n", j );
    auto& plri = asps.getPointLocalReconstructionInformation( j );
    bitstream.write( plri.getMapEnabledFlag(), 1 );  // u(1)
    if ( plri.getMapEnabledFlag() ) {
      bitstream.write( plri.getNumberOfModesMinus1(), 4 );  // u(4)
      TRACE_BITSTREAM( "  NumberOfModesMinus1 = %u \n", plri.getNumberOfModesMinus1() );
      for ( size_t i = 0; i < plri.getNumberOfModesMinus1(); i++ ) {
        bitstream.write( plri.getInterpolateFlag( i ), 1 );  // u(1)
        bitstream.write( plri.getFillingFlag( i ), 1 );      // u(1)
        bitstream.write( plri.getMinimumDepth( i ), 2 );     // u(2)
        bitstream.write( plri.getNeighbourMinus1( i ), 2 );  // u(2)
        TRACE_BITSTREAM( "  Mode[%zu] = I = %d F = %d D = %u N = %u \n", i, plri.getInterpolateFlag( i ),
                         plri.getFillingFlag( i ), plri.getMinimumDepth( i ), plri.getNeighbourMinus1( i ) );
      }
      bitstream.write( plri.getBlockThresholdPerPatchMinus1(), 6 );  // u(6)
      TRACE_BITSTREAM( "  BlockThresholdPerPatchMinus1 = %u \n", plri.getBlockThresholdPerPatchMinus1() );
    }
  }
}

// 7.2 Specification of syntax functions and descriptors
bool PCCBitstreamWriter::byteAligned( PCCBitstream& bitstream ) { return bitstream.byteAligned(); }
bool PCCBitstreamWriter::moreDataInPayload( PCCBitstream& bitstream ) { return false; }
bool PCCBitstreamWriter::moreRbspData( PCCBitstream& bitstream ) { return false; }
bool PCCBitstreamWriter::moreRbspTrailingData( PCCBitstream& bitstream ) { return false; }
bool PCCBitstreamWriter::moreDataInV3CUnit( PCCBitstream& bitstream ) { return false; }
void PCCBitstreamWriter::rbspTrailingBits( PCCBitstream& bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( 1, 1 );  // f(1): equal to 1
  while ( !bitstream.byteAligned() ) {
    bitstream.write( 0, 1 );  // f(1): equal to 0
  }
}
bool PCCBitstreamWriter::payloadExtensionPresent( PCCBitstream& bitstream ) { return false; }

// 7.3.6.10	Frame order count RBSP syntax
void PCCBitstreamWriter::frameOrderCountRbsp( PCCBitstream&                    bitstream,
                                              V3CUnitHeader&                   vuh,
                                              AtlasSequenceParameterSetRbsp&   asps,
                                              AtlasAdaptationParameterSetRbsp& aaps,
                                              uint32_t                         frmOrderContLsb ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  uint8_t bitCount = 0;
  if ( vuh.getAtlasId() < 0x3E ) {
    bitCount = asps.getLog2MaxAtlasFrameOrderCntLsbMinus4() + 4;
  } else {
    bitCount = aaps.getLog2MaxAtlasFrameOrderCntLsbMinus4() + 4;
  }
  bitstream.write( frmOrderContLsb, bitCount );
}

// 7.3.6.3  Atlas frame parameter set Rbsp syntax
void PCCBitstreamWriter::atlasFrameParameterSetRbsp( AtlasFrameParameterSetRbsp& afps,
                                                     PCCHighLevelSyntax&         syntax,
                                                     PCCBitstream&               bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.writeUvlc( afps.getAtlasFrameParameterSetId() );     // ue(v)
  bitstream.writeUvlc( afps.getAtlasSequenceParameterSetId() );  // ue(v)
  auto& asps = syntax.getAtlasSequenceParameterSet( afps.getAtlasSequenceParameterSetId() );
  atlasFrameTileInformation( afps.getAtlasFrameTileInformation(), asps, bitstream );
  bitstream.write( afps.getOutputFlagPresentFlag(), 1 );
  bitstream.writeUvlc( afps.getNumRefIdxDefaultActiveMinus1() );     // ue(v)
  bitstream.writeUvlc( afps.getAdditionalLtAfocLsbLen() );           // ue(v)
  bitstream.write( afps.getLodModeEnableFlag(), 1 );                 // u(1)
  bitstream.write( afps.getRaw3dPosBitCountExplicitModeFlag(), 1 );  // u(1)
  bitstream.write( afps.getExtensionFlag(), 1 );                     // u(1)
  if ( afps.getExtensionFlag() ) {
    bitstream.write( afps.getVpccExtensionFlag(), 1 );  // u(1)
    bitstream.write( afps.getMivExtensionFlag(), 1 );   // u(1)
    bitstream.write( afps.getExtension6Bits(), 6 );     // u(6)
  }

  if ( afps.getVpccExtensionFlag() ) { afpsVpccExtension( bitstream, afps.getAfpsVpccExtension() ); }
  if ( afps.getMivExtensionFlag() ) { afpsMivExtension( bitstream ); }
  if ( afps.getExtension6Bits() ) {
    while ( moreRbspData( bitstream ) ) { bitstream.write( 0, 1 ); }
  }
  rbspTrailingBits( bitstream );
}

// 7.3.6.4  Atlas frame tile information syntax
void PCCBitstreamWriter::atlasFrameTileInformation( AtlasFrameTileInformation&     afti,
                                                    AtlasSequenceParameterSetRbsp& asps,
                                                    PCCBitstream&                  bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( afti.getSingleTileInAtlasFrameFlag(), 1 );  // u(1)
  if ( !afti.getSingleTileInAtlasFrameFlag() ) {
    bitstream.write( afti.getUniformPartitionSpacingFlag(), 1 );  // u(1)
    if ( afti.getUniformPartitionSpacingFlag() ) {
      bitstream.writeUvlc( afti.getPartitionColsWidthMinus1( 0 ) );   //  ue(v)
      bitstream.writeUvlc( afti.getPartitionRowsHeightMinus1( 0 ) );  //  ue(v)
    } else {
      bitstream.writeUvlc( afti.getNumPartitionColumnsMinus1() );  //  ue(v)
      bitstream.writeUvlc( afti.getNumPartitionRowsMinus1() );     //  ue(v)
      for ( size_t i = 0; i < afti.getNumPartitionColumnsMinus1(); i++ ) {
        bitstream.writeUvlc( afti.getPartitionColsWidthMinus1( i ) );  //  ue(v)
      }
      for ( size_t i = 0; i < afti.getNumPartitionRowsMinus1(); i++ ) {
        bitstream.writeUvlc( afti.getPartitionRowsHeightMinus1( i ) );  //  ue(v)
      }
    }
    bitstream.write( afti.getSinglePartitionPerTileFlag(), 1 );  //  u(1)
    if ( afti.getSinglePartitionPerTileFlag() == 0u ) {
      uint32_t NumPartitionsInAtlasFrame =
          ( afti.getNumPartitionColumnsMinus1() + 1 ) * ( afti.getNumPartitionRowsMinus1() + 1 );
      bitstream.writeUvlc( afti.getNumTilesInAtlasFrameMinus1() );  // ue(v)
      for ( size_t i = 0; i <= afti.getNumTilesInAtlasFrameMinus1(); i++ ) {
        uint8_t bitCount = ceilLog2( NumPartitionsInAtlasFrame );
        bitstream.write( afti.getTopLeftPartitionIdx( i ), bitCount );         // u(v) :
        bitstream.writeUvlc( afti.getBottomRightPartitionColumnOffset( i ) );  // ue(v)
        bitstream.writeUvlc( afti.getBottomRightPartitionRowOffset( i ) );     // ue(v)
      }
    }
  }
  if ( asps.getAuxiliaryVideoEnabledFlag() ) {
    bitstream.writeUvlc( afti.getAuxiliaryVideoTileRowWidthMinus1() );  // ue(v)
    for ( size_t i = 0; i <= afti.getNumTilesInAtlasFrameMinus1() + 1; i++ ) {
      bitstream.writeUvlc( afti.getAuxiliaryVideoTileRowHeight( i ) );  // ue(v)
    }
  }
  bitstream.write( afti.getSignalledTileIdFlag(), 1 );  // u(1)
  if ( afti.getSignalledTileIdFlag() ) {
    bitstream.writeUvlc( afti.getSignalledTileIdLengthMinus1() );  // ue(v)
    for ( size_t i = 0; i <= afti.getSignalledTileIdLengthMinus1(); i++ ) {
      uint8_t bitCount = afti.getSignalledTileIdLengthMinus1() + 1;
      bitstream.write( afti.getTileId( i ), bitCount );  // u(v)
    }
  }
}

// 7.3.6.3	Atlas adaptation parameter set RBSP syntax
// 7.3.6.3.1	General atlas adaptation parameter set RBSP syntax
void PCCBitstreamWriter::atlasAdaptationParameterSetRbsp( AtlasAdaptationParameterSetRbsp& aaps,
                                                          PCCBitstream&                    bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.writeUvlc( aaps.getAtlasAdaptationParameterSetId() );  // ue(v)
  bitstream.write( aaps.getLog2MaxAfocPresentFlag(), 1 );          // u(1)
  if ( aaps.getLog2MaxAfocPresentFlag() ) {
    bitstream.writeUvlc( aaps.getLog2MaxAtlasFrameOrderCntLsbMinus4() );  // ue(v)
  }
  bitstream.write( aaps.getExtensionFlag(), 1 );  // u(1)
  if ( aaps.getExtensionFlag() ) {
    bitstream.write( aaps.getVpccExtensionFlag(), 1 );  // u(1)
    bitstream.write( aaps.getMivExtensionFlag(), 1 );   // u(1)
    bitstream.write( aaps.getExtension6Bits(), 6 );     // u(6)
  }
  if ( aaps.getVpccExtensionFlag() ) { aapsVpccExtension( bitstream, aaps.getAapsVpccExtension() ); }
  if ( aaps.getMivExtensionFlag() ) { aapsMivExtension( bitstream ); }
  if ( aaps.getExtension6Bits() ) {
    while ( moreRbspData( bitstream ) ) { bitstream.write( 0, 1 ); }  // u(1)
  }
  rbspTrailingBits( bitstream );
}

// 7.3.6.5  Supplemental enhancement information Rbsp syntax
void PCCBitstreamWriter::seiRbsp( PCCHighLevelSyntax& syntax,
                                  PCCBitstream&       bitstream,
                                  SEI&                sei,
                                  NalUnitType         nalUnitType ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  // do { seiMessage( syntax, bitstream ); } while ( moreRbspData( bitstream )
  // );
  seiMessage( bitstream, syntax, sei, nalUnitType );
}

// 7.3.6.10  Atlas tile group layer Rbsp syntax = patchTileLayerUnit
void PCCBitstreamWriter::atlasTileLayerRbsp( AtlasTileLayerRbsp& atgl,
                                             PCCHighLevelSyntax& syntax,
                                             PCCBitstream&       bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  atlasTileHeader( atgl.getHeader(), syntax, bitstream );
  if ( atgl.getHeader().getType() != SKIP_TILE ) {
    atlasTileDataUnit( atgl.getDataUnit(), atgl.getHeader(), syntax, bitstream );
  }
  rbspTrailingBits( bitstream );
}

// 7.3.6.11  Atlas tile group header syntax
void PCCBitstreamWriter::atlasTileHeader( AtlasTileHeader& ath, PCCHighLevelSyntax& syntax, PCCBitstream& bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  size_t                         afpsId = ath.getAtlasFrameParameterSetId();
  AtlasFrameParameterSetRbsp&    afps   = syntax.getAtlasFrameParameterSet( afpsId );
  size_t                         aspsId = afps.getAtlasSequenceParameterSetId();
  AtlasSequenceParameterSetRbsp& asps   = syntax.getAtlasSequenceParameterSet( aspsId );
  AtlasFrameTileInformation&     afti   = afps.getAtlasFrameTileInformation();
  bitstream.writeUvlc( ath.getAtlasFrameParameterSetId() );       // ue(v)
  bitstream.writeUvlc( ath.getAtlasAdaptationParameterSetId() );  // ue(v)
  if ( afti.getSignalledTileIdFlag() )
    bitstream.write( uint32_t( ath.getId() ), afti.getSignalledTileIdLengthMinus1() + 1 );  // u(v)
  else {
    if ( afti.getNumTilesInAtlasFrameMinus1() != 0 )
      bitstream.write( uint32_t( ath.getId() ), ceilLog2( afti.getNumTilesInAtlasFrameMinus1() + 1 ) );  // u(v)
  }
  bitstream.writeUvlc( ath.getType() );  // ue(v)
  if ( afps.getOutputFlagPresentFlag() ) { bitstream.write( ath.getAtlasOutputFlag(), 1 ); }
  bitstream.write( ath.getAtlasFrmOrderCntLsb(), asps.getLog2MaxAtlasFrameOrderCntLsbMinus4() + 4 );  // u(v)
  TRACE_BITSTREAM( " AtlasFrameParameterSetId: %zu\n", ath.getAtlasFrameParameterSetId() );
  TRACE_BITSTREAM( " AtlasSequenceParameterSetId: %zu\n", afps.getAtlasSequenceParameterSetId() );
  TRACE_BITSTREAM( " Address       %zu\n", (size_t)ath.getId() );
  TRACE_BITSTREAM( " Type          %zu\n", (size_t)ath.getType() );
  TRACE_BITSTREAM( " FrameOrderCnt %zu\n", (size_t)ath.getAtlasFrmOrderCntLsb() );
  if ( asps.getNumRefAtlasFrameListsInAsps() > 0 ) {
    bitstream.write( ath.getRefAtlasFrameListSpsFlag(), 1 );  // u(1)
  }
  if ( static_cast<int>( ath.getRefAtlasFrameListSpsFlag() ) == 0 ) {
    refListStruct( ath.getRefListStruct(), asps, bitstream );
  } else if ( asps.getNumRefAtlasFrameListsInAsps() > 1 ) {
    size_t bitCount = ceilLog2( asps.getNumRefAtlasFrameListsInAsps() );
    bitstream.write( ath.getRefAtlasFrameListIdx(), bitCount );  // u(v)
  }
  uint8_t rlsIdx  = ath.getRefAtlasFrameListIdx();
  auto&   refList = ath.getRefAtlasFrameListSpsFlag() ? asps.getRefListStruct( rlsIdx ) : ath.getRefListStruct();
  size_t  numLtrAtlasFrmEntries = 0;
  for ( size_t i = 0; i < refList.getNumRefEntries(); i++ ) {
    if ( !refList.getStRefAtalsFrameFlag( i ) ) { numLtrAtlasFrmEntries++; }
  }
  TRACE_BITSTREAM( " rlsIdx %u numLtrAtlasFrmEntries %zu \n", rlsIdx, (size_t)numLtrAtlasFrmEntries );
  for ( size_t j = 0; j < numLtrAtlasFrmEntries; j++ ) {
    bitstream.write( ath.getAdditionalAfocLsbPresentFlag( j ), 1 );  // u(1)
    if ( ath.getAdditionalAfocLsbPresentFlag( j ) ) {
      uint8_t bitCount = afps.getAdditionalLtAfocLsbLen();
      bitstream.write( ath.getAdditionalAfocLsbVal( j ), bitCount );  // u(v)
    }
  }
  if ( ath.getType() != SKIP_TILE ) {
    if ( asps.getNormalAxisLimitsQuantizationEnabledFlag() ) {
      bitstream.write( ath.getPosMinZQuantizer(), 5 );  // u(5)
      TRACE_BITSTREAM( " AtghPosMinZQuantizer = %zu \n", ath.getPosMinZQuantizer() );
      if ( asps.getNormalAxisMaxDeltaValueEnabledFlag() ) {
        bitstream.write( ath.getPosDeltaMaxZQuantizer(), 5 );  // u(5)
      }
    }
    if ( asps.getPatchSizeQuantizerPresentFlag() ) {
      bitstream.write( ath.getPatchSizeXinfoQuantizer(), 3 );  // u(3)
      bitstream.write( ath.getPatchSizeYinfoQuantizer(), 3 );  // u(3)
    }
    auto& gi = syntax.getVps().getGeometryInformation( 0 );
    TRACE_BITSTREAM( "Raw3dPosBitCountExplicitModeFlag    = %d \n", afps.getRaw3dPosBitCountExplicitModeFlag() );
    if ( afps.getRaw3dPosBitCountExplicitModeFlag() ) {
      size_t bitCount = ceilLog2( gi.getGeometry3dCoordinatesBitdepthMinus1() + 1 );
      TRACE_BITSTREAM( "Geometry3dCoordinatesBitdepthMinus1 = %zu \n", gi.getGeometry3dCoordinatesBitdepthMinus1() );
      bitstream.write( ath.getRaw3dPosAxisBitCountMinus1(), bitCount );  // u(v)
    } else {
      TRACE_BITSTREAM( "Geometry3dCoordinatesBitdepthMinus1 = %zu \n", gi.getGeometry3dCoordinatesBitdepthMinus1() );
      TRACE_BITSTREAM( "GeometryNominal2dBitdepthMinus1     = %zu \n", gi.getGeometryNominal2dBitdepthMinus1() );
    }
    if ( ath.getType() == P_TILE && refList.getNumRefEntries() > 1 ) {
      bitstream.write( ath.getNumRefIdxActiveOverrideFlag(), 1 );  // u(1)
      if ( ath.getNumRefIdxActiveOverrideFlag() ) {
        bitstream.writeUvlc( ath.getNumRefIdxActiveMinus1() );  // ue(v)
      }
    }
    TRACE_BITSTREAM( "==> Raw3dPosAxisBitCountMinus1  = %zu \n", ath.getRaw3dPosAxisBitCountMinus1() );
    TRACE_BITSTREAM( "==> NumRefIdxActiveOverrideFlag = %zu \n", ath.getNumRefIdxActiveOverrideFlag() );
    TRACE_BITSTREAM( "==> NumRefIdxActiveMinus1       = %zu \n", ath.getNumRefIdxActiveMinus1() );
  }
  byteAlignment( bitstream );
}

// 7.3.6.12  Reference list structure syntax
void PCCBitstreamWriter::refListStruct( RefListStruct&                 rls,
                                        AtlasSequenceParameterSetRbsp& asps,
                                        PCCBitstream&                  bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.writeUvlc( rls.getNumRefEntries() );  // ue(v)
  TRACE_BITSTREAM( "NumRefEntries = %zu  \n", rls.getNumRefEntries() );
  for ( size_t i = 0; i < rls.getNumRefEntries(); i++ ) {
    if ( asps.getLongTermRefAtlasFramesFlag() ) { bitstream.write( rls.getStRefAtalsFrameFlag( i ), 1 ); }  // u(1)

    if ( rls.getStRefAtalsFrameFlag( i ) ) {
      bitstream.writeUvlc( rls.getAbsDeltaAfocSt( i ) );  // ue(v)
      if ( rls.getAbsDeltaAfocSt( i ) > 0 ) {
        bitstream.write( rls.getStrafEntrySignFlag( i ), 1 );  // u(1)
      }
    } else {
      uint8_t bitCount = asps.getLog2MaxAtlasFrameOrderCntLsbMinus4() + 4;
      bitstream.write( rls.getAfocLsbLt( i ), bitCount );  // u(v)
      // note: psps_log2_max_patch_frame_order_cnt_lsb_minus4 + 4 bits
    }
    TRACE_BITSTREAM( "stRefAtalsFrameFlag = %zu  \n", rls.getStRefAtalsFrameFlag( i ) );
    TRACE_BITSTREAM( "absDeltaAfocSt      = %zu  \n", rls.getAbsDeltaAfocSt( i ) );
    TRACE_BITSTREAM( "strpfEntrySignFlag  = %zu  \n", rls.getStrafEntrySignFlag( i ) );
  }
  TRACE_BITSTREAM( "%s done \n", __func__ );
}

// 7.3.7.1  General atlas tile group data unit syntax =patchTileDataUnit
void PCCBitstreamWriter::atlasTileDataUnit( AtlasTileDataUnit&  atdu,
                                            AtlasTileHeader&    ath,
                                            PCCHighLevelSyntax& syntax,
                                            PCCBitstream&       bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  TRACE_BITSTREAM( "Type = %zu \n", ath.getType() );
  for ( size_t puCount = 0; puCount < atdu.getPatchCount(); puCount++ ) {
    TRACE_BITSTREAM( "patch %zu : \n", puCount );
    bitstream.writeUvlc( uint32_t( atdu.getPatchMode( puCount ) ) );
    auto& pid = atdu.getPatchInformationData( puCount );
    TRACE_BITSTREAM( "patchMode = %zu \n", atdu.getPatchMode( puCount ) );
    pid.setFrameIndex( atdu.getFrameIndex() );
    pid.setPatchIndex( puCount );
    patchInformationData( pid, atdu.getPatchMode( puCount ), ath, syntax, bitstream );
  }
  TRACE_BITSTREAM( "atdu.getPatchCount() including END = %zu \n", atdu.getPatchCount() );
  byteAlignment( bitstream );
}

// 7.3.7.2  Patch information data syntax
void PCCBitstreamWriter::patchInformationData( PatchInformationData& pid,
                                               size_t                patchMode,
                                               AtlasTileHeader&      ath,
                                               PCCHighLevelSyntax&   syntax,
                                               PCCBitstream&         bitstream ) {
  TRACE_BITSTREAM( "%s: AtghType = %zu patchMode = %zu \n", __func__, ath.getType(), patchMode );
  if ( ath.getType() == SKIP_TILE ) {
    // skip mode: currently not supported but added it for convenience. Could
    // easily be removed
  } else if ( ath.getType() == P_TILE ) {
    if ( patchMode == P_SKIP ) {
      // skip mode: currently not supported but added it for convenience. Could
      // easily be removed
    } else if ( patchMode == P_MERGE ) {
      auto& mpdu = pid.getMergePatchDataUnit();
      mpdu.setFrameIndex( pid.getFrameIndex() );
      mpdu.setPatchIndex( pid.getPatchIndex() );
      mergePatchDataUnit( mpdu, ath, syntax, bitstream );
    } else if ( patchMode == P_INTRA ) {
      auto& pdu = pid.getPatchDataUnit();
      pdu.setFrameIndex( pid.getFrameIndex() );
      pdu.setPatchIndex( pid.getPatchIndex() );
      patchDataUnit( pdu, ath, syntax, bitstream );
    } else if ( patchMode == P_INTER ) {
      auto& ipdu = pid.getInterPatchDataUnit();
      ipdu.setFrameIndex( pid.getFrameIndex() );
      ipdu.setPatchIndex( pid.getPatchIndex() );
      interPatchDataUnit( ipdu, ath, syntax, bitstream );
    } else if ( patchMode == P_RAW ) {
      auto& rpdu = pid.getRawPatchDataUnit();
      rpdu.setFrameIndex( pid.getFrameIndex() );
      rpdu.setPatchIndex( pid.getPatchIndex() );
      rawPatchDataUnit( rpdu, ath, syntax, bitstream );
    } else if ( patchMode == P_EOM ) {
      auto& epdu = pid.getEomPatchDataUnit();
      epdu.setFrameIndex( pid.getFrameIndex() );
      epdu.setPatchIndex( pid.getPatchIndex() );
      eomPatchDataUnit( epdu, ath, syntax, bitstream );
    }
  } else if ( ath.getType() == I_TILE ) {
    if ( patchMode == I_INTRA ) {
      auto& pdu = pid.getPatchDataUnit();
      pdu.setFrameIndex( pid.getFrameIndex() );
      pdu.setPatchIndex( pid.getPatchIndex() );
      patchDataUnit( pdu, ath, syntax, bitstream );
    } else if ( patchMode == I_RAW ) {
      auto& rpdu = pid.getRawPatchDataUnit();
      rpdu.setFrameIndex( pid.getFrameIndex() );
      rpdu.setPatchIndex( pid.getPatchIndex() );
      rawPatchDataUnit( rpdu, ath, syntax, bitstream );
    } else if ( patchMode == I_EOM ) {
      auto& epdu = pid.getEomPatchDataUnit();
      epdu.setFrameIndex( pid.getFrameIndex() );
      epdu.setPatchIndex( pid.getPatchIndex() );
      eomPatchDataUnit( epdu, ath, syntax, bitstream );
    }
  }
}

// 7.3.7.3  Patch data unit syntax : AtlasTileHeader instead of
// PatchTileHeader
void PCCBitstreamWriter::patchDataUnit( PatchDataUnit&      pdu,
                                        AtlasTileHeader&    ath,
                                        PCCHighLevelSyntax& syntax,
                                        PCCBitstream&       bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  size_t                         afpsId = ath.getAtlasFrameParameterSetId();
  AtlasFrameParameterSetRbsp&    afps   = syntax.getAtlasFrameParameterSet( afpsId );
  size_t                         aspsId = afps.getAtlasSequenceParameterSetId();
  AtlasSequenceParameterSetRbsp& asps   = syntax.getAtlasSequenceParameterSet( aspsId );
  bitstream.writeUvlc( pdu.get2dPosX() );  // ue(v)
  bitstream.writeUvlc( pdu.get2dPosY() );  // ue(v)
  TRACE_BITSTREAM( " 2dPosXY: %zu,%zu\n", pdu.get2dPosX(), pdu.get2dPosX() );
  bitstream.writeUvlc( pdu.get2dSizeXMinus1() );
  bitstream.writeUvlc( pdu.get2dSizeYMinus1() );
  TRACE_BITSTREAM( " 2dSizeXY: %d,%d\n", int32_t( pdu.get2dSizeXMinus1() + 1 ), int32_t( pdu.get2dSizeYMinus1() + 1 ) );
  uint8_t bitCount3DPos = syntax.getVps().getGeometryInformation( 0 ).getGeometry3dCoordinatesBitdepthMinus1() + 1;
  bitstream.write( pdu.get3dPosX(), bitCount3DPos );  // u(v)
  bitstream.write( pdu.get3dPosY(), bitCount3DPos );  // u(v)
  TRACE_BITSTREAM( " 3dPosXY: %zu,%zu\n", pdu.get3dPosX(), pdu.get3dPosY() );
  const uint8_t bitCountForMinDepth =
      syntax.getVps().getGeometryInformation( 0 ).getGeometry3dCoordinatesBitdepthMinus1() - ath.getPosMinZQuantizer() +
      2;
  bitstream.write( pdu.get3dPosMinZ(), bitCountForMinDepth );  // u(v)
  TRACE_BITSTREAM( " Pdu3dPosMinZ: %zu ( bitCountForMinDepth = %u = %u - %u + %u ) \n", pdu.get3dPosMinZ(),
                   bitCountForMinDepth,
                   syntax.getVps().getGeometryInformation( 0 ).getGeometry3dCoordinatesBitdepthMinus1(),
                   ath.getPosMinZQuantizer(), 2 );

  if ( asps.getNormalAxisMaxDeltaValueEnabledFlag() ) {
#if EXPAND_RANGE_CONDITIONAL
    uint8_t bitCountForMaxDepth = syntax.getVps().getGeometryInformation( 0 ).getGeometry3dCoordinatesBitdepthMinus1() -
                                  ath.getPosDeltaMaxZQuantizer() + 1 + asps.getExtendedProjectionEnabledFlag();
#else
#ifdef EXPAND_RANGE_ENCODER
    uint8_t bitCountForMaxDepth = syntax.getVps().getGeometryInformation( 0 ).getGeometry3dCoordinatesBitdepthMinus1() -
                                  ath.getPosDeltaMaxZQuantizer() + 1;
#else
    uint8_t bitCountForMaxDepth = syntax.getVps().getGeometryInformation( 0 ).getGeometry3dCoordinatesBitdepthMinus1() -
                                  ath.getPosDeltaMaxZQuantizer() + 2;
#endif
#endif
    // if ( asps.getExtendedProjectionEnabledFlag() ) { bitCountForMaxDepth++; }
    bitstream.write( pdu.get3dPosDeltaMaxZ(), bitCountForMaxDepth );
    TRACE_BITSTREAM( " Pdu3dPosDeltaMaxZ: %zu ( bitCountForMaxDepth = %u) \n", pdu.get3dPosDeltaMaxZ(),
                     bitCountForMaxDepth );
  }

  bitstream.write( pdu.getProjectionId(),
                   ceilLog2( asps.getMaxNumberProjectionsMinus1() + 1 ) );  // u(5 or 3)
  TRACE_BITSTREAM( "PduProjectionId = %zu ( MaxNumberProjectionsMinus1 = %d ) \n", pdu.getProjectionId(),
                   asps.getMaxNumberProjectionsMinus1() );
  bitstream.write( pdu.getOrientationIndex(),
                   asps.getUseEightOrientationsFlag() ? 3 : 1 );  // u(3 or 1)
  if ( afps.getLodModeEnableFlag() ) {
    bitstream.write( pdu.getLodEnableFlag(), 1 );  // u(1)
    if ( pdu.getLodEnableFlag() ) {
      bitstream.writeUvlc( pdu.getLodScaleXminus1() );  // ue(v)
      bitstream.writeUvlc( pdu.getLodScaleY() );        // ue(v)
    }
  }
  TRACE_BITSTREAM( "PointLocalReconstructionEnabledFlag = %d \n", asps.getPointLocalReconstructionEnabledFlag() );
  if ( asps.getPointLocalReconstructionEnabledFlag() ) {
    auto& plrd = pdu.getPointLocalReconstructionData();
    TRACE_BITSTREAM( "Size = %ld %ld\n", pdu.get2dSizeXMinus1() + 1, pdu.get2dSizeYMinus1() + 1 );
    pointLocalReconstructionData( plrd, syntax, asps, bitstream );
  }
#ifdef BITSTREAM_TRACE
  if ( asps.getMivExtensionFlag() ) { TRACE_BITSTREAM( "MivExtension \n" ); }
#endif
  TRACE_BITSTREAM(
      "Frame %zu, Patch(%zu) => 2Dpos = %4zu %4zu 2Dsize = %4ld %4ld 3Dpos = "
      "%ld %ld %ld DeltaMaxZ = %ld Projection = "
      "%zu "
      "Orientation = %zu lod=(%zu) %zu %zu\n ",
      pdu.getFrameIndex(), pdu.getPatchIndex(), pdu.get2dPosX(), pdu.get2dPosY(), pdu.get2dSizeXMinus1() + 1,
      pdu.get2dSizeYMinus1() + 1, pdu.get3dPosX(), pdu.get3dPosY(), pdu.get3dPosMinZ(), pdu.get3dPosDeltaMaxZ(),
      pdu.getProjectionId(), pdu.getOrientationIndex(), pdu.getLodEnableFlag(),
      pdu.getLodEnableFlag() ? pdu.getLodScaleXminus1() : (uint8_t)0,
      pdu.getLodEnableFlag() ? pdu.getLodScaleY() : (uint8_t)0 );
}

// 7.3.7.4  Skip patch data unit syntax
void PCCBitstreamWriter::skipPatchDataUnit( SkipPatchDataUnit&  spdu,
                                            AtlasTileHeader&    ath,
                                            PCCHighLevelSyntax& syntax,
                                            PCCBitstream&       bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
}

// 7.3.7.5  Merge patch data unit syntax
void PCCBitstreamWriter::mergePatchDataUnit( MergePatchDataUnit& mpdu,
                                             AtlasTileHeader&    ath,
                                             PCCHighLevelSyntax& syntax,
                                             PCCBitstream&       bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  size_t                         afpsId          = ath.getAtlasFrameParameterSetId();
  AtlasFrameParameterSetRbsp&    afps            = syntax.getAtlasFrameParameterSet( afpsId );
  size_t                         aspsId          = afps.getAtlasSequenceParameterSetId();
  AtlasSequenceParameterSetRbsp& asps            = syntax.getAtlasSequenceParameterSet( aspsId );
  bool                           overridePlrFlag = false;
  size_t                         numRefIdxActive = syntax.getNumRefIdxActive( ath );
  if ( numRefIdxActive > 1 ) { bitstream.writeUvlc( mpdu.getRefIndex() ); }  // ue(v)
  bitstream.write( mpdu.getOverride2dParamsFlag(), 1 );                      // u(1)
  if ( mpdu.getOverride2dParamsFlag() ) {
    bitstream.writeSvlc( mpdu.get2dPosX() );        // se(v)
    bitstream.writeSvlc( mpdu.get2dPosY() );        // se(v)
    bitstream.writeSvlc( mpdu.get2dDeltaSizeX() );  // se(v)
    bitstream.writeSvlc( mpdu.get2dDeltaSizeY() );  // se(v)
    if ( asps.getPointLocalReconstructionEnabledFlag() ) { overridePlrFlag = true; }
  } else {
    bitstream.write( mpdu.getOverride3dParamsFlag(), 1 );  // u(1)
    if ( mpdu.getOverride3dParamsFlag() ) {
      bitstream.writeSvlc( mpdu.get3dPosX() );     // se(v)
      bitstream.writeSvlc( mpdu.get3dPosY() );     // se(v)
      bitstream.writeSvlc( mpdu.get3dPosMinZ() );  // se(v)
      if ( asps.getNormalAxisMaxDeltaValueEnabledFlag() ) {
        bitstream.writeSvlc( mpdu.get3dPosDeltaMaxZ() );  // se(v)
      }
      if ( asps.getPointLocalReconstructionEnabledFlag() ) {
        bitstream.write( mpdu.getOverridePlrFlag(), 1 );  // u(1)
      }
    }
  }
  if ( overridePlrFlag && asps.getPointLocalReconstructionEnabledFlag() ) {
    auto& plrd = mpdu.getPointLocalReconstructionData();
    TRACE_BITSTREAM( "Prev Size = %d %d Delta Size = %ld %ld => %ld %ld \n",
                     plrd.getBlockToPatchMapWidth() - mpdu.get2dDeltaSizeX(),
                     plrd.getBlockToPatchMapHeight() - mpdu.get2dDeltaSizeY(),

                     mpdu.get2dDeltaSizeX(), mpdu.get2dDeltaSizeY(), plrd.getBlockToPatchMapWidth(),
                     plrd.getBlockToPatchMapHeight() );
    pointLocalReconstructionData( plrd, syntax, asps, bitstream );
  }
#ifdef BITSTREAM_TRACE
  if ( asps.getMivExtensionFlag() ) { TRACE_BITSTREAM( "MivExtension \n" ); }
#endif
  TRACE_BITSTREAM(
      "%zu frame %zu MergePatch => Ref Frame Idx = %d ShiftUV = %ld %ld "
      "DeltaSize = %ld %ld Axis = %ld %ld %ld\n",
      mpdu.getRefIndex(), mpdu.get2dPosX(), mpdu.get2dPosY(), mpdu.get2dDeltaSizeX(), mpdu.get2dDeltaSizeY(),
      mpdu.get3dPosX(), mpdu.get3dPosY(), mpdu.get3dPosMinZ(), mpdu.get3dPosDeltaMaxZ() );
}

// 7.3.7.6  Inter patch data unit syntax
void PCCBitstreamWriter::interPatchDataUnit( InterPatchDataUnit& ipdu,
                                             AtlasTileHeader&    ath,
                                             PCCHighLevelSyntax& syntax,
                                             PCCBitstream&       bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  size_t                         afpsId          = ath.getAtlasFrameParameterSetId();
  AtlasFrameParameterSetRbsp&    afps            = syntax.getAtlasFrameParameterSet( afpsId );
  size_t                         aspsId          = afps.getAtlasSequenceParameterSetId();
  AtlasSequenceParameterSetRbsp& asps            = syntax.getAtlasSequenceParameterSet( aspsId );
  size_t                         numRefIdxActive = syntax.getNumRefIdxActive( ath );
  if ( numRefIdxActive > 1 ) {
    bitstream.writeUvlc( int32_t( ipdu.getRefIndex() ) );  // ue(v)
  }
  bitstream.writeSvlc( int32_t( ipdu.getRefPatchIndex() ) );  // se(v)
  bitstream.writeSvlc( int32_t( ipdu.get2dPosX() ) );         // se(v)
  bitstream.writeSvlc( int32_t( ipdu.get2dPosY() ) );         // se(v)
  bitstream.writeSvlc( int32_t( ipdu.get2dDeltaSizeX() ) );   // se(v)
  bitstream.writeSvlc( int32_t( ipdu.get2dDeltaSizeY() ) );   // se(v)
  bitstream.writeSvlc( int32_t( ipdu.get3dPosX() ) );         // se(v)
  bitstream.writeSvlc( int32_t( ipdu.get3dPosY() ) );         // se(v)
  bitstream.writeSvlc( int32_t( ipdu.get3dPosMinZ() ) );      // se(v)
  if ( asps.getNormalAxisMaxDeltaValueEnabledFlag() ) {
    bitstream.writeSvlc( int32_t( ipdu.get3dPosDeltaMaxZ() ) );  // se(v)
  }
  TRACE_BITSTREAM(
      "%zu frame: numRefIdxActive = %zu reference = frame%zu patch%d 2Dpos = "
      "%ld %ld 2DdeltaSize = %ld %ld 3Dpos = %ld "
      "%ld %ld DeltaMaxZ = %ld\n",
      ipdu.getFrameIndex(), numRefIdxActive, ipdu.getRefIndex(), ipdu.getRefPatchIndex(), ipdu.get2dPosX(),
      ipdu.get2dPosY(), ipdu.get2dDeltaSizeX(), ipdu.get2dDeltaSizeY(), ipdu.get3dPosX(), ipdu.get3dPosY(),
      ipdu.get3dPosMinZ(), ipdu.get3dPosDeltaMaxZ() );

  if ( asps.getPointLocalReconstructionEnabledFlag() ) {
    auto& plrd = ipdu.getPointLocalReconstructionData();
    TRACE_BITSTREAM( "Prev Size = %d %d Delta Size = %ld %ld => %ld %ld \n",
                     plrd.getBlockToPatchMapWidth() - ipdu.get2dDeltaSizeX(),
                     plrd.getBlockToPatchMapHeight() - ipdu.get2dDeltaSizeY(), ipdu.get2dDeltaSizeX(),
                     ipdu.get2dDeltaSizeY(), plrd.getBlockToPatchMapWidth(), plrd.getBlockToPatchMapHeight() );
    pointLocalReconstructionData( plrd, syntax, asps, bitstream );
  }
#ifdef BITSTREAM_TRACE
  if ( asps.getMivExtensionFlag() ) { TRACE_BITSTREAM( "MivExtension \n" ); }
#endif
}

// 7.3.7.7 raw patch data unit syntax
void PCCBitstreamWriter::rawPatchDataUnit( RawPatchDataUnit&   rpdu,
                                           AtlasTileHeader&    ath,
                                           PCCHighLevelSyntax& syntax,
                                           PCCBitstream&       bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto&   vps        = syntax.getVps();
  size_t  atlasIndex = 0;
  int32_t bitCount   = ath.getRaw3dPosAxisBitCountMinus1() + 1;
  TRACE_BITSTREAM( " AtghRaw3dPosAxisBitCountMinus1 = %zu => bitcount = %d \n", ath.getRaw3dPosAxisBitCountMinus1(),
                   bitCount );

  if ( 1 /* TODO: evaluate: AuxTileHeight[ TileIdToIndex[ ath_id ] ] > 0 */ ) {
    bitstream.write( rpdu.getPatchInAuxiliaryVideoFlag(), 1 );  // u(1)
  }
  bitstream.writeUvlc( rpdu.get2dPosX() );           // ue(v)
  bitstream.writeUvlc( rpdu.get2dPosY() );           // ue(v)
  bitstream.writeUvlc( rpdu.get2dSizeXMinus1() );    // se(v)
  bitstream.writeUvlc( rpdu.get2dSizeYMinus1() );    // se(v)
  bitstream.write( rpdu.get3dPosX(), bitCount );     // u(v)
  bitstream.write( rpdu.get3dPosY(), bitCount );     // u(v)
  bitstream.write( rpdu.get3dPosZ(), bitCount );     // u(v)
  bitstream.writeUvlc( rpdu.getRawPointsMinus1() );  // ue(v)
  TRACE_BITSTREAM(
      "Raw Patch => UV %4zu %4zu  S=%4ld %4ld  UVD1=%4ld %4ld %4ld "
      "NumPcmPoints=%zu PatchInRawVideoFlag=%d \n",
      rpdu.get2dPosX(), rpdu.get2dPosY(), rpdu.get2dSizeXMinus1() + 1, rpdu.get2dSizeYMinus1() + 1, rpdu.get3dPosX(),
      rpdu.get3dPosY(), rpdu.get3dPosZ(), rpdu.getRawPointsMinus1() + 1, rpdu.getPatchInAuxiliaryVideoFlag() );
}

// 7.3.7.8 EOM patch data unit syntax
void PCCBitstreamWriter::eomPatchDataUnit( EOMPatchDataUnit&   epdu,
                                           AtlasTileHeader&    ath,
                                           PCCHighLevelSyntax& syntax,
                                           PCCBitstream&       bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );

  if ( 1 /* TODO: evaluate: AuxTileHeight[ TileIdToIndex[ ath_id ] ] > 0 */ ) {
    bitstream.write( epdu.getPatchInAuxiliaryVideoFlag(), 1 );  // u(1)
  }
  bitstream.writeUvlc( epdu.get2dPosX() );            // ue(v)
  bitstream.writeUvlc( epdu.get2dPosY() );            // ue(v)
  bitstream.writeUvlc( epdu.get2dSizeXMinus1() );     // ue(v)
  bitstream.writeUvlc( epdu.get2dSizeYMinus1() );     // ue(v)
  bitstream.writeUvlc( epdu.getPatchCountMinus1() );  //  ue(v)
  for ( size_t i = 0; i < epdu.getPatchCountMinus1() + 1; i++ ) {
    bitstream.writeUvlc( epdu.getAssociatedPatchesIdx( i ) );  //  ue(v)
    bitstream.writeUvlc( epdu.getPoints( i ) );                //  ue(v)
  }
#ifdef BITSTREAM_TRACE
  TRACE_BITSTREAM( "EOM Patch => UV %4zu %4zu  S=%4ld %4ld  N=%4ld\n", epdu.get2dPosX(), epdu.get2dPosY(),
                   epdu.get2dSizeXMinus1() + 1, epdu.get2dSizeYMinus1() + 1, epdu.getPatchCountMinus1() + 1 );
  for ( size_t i = 0; i < epdu.getPatchCountMinus1() + 1; i++ ) {
    TRACE_BITSTREAM( "%4ld, %4ld\n", epdu.getAssociatedPatchesIdx( i ), epdu.getPoints( i ) );
  }
#endif
}

void PCCBitstreamWriter::atlasSubStream( PCCHighLevelSyntax& syntax, PCCBitstream& bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  SampleStreamNalUnit   ssnu;
  uint32_t              maxUnitSize = 0;
  PCCBitstream          tempBitStream;
  std::vector<uint32_t> aspsSizeList;
  std::vector<uint32_t> afpsSizeList;
  std::vector<uint32_t> atglSizeList;
  std::vector<uint32_t> seiPrefixSizeList;
  std::vector<uint32_t> seiSuffixSizeList;
  aspsSizeList.resize( syntax.getAtlasSequenceParameterSetList().size() );
  afpsSizeList.resize( syntax.getAtlasFrameParameterSetList().size() );
  atglSizeList.resize( syntax.getGofSize() );
  seiPrefixSizeList.resize( syntax.getSeiPrefix().size() );
  seiSuffixSizeList.resize( syntax.getSeiSuffix().size() );
  uint32_t lastSize      = 0;
  size_t   nalHeaderSize = 2;
  for ( size_t aspsIdx = 0; aspsIdx < syntax.getAtlasSequenceParameterSetList().size(); aspsIdx++ ) {
    atlasSequenceParameterSetRbsp( syntax.getAtlasSequenceParameterSet( aspsIdx ), syntax, tempBitStream );
    aspsSizeList[aspsIdx] = tempBitStream.size() - lastSize + nalHeaderSize;
    lastSize              = tempBitStream.size();
    if ( maxUnitSize < aspsSizeList[aspsIdx] ) { maxUnitSize = aspsSizeList[aspsIdx]; }
  }
  for ( size_t afpsIdx = 0; afpsIdx < syntax.getAtlasFrameParameterSetList().size(); afpsIdx++ ) {
    atlasFrameParameterSetRbsp( syntax.getAtlasFrameParameterSet( afpsIdx ), syntax, tempBitStream );
    afpsSizeList[afpsIdx] = tempBitStream.size() - lastSize + nalHeaderSize;
    lastSize              = tempBitStream.size();
    if ( maxUnitSize < afpsSizeList[afpsIdx] ) { maxUnitSize = afpsSizeList[afpsIdx]; }
  }
  for ( size_t i = 0; i < syntax.getSeiPrefix().size(); i++ ) {
    seiRbsp( syntax, tempBitStream, syntax.getSeiPrefix( i ), NAL_PREFIX_ESEI );
    seiPrefixSizeList[i] = tempBitStream.size() - lastSize + nalHeaderSize;
    lastSize             = tempBitStream.size();
    if ( maxUnitSize < seiPrefixSizeList[i] ) { maxUnitSize = seiPrefixSizeList[i]; }
  }
  for ( size_t atglIdx = 0; atglIdx < atglSizeList.size(); atglIdx++ ) {
    int numTilesPerFrame = 1;  // (afps.getAtlasFrameTileInformation().getNumPartitionRowsMinus1() + 1)
                               // *
    // (afps.getAtlasFrameTileInformation().getNumPartitionColumnsMinus1() + 1);
    for ( size_t tileId = 0; tileId < numTilesPerFrame; tileId++ ) {  // TODO: make this more than just
                                                                      // one tile groups per frame
      atlasTileLayerRbsp( syntax.getAtlasTileLayer( atglIdx ), syntax, tempBitStream );
      atglSizeList[atglIdx] = tempBitStream.size() - lastSize + nalHeaderSize;
      lastSize              = tempBitStream.size();
      if ( maxUnitSize < atglSizeList[atglIdx] ) { maxUnitSize = atglSizeList[atglIdx]; }
    }
  }
  for ( size_t i = 0; i < syntax.getSeiSuffix().size(); i++ ) {
    seiRbsp( syntax, tempBitStream, syntax.getSeiSuffix( i ), NAL_SUFFIX_ESEI );
    seiSuffixSizeList[i] = tempBitStream.size() - lastSize + nalHeaderSize;
    lastSize             = tempBitStream.size();
    if ( maxUnitSize < seiSuffixSizeList[i] ) { maxUnitSize = seiSuffixSizeList[i]; }
  }
  // calculation of the max unit size done
  TRACE_BITSTREAM(
      "maxUnitSize                                                        = %u "
      "\n",
      maxUnitSize );
  TRACE_BITSTREAM(
      "ceilLog2( maxUnitSize + 1 )                                        = %d "
      "\n",
      ceilLog2( maxUnitSize + 1 ) );
  TRACE_BITSTREAM(
      "ceil( static_cast<double>( ceilLog2( maxUnitSize + 1 ) ) / 8.0 ) ) = %f "
      "\n",
      ceil( static_cast<double>( ceilLog2( maxUnitSize + 1 ) ) / 8.0 ) );

  uint32_t precision = static_cast<uint32_t>(
      min( max( static_cast<int>( ceil( static_cast<double>( ceilLog2( maxUnitSize + 1 ) ) / 8.0 ) ), 1 ), 8 ) - 1 );
  TRACE_BITSTREAM( "precision                                                      = %u \n", precision );
  ssnu.setSizePrecisionBytesMinus1( precision );
  sampleStreamNalHeader( bitstream, ssnu );
  for ( size_t aspsCount = 0; aspsCount < syntax.getAtlasSequenceParameterSetList().size(); aspsCount++ ) {
    NalUnit nu( NAL_ASPS, 0, 1 );
    nu.setSize( aspsSizeList[aspsCount] );
    sampleStreamNalUnit( syntax, bitstream, ssnu, nu, aspsCount );
    TRACE_BITSTREAM(
        "nalu[%d]:%s, nalSizePrecision:%d, naluSize:%zu, sizeBitstream "
        "written: %llu\n",
        (int)nu.getType(), toString( nu.getType() ).c_str(), ( ssnu.getSizePrecisionBytesMinus1() + 1 ), nu.getSize(),
        bitstream.size() );
  }
  for ( size_t afpsCount = 0; afpsCount < syntax.getAtlasFrameParameterSetList().size(); afpsCount++ ) {
    NalUnit nu( NAL_AFPS, 0, 1 );
    nu.setSize( afpsSizeList[afpsCount] );
    sampleStreamNalUnit( syntax, bitstream, ssnu, nu, afpsCount );
    TRACE_BITSTREAM(
        "nalu[%d]:%s, nalSizePrecision:%d, naluSize:%zu, sizeBitstream "
        "written: %llu\n",
        (int)nu.getType(), toString( nu.getType() ).c_str(), ( ssnu.getSizePrecisionBytesMinus1() + 1 ), nu.getSize(),
        bitstream.size() );
  }
  // NAL_PREFIX_SEI
  for ( size_t i = 0; i < syntax.getSeiPrefix().size(); i++ ) {
    NalUnit nu( NAL_PREFIX_ESEI, 0, 1 );
    nu.setSize( seiPrefixSizeList[i] );
    sampleStreamNalUnit( syntax, bitstream, ssnu, nu, i );
    TRACE_BITSTREAM(
        "nalu[%d]:%s, nalSizePrecision:%d, naluSize:%zu, sizeBitstream "
        "written: %llu\n",
        (int)nu.getType(), toString( nu.getType() ).c_str(), ( ssnu.getSizePrecisionBytesMinus1() + 1 ), nu.getSize(),
        bitstream.size() );
  }
  // NAL_TRAIL, NAL_TSA, NAL_STSA, NAL_RADL, NAL_RASL,NAL_SKIP
  for ( size_t frameIdx = 0; frameIdx < atglSizeList.size(); frameIdx++ ) {
    NalUnit nu( NAL_TSA_N, 0, 1 );
    nu.setSize( atglSizeList[frameIdx] );  //+headsize
    auto& atgl = syntax.getAtlasTileLayer( frameIdx );
    atgl.getDataUnit().setFrameIndex( frameIdx );
    sampleStreamNalUnit( syntax, bitstream, ssnu, nu, frameIdx );
    TRACE_BITSTREAM(
        "nalu[%d]:%s, nalSizePrecision:%d, naluSize:%zu, sizeBitstream "
        "written: %llu\n",
        (int)nu.getType(), toString( nu.getType() ).c_str(), ( ssnu.getSizePrecisionBytesMinus1() + 1 ), nu.getSize(),
        bitstream.size() );
  }
  // NAL_SUFFIX_SEI
  for ( size_t i = 0; i < syntax.getSeiSuffix().size(); i++ ) {
    NalUnit nu( NAL_SUFFIX_ESEI, 0, 1 );
    nu.setSize( seiSuffixSizeList[i] );
    sampleStreamNalUnit( syntax, bitstream, ssnu, nu, i );
    TRACE_BITSTREAM(
        "nalu[%d]:%s, nalSizePrecision:%d, naluSize:%zu, sizeBitstream "
        "written: %llu\n",
        (int)nu.getType(), toString( nu.getType() ).c_str(), ( ssnu.getSizePrecisionBytesMinus1() + 1 ), nu.getSize(),
        bitstream.size() );
  }
}

// 7.3.6.6 Point local reconstruction data syntax
void PCCBitstreamWriter::pointLocalReconstructionData( PointLocalReconstructionData&  plrd,
                                                       PCCHighLevelSyntax&            syntax,
                                                       AtlasSequenceParameterSetRbsp& asps,
                                                       PCCBitstream&                  bitstream ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto&        plri         = asps.getPointLocalReconstructionInformation( 0 );
  const size_t blockCount   = plrd.getBlockToPatchMapWidth() * plrd.getBlockToPatchMapHeight();
  const auto   bitCountMode = uint8_t( ceilLog2( uint32_t( plri.getNumberOfModesMinus1() ) ) );
  TRACE_BITSTREAM( "WxH= %zu x %zu => blockCount = %zu \n", plrd.getBlockToPatchMapWidth(),
                   plrd.getBlockToPatchMapHeight(), blockCount );
  TRACE_BITSTREAM( "  NumberOfModesMinus1 = %u  => bitCountMode = %u  \n", plri.getNumberOfModesMinus1(),
                   bitCountMode );
  if ( blockCount > plri.getBlockThresholdPerPatchMinus1() + 1 ) {
    bitstream.write( plrd.getLevelFlag(), 1 );  // u(1)
  }
  TRACE_BITSTREAM( "  LevelFlag = %u \n", plrd.getLevelFlag() );
  if ( plrd.getLevelFlag() ) {
    bitstream.write( plrd.getPresentFlag(), 1 );  // u(1)
    if ( plrd.getPresentFlag() ) {
      bitstream.write( plrd.getModeMinus1(), bitCountMode );  // u(v)
    }
    TRACE_BITSTREAM( "  ModePatch: Present = %d ModeMinus1 = %d \n", plrd.getPresentFlag(),
                     plrd.getPresentFlag() ? (int32_t)plrd.getModeMinus1() : -1 );
  } else {
    for ( size_t i = 0; i < blockCount; i++ ) {
      bitstream.write( plrd.getBlockPresentFlag( i ), 1 );  // u(1)
      if ( plrd.getBlockPresentFlag( i ) ) {
        bitstream.write( plrd.getBlockModeMinus1( i ), bitCountMode );  // u(v)
      }
      TRACE_BITSTREAM( "  Mode[ %4zu / %4zu ]: Present = %d ModeMinus1 = %d \n", i, blockCount,
                       plrd.getBlockPresentFlag( i ),
                       plrd.getBlockPresentFlag( i ) ? plrd.getBlockModeMinus1( i ) : -1 );
    }
  }
#ifdef BITSTREAM_TRACE
  for ( size_t v0 = 0; v0 < plrd.getBlockToPatchMapHeight(); ++v0 ) {
    for ( size_t u0 = 0; u0 < plrd.getBlockToPatchMapWidth(); ++u0 ) {
      size_t i = v0 * plrd.getBlockToPatchMapWidth() + u0;
      TRACE_BITSTREAM(
          "Patch Block[ %2lu %2lu <=> %4zu ] / [ %2lu %2lu ] Level = %d "
          "Present = %d Mode = %d \n",
          u0, v0, i, plrd.getBlockToPatchMapWidth(), plrd.getBlockToPatchMapHeight(), plrd.getLevelFlag(),
          plrd.getLevelFlag() ? plrd.getPresentFlag() : plrd.getBlockPresentFlag( i ),
          plrd.getLevelFlag() ? plrd.getPresentFlag() ? (int32_t)plrd.getModeMinus1() : -1
                              : plrd.getBlockPresentFlag( i ) ? plrd.getBlockModeMinus1( i ) : -1 );
    }
  }
#endif
}

// 7.3.8 Supplemental enhancement information message syntax
void PCCBitstreamWriter::seiMessage( PCCBitstream&       bitstream,
                                     PCCHighLevelSyntax& syntax,
                                     SEI&                sei,
                                     NalUnitType         nalUnitType ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto payloadType = static_cast<int32_t>( sei.getPayloadType() );
  for ( ; payloadType >= 0xff; payloadType -= 0xff ) {
    bitstream.write( 0xff, 8 );  // u(8)
  }
  bitstream.write( payloadType, 8 );  // u(8)

  // calculating the size of the sei message before writing it into the
  // bitstream, is there a better way to do this???
  PCCBitstream tempbitstream;
  seiPayload( tempbitstream, syntax, sei, nalUnitType );
  sei.setPayloadSize( tempbitstream.size() );

  auto payloadSize = static_cast<int32_t>( sei.getPayloadSize() );
  for ( ; payloadSize >= 0xff; payloadSize -= 0xff ) {
    bitstream.write( 0xff, 8 );  // u(8)
  }
  bitstream.write( payloadSize, 8 );  // u(8)
  seiPayload( bitstream, syntax, sei, nalUnitType );
}

// C.2 Sample stream NAL unit syntax and semantics
// C.2.1 Sample stream NAL header syntax
void PCCBitstreamWriter::sampleStreamNalHeader( PCCBitstream& bitstream, SampleStreamNalUnit& ssnu ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( ssnu.getSizePrecisionBytesMinus1(), 3 );  // u(3)
  TRACE_BITSTREAM( "UnitSizePrecisionBytesMinus1 = %lu \n", ssnu.getSizePrecisionBytesMinus1() );
  bitstream.write( 0, 5 );  // u(5)
}

// C.2.2 Sample stream NAL unit syntax
void PCCBitstreamWriter::sampleStreamNalUnit( PCCHighLevelSyntax&  syntax,
                                              PCCBitstream&        bitstream,
                                              SampleStreamNalUnit& ssnu,
                                              NalUnit&             nalu,
                                              size_t               index ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  TRACE_BITSTREAM( "UnitSizePrecisionBytesMinus1 = %lu \n", ssnu.getSizePrecisionBytesMinus1() );
  bitstream.write( nalu.getSize(),
                   8 * ( ssnu.getSizePrecisionBytesMinus1() + 1 ) );  // u(v)
  nalUnitHeader( bitstream, nalu );
  switch ( nalu.getType() ) {
    case NAL_ASPS:
      atlasSequenceParameterSetRbsp( syntax.getAtlasSequenceParameterSet( index ), syntax, bitstream );
      break;
    case NAL_AFPS: atlasFrameParameterSetRbsp( syntax.getAtlasFrameParameterSet( index ), syntax, bitstream ); break;
    case NAL_TRAIL_N:
    case NAL_TRAIL_R:
    case NAL_TSA_N:
    case NAL_TSA_R:
    case NAL_STSA_N:
    case NAL_STSA_R:
    case NAL_RADL_N:
    case NAL_RADL_R:
    case NAL_RASL_N:
    case NAL_RASL_R:
    case NAL_SKIP_N:
    case NAL_SKIP_R: atlasTileLayerRbsp( syntax.getAtlasTileLayer( index ), syntax, bitstream ); break;
    case NAL_SUFFIX_ESEI:
    case NAL_SUFFIX_NSEI: seiRbsp( syntax, bitstream, syntax.getSeiSuffix( index ), nalu.getType() );
    case NAL_PREFIX_ESEI:
    case NAL_PREFIX_NSEI: seiRbsp( syntax, bitstream, syntax.getSeiPrefix( index ), nalu.getType() ); break;
    default: fprintf( stderr, "sampleStreamNalUnit type = %d not supported\n", static_cast<int32_t>( nalu.getType() ) );
  }
}

// F.2  SEI payload syntax
// F.2.1  General SEI message syntax
void PCCBitstreamWriter::seiPayload( PCCBitstream&       bitstream,
                                     PCCHighLevelSyntax& syntax,
                                     SEI&                sei,
                                     NalUnitType         nalUnitType ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto payloadType = sei.getPayloadType();
  if ( nalUnitType == NAL_PREFIX_ESEI || nalUnitType == NAL_PREFIX_NSEI ) {
    if ( payloadType == BUFFERING_PERIOD ) {  // 0
      bool                 NalHrdBpPresentFlag = false;
      bool                 AclHrdBpPresentFlag = false;
      std::vector<uint8_t> hrdCabCntMinus1;
      bufferingPeriod( bitstream, sei, NalHrdBpPresentFlag, AclHrdBpPresentFlag, hrdCabCntMinus1 );
    } else if ( payloadType == ATLAS_FRAME_TIMING ) {  // 1
      atlasFrameTiming( bitstream, sei, false );
    } else if ( payloadType == FILLER_PAYLOAD ) {  // 2
      fillerPayload( bitstream, sei );
    } else if ( payloadType == USER_DATAREGISTERED_ITUTT35 ) {  // 3
      userDataRegisteredItuTT35( bitstream, sei );
    } else if ( payloadType == USER_DATA_UNREGISTERED ) {  // 4
      userDataUnregistered( bitstream, sei );
    } else if ( payloadType == RECOVERY_POINT ) {  // 5
      recoveryPoint( bitstream, sei );
    } else if ( payloadType == NO_DISPLAY ) {  // 6
      noDisplay( bitstream, sei );
    } else if ( payloadType == TIME_CODE ) {  // 7
      // timeCode( bitstream, sei );
    } else if ( payloadType == REGIONAL_NESTING ) {  // 8
      // regionalNesting( bitstream, sei );
    } else if ( payloadType == SEI_MANIFEST ) {  // 9
      seiManifest( bitstream, sei );
    } else if ( payloadType == SEI_PREFIX_INDICATION ) {  // 10
      seiPrefixIndication( bitstream, sei );
    } else if ( payloadType == ATTRIBUTE_TRANSFORMATION_PARAMS ) {  // 11
      attributeTransformationParams( bitstream, sei );
    } else if ( payloadType == ACTIVE_SUB_BITSTREAMS ) {  // 12
      activeSubBitstreams( bitstream, sei );
    } else if ( payloadType == COMPONENT_CODEC_MAPPING ) {  // 13
      componentCodecMapping( bitstream, sei );
    } else if ( payloadType == SCENE_OBJECT_INFORMATION ) {  // 14
      sceneObjectInformation( bitstream, sei );
    } else if ( payloadType == OBJECT_LABEL_INFORMATION ) {  // 15
      objectLabelInformation( bitstream, sei );
    } else if ( payloadType == PATCH_INFORMATION ) {  // 16
      patchInformation( bitstream, sei );
    } else if ( payloadType == VOLUMETRIC_RECTANGLE_INFORMATION ) {  // 17
      volumetricRectangleInformation( bitstream, sei );
    } else if ( payloadType == ATLAS_INFORMATION ) {  // 18
      atlasInformation( bitstream, sei );
    } else if ( payloadType == VIEWPORT_CAMERA_PARAMETERS ) {  // 19
      viewportCameraParameters( bitstream, sei );
    } else if ( payloadType == VIEWPORT_POSITION ) {  // 20
      viewportPosition( bitstream, sei );
    } else if ( payloadType == GEOMETRY_SMOOTHING ) {  // 64
      geometrySmoothing( bitstream, sei );
    } else if ( payloadType == ATTRIBUTE_SMOOTHING ) {  // 65
      attributeSmoothing( bitstream, sei );
    } else {
      reservedSeiMessage( bitstream, sei );
    }
  } else { /* psdUnitType  ==  NAL_SUFFIX_SEI */
    SEI& sei = syntax.addSeiSuffix( payloadType, nalUnitType == NAL_SUFFIX_ESEI );
    if ( payloadType == 2 ) {
      fillerPayload( bitstream, sei );
    } else if ( payloadType == 3 ) {
      userDataRegisteredItuTT35( bitstream, sei );
    } else if ( payloadType == 4 ) {
      userDataUnregistered( bitstream, sei );
    } else {
      reservedSeiMessage( bitstream, sei );
    }
  }
  // if ( moreDataInPayload() ) {
  //   if ( payloadExtensionPresent() ) {
  //     spReservedPayloadExtensionData;  // u(v)
  //   }
  //   byteAlignment( bitstream );
  // }
  if ( !bitstream.byteAligned() ) {  // this prevents from writing one more byte,
                                     // in case the payload is already byte
                                     // aligned (see xWriteByteAlign in HM)
    byteAlignment( bitstream );
  }
}

// F.2.2  Filler payload SEI message syntax
void PCCBitstreamWriter::fillerPayload( PCCBitstream& bitstream, SEI& sei ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto payloadSize = sei.getPayloadSize();
  for ( size_t k = 0; k < payloadSize; k++ ) {
    bitstream.write( 0xFF, 8 );  // f(8) equal to 0xFF
  }
}

// F.2.3  User data registered by Recommendation ITU-T T.35 SEI message syntax
void PCCBitstreamWriter::userDataRegisteredItuTT35( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei         = static_cast<SEIUserDataRegisteredItuTT35&>( seiAbstract );
  auto  payloadSize = sei.getPayloadSize();
  bitstream.write( sei.getCountryCode(), 8 );  // b(8)
  payloadSize--;
  if ( sei.getCountryCode() == 0xFF ) {
    bitstream.write( sei.getCountryCodeExtensionByte(), 8 );  // b(8)
    payloadSize--;
  }
  auto& payload = sei.getPayloadByte();
  for ( auto& element : payload ) {
    bitstream.write( element, 8 );  // b(8)
  }
}

// F.2.4  User data unregistered SEI message syntax
void PCCBitstreamWriter::userDataUnregistered( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei         = static_cast<SEIUserDataUnregistered&>( seiAbstract );
  auto  payloadSize = sei.getPayloadSize();
  for ( size_t i = 0; i < 16; i++ ) {
    bitstream.write( sei.getUuidIsoIec11578( i ), 8 );  // u(128) <=> 16 * u(8)
  }
  payloadSize -= 16;
  for ( size_t i = 0; i < payloadSize; i++ ) {
    bitstream.write( sei.getUserDataPayloadByte( i ), 8 );  // b(8)
  }
}

// F.2.5  Recovery point SEI message syntax
void PCCBitstreamWriter::recoveryPoint( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIRecoveryPoint&>( seiAbstract );
  bitstream.writeSvlc( sei.getRecoveryAfocCnt() );  // se(v)
  bitstream.write( sei.getExactMatchFlag(), 1 );    // u(1)
  bitstream.write( sei.getBrokenLinkFlag(), 1 );    // u(1)
}

// F.2.6  No display SEI message syntax
void PCCBitstreamWriter::noDisplay( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
}

// F.2.7  Reserved SEI message syntax
void PCCBitstreamWriter::reservedSeiMessage( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei         = static_cast<SEIReservedSeiMessage&>( seiAbstract );
  auto  payloadSize = sei.getPayloadSize();
  for ( size_t i = 0; i < payloadSize; i++ ) {
    bitstream.write( sei.getPayloadByte( i ), 8 );  // b(8)
  }
}

// F.2.8  SEI manifest SEI message syntax
void PCCBitstreamWriter::seiManifest( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIManifest&>( seiAbstract );
  bitstream.write( sei.getNumSeiMsgTypes(), 16 );  // u(16)
  for ( size_t i = 0; i < sei.getNumSeiMsgTypes(); i++ ) {
    bitstream.write( sei.getSeiPayloadType( i ), 16 );  // u(16)
    bitstream.write( sei.getSeiDescription( i ), 8 );   // u(8)
  }
}

// F.2.9  SEI prefix indication SEI message syntax
void PCCBitstreamWriter::seiPrefixIndication( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIPrefixIndication&>( seiAbstract );
  bitstream.write( sei.getPrefixSeiPayloadType(), 16 );          // u(16)
  bitstream.write( sei.getNumSeiPrefixIndicationsMinus1(), 8 );  // u(8)
  for ( size_t i = 0; i <= sei.getNumSeiPrefixIndicationsMinus1(); i++ ) {
    bitstream.write( sei.getNumBitsInPrefixIndicationMinus1( i ), 16 );  // u(16)
    for ( size_t j = 0; j <= sei.getNumBitsInPrefixIndicationMinus1( i ); j++ ) {
      bitstream.write( sei.getSeiPrefixDataBit( i, j ), 1 );  // u(1)
    }
    while ( !bitstream.byteAligned() ) {
      bitstream.write( 1, 1 );  // f(1): equal to 1
    }
  }
}

// F.2.10  Attribute transformation parameters SEI message syntax
void PCCBitstreamWriter::attributeTransformationParams( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIAttributeTransformationParams&>( seiAbstract );
  bitstream.write( sei.getCancelFlag(), 1 );  // u(1)
  if ( !sei.getCancelFlag() ) {
    bitstream.writeUvlc( sei.getNumAttributeUpdates() );  // ue(v)
    for ( size_t j = 0; j < sei.getNumAttributeUpdates(); j++ ) {
      bitstream.write( sei.getAttributeIdx( j ), 8 );  // u(8)
      size_t index = sei.getAttributeIdx( j );
      bitstream.write( sei.getDimensionMinus1( index ), 8 );  // u(8)
      for ( size_t i = 0; i < sei.getDimensionMinus1( index ); i++ ) {
        bitstream.write( sei.getScaleParamsEnabledFlag( index, i ), 1 );  // u(1)
        bitstream.write( sei.getOffsetParamsEnabledFlag( index, i ),
                         1 );  // u(1)
        if ( sei.getScaleParamsEnabledFlag( index, i ) ) {
          bitstream.write( sei.getAttributeScale( index, i ), 32 );  // u(32)
        }
        if ( sei.getOffsetParamsEnabledFlag( index, i ) ) {
          bitstream.writeS( sei.getAttributeOffset( index, i ), 32 );  // i(32)
        }
      }
    }
    bitstream.write( sei.getPersistenceFlag(), 1 );  // u(1)
  }
}

// F.2.11  Active substreams SEI message syntax
void PCCBitstreamWriter::activeSubBitstreams( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIActiveSubBitstreams&>( seiAbstract );
  bitstream.write( sei.getActiveAttributesChangesFlag(), 1 );          // u(1)
  bitstream.write( sei.getActiveMapsChangesFlag(), 1 );                // u(1)
  bitstream.write( sei.getAuxiliaryPointsSubstreamsActiveFlag(), 1 );  // u(1)
  if ( sei.getActiveAttributesChangesFlag() ) {
    bitstream.write( sei.getAllAttributesActiveFlag(), 1 );  // u(1)
    if ( !sei.getAllAttributesActiveFlag() ) {
      bitstream.write( sei.getActiveAttributeCountMinus1(), 7 );  // u(7)
      for ( size_t i = 0; i <= sei.getActiveAttributeCountMinus1(); i++ ) {
        bitstream.write( sei.getActiveAttributeIdx( i ), 7 );  // u(7)
      }
    }
  }
  if ( sei.getActiveMapsChangesFlag() ) {
    bitstream.write( sei.getAllMapsActiveFlag(), 1 );  // u(1)
    if ( !sei.getAllMapsActiveFlag() ) {
      bitstream.write( sei.getActiveMapCountMinus1(), 4 );  // u(4)
      // sei.getActiveMapIdx().resize( sei.getActiveMapCountMinus1() + 1, 0 );
      for ( size_t i = 0; i <= sei.getActiveMapCountMinus1(); i++ ) {
        bitstream.write( sei.getActiveMapIdx( i ), 4 );  // u(4)
      }
    }
  }
}

// F.2.12  Component codec mapping SEI message syntax
void PCCBitstreamWriter::componentCodecMapping( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIComponentCodecMapping&>( seiAbstract );
  bitstream.write( sei.getCodecMappingsCountMinus1(), 8 );  // u(8)
  sei.allocate();
  for ( size_t i = 0; i <= sei.getCodecMappingsCountMinus1(); i++ ) {
    bitstream.write( sei.getCodecId( i ), 8 );                        // u(8)
    bitstream.writeString( sei.getCodec4cc( sei.getCodecId( i ) ) );  // st(v)
  }
}

// m52705
// F.2.14  Volumetric Tiling SEI message syntax
// F.2.14.1  General
// F.2.14.2  Volumetric Tiling Info Labels
// F.2.14.3  Volumetric Tiling Info Objects

// F.2.13.1	Scene object information SEI message syntax
void PCCBitstreamWriter::sceneObjectInformation( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto&         sei           = static_cast<SEISceneObjectInformation&>( seiAbstract );
  const int32_t fixedBitcount = 16;
  bitstream.write( sei.getCancelFlag(), 1 );
  bitstream.writeUvlc( sei.getNumObjectUpdates() );
  if ( sei.getNumObjectUpdates() > 0 ) {
    bitstream.write( sei.getSimpleObjectsFlag(), 1 );
    if ( static_cast<int>( sei.getSimpleObjectsFlag() ) == 0 ) {
      bitstream.write( sei.getObjectLabelPresentFlag(), 1 );
      bitstream.write( sei.getPriorityPresentFlag(), 1 );
      bitstream.write( sei.getObjectHiddenPresentFlag(), 1 );
      bitstream.write( sei.getObjectDependencyPresentFlag(), 1 );
      bitstream.write( sei.getVisibilityConesPresentFlag(), 1 );
      bitstream.write( sei.get3dBoundingBoxPresentFlag(), 1 );
      bitstream.write( sei.getCollisionShapePresentFlag(), 1 );
      bitstream.write( sei.getPointStylePresentFlag(), 1 );
      bitstream.write( sei.getMaterialIdPresentFlag(), 1 );
      bitstream.write( sei.getExtensionPresentFlag(), 1 );
    }
    if ( sei.get3dBoundingBoxPresentFlag() ) {
      bitstream.write( sei.get3dBoundingBoxScaleLog2(), 5 );
      bitstream.write( sei.get3dBoundingBoxPrecisionMinus8(), 5 );
    }
    bitstream.write( sei.getLog2MaxObjectIdxUpdated(), 5 );
    if ( sei.getObjectDependencyPresentFlag() ) { bitstream.write( sei.getLog2MaxObjectDependencyIdx(), 5 ); }
    for ( size_t i = 0; i <= sei.getNumObjectUpdates(); i++ ) {
      assert( sei.getObjectIdx( i ) >= sei.getNumObjectUpdates() );
      bitstream.write( sei.getObjectIdx( i ), sei.getLog2MaxObjectIdxUpdated() );
      size_t k = sei.getObjectIdx( i );
      bitstream.write( sei.getObjectCancelFlag( k ), 1 );
      // ObjectTracked[k]=!piObjectCancelFlag[k];
      if ( sei.getObjectCancelFlag( k ) ) {
        if ( sei.getObjectLabelPresentFlag() ) {
          bitstream.write( sei.getObjectLabelUpdateFlag( k ), 1 );
          if ( sei.getObjectLabelUpdateFlag( k ) ) { bitstream.writeUvlc( sei.getObjectLabelIdx( k ) ); }
        }
        if ( sei.getPriorityPresentFlag() ) {
          bitstream.write( sei.getPriorityUpdateFlag( k ), 1 );
          if ( sei.getPriorityUpdateFlag( k ) ) { bitstream.write( sei.getPriorityValue( k ), 4 ); }
        }
        if ( sei.getObjectHiddenPresentFlag() ) { bitstream.write( sei.getObjectHiddenFlag( k ), 1 ); }

        if ( sei.getObjectDependencyPresentFlag() ) {
          bitstream.write( sei.getObjectDependencyUpdateFlag( k ), 1 );
          if ( sei.getObjectDependencyUpdateFlag( k ) ) {
            bitstream.write( sei.getObjectNumDependencies( k ), 4 );
            size_t bitCount = ceil( log2( sei.getObjectNumDependencies( k ) ) + 0.5 );
            for ( size_t j = 0; j < sei.getObjectNumDependencies( k ); j++ ) {
              bitstream.write( sei.getObjectDependencyIdx( k, j ), bitCount );
            }
          }
        }
        if ( sei.getVisibilityConesPresentFlag() ) {
          bitstream.write( sei.getVisibilityConesUpdateFlag( k ), 1 );
          if ( sei.getVisibilityConesUpdateFlag( k ) ) {
            bitstream.write( sei.getDirectionX( k ), 32 );
            bitstream.write( sei.getDirectionY( k ), 32 );
            bitstream.write( sei.getDirectionZ( k ), 32 );
            bitstream.write( sei.getAngle( k ), 16 );
          }
        }  // cones

        if ( sei.get3dBoundingBoxPresentFlag() ) {
          bitstream.write( sei.get3dBoundingBoxUpdateFlag( k ), 1 );
          if ( sei.get3dBoundingBoxUpdateFlag( k ) ) {
            bitstream.write( sei.get3dBoundingBoxX( k ), fixedBitcount );
            bitstream.write( sei.get3dBoundingBoxY( k ), fixedBitcount );
            bitstream.write( sei.get3dBoundingBoxZ( k ), fixedBitcount );
            bitstream.write( sei.get3dBoundingBoxDeltaX( k ), fixedBitcount );
            bitstream.write( sei.get3dBoundingBoxDeltaY( k ), fixedBitcount );
            bitstream.write( sei.get3dBoundingBoxDeltaZ( k ), fixedBitcount );
          }
        }  // 3dBB

        if ( sei.getCollisionShapePresentFlag() ) {
          bitstream.write( sei.getCollisionShapeUpdateFlag( k ), 1 );
          if ( sei.getCollisionShapeUpdateFlag( k ) ) { bitstream.write( sei.getCollisionShapeId( k ), 16 ); }
        }  // collision
        if ( sei.getPointStylePresentFlag() ) {
          bitstream.write( sei.getPointStyleUpdateFlag( k ), 1 );
          if ( sei.getPointStyleUpdateFlag( k ) ) {
            bitstream.write( sei.getPointShapeId( k ), 8 );  // only shape??
          }
          bitstream.write( sei.getPointSize( k ), 16 );
        }  // pointstyle
        if ( sei.getMaterialIdPresentFlag() ) {
          bitstream.write( sei.getMaterialIdUpdateFlag( k ), 1 );
          if ( sei.getMaterialIdUpdateFlag( k ) ) { bitstream.write( sei.getMaterialId( k ), 16 ); }
        }  // materialid
      }    // sei.getObjectCancelFlag(k)
    }      // for(size_t i=0; i<=sei.getNumObjectUpdates(); i++)
  }        // if( sei.getNumObjectUpdates() > 0 )
}

// F.2.13.2 Object label information SEI message syntax
void PCCBitstreamWriter::objectLabelInformation( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIObjectLabelInformation&>( seiAbstract );
  bitstream.write( sei.getCancelFlag(), 1 );
  if ( !sei.getCancelFlag() ) {
    bitstream.write( sei.getLabelLanguagePresentFlag(), 1 );
    if ( sei.getLabelLanguagePresentFlag() ) {
      while ( !bitstream.byteAligned() ) { bitstream.write( uint32_t( 0 ), 1 ); }
      bitstream.writeString( sei.getLabelLanguage() );
    }
    bitstream.writeUvlc( sei.getNumLabelUpdates() );
    for ( size_t i = 0; i < sei.getNumLabelUpdates(); i++ ) {
      bitstream.writeUvlc( sei.getLabelIdx( i ) );
      if ( !sei.getLabelCancelFlag() ) {
        while ( !bitstream.byteAligned() ) { bitstream.write( uint32_t( 0 ), 1 ); }
        bitstream.writeString( sei.getLabel( sei.getLabelIdx( i ) ) );
      }
    }
    bitstream.write( sei.getPersistenceFlag(), 1 );  // u(1)
  }
};

// F.2.13.3 Patch information SEI message syntax
void PCCBitstreamWriter::patchInformation( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIPatchInformation&>( seiAbstract );
  bitstream.write( sei.getPersistenceFlag(), 1 );  // u(1)
  bitstream.write( sei.getResetFlag(), 1 );
  bitstream.writeUvlc( sei.getNumTileUpdates() );
  if ( sei.getNumTileUpdates() > 0 ) {
    bitstream.write( sei.getLog2MaxObjectIdxTracked(), 5 );
    bitstream.write( sei.getLog2MaxPatchIdxUpdated(), 4 );
  }
  for ( size_t i = 0; i < sei.getNumTileUpdates(); i++ ) {
    bitstream.writeUvlc( sei.getTileAddress( i ) );
    size_t j = sei.getTileAddress( i );
    bitstream.write( sei.getTileCancelFlag( j ), 1 );
    bitstream.writeUvlc( sei.getNumPatchUpdates( j ) );
    for ( size_t k = 0; k < sei.getNumPatchUpdates( j ); k++ ) {
      bitstream.write( sei.getPatchIdx( j, k ), sei.getLog2MaxPatchIdxUpdated() );
      auto p = sei.getPatchIdx( j, k );
      bitstream.write( sei.getPatchCancelFlag( j, p ), 1 );
      if ( !sei.getPatchCancelFlag( j, p ) ) {
        bitstream.writeUvlc( sei.getPatchNumberOfObjectsMinus1( j, p ) );
        for ( size_t n = 0; n < sei.getPatchNumberOfObjectsMinus1( j, p ) + 1; n++ ) {
          bitstream.write( sei.getPatchObjectIdx( j, p, n ),
                           sei.getLog2MaxObjectIdxTracked() );  //?pi_log2_max_object_idx_updated?
        }
      }
    }
  }
};

// F.2.13.4 Volumetric rectangle information SEI message syntax
void PCCBitstreamWriter::volumetricRectangleInformation( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto&         sei           = static_cast<SEIVolumetricRectangleInformation&>( seiAbstract );
  const int32_t fixedBitcount = 16;
  bitstream.write( sei.getPersistenceFlag(), 1 );  // u(1)
  bitstream.write( sei.getResetFlag(), 1 );        // u(1)
  bitstream.writeUvlc( sei.getNumRectanglesUpdates() );
  if ( sei.getNumRectanglesUpdates() > 0 ) {
    bitstream.write( sei.getLog2MaxObjectIdxTracked(), 5 );
    bitstream.write( sei.getLog2MaxRectangleIdxUpdated(), 4 );
  }
  for ( size_t k = 0; k < sei.getNumRectanglesUpdates(); k++ ) {
    bitstream.write( sei.getRectangleIdx( k ), sei.getLog2MaxRectangleIdxUpdated() );
    auto p = sei.getRectangleIdx( k );
    bitstream.write( sei.getRectangleCancelFlag( p ), 1 );
    if ( !sei.getRectangleCancelFlag( p ) ) {
      bitstream.write( sei.getBoundingBoxUpdateFlag( p ), 1 );
      if ( sei.getBoundingBoxUpdateFlag( p ) ) {
        bitstream.write( sei.getBoundingBoxTop( p ), fixedBitcount );
        bitstream.write( sei.getBoundingBoxLeft( p ), fixedBitcount );
        bitstream.write( sei.getBoundingBoxWidth( p ), fixedBitcount );
        bitstream.write( sei.getBoundingBoxHeight( p ), fixedBitcount );
      }
      bitstream.writeUvlc( sei.getRectangleNumberOfObjectsMinus1( p ) );
      for ( size_t n = 0; n < sei.getRectangleNumberOfObjectsMinus1( p ) + 1; n++ ) {
        bitstream.write( sei.getRectangleObjectIdx( p, n ), sei.getLog2MaxObjectIdxTracked() );
      }
    }
  }
};

// F.2.15  Buffering period SEI message syntax
void PCCBitstreamWriter::bufferingPeriod( PCCBitstream&        bitstream,
                                          SEI&                 seiAbstract,
                                          bool                 NalHrdBpPresentFlag,
                                          bool                 AclHrdBpPresentFlag,
                                          std::vector<uint8_t> hrdCabCntMinus1 ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto&         sei           = static_cast<SEIBufferingPeriod&>( seiAbstract );
  const int32_t fixedBitcount = 16;
  bitstream.writeUvlc( sei.getAtlasSequenceParameterSetId() );  // ue(v)
  bitstream.write( sei.getIrapCabParamsPresentFlag(), 1 );      // u(1)
  if ( sei.getIrapCabParamsPresentFlag() ) {
    bitstream.write( sei.getCabDelayOffset(), fixedBitcount );  // u(v)
    bitstream.write( sei.getDabDelayOffset(), fixedBitcount );  // u(v)
  }
  bitstream.write( sei.getConcatenationFlag(), 1 );  // u(1)
  bitstream.write( sei.getAtlasCabRemovalDelayDeltaMinus1(),
                   fixedBitcount );                   // u(v)
  bitstream.write( sei.getMaxSubLayersMinus1(), 3 );  // u(3)
  for ( size_t i = 0; i <= sei.getMaxSubLayersMinus1(); i++ ) {
    if ( NalHrdBpPresentFlag ) {
      for ( size_t j = 0; j < hrdCabCntMinus1[i] + 1; j++ ) {
        bitstream.write( sei.getNalInitialCabRemovalDelay( i, j ),
                         fixedBitcount );  // u(v)
        bitstream.write( sei.getNalInitialCabRemovalOffset( i, j ),
                         fixedBitcount );  // u(v)
      }
      if ( sei.getIrapCabParamsPresentFlag() ) {
        bitstream.write( sei.getNalInitialAltCabRemovalDelay( i ),
                         fixedBitcount );  // u(v)
        bitstream.write( sei.getNalInitialAltCabRemovalOffset( i ),
                         fixedBitcount );  // u(v)
      }
    }
    if ( AclHrdBpPresentFlag ) {
      for ( size_t j = 0; j < hrdCabCntMinus1[i] + 1; j++ ) {
        bitstream.write( sei.getAclInitialCabRemovalDelay( i, j ),
                         fixedBitcount );  // u(v)
        bitstream.write( sei.getAclInitialCabRemovalOffset( i, j ),
                         fixedBitcount );  // u(v)
      }
      if ( sei.getIrapCabParamsPresentFlag() ) {
        bitstream.write( sei.getAclInitialAltCabRemovalDelay( i ),
                         fixedBitcount );  // u(v)
        bitstream.write( sei.getAclInitialAltCabRemovalOffset( i ),
                         fixedBitcount );  // u(v)
      }
    }
  }
}

// F.2.16  Atlas frame timing SEI message syntax
void PCCBitstreamWriter::atlasFrameTiming( PCCBitstream& bitstream, SEI& seiAbstract, bool cabDabDelaysPresentFlag ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto&         sei           = static_cast<SEIAtlasFrameTiming&>( seiAbstract );
  const int32_t fixedBitcount = 16;
  if ( cabDabDelaysPresentFlag ) {
    bitstream.write( sei.getAftCabRemovalDelayMinus1(), fixedBitcount );  // u(v)
    bitstream.write( sei.getAftDabOutputDelay(), fixedBitcount );         // u(v)
  }
}

// F.2.13.5  Atlas information  SEI message syntax
void PCCBitstreamWriter::atlasInformation( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIAtlasInformation&>( seiAbstract );
  bitstream.write( sei.getPersistenceFlag(), 1 );   //	u(1)
  bitstream.write( sei.getResetFlag(), 1 );         // 	u(1)
  bitstream.write( sei.getNumAtlasesMinus1(), 6 );  // 	u(6)
  bitstream.writeUvlc( sei.getNumUpdates() );       // ue(v)
  if ( sei.getNumUpdates() > 0 ) {
    bitstream.write( sei.getLog2MaxObjectIdxTracked(), 5 );  //	u(5)
    for ( size_t i = 0; i < sei.getNumUpdates() + 1; i++ ) {
      bitstream.write( sei.getObjectIdx( i ), sei.getLog2MaxObjectIdxTracked() );  // u(v)
      for ( size_t j = 0; j < sei.getNumAtlasesMinus1() + 1; j++ ) {
        bitstream.write( sei.getObjectInAtlasPresentFlag( i, j ), 1 );  // u(1)
      }
    }
  }
}

// F.2.16.1	Viewport camera parameters SEI messages syntax
void PCCBitstreamWriter::viewportCameraParameters( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIViewportCameraParameters&>( seiAbstract );
  bitstream.write( sei.getCameraId(), 10 );   // u(10)
  bitstream.write( sei.getCancelFlag(), 1 );  // u(1)
  if ( sei.getCameraId() > 0 && !sei.getCancelFlag() ) {
    bitstream.write( sei.getPersistenceFlag(), 1 );              // u(1)
    bitstream.write( sei.getCameraType(), 3 );                   // u(3)
    if ( sei.getCameraType() == 0 ) {                            // equirectangular
      bitstream.write( sei.getErpHorizontalFov(), 32 );          // fl(32)
      bitstream.write( sei.getErpVerticalFov(), 32 );            // fl(32)
    } else if ( sei.getCameraType() == 1 ) {                     // perspective
      bitstream.write( sei.getPerspectiveAspectRatio(), 32 );    // fl(32)
      bitstream.write( sei.getPerspectiveHorizontalFov(), 32 );  // fl(32)
    } else if ( sei.getCameraType() == 2 ) {                     /* orthographic */
      bitstream.write( sei.getOrthoAspectRatio(), 32 );          // fl(32)
      bitstream.write( sei.getOrthoHorizontalSize(), 32 );       // fl(32)
    }
    bitstream.write( sei.getClippingNearPlane(), 32 );  // fl(32)
    bitstream.write( sei.getClippingFarPlane(), 32 );   // fl(32)
  }
}

// F.2.16.1	Viewport camera parameters SEI messages syntax
void PCCBitstreamWriter::viewportPosition( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIViewportPosition&>( seiAbstract );
  bitstream.write( sei.getViewportId(), 8 );                   // u(8)
  bitstream.write( sei.getCameraParametersPresentFlag(), 1 );  // u(1)
  if ( sei.getCameraParametersPresentFlag() ) {
    bitstream.write( sei.getViewportId(), 10 );  //	u(10)
  }
  bitstream.write( sei.getCancelFlag(), 1 );  // u(1)
  if ( !sei.getCancelFlag() ) {
    bitstream.write( sei.getPersistenceFlag(), 1 );  // u(1)
    for ( size_t d = 0; d < 3; d++ ) {
      bitstream.write( sei.getPosition( d ), 32 );  //	fl(32)
    }
    bitstream.write( sei.getQuaternionX(), 32 );    //	fl(32)
    bitstream.write( sei.getQuaternionY(), 32 );    //	fl(32)
    bitstream.write( sei.getQuaternionZ(), 32 );    //	fl(32)
    bitstream.write( sei.getCenterViewFlag(), 1 );  // 	u(1)
    if ( !sei.getCenterViewFlag() ) {
      bitstream.write( sei.getLeftViewFlag(), 1 );  // u(1)
    }
  }
}

// H.16.2.1 Geometry smoothing SEI message syntax
void PCCBitstreamWriter::geometrySmoothing( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIGeometrySmoothing&>( seiAbstract );
  bitstream.write( sei.getSmoothingPersistenceFlag(), 1 );   //	u(1)
  bitstream.write( sei.getSmoothingResetFlag(), 1 );         //	u(1)
  bitstream.write( sei.getSmoothingInstancesUpdated(), 8 );  //	u(8)
  for ( size_t i = 0; i < sei.getSmoothingInstancesUpdated(); i++ ) {
    bitstream.write( sei.getSmoothingInstanceIndex( i ), 8 );  // u(8)
    size_t k = sei.getSmoothingInstanceIndex( i );
    bitstream.write( sei.getSmoothingInstanceCancelFlag( k ), 1 );  //	u(1)
    if ( !sei.getSmoothingInstanceCancelFlag( k ) ) {
      bitstream.write( sei.getSmoothingMethodType( k ), 8 );  // u(8)
      if ( sei.getSmoothingMethodType( k ) == 1 ) {
        bitstream.write( sei.getSmoothingGridSizeMinus2( k ), 7 );  // u(8)
        bitstream.write( sei.getSmoothingThreshold( k ), 8 );       // u(8)
      } else if ( sei.getSmoothingMethodType( k ) == 2 ) {
        bitstream.write( sei.getPbfLog2ThresholdMinus1( k ), 2 );  //	u(2)
        bitstream.write( sei.getPbfPassesCountMinus1( k ), 2 );    //	u(2)
        bitstream.write( sei.getPbfFilterSizeMinus1( k ), 3 );     //	u(3)
      }
    }
  }
}

// H.16.2.2	Attribute smoothing SEI message syntax
void PCCBitstreamWriter::attributeSmoothing( PCCBitstream& bitstream, SEI& seiAbstract ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  auto& sei = static_cast<SEIAttributeSmoothing&>( seiAbstract );
  bitstream.write( sei.getSmoothingPersistenceFlag(), 1 );  //	u(1)
  bitstream.write( sei.getSmoothingResetFlag(), 1 );        //	u(1)
  bitstream.writeUvlc( sei.getNumAttributesUpdated() );     //	ue(v)
  for ( size_t j = 0; j < sei.getNumAttributesUpdated(); j++ ) {
    bitstream.write( sei.getAttributeIdx( j ), 7 );  // u(7)
    size_t k = sei.getAttributeIdx( j );
    bitstream.write( sei.getAttributeSmoothingCancelFlag( k ), 1 );  // u(1)
    bitstream.write( sei.getSmoothingInstancesUpdated( k ), 8 );     //	u(8)
    for ( size_t i = 0; i < sei.getSmoothingInstancesUpdated( k ); i++ ) {
      bitstream.write( sei.getSmoothingInstanceIndex( k, i ), 8 );  //	u(8)
      size_t m = sei.getSmoothingInstanceIndex( k, i );
      bitstream.write( sei.getSmoothingInstanceCancelFlag( k, m ), 1 );  // u(1)
      if ( sei.getSmoothingInstanceCancelFlag( k, m ) != 1 ) {
        bitstream.write( sei.getSmoothingMethodType( k, m ), 8 );  // u(8)
        if ( sei.getSmoothingMethodType( k, m ) ) {
          bitstream.write( sei.getSmoothingGridSizeMinus2( k, m ), 8 );       //	u(8)
          bitstream.write( sei.getSmoothingThreshold( k, m ), 8 );            // u(8)
          bitstream.write( sei.getSmoothingThresholdVariation( k, m ), 8 );   // u(8)
          bitstream.write( sei.getSmoothingThresholdDifference( k, m ), 8 );  // u(8)
        }
      }
    }
  }
}

// G.2 VUI syntax
// G.2.1 VUI parameters syntax
void PCCBitstreamWriter::vuiParameters( PCCBitstream& bitstream, VUIParameters& vp ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( vp.getTimingInfoPresentFlag(), 1 );  // u(1)
  if ( vp.getTimingInfoPresentFlag() ) {
    bitstream.write( vp.getNumUnitsInTick(), 32 );              // u(32)
    bitstream.write( vp.getTimeScale(), 32 );                   // u(32)
    bitstream.write( vp.getPocProportionalToTimingFlag(), 1 );  // u(1)
    if ( vp.getPocProportionalToTimingFlag() ) {
      bitstream.writeUvlc( vp.getNumTicksPocDiffOneMinus1() );  // ue(v)
    }
    bitstream.write( vp.getHrdParametersPresentFlag(), 1 );  // u(1)
    if ( vp.getHrdParametersPresentFlag() ) { hrdParameters( bitstream, vp.getHrdParameters() ); }
  }
  bitstream.write( vp.getBitstreamRestrictionPresentFlag(), 1 );  // u(1)
  if ( vp.getBitstreamRestrictionPresentFlag() ) {
    bitstream.write( vp.getTilesRestrictedFlag(), 1 );                    // u(1)
    bitstream.write( vp.getConsistentTilesForVideoComponentsFlag(), 1 );  // u(1)
    bitstream.writeUvlc( vp.getMaxNumTilesPerAtlas() );                   // ue(v)
  }
  bitstream.write( vp.getCoordinateSystemParametersPresentFlag(), 1 );  // u(1)
  if ( vp.getCoordinateSystemParametersPresentFlag() ) {
    coordinateSystemParameters( bitstream, vp.getCoordinateSystemParameters() );
  }
  bitstream.write( vp.getUnitInMetresFlag(), 1 );           // u(1)
  bitstream.write( vp.getDisplayBoxInfoPresentFlag(), 1 );  // u(1)
  if ( vp.getDisplayBoxInfoPresentFlag() ) {
    for ( size_t d = 0; d < 3; d++ ) {
      bitstream.writeUvlc( vp.getDisplayBoxOrigin( d ) );  // ue(v)
      bitstream.writeUvlc( vp.getDisplayBoxSize( d ) );    // ue(v)
    }
    bitstream.write( vp.getAnchorPointPresentFlag(), 1 );  // u(1)
    if ( vp.getAnchorPointPresentFlag() ) {
      for ( size_t d = 0; d < 3; d++ ) {
        bitstream.write( vp.getAnchorPoint( d ), 32 );  // fl(32)
      }
    }
  }
}

// G.2.2  HRD parameters syntax
void PCCBitstreamWriter::hrdParameters( PCCBitstream& bitstream, HrdParameters& hp ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( hp.getNalParametersPresentFlag(), 1 );  // u(1)
  bitstream.write( hp.getAclParametersPresentFlag(), 1 );  // u(1)
  if ( hp.getNalParametersPresentFlag() || hp.getAclParametersPresentFlag() ) {
    bitstream.write( hp.getBitRateScale(), 4 );                        // u(4)
    bitstream.write( hp.getCabSizeScale(), 4 );                        // u(4)
    bitstream.write( hp.getInitialCabRemovalDelayLengthMinus1(), 5 );  // u(5)
    bitstream.write( hp.getAuCabRemovalDelayLengthMinus1(), 5 );       // u(5)
    bitstream.write( hp.getDabOutputDelayLengthMinus1(), 5 );          // u(5)
  }
  for ( size_t i = 0; i <= hp.getMaxNumSubLayersMinus1(); i++ ) {
    bitstream.write( hp.getFixedAtlasRateGeneralFlag( i ), 1 );  // u(1)
    if ( !hp.getFixedAtlasRateGeneralFlag( i ) ) {
      bitstream.write( hp.getFixedAtlasRateWithinCasFlag( i ), 1 );  // u(1)
    }
    if ( hp.getFixedAtlasRateWithinCasFlag( i ) ) {
      bitstream.write( hp.getElementalDurationInTcMinus1( i ), 1 );  // ue(v)
    } else {
      bitstream.write( hp.getLowDelayFlag( i ), 1 );  // u(1)
    }
    if ( !hp.getLowDelayFlag( i ) ) {
      bitstream.write( hp.getCabCntMinus1( i ), 1 );  // ue(v)
    }
    if ( hp.getNalParametersPresentFlag() ) {
      hrdSubLayerParameters( bitstream, hp.getHdrSubLayerParameters( 0, i ),
                             static_cast<size_t>( hp.getCabCntMinus1( i ) ) );
    }
    if ( hp.getAclParametersPresentFlag() ) {
      hrdSubLayerParameters( bitstream, hp.getHdrSubLayerParameters( 1, i ),
                             static_cast<size_t>( hp.getCabCntMinus1( i ) ) );
    }
  }
}

// G.2.3  Sub-layer HRD parameters syntax
void PCCBitstreamWriter::hrdSubLayerParameters( PCCBitstream& bitstream, HrdSubLayerParameters& hlsp, size_t cabCnt ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  for ( size_t i = 0; i <= cabCnt; i++ ) {
    bitstream.writeUvlc( hlsp.getBitRateValueMinus1( i ) );  // ue(v)
    bitstream.writeUvlc( hlsp.getCabSizeValueMinus1( i ) );  // ue(v)
    bitstream.write( hlsp.getCbrFlag( i ), 1 );              // u(1)
  }
}

// G.2.4	Coordinate system parameters syntax
void PCCBitstreamWriter::coordinateSystemParameters( PCCBitstream& bitstream, CoordinateSystemParameters& csp ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( csp.getForwardAxis(), 2 );    // u(2)
  bitstream.write( csp.getDeltaLeftAxis(), 1 );  // u(1)
  bitstream.write( csp.getForwardSign(), 1 );    // u(1)
  bitstream.write( csp.getLeftSign(), 1 );       // u(1)
  bitstream.write( csp.getUpSign(), 1 );         // u(1)
}

// H.7.3.4.1	VPS V-PCC extension syntax
void PCCBitstreamWriter::vpsVpccExtension( PCCBitstream& bitstream, VpsVpccExtension& ext ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
}

// H.7.3.4.2	VPS MIV extension syntax
void PCCBitstreamWriter::vpsMivExtension( PCCBitstream& bitstream ) { TRACE_BITSTREAM( "%s \n", __func__ ); }

// H.7.3.6.1.1	ASPS V-PCC extension syntax
void PCCBitstreamWriter::aspsVpccExtension( PCCBitstream&                  bitstream,
                                            AtlasSequenceParameterSetRbsp& asps,
                                            AspsVpccExtension&             ext ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( ext.getRemoveDuplicatePointEnableFlag(), 1 );  // u(1)
  if ( asps.getPixelDeinterleavingFlag() || asps.getPointLocalReconstructionEnabledFlag() ) {
    bitstream.write( ext.getSurfaceThicknessMinus1(), 7 );  // u(?)
  }
}
// H.7.3.6.1.2	ASPS MIV extension syntax
void PCCBitstreamWriter::aspsMivExtension( PCCBitstream& bitstream ) { TRACE_BITSTREAM( "%s \n", __func__ ); }

// H.7.3.6.2.1	AFPS V-PCC extension syntax
void PCCBitstreamWriter::afpsVpccExtension( PCCBitstream& bitstream, AfpsVpccExtension& ext ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
}
// H.7.3.6.2.2	AFPS MIV extension syntax
void PCCBitstreamWriter::afpsMivExtension( PCCBitstream& bitstream ) { TRACE_BITSTREAM( "%s \n", __func__ ); }

// H.7.3.6.3.1	AAPS V-PCC extension syntax
void PCCBitstreamWriter::aapsVpccExtension( PCCBitstream& bitstream, AapsVpccExtension& ext ) {
  TRACE_BITSTREAM( "%s \n", __func__ );
  bitstream.write( ext.getCameraParametersPresentFlag(), 1 );  // u(1);
  if ( ext.getCameraParametersPresentFlag() ) { atlasCameraParameters( bitstream, ext.getAtlasCameraParameters() ); }
}
// H.7.3.6.3.2	AAPS MIV extension syntax
void PCCBitstreamWriter::aapsMivExtension( PCCBitstream& bitstream ) { TRACE_BITSTREAM( "%s \n", __func__ ); }

// H.7.3.6.3.2	Atlas camera parameters syntax
void PCCBitstreamWriter::atlasCameraParameters( PCCBitstream& bitstream, AtlasCameraParameters& acp ) {
  bitstream.write( acp.getCameraModel(), 8 );  // u(8)
  if ( acp.getCameraModel() == 1 ) {
    bitstream.write( acp.getScaleEnabledFlag(), 1 );     // u(1)
    bitstream.write( acp.getOffsetEnabledFlag(), 1 );    // u(1)
    bitstream.write( acp.getRotationEnabledFlag(), 1 );  // u(1)
    if ( acp.getScaleEnabledFlag() ) {
      for ( size_t i = 0; i < 3; i++ ) { bitstream.write( acp.getScaleOnAxis( i ), 32 ); }  // u(32)
    }
    if ( acp.getOffsetEnabledFlag() ) {
      for ( size_t i = 0; i < 3; i++ ) { bitstream.writeS( acp.getOffsetOnAxis( i ), 32 ); }  // i(32)
    }
    if ( acp.getRotationEnabledFlag() ) {
      for ( size_t i = 0; i < 3; i++ ) { bitstream.writeS( acp.getRotation( i ), 16 ); }  // i(16)
    }
  }
}
