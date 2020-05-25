// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#include "primitives/block.h"

#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "crypto/common.h"
#include "komodo_defs.h"


// default hash algorithm for block
uint256 (CBlockHeader::*CBlockHeader::hashFunction)() const = &CBlockHeader::GetSHA256DHash;

uint256 CBlockHeader::GetSHA256DHash() const
{
    return SerializeHash(*this);
}

uint256 CBlockHeader::GetVerusHash() const
{
    if (hashPrevBlock.IsNull())
        // always use SHA256D for genesis block
        return SerializeHash(*this);
    else
        return SerializeVerusHash(*this);
}

uint256 CBlockHeader::GetVerusV2Hash() const
{
    if (hashPrevBlock.IsNull())
        // always use SHA256D for genesis block
        return SerializeHash(*this);
    else
        return SerializeVerusHashV2(*this);
}

void CBlockHeader::SetSHA256DHash()
{
    CBlockHeader::hashFunction = &CBlockHeader::GetSHA256DHash;
}

void CBlockHeader::SetVerusHash()
{
    CBlockHeader::hashFunction = &CBlockHeader::GetVerusHash;
}

void CBlockHeader::SetVerusHashV2()
{
    CBlockHeader::hashFunction = &CBlockHeader::GetVerusV2Hash;
}

// returns false if unable to fast calculate the VerusPOSHash from the header. 
// if it returns false, value is set to 0, but it can still be calculated from the full block
// in that case. the only difference between this and the POS hash for the contest is that it is not divided by the value out
// this is used as a source of entropy
bool CBlockHeader::GetRawVerusPOSHash(uint256 &ret, int32_t nHeight) const
{
    // if below the required height or no storage space in the solution, we can't get
    // a cached txid value to calculate the POSHash from the header
    if (!(CPOSNonce::NewNonceActive(nHeight) && IsVerusPOSBlock()))
    {
        ret = uint256();
        return false;
    }
    
    // if we can calculate, this assumes the protocol that the POSHash calculation is:
    //    hashWriter << ASSETCHAINS_MAGIC;
    //    hashWriter << nNonce; (nNonce is:
    //                           (high 128 bits == low 128 bits of verus hash of low 128 bits of nonce)
    //                           (low 32 bits == compact PoS difficult)
    //                           (mid 96 bits == low 96 bits of HASH(pastHash, txid, voutnum)
    //                              pastHash is hash of height - 100, either PoW hash of block or PoS hash, if new PoS
    //                          )
    //    hashWriter << height;
    //    return hashWriter.GetHash();
    CVerusHashWriter hashWriter = CVerusHashWriter(SER_GETHASH, PROTOCOL_VERSION);

    hashWriter << ASSETCHAINS_MAGIC;
    hashWriter << nNonce;
    hashWriter << nHeight;
    ret = hashWriter.GetHash();
    return true;
}

bool CBlockHeader::GetVerusPOSHash(arith_uint256 &ret, int32_t nHeight, CAmount value) const
{
    uint256 raw;
    if (GetRawVerusPOSHash(raw, nHeight))
    {
        ret = UintToArith256(raw) / value;
        return true;
    }
    return false;
}

// depending on the height of the block and its type, this returns the POS hash or the POW hash
uint256 CBlockHeader::GetVerusEntropyHash(int32_t height) const
{
    uint256 retVal;
    // if we qualify as PoW, use PoW hash, regardless of PoS state
    if (GetRawVerusPOSHash(retVal, height))
    {
        // POS hash
        return retVal;
    }
    return GetHash();
}

