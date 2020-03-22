#include <gtest/gtest.h>
#include "consensus/upgrades.h"
#include "key.h"
#include "keystore.h"
#include "policy/policy.h"
#include "script/script.h"
#include "script/script_error.h"
#include "script/interpreter.h"
#include "script/sign.h"
#include "wallet/wallet_ismine.h"
#include "uint256.h"


namespace TestMultiSigTests {

    CScript sign_multisig(CScript scriptPubKey, std::vector<CKey> keys, CTransaction transaction, int whichIn, uint32_t consensusBranchId)
    {
        uint256 hash = SignatureHash(scriptPubKey, transaction, whichIn, SIGHASH_ALL, 0, consensusBranchId);

        CScript result;
        result << OP_0; // CHECKMULTISIG bug workaround
        for(const CKey &key : keys)
        {
            std::vector<unsigned char> vchSig;
            EXPECT_TRUE(key.Sign(hash, vchSig));
            vchSig.push_back((unsigned char)SIGHASH_ALL);
            result << vchSig;
        }
        return result;
    }

    TEST(TestMultiSigTests , multisig_IsStandard) {

        CKey key[10];
        for (int i = 0; i < 10; i++)
            key[i].MakeNewKey(true);

        txnouttype whichType;

        CScript a_and_b;
        a_and_b << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
        ASSERT_TRUE(::IsStandard(a_and_b, whichType));

        CScript a_or_b;
        a_or_b  << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
        ASSERT_TRUE(::IsStandard(a_or_b, whichType));

        CScript escrow;
        escrow << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;
        ASSERT_TRUE(::IsStandard(escrow, whichType));

        // Komodo supported up to x-of-9 multisig txns as standard (ZCash only x-of-3)

        CScript one_of_four;
        one_of_four << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << ToByteVector(key[3].GetPubKey()) << OP_4 << OP_CHECKMULTISIG;
        ASSERT_TRUE(::IsStandard(one_of_four, whichType));

        CScript one_of_nine;
        one_of_nine << OP_1;
        for (int i = 0; i < 9; i++) one_of_nine << ToByteVector(key[i].GetPubKey()); 
        one_of_nine << OP_9 << OP_CHECKMULTISIG;
        ASSERT_TRUE(::IsStandard(one_of_nine, whichType));

        CScript one_of_ten;
        one_of_ten << OP_1;
        for (int i = 0; i < 9; i++) one_of_ten << ToByteVector(key[i].GetPubKey()); 
        one_of_nine << OP_10 << OP_CHECKMULTISIG;
        ASSERT_TRUE(!::IsStandard(one_of_ten, whichType));

        CScript malformed[6];
        malformed[0] << OP_3 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
        malformed[1] << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;
        malformed[2] << OP_0 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
        malformed[3] << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_0 << OP_CHECKMULTISIG;
        malformed[4] << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_CHECKMULTISIG;
        malformed[5] << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey());

