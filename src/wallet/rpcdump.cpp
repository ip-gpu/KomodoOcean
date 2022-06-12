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

#include "chain.h"
#include "key_io.h"
#include "rpc/server.h"
#include "init.h"
#include "main.h"
#include "script/script.h"
#include "script/standard.h"
#include "sync.h"
#include "util.h"
#include "utiltime.h"
#include "wallet.h"

#include <fstream>
#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <univalue.h>

using namespace std;

void EnsureWalletIsUnlocked();
bool EnsureWalletIsAvailable(bool avoidException);

UniValue dumpwallet_impl(const UniValue& params, bool fHelp, bool fDumpZKeys);
UniValue importwallet_impl(const UniValue& params, bool fHelp, bool fImportZKeys);


std::string static EncodeDumpTime(int64_t nTime) {
    return DateTimeStrFormat("%Y-%m-%dT%H:%M:%SZ", nTime);
}

int64_t static DecodeDumpTime(const std::string &str) {
    static const boost::posix_time::ptime epoch = boost::posix_time::from_time_t(0);
    static const std::locale loc(std::locale::classic(),
        new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%SZ"));
    std::istringstream iss(str);
    iss.imbue(loc);
    boost::posix_time::ptime ptime(boost::date_time::not_a_date_time);
    iss >> ptime;
    if (ptime.is_not_a_date_time())
        return 0;
    return (ptime - epoch).total_seconds();
}

std::string static EncodeDumpString(const std::string &str) {
    std::stringstream ret;
    BOOST_FOREACH(unsigned char c, str) {
        if (c <= 32 || c >= 128 || c == '%') {
            ret << '%' << HexStr(&c, &c + 1);
        } else {
            ret << c;
        }
    }
    return ret.str();
}

std::string DecodeDumpString(const std::string &str) {
    std::stringstream ret;
    for (unsigned int pos = 0; pos < str.length(); pos++) {
        unsigned char c = str[pos];
        if (c == '%' && pos+2 < str.length()) {
            c = (((str[pos+1]>>6)*9+((str[pos+1]-'0')&15)) << 4) |
                ((str[pos+2]>>6)*9+((str[pos+2]-'0')&15));
            pos += 2;
        }
        ret << c;
    }
    return ret.str();
}

UniValue convertpassphrase(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if (fHelp || params.size() < 1 || params.size() > 1)
        throw runtime_error(
            "convertpassphrase \"agamapassphrase\"\n"
            "\nConverts Agama passphrase to a private key and WIF (for import with importprivkey).\n"
            "\nArguments:\n"
            "1. \"agamapassphrase\"   (string, required) Agama passphrase\n"
            "\nResult:\n"
            "\"agamapassphrase\": \"agamapassphrase\",   (string) Agama passphrase you entered\n"
            "\"address\": \"komodoaddress\",             (string) Address corresponding to your passphrase\n"
            "\"pubkey\": \"publickeyhex\",               (string) The hex value of the raw public key\n"
            "\"privkey\": \"privatekeyhex\",             (string) The hex value of the raw private key\n"
            "\"wif\": \"wif\"                            (string) The private key in WIF format to use with 'importprivkey'\n"
            "\nExamples:\n"
            + HelpExampleCli("convertpassphrase", "\"agamapassphrase\"")
            + HelpExampleRpc("convertpassphrase", "\"agamapassphrase\"")
        );

    bool fCompressed = true;
    string strAgamaPassphrase = params[0].get_str();

    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("agamapassphrase", strAgamaPassphrase));

    CKey tempkey = DecodeSecret(strAgamaPassphrase);
    /* first we should check if user pass wif to method, instead of passphrase */
    if (!tempkey.IsValid()) {
        /* it's a passphrase, not wif */
        uint256 sha256;
        CSHA256().Write((const unsigned char *)strAgamaPassphrase.c_str(), strAgamaPassphrase.length()).Finalize(sha256.begin());
        std::vector<unsigned char> privkey(sha256.begin(), sha256.begin() + sha256.size());
        privkey.front() &= 0xf8;
        privkey.back()  &= 0x7f;
        privkey.back()  |= 0x40;
        CKey key;
        key.Set(privkey.begin(),privkey.end(), fCompressed);
        CPubKey pubkey = key.GetPubKey();
        assert(key.VerifyPubKey(pubkey));
        CKeyID vchAddress = pubkey.GetID();

        ret.push_back(Pair("address", EncodeDestination(vchAddress)));
        ret.push_back(Pair("pubkey", HexStr(pubkey)));
        ret.push_back(Pair("privkey", HexStr(privkey)));
        ret.push_back(Pair("wif", EncodeSecret(key)));
    } else {
        /* seems it's a wif */
        CPubKey pubkey = tempkey.GetPubKey();
        assert(tempkey.VerifyPubKey(pubkey));
        CKeyID vchAddress = pubkey.GetID();
        ret.push_back(Pair("address", EncodeDestination(vchAddress)));
        ret.push_back(Pair("pubkey", HexStr(pubkey)));
        ret.push_back(Pair("privkey", HexStr(tempkey)));
        ret.push_back(Pair("wif", strAgamaPassphrase));
    }

    return ret;
}

