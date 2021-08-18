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
#include "komodo_utils.h"
#include "komodo_extern_globals.h"
#include "komodo_notary.h"

int32_t komodo_is_issuer()
{
    if ( ASSETCHAINS_SYMBOL[0] != 0 && komodo_baseid(ASSETCHAINS_SYMBOL) >= 0 )
        return(1);
    else return(0);
}

// given a block height, this returns the unlock time for that block height, derived from
// the ASSETCHAINS_MAGIC number as well as the block height, providing different random numbers
// for corresponding blocks across chains, but the same sequence in each chain
int64_t komodo_block_unlocktime(uint32_t nHeight)
{
    uint64_t fromTime, toTime, unlocktime;

    if ( ASSETCHAINS_TIMEUNLOCKFROM == ASSETCHAINS_TIMEUNLOCKTO )
        unlocktime = ASSETCHAINS_TIMEUNLOCKTO;
    else
    {
        if (strcmp(ASSETCHAINS_SYMBOL, "VRSC") != 0 || nHeight >= 12800)
        {
            unlocktime = komodo_block_prg(nHeight) % (ASSETCHAINS_TIMEUNLOCKTO - ASSETCHAINS_TIMEUNLOCKFROM);
            unlocktime += ASSETCHAINS_TIMEUNLOCKFROM;
        }
        else
        {
            unlocktime = komodo_block_prg(nHeight) / (0xffffffffffffffff / ((ASSETCHAINS_TIMEUNLOCKTO - ASSETCHAINS_TIMEUNLOCKFROM) + 1));
            // boundary and power of 2 can make it exceed to time by 1
            unlocktime = unlocktime + ASSETCHAINS_TIMEUNLOCKFROM;
            if (unlocktime > ASSETCHAINS_TIMEUNLOCKTO)
                unlocktime--;
        }
    }
    return ((int64_t)unlocktime);
}

uint16_t _komodo_userpass(char *username,char *password,FILE *fp)
{
    char *rpcuser,*rpcpassword,*str,line[8192]; uint16_t port = 0;
    rpcuser = rpcpassword = 0;
    username[0] = password[0] = 0;
    while ( fgets(line,sizeof(line),fp) != 0 )
    {
        if ( line[0] == '#' )
            continue;
        //LogPrintf("line.(%s) %p %p\n",line,strstr(line,(char *)"rpcuser"),strstr(line,(char *)"rpcpassword"));
        if ( (str= strstr(line,(char *)"rpcuser")) != 0 )
            rpcuser = parse_conf_line(str,(char *)"rpcuser");
        else if ( (str= strstr(line,(char *)"rpcpassword")) != 0 )
            rpcpassword = parse_conf_line(str,(char *)"rpcpassword");
        else if ( (str= strstr(line,(char *)"rpcport")) != 0 )
        {
            port = atoi(parse_conf_line(str,(char *)"rpcport"));
            //LogPrintf("rpcport.%u in file\n",port);
        }
    }
    if ( rpcuser != 0 && rpcpassword != 0 )
    {
        strcpy(username,rpcuser);
        strcpy(password,rpcpassword);
    }
    //LogPrintf("rpcuser.(%s) rpcpassword.(%s) KMDUSERPASS.(%s) %u\n",rpcuser,rpcpassword,KMDUSERPASS,port);
    if ( rpcuser != 0 )
        free(rpcuser);
    if ( rpcpassword != 0 )
        free(rpcpassword);
    return(port);
}

void komodo_statefname(char *fname,char *symbol,char *str)
{
    int32_t n,len;
    sprintf(fname,"%s",GetDataDir(false).string().c_str());
    if ( (n= (int32_t)strlen(ASSETCHAINS_SYMBOL)) != 0 )
    {
        len = (int32_t)strlen(fname);
        if ( !mapArgs.count("-datadir") && strcmp(ASSETCHAINS_SYMBOL,&fname[len - n]) == 0 )
            fname[len - n] = 0;
        else if(mapArgs.count("-datadir")) LogPrintf("DEBUG - %s:%d: custom datadir\n", __FILE__, __LINE__);
        else
        {
            if ( strcmp(symbol,"REGTEST") != 0 )
                LogPrintf("unexpected fname.(%s) vs %s [%s] n.%d len.%d (%s)\n",fname,symbol,ASSETCHAINS_SYMBOL,n,len,&fname[len - n]);
            return;
        }
    }
    else
    {
#ifdef _WIN32
        strcat(fname,"\\");
#else
        strcat(fname,"/");
#endif
    }
    if ( symbol != 0 && symbol[0] != 0 && strcmp("KMD",symbol) != 0 )
    {
        if(!mapArgs.count("-datadir")) strcat(fname,symbol);
        //printf("statefname.(%s) -> (%s)\n",symbol,fname);
#ifdef _WIN32
        strcat(fname,"\\");
#else
        strcat(fname,"/");
#endif
    }
    strcat(fname,str);
    //LogPrintf("test.(%s) -> [%s] statename.(%s) %s\n",test,ASSETCHAINS_SYMBOL,symbol,fname);
}

void komodo_configfile(char *symbol,uint16_t rpcport)
{
    static char myusername[512],mypassword[8192];
    FILE *fp; uint16_t kmdport; uint8_t buf2[33]; char fname[512],buf[128],username[512],password[8192]; uint32_t crc,r,r2,i;
    if ( symbol != 0 && rpcport != 0 )
    {
        r = (uint32_t)time(NULL);
        r2 = OS_milliseconds();
        memcpy(buf,&r,sizeof(r));
        memcpy(&buf[sizeof(r)],&r2,sizeof(r2));
        memcpy(&buf[sizeof(r)+sizeof(r2)],symbol,strlen(symbol));
        crc = calc_crc32(0,(uint8_t *)buf,(int32_t)(sizeof(r)+sizeof(r2)+strlen(symbol)));
				#ifdef _WIN32
				randombytes_buf(buf2,sizeof(buf2));
				#else
        OS_randombytes(buf2,sizeof(buf2));
				#endif
        for (i=0; i<sizeof(buf2); i++)
            sprintf(&password[i*2],"%02x",buf2[i]);
        password[i*2] = 0;
        sprintf(buf,"%s.conf",symbol);
        BITCOIND_RPCPORT = rpcport;
#ifdef _WIN32
        sprintf(fname,"%s\\%s",GetDataDir(false).string().c_str(),buf);
#else
        sprintf(fname,"%s/%s",GetDataDir(false).string().c_str(),buf);
#endif
        if(mapArgs.count("-conf")) sprintf(fname, "%s", GetConfigFile().string().c_str());
        if ( (fp= fopen(fname,"rb")) == 0 )
        {
#ifndef FROM_CLI
            if ( (fp= fopen(fname,"wb")) != 0 )
            {
                fprintf(fp,"rpcuser=user%u\nrpcpassword=pass%s\nrpcport=%u\nserver=1\ntxindex=1\nrpcworkqueue=256\nrpcallowip=127.0.0.1\nrpcbind=127.0.0.1\n",crc,password,rpcport);
                fclose(fp);
                LogPrintf("Created (%s)\n",fname);
            } else LogPrintf("Couldnt create (%s)\n",fname);
#endif
        }
        else
        {
            _komodo_userpass(myusername,mypassword,fp);
            mapArgs["-rpcpassword"] = mypassword;
            mapArgs["-rpcusername"] = myusername;
            //LogPrintf("myusername.(%s)\n",myusername);
            fclose(fp);
        }
    }
    strcpy(fname,GetDataDir().string().c_str());
#ifdef _WIN32
    while ( fname[strlen(fname)-1] != '\\' )
        fname[strlen(fname)-1] = 0;
    strcat(fname,"komodo.conf");
#else
    while ( fname[strlen(fname)-1] != '/' )
        fname[strlen(fname)-1] = 0;
#ifdef __APPLE__
    strcat(fname,"Komodo.conf");
#else
    strcat(fname,"komodo.conf");
#endif
#endif
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        if ( (kmdport= _komodo_userpass(username,password,fp)) != 0 )
            KMD_PORT = kmdport;
        sprintf(KMDUSERPASS,"%s:%s",username,password);
        fclose(fp);
//LogPrintf("KOMODO.(%s) -> userpass.(%s)\n",fname,KMDUSERPASS);
    } //else LogPrintf("couldnt open.(%s)\n",fname);
}

