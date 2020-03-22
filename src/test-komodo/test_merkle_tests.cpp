#include <gtest/gtest.h>

// #include "chainparamsbase.h"
// #include "fs.h"
// #include "key.h"
// #include "pubkey.h"
// #include "random.h"
// #include "scheduler.h"
// #include "txdb.h"
// #include "txmempool.h"

// #include <memory>
// #include <type_traits>

#include "uint256.h"
#include "hash.h"
#include "primitives/block.h"
#include "consensus/merkle.h"
#include "random.h"
#include "core_io.h"

namespace TestMerkleTests {

    class TestMerkleTests : public ::testing::Test {};

    static uint256 ComputeMerkleRootFromBranch(const uint256& leaf, const std::vector<uint256>& vMerkleBranch, uint32_t nIndex) {
        uint256 hash = leaf;
        for (std::vector<uint256>::const_iterator it = vMerkleBranch.begin(); it != vMerkleBranch.end(); ++it) {
            if (nIndex & 1) {
                hash = Hash(it->begin(), it->end(), hash.begin(), hash.end());
            } else {
                hash = Hash(hash.begin(), hash.end(), it->begin(), it->end());
            }
            nIndex >>= 1;
        }
        return hash;
    }

    /* This implements a constant-space merkle root/path calculator, limited to 2^32 leaves. */
    static void MerkleComputation(const std::vector<uint256>& leaves, uint256* proot, bool* pmutated, uint32_t branchpos, std::vector<uint256>* pbranch) {
        if (pbranch) pbranch->clear();
        if (leaves.size() == 0) {
            if (pmutated) *pmutated = false;
            if (proot) *proot = uint256();
            return;
        }
        bool mutated = false;
        // count is the number of leaves processed so far.
        uint32_t count = 0;
        // inner is an array of eagerly computed subtree hashes, indexed by tree
        // level (0 being the leaves).
        // For example, when count is 25 (11001 in binary), inner[4] is the hash of
        // the first 16 leaves, inner[3] of the next 8 leaves, and inner[0] equal to
        // the last leaf. The other inner entries are undefined.
        uint256 inner[32];
        // Which position in inner is a hash that depends on the matching leaf.
        int matchlevel = -1;
        // First process all leaves into 'inner' values.
        while (count < leaves.size()) {
            uint256 h = leaves[count];
            bool matchh = count == branchpos;
            count++;
            int level;
            // For each of the lower bits in count that are 0, do 1 step. Each
            // corresponds to an inner value that existed before processing the
            // current leaf, and each needs a hash to combine it.
            for (level = 0; !(count & (((uint32_t)1) << level)); level++) {
                if (pbranch) {
                    if (matchh) {
                        pbranch->push_back(inner[level]);
                    } else if (matchlevel == level) {
                        pbranch->push_back(h);
                        matchh = true;
                    }
                }
                mutated |= (inner[level] == h);
                CHash256().Write(inner[level].begin(), 32).Write(h.begin(), 32).Finalize(h.begin());
            }
            // Store the resulting hash at inner position level.
            inner[level] = h;
            if (matchh) {
                matchlevel = level;
            }
        }
        // Do a final 'sweep' over the rightmost branch of the tree to process
        // odd levels, and reduce everything to a single top value.
        // Level is the level (counted from the bottom) up to which we've sweeped.
        int level = 0;
        // As long as bit number level in count is zero, skip it. It means there
        // is nothing left at this level.
        while (!(count & (((uint32_t)1) << level))) {
            level++;
        }
        uint256 h = inner[level];
        bool matchh = matchlevel == level;
        while (count != (((uint32_t)1) << level)) {
            // If we reach this point, h is an inner value that is not the top.
            // We combine it with itself (Bitcoin's special rule for odd levels in
            // the tree) to produce a higher level one.
            if (pbranch && matchh) {
                pbranch->push_back(h);
            }
            CHash256().Write(h.begin(), 32).Write(h.begin(), 32).Finalize(h.begin());
            // Increment count to the value it would have if two entries at this
            // level had existed.
            count += (((uint32_t)1) << level);
            level++;
            // And propagate the result upwards accordingly.
            while (!(count & (((uint32_t)1) << level))) {
                if (pbranch) {
                    if (matchh) {
                        pbranch->push_back(inner[level]);
                    } else if (matchlevel == level) {
                        pbranch->push_back(h);
                        matchh = true;
                    }
                }
                CHash256().Write(inner[level].begin(), 32).Write(h.begin(), 32).Finalize(h.begin());
                level++;
            }
        }
        // Return result.
        if (pmutated) *pmutated = mutated;
        if (proot) *proot = h;
    }

