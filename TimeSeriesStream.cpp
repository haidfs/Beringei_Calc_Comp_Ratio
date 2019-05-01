/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "TimeSeriesStream.h"

#include <vector>

#include "BitUtil.h"

DEFINE_int64(
    gorilla_blacklisted_time_min,
    0,
    //下限时间和上限时间
    "Lower time for blacklisted unix times to not return when decompressing"
    " gorilla block");
DEFINE_int64(
    gorilla_blacklisted_time_max,
    0,
    "Upper time for blacklisted unix times to not return when decompressing"
    " gorilla block");

namespace facebook {
namespace gorilla {
struct {
  int64_t bitsForValue;
  uint32_t controlValue;
  uint32_t controlValueBitLength;
} static const timestampEncodings[4] = {{7, 2, 2},
                                        {9, 6, 3},
                                        {12, 14, 4},
                                        {32, 15, 4}};

TimeSeriesStream::TimeSeriesStream() {
  prevTimestamp_ = 0;
  reset();
}

void TimeSeriesStream::reset() {
  // Shrink to fit before clearing to possibly free some unused
  // memory. (It will only free memory if there's more than 50% extra
  // allocated.)
  data_.shrink_to_fit();

  // This won't actually release the memory.
  data_.clear();
  numBits_ = 0;
  prevTimestampDelta_ = 0;
  previousValue_ = 0;
  previousValueLeadingZeros_ = 0;
  previousValueTrailingZeros_ = 0;

  // Do not reset the `prevTimestamp_` because it is still useful for
  // the callers. When data_ is empty, it is used only to enforce the
  // minTimestampDelta check across buckets.
}

uint32_t TimeSeriesStream::size() {
  return data_.size();
}

uint32_t TimeSeriesStream::capacity() {
  return data_.capacity();
}

void TimeSeriesStream::readData(char* out, uint32_t size) {
  memcpy(out, data_.data(), size);
}

void TimeSeriesStream::readData(std::string& out) {
  out = data_.toStdString();
}

const char* TimeSeriesStream::getDataPtr() {
  return data_.data();
}

bool TimeSeriesStream::append(
    const TimeValuePair& value,
    int64_t minTimestampDelta) {
    return append(value.unixTime, value.value, minTimestampDelta);
   // return true;
}

bool TimeSeriesStream::append(
    int64_t unixTime,
    double value,
    int64_t minTimestampDelta) {
  if (!appendTimestamp(unixTime, minTimestampDelta)) {
    return false;
  }

  appendValue(value);
  return true;
    //return true;
}

bool TimeSeriesStream::appendTimestamp(
    int64_t timestamp,
    int64_t minTimestampDelta) {
  // Store a delta of delta for the rest of the values in one of the
  // following ways
  //
  // '0' = delta of delta did not change
  // '10' followed by a value length of 7
  // '110' followed by a value length of 9
  // '1110' followed by a value length of 12
  // '1111' followed by a value length of 32



  //这里在原本的函数中加了Counter的功能，加了bitsforcompressts变量来统计ts压缩后的bit数，不写全局变量一起统计的原因是
  //为了避免线程不安全的情况发生，所以在appendTimestamp和后面的appendValue函数中分别加入临时变量。
  int64_t  bitsforcompressts=0;
  int64_t delta = timestamp - prevTimestamp_;

  // Skip the minTimestampDelta check for the first timestamp.
  if (delta < minTimestampDelta && prevTimestamp_ != 0) {
    return false;
  }

  if (data_.empty()) {
    // Store the first value as is
    BitUtil::addValueToBitString(
        timestamp, kBitsForFirstTimestamp, data_, numBits_);
    bitsforcompressts +=kBitsForFirstTimestamp;
    LOG(INFO) << "Wrote compressed TimeValuePairTS: 64 bits  -> "<< bitsforcompressts<<" bits";
   // bitsforcompressts=0;
    prevTimestamp_ = timestamp;
    prevTimestampDelta_ = kDefaultDelta;
    return true;
  }

  int64_t deltaOfDelta = delta - prevTimestampDelta_;

  if (deltaOfDelta == 0) {
    prevTimestamp_ = timestamp;
    BitUtil::addValueToBitString(0, 1, data_, numBits_);
     bitsforcompressts +=1;
    LOG(INFO) << "Wrote compressed TimeValuePairTS: 64 bits  -> "<< bitsforcompressts<<" bits";
    // bitsforcompressts=0;
    return true;
  }

  if (deltaOfDelta > 0) {
    // There are no zeros. Shift by one to fit in x number of bits
    deltaOfDelta--;
  }

  int64_t absValue = std::abs(deltaOfDelta);

  for (int i = 0; i < 4; i++) {
    if (absValue < ((int64_t)1 << (timestampEncodings[i].bitsForValue - 1))) {
      BitUtil::addValueToBitString(
          timestampEncodings[i].controlValue,
          timestampEncodings[i].controlValueBitLength,
          data_,
          numBits_);
      //注释掉了最初的加法计算，采用switch，尽量使用原子操作。

     // bitsforcompressts +=timestampEncodings[i].controlValueBitLength;
      // Make this value between [0, 2^timestampEncodings[i].bitsForValue - 1]
      int64_t encodedValue = deltaOfDelta +
          ((int64_t)1 << (timestampEncodings[i].bitsForValue - 1));

      BitUtil::addValueToBitString(
          encodedValue, timestampEncodings[i].bitsForValue, data_, numBits_);
      // bitsforcompressts +=timestampEncodings[i].bitsForValue;
      switch (i)
      {
	      case 0:  bitsforcompressts=9;LOG(INFO) << "Wrote compressed TimeValuePairTS: 64 bits  -> "<< bitsforcompressts<<" bits"; bitsforcompressts=0;break;
	      case 1:  bitsforcompressts=12;LOG(INFO) << "Wrote compressed TimeValuePairTS: 64 bits  -> "<< bitsforcompressts<<" bits"; bitsforcompressts=0;break;
	      case 2:  bitsforcompressts=16;LOG(INFO) << "Wrote compressed TimeValuePairTS: 64 bits  -> "<< bitsforcompressts<<" bits"; bitsforcompressts=0;break;
	      case 3:  bitsforcompressts=36;LOG(INFO) << "Wrote compressed TimeValuePairTS: 64 bits  -> "<< bitsforcompressts<<" bits"; bitsforcompressts=0;break;
	      default:break;

      }
      break;
    }
  }

  prevTimestamp_ = timestamp;
  prevTimestampDelta_ = delta;

  return true;
}

void TimeSeriesStream::appendValue(double value) {
  uint64_t* p = (uint64_t*)&value;
  uint64_t xorWithPrevius = previousValue_ ^ *p;

  // Doubles are encoded by XORing them with the previous value.  If
  // XORing results in a zero value (value is the same as the previous
  // value), only a single zero bit is stored, otherwise 1 bit is
  // stored. TODO : improve this with RLE for the number of zeros
  //
  // For non-zero XORred results, there are two choices:
  //
  // 1) If the block of meaningful bits falls in between the block of
  //    previous meaningful bits, i.e., there are at least as many
  //    leading zeros and as many trailing zeros as with the previous
  //    value, use that information for the block position and just
  //    store the XORred value.
  //
  // 2) Length of the number of leading zeros is stored in the next 5
  //    bits, then length of the XORred value is stored in the next 6
  //    bits and finally the XORred value is stored.
  int64_t  bitsforcompressvalue = 0;
  if (xorWithPrevius == 0) {
    BitUtil::addValueToBitString(0, 1, data_, numBits_);
    bitsforcompressvalue +=1;
   LOG(INFO) << "Wrote compressed TimeValuePairValue: 64 bits  -> "<< bitsforcompressvalue<<" bits";
    return;
  }

  BitUtil::addValueToBitString(1, 1, data_, numBits_);
  bitsforcompressvalue +=1;
  int leadingZeros = __builtin_clzll(xorWithPrevius);
  int trailingZeros = __builtin_ctzll(xorWithPrevius);

  if (leadingZeros > kMaxLeadingZerosLength) {
    leadingZeros = kMaxLeadingZerosLength;
  }

  int blockSize = 64 - leadingZeros - trailingZeros;
  uint32_t expectedSize =
      kLeadingZerosLengthBits + kBlockSizeLengthBits + blockSize;
  uint32_t previousBlockInformationSize =
      64 - previousValueTrailingZeros_ - previousValueLeadingZeros_;

  if (leadingZeros >= previousValueLeadingZeros_ &&
      trailingZeros >= previousValueTrailingZeros_ &&
      previousBlockInformationSize < expectedSize) {
    // Control bit for using previous block information.
    BitUtil::addValueToBitString(1, 1, data_, numBits_);
    bitsforcompressvalue +=1;
    uint64_t blockValue = xorWithPrevius >> previousValueTrailingZeros_;
    BitUtil::addValueToBitString(
        blockValue, previousBlockInformationSize, data_, numBits_);
    bitsforcompressvalue +=previousBlockInformationSize;
    //add LOG(data_.length())
    if (data_.length() <16){
        //icompresscount=1;
    }
    LOG(INFO) << "Wrote compressed TimeValuePairValue: 64 bits  -> "<<bitsforcompressvalue<<" bits";
   //LOG(INFO) << "Wrote compressed TimeValuePairValue :"<<16*//icompresscount<<" Bytes  -> "<<data_.length()<<" Bytes";
    //// bitsforcompressvalue=0;

  } else {
    // Control bit for not using previous block information.
    BitUtil::addValueToBitString(0, 1, data_, numBits_);
    bitsforcompressvalue +=1;
    BitUtil::addValueToBitString(
        leadingZeros, kLeadingZerosLengthBits, data_, numBits_);
    bitsforcompressvalue +=kLeadingZerosLengthBits;
    BitUtil::addValueToBitString(
        // To fit in 6 bits. There will never be a zero size block
        blockSize - kBlockSizeAdjustment,
        kBlockSizeLengthBits,
        data_,
        numBits_);
    bitsforcompressvalue +=kBlockSizeLengthBits;

    uint64_t blockValue = xorWithPrevius >> trailingZeros;
    BitUtil::addValueToBitString(blockValue, blockSize, data_, numBits_);
    bitsforcompressvalue +=blockSize;
    previousValueTrailingZeros_ = trailingZeros;
    previousValueLeadingZeros_ = leadingZeros;
    LOG(INFO) << "Wrote compressed TimeValuePairValue: 64 bits  -> "<<bitsforcompressvalue<<" bits";
    // bitsforcompressvalue=0;
  }

  previousValue_ = *p;
}

int64_t TimeSeriesStream::readNextTimestamp(
    const char* data,
    uint64_t& bitPos,
    int64_t& prevValue,
    int64_t& prevDelta) {
  uint32_t type = BitUtil::findTheFirstZeroBit(data, bitPos, 4);
  if (type > 0) {
    // Delta of delta is non zero. Calculate the new delta. `index`
    // will be used to find the right length for the value that is
    // read.
    int index = type - 1;
    int64_t decodedValue = BitUtil::readValueFromBitString(
        data, bitPos, timestampEncodings[index].bitsForValue);

    // [0,255] becomes [-128,127]
    decodedValue -=
        ((int64_t)1 << (timestampEncodings[index].bitsForValue - 1));
    if (decodedValue >= 0) {
      // [-128,127] becomes [-128,128] without the zero in the middle
      decodedValue++;
    }

    prevDelta += decodedValue;
  }

  prevValue += prevDelta;
  return prevValue;
}

double TimeSeriesStream::readNextValue(
    const char* data,
    uint64_t& bitPos,
    uint64_t& previousValue,
    uint64_t& previousLeadingZeros,
    uint64_t& previousTrailingZeros) {
  uint64_t nonZeroValue = BitUtil::readValueFromBitString(data, bitPos, 1);

  if (!nonZeroValue) {
    double* p = (double*)&previousValue;
    return *p;
  }

  uint64_t usePreviousBlockInformation =
      BitUtil::readValueFromBitString(data, bitPos, 1);

  uint64_t xorValue;
  if (usePreviousBlockInformation) {
    xorValue = BitUtil::readValueFromBitString(
        data, bitPos, 64 - previousLeadingZeros - previousTrailingZeros);
    xorValue <<= previousTrailingZeros;
  } else {
    uint64_t leadingZeros =
        BitUtil::readValueFromBitString(data, bitPos, kLeadingZerosLengthBits);
    uint64_t blockSize =
        BitUtil::readValueFromBitString(data, bitPos, kBlockSizeLengthBits) +
        kBlockSizeAdjustment;

    previousTrailingZeros = 64 - blockSize - leadingZeros;
    xorValue = BitUtil::readValueFromBitString(data, bitPos, blockSize);
    xorValue <<= previousTrailingZeros;
    previousLeadingZeros = leadingZeros;
  }

  uint64_t value = xorValue ^ previousValue;
  previousValue = value;

  double* p = (double*)&value;
  return *p;
}

uint32_t TimeSeriesStream::getFirstTimeStamp() {
  if (data_.length() == 0) {
    return 0;
  }

  uint64_t bitPos = 0;
  return BitUtil::readValueFromBitString(
      data_.c_str(), bitPos, kBitsForFirstTimestamp);
}
}
} // facebook::gorilla