uint16_t komodo_userpass(char *userpass,char *symbol)
{
    FILE *fp; uint16_t port = 0; char fname[512],username[512],password[512],confname[KOMODO_ASSETCHAIN_MAXLEN];
    userpass[0] = 0;
    if ( strcmp("KMD",symbol) == 0 )
    {
#ifdef __APPLE__
        sprintf(confname,"Komodo.conf");
#else
        sprintf(confname,"komodo.conf");
#endif
    }
    else if(!mapArgs.count("-conf")) {
        sprintf(confname,"%s.conf",symbol);
        komodo_statefname(fname,symbol,confname);
    } else sprintf(fname,"%s",GetDataDir().string().c_str());
    
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        port = _komodo_userpass(username,password,fp);
        sprintf(userpass,"%s:%s",username,password);
        if ( strcmp(symbol,ASSETCHAINS_SYMBOL) == 0 )
            strcpy(ASSETCHAINS_USERPASS,userpass);
        fclose(fp);
    }
    return(port);
}

uint32_t komodo_assetmagic(char *symbol,uint64_t supply,uint8_t *extraptr,int32_t extralen)
{
    uint8_t buf[512]; uint32_t crc0=0; int32_t len = 0; bits256 hash;
    if ( strcmp(symbol,"KMD") == 0 )
        return(0x8de4eef9);
    len = iguana_rwnum(1,&buf[len],sizeof(supply),(void *)&supply);
    strcpy((char *)&buf[len],symbol);
    len += strlen(symbol);
    if ( extraptr != 0 && extralen != 0 )
    {
        vcalc_sha256(0,hash.bytes,extraptr,extralen);
        crc0 = hash.uints[0];
        int32_t i; for (i=0; i<extralen; i++)
            LogPrintf("%02x",extraptr[i]);
        LogPrintf(" extralen.%d crc0.%x\n",extralen,crc0);
    }
    return(calc_crc32(crc0,buf,len));
}

uint16_t komodo_assetport(uint32_t magic,int32_t extralen)
{
    if ( magic == 0x8de4eef9 )
        return(7770);
    else if ( extralen == 0 )
        return(8000 + (magic % 7777));
    else return(16000 + (magic % 49500));
}

uint16_t komodo_port(char *symbol,uint64_t supply,uint32_t *magicp,uint8_t *extraptr,int32_t extralen)
{
    if ( symbol == 0 || symbol[0] == 0 || strcmp("KMD",symbol) == 0 )
    {
        *magicp = 0x8de4eef9;
        return(7770);
    }
    *magicp = komodo_assetmagic(symbol,supply,extraptr,extralen);
    return(komodo_assetport(*magicp,extralen));
}

/*void komodo_ports(uint16_t ports[MAX_CURRENCIES])
{
    int32_t i; uint32_t magic;
    for (i=0; i<MAX_CURRENCIES; i++)
    {
        ports[i] = komodo_port(CURRENCIES[i],10,&magic);
        LogPrintf("%u ",ports[i]);
    }
    LogPrintf("ports\n");
}*/

char *iguanafmtstr = (char *)"curl --url \"http://127.0.0.1:7776\" --data \"{\\\"conf\\\":\\\"%s.conf\\\",\\\"path\\\":\\\"${HOME#\"/\"}/.komodo/%s\\\",\\\"unitval\\\":\\\"20\\\",\\\"zcash\\\":1,\\\"RELAY\\\":-1,\\\"VALIDATE\\\":0,\\\"prefetchlag\\\":-1,\\\"poll\\\":100,\\\"active\\\":1,\\\"agent\\\":\\\"iguana\\\",\\\"method\\\":\\\"addcoin\\\",\\\"startpend\\\":4,\\\"endpend\\\":4,\\\"services\\\":129,\\\"maxpeers\\\":8,\\\"newcoin\\\":\\\"%s\\\",\\\"name\\\":\\\"%s\\\",\\\"hasheaders\\\":1,\\\"useaddmultisig\\\":0,\\\"netmagic\\\":\\\"%s\\\",\\\"p2p\\\":%u,\\\"rpc\\\":%u,\\\"pubval\\\":60,\\\"p2shval\\\":85,\\\"wifval\\\":188,\\\"txfee_satoshis\\\":\\\"10000\\\",\\\"isPoS\\\":0,\\\"minoutput\\\":10000,\\\"minconfirms\\\":2,\\\"genesishash\\\":\\\"027e3758c3a65b12aa1046462b486d0a63bfa1beae327897f56c5cfb7daaae71\\\",\\\"protover\\\":170002,\\\"genesisblock\\\":\\\"0100000000000000000000000000000000000000000000000000000000000000000000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a000000000000000000000000000000000000000000000000000000000000000029ab5f490f0f0f200b00000000000000000000000000000000000000000000000000000000000000fd4005000d5ba7cda5d473947263bf194285317179d2b0d307119c2e7cc4bd8ac456f0774bd52b0cd9249be9d40718b6397a4c7bbd8f2b3272fed2823cd2af4bd1632200ba4bf796727d6347b225f670f292343274cc35099466f5fb5f0cd1c105121b28213d15db2ed7bdba490b4cedc69742a57b7c25af24485e523aadbb77a0144fc76f79ef73bd8530d42b9f3b9bed1c135ad1fe152923fafe98f95f76f1615e64c4abb1137f4c31b218ba2782bc15534788dda2cc08a0ee2987c8b27ff41bd4e31cd5fb5643dfe862c9a02ca9f90c8c51a6671d681d04ad47e4b53b1518d4befafefe8cadfb912f3d03051b1efbf1dfe37b56e93a741d8dfd80d576ca250bee55fab1311fc7b3255977558cdda6f7d6f875306e43a14413facdaed2f46093e0ef1e8f8a963e1632dcbeebd8e49fd16b57d49b08f9762de89157c65233f60c8e38a1f503a48c555f8ec45dedecd574a37601323c27be597b956343107f8bd80f3a925afaf30811df83c402116bb9c1e5231c70fff899a7c82f73c902ba54da53cc459b7bf1113db65cc8f6914d3618560ea69abd13658fa7b6af92d374d6eca9529f8bd565166e4fcbf2a8dfb3c9b69539d4d2ee2e9321b85b331925df195915f2757637c2805e1d4131e1ad9ef9bc1bb1c732d8dba4738716d351ab30c996c8657bab39567ee3b29c6d054b711495c0d52e1cd5d8e55b4f0f0325b97369280755b46a02afd54be4ddd9f77c22272b8bbb17ff5118fedbae2564524e797bd28b5f74f7079d532ccc059807989f94d267f47e724b3f1ecfe00ec9e6541c961080d8891251b84b4480bc292f6a180bea089fef5bbda56e1e41390d7c0e85ba0ef530f7177413481a226465a36ef6afe1e2bca69d2078712b3912bba1a99b1fbff0d355d6ffe726d2bb6fbc103c4ac5756e5bee6e47e17424ebcbf1b63d8cb90ce2e40198b4f4198689daea254307e52a25562f4c1455340f0ffeb10f9d8e914775e37d0edca019fb1b9c6ef81255ed86bc51c5391e0591480f66e2d88c5f4fd7277697968656a9b113ab97f874fdd5f2465e5559533e01ba13ef4a8f7a21d02c30c8ded68e8c54603ab9c8084ef6d9eb4e92c75b078539e2ae786ebab6dab73a09e0aa9ac575bcefb29e930ae656e58bcb513f7e3c17e079dce4f05b5dbc18c2a872b22509740ebe6a3903e00ad1abc55076441862643f93606e3dc35e8d9f2caef3ee6be14d513b2e062b21d0061de3bd56881713a1a5c17f5ace05e1ec09da53f99442df175a49bd154aa96e4949decd52fed79ccf7ccbce32941419c314e374e4a396ac553e17b5340336a1a25c22f9e42a243ba5404450b650acfc826a6e432971ace776e15719515e1634ceb9a4a35061b668c74998d3dfb5827f6238ec015377e6f9c94f38108768cf6e5c8b132e0303fb5a200368f845ad9d46343035a6ff94031df8d8309415bb3f6cd5ede9c135fdabcc030599858d803c0f85be7661c88984d88faa3d26fb0e9aac0056a53f1b5d0baed713c853c4a2726869a0a124a8a5bbc0fc0ef80c8ae4cb53636aa02503b86a1eb9836fcc259823e2692d921d88e1ffc1e6cb2bde43939ceb3f32a611686f539f8f7c9f0bf00381f743607d40960f06d347d1cd8ac8a51969c25e37150efdf7aa4c2037a2fd0516fb444525ab157a0ed0a7412b2fa69b217fe397263153782c0f64351fbdf2678fa0dc8569912dcd8e3ccad38f34f23bbbce14c6a26ac24911b308b82c7e43062d180baeac4ba7153858365c72c63dcf5f6a5b08070b730adb017aeae925b7d0439979e2679f45ed2f25a7edcfd2fb77a8794630285ccb0a071f5cce410b46dbf9750b0354aae8b65574501cc69efb5b6a43444074fee116641bb29da56c2b4a7f456991fc92b2\\\",\\\"debug\\\":0,\\\"seedipaddr\\\":\\\"%s\\\",\\\"sapling\\\":1,\\\"notarypay\\\":%i}\"";



