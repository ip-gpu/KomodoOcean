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
#pragma once

#include "komodo_defs.h"
#include "komodo_cJSON.h"

#include "notaries_staked.h"
#include "komodo_hardfork.h"

#define KOMODO_MAINNET_START 178999
#define KOMODO_NOTARIES_HEIGHT1 814000
#define KOMODO_NOTARIES_HEIGHT2 2588672

#define CRYPTO777_PUBSECPSTR "020e46e79a2a8d12b9b5d12c7a91adb4e454edfae43c0a0cb805427d2ac7613fd9"

int32_t getkmdseason(int32_t height);

int32_t getacseason(uint32_t timestamp);

int32_t komodo_notaries(uint8_t pubkeys[64][33],int32_t height,uint32_t timestamp);

int32_t komodo_electednotary(int32_t *numnotariesp,uint8_t *pubkey33,int32_t height,uint32_t timestamp);

int32_t komodo_ratify_threshold(int32_t height,uint64_t signedmask);

void komodo_notarysinit(int32_t origheight,uint8_t pubkeys[64][33],int32_t num);

int32_t komodo_chosennotary(int32_t *notaryidp,int32_t height,uint8_t *pubkey33,uint32_t timestamp);

struct notarized_checkpoint *komodo_npptr_for_height(int32_t height, int *idx);

struct notarized_checkpoint *komodo_npptr(int32_t height);

struct notarized_checkpoint *komodo_npptr_at(int idx);

int32_t komodo_prevMoMheight();

int32_t komodo_notarized_height(int32_t *prevMoMheightp,uint256 *hashp,uint256 *txidp);

int32_t komodo_dpowconfs(int32_t txheight,int32_t numconfs);

int32_t komodo_MoMdata(int32_t *notarized_htp,uint256 *MoMp,uint256 *kmdtxidp,int32_t height,uint256 *MoMoMp,int32_t *MoMoMoffsetp,int32_t *MoMoMdepthp,int32_t *kmdstartip,int32_t *kmdendip);

int32_t komodo_notarizeddata(int32_t nHeight,uint256 *notarized_hashp,uint256 *notarized_desttxidp);

void komodo_notarized_update(struct komodo_state *sp,int32_t nHeight,int32_t notarized_height,uint256 notarized_hash,uint256 notarized_desttxid,uint256 MoM,int32_t MoMdepth);

void komodo_init(int32_t height);