UniValue importprivkey(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() < 1 || params.size() > 5)
        throw runtime_error(
            "importprivkey \"komodoprivkey\" ( \"label\" rescan height secret_key)\n"
            "\nAdds a private key (as returned by dumpprivkey) to your wallet.\n"
            "\nArguments:\n"
            "1. \"komodoprivkey\"   (string, required) The private key (see dumpprivkey)\n"
            "2. \"label\"            (string, optional, default=\"\") An optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the wallet for transactions\n"
            "4. height               (integer, optional, default=0) start at block height?\n"
            "5. secret_key           (integer, optional, default=188) used to import WIFs of other coins\n" 
            "\nNote: This call can take minutes to complete if rescan is true.\n"
            "\nExamples:\n"
            "\nDump a private key\n"
            + HelpExampleCli("dumpprivkey", "\"myaddress\"") +
            "\nImport the private key with rescan\n"
            + HelpExampleCli("importprivkey", "\"mykey\"") +
            "\nImport using a label and without rescan\n"
            + HelpExampleCli("importprivkey", "\"mykey\" \"testing\" false") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("importprivkey", "\"mykey\", \"testing\", false") +
            "\nImport with rescan from a block height\n"
            + HelpExampleCli("importprivkey", "\"mykey\" \"testing\" true 1000") +
            "\nImport a BTC WIF with rescan\n"
            + HelpExampleCli("importprivkey", "\"BTCWIF\" \"testing\" true 0 128") +
            "\nImport a BTC WIF without rescan\n"
            + HelpExampleCli("importprivkey", "\"BTCWIF\" \"testing\" false 0 128") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("importprivkey", "\"mykey\", \"testing\", true, 1000")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    string strSecret = params[0].get_str();
    string strLabel = "";
    int32_t height = 0;
    uint8_t secret_key = 0;
    CKey key;
    if (params.size() > 1)
        strLabel = params[1].get_str();

    // Whether to perform rescan after import
    bool fRescan = true;
    if (params.size() > 2)
        fRescan = params[2].get_bool();
    if ( fRescan && params.size() == 4 )
        height = params[3].get_int();


    if (params.size() > 4)
    {
        auto secret_key = AmountFromValue(params[4])/100000000;
        key = DecodeCustomSecret(strSecret, secret_key);
    } else {
        key = DecodeSecret(strSecret);
    }

    if ( height < 0 || height > chainActive.Height() )
        throw JSONRPCError(RPC_WALLET_ERROR, "Rescan height is out of range.");
    
    if (!key.IsValid()) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");

    CPubKey pubkey = key.GetPubKey();
    assert(key.VerifyPubKey(pubkey));
    CKeyID vchAddress = pubkey.GetID();
    {
        pwalletMain->MarkDirty();
        pwalletMain->SetAddressBook(vchAddress, strLabel, "receive");

        // Don't throw error in case a key is already there
        if (pwalletMain->HaveKey(vchAddress)) {
            return EncodeDestination(vchAddress);
        }

        pwalletMain->mapKeyMetadata[vchAddress].nCreateTime = 1;

        if (!pwalletMain->AddKeyPubKey(key, pubkey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error adding key to wallet");

        // whenever a key is imported, we need to scan the whole chain
        pwalletMain->nTimeFirstKey = 1; // 0 would be considered 'no value'

        if (fRescan) {
            pwalletMain->ScanForWalletTransactions(chainActive[height], true);
        }
    }

    return EncodeDestination(vchAddress);
}

UniValue importaddress(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "importaddress \"address\" ( \"label\" rescan )\n"
            "\nAdds an address or script (in hex) that can be watched as if it were in your wallet but cannot be used to spend.\n"
            "\nArguments:\n"
            "1. \"address\"          (string, required) The address\n"
            "2. \"label\"            (string, optional, default=\"\") An optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the wallet for transactions\n"
            "\nNote: This call can take minutes to complete if rescan is true.\n"
            "\nExamples:\n"
            "\nImport an address with rescan\n"
            + HelpExampleCli("importaddress", "\"myaddress\"") +
            "\nImport using a label without rescan\n"
            + HelpExampleCli("importaddress", "\"myaddress\" \"testing\" false") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("importaddress", "\"myaddress\", \"testing\", false")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CScript script;

    CTxDestination dest = DecodeDestination(params[0].get_str());
    if (IsValidDestination(dest)) {
        script = GetScriptForDestination(dest);
    } else if (IsHex(params[0].get_str())) {
        std::vector<unsigned char> data(ParseHex(params[0].get_str()));
        script = CScript(data.begin(), data.end());
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Komodo address or script");
    }

    string strLabel = "";
    if (params.size() > 1)
        strLabel = params[1].get_str();

    // Whether to perform rescan after import
    bool fRescan = true;
    if (params.size() > 2)
        fRescan = params[2].get_bool();

    {
        if (::IsMine(*pwalletMain, script) == ISMINE_SPENDABLE)
            throw JSONRPCError(RPC_WALLET_ERROR, "The wallet already contains the private key for this address or script");

        // add to address book or update label
        if (IsValidDestination(dest))
            pwalletMain->SetAddressBook(dest, strLabel, "receive");

        // Don't throw error in case an address is already there
        if (pwalletMain->HaveWatchOnly(script))
            return NullUniValue;

        pwalletMain->MarkDirty();

        if (!pwalletMain->AddWatchOnly(script))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error adding address to wallet");

        if (fRescan)
        {
            pwalletMain->ScanForWalletTransactions(chainActive.Genesis(), true);
            pwalletMain->ReacceptWalletTransactions();
        }
    }

    return NullUniValue;
}

UniValue z_importwallet(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() != 1)
        throw runtime_error(
            "z_importwallet \"filename\"\n"
            "\nImports taddr and zaddr keys from a wallet export file (see z_exportwallet).\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The wallet file\n"
            "\nExamples:\n"
            "\nDump the wallet\n"
            + HelpExampleCli("z_exportwallet", "\"nameofbackup\"") +
            "\nImport the wallet\n"
            + HelpExampleCli("z_importwallet", "\"path/to/exportdir/nameofbackup\"") +
            "\nImport using the json rpc call\n"
            + HelpExampleRpc("z_importwallet", "\"path/to/exportdir/nameofbackup\"")
        );

	return importwallet_impl(params, fHelp, true);
}

UniValue importwallet(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() != 1)
        throw runtime_error(
            "importwallet \"filename\"\n"
            "\nImports taddr keys from a wallet dump file (see dumpwallet).\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The wallet file\n"
            "\nExamples:\n"
            "\nDump the wallet\n"
            + HelpExampleCli("dumpwallet", "\"nameofbackup\"") +
            "\nImport the wallet\n"
            + HelpExampleCli("importwallet", "\"path/to/exportdir/nameofbackup\"") +
            "\nImport using the json rpc call\n"
            + HelpExampleRpc("importwallet", "\"path/to/exportdir/nameofbackup\"")
        );

	return importwallet_impl(params, fHelp, false);
}

