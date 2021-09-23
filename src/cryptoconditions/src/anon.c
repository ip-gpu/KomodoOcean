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

#include "asn/Condition.h"
#include "asn/Fulfillment.h"
#include "asn/PrefixFingerprintContents.h"
#include "asn/OCTET_STRING.h"
#include "include/cJSON.h"
#include "cryptoconditions.h"


struct CCType CC_AnonType;


CC *mkAnon(const Condition_t *asnCond) {

    CCType *realType = getTypeByAsnEnum(asnCond->present);
    if (!realType) {
        fprintf(stderr, "Unknown ASN type: %i", asnCond->present);
        return 0;
    }
    CC *cond = cc_new(CC_Anon);
    cond->conditionType = realType;
    const CompoundSha256Condition_t *deets = &asnCond->choice.thresholdSha256;
    memcpy(cond->fingerprint, deets->fingerprint.buf, 32);
    cond->cost = deets->cost;
    if (realType->getSubtypes) {
        cond->subtypes = fromAsnSubtypes(deets->subtypes);
    }
    return cond;
}



static void anonToJSON(const CC *cond, cJSON *params) {
    unsigned char *b64 = base64_encode(cond->fingerprint, 32);
    cJSON_AddItemToObject(params, "fingerprint", cJSON_CreateString(b64));
    free(b64);
    cJSON_AddItemToObject(params, "cost", cJSON_CreateNumber(cond->cost));
    cJSON_AddItemToObject(params, "subtypes", cJSON_CreateNumber(cond->subtypes));
}


static void anonFingerprint(const CC *cond, uint8_t *out) {
    memcpy(out, cond->fingerprint, 32);
}


static unsigned long anonCost(const CC *cond) {
    return cond->cost;
}


static uint32_t anonSubtypes(const CC *cond) {
    return cond->subtypes;
}


static Fulfillment_t *anonFulfillment(const CC *cond) {
    return NULL;
}


static void anonFree(CC *cond) {
}


static int anonIsFulfilled(const CC *cond) {
    return 0;
}


struct CCType CC_AnonType = { -1, "(anon)", Condition_PR_NOTHING, NULL, &anonFingerprint, &anonCost, &anonSubtypes, NULL, &anonToJSON, NULL, &anonFulfillment, &anonIsFulfilled, &anonFree };
