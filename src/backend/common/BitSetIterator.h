// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BACKEND_COMMON_BITSETITERATOR_H_
#define BACKEND_COMMON_BITSETITERATOR_H_

#include "Forward.h"
#include "Math.h"

#include <bitset>
#include <limits>

// This is ANGLE's BitSetIterator class with a customizable return type
// TODO(cwallez@chromium.org): it could be optimized, in particular when N <= 64

namespace backend {

    template <typename T>
    T roundUp(const T value, const T alignment) {
        auto temp = value + alignment - static_cast<T>(1);
        return temp - temp % alignment;
    }

    template <size_t N, typename T>
    class BitSetIterator final {
        public:
            BitSetIterator(const std::bitset<N>& bitset);
            BitSetIterator(const BitSetIterator& other);
            BitSetIterator &operator=(const BitSetIterator& other);

            class Iterator final {
                public:
                    Iterator(const std::bitset<N>& bits);
                    Iterator& operator++();

                    bool operator==(const Iterator& other) const;
                    bool operator!=(const Iterator& other) const;
                    T operator*() const { return static_cast<T>(mCurrentBit); }

                private:
                    unsigned long getNextBit();

                    static const size_t BitsPerWord = sizeof(unsigned long) * 8;
                    std::bitset<N> mBits;
                    unsigned long mCurrentBit;
                    unsigned long mOffset;
            };

            Iterator begin() const { return Iterator(mBits); }
            Iterator end() const { return Iterator(std::bitset<N>(0)); }

        private:
            const std::bitset<N> mBits;
    };

    template <size_t N, typename T>
    BitSetIterator<N, T>::BitSetIterator(const std::bitset<N>& bitset)
        : mBits(bitset) {
    }

    template <size_t N, typename T>
    BitSetIterator<N, T>::BitSetIterator(const BitSetIterator& other)
        : mBits(other.mBits) {
    }

    template <size_t N, typename T>
    BitSetIterator<N, T>& BitSetIterator<N, T>::operator=(const BitSetIterator& other) {
        mBits = other.mBits;
        return *this;
    }

    template <size_t N, typename T>
    BitSetIterator<N, T>::Iterator::Iterator(const std::bitset<N>& bits)
        : mBits(bits), mCurrentBit(0), mOffset(0) {
        if (bits.any()) {
            mCurrentBit = getNextBit();
        } else {
            mOffset = static_cast<unsigned long>(roundUp(N, BitsPerWord));
        }
    }

    template <size_t N, typename T>
    typename BitSetIterator<N, T>::Iterator& BitSetIterator<N, T>::Iterator::operator++() {
        ASSERT(mBits.any());
        mBits.set(mCurrentBit - mOffset, 0);
        mCurrentBit = getNextBit();
        return *this;
    }

    template <size_t N, typename T>
    bool BitSetIterator<N, T>::Iterator::operator==(const Iterator& other) const {
        return mOffset == other.mOffset && mBits == other.mBits;
    }

    template <size_t N, typename T>
    bool BitSetIterator<N, T>::Iterator::operator!=(const Iterator& other) const {
        return !(*this == other);
    }

    template <size_t N, typename T>
    unsigned long BitSetIterator<N, T>::Iterator::getNextBit() {
        static std::bitset<N> wordMask(std::numeric_limits<unsigned long>::max());

        while (mOffset < N) {
            unsigned long wordBits = (mBits & wordMask).to_ulong();
            if (wordBits != 0ul) {
                return ScanForward(wordBits) + mOffset;
            }

            mBits >>= BitsPerWord;
            mOffset += BitsPerWord;
        }
        return 0;
    }

    // Helper to avoid needing to specify the template parameter size
    template <size_t N>
    BitSetIterator<N, uint32_t> IterateBitSet(const std::bitset<N>& bitset) {
        return BitSetIterator<N, uint32_t>(bitset);
    }

}

#endif  // BACKEND_COMMON_BITSETITERATOR_H_