UniValue importwallet_impl(const UniValue& params, bool fHelp, bool fImportZKeys)
{
    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    ifstream file;
    file.open(params[0].get_str().c_str(), std::ios::in | std::ios::ate);
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

    int64_t nTimeBegin = chainActive.LastTip()->GetBlockTime();

    bool fGood = true;

    int64_t nFilesize = std::max((int64_t)1, (int64_t)file.tellg());
    file.seekg(0, file.beg);

    pwalletMain->ShowProgress(_("Importing..."), 0); // show progress dialog in GUI
    while (file.good()) {
        pwalletMain->ShowProgress("", std::max(1, std::min(99, (int)(((double)file.tellg() / (double)nFilesize) * 100))));
        std::string line;
        std::getline(file, line);
        if (line.empty() || line[0] == '#')
            continue;

        std::vector<std::string> vstr;
        boost::split(vstr, line, boost::is_any_of(" "));
        if (vstr.size() < 2)
            continue;

        // Let's see if the address is a valid Zcash spending key
        if (fImportZKeys) {
            auto spendingkey = DecodeSpendingKey(vstr[0]);
            int64_t nTime = DecodeDumpTime(vstr[1]);
            // Only include hdKeypath and seedFpStr if we have both
            boost::optional<std::string> hdKeypath = (vstr.size() > 3) ? boost::optional<std::string>(vstr[2]) : boost::none;
            boost::optional<std::string> seedFpStr = (vstr.size() > 3) ? boost::optional<std::string>(vstr[3]) : boost::none;
            if (IsValidSpendingKey(spendingkey)) {
                auto addResult = boost::apply_visitor(
                    AddSpendingKeyToWallet(pwalletMain, Params().GetConsensus(), nTime, hdKeypath, seedFpStr, true), spendingkey);
                if (addResult == KeyAlreadyExists){
                    LogPrint("zrpc", "Skipping import of zaddr (key already present)\n");
                } else if (addResult == KeyNotAdded) {
                    // Something went wrong
                    fGood = false;
                }
                continue;
            } else {
                LogPrint("zrpc", "Importing detected an error: invalid spending key. Trying as a transparent key...\n");
                // Not a valid spending key, so carry on and see if it's a Zcash style t-address.
            }
        }

        CKey key = DecodeSecret(vstr[0]);
        if (!key.IsValid())
            continue;
        CPubKey pubkey = key.GetPubKey();
        assert(key.VerifyPubKey(pubkey));
        CKeyID keyid = pubkey.GetID();
        if (pwalletMain->HaveKey(keyid)) {
            LogPrintf("Skipping import of %s (key already present)\n", EncodeDestination(keyid));
            continue;
        }
        int64_t nTime = DecodeDumpTime(vstr[1]);
        std::string strLabel;
        bool fLabel = true;
        for (unsigned int nStr = 2; nStr < vstr.size(); nStr++) {
            if (boost::algorithm::starts_with(vstr[nStr], "#"))
                break;
            if (vstr[nStr] == "change=1")
                fLabel = false;
            if (vstr[nStr] == "reserve=1")
                fLabel = false;
            if (boost::algorithm::starts_with(vstr[nStr], "label=")) {
                strLabel = DecodeDumpString(vstr[nStr].substr(6));
                fLabel = true;
            }
        }
        LogPrintf("Importing %s...\n", EncodeDestination(keyid));
        if (!pwalletMain->AddKeyPubKey(key, pubkey)) {
            fGood = false;
            continue;
        }
        pwalletMain->mapKeyMetadata[keyid].nCreateTime = nTime;
        if (fLabel)
            pwalletMain->SetAddressBook(keyid, strLabel, "receive");
        nTimeBegin = std::min(nTimeBegin, nTime);
    }
    file.close();
    pwalletMain->ShowProgress("", 100); // hide progress dialog in GUI

    CBlockIndex *pindex = chainActive.LastTip();
    while (pindex && pindex->pprev && pindex->GetBlockTime() > nTimeBegin - 7200)
        pindex = pindex->pprev;

    if (!pwalletMain->nTimeFirstKey || nTimeBegin < pwalletMain->nTimeFirstKey)
        pwalletMain->nTimeFirstKey = nTimeBegin;

    LogPrintf("Rescanning last %i blocks\n", chainActive.Height() - pindex->nHeight + 1);
    pwalletMain->ScanForWalletTransactions(pindex);
    pwalletMain->MarkDirty();

    if (!fGood)
        throw JSONRPCError(RPC_WALLET_ERROR, "Error adding some keys to wallet");

    return NullUniValue;
}