int32_t komodo_whoami(char *pubkeystr,int32_t height,uint32_t timestamp)
{
    int32_t i,notaryid;
    for (i=0; i<33; i++)
        sprintf(&pubkeystr[i<<1],"%02x",NOTARY_PUBKEY33[i]);
    pubkeystr[66] = 0;
    komodo_chosennotary(&notaryid,height,NOTARY_PUBKEY33,timestamp);
    return(notaryid);
}

char *argv0suffix[] =
{
    (char *)"mnzd", (char *)"mnz-cli", (char *)"mnzd.exe", (char *)"mnz-cli.exe", (char *)"btchd", (char *)"btch-cli", (char *)"btchd.exe", (char *)"btch-cli.exe"
};

char *argv0names[] =
{
    (char *)"MNZ", (char *)"MNZ", (char *)"MNZ", (char *)"MNZ", (char *)"BTCH", (char *)"BTCH", (char *)"BTCH", (char *)"BTCH"
};

uint64_t komodo_max_money()
{
    return komodo_current_supply(10000000);
}

uint64_t komodo_ac_block_subsidy(int nHeight)
{
    // we have to find our era, start from beginning reward, and determine current subsidy
    int64_t numerator, denominator, subsidy = 0;
    int64_t subsidyDifference;
    int32_t numhalvings, curEra = 0, sign = 1;
    static uint64_t cached_subsidy; static int32_t cached_numhalvings; static int cached_era;

    // check for backwards compat, older chains with no explicit rewards had 0.0001 block reward
    if ( ASSETCHAINS_ENDSUBSIDY[0] == 0 && ASSETCHAINS_REWARD[0] == 0 )
        subsidy = 10000;
    else if ( (ASSETCHAINS_ENDSUBSIDY[0] == 0 && ASSETCHAINS_REWARD[0] != 0) || ASSETCHAINS_ENDSUBSIDY[0] != 0 )
    {
        // if we have an end block in the first era, find our current era
        if ( ASSETCHAINS_ENDSUBSIDY[0] != 0 )
        {
            for ( curEra = 0; curEra <= ASSETCHAINS_LASTERA; curEra++ )
            {
                if ( ASSETCHAINS_ENDSUBSIDY[curEra] > nHeight || ASSETCHAINS_ENDSUBSIDY[curEra] == 0 )
                    break;
            }
        }
        if ( curEra <= ASSETCHAINS_LASTERA )
        {
            int64_t nStart = curEra ? ASSETCHAINS_ENDSUBSIDY[curEra - 1] : 0;
            subsidy = (int64_t)ASSETCHAINS_REWARD[curEra];
            if ( subsidy || (curEra != ASSETCHAINS_LASTERA && ASSETCHAINS_REWARD[curEra + 1] != 0) )
            {
                if ( ASSETCHAINS_HALVING[curEra] != 0 )
                {
                    if ( (numhalvings = ((nHeight - nStart) / ASSETCHAINS_HALVING[curEra])) > 0 )
                    {
                        if ( ASSETCHAINS_DECAY[curEra] == 0 )
                            subsidy >>= numhalvings;
                        else if ( ASSETCHAINS_DECAY[curEra] == 100000000 && ASSETCHAINS_ENDSUBSIDY[curEra] != 0 )
                        {
                            if ( curEra == ASSETCHAINS_LASTERA )
                            {
                                subsidyDifference = subsidy;
                            }
                            else
                            {
                                // Ex: -ac_eras=3 -ac_reward=0,384,24 -ac_end=1440,260640,0 -ac_halving=1,1440,2103840 -ac_decay 100000000,97750000,0
                                subsidyDifference = subsidy - ASSETCHAINS_REWARD[curEra + 1];
                                if (subsidyDifference < 0)
                                {
                                    sign = -1;
                                    subsidyDifference *= sign;
                                }
                            }
                            denominator = ASSETCHAINS_ENDSUBSIDY[curEra] - nStart;
                            numerator = denominator - ((ASSETCHAINS_ENDSUBSIDY[curEra] - nHeight) + ((nHeight - nStart) % ASSETCHAINS_HALVING[curEra]));
                            subsidy = subsidy - sign * ((subsidyDifference * numerator) / denominator);
                        }
                        else
                        {
                            if ( cached_subsidy > 0 && cached_era == curEra && cached_numhalvings == numhalvings )
                                subsidy = cached_subsidy;
                            else
                            {
                                for (int i=0; i < numhalvings && subsidy != 0; i++)
                                    subsidy = (subsidy * ASSETCHAINS_DECAY[curEra]) / 100000000;
                                cached_subsidy = subsidy;
                                cached_numhalvings = numhalvings;
                                cached_era = curEra;
                            }
                        }
                    }
                }
            }
        }
    }
    uint32_t magicExtra = ASSETCHAINS_STAKED ? ASSETCHAINS_MAGIC : (ASSETCHAINS_MAGIC & 0xffffff);
    if ( ASSETCHAINS_SUPPLY > 10000000000 ) // over 10 billion?
    {
        if ( nHeight <= ASSETCHAINS_SUPPLY/1000000000 )
        {
            subsidy += (uint64_t)1000000000 * COIN;
            if ( nHeight == 1 )
                subsidy += (ASSETCHAINS_SUPPLY % 1000000000)*COIN + magicExtra;
        }
    }
    else if ( nHeight == 1 )
    {
        if ( ASSETCHAINS_LASTERA == 0 )
            subsidy = ASSETCHAINS_SUPPLY * SATOSHIDEN + magicExtra;
        else
            subsidy += ASSETCHAINS_SUPPLY * SATOSHIDEN + magicExtra;
    }
    else if ( is_STAKED(ASSETCHAINS_SYMBOL) == 2 )
        return(0);
    // LABS fungible chains, cannot have any block reward!
    return(subsidy);
}

extern int64_t MAX_MONEY;
void komodo_cbopretupdate(int32_t forceflag);
void SplitStr(const std::string& strVal, std::vector<std::string> &outVals);

int8_t equihash_params_possible(uint64_t n, uint64_t k)
{
    /* To add more of these you also need to edit:
    * equihash.cpp very end of file with the tempate to point to the new param numbers 
    * equihash.h
    *  line 210/217 (declaration of equihash class)
    * Add this object to the following functions: 
    *  EhInitialiseState 
    *  EhBasicSolve
    *  EhOptimisedSolve
    *  EhIsValidSolution
    * Alternatively change ASSETCHAINS_N and ASSETCHAINS_K in komodo_nk.h for fast testing.
    */
    if ( k == 9 && (n == 200 || n == 210) )
        return(0);
    if ( k == 5 && (n == 150 || n == 144 || n == 96 || n == 48) )
        return(0);
    if ( k == ASSETCHAINS_K && n == ASSETCHAINS_N)
        return(0);
    return(-1);
}

/***
 * Parse command line arguments
 * @param argv0 the arguments
 */
