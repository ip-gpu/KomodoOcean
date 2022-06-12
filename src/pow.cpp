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

#include "pow.h"
#include "consensus/upgrades.h"

#include "arith_uint256.h"
#include "chain.h"
#include "chainparams.h"
#include "crypto/equihash.h"
#include "primitives/block.h"
#include "streams.h"
#include "uint256.h"
#include "util.h"

#include "sodium.h"

#ifdef ENABLE_RUST
#include "librustzcash.h"
#endif // ENABLE_RUST
uint32_t komodo_chainactive_timestamp();

#include "komodo_defs.h"

/* from zawy repo
 Preliminary code for super-fast increases in difficulty.
 Requires the ability to change the difficulty during the current block,
 based on the timestamp the miner selects. See my github issue #36 and KMD.
 Needs intr-block exponential decay function because
 this can make difficulty jump very high.
 Miners need to caclulate new difficulty with each second, or
 maybe 3 seconds.  FTL, MTP, and revert to local times must be small.
 MTP=1 if using Digishield. Out-of-sequence timestamps must be forbidden.
 1) bnTarget = Digishield() or other baseline DA
 2) bnTarget = RT_CST_RST()
 3) bnTarget = max(bnTarget,expdecay())
 RT_CST_RST() multiplies Recent Target(s), Current Solvetimes, &
 Recent SolveTime if RST had an unlikely 1/200 block chance of
 being too fast on accident. This estimates and adjusts for recent
 hashrate aggressively (lots of random error) but corrects the error by
 CST adjusting the difficulty during the block.
 It checks to see if there was an "active trigger" still in play which
 occurs when recent block emission rate has been too fast. Triggers
 are supposed to be active if emission rate has not slowed up enough
 to get back on track. It checks the longest range first because it's
 the least aggressive.
 T = target blocktime
 ts = timestamp vector, 62 elements, 62 is oldest  (elements needed are 50+W)
 ct = cumulative targets, 62 elements, 62 is oldest
 W = window size of recent solvetimes and targets to use that estimates hashrate
 numerator & deonominator needed for 1/200 possion estimator
 past = how far back in past to look for beginning of a trigger
 */

/* create ts and cw vectors
// Get bnTarget = Digishield();

arith_uint256 past = 50;

arith_uint256 W = 12;
arith_uint256 numerator = 12;
arith_uint256 denominator = 7;

// bnTarget = RT_CST_RST (bnTarget, ts, cw, numerator, denominator, W, T, past);

W = 6; top = 7; denominator = 3;

// bnTarget = RT_CST_RST (bnTarget, ts, cw, numerator, denominator, W, T, past);

W = 3; top = 1; denominator = 2;

bnTarget = RT_CST_RST (bnTarget, ts, cw, numerator, denominator, W, T, past);
*/

#define T ASSETCHAINS_BLOCKTIME
#define K ((int64_t)1000000)