UniValue dumpprivkey(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpprivkey \"t-addr\"\n"
            "\nReveals the private key corresponding to 't-addr'.\n"
            "Then the importprivkey can be used with this output\n"
            "\nArguments:\n"
            "1. \"t-addr\"   (string, required) The transparent address for the private key\n"
            "\nResult:\n"
            "\"key\"         (string) The private key\n"
            "\nExamples:\n"
            + HelpExampleCli("dumpprivkey", "\"myaddress\"")
            + HelpExampleCli("importprivkey", "\"mykey\"")
            + HelpExampleRpc("dumpprivkey", "\"myaddress\"")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    std::string strAddress = params[0].get_str();
    CTxDestination dest = DecodeDestination(strAddress);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid transparent address");
    }
    const CKeyID *keyID = boost::get<CKeyID>(&dest);
    if (!keyID) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    }
    CKey vchSecret;
    if (!pwalletMain->GetKey(*keyID, vchSecret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    }
    return EncodeSecret(vchSecret);
}



UniValue z_exportwallet(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() != 1)
        throw runtime_error(
            "z_exportwallet \"filename\"\n"
            "\nExports all wallet keys, for taddr and zaddr, in a human-readable format.  Overwriting an existing file is not permitted.\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The filename, saved in folder set by komodod -exportdir option\n"
            "\nResult:\n"
            "\"path\"           (string) The full path of the destination file\n"
            "\nExamples:\n"
            + HelpExampleCli("z_exportwallet", "\"test\"")
            + HelpExampleRpc("z_exportwallet", "\"test\"")
        );

	return dumpwallet_impl(params, fHelp, true);
}

UniValue dumpwallet(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpwallet \"filename\"\n"
            "\nDumps taddr wallet keys in a human-readable format.  Overwriting an existing file is not permitted.\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The filename, saved in folder set by komodod -exportdir option\n"
            "\nResult:\n"
            "\"path\"           (string) The full path of the destination file\n"
            "\nExamples:\n"
            + HelpExampleCli("dumpwallet", "\"test\"")
            + HelpExampleRpc("dumpwallet", "\"test\"")
        );

	return dumpwallet_impl(params, fHelp, false);
}

UniValue dumpwallet_impl(const UniValue& params, bool fHelp, bool fDumpZKeys)
{
    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    boost::filesystem::path exportdir;
    try {
        exportdir = GetExportDir();
    } catch (const std::runtime_error& e) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, e.what());
    }
    if (exportdir.empty()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Cannot export wallet until the komodod -exportdir option has been set");
    }
    std::string unclean = params[0].get_str();
    std::string clean = SanitizeFilename(unclean);
    if (clean.compare(unclean) != 0) {
        throw JSONRPCError(RPC_WALLET_ERROR, strprintf("Filename is invalid as only alphanumeric characters are allowed.  Try '%s' instead.", clean));
    }
    boost::filesystem::path exportfilepath = exportdir / clean;

    if (boost::filesystem::exists(exportfilepath)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot overwrite existing file " + exportfilepath.string());
    }

    ofstream file;
    file.open(exportfilepath.string().c_str());
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

    std::map<CKeyID, int64_t> mapKeyBirth;
    std::set<CKeyID> setKeyPool;
    pwalletMain->GetKeyBirthTimes(mapKeyBirth);
    pwalletMain->GetAllReserveKeys(setKeyPool);

    // sort time/key pairs
    std::vector<std::pair<int64_t, CKeyID> > vKeyBirth;
    for (std::map<CKeyID, int64_t>::const_iterator it = mapKeyBirth.begin(); it != mapKeyBirth.end(); it++) {
        vKeyBirth.push_back(std::make_pair(it->second, it->first));
    }
    mapKeyBirth.clear();
    std::sort(vKeyBirth.begin(), vKeyBirth.end());

    // produce output
    file << strprintf("# Wallet dump created by Komodo %s (%s)\n", CLIENT_BUILD, CLIENT_DATE);
    file << strprintf("# * Created on %s\n", EncodeDumpTime(GetTime()));
    file << strprintf("# * Best block at time of backup was %i (%s),\n", chainActive.Height(), chainActive.Tip()->GetBlockHash().ToString());
    file << strprintf("#   mined on %s\n", EncodeDumpTime(chainActive.Tip()->GetBlockTime()));
    {
        HDSeed hdSeed;
        pwalletMain->GetHDSeed(hdSeed);
        auto rawSeed = hdSeed.RawSeed();
        file << strprintf("# HDSeed=%s fingerprint=%s", HexStr(rawSeed.begin(), rawSeed.end()), hdSeed.Fingerprint().GetHex());
        file << "\n";
    }
    file << "\n";
    for (std::vector<std::pair<int64_t, CKeyID> >::const_iterator it = vKeyBirth.begin(); it != vKeyBirth.end(); it++) {
        const CKeyID &keyid = it->second;
        std::string strTime = EncodeDumpTime(it->first);
        std::string strAddr = EncodeDestination(keyid);
        CKey key;
        if (pwalletMain->GetKey(keyid, key)) {
            if (pwalletMain->mapAddressBook.count(keyid)) {
                file << strprintf("%s %s label=%s # addr=%s\n", EncodeSecret(key), strTime, EncodeDumpString(pwalletMain->mapAddressBook[keyid].name), strAddr);
            } else if (setKeyPool.count(keyid)) {
                file << strprintf("%s %s reserve=1 # addr=%s\n", EncodeSecret(key), strTime, strAddr);
            } else {
                file << strprintf("%s %s change=1 # addr=%s\n", EncodeSecret(key), strTime, strAddr);
            }
        }
    }
    file << "\n";

    if (fDumpZKeys) {
        std::set<libzcash::SproutPaymentAddress> sproutAddresses;
        pwalletMain->GetSproutPaymentAddresses(sproutAddresses);
        file << "\n";
        file << "# Zkeys\n";
        file << "\n";
        for (auto addr : sproutAddresses) {
            libzcash::SproutSpendingKey key;
            if (pwalletMain->GetSproutSpendingKey(addr, key)) {
                std::string strTime = EncodeDumpTime(pwalletMain->mapSproutZKeyMetadata[addr].nCreateTime);
                file << strprintf("%s %s # zaddr=%s\n", EncodeSpendingKey(key), strTime, EncodePaymentAddress(addr));
            }
        }
        std::set<libzcash::SaplingPaymentAddress> saplingAddresses;
        pwalletMain->GetSaplingPaymentAddresses(saplingAddresses);
        file << "\n";
        file << "# Sapling keys\n";
        file << "\n";
        for (auto addr : saplingAddresses) {
            libzcash::SaplingExtendedSpendingKey extsk;
            if (pwalletMain->GetSaplingExtendedSpendingKey(addr, extsk)) {
                auto ivk = extsk.expsk.full_viewing_key().in_viewing_key();
                CKeyMetadata keyMeta = pwalletMain->mapSaplingZKeyMetadata[ivk];
                std::string strTime = EncodeDumpTime(keyMeta.nCreateTime);
                // Keys imported with z_importkey do not have zip32 metadata
                if (keyMeta.hdKeypath.empty() || keyMeta.seedFp.IsNull()) {
                    file << strprintf("%s %s # zaddr=%s\n", EncodeSpendingKey(extsk), strTime, EncodePaymentAddress(addr));
                } else {
                    file << strprintf("%s %s %s %s # zaddr=%s\n", EncodeSpendingKey(extsk), strTime, keyMeta.hdKeypath, keyMeta.seedFp.GetHex(), EncodePaymentAddress(addr));
                }
            }
        }
        file << "\n";
    }

    file << "# End of dump\n";
    file.close();

    return exportfilepath.string();
}