void komodo_args(char *argv0)
{
    uint8_t extrabuf[32756], *extraptr = 0;
    int32_t i;
    int32_t n;
    int32_t nonz=0;
    int32_t extralen = 0; 
    uint64_t ccEnablesHeight[512] = {0}; 

    std::string ntz_dest_path;
    ntz_dest_path = GetArg("-notary", "");
    IS_KOMODO_NOTARY = !ntz_dest_path.empty();

    STAKED_NOTARY_ID = GetArg("-stakednotary", -1);
    KOMODO_NSPV = GetArg("-nSPV",0);
    uint8_t disablebits[32];
    memset(disablebits,0,sizeof(disablebits));
    memset(ccEnablesHeight,0,sizeof(ccEnablesHeight));
    if ( GetBoolArg("-gen", false) != 0 )
    {
        KOMODO_MININGTHREADS = GetArg("-genproclimit",-1);
    }
    if ( (IS_MODE_EXCHANGEWALLET = GetBoolArg("-exchange", false)) )
        LogPrintf("IS_MODE_EXCHANGEWALLET mode active\n");
    DONATION_PUBKEY = GetArg("-donation", "");
    NOTARY_PUBKEY = GetArg("-pubkey", "");
    IS_KOMODO_DEALERNODE = GetBoolArg("-dealer",false);
    IS_KOMODO_TESTNODE = GetArg("-testnode",0);
    ASSETCHAINS_STAKED_SPLIT_PERCENTAGE = GetArg("-splitperc",0);
    if ( NOTARY_PUBKEY.size() == 66 )
    {
        decode_hex(NOTARY_PUBKEY33,33,NOTARY_PUBKEY.c_str());
        USE_EXTERNAL_PUBKEY = 1;
        if ( !IS_KOMODO_NOTARY )
        {
            // We dont have any chain data yet, so use system clock to guess. 
            // I think on season change should reccomend notaries to use -notary to avoid needing this. 
            int32_t kmd_season = getacseason(time(NULL));
            for (i=0; i<64; i++)
            {
                if ( strcmp(NOTARY_PUBKEY.c_str(),notaries_elected[kmd_season-1][i][1]) == 0 )
                {
                    IS_KOMODO_NOTARY = true;
                    KOMODO_MININGTHREADS = 1;
                    mapArgs ["-genproclimit"] = itostr(KOMODO_MININGTHREADS);
                    STAKED_NOTARY_ID = -1;
                    LogPrintf("running as notary.%d %s\n",i,notaries_elected[kmd_season-1][i][0]);
                    break;
                }
            }
        }
    }
    if ( STAKED_NOTARY_ID != -1 && IS_KOMODO_NOTARY ) {
        LogPrintf( "Cannot be STAKED and KMD notary at the same time!\n");
        StartShutdown();
    }
	std::string name = GetArg("-ac_name","");
    if ( argv0 != 0 )
    {
        size_t len = strlen(argv0);
        for (i=0; i<sizeof(argv0suffix)/sizeof(*argv0suffix); i++)
        {
            n = (int32_t)strlen(argv0suffix[i]);
            if ( strcmp(&argv0[len - n],argv0suffix[i]) == 0 )
            {
                //LogPrintf("ARGV0.(%s) -> matches suffix (%s) -> ac_name.(%s)\n",argv0,argv0suffix[i],argv0names[i]);
                name = argv0names[i];
                break;
            }
        }
    }
    KOMODO_STOPAT = GetArg("-stopat",0);
    MAX_REORG_LENGTH = GetArg("-maxreorg",MAX_REORG_LENGTH);
    WITNESS_CACHE_SIZE = MAX_REORG_LENGTH+10;
    ASSETCHAINS_CC = GetArg("-ac_cc",0);
    KOMODO_CCACTIVATE = GetArg("-ac_ccactivate",0);
    ASSETCHAINS_BLOCKTIME = GetArg("-ac_blocktime",60);
    ASSETCHAINS_PUBLIC = GetArg("-ac_public",0);
    ASSETCHAINS_PRIVATE = GetArg("-ac_private",0);
    KOMODO_SNAPSHOT_INTERVAL = GetArg("-ac_snapshot",0);
    Split(GetArg("-ac_nk",""), sizeof(ASSETCHAINS_NK)/sizeof(*ASSETCHAINS_NK), ASSETCHAINS_NK, 0);
    
    // -ac_ccactivateht=evalcode,height,evalcode,height,evalcode,height....
    Split(GetArg("-ac_ccactivateht",""), sizeof(ccEnablesHeight)/sizeof(*ccEnablesHeight), ccEnablesHeight, 0);
    // fill map with all eval codes and activation height of 0.
    for ( int i = 0; i < 256; i++ )
        mapHeightEvalActivate[i] = 0;
    for ( int i = 0; i < 512; i++ )
    {
        int32_t ecode = ccEnablesHeight[i];
        int32_t ht = ccEnablesHeight[i+1];
        if ( i > 1 && ccEnablesHeight[i-2] == ecode )
            break;
        if ( ecode > 255 || ecode < 0 )
            LogPrintf( "ac_ccactivateht: invalid evalcode.%i must be between 0 and 256.\n", ecode);
        else if ( ht > 0 )
        {
            // update global map. 
            mapHeightEvalActivate[ecode] = ht;
            LogPrintf( "ac_ccactivateht: ecode.%i activates at height.%i\n", ecode, mapHeightEvalActivate[ecode]);
        }
        i++;
    }
    
    if ( (KOMODO_REWIND= GetArg("-rewind",0)) != 0 )
    {
        LogPrintf("KOMODO_REWIND %d\n",KOMODO_REWIND);
    }
    KOMODO_EARLYTXID = Parseuint256(GetArg("-earlytxid","0").c_str());    
    ASSETCHAINS_EARLYTXIDCONTRACT = GetArg("-ac_earlytxidcontract",0);
    if ( name.c_str()[0] != 0 )
    {
        std::string selectedAlgo = GetArg("-ac_algo", std::string(ASSETCHAINS_ALGORITHMS[0]));

        for ( int i = 0; i < ASSETCHAINS_NUMALGOS; i++ )
        {
            if (std::string(ASSETCHAINS_ALGORITHMS[i]) == selectedAlgo)
            {
                ASSETCHAINS_ALGO = i;
                STAKING_MIN_DIFF = ASSETCHAINS_MINDIFF[i];
                // only worth mentioning if it's not equihash
                if (ASSETCHAINS_ALGO != ASSETCHAINS_EQUIHASH)
                    LogPrintf("ASSETCHAINS_ALGO, algorithm set to %s\n", selectedAlgo.c_str());
                break;
            }
        }
        if ( ASSETCHAINS_ALGO == ASSETCHAINS_EQUIHASH && ASSETCHAINS_NK[0] != 0 && ASSETCHAINS_NK[1] != 0 )
        {
            if ( equihash_params_possible(ASSETCHAINS_NK[0], ASSETCHAINS_NK[1]) == -1 ) 
            {
                LogPrintf("equihash values N.%li and K.%li are not currently available\n", ASSETCHAINS_NK[0], ASSETCHAINS_NK[1]);
                exit(0);
            } else LogPrintf("ASSETCHAINS_ALGO, algorithm set to equihash with N.%li and K.%li\n", ASSETCHAINS_NK[0], ASSETCHAINS_NK[1]);
        }
        if (i == ASSETCHAINS_NUMALGOS)
        {
            LogPrintf("ASSETCHAINS_ALGO, %s not supported. using equihash\n", selectedAlgo.c_str());
        }

        ASSETCHAINS_LASTERA = GetArg("-ac_eras", 1);
        if ( ASSETCHAINS_LASTERA < 1 || ASSETCHAINS_LASTERA > ASSETCHAINS_MAX_ERAS )
        {
            ASSETCHAINS_LASTERA = 1;
            LogPrintf("ASSETCHAINS_LASTERA, if specified, must be between 1 and %u. ASSETCHAINS_LASTERA set to %lu\n", ASSETCHAINS_MAX_ERAS, ASSETCHAINS_LASTERA);
        }
        ASSETCHAINS_LASTERA -= 1;

        ASSETCHAINS_TIMELOCKGTE = (uint64_t)GetArg("-ac_timelockgte", _ASSETCHAINS_TIMELOCKOFF);
        ASSETCHAINS_TIMEUNLOCKFROM = GetArg("-ac_timeunlockfrom", 0);
        ASSETCHAINS_TIMEUNLOCKTO = GetArg("-ac_timeunlockto", 0);
        if ( ASSETCHAINS_TIMEUNLOCKFROM > ASSETCHAINS_TIMEUNLOCKTO )
        {
            LogPrintf("ASSETCHAINS_TIMELOCKGTE - must specify valid ac_timeunlockfrom and ac_timeunlockto\n");
            ASSETCHAINS_TIMELOCKGTE = _ASSETCHAINS_TIMELOCKOFF;
            ASSETCHAINS_TIMEUNLOCKFROM = ASSETCHAINS_TIMEUNLOCKTO = 0;
        }

        Split(GetArg("-ac_end",""), sizeof(ASSETCHAINS_ENDSUBSIDY)/sizeof(*ASSETCHAINS_ENDSUBSIDY),  ASSETCHAINS_ENDSUBSIDY, 0);
        Split(GetArg("-ac_reward",""), sizeof(ASSETCHAINS_REWARD)/sizeof(*ASSETCHAINS_REWARD),  ASSETCHAINS_REWARD, 0);
        Split(GetArg("-ac_halving",""), sizeof(ASSETCHAINS_HALVING)/sizeof(*ASSETCHAINS_HALVING),  ASSETCHAINS_HALVING, 0);
        Split(GetArg("-ac_decay",""), sizeof(ASSETCHAINS_DECAY)/sizeof(*ASSETCHAINS_DECAY),  ASSETCHAINS_DECAY, 0);
        Split(GetArg("-ac_notarypay",""), sizeof(ASSETCHAINS_NOTARY_PAY)/sizeof(*ASSETCHAINS_NOTARY_PAY),  ASSETCHAINS_NOTARY_PAY, 0);

        for ( int i = 0; i < ASSETCHAINS_MAX_ERAS; i++ )
        {
            if ( ASSETCHAINS_DECAY[i] == 100000000 && ASSETCHAINS_ENDSUBSIDY == 0 )
            {
                ASSETCHAINS_DECAY[i] = 0;
                LogPrintf("ERA%u: ASSETCHAINS_DECAY of 100000000 means linear and that needs ASSETCHAINS_ENDSUBSIDY\n", i);
            }
            else if ( ASSETCHAINS_DECAY[i] > 100000000 )
            {
                ASSETCHAINS_DECAY[i] = 0;
                LogPrintf("ERA%u: ASSETCHAINS_DECAY cant be more than 100000000\n", i);
            }
        }

        MAX_BLOCK_SIGOPS = 60000;
        ASSETCHAINS_TXPOW = GetArg("-ac_txpow",0) & 3;
        ASSETCHAINS_FOUNDERS = GetArg("-ac_founders",0);// & 1;
		ASSETCHAINS_FOUNDERS_REWARD = GetArg("-ac_founders_reward",0);
        ASSETCHAINS_SUPPLY = GetArg("-ac_supply",10);
        if ( ASSETCHAINS_SUPPLY > (uint64_t)90*1000*1000000 )
        {
            LogPrintf("-ac_supply must be less than 90 billion\n");
            StartShutdown();
        }
        LogPrintf("ASSETCHAINS_SUPPLY %llu\n",(long long)ASSETCHAINS_SUPPLY);
        
        ASSETCHAINS_COMMISSION = GetArg("-ac_perc",0);
        ASSETCHAINS_OVERRIDE_PUBKEY = GetArg("-ac_pubkey","");
        ASSETCHAINS_SCRIPTPUB = GetArg("-ac_script","");
        ASSETCHAINS_BEAMPORT = GetArg("-ac_beam",0);
        ASSETCHAINS_CODAPORT = GetArg("-ac_coda",0);
        ASSETCHAINS_CBOPRET = GetArg("-ac_cbopret",0);
        ASSETCHAINS_CBMATURITY = GetArg("-ac_cbmaturity",0);
        ASSETCHAINS_ADAPTIVEPOW = GetArg("-ac_adaptivepow",0);
        //fprintf(stderr,"ASSETCHAINS_CBOPRET.%llx\n",(long long)ASSETCHAINS_CBOPRET);
        if ( ASSETCHAINS_CBOPRET != 0 )
        {
            SplitStr(GetArg("-ac_prices",""),  ASSETCHAINS_PRICES);
            if ( ASSETCHAINS_PRICES.size() > 0 )
                ASSETCHAINS_CBOPRET |= 4;
            SplitStr(GetArg("-ac_stocks",""),  ASSETCHAINS_STOCKS);
            if ( ASSETCHAINS_STOCKS.size() > 0 )
                ASSETCHAINS_CBOPRET |= 8;
            for (i=0; i<ASSETCHAINS_PRICES.size(); i++)
                LogPrintf("%s ",ASSETCHAINS_PRICES[i].c_str());
            LogPrintf("%d -ac_prices\n",(int32_t)ASSETCHAINS_PRICES.size());
            for (i=0; i<ASSETCHAINS_STOCKS.size(); i++)
                LogPrintf("%s ",ASSETCHAINS_STOCKS[i].c_str());
            LogPrintf("%d -ac_stocks\n",(int32_t)ASSETCHAINS_STOCKS.size());
        }
        std::string hexstr = GetArg("-ac_mineropret","");
        if ( !hexstr.empty() )
        {
            Mineropret.resize(hexstr.size()/2);
            decode_hex(Mineropret.data(),hexstr.size()/2,hexstr.c_str());
            for (i=0; i<Mineropret.size(); i++)
                LogPrintf("%02x",Mineropret[i]);
            LogPrintf(" Mineropret\n");
        }
        if ( ASSETCHAINS_COMMISSION != 0 && ASSETCHAINS_FOUNDERS_REWARD != 0 )
        {
            LogPrintf("cannot use founders reward and commission on the same chain.\n");
            StartShutdown();
        }
        if ( ASSETCHAINS_CC != 0 )
        {
            uint8_t prevCCi = 0;
            uint64_t ccenables[256];
            memset(ccenables,0,sizeof(ccenables));
            ASSETCHAINS_CCLIB = GetArg("-ac_cclib","");
            Split(GetArg("-ac_ccenable",""), sizeof(ccenables)/sizeof(*ccenables),  ccenables, 0);
            for (i=nonz=0; i<0x100; i++)
            {
                if ( ccenables[i] != prevCCi && ccenables[i] != 0 )
                {
                    nonz++;
                    prevCCi = ccenables[i];
                    LogPrintf("%d ",(uint8_t)(ccenables[i] & 0xff));
                }
            }
            LogPrintf("nonz.%d ccenables[]\n",nonz);
            if ( nonz > 0 )
            {
                for (i=0; i<256; i++)
                {
                    ASSETCHAINS_CCDISABLES[i] = 1;
                    SETBIT(disablebits,i);
                }
                for (i=0; i<nonz; i++)
                {
                    CLEARBIT(disablebits,(ccenables[i] & 0xff));
                    ASSETCHAINS_CCDISABLES[ccenables[i] & 0xff] = 0;
                }
                CLEARBIT(disablebits,0);
            }
            /*if ( ASSETCHAINS_CCLIB.size() > 0 )
            {
                for (i=first; i<=last; i++)
                {
                    CLEARBIT(disablebits,i);
                    ASSETCHAINS_CCDISABLES[i] = 0;
                }
            }*/
        }
        if ( ASSETCHAINS_BEAMPORT != 0 )
        {
            LogPrintf("can only have one of -ac_beam or -ac_coda\n");
            StartShutdown();
        }
        ASSETCHAINS_SELFIMPORT = GetArg("-ac_import",""); // BEAM, CODA, PUBKEY, GATEWAY
        if ( ASSETCHAINS_SELFIMPORT == "PUBKEY" )
        {
            if ( strlen(ASSETCHAINS_OVERRIDE_PUBKEY.c_str()) != 66 )
            {
                LogPrintf("invalid -ac_pubkey for -ac_import=PUBKEY\n");
                StartShutdown();
            }
        }
        else if ( ASSETCHAINS_SELFIMPORT == "BEAM" )
        {
            if (ASSETCHAINS_BEAMPORT == 0)
            {
                fprintf(stderr,"missing -ac_beam for BEAM rpcport\n");
                StartShutdown();
            }
        }
        else if ( ASSETCHAINS_SELFIMPORT == "CODA" )
        {
            if (ASSETCHAINS_CODAPORT == 0)
            {
                fprintf(stderr,"missing -ac_coda for CODA rpcport\n");
                StartShutdown();
            }
        }
        else if ( ASSETCHAINS_SELFIMPORT == "PEGSCC")
        {
            Split(GetArg("-ac_pegsccparams",""), sizeof(ASSETCHAINS_PEGSCCPARAMS)/sizeof(*ASSETCHAINS_PEGSCCPARAMS), ASSETCHAINS_PEGSCCPARAMS, 0);
            if (ASSETCHAINS_ENDSUBSIDY[0]!=1 || ASSETCHAINS_COMMISSION!=0)
            {
                fprintf(stderr,"when using import for pegsCC these must be set: -ac_end=1 -ac_perc=0\n");
                StartShutdown();
            }
        }
        // else it can be gateway coin
        else if (!ASSETCHAINS_SELFIMPORT.empty() && (ASSETCHAINS_ENDSUBSIDY[0]!=1 || ASSETCHAINS_SUPPLY>0 || ASSETCHAINS_COMMISSION!=0))
        {
            LogPrintf("when using gateway import these must be set: -ac_end=1 -ac_supply=0 -ac_perc=0\n");
            StartShutdown();
        }
        

        if ( (ASSETCHAINS_STAKED= GetArg("-ac_staked",0)) > 100 )
            ASSETCHAINS_STAKED = 100;

        // for now, we only support 50% PoS due to other parts of the algorithm needing adjustment for
        // other values
        if ( (ASSETCHAINS_LWMAPOS = GetArg("-ac_veruspos",0)) != 0 )
        {
            ASSETCHAINS_LWMAPOS = 50;
        }
        ASSETCHAINS_SAPLING = GetArg("-ac_sapling", -1);
        if (ASSETCHAINS_SAPLING == -1)
        {
            ASSETCHAINS_OVERWINTER = GetArg("-ac_overwinter", -1);
        }
        else
        {
            ASSETCHAINS_OVERWINTER = GetArg("-ac_overwinter", ASSETCHAINS_SAPLING);
        }
        if ( strlen(ASSETCHAINS_OVERRIDE_PUBKEY.c_str()) == 66 || ASSETCHAINS_SCRIPTPUB.size() > 1 )
        {
            if ( ASSETCHAINS_SUPPLY > 10000000000 )
            {
                LogPrintf("ac_pubkey or ac_script wont work with ac_supply over 10 billion\n");
                StartShutdown();
            }
            if ( ASSETCHAINS_NOTARY_PAY[0] != 0 )
            {
                LogPrintf("Assetchains NOTARY PAY cannot be used with ac_pubkey or ac_script.\n");
                StartShutdown();
            }
            if ( strlen(ASSETCHAINS_OVERRIDE_PUBKEY.c_str()) == 66 )
            {
                decode_hex(ASSETCHAINS_OVERRIDE_PUBKEY33,33,ASSETCHAINS_OVERRIDE_PUBKEY.c_str());
                calc_rmd160_sha256(ASSETCHAINS_OVERRIDE_PUBKEYHASH,ASSETCHAINS_OVERRIDE_PUBKEY33,33);
            }
            if ( ASSETCHAINS_COMMISSION == 0 && ASSETCHAINS_FOUNDERS != 0 )
            {
                if ( ASSETCHAINS_FOUNDERS_REWARD == 0 )
                {
                    ASSETCHAINS_COMMISSION = 53846154; // maps to 35%
                    LogPrintf("ASSETCHAINS_COMMISSION defaulted to 35%% when founders reward active\n");
                }
                else
                {
                    LogPrintf("ASSETCHAINS_FOUNDERS_REWARD set to %ld\n", ASSETCHAINS_FOUNDERS_REWARD);
                }
                /*else if ( ASSETCHAINS_SELFIMPORT.size() == 0 )
                {
                    //ASSETCHAINS_OVERRIDE_PUBKEY.clear();
                    LogPrintf("-ac_perc must be set with -ac_pubkey\n");
                }*/
            }
        }
        else
        {
            if ( ASSETCHAINS_COMMISSION != 0 )
            {
                ASSETCHAINS_COMMISSION = 0;
                LogPrintf("ASSETCHAINS_COMMISSION needs an ASSETCHAINS_OVERRIDE_PUBKEY and cant be more than 100000000 (100%%)\n");
            }
            if ( ASSETCHAINS_FOUNDERS != 0 )
            {
                ASSETCHAINS_FOUNDERS = 0;
                LogPrintf("ASSETCHAINS_FOUNDERS needs an ASSETCHAINS_OVERRIDE_PUBKEY or ASSETCHAINS_SCRIPTPUB\n");
            }
        }
        if ( ASSETCHAINS_ENDSUBSIDY[0] != 0 || ASSETCHAINS_REWARD[0] != 0 || ASSETCHAINS_HALVING[0] != 0 
                || ASSETCHAINS_DECAY[0] != 0 || ASSETCHAINS_COMMISSION != 0 || ASSETCHAINS_PUBLIC != 0 
                || ASSETCHAINS_PRIVATE != 0 || ASSETCHAINS_TXPOW != 0 || ASSETCHAINS_FOUNDERS != 0 
                || ASSETCHAINS_SCRIPTPUB.size() > 1 || ASSETCHAINS_SELFIMPORT.size() > 0 
                || ASSETCHAINS_OVERRIDE_PUBKEY33[0] != 0 || ASSETCHAINS_TIMELOCKGTE != _ASSETCHAINS_TIMELOCKOFF
                || ASSETCHAINS_ALGO != ASSETCHAINS_EQUIHASH || ASSETCHAINS_LWMAPOS != 0 || ASSETCHAINS_LASTERA > 0 
                || ASSETCHAINS_BEAMPORT != 0 || ASSETCHAINS_CODAPORT != 0 || nonz > 0 
                || ASSETCHAINS_CCLIB.size() > 0 || ASSETCHAINS_FOUNDERS_REWARD != 0 || ASSETCHAINS_NOTARY_PAY[0] != 0 
                || ASSETCHAINS_BLOCKTIME != 60 || ASSETCHAINS_CBOPRET != 0 || Mineropret.size() != 0 
                || (ASSETCHAINS_NK[0] != 0 && ASSETCHAINS_NK[1] != 0) || KOMODO_SNAPSHOT_INTERVAL != 0 
                || ASSETCHAINS_EARLYTXIDCONTRACT != 0 || ASSETCHAINS_CBMATURITY != 0 || ASSETCHAINS_ADAPTIVEPOW != 0 )
        {
            LogPrintf("perc %.4f%% ac_pub=[%02x%02x%02x...] acsize.%d\n",dstr(ASSETCHAINS_COMMISSION)*100,
                    ASSETCHAINS_OVERRIDE_PUBKEY33[0],ASSETCHAINS_OVERRIDE_PUBKEY33[1],ASSETCHAINS_OVERRIDE_PUBKEY33[2],
                    (int32_t)ASSETCHAINS_SCRIPTPUB.size());
            extraptr = extrabuf;
            memcpy(extraptr,ASSETCHAINS_OVERRIDE_PUBKEY33,33), extralen = 33;

            // if we have one era, this should create the same data structure as it used to, same if we increase _MAX_ERAS
            for ( int i = 0; i <= ASSETCHAINS_LASTERA; i++ )
            {
                LogPrintf("ERA%u: end.%llu reward.%llu halving.%llu decay.%llu notarypay.%llu\n", i,
                       (long long)ASSETCHAINS_ENDSUBSIDY[i],
                       (long long)ASSETCHAINS_REWARD[i],
                       (long long)ASSETCHAINS_HALVING[i],
                       (long long)ASSETCHAINS_DECAY[i],
                       (long long)ASSETCHAINS_NOTARY_PAY[i]);

                // TODO: Verify that we don't overrun extrabuf here, which is a 256 byte buffer
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_ENDSUBSIDY[i]),(void *)&ASSETCHAINS_ENDSUBSIDY[i]);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_REWARD[i]),(void *)&ASSETCHAINS_REWARD[i]);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_HALVING[i]),(void *)&ASSETCHAINS_HALVING[i]);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_DECAY[i]),(void *)&ASSETCHAINS_DECAY[i]);
                if ( ASSETCHAINS_NOTARY_PAY[0] != 0 )
                    extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_NOTARY_PAY[i]),(void *)&ASSETCHAINS_NOTARY_PAY[i]);
            }

            if (ASSETCHAINS_LASTERA > 0)
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_LASTERA),(void *)&ASSETCHAINS_LASTERA);
            }

            // hash in lock above for time locked coinbase transactions above a certain reward value only if the lock above
            // param was specified, otherwise, for compatibility, do nothing
            if ( ASSETCHAINS_TIMELOCKGTE != _ASSETCHAINS_TIMELOCKOFF )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_TIMELOCKGTE),(void *)&ASSETCHAINS_TIMELOCKGTE);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_TIMEUNLOCKFROM),(void *)&ASSETCHAINS_TIMEUNLOCKFROM);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_TIMEUNLOCKTO),(void *)&ASSETCHAINS_TIMEUNLOCKTO);
            }

            if ( ASSETCHAINS_ALGO != ASSETCHAINS_EQUIHASH )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_ALGO),(void *)&ASSETCHAINS_ALGO);
            }

            if ( ASSETCHAINS_LWMAPOS != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_LWMAPOS),(void *)&ASSETCHAINS_LWMAPOS);
            }

            uint64_t val = ASSETCHAINS_COMMISSION | (((int64_t)ASSETCHAINS_STAKED & 0xff) << 32) | (((uint64_t)ASSETCHAINS_CC & 0xffff) << 40) | ((ASSETCHAINS_PUBLIC != 0) << 7) | ((ASSETCHAINS_PRIVATE != 0) << 6) | ASSETCHAINS_TXPOW;
            extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(val),(void *)&val);
            
            if ( ASSETCHAINS_FOUNDERS != 0 )
            {
                uint8_t tmp = 1;
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(tmp),(void *)&tmp);
                if ( ASSETCHAINS_FOUNDERS > 1 )
                    extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_FOUNDERS),(void *)&ASSETCHAINS_FOUNDERS);
                if ( ASSETCHAINS_FOUNDERS_REWARD != 0 )
                {
                    LogPrintf( "set founders reward.%li\n",ASSETCHAINS_FOUNDERS_REWARD);
                    extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_FOUNDERS_REWARD),(void *)&ASSETCHAINS_FOUNDERS_REWARD);
                }
            }
            if ( ASSETCHAINS_SCRIPTPUB.size() > 1 )
            {
                decode_hex(&extraptr[extralen],ASSETCHAINS_SCRIPTPUB.size()/2,ASSETCHAINS_SCRIPTPUB.c_str());
                extralen += ASSETCHAINS_SCRIPTPUB.size()/2;
                //extralen += iguana_rwnum(1,&extraptr[extralen],(int32_t)ASSETCHAINS_SCRIPTPUB.size(),(void *)ASSETCHAINS_SCRIPTPUB.c_str());
                LogPrintf("append ac_script %s\n",ASSETCHAINS_SCRIPTPUB.c_str());
            }
            if ( ASSETCHAINS_SELFIMPORT.size() > 0 )
            {
                memcpy(&extraptr[extralen],(char *)ASSETCHAINS_SELFIMPORT.c_str(),ASSETCHAINS_SELFIMPORT.size());
                for (i=0; i<ASSETCHAINS_SELFIMPORT.size(); i++)
                    LogPrintf("%c",extraptr[extralen+i]);
                LogPrintf(" selfimport\n");
                extralen += ASSETCHAINS_SELFIMPORT.size();
            }
            if ( ASSETCHAINS_BEAMPORT != 0 )
                extraptr[extralen++] = 'b';
            if ( ASSETCHAINS_CODAPORT != 0 )
                extraptr[extralen++] = 'c';
            LogPrintf("extralen.%d before disable bits\n",extralen);
            if ( nonz > 0 )
            {
                memcpy(&extraptr[extralen],disablebits,sizeof(disablebits));
                extralen += sizeof(disablebits);
            }
            if ( ASSETCHAINS_CCLIB.size() > 1 )
            {
                for (i=0; i<ASSETCHAINS_CCLIB.size(); i++)
                {
                    extraptr[extralen++] = ASSETCHAINS_CCLIB[i];
                    LogPrintf("%c",ASSETCHAINS_CCLIB[i]);
                }
                LogPrintf(" <- CCLIB name\n");
            }
            if ( ASSETCHAINS_BLOCKTIME != 60 )
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_BLOCKTIME),(void *)&ASSETCHAINS_BLOCKTIME);
            if ( Mineropret.size() != 0 )
            {
                for (i=0; i<Mineropret.size(); i++)
                    extraptr[extralen++] = Mineropret[i];
            }
            if ( ASSETCHAINS_CBOPRET != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_CBOPRET),(void *)&ASSETCHAINS_CBOPRET);
                if ( ASSETCHAINS_PRICES.size() != 0 )
                {
                    for (i=0; i<ASSETCHAINS_PRICES.size(); i++)
                    {
                        std::string symbol = ASSETCHAINS_PRICES[i];
                        memcpy(&extraptr[extralen],(char *)symbol.c_str(),symbol.size());
                        extralen += symbol.size();
                    }
                }
                if ( ASSETCHAINS_STOCKS.size() != 0 )
                {
                    for (i=0; i<ASSETCHAINS_STOCKS.size(); i++)
                    {
                        std::string symbol = ASSETCHAINS_STOCKS[i];
                        memcpy(&extraptr[extralen],(char *)symbol.c_str(),symbol.size());
                        extralen += symbol.size();
                    }
                }
                //komodo_pricesinit();
                komodo_cbopretupdate(1); // will set Mineropret
                LogPrintf("This blockchain uses data produced from CoinDesk Bitcoin Price Index\n");
            }
            if ( ASSETCHAINS_NK[0] != 0 && ASSETCHAINS_NK[1] != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_NK[0]),(void *)&ASSETCHAINS_NK[0]);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_NK[1]),(void *)&ASSETCHAINS_NK[1]);
            }
            if ( KOMODO_SNAPSHOT_INTERVAL != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(KOMODO_SNAPSHOT_INTERVAL),(void *)&KOMODO_SNAPSHOT_INTERVAL);
            }
            if ( ASSETCHAINS_EARLYTXIDCONTRACT != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_EARLYTXIDCONTRACT),(void *)&ASSETCHAINS_EARLYTXIDCONTRACT);
            }
            if ( ASSETCHAINS_CBMATURITY != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_CBMATURITY),(void *)&ASSETCHAINS_CBMATURITY);
            }
            if ( ASSETCHAINS_ADAPTIVEPOW != 0 )
                extraptr[extralen++] = ASSETCHAINS_ADAPTIVEPOW;
        }
        
        std::string addn = GetArg("-seednode","");
        if ( !addn.empty() )
            ASSETCHAINS_SEED = 1;

        strncpy(ASSETCHAINS_SYMBOL,name.c_str(),sizeof(ASSETCHAINS_SYMBOL)-1);

        MAX_MONEY = komodo_max_money();

        int32_t baseid;
        if ( (baseid = komodo_baseid(ASSETCHAINS_SYMBOL)) >= 0 && baseid < 32 )
        {
            LogPrintf("baseid.%d MAX_MONEY.%s %.8f\n",baseid,ASSETCHAINS_SYMBOL,(double)MAX_MONEY/SATOSHIDEN);
        }

        if ( ASSETCHAINS_CC >= KOMODO_FIRSTFUNGIBLEID && MAX_MONEY < 1000000LL*SATOSHIDEN )
            MAX_MONEY = 1000000LL*SATOSHIDEN;
        if ( KOMODO_BIT63SET(MAX_MONEY) != 0 )
            MAX_MONEY = KOMODO_MAXNVALUE;
        LogPrintf("MAX_MONEY %llu %.8f\n",(long long)MAX_MONEY,(double)MAX_MONEY/SATOSHIDEN);
        uint16_t tmpport = komodo_port(ASSETCHAINS_SYMBOL,ASSETCHAINS_SUPPLY,&ASSETCHAINS_MAGIC,extraptr,extralen);
        if ( GetArg("-port",0) != 0 )
        {
            ASSETCHAINS_P2PPORT = GetArg("-port",0);
            LogPrintf("set p2pport.%u\n",ASSETCHAINS_P2PPORT);
        } else ASSETCHAINS_P2PPORT = tmpport;

        char *dirname;
        while ( (dirname = (char *)GetDataDir(false).string().c_str()) == 0 || dirname[0] == 0 )
        {
            LogPrintf("waiting for datadir (%s)\n",dirname);
#ifndef _WIN32
            sleep(3);
#else
            boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
#endif
        }
        if ( ASSETCHAINS_SYMBOL[0] != 0 )
        {
            int32_t komodo_baseid(char *origbase);
            extern int COINBASE_MATURITY;
            if ( strcmp(ASSETCHAINS_SYMBOL,"KMD") == 0 )
            {
                LogPrintf("cant have assetchain named KMD\n");
                StartShutdown();
            }
            uint16_t port;
            if ( (port= komodo_userpass(ASSETCHAINS_USERPASS,ASSETCHAINS_SYMBOL)) != 0 )
                ASSETCHAINS_RPCPORT = port;
            else komodo_configfile(ASSETCHAINS_SYMBOL,ASSETCHAINS_P2PPORT + 1);

            if (ASSETCHAINS_CBMATURITY != 0)
                COINBASE_MATURITY = ASSETCHAINS_CBMATURITY;
            else if (ASSETCHAINS_LASTERA == 0 || is_STAKED(ASSETCHAINS_SYMBOL) != 0)
                COINBASE_MATURITY = 1;
            if (COINBASE_MATURITY < 1)
            {
                LogPrintf("ac_cbmaturity must be >0, shutting down\n");
                StartShutdown();
            }
            //fprintf(stderr,"ASSETCHAINS_RPCPORT (%s) %u\n",ASSETCHAINS_SYMBOL,ASSETCHAINS_RPCPORT);
        }
        if ( ASSETCHAINS_RPCPORT == 0 )
            ASSETCHAINS_RPCPORT = ASSETCHAINS_P2PPORT + 1;
        uint8_t magic[4];
        iguana_rwnum(1,magic,sizeof(ASSETCHAINS_MAGIC),(void *)&ASSETCHAINS_MAGIC);