#ifdef original_algo
arith_uint256 oldRT_CST_RST(int32_t height,uint32_t nTime,arith_uint256 bnTarget,uint32_t *ts,arith_uint256 *ct,int32_t numerator,int32_t denominator,int32_t W,int32_t past)
{
    //if (ts.size() < 2*W || ct.size() < 2*W ) { exit; } // error. a vector was too small
    //if (ts.size() < past+W || ct.size() < past+W ) { past = min(ct.size(), ts.size()) - W; } // past was too small, adjust
    int64_t altK; int32_t i,j,k,ii=0; // K is a scaling factor for integer divisions
    if ( height < 64 )
        return(bnTarget);
    //if ( ((ts[0]-ts[W]) * W * 100)/(W-1) < (T * numerator * 100)/denominator )
    if ( (ts[0] - ts[W]) < (T * numerator)/denominator )
    {
        //bnTarget = ((ct[0]-ct[1])/K) * max(K,(K*(nTime-ts[0])*(ts[0]-ts[W])*denominator/numerator)/T/T);
        bnTarget = ct[0] / arith_uint256(K);
        //altK = (K * (nTime-ts[0]) * (ts[0]-ts[W]) * denominator * W) / (numerator * (W-1) * (T * T));
        altK = (K * (nTime-ts[0]) * (ts[0]-ts[W]) * denominator) / (numerator * (T * T));
        LogPrint("pow", "ht.%d initial altK.%lld %d * %d * %d / %d\n",height,(long long)altK,(nTime-ts[0]),(ts[0]-ts[W]),denominator,numerator);
        if ( altK > K )
            altK = K;
        bnTarget *= arith_uint256(altK);
        if ( altK < K )
            return(bnTarget);
    }
    /*  Check past 24 blocks for any sum of 3 STs < T/2 triggers. This is messy
     because the blockchain does not allow us to store a variable to know
     if we are currently in a triggered state that is making a sequence of
     adjustments to prevTargets, so we have to look for them.
     Nested loops do this: if block emission has not slowed to be back on track at
     any time since most recent trigger and we are at current block, aggressively
     adust prevTarget. */
    
    for (j=past-1; j>=2; j--)
    {
        if ( ts[j]-ts[j+W] < T*numerator/denominator )
        {
            ii = 0;
            for (i=j-2; i>=0; i--)
            {
                ii++;
                // Check if emission caught up. If yes, "trigger stopped at i".
                // Break loop to try more recent j's to see if trigger activates again.
                if ( (ts[i] - ts[j+W]) > (ii+W)*T )
                    break;
                
                // We're here, so there was a TS[j]-TS[j-3] < T/2 trigger in the past and emission rate has not yet slowed up to be back on track so the "trigger is still active", aggressively adjusting target here at block "i"
                if ( i == 0 )
                {
                    /* We made it all the way to current block. Emission rate since
                     last trigger never slowed enough to get back on track, so adjust again.
                     If avg last 3 STs = T, this increases target to prevTarget as ST increases to T.
                     This biases it towards ST=~1.75*T to get emission back on track.
                     If avg last 3 STs = T/2, target increases to prevTarget at 2*T.
                     Rarely, last 3 STs can be 1/2 speed => target = prevTarget at T/2, & 1/2 at T.*/
                    
                    //bnTarget = ((ct[0]-ct[W])/W/K) * (K*(nTime-ts[0])*(ts[0]-ts[W]))/W/T/T;
                    bnTarget = ct[0];
                    for (k=1; k<W; k++)
                        bnTarget += ct[k];
                    bnTarget /= arith_uint256(W * K);
                    altK = (K * (nTime-ts[0]) * (ts[0]-ts[W])) / (W * T * T);
                    LogPrint("pow", "ht.%d made it to i == 0, j.%d ii.%d altK %lld (%d * %d) %u - %u W.%d\n",height,j,ii,(long long)altK,(nTime-ts[0]),(ts[0]-ts[W]),ts[0],ts[W],W);
                    bnTarget *= arith_uint256(altK);
                    j = 0; // It needed adjusting, we adjusted it, we're finished, so break out of j loop.
                }
            }
        }
    }
    return(bnTarget);
}
#endif

arith_uint256 RT_CST_RST_outer(int32_t height,uint32_t nTime,arith_uint256 bnTarget,uint32_t *ts,arith_uint256 *ct,int32_t numerator,int32_t denominator,int32_t W,int32_t past)
{
    int64_t outerK; int32_t cmpval; arith_uint256 mintarget = bnTarget / arith_uint256(2);
    cmpval = (T * numerator)/denominator;
    if ( cmpval < 2 )
        cmpval = 2;
    if ( (ts[0] - ts[W]) < cmpval )
    {
        outerK = (K * (nTime-ts[0]) * (ts[0]-ts[W]) * denominator) / (numerator * (T * T));
        if ( outerK < K )
        {
            bnTarget = ct[0] / arith_uint256(K);
            bnTarget *= arith_uint256(outerK);
        }
        if ( bnTarget > mintarget )
            bnTarget = mintarget;
        {
            int32_t z;
            for (z=31; z>=0; z--)
                LogPrint("pow", "%02x",((uint8_t *)&bnTarget)[z]);
        }
        LogPrint("pow", " ht.%d initial W.%d outerK.%lld %d * %d * %d / %d\n",height,W,(long long)outerK,(nTime-ts[0]),(ts[0]-ts[W]),denominator,numerator);
    } //else LogPrint("pow", "ht.%d no outer trigger %d >= %d\n",height,(ts[0] - ts[W]),(T * numerator)/denominator);
    return(bnTarget);
}

arith_uint256 RT_CST_RST_target(int32_t height,uint32_t nTime,arith_uint256 bnTarget,uint32_t *ts,arith_uint256 *ct,int32_t width)
{
    int32_t i; int64_t innerK;
    bnTarget = ct[0];
    for (i=1; i<width; i++)
        bnTarget += ct[i];
    bnTarget /= arith_uint256(width * K);
    innerK = (K * (nTime-ts[0]) * (ts[0]-ts[width])) / (width * T * T);
    bnTarget *= arith_uint256(innerK);
    if ( 0 )
    {
        int32_t z;
        for (z=31; z>=0; z--)
            LogPrint("pow", "%02x",((uint8_t *)&bnTarget)[z]);
        LogPrint("pow", " ht.%d innerK %lld (%d * %d) %u - %u width.%d\n",height,(long long)innerK,(nTime-ts[0]),(ts[0]-ts[width]),ts[0],ts[width],width);
    }
    return(bnTarget);
}