UniValue z_importkey(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "z_importkey \"zkey\" ( rescan startHeight )\n"
            "\nAdds a zkey (as returned by z_exportkey) to your wallet.\n"
            "\nArguments:\n"
            "1. \"zkey\"             (string, required) The zkey (see z_exportkey)\n"
            "2. rescan             (string, optional, default=\"whenkeyisnew\") Rescan the wallet for transactions - can be \"yes\", \"no\" or \"whenkeyisnew\"\n"
            "3. startHeight        (numeric, optional, default=0) Block height to start rescan from\n"
            "\nNote: This call can take minutes to complete if rescan is true.\n"
            "\nExamples:\n"
            "\nExport a zkey\n"
            + HelpExampleCli("z_exportkey", "\"myaddress\"") +
            "\nImport the zkey with rescan\n"
            + HelpExampleCli("z_importkey", "\"mykey\"") +
            "\nImport the zkey with partial rescan\n"
            + HelpExampleCli("z_importkey", "\"mykey\" whenkeyisnew 30000") +
            "\nRe-import the zkey with longer partial rescan\n"
            + HelpExampleCli("z_importkey", "\"mykey\" yes 20000") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("z_importkey", "\"mykey\", \"no\"")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    // Whether to perform rescan after import
    bool fRescan = true;
    bool fIgnoreExistingKey = true;
    if (params.size() > 1) {
        auto rescan = params[1].get_str();
        if (rescan.compare("whenkeyisnew") != 0) {
            fIgnoreExistingKey = false;
            if (rescan.compare("yes") == 0) {
                fRescan = true;
            } else if (rescan.compare("no") == 0) {
                fRescan = false;
            } else {
                // Handle older API
                UniValue jVal;
                if (!jVal.read(std::string("[")+rescan+std::string("]")) ||
                    !jVal.isArray() || jVal.size()!=1 || !jVal[0].isBool()) {
                    throw JSONRPCError(
                        RPC_INVALID_PARAMETER,
                        "rescan must be \"yes\", \"no\" or \"whenkeyisnew\"");
                }
                fRescan = jVal[0].getBool();
            }
        }
    }

    // Height to rescan from
    int nRescanHeight = 0;
    if (params.size() > 2)
        nRescanHeight = params[2].get_int();
    if (nRescanHeight < 0 || nRescanHeight > chainActive.Height()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
    }

    string strSecret = params[0].get_str();
    auto spendingkey = DecodeSpendingKey(strSecret);
    if (!IsValidSpendingKey(spendingkey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid spending key");
    }

    // Sapling support
    auto addResult = boost::apply_visitor(AddSpendingKeyToWallet(pwalletMain, Params().GetConsensus()), spendingkey);
    if (addResult == KeyAlreadyExists && fIgnoreExistingKey) {
        return NullUniValue;
    }
    pwalletMain->MarkDirty();
    if (addResult == KeyNotAdded) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error adding spending key to wallet");
    }
    
    // whenever a key is imported, we need to scan the whole chain
    pwalletMain->nTimeFirstKey = 1; // 0 would be considered 'no value'
    
    // We want to scan for transactions and notes
    if (fRescan) {
        pwalletMain->ScanForWalletTransactions(chainActive[nRescanHeight], true);
    }

    return NullUniValue;
}