#ifndef FROM_CLI
        std::string fname = std::string(ASSETCHAINS_SYMBOL) + "_7776";
        FILE* fp;
        if ( (fp= fopen(fname.c_str(),"wb")) != 0 )
        {
            char magicstr[9];         
            for (i=0; i<4; i++)
                sprintf(&magicstr[i<<1],"%02x",magic[i]);
            magicstr[8] = 0;
            int8_t notarypay = 0;
            if ( ASSETCHAINS_NOTARY_PAY[0] != 0 )
                notarypay = 1;
            fprintf(fp,iguanafmtstr,name.c_str(),name.c_str(),name.c_str(),name.c_str(),magicstr,ASSETCHAINS_P2PPORT,ASSETCHAINS_RPCPORT,"78.47.196.146",notarypay);
            fclose(fp);
        } else LogPrintf("error creating (%s)\n",fname.c_str());
#endif
        if ( ASSETCHAINS_CC < 2 )
        {
            if ( KOMODO_CCACTIVATE != 0 )
            {
                ASSETCHAINS_CC = 2;
                fprintf(stderr,"smart utxo CC contracts will activate at height.%d\n",KOMODO_CCACTIVATE);
            }
            else if ( ccEnablesHeight[0] != 0 )
            {
                ASSETCHAINS_CC = 2;
                fprintf(stderr,"smart utxo CC contract %d will activate at height.%d\n",(int32_t)ccEnablesHeight[0],(int32_t)ccEnablesHeight[1]);
            }
        }
    }
    else
    {
        char fname[512],username[512],password[4096]; 
        int32_t iter; 
        FILE *fp;
        ASSETCHAINS_P2PPORT = 7770;
        ASSETCHAINS_RPCPORT = 7771;
        for (iter=0; iter<2; iter++)
        {
            strcpy(fname,GetDataDir().string().c_str());
#ifdef _WIN32
            while ( fname[strlen(fname)-1] != '\\' )
                fname[strlen(fname)-1] = 0;
            if ( iter == 0 )
                strcat(fname,"Komodo\\komodo.conf");
            else strcat(fname,ntz_dest_path.c_str());
#else
            while ( fname[strlen(fname)-1] != '/' )
                fname[strlen(fname)-1] = 0;
#ifdef __APPLE__
            if ( iter == 0 )
                strcat(fname,"Komodo/Komodo.conf");
            else strcat(fname,ntz_dest_path.c_str());
#else
            if ( iter == 0 )
                strcat(fname,".komodo/komodo.conf");
            else strcat(fname,ntz_dest_path.c_str());
#endif
#endif
            if ( (fp= fopen(fname,"rb")) != 0 )
            {
                uint16_t dest_rpc_port = _komodo_userpass(username,password,fp);
                DEST_PORT = iter == 1 ? dest_rpc_port : 0;
                sprintf(iter == 0 ? KMDUSERPASS : BTCUSERPASS,"%s:%s",username,password);
                fclose(fp);
            } else LogPrintf("couldnt open.(%s) will not validate dest notarizations\n",fname);
            if ( !IS_KOMODO_NOTARY )
                break;
        }
    }
    int32_t dpowconfs = KOMODO_DPOWCONFS;
    if ( ASSETCHAINS_SYMBOL[0] != 0 )
    {
        BITCOIND_RPCPORT = GetArg("-rpcport", ASSETCHAINS_RPCPORT);
        if ( strcmp("PIRATE",ASSETCHAINS_SYMBOL) == 0 && ASSETCHAINS_HALVING[0] == 77777 )
        {
            ASSETCHAINS_HALVING[0] *= 5;
            LogPrintf("PIRATE halving changed to %d %.1f days ASSETCHAINS_LASTERA.%lu\n",(int32_t)ASSETCHAINS_HALVING[0],(double)ASSETCHAINS_HALVING[0]/1440,ASSETCHAINS_LASTERA);
        }
        else if ( strcmp("VRSC",ASSETCHAINS_SYMBOL) == 0 )
            dpowconfs = 0;
        else if ( ASSETCHAINS_PRIVATE != 0 )
        {
            LogPrintf("-ac_private for a non-PIRATE chain is not supported. The only reason to have an -ac_private chain is for total privacy and that is best achieved with the largest anon set. PIRATE has that and it is recommended to just use PIRATE\n");
            StartShutdown();
        }
        // Set cc enables for all existing ac_cc chains here. 
        if ( strcmp("AXO",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet.
            CCDISABLEALL;
        }
        if ( strcmp("CCL",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
            CCENABLE(EVAL_TOKENS);
            CCENABLE(EVAL_HEIR);
        }
        if ( strcmp("COQUI",ASSETCHAINS_SYMBOL) == 0 )
        {
            CCDISABLEALL;
            CCENABLE(EVAL_DICE);
            CCENABLE(EVAL_CHANNELS);
            CCENABLE(EVAL_ORACLES);
            CCENABLE(EVAL_ASSETS);
            CCENABLE(EVAL_TOKENS);
        }
        if ( strcmp("DION",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        
        if ( strcmp("EQL",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        if ( strcmp("ILN",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        if ( strcmp("OUR",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        if ( strcmp("ZEXO",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        if ( strcmp("SEC",ASSETCHAINS_SYMBOL) == 0 )
        {
            CCDISABLEALL;
            CCENABLE(EVAL_ASSETS);
            CCENABLE(EVAL_TOKENS);
            CCENABLE(EVAL_ORACLES);
        }
        if ( strcmp("KMDICE",ASSETCHAINS_SYMBOL) == 0 )
        {
            CCDISABLEALL;
            CCENABLE(EVAL_FAUCET);
            CCENABLE(EVAL_DICE);
            CCENABLE(EVAL_ORACLES);
        }
    } else BITCOIND_RPCPORT = GetArg("-rpcport", BaseParams().RPCPort());
    KOMODO_DPOWCONFS = GetArg("-dpowconfs",dpowconfs);
    if ( ASSETCHAINS_SYMBOL[0] == 0 || strcmp(ASSETCHAINS_SYMBOL,"SUPERNET") == 0 || strcmp(ASSETCHAINS_SYMBOL,"DEX") == 0 || strcmp(ASSETCHAINS_SYMBOL,"COQUI") == 0 || strcmp(ASSETCHAINS_SYMBOL,"PIRATE") == 0 || strcmp(ASSETCHAINS_SYMBOL,"KMDICE") == 0 )
        KOMODO_EXTRASATOSHI = 1;
}

void komodo_nameset(char *symbol,char *dest,char *source)
{
    if ( source[0] == 0 )
    {
        strcpy(symbol,(char *)"KMD");
        strcpy(dest,(char *)"BTC");
    }
    else
    {
        strcpy(symbol,source);
        strcpy(dest,(char *)"KMD");
    }
}

struct komodo_state *komodo_stateptrget(char *base)
{
    int32_t baseid;
    if ( base == 0 || base[0] == 0 || strcmp(base,(char *)"KMD") == 0 )
        return(&KOMODO_STATES[33]);
    else if ( (baseid= komodo_baseid(base)) >= 0 )
        return(&KOMODO_STATES[baseid+1]);
    else return(&KOMODO_STATES[0]);
}

struct komodo_state *komodo_stateptr(char *symbol,char *dest)
{
    int32_t baseid;
    komodo_nameset(symbol,dest,ASSETCHAINS_SYMBOL);
    return(komodo_stateptrget(symbol));
}

/*
void komodo_prefetch(FILE *fp)
{
    long fsize,fpos; int32_t incr = 16*1024*1024;
    fpos = ftell(fp);
    fseek(fp,0,SEEK_END);
    fsize = ftell(fp);
    if ( fsize > incr )
    {
        char *ignore = (char *)malloc(incr);
        if ( ignore != 0 )
        {
            rewind(fp);
            while ( fread(ignore,1,incr,fp) == incr ) // prefetch
                {
                    // LogPrintf(".");
                }
            free(ignore);
        }
    }
    fseek(fp,fpos,SEEK_SET);
}
*/

// check if block timestamp is more than S5 activation time
// this function is to activate the ExtractDestination fix
bool komodo_is_vSolutionsFixActive()
{
    return GetLatestTimestamp(komodo_currentheight()) > nS5Timestamp;
}