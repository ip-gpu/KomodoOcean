// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Komodo Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KOMODO_CONSENSUS_CONSENSUS_H
#define KOMODO_CONSENSUS_CONSENSUS_H

/** The minimum allowed block version (network rule) */
static const long MIN_BLOCK_VERSION = 4;
/** The minimum allowed transaction version (network rule) */
static const long MIN_TX_VERSION = 1;
/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MAX_BLOCK_SIZE = 2000000;
/** The maximum allowed number of signature check operations in a block (network rule) */
static const unsigned int MAX_BLOCK_SIGOPS = 20000;
/** The maximum allowed number of signature check operations in a block (network rule) */
static const long long MAX_BLOCK_SIGOPS_COST = 80000;
/** The maximum size of a transaction (network rule) */
static const unsigned int MAX_TX_SIZE = 100000;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
extern int COINBASE_MATURITY;

/** Flags for nSequence and nLockTime locks */
enum {
    /* Interpret sequence numbers as relative lock-time constraints. */
    LOCKTIME_VERIFY_SEQUENCE = (1 << 0),

    /* Use GetMedianTimePast() instead of nTime for end point timestamp. */
    LOCKTIME_MEDIAN_TIME_PAST = (1 << 1),
};

/** Used as the flags parameter to CheckFinalTx() in non-consensus code */
static const unsigned int STANDARD_LOCKTIME_VERIFY_FLAGS = LOCKTIME_MEDIAN_TIME_PAST;

/** The maximum allowed weight for a block, see BIP 141 (network rule) */
static const unsigned int MAX_BLOCK_WEIGHT = 4000000;
static const int WITNESS_SCALE_FACTOR = 4;
static const size_t MIN_SERIALIZABLE_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 10; // 10 is the lower bound for the size of a serialized CTransaction

#endif // KOMODO_CONSENSUS_CONSENSUS_H