UniValue z_importviewingkey(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() < 1 || params.size() > 4)
        throw runtime_error(
            "z_importviewingkey \"vkey\" ( rescan startHeight )\n"
            "\nAdds a viewing key (as returned by z_exportviewingkey) to your wallet.\n"
            "\nArguments:\n"
            "1. \"vkey\"             (string, required) The viewing key (see z_exportviewingkey)\n"
            "2. rescan             (string, optional, default=\"whenkeyisnew\") Rescan the wallet for transactions - can be \"yes\", \"no\" or \"whenkeyisnew\"\n"
            "3. startHeight        (numeric, optional, default=0) Block height to start rescan from\n"
            "4. zaddr               (string, optional, default=\"\") zaddr in case of importing viewing key for Sapling\n"
            "\nNote: This call can take minutes to complete if rescan is true.\n"
            "\nExamples:\n"
            "\nImport a viewing key\n"
            + HelpExampleCli("z_importviewingkey", "\"vkey\"") +
            "\nImport the viewing key without rescan\n"
            + HelpExampleCli("z_importviewingkey", "\"vkey\", no") +
            "\nImport the viewing key with partial rescan\n"
            + HelpExampleCli("z_importviewingkey", "\"vkey\" whenkeyisnew 30000") +
            "\nRe-import the viewing key with longer partial rescan\n"
            + HelpExampleCli("z_importviewingkey", "\"vkey\" yes 20000") +
            "\nImport the viewing key for Sapling address\n"
            + HelpExampleCli("z_importviewingkey", "\"vkey\" no 0 \"zaddr\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("z_importviewingkey", "\"vkey\", \"no\"")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    // Whether to perform rescan after import
    bool fRescan = true;
    bool fIgnoreExistingKey = true;
    if (params.size() > 1) {
        auto rescan = params[1].get_str();
        if (rescan.compare("whenkeyisnew") != 0) {
            fIgnoreExistingKey = false;
            if (rescan.compare("no") == 0) {
                fRescan = false;
            } else if (rescan.compare("yes") != 0) {
                throw JSONRPCError(
                    RPC_INVALID_PARAMETER,
                    "rescan must be \"yes\", \"no\" or \"whenkeyisnew\"");
            }
        }
    }

    // Height to rescan from
    int nRescanHeight = 0;
    if (params.size() > 2) {
        nRescanHeight = params[2].get_int();
    }
    if (nRescanHeight < 0 || nRescanHeight > chainActive.Height()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
    }

    string strVKey = params[0].get_str();
    auto viewingkey = DecodeViewingKey(strVKey);
    if (!IsValidViewingKey(viewingkey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid viewing key");
    }

    if (boost::get<libzcash::SproutViewingKey>(&viewingkey) == nullptr) {
        if (params.size() < 4) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Missing zaddr for Sapling viewing key.");
        }
        string strAddress = params[3].get_str();
        auto address = DecodePaymentAddress(strAddress);
        if (!IsValidPaymentAddress(address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid zaddr");
        }

        auto addr = boost::get<libzcash::SaplingPaymentAddress>(address);
        auto ivk = boost::get<libzcash::SaplingIncomingViewingKey>(viewingkey);

        if (pwalletMain->HaveSaplingIncomingViewingKey(addr)) {
            if (fIgnoreExistingKey) {
                return NullUniValue;
            }
        } else {
            pwalletMain->MarkDirty();

            if (!pwalletMain->AddSaplingIncomingViewingKey(ivk, addr)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Error adding viewing key to wallet");
            }
        }
    } else {
        auto vkey = boost::get<libzcash::SproutViewingKey>(viewingkey);
        auto addr = vkey.address();
        if (pwalletMain->HaveSproutSpendingKey(addr)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "The wallet already contains the private key for this viewing key");
        }

        // Don't throw error in case a viewing key is already there
        if (pwalletMain->HaveSproutViewingKey(addr)) {
            if (fIgnoreExistingKey) {
                return NullUniValue;
            }
        } else {
            pwalletMain->MarkDirty();

            if (!pwalletMain->AddSproutViewingKey(vkey)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Error adding viewing key to wallet");
            }
        }
    }

    // We want to scan for transactions and notes
    if (fRescan) {
        pwalletMain->ScanForWalletTransactions(chainActive[nRescanHeight], true);
    }
    return NullUniValue;
}

UniValue z_exportkey(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() != 1)
        throw runtime_error(
            "z_exportkey \"zaddr\"\n"
            "\nReveals the zkey corresponding to 'zaddr'.\n"
            "Then the z_importkey can be used with this output\n"
            "\nArguments:\n"
            "1. \"zaddr\"   (string, required) The zaddr for the private key\n"
            "\nResult:\n"
            "\"key\"                  (string) The private key\n"
            "\nExamples:\n"
            + HelpExampleCli("z_exportkey", "\"myaddress\"")
            + HelpExampleCli("z_importkey", "\"mykey\"")
            + HelpExampleRpc("z_exportkey", "\"myaddress\"")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();

    auto address = DecodePaymentAddress(strAddress);
    if (!IsValidPaymentAddress(address)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid zaddr");
    }

    // Sapling support
    auto sk = boost::apply_visitor(GetSpendingKeyForPaymentAddress(pwalletMain), address);
    if (!sk) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet does not hold private zkey for this zaddr");
    }
    return EncodeSpendingKey(sk.get());
}