        for (int i = 0; i < 6; i++)
            ASSERT_TRUE(!::IsStandard(malformed[i], whichType));

    }

    TEST(TestMultiSigTests , multisig_verify) {
        // Parameterized testing over consensus branch ids
        for ( int idx = Consensus::BASE_SPROUT; idx != Consensus::MAX_NETWORK_UPGRADES; idx++ ) {
            
            Consensus::UpgradeIndex sample = static_cast<Consensus::UpgradeIndex>(idx);
            uint32_t consensusBranchId = NetworkUpgradeInfo[sample].nBranchId;
            unsigned int flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC;

            ScriptError err;
            CKey key[4];
            CAmount amount = 0;
            for (int i = 0; i < 4; i++)
                key[i].MakeNewKey(true);

            CScript a_and_b;
            a_and_b << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

            CScript a_or_b;
            a_or_b << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

            CScript escrow;
            escrow << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;

            CMutableTransaction txFrom;  // Funding transaction
            txFrom.vout.resize(3);
            txFrom.vout[0].scriptPubKey = a_and_b;
            txFrom.vout[1].scriptPubKey = a_or_b;
            txFrom.vout[2].scriptPubKey = escrow;

            CMutableTransaction txTo[3]; // Spending transaction
            for (int i = 0; i < 3; i++)
            {
                txTo[i].vin.resize(1);
                txTo[i].vout.resize(1);
                txTo[i].vin[0].prevout.n = i;
                txTo[i].vin[0].prevout.hash = txFrom.GetHash();
                txTo[i].vout[0].nValue = 1;
            }

            std::vector<CKey> keys;
            CScript s;

            // Test a AND b:
            keys.assign(1,key[0]);
            keys.push_back(key[1]);
            s = sign_multisig(a_and_b, keys, txTo[0], 0, consensusBranchId);
            ASSERT_TRUE(VerifyScript(s, a_and_b, flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount), consensusBranchId, &err));
            ASSERT_TRUE(err == SCRIPT_ERR_OK) << ScriptErrorString(err);

            for (int i = 0; i < 4; i++)
            {
                keys.assign(1,key[i]);
                s = sign_multisig(a_and_b, keys, txTo[0], 0, consensusBranchId);
                ASSERT_TRUE(!VerifyScript(s, a_and_b, flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount), consensusBranchId, &err)) << strprintf("a&b 1: %d", i);
                ASSERT_TRUE(err == SCRIPT_ERR_INVALID_STACK_OPERATION) << ScriptErrorString(err);

                keys.assign(1,key[1]);
                keys.push_back(key[i]);
                s = sign_multisig(a_and_b, keys, txTo[0], 0, consensusBranchId);
                ASSERT_TRUE(!VerifyScript(s, a_and_b, flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount), consensusBranchId, &err)) << strprintf("a&b 2: %d", i);
                ASSERT_TRUE(err == SCRIPT_ERR_EVAL_FALSE) << ScriptErrorString(err);
            }

            // Test a OR b:
            for (int i = 0; i < 4; i++)
            {
                keys.assign(1,key[i]);
                s = sign_multisig(a_or_b, keys, txTo[1], 0, consensusBranchId);
                if (i == 0 || i == 1)
                {
                    ASSERT_TRUE(VerifyScript(s, a_or_b, flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount), consensusBranchId, &err)) << strprintf("a|b: %d", i);
                    ASSERT_TRUE(err == SCRIPT_ERR_OK) << ScriptErrorString(err);
                }
                else
                {
                    ASSERT_TRUE(!VerifyScript(s, a_or_b, flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount), consensusBranchId, &err)) << strprintf("a|b: %d", i);
                    ASSERT_TRUE(err == SCRIPT_ERR_EVAL_FALSE) << ScriptErrorString(err);
                }
            }
            s.clear();
            s << OP_0 << OP_1;
            ASSERT_TRUE(!VerifyScript(s, a_or_b, flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount), consensusBranchId, &err));
            ASSERT_TRUE(err == SCRIPT_ERR_SIG_DER) << ScriptErrorString(err);


            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                {
                    keys.assign(1,key[i]);
                    keys.push_back(key[j]);
                    s = sign_multisig(escrow, keys, txTo[2], 0, consensusBranchId);
                    if (i < j && i < 3 && j < 3)
                    {
                        ASSERT_TRUE(VerifyScript(s, escrow, flags, MutableTransactionSignatureChecker(&txTo[2], 0, amount), consensusBranchId, &err)) << strprintf("escrow 1: %d %d", i, j);
                        ASSERT_TRUE(err == SCRIPT_ERR_OK) << ScriptErrorString(err);
                    }
                    else
                    {
                        ASSERT_TRUE(!VerifyScript(s, escrow, flags, MutableTransactionSignatureChecker(&txTo[2], 0, amount), consensusBranchId, &err)) << strprintf("escrow 2: %d %d", i, j);
                        ASSERT_TRUE(err == SCRIPT_ERR_EVAL_FALSE) << ScriptErrorString(err);
                    }
                }


        }
    }

    TEST(TestMultiSigTests , multisig_Sign) {
        // Parameterized testing over consensus branch ids
        for ( int idx = Consensus::BASE_SPROUT; idx != Consensus::MAX_NETWORK_UPGRADES; idx++ ) {
            
            Consensus::UpgradeIndex sample = static_cast<Consensus::UpgradeIndex>(idx);
            uint32_t consensusBranchId = NetworkUpgradeInfo[sample].nBranchId;

            // Test SignSignature() (and therefore the version of Solver() that signs transactions)
            CBasicKeyStore keystore;
            CKey key[4];
            for (int i = 0; i < 4; i++)
            {
                key[i].MakeNewKey(true);
                keystore.AddKey(key[i]);
            }

            CScript a_and_b;
            a_and_b << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

            CScript a_or_b;
            a_or_b  << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

            CScript escrow;
            escrow << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;

            CMutableTransaction txFrom;  // Funding transaction
            txFrom.vout.resize(3);
            txFrom.vout[0].scriptPubKey = a_and_b;
            txFrom.vout[1].scriptPubKey = a_or_b;
            txFrom.vout[2].scriptPubKey = escrow;

            CMutableTransaction txTo[3]; // Spending transaction
            for (int i = 0; i < 3; i++)
            {
                txTo[i].vin.resize(1);
                txTo[i].vout.resize(1);
                txTo[i].vin[0].prevout.n = i;
                txTo[i].vin[0].prevout.hash = txFrom.GetHash();
                txTo[i].vout[0].nValue = 1;
            }

            for (int i = 0; i < 3; i++)
            {
                ASSERT_TRUE(SignSignature(keystore, txFrom, txTo[i], 0, SIGHASH_ALL, consensusBranchId)) << strprintf("SignSignature %d", i);
            }

        }
    }

}