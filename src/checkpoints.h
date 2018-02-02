// Copyright (c) 2009-2014 The Komodo Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KOMODO_CHECKPOINTS_H
#define KOMODO_CHECKPOINTS_H

#include "uint256.h"
#include <boost/assign/list_of.hpp>

#include <map>

class CBlockIndex;

/**
 * Block-chain checkpoints are compiled-in sanity checks.
 * They are updated every release or three.
 */
namespace Checkpoints
{
typedef std::map<int, uint256> MapCheckpoints;

struct CCheckpointData {
    MapCheckpoints mapCheckpoints;
    int64_t nTimeLastCheckpoint;
    int64_t nTransactionsLastCheckpoint;
    double fTransactionsPerDay;
};

bool CheckBlock(const CCheckpointData& data, int nHeight, const uint256& hash);

//! Return conservative estimate of total number of blocks, 0 if unknown
int GetTotalBlocksEstimate(const CCheckpointData& data);

//! Returns last CBlockIndex* in mapBlockIndex that is a checkpoint
CBlockIndex* GetLastCheckpoint(const CCheckpointData& data);

double GuessVerificationProgress(const CCheckpointData& data, CBlockIndex* pindex, bool fSigchecks = true);

} //namespace Checkpoints

#endif // KOMODO_CHECKPOINTS_H