UniValue z_exportviewingkey(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() != 1)
        throw runtime_error(
            "z_exportviewingkey \"zaddr\"\n"
            "\nReveals the viewing key corresponding to 'zaddr'.\n"
            "Then the z_importviewingkey can be used with this output\n"
            "\nArguments:\n"
            "1. \"zaddr\"   (string, required) The zaddr for the viewing key\n"
            "\nResult:\n"
            "\"vkey\"                  (string) The viewing key\n"
            "\nExamples:\n"
            + HelpExampleCli("z_exportviewingkey", "\"myaddress\"")
            + HelpExampleRpc("z_exportviewingkey", "\"myaddress\"")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();

    auto address = DecodePaymentAddress(strAddress);
    if (!IsValidPaymentAddress(address)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid zaddr");
    }

    if (boost::get<libzcash::SproutPaymentAddress>(&address) == nullptr) {
        auto addr = boost::get<libzcash::SaplingPaymentAddress>(address);
        libzcash::SaplingIncomingViewingKey ivk;
        if(!pwalletMain->GetSaplingIncomingViewingKey(addr, ivk)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Wallet does not hold viewing key for this zaddr");
        }
        return EncodeViewingKey(ivk);
    }

    auto addr = boost::get<libzcash::SproutPaymentAddress>(address);
    libzcash::SproutViewingKey vk;
    if (!pwalletMain->GetSproutViewingKey(addr, vk)) {
        libzcash::SproutSpendingKey k;
        if (!pwalletMain->GetSproutSpendingKey(addr, k)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Wallet does not hold private key or viewing key for this zaddr");
        }
        vk = k.viewing_key();
    }

    return EncodeViewingKey(vk);
}

extern int32_t KOMODO_NSPV;
#ifndef KOMODO_NSPV_FULLNODE
#define KOMODO_NSPV_FULLNODE (KOMODO_NSPV <= 0)
#endif // !KOMODO_NSPV_FULLNODE
#ifndef KOMODO_NSPV_SUPERLITE
#define KOMODO_NSPV_SUPERLITE (KOMODO_NSPV > 0)
#endif // !KOMODO_NSPV_SUPERLITE
uint256 zeroid;
UniValue NSPV_getinfo_req(int32_t reqht);
UniValue NSPV_login(char *wifstr);
UniValue NSPV_logout();
UniValue NSPV_addresstxids(char *coinaddr,int32_t CCflag,int32_t skipcount,int32_t filter);
UniValue NSPV_addressutxos(char *coinaddr,int32_t CCflag,int32_t skipcount,int32_t filter);
UniValue NSPV_mempooltxids(char *coinaddr,int32_t CCflag,uint8_t funcid,uint256 txid,int32_t vout);
UniValue NSPV_broadcast(char *hex);
UniValue NSPV_spend(char *srcaddr,char *destaddr,int64_t satoshis);
UniValue NSPV_spentinfo(uint256 txid,int32_t vout);
UniValue NSPV_notarizations(int32_t height);
UniValue NSPV_hdrsproof(int32_t prevheight,int32_t nextheight);
UniValue NSPV_txproof(int32_t vout,uint256 txid,int32_t height);
UniValue NSPV_ccmoduleutxos(char *coinaddr, int64_t amount, uint8_t evalcode, std::string funcids, uint256 filtertxid);

uint256 Parseuint256(const char *hexstr);
extern std::string NSPV_address;

UniValue nspv_getinfo(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    int32_t reqht = 0;
    if ( fHelp || params.size() > 1 )
        throw runtime_error("nspv_getinfo [hdrheight]\n");
    if ( KOMODO_NSPV_FULLNODE )
        throw runtime_error("-nSPV=1 must be set to use nspv\n");
    if ( params.size() == 1 )
        reqht = atoi((char *)params[0].get_str().c_str());
    return(NSPV_getinfo_req(reqht));
}

UniValue nspv_logout(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if ( fHelp || params.size() != 0 )
        throw runtime_error("nspv_logout\n");
    if ( KOMODO_NSPV_FULLNODE )
        throw runtime_error("-nSPV=1 must be set to use nspv\n");
    return(NSPV_logout());
}

UniValue nspv_login(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if ( fHelp || params.size() != 1 )
        throw runtime_error("nspv_login wif\n");
    if ( KOMODO_NSPV_FULLNODE )
        throw runtime_error("-nSPV=1 must be set to use nspv\n");
    return(NSPV_login((char *)params[0].get_str().c_str()));
}

UniValue nspv_listunspent(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    int32_t skipcount = 0,CCflag = 0;
    if ( fHelp || params.size() > 3 )
        throw runtime_error("nspv_listunspent [address [isCC [skipcount]]]\n");
    if ( KOMODO_NSPV_FULLNODE )
        throw runtime_error("-nSPV=1 must be set to use nspv\n");
    if ( params.size() == 0 )
    {
        if ( NSPV_address.size() != 0 )
            return(NSPV_addressutxos((char *)NSPV_address.c_str(),0,0,0));
        else throw runtime_error("nspv_listunspent [address [isCC [skipcount]]]\n");
    }
    if ( params.size() >= 1 )
    {
        if ( params.size() >= 2 )
            CCflag = atoi((char *)params[1].get_str().c_str());
        if ( params.size() == 3 )
            skipcount = atoi((char *)params[2].get_str().c_str());
        return(NSPV_addressutxos((char *)params[0].get_str().c_str(),CCflag,skipcount,0));
    }
    else throw runtime_error("nspv_listunspent [address [isCC [skipcount]]]\n");
}

UniValue nspv_mempool(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    int32_t vout = 0,CCflag = 0; uint256 txid; uint8_t funcid; char *coinaddr;
    memset(&txid,0,sizeof(txid));
    if ( fHelp || params.size() > 5 )
        throw runtime_error("nspv_mempool func(0 all, 1 address recv, 2 txid/vout spent, 3 txid inmempool) address isCC [txid vout]]]\n");
    if ( KOMODO_NSPV_FULLNODE )
        throw runtime_error("-nSPV=1 must be set to use nspv\n");
    funcid = atoi((char *)params[0].get_str().c_str());
    coinaddr = (char *)params[1].get_str().c_str();
    CCflag = atoi((char *)params[2].get_str().c_str());
    if ( params.size() > 3 )
    {
        if ( params.size() != 5 )
            throw runtime_error("nspv_mempool func(0 all, 1 address recv, 2 txid/vout spent, 3 txid inmempool) address isCC [txid vout]]]\n");
        txid = Parseuint256((char *)params[3].get_str().c_str());
        vout = atoi((char *)params[4].get_str().c_str());
    }
    return(NSPV_mempooltxids(coinaddr,CCflag,funcid,txid,vout));
}