    static std::vector<uint256> ComputeMerkleBranch(const std::vector<uint256>& leaves, uint32_t position) {
        std::vector<uint256> ret;
        MerkleComputation(leaves, nullptr, nullptr, position, &ret);
        return ret;
    }

    static std::vector<uint256> BlockMerkleBranch(const CBlock& block, uint32_t position)
    {
        std::vector<uint256> leaves;
        leaves.resize(block.vtx.size());
        for (size_t s = 0; s < block.vtx.size(); s++) {
            //leaves[s] = block.vtx[s]->GetHash();
            leaves[s] = block.vtx[s].GetHash();
        }
        return ComputeMerkleBranch(leaves, position);
    }

    // Older version of the merkle root computation code, for comparison.
    static uint256 BlockBuildMerkleTree(const CBlock& block, bool* fMutated, std::vector<uint256>& vMerkleTree)
    {
        vMerkleTree.clear();
        vMerkleTree.reserve(block.vtx.size() * 2 + 16); // Safe upper bound for the number of total nodes.
        //for (std::vector<CTransactionRef>::const_iterator it(block.vtx.begin()); it != block.vtx.end(); ++it)
        for (std::vector<CTransaction>::const_iterator it(block.vtx.begin()); it != block.vtx.end(); ++it) {
            // vMerkleTree.push_back((*it)->GetHash());
            vMerkleTree.push_back((*it).GetHash());
        }

        int j = 0;
        bool mutated = false;
        for (int nSize = block.vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
        {
            for (int i = 0; i < nSize; i += 2)
            {
                int i2 = std::min(i+1, nSize-1);
                if (i2 == i + 1 && i2 + 1 == nSize && vMerkleTree[j+i] == vMerkleTree[j+i2]) {
                    // Two identical hashes at the end of the list at a particular level.
                    mutated = true;
                }
                vMerkleTree.push_back(Hash(vMerkleTree[j+i].begin(), vMerkleTree[j+i].end(),
                                        vMerkleTree[j+i2].begin(), vMerkleTree[j+i2].end()));
            }
            j += nSize;
        }
        if (fMutated) {
            *fMutated = mutated;
        }
        return (vMerkleTree.empty() ? uint256() : vMerkleTree.back());
    }

    // Older version of the merkle branch computation code, for comparison.
    static std::vector<uint256> BlockGetMerkleBranch(const CBlock& block, const std::vector<uint256>& vMerkleTree, int nIndex)
    {
        std::vector<uint256> vMerkleBranch;
        int j = 0;
        for (int nSize = block.vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
        {
            int i = std::min(nIndex^1, nSize-1);
            vMerkleBranch.push_back(vMerkleTree[j+i]);
            nIndex >>= 1;
            j += nSize;
        }
        return vMerkleBranch;
    }

    static inline int ctz(uint32_t i) {
        if (i == 0) return 0;
        int j = 0;
        while (!(i & 1)) {
            j++;
            i >>= 1;
        }
        return j;
    }

    TEST(TestMerkleTests, merkle_test) {
    // standart bitcoin test from merkle_tests.cpp

        for (int i = 0; i < 32; i++) {
        // Try 32 block sizes: all sizes from 0 to 16 inclusive, and then 15 random sizes.
        int ntx = (i <= 16) ? i : 17 + ( GetRand(4000) ); // InsecureRandRange(4000)
        // Try up to 3 mutations.
        for (int mutate = 0; mutate <= 3; mutate++) {
            int duplicate1 = mutate >= 1 ? 1 << ctz(ntx) : 0; // The last how many transactions to duplicate first.
            if (duplicate1 >= ntx) break; // Duplication of the entire tree results in a different root (it adds a level).
            int ntx1 = ntx + duplicate1; // The resulting number of transactions after the first duplication.
            int duplicate2 = mutate >= 2 ? 1 << ctz(ntx1) : 0; // Likewise for the second mutation.
            if (duplicate2 >= ntx1) break;
            int ntx2 = ntx1 + duplicate2;
            int duplicate3 = mutate >= 3 ? 1 << ctz(ntx2) : 0; // And for the third mutation.
            if (duplicate3 >= ntx2) break;
            int ntx3 = ntx2 + duplicate3;
            // Build a block with ntx different transactions.
            CBlock block;
            block.vtx.resize(ntx);
            for (int j = 0; j < ntx; j++) {
                CMutableTransaction mtx;
                mtx.nLockTime = j;
                //block.vtx[j] = MakeTransactionRef(std::move(mtx));
                block.vtx[j] = mtx;
            }

            // Compute the root of the block before mutating it.
            bool unmutatedMutated = false;
            uint256 unmutatedRoot = BlockMerkleRoot(block, &unmutatedMutated);
            ASSERT_TRUE(unmutatedMutated == false);
            // Optionally mutate by duplicating the last transactions, resulting in the same merkle root.
            block.vtx.resize(ntx3);
            for (int j = 0; j < duplicate1; j++) {
                block.vtx[ntx + j] = block.vtx[ntx + j - duplicate1];
            }
            for (int j = 0; j < duplicate2; j++) {
                block.vtx[ntx1 + j] = block.vtx[ntx1 + j - duplicate2];
            }
            for (int j = 0; j < duplicate3; j++) {
                block.vtx[ntx2 + j] = block.vtx[ntx2 + j - duplicate3];
            }
            // Compute the merkle root and merkle tree using the old mechanism.
            bool oldMutated = false;
            std::vector<uint256> merkleTree;
            uint256 oldRoot = BlockBuildMerkleTree(block, &oldMutated, merkleTree);
            // Compute the merkle root using the new mechanism.
            bool newMutated = false;
            uint256 newRoot = BlockMerkleRoot(block, &newMutated);
            ASSERT_TRUE(oldRoot == newRoot);
            ASSERT_TRUE(newRoot == unmutatedRoot);
            ASSERT_TRUE((newRoot == uint256()) == (ntx == 0));
            ASSERT_TRUE(oldMutated == newMutated);
            ASSERT_TRUE(newMutated == !!mutate);
            // If no mutation was done (once for every ntx value), try up to 16 branches.
            if (mutate == 0) {
                for (int loop = 0; loop < std::min(ntx, 16); loop++) {
                    // If ntx <= 16, try all branches. Otherwise, try 16 random ones.
                    int mtx = loop;
                    if (ntx > 16) {
                        mtx = GetRand(mtx); // InsecureRandRange(ntx)
                    }
                    std::vector<uint256> newBranch = BlockMerkleBranch(block, mtx);
                    std::vector<uint256> oldBranch = BlockGetMerkleBranch(block, merkleTree, mtx);
                    ASSERT_TRUE(oldBranch == newBranch);
                    //BOOST_CHECK(ComputeMerkleRootFromBranch(block.vtx[mtx]->GetHash(), newBranch, mtx) == oldRoot);
                    ASSERT_TRUE(ComputeMerkleRootFromBranch(block.vtx[mtx].GetHash(), newBranch, mtx) == oldRoot);
                    }
                }
            }
        }
    }

    TEST(TestMerkleTests, merkle_real_one) {
        CBlock b;
        // Block #1795885
        std::string strHexBlock = "040000009db54b2301a7a2c3cc9ba11595f38bc72c635c2b10ea3e1d88170de7c65bec06c4214e1c5767ab99d69c50581b0edb506c5c3ca2b95ebb9e6c8e4147d9ec630ba2d8a734eb73a4dc734072dbfd12406f1e7121bfe0e3d6c10922495c44e5cc1c2422725ea8f7001d0a00f67390367e032efd0eb7ac9a410d4f4bea82cf759e5b02ef85bd714c0000fd40050014d0c2e1c56c7522655035ed399ef8b11577fbe00b03077d075779fcdfc6428240e2bc314ff897c82c0ab1518b5e04c219ff8b920185ef2ae51fcbd1c4c8463617f97fa89cb34d0c27a5b6e0870e41d27de7820474b2f826dc5f85374f005cc7e3b53429ae0359f350cac4d204d9126fa0f3979502da9d1209249e1e452f581aa749e24a9948c9384996e08aceb4af9efab28f7e24af056acc0dfd756a92205b0e1b0a01fcace40132d20ab675e7f9d01e22f247d53b5e68c1def058306a14522812e82cb2cf24dbc0b851d953095adfba014e819b5cca96594ccdd049a19631ba0590f9d672071203fe735208f51dcf43bf857188620a95bb1d0d0769024ec072a187a22ac2b73445274e155e17cbc1186a5ac9890b4d90ff5a658373be55070ad03fb24a0ec88b5be890bfa1dc896689b7bb3a1db3a66e6f1b681ca53de75c7904f295b7ed4cd0804eac5b59e378009c3f7680501d1ae52b046dbe46b7cd3e8a3f8d490ad0a687eb9918b153f160c9deb1371500096b2bc007ddd2b365644529315e00c1b793372274ff5ad68e2584c5075792cb3d1e49077cc349f5fe961379a2f602696a8e451310e3b01e3345b6cad1d1b357f7caea077a0d3f72591027e749027a52f912462a3f7231f1054f38d8ec9ff8c9352772a6aea1928515bf1b801e1c7909e0cc26e46f46753a048deabaa384397e187d00cc2d7678a2b1dbb5e232bc66c2ea6cd0e852b18a1596cbc2e988607519dab2ea437797f6180c9546f203b1256d06e36d9d5388b1d2bc1ef54d150c1acbfc1ccc47ab81fa0adbd8e3340094f8c782a0e17c8359021b284ff2c373ac1c0e31541511b53263be54c9c621a4a2aec9a4f01da52ae2ac1f95b2ef46fefdb1290cdb8c470035e363bc9eb33ca9aefc61dbf07f268735b703577f900221894b95303c422181e5fdef401b001cef9e9b1df991de14230371fea7ad5082f885901c4dba0dd896f37ec38a323f9a62fcf60b9ade79541024b3b285ded1a54695513b71924690741d3f4bda2cb86691aa691abbd201144294d5656a8266571177041a56a1b602d3af648890f71934e508c617e98ff72636beda5d29c73776c965358be503d6d84d976d0b0b4a267c554e5b9dabbab0df14589f44544fb6c7194a325eb6a7192c23efc785041d4f4776141d72525e023833e3654e85b1521253fc49b6a60b693cffa3d204819acdcce29689c92742acbcbf0908e0d4e982a3076fd0593810e88dbc7275313a563b1726a6df3bd22126dfe1f36fb6ebc91a955808ccbe63d01afedac11dee75316228c081b2811471a357f1d6c0275956be306a6d55032b6539e95805032973ead9e5f0feb7031fefd77cef91a85ee200720051f5b4148ef58b75a1317fc7de495b48d8fd1c6a5e39d49872d9827f3f1b00c574e59307008f8d2540efb326ae81f5439d924f1fb187189ed6d5cf886762c21bfb1950e17cdace422aa5448cf48cce7514ae341ab9ecf9c1aa291912f632c7ebf55d93ba51747f338cb5da2b89e4425704c10d8c70b4f00ea741bbf7e5b9ebb012d27c68770353340401c62f326bcb9a0ca5dc89d74ac9bf8d78b6a2196f916a3f12cab2b2783524c2bd9bee22adffc18e1ace3c5edbb155ffec51190b8b731eb67aefba1d1f03a0ef35a20ed34f6bbe7364aca8118708f1dffed807381304691708796609a18f8dbe4a7cb6957bfde023ae662e83d23cf3d4c1428b94ed781aa07e7b3def50a2ded3b2eefe13f7e1b718445ec72a415fba507b090bcf8d74dec46d7970f143abf116fddc4c581fe50af58d0ac12b1f2b811843df5a54c5b2e21c3a618c1edd2c4499d0da1ad69c6630fcc7946665187808c431b0ace7c624036da2ec53878b7d7bccfb0aeca54a040400008085202f89010000000000000000000000000000000000000000000000000000000000000000ffffffff06032d671b0101ffffffff015034e21100000000232103ffdf1a116300a78729608d9930742cd349f11a9d64fcc336b8f18592dd9c91bcace721725e0000000000000000000000000000000400008085202f89011564d4cca16e344f52d12990586f27be394a1a96e353eaa44630de23b6d82bf4010000006a47304402204f5ddd47966b572f84ebc9c83b67355c5337eb6d9f11b7d0a0e0d26d3c954f7602201c9518d5859a51eb7c7ee588d5dc8f1fe60e6b23a63f16a528b71b978f531cda0121023e7363600c74eab77f6049339767660988936f55aac603d8f9a424d61089e012ffffffff025bda0600000000001976a914ca1e04745e8ca0c60d8c5881531d51bec470743f88ac0c07d004000000001976a914dbdfc4cefcf7fa0fa0c40270eb95ad6abb2ee35c88ace519725e0000000000000000000000000000000400008085202f890d27f51e72e562595212f9a92e9cfe5904b02f2d30bf16a006a55f8f595cf8fc2207000000494830450221009df689a80276a165bdddda3c10bc159b73f2b7fbe43b356fb36399e40121cbf0022032021d3c0e29ad562f33be3ccc2db1a7a9bfa214d69a0d0933bc43bb7fb5628901ffffffff0bda2bf0036db1367a0e03200a013d7487ed3f0f9b97f6a01ad56230b89526c61900000049483045022100c31d8504468aef7def59bfb58f5cd41c9261b7dd5cce4fc3687553bd183fb32c02207001e0883d6f0e73b2fd662c822ad21139bd5b19681cad27f2f48ceab67e78af01ffffffff632a9fb8a154e8e6019aad2367ddf03151659f844325b3eae7f4cc60f55c30810e00000049483045022100ffd197a2e05ae73f2073762d9530de765927e32af3697af0ce493b561737ba1102204d2b190d222dfd0e93689047e0e220f48a5a5e44f3a2762b56bab337b7f50c4f01fffffffff6800e723bba635aeaa4d3111bd4ae6ef8d01d98a95c4e2a1aaae8c2b11846ec3400000049483045022100e5f2a94e10c2f6d92b3ff48ae1582f0c0244e4ab4d141672c9d25bc86d2e8ad0022071d850fdb13816546e26c5f672fb9af4b4aa6a158a62a9a7ccf986b61096c5d601ffffffff638f262670789a20f561926bf1af50939dc1772af0f97c8be606e1dd5889105e0a000000494830450221008ae752a726cc1cf0fcb7127d73b87da31d02bf637cf943600a70c6d9a6983eb3022058c71a9219cfd4e5dcf644f3d22c7ad2cdee9863adcf1019338c2312fb2564ff01ffffffffcdd6056065b5f1764c0b7152432a7cac431b06be5ea530478469c865733f55f10a0000004847304402202ae8b2112c61c08d2739f1ff706cabbf93a10e563de9e229e75d699c95697fad02203be2568a811ffaa569d2469a2ed623dea3a0e45494733c38c90ed2b3179cc6de01ffffffff0ada9ffbbf628f1086ea873b004c0caea793aafa2d0959bc1ffa5f8b0e6eb493160000004847304402206c25273b9a6ddc386bc45fa045d3ddefe3dd582756183a1f3949597c8d5b24a602203b1247be908c717e9b0e7ce5add789fd27769de7482f95656bcb791188ad84d201ffffffff399bd17a83e419b5cd3f9685b7c768eea6483142d99f684d2572f2c32c9ea7d30000000049483045022100a0c744090ea03ad8b49e1aa7bc117eb88ddeb3cc357b47120cbf0a63bcefc934022074e60140c2704c62b6e23b3640a58567cdb48c169df82215646789fc5fcfcbbd01ffffffffee20f6d4c372ac04de49e9e91a12bf4ae2039b80c70e9c2351276908bc1f30761700000049483045022100e928f5c15fbf691b8b85aef0b322578701aafa5c8e88ba6273d139f46c31fbe302203e87797d1c9ff013ffadf174e27107ba584cfd10d5d7a52a3d5fe37b9ae800c001ffffffff9dbab35ce824b1cda10fd42ca2039970ff1c521ee0640b1b88d48811c19cecd82c00000049483045022100e00ed0eeaeef2df20aa6818efcc936af83514416e41ca0cf9631b1e4502e68190220711e0529190ddec4a51593446ec0f6496ce1bf7a43b09dccb71ab32e731b9c3401ffffffff6ea9427838bf315dd359925dcbddb57762a114e95ad7d2ff24a1985ccb16cedb1300000049483045022100daec766e4a578944d5e0df4f25767671cb5ade2b7b8d58346367d2da22f70f31022049b167ef75a9fe034efb0f09e9504147fc8482fedcc92c1a006869ec4c21bab301ffffffff4382e822aa22a632f252b08ab4723d739d01f79b40191cc314d9487efbd56d7a1d00000049483045022100fdda851870b0008fe1f45a9f6754a16320d825363075e21cde6a81410a6c44be02207accaa8ad8ed5a250646c0b50859247e0755acfc161b00d90b3f73904d64411201ffffffffa3d918bde33d20be628bfaaf8c5c354633b5d0dfed637f0f6160e95d9d6d19301b00000049483045022100f1493d37f036bed1d7fbdbb8c82d07a8bae296abaef718736221181b46932ef80220496ffc0a5c68f37d35b115a0db242de55a759100acbf5051ebe7b5317d799b2701ffffffff02f0810100000000002321020e46e79a2a8d12b9b5d12c7a91adb4e454edfae43c0a0cb805427d2ac7613fd9ac00000000000000004f6a4c4cbf2febf989888339fd19cc326553bfa506d6d97eeb4f678c95042d292b3c7e0e68710500544843006e99f983dc132bb584ef77ecbccc012b8b398b6209f1dfdca5098b7668fde98008000200000000000000000000000000000000000000000400008085202f89015b742fe720038fb5d4ffa67d712074f8f06027e635dcfd28593c3927f6f8a22a0c00000049483045022100c9ba5302eff4dc788eeaf56410cb111fe2b1a1287bab330cbdcf77a582f468e202205f84ba5db019de8c1b80e0e0b3ce15e6143123454240e261c663ba668c5c4a9501ffffffff0288130000000000002321020e46e79a2a8d12b9b5d12c7a91adb4e454edfae43c0a0cb805427d2ac7613fd9ac0000000000000000226a20afd3070729dad0191ff74b26e072eacd2382248047f8f4428cda690a67389c802422725ef5671b000000000000000000000000";
        std::string strHashMerkleRoot = "0b63ecd947418e6c9ebb5eb9a23c5c6c50db0e1b58509cd699ab67571c4e21c4";

        if (!DecodeHexBlk(b, strHexBlock)) {
            ASSERT_TRUE(false);
        }

        uint256 hashMerkleRoot, hashMerkleRoot1, hashMerkleRoot2, hashMerkleRoot3, hashMerkleRoot4;
        bool mutated;

        // using old implementation directly from primitives/block.h
        hashMerkleRoot1 = b.BuildMerkleTree(&mutated);
        // using new implementation in consensus/merkle.h
        hashMerkleRoot2 = BlockMerkleRoot(b, &mutated);

        // old version of merkleroot computation code from this
        std::vector<uint256> leaves;
        std::vector<uint256> vMerkleTree;
        for (int i=0; i<b.vtx.size(); i++) leaves.push_back(b.vtx[i].GetHash());
        vMerkleTree.clear();
        vMerkleTree.reserve(leaves.size() * 2 + 16);
        for (std::vector<uint256>::const_iterator it(leaves.begin()); it != leaves.end(); ++it)
            vMerkleTree.push_back(*it);
        hashMerkleRoot3 = BlockBuildMerkleTree(b, &mutated, vMerkleTree);

        hashMerkleRoot = uint256S(strHashMerkleRoot);

        /* std::cerr << hashMerkleRoot.ToString() << std::endl;
        std::cerr << hashMerkleRoot1.ToString() << std::endl;
        std::cerr << hashMerkleRoot2.ToString() << std::endl;
        std::cerr << hashMerkleRoot3.ToString() << std::endl; */

        ASSERT_EQ(hashMerkleRoot, hashMerkleRoot1);
        ASSERT_EQ(hashMerkleRoot, hashMerkleRoot2);
        ASSERT_EQ(hashMerkleRoot, hashMerkleRoot3);

    }
}