arith_uint256 RT_CST_RST_inner(int32_t height,uint32_t nTime,arith_uint256 bnTarget,uint32_t *ts,arith_uint256 *ct,int32_t W,int32_t outeri)
{
    int32_t expected,elapsed,width = outeri+W; arith_uint256 mintarget,origtarget;
    expected = (width+1) * T;
    origtarget = bnTarget;
    if ( (elapsed= (ts[0] - ts[width])) < expected )
    {
        mintarget = (bnTarget / arith_uint256(101)) * arith_uint256(100);
        bnTarget = RT_CST_RST_target(height,nTime,bnTarget,ts,ct,W);
        if ( bnTarget == origtarget ) // force zawyflag to 1
            bnTarget = mintarget;
        {
            int32_t z;
            for (z=31; z>=0; z--)
                LogPrint("pow", "%02x",((uint8_t *)&bnTarget)[z]);
        }
        LogPrint("pow", " height.%d O.%-2d, W.%-2d width.%-2d %4d vs %-4d, deficit %4d tip.%d\n",height,outeri,W,width,(ts[0] - ts[width]),expected,expected - (ts[0] - ts[width]),nTime-ts[0]);
    }
    return(bnTarget);
}

arith_uint256 zawy_targetMA(arith_uint256 easy,arith_uint256 bnSum,int32_t num,int32_t numerator,int32_t divisor)
{
    bnSum /= arith_uint256(ASSETCHAINS_BLOCKTIME * num * num * divisor);
    bnSum *= arith_uint256(numerator);
    if ( bnSum > easy )
        bnSum = easy;
    return(bnSum);
}

int64_t zawy_exponential_val360000(int32_t num)
{
    int32_t i,n,modval; int64_t A = 1, B = 3600 * 100;
    if ( (n= (num/ASSETCHAINS_BLOCKTIME)) > 0 )
    {
        for (i=1; i<=n; i++)
            A *= 3;
    }
    if ( (modval= (num % ASSETCHAINS_BLOCKTIME)) != 0 )
    {
        B += (3600 * 110 * modval) / ASSETCHAINS_BLOCKTIME;
        B += (3600 * 60 * modval * modval) / (ASSETCHAINS_BLOCKTIME * ASSETCHAINS_BLOCKTIME);
    }
    return(A * B);
}

arith_uint256 zawy_exponential(arith_uint256 bnTarget,int32_t mult)
{
    bnTarget /= arith_uint256(100 * 3600);
    bnTarget *= arith_uint256(zawy_exponential_val360000(mult));
    return(bnTarget);
}

arith_uint256 zawy_ctB(arith_uint256 bnTarget,uint32_t solvetime)
{
    int64_t num;
    num = ((int64_t)1000 * solvetime * solvetime * 1000) / (T * T * 784);
    if ( num > 1 )
    {
        bnTarget /= arith_uint256(1000);
        bnTarget *= arith_uint256(num);
    }
    return(bnTarget);
}

arith_uint256 zawy_TSA_EMA(int32_t height,int32_t tipdiff,arith_uint256 prevTarget,int32_t solvetime)
{
    arith_uint256 A,B,C,bnTarget;
    if ( tipdiff < 4 )
        tipdiff = 4;
    tipdiff &= ~1;
    bnTarget = prevTarget / arith_uint256(K*T);
    A = bnTarget * arith_uint256(T);
    B = (bnTarget / arith_uint256(360000)) * arith_uint256(tipdiff * zawy_exponential_val360000(tipdiff/2));
    C = (bnTarget / arith_uint256(360000)) * arith_uint256(T * zawy_exponential_val360000(tipdiff/2));
    bnTarget = ((A + B - C) / arith_uint256(tipdiff)) * arith_uint256(K*T);
    {
        int32_t z;
        for (z=31; z>=0; z--)
            LogPrint("pow", "%02x",((uint8_t *)&bnTarget)[z]);
    }
    LogPrint("pow", " ht.%d TSA bnTarget tipdiff.%d\n",height,tipdiff);
    return(bnTarget);
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    arith_uint256 bnLimit;
    if (ASSETCHAINS_ALGO == ASSETCHAINS_EQUIHASH)
        bnLimit = UintToArith256(params.powLimit);
    else
        bnLimit = UintToArith256(params.powAlternate);
    unsigned int nProofOfWorkLimit = bnLimit.GetCompact();
    // Genesis block
    if (pindexLast == NULL )
        return nProofOfWorkLimit;

    //{
        // Comparing to pindexLast->nHeight with >= because this function
        // returns the work required for the block after pindexLast.
        //if (params.nPowAllowMinDifficultyBlocksAfterHeight != boost::none &&
        //    pindexLast->nHeight >= params.nPowAllowMinDifficultyBlocksAfterHeight.get())
        //{
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 6 * 2.5 minutes
            // then allow mining of a min-difficulty block.
        //    if (pblock && pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 6)
        //        return nProofOfWorkLimit;
        //}
    //}

    // Find the first block in the averaging interval
    const CBlockIndex* pindexFirst = pindexLast;
    arith_uint256 ct[64],ctinv[64],bnTmp,bnPrev,bnTarget,bnTarget2,bnTarget3,bnTarget6,bnTarget12,bnTot {0};
    uint32_t nbits,blocktime,ts[sizeof(ct)/sizeof(*ct)]; int32_t zflags[sizeof(ct)/sizeof(*ct)],i,diff,height=0,mult = 0,tipdiff = 0;
    memset(ts,0,sizeof(ts));
    memset(ct,0,sizeof(ct));
    memset(ctinv,0,sizeof(ctinv));
    memset(zflags,0,sizeof(zflags));
    if ( pindexLast != 0 )
        height = (int32_t)pindexLast->nHeight + 1;
    if ( ASSETCHAINS_ADAPTIVEPOW > 0 && pindexFirst != 0 && pblock != 0 && height >= (int32_t)(sizeof(ct)/sizeof(*ct)) )
    {
        tipdiff = (pblock->nTime - pindexFirst->nTime);
        mult = tipdiff - 7 * ASSETCHAINS_BLOCKTIME;
        bnPrev.SetCompact(pindexFirst->nBits);
        for (i=0; pindexFirst != 0 && i<(int32_t)(sizeof(ct)/sizeof(*ct)); i++)
        {
            zflags[i] = (pindexFirst->nBits & 3);
            ct[i].SetCompact(pindexFirst->nBits);
            ts[i] = pindexFirst->nTime;
            pindexFirst = pindexFirst->pprev;
        }
        for (i=0; pindexFirst != 0 && i<(int32_t)(sizeof(ct)/sizeof(*ct))-1; i++)
        {
            if ( zflags[i] == 1 || zflags[i] == 2 ) // I, O and if TSA made it harder
                ct[i] = zawy_ctB(ct[i],ts[i] - ts[i+1]);
        }
        if ( ASSETCHAINS_ADAPTIVEPOW == 2 ) // TSA
        {
            bnTarget = zawy_TSA_EMA(height,tipdiff,ct[0],ts[0] - ts[1]);
            nbits = bnTarget.GetCompact();
            nbits = (nbits & 0xfffffffc) | 0;
            return(nbits);
        }
    }
    pindexFirst = pindexLast;
    for (i = 0; pindexFirst && i < params.nPowAveragingWindow; i++)
    {
        bnTmp.SetCompact(pindexFirst->nBits);
        if ( ASSETCHAINS_ADAPTIVEPOW > 0 && pblock != 0 )
        {
            blocktime = pindexFirst->nTime;
            diff = (pblock->nTime - blocktime);
            //LogPrintf("%d ",diff);
            if ( i < 6 )
            {
                diff -= (8+i)*ASSETCHAINS_BLOCKTIME;
                if ( diff > mult )
                {
                    //LogPrintf("i.%d diff.%d (%u - %u - %dx)\n",i,(int32_t)diff,pblock->nTime,pindexFirst->nTime,(8+i));
                    mult = diff;
                }
            }
            if ( zflags[i] != 0 && zflags[0] == 0 ) // an RST block, but the most recent has no RST
                bnTmp = (bnTmp / arith_uint256(8)) * arith_uint256(7);
        }
        bnTot += bnTmp;
        pindexFirst = pindexFirst->pprev;
    }
    //LogPrintf("diffs %d\n",height);
    // Check we have enough blocks
    if (pindexFirst == NULL)
        return nProofOfWorkLimit;

    bool fNegative,fOverflow; int32_t zawyflag = 0; arith_uint256 easy,origtarget,bnAvg {bnTot / params.nPowAveragingWindow};
    nbits = CalculateNextWorkRequired(bnAvg, pindexLast->GetMedianTimePast(), pindexFirst->GetMedianTimePast(), params);
    if ( ASSETCHAINS_ADAPTIVEPOW > 0 )
    {
        bnTarget = arith_uint256().SetCompact(nbits);
        if ( height > (int32_t)(sizeof(ct)/sizeof(*ct)) && pblock != 0 && tipdiff > 0 )
        {
            easy.SetCompact(KOMODO_MINDIFF_NBITS & (~3),&fNegative,&fOverflow);
            if ( pblock != 0 )
            {
                origtarget = bnTarget;
                if ( zflags[0] == 0 || zflags[0] == 3 )
                {
                    // 15 51 102 162 230 303 380 460 543 627 714 803 892 983 1075 These are the 0.5% per blk numerator constants for W=2 to 16 if denominator is 100. - zawy
                    if ( ASSETCHAINS_BLOCKTIME >= 60 && ASSETCHAINS_BLOCKTIME < 100 )
                        bnTarget = RT_CST_RST_outer(height,pblock->nTime,bnTarget,ts,ct,1,60,1,10);
                    else if ( ASSETCHAINS_BLOCKTIME >= 100 )
                        bnTarget = RT_CST_RST_outer(height,pblock->nTime,bnTarget,ts,ct,1,100,1,10);
                    if ( bnTarget < origtarget )
                        zawyflag = 2;
                    else
                    {
                        bnTarget = RT_CST_RST_outer(height,pblock->nTime,origtarget,ts,ct,15,100,2,20);
                        if ( bnTarget < origtarget )
                            zawyflag = 2;
                        else
                        {
                            bnTarget = RT_CST_RST_outer(height,pblock->nTime,origtarget,ts,ct,1,2,3,30);
                            if ( bnTarget < origtarget )
                                zawyflag = 2;
                            else
                            {
                                bnTarget = RT_CST_RST_outer(height,pblock->nTime,origtarget,ts,ct,7,3,6,40);
                                if ( bnTarget < origtarget )
                                    zawyflag = 2;
                                else
                                {
                                    bnTarget = RT_CST_RST_outer(height,pblock->nTime,origtarget,ts,ct,12,7,12,50);
                                    if ( bnTarget < origtarget )
                                        zawyflag = 2;
                                }
                            }
                        }
                    }
                }
                else
                {
                    for (i=0; i<50; i++)
                        if ( zflags[i] == 2 )
                            break;
                    if ( i < 10 )
                    {
                        bnTarget = RT_CST_RST_inner(height,pblock->nTime,bnTarget,ts,ct,1,i);
                        if ( bnTarget > origtarget )
                            bnTarget = origtarget;
                    }
                    if ( i < 20 )
                    {
                        bnTarget2 = RT_CST_RST_inner(height,pblock->nTime,bnTarget,ts,ct,2,i);
                        if ( bnTarget2 < bnTarget )
                            bnTarget = bnTarget2;
                    }
                    if ( i < 30 )
                    {
                        bnTarget3 = RT_CST_RST_inner(height,pblock->nTime,bnTarget,ts,ct,3,i);
                        if ( bnTarget3 < bnTarget )
                            bnTarget = bnTarget3;
                    }
                    if ( i < 40 )
                    {
                        bnTarget6 = RT_CST_RST_inner(height,pblock->nTime,bnTarget,ts,ct,6,i);
                        if ( bnTarget6 < bnTarget )
                            bnTarget = bnTarget6;
                    }
                    if ( i < 50 )
                    {
                        bnTarget12 = RT_CST_RST_inner(height,pblock->nTime,bnTarget,ts,ct,12,i);
                        if ( bnTarget12 < bnTarget)
                            bnTarget = bnTarget12;
                    }
                    if ( bnTarget != origtarget )
                        zawyflag = 1;
                }
            }
            if ( mult > 1 ) // e^mult case, jl777:  test of mult > 1 failed when it was int64_t???
            {
                origtarget = bnTarget;
                bnTarget = zawy_exponential(bnTarget,mult);
                if ( bnTarget < origtarget || bnTarget > easy )
                {
                    bnTarget = easy;
                    LogPrint("pow", "cmp.%d mult.%d ht.%d -> easy target\n",mult>1,(int32_t)mult,height);
                    return(KOMODO_MINDIFF_NBITS & (~3));
                }
                {
                    int32_t z;
                    for (z=31; z>=0; z--)
                        LogPrint("pow", "%02x",((uint8_t *)&bnTarget)[z]);
                }
                LogPrint("pow", " exp() to the rescue cmp.%d mult.%d for ht.%d\n",mult>1,(int32_t)mult,height);
            }
            if ( 0 && zflags[0] == 0 && zawyflag == 0 && mult <= 1 )
            {
                bnTarget = zawy_TSA_EMA(height,tipdiff,(bnTarget+ct[0]+ct[1])/arith_uint256(3),ts[0] - ts[1]);
                if ( bnTarget < origtarget )
                    zawyflag = 3;
            }
        }
        nbits = bnTarget.GetCompact();
        nbits = (nbits & 0xfffffffc) | zawyflag;
    }
    return(nbits);
}

unsigned int CalculateNextWorkRequired(arith_uint256 bnAvg,
                                       int64_t nLastBlockTime, int64_t nFirstBlockTime,
                                       const Consensus::Params& params)
{
    // Limit adjustment step
    // Use medians to prevent time-warp attacks
    int64_t nActualTimespan = nLastBlockTime - nFirstBlockTime;
    LogPrint("pow", "  nActualTimespan = %d  before dampening\n", nActualTimespan);
    nActualTimespan = params.AveragingWindowTimespan() + (nActualTimespan - params.AveragingWindowTimespan())/4;
    LogPrint("pow", "  nActualTimespan = %d  before bounds\n", nActualTimespan);

    if ( ASSETCHAINS_ADAPTIVEPOW <= 0 )
    {
        if (nActualTimespan < params.MinActualTimespan())
            nActualTimespan = params.MinActualTimespan();
        if (nActualTimespan > params.MaxActualTimespan())
            nActualTimespan = params.MaxActualTimespan();
    }
    // Retarget
    arith_uint256 bnLimit;
    if (ASSETCHAINS_ALGO == ASSETCHAINS_EQUIHASH)
        bnLimit = UintToArith256(params.powLimit);
    else
        bnLimit = UintToArith256(params.powAlternate);

    const arith_uint256 bnPowLimit = bnLimit; //UintToArith256(params.powLimit);
    arith_uint256 bnNew {bnAvg};
    bnNew /= params.AveragingWindowTimespan();
    bnNew *= nActualTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    /// debug print
    LogPrint("pow", "GetNextWorkRequired RETARGET\n");
    LogPrint("pow", "params.AveragingWindowTimespan() = %d    nActualTimespan = %d\n", params.AveragingWindowTimespan(), nActualTimespan);
    LogPrint("pow", "Current average: %08x  %s\n", bnAvg.GetCompact(), bnAvg.ToString());
    LogPrint("pow", "After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}

bool CheckEquihashSolution(const CBlockHeader *pblock, const CChainParams& params)
{
    if (ASSETCHAINS_ALGO != ASSETCHAINS_EQUIHASH)
        return true;
    
    if ( ASSETCHAINS_NK[0] != 0 && ASSETCHAINS_NK[1] != 0 && pblock->GetHash().ToString() == "027e3758c3a65b12aa1046462b486d0a63bfa1beae327897f56c5cfb7daaae71" )
        return true;

    unsigned int n = params.EquihashN();
    unsigned int k = params.EquihashK();

    if ( Params().NetworkIDString() == "regtest" )
        return(true);
    // Hash state
    crypto_generichash_blake2b_state state;
    EhInitialiseState(n, k, state);

    // I = the block header minus nonce and solution.
    CEquihashInput I{*pblock};
    // I||V
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << I;
    ss << pblock->nNonce;

    // H(I||V||...
    crypto_generichash_blake2b_update(&state, (unsigned char*)&ss[0], ss.size());

    bool isValid;
    EhIsValidSolution(n, k, state, pblock->nSolution, isValid);

    if (!isValid)
        return error("CheckEquihashSolution(): invalid solution");

    return true;
}

int32_t komodo_chosennotary(int32_t *notaryidp,int32_t height,uint8_t *pubkey33,uint32_t timestamp);
int32_t komodo_is_special(uint8_t pubkeys[66][33],int32_t mids[66],uint32_t blocktimes[66],int32_t height,uint8_t pubkey33[33],uint32_t blocktime);
int32_t komodo_currentheight();
void komodo_index2pubkey33(uint8_t *pubkey33,CBlockIndex *pindex,int32_t height);
bool komodo_checkopret(CBlock *pblock, CScript &merkleroot);
CScript komodo_makeopret(CBlock *pblock, bool fNew);
extern char ASSETCHAINS_SYMBOL[KOMODO_ASSETCHAIN_MAXLEN];
#define KOMODO_ELECTION_GAP 2000

int32_t komodo_eligiblenotary(uint8_t pubkeys[66][33],int32_t *mids,uint32_t blocktimes[66],int32_t *nonzpkeysp,int32_t height);
int32_t KOMODO_LOADINGBLOCKS = 1;

extern std::string NOTARY_PUBKEY;

bool isSecondBlockAllowed(int32_t notaryid, uint32_t blocktime, uint32_t threshold, uint32_t delta, const std::vector<int32_t> &vPriorityList)
{

    if (blocktime >= threshold && delta > 0 &&
        vPriorityList.size() == 64 && notaryid >= 0)
    {
        size_t nPos = ((blocktime - threshold) / delta);

        if (nPos < vPriorityList.size())
        {
            // if nodeid found in current range of priority -> allow it
            if (std::count(vPriorityList.begin(), vPriorityList.begin() + nPos + 1, notaryid) > 0)
                return true;
        }
        else
            return true; // if time is bigger than the biggest range -> all nodes allowed
    }
    return false;
}

bool CheckProofOfWork(const CBlockHeader &blkHeader, uint8_t *pubkey33, int32_t height, const Consensus::Params& params)
{
    extern int32_t KOMODO_REWIND;
    uint256 hash;
    bool fNegative,fOverflow; uint8_t origpubkey33[33]; int32_t i,nonzpkeys=0,nonz=0,special=0,special2=0,notaryid=-1,flag = 0, mids[66]; uint32_t tiptime,blocktimes[66];
    arith_uint256 bnTarget; uint8_t pubkeys[66][33];
    //for (i=31; i>=0; i--)
    //    LogPrintf("%02x",((uint8_t *)&hash)[i]);
    //LogPrintf(" checkpow\n");
    memcpy(origpubkey33,pubkey33,33);
    memset(blocktimes,0,sizeof(blocktimes));
    tiptime = komodo_chainactive_timestamp();
    bnTarget.SetCompact(blkHeader.nBits, &fNegative, &fOverflow);
    if ( height == 0 )
    {
        height = komodo_currentheight() + 1;
        //LogPrintf("set height to %d\n",height);
    }
    if ( height > 34000 && ASSETCHAINS_SYMBOL[0] == 0 ) // 0 -> non-special notary
    {
        special = komodo_chosennotary(&notaryid,height,pubkey33,tiptime);
        for (i=0; i<33; i++)
        {
            if ( pubkey33[i] != 0 )
                nonz++;
        }
        if ( nonz == 0 )
        {
            //LogPrintf("ht.%d null pubkey checkproof return\n",height);
            return(true); // will come back via different path with pubkey set
        }
        flag = komodo_eligiblenotary(pubkeys,mids,blocktimes,&nonzpkeys,height);
        special2 = komodo_is_special(pubkeys,mids,blocktimes,height,pubkey33,blkHeader.nTime);
        if ( notaryid >= 0 )
        {
            if ( height > 10000 && height < 80000 && (special != 0 || special2 > 0) )
                flag = 1;
            else if ( height >= 80000 && height < 108000 && special2 > 0 )
                flag = 1;
            else if ( height >= 108000 && special2 > 0 )
                flag = (height > 1000000 || (height % KOMODO_ELECTION_GAP) > 64 || (height % KOMODO_ELECTION_GAP) == 0);
            else if ( height == 790833 )
                flag = 1;
            else if ( special2 < 0 )
            {
                if ( height > 792000 )
                    flag = 0;
                else LogPrintf("ht.%d notaryid.%d special.%d flag.%d special2.%d\n",height,notaryid,special,flag,special2);
            }

            /* hf22 rule applied to stale blocks */
            if (params.nHF22Height != boost::none) {

                const uint32_t nHeightAfterGAPSecondBlockAllowed = params.nHF22Height.get();
                const uint32_t nMaxGAPAllowed = params.nMaxFutureBlockTime + 1;

                /*  in KMD chain after nHeightAfterGAPSecondBlockAllowed height we should allow
                    notary nodes to mine a second block if nMaxGAPAllowed since last block passed
                */
                if (ASSETCHAINS_SYMBOL[0] == 0 && height > nHeightAfterGAPSecondBlockAllowed)
                {
                    const uint32_t &blocktime = blkHeader.nTime;
                    if (blocktime /* upcoming block time */ >= tiptime /* last block in chain time */ + nMaxGAPAllowed &&
                        tiptime == blocktimes[1] /* just for ensure that all is correct */ )
                    {
                        assert(notaryid >= 0); /* assume to be notary */

                        /* here special2 is:
                                -2 - if it's notary but 57 seconds from last block is not passed
                                -1 - notary repeat in mids[1..65], i.e. in last 65 blocks
                                 0 - non-notary
                                 1 - notary
                                 2 - notary with easy diff allowed, bcz of "after gap"
                        */

                        LogPrint("hfnet","%s[%d]: time.(%lu >= %lu), special2.%ld, notaryid.%ld\n", __func__, __LINE__, blocktime, tiptime, special2, notaryid);

                        /* build priority list */
                        std::vector<int32_t> vPriorityList(64);
                        // fill the priority list by notaries numbers, 0..63
                        int id = 0;
                        std::generate(vPriorityList.begin(), vPriorityList.end(), [&id] { return id++; }); // std::iota
                        // move the notaries participated in last 65 to the end of priority list
                        std::vector<int32_t>::iterator it;
                        for (size_t i = sizeof(mids)/sizeof(mids[0]) - 1; i > 0; --i) { // ! mids[0] is not included
                            if (mids[i] != -1 ) {
                                it = std::find(vPriorityList.begin(), vPriorityList.end(), mids[i]);
                                if (it != vPriorityList.end() && std::next(it) != vPriorityList.end()) {
                                    std::rotate(it, std::next(it), vPriorityList.end());
                                }
                            }
                        }

                        LogPrint("hfnet","%s[%d]: vPriorityList = \n    [ ", __func__, __LINE__);
                        for (const auto &id : vPriorityList) {
                            LogPrint("hfnet", "%ld ", id);
                        }
                        LogPrint("hfnet", "]\n");

                        /* reconsider special2 if allowed */
                        if ((-1 == special2 || 1 == special2) &&
                            isSecondBlockAllowed(notaryid, blocktime, tiptime + nMaxGAPAllowed, params.nHF22NotariesPriorityRotateDelta, vPriorityList))
                        {
                            special2 = 2;
                            LogPrint("hfnet", "%s[%d]: special2.%ld, notaryid.%ld, ht.%ld, hash.%s\n", __func__, __LINE__, special2, notaryid, height, blkHeader.GetHash().ToString());
                        }

                        LogPrint("hfnet","%s[%d]: Allowed to mine: \n    [ ", __func__, __LINE__);
                        for (int i = 0; i < 64; i++) {
                            if (isSecondBlockAllowed(i, blocktime, tiptime + nMaxGAPAllowed, params.nHF22NotariesPriorityRotateDelta, vPriorityList))
                                LogPrint("hfnet", "%d ", i);
                        }
                        LogPrint("hfnet", "]\n");

                    }
                }
            }

            if ( (flag != 0 || special2 > 0) && special2 != -2 )
            {
                //LogPrintf("EASY MINING ht.%d\n",height);
                bnTarget.SetCompact(KOMODO_MINDIFF_NBITS,&fNegative,&fOverflow);
                /*
                const void* pblock = &blkHeader;
                CScript merkleroot = CScript();
                if ( height > nDecemberHardforkHeight && !komodo_checkopret((CBlock*)pblock, merkleroot) ) // December 2019 hardfork
                {
                    LogPrintf("failed or missing merkleroot expected.%s != merkleroot.%s\n", komodo_makeopret((CBlock*)pblock, false).ToString().c_str(), merkleroot.ToString().c_str());
                    return false;
                }
                */
            }
        }
    }
    arith_uint256 bnLimit = (height <= 1 || ASSETCHAINS_ALGO == ASSETCHAINS_EQUIHASH) ? UintToArith256(params.powLimit) : UintToArith256(params.powAlternate);
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > bnLimit)
        return error("CheckProofOfWork(): nBits below minimum work");
    if ( ASSETCHAINS_STAKED != 0 )
    {
        arith_uint256 bnMaxPoSdiff;
        bnTarget.SetCompact(KOMODO_MINDIFF_NBITS,&fNegative,&fOverflow);
    }
    //else if ( ASSETCHAINS_ADAPTIVEPOW > 0 && ASSETCHAINS_STAKED == 0 )
    //    bnTarget = komodo_adaptivepow_target(height,bnTarget,blkHeader.nTime);
    // Check proof of work matches claimed amount
    if ( UintToArith256(hash = blkHeader.GetHash()) > bnTarget )
    {
        if ( KOMODO_LOADINGBLOCKS != 0 )
            return true;

        if ( ASSETCHAINS_SYMBOL[0] != 0 || height > 792000 )
        {
            //if ( 0 && height > 792000 )
            if ( Params().NetworkIDString() != "regtest" )
            {
                for (i=31; i>=0; i--)
                    LogPrintf("%02x",((uint8_t *)&hash)[i]);
                LogPrintf(" hash vs ");
                for (i=31; i>=0; i--)
                    LogPrintf("%02x",((uint8_t *)&bnTarget)[i]);
                LogPrintf(" ht.%d special.%d special2.%d flag.%d notaryid.%d mod.%d error\n",height,special,special2,flag,notaryid,(height % 35));
                for (i=0; i<33; i++)
                    LogPrintf("%02x",pubkey33[i]);
                LogPrintf(" <- pubkey\n");
                for (i=0; i<33; i++)
                    LogPrintf("%02x",origpubkey33[i]);
                LogPrintf(" <- origpubkey\n");
            }
            return false;
        }
    }
    /*for (i=31; i>=0; i--)
     LogPrintf("%02x",((uint8_t *)&hash)[i]);
     LogPrintf(" hash vs ");
     for (i=31; i>=0; i--)
     LogPrintf("%02x",((uint8_t *)&bnTarget)[i]);
     LogPrintf(" height.%d notaryid.%d PoW valid\n",height,notaryid);*/
    return true;
}

arith_uint256 GetBlockProof(const CBlockIndex& block)
{
    arith_uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for an arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (bnTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}

int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params& params)
{
    arith_uint256 r;
    int sign = 1;
    if (to.nChainWork > from.nChainWork) {
        r = to.nChainWork - from.nChainWork;
    } else {
        r = from.nChainWork - to.nChainWork;
        sign = -1;
    }
    r = r * arith_uint256(params.nPowTargetSpacing) / GetBlockProof(tip);
    if (r.bits() > 63) {
        return sign * std::numeric_limits<int64_t>::max();
    }
    return sign * r.GetLow64();
}