UniValue nspv_listtransactions(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    int32_t skipcount = 0,CCflag = 0;
    if ( fHelp || params.size() > 3 )
        throw runtime_error("nspv_listtransactions [address [isCC [skipcount]]]\n");
    if ( KOMODO_NSPV_FULLNODE )
        throw runtime_error("-nSPV=1 must be set to use nspv\n");
    if ( params.size() == 0 )
    {
        if ( NSPV_address.size() != 0 )
            return(NSPV_addresstxids((char *)NSPV_address.c_str(),0,0,0));
        else throw runtime_error("nspv_listtransactions [address [isCC [skipcount]]]\n");
    }
    if ( params.size() >= 1 )
    {
        if ( params.size() >= 2 )
            CCflag = atoi((char *)params[1].get_str().c_str());
        if ( params.size() == 3 )
            skipcount = atoi((char *)params[2].get_str().c_str());
        //fprintf(stderr,"call txids cc.%d skip.%d\n",CCflag,skipcount);
        return(NSPV_addresstxids((char *)params[0].get_str().c_str(),CCflag,skipcount,0));
    }
    else throw runtime_error("nspv_listtransactions [address [isCC [skipcount]]]\n");
}

UniValue nspv_spentinfo(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 txid; int32_t vout;
    if ( fHelp || params.size() != 2 )
        throw runtime_error("nspv_spentinfo txid vout\n");
    if ( KOMODO_NSPV_FULLNODE )
        throw runtime_error("-nSPV=1 must be set to use nspv\n");
    txid = Parseuint256((char *)params[0].get_str().c_str());
    vout = atoi((char *)params[1].get_str().c_str());
    return(NSPV_spentinfo(txid,vout));
}

UniValue nspv_notarizations(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    int32_t height;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("nspv_notarizations height\n");
    if ( KOMODO_NSPV_FULLNODE )
        throw runtime_error("-nSPV=1 must be set to use nspv\n");
    height = atoi((char *)params[0].get_str().c_str());
    return(NSPV_notarizations(height));
}

UniValue nspv_hdrsproof(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    int32_t prevheight,nextheight;
    if ( fHelp || params.size() != 2 )
        throw runtime_error("nspv_hdrsproof prevheight nextheight\n");
    if ( KOMODO_NSPV_FULLNODE )
        throw runtime_error("-nSPV=1 must be set to use nspv\n");
    prevheight = atoi((char *)params[0].get_str().c_str());
    nextheight = atoi((char *)params[1].get_str().c_str());
    return(NSPV_hdrsproof(prevheight,nextheight));
}

UniValue nspv_txproof(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 txid; int32_t height;
    if ( fHelp || params.size() != 2 )
        throw runtime_error("nspv_txproof txid height\n");
    if ( KOMODO_NSPV_FULLNODE )
        throw runtime_error("-nSPV=1 must be set to use nspv\n");
    txid = Parseuint256((char *)params[0].get_str().c_str());
    height = atoi((char *)params[1].get_str().c_str());
    return(NSPV_txproof(0,txid,height));
}

UniValue nspv_spend(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint64_t satoshis;
    if ( fHelp || params.size() != 2 )
        throw runtime_error("nspv_spend address amount\n");
    if ( KOMODO_NSPV_FULLNODE )
        throw runtime_error("-nSPV=1 must be set to use nspv\n");
    if ( NSPV_address.size() == 0 )
        throw runtime_error("to nspv_send you need an active nspv_login\n");
    satoshis = atof(params[1].get_str().c_str())*COIN + 0.0000000049;
    //fprintf(stderr,"satoshis.%lld from %.8f\n",(long long)satoshis,atof(params[1].get_str().c_str()));
    if ( satoshis < 1000 )
        throw runtime_error("amount too small\n");
    return(NSPV_spend((char *)NSPV_address.c_str(),(char *)params[0].get_str().c_str(),satoshis));
}

UniValue nspv_broadcast(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if ( fHelp || params.size() != 1 )
        throw runtime_error("nspv_broadcast hex\n");
    if ( KOMODO_NSPV_FULLNODE )
        throw runtime_error("-nSPV=1 must be set to use nspv\n");
    return(NSPV_broadcast((char *)params[0].get_str().c_str()));
}

UniValue nspv_listccmoduleunspent(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    int32_t skipcount = 0, CCflag = 0;
    if (fHelp || params.size() != 5)
        throw runtime_error("nspv_listccmoduleunspent address amount evalcode funcids txid\n\n" 
        "returns utxos from the address, filtered by evalcode funcids and txid in opret.\n"
        "if amount is 0 just returns no utxos and available total.\n"
        "funcids is a string of funcid symbols. The first symbol is considered as the creation funcid, so the txid param will be compared to the creation tx id.\n"
        "For the second+ funcids the txid param will be compared to txid in opreturn\n\n" );
    if (KOMODO_NSPV_FULLNODE)
        throw runtime_error("-nSPV=1 must be set to use nspv\n");

    std::string address = params[0].get_str().c_str();
    int64_t amount = atof(params[1].get_str().c_str());
    uint8_t evalcode = atoi(params[2].get_str().c_str());
    std::string funcids = params[3].get_str().c_str();
    uint256 txid = Parseuint256( params[4].get_str().c_str() );
    return(NSPV_ccmoduleutxos((char*)address.c_str(), amount, evalcode, funcids, txid));
}