uint256 BuildMerkleTree(bool* fMutated, const std::vector<uint256> leaves,
        std::vector<uint256> &vMerkleTree)
{
    /* WARNING! If you're reading this because you're learning about crypto
       and/or designing a new system that will use merkle trees, keep in mind
       that the following merkle tree algorithm has a serious flaw related to
       duplicate txids, resulting in a vulnerability (CVE-2012-2459).

       The reason is that if the number of hashes in the list at a given time
       is odd, the last one is duplicated before computing the next level (which
       is unusual in Merkle trees). This results in certain sequences of
       transactions leading to the same merkle root. For example, these two
       trees:

                   A                A
                 /  \            /    \
                B    C          B       C
               / \    \        / \     / \
              D   E   F       D   E   F   F
             / \ / \ / \     / \ / \ / \ / \
             1 2 3 4 5 6     1 2 3 4 5 6 5 6

       for transaction lists [1,2,3,4,5,6] and [1,2,3,4,5,6,5,6] (where 5 and
       6 are repeated) result in the same root hash A (because the hash of both
       of (F) and (F,F) is C).

       The vulnerability results from being able to send a block with such a
       transaction list, with the same merkle root, and the same block hash as
       the original without duplication, resulting in failed validation. If the
       receiving node proceeds to mark that block as permanently invalid
       however, it will fail to accept further unmodified (and thus potentially
       valid) versions of the same block. We defend against this by detecting
       the case where we would hash two identical hashes at the end of the list
       together, and treating that identically to the block having an invalid
       merkle root. Assuming no double-SHA256 collisions, this will detect all
       known ways of changing the transactions without affecting the merkle
       root.
    */

    vMerkleTree.clear();
    vMerkleTree.reserve(leaves.size() * 2 + 16); // Safe upper bound for the number of total nodes.
    for (std::vector<uint256>::const_iterator it(leaves.begin()); it != leaves.end(); ++it)
        vMerkleTree.push_back(*it);
    int j = 0;
    bool mutated = false;
    for (int nSize = leaves.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        for (int i = 0; i < nSize; i += 2)
        {
            int i2 = std::min(i+1, nSize-1);
            if (i2 == i + 1 && i2 + 1 == nSize && vMerkleTree[j+i] == vMerkleTree[j+i2]) {
                // Two identical hashes at the end of the list at a particular level.
                mutated = true;
            }
            vMerkleTree.push_back(Hash(BEGIN(vMerkleTree[j+i]),  END(vMerkleTree[j+i]),
                                       BEGIN(vMerkleTree[j+i2]), END(vMerkleTree[j+i2])));
        }
        j += nSize;
    }
    if (fMutated) {
        *fMutated = mutated;
    }
    return (vMerkleTree.empty() ? uint256() : vMerkleTree.back());
}


uint256 CBlock::BuildMerkleTree(bool* fMutated) const
{
    std::vector<uint256> leaves;
    for (int i=0; i<vtx.size(); i++) leaves.push_back(vtx[i].GetHash());
    return ::BuildMerkleTree(fMutated, leaves, vMerkleTree);
}


std::vector<uint256> GetMerkleBranch(int nIndex, int nLeaves, const std::vector<uint256> &vMerkleTree)
{
    std::vector<uint256> vMerkleBranch;
    int j = 0;
    for (int nSize = nLeaves; nSize > 1; nSize = (nSize + 1) / 2)
    {
        int i = std::min(nIndex^1, nSize-1);
        vMerkleBranch.push_back(vMerkleTree[j+i]);
        nIndex >>= 1;
        j += nSize;
    }
    return vMerkleBranch;
}


std::vector<uint256> CBlock::GetMerkleBranch(int nIndex) const
{
    if (vMerkleTree.empty())
        BuildMerkleTree();
    return ::GetMerkleBranch(nIndex, vtx.size(), vMerkleTree);
}


uint256 CBlock::CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex)
{
    if (nIndex == -1)
        return uint256();
    for (std::vector<uint256>::const_iterator it(vMerkleBranch.begin()); it != vMerkleBranch.end(); ++it)
    {
        if (nIndex & 1)
            hash = Hash(BEGIN(*it), END(*it), BEGIN(hash), END(hash));
        else
            hash = Hash(BEGIN(hash), END(hash), BEGIN(*it), END(*it));
        nIndex >>= 1;
    }
    return hash;
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, hashFinalSaplingRoot=%s, nTime=%u, nBits=%08x, nNonce=%s, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        hashFinalSaplingRoot.ToString(),
        nTime, nBits, nNonce.ToString(),
        vtx.size());
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        s << "  " << vtx[i].ToString() << "\n";
    }
    s << "  vMerkleTree: ";
    for (unsigned int i = 0; i < vMerkleTree.size(); i++)
        s << " " << vMerkleTree[i].ToString();
    s << "\n";
    return s.str();
}
