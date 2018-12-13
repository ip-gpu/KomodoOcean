#include "asn/Condition.h"
#include "asn/Fulfillment.h"
#include "asn/EvalFulfillment.h"
#include "asn/OCTET_STRING.h"
#include "cryptoconditions.h"
#include "internal.h"
#include "cJSON.h"


//struct CCType CC_EvalType;


static unsigned char *evalFingerprint(const CC *cond) {
    unsigned char *hash = (unsigned char *)calloc(1, 32);
    sha256(cond->code, cond->codeLength, hash);
    return hash;
}


static unsigned long evalCost(const CC *cond) {
    return 1048576;  // Pretty high
}


static CC *evalFromJSON(const cJSON *params, char *err) {
    size_t codeLength;
    unsigned char *code = 0;

    if (!jsonGetBase64(params, (char*)"code", err, &code, &codeLength)) {
        return NULL;
    }

    CC *cond = cc_new(CC_Eval);
    cond->code = code;
    cond->codeLength = codeLength;
    return cond;
}


static void evalToJSON(const CC *cond, cJSON *code) {

    // add code
    unsigned char *b64 = base64_encode(cond->code, cond->codeLength);
    cJSON_AddItemToObject(code, "code", cJSON_CreateString((const char*)b64));
    free(b64);
}


static CC *evalFromFulfillment(const Fulfillment_t *ffill) {
    CC *cond = cc_new(CC_Eval);

    EvalFulfillment_t *eval = (EvalFulfillment_t *)(&ffill->choice.evalSha256);

    OCTET_STRING_t octets = eval->code;
    cond->codeLength = octets.size;
    cond->code = (uint8_t*)calloc(1,octets.size);
    memcpy(cond->code, octets.buf, octets.size);

    return cond;
}


static Fulfillment_t *evalToFulfillment(const CC *cond) {
    Fulfillment_t *ffill = (Fulfillment_t *)calloc(1, sizeof(Fulfillment_t));
    ffill->present = Fulfillment_PR_evalSha256;
    EvalFulfillment_t *eval = &ffill->choice.evalSha256;
    OCTET_STRING_fromBuf(&eval->code, (const char*)(cond->code), cond->codeLength);
    return ffill;
}


int evalIsFulfilled(const CC *cond) {
    return 1;
}


static void evalFree(CC *cond) {
    free(cond->code);
}


static uint32_t evalSubtypes(const CC *cond) {
    return 0;
}


/*
 * The JSON api doesn't contain custom verifiers, so a stub method is provided suitable for testing
 */
int jsonVerifyEval(CC *cond, void *context) {
    if (cond->codeLength == 5 && 0 == memcmp(cond->code, "TEST", 4)) {
        return cond->code[5];
    }
    fprintf(stderr,"Cannot verify eval; user function unknown\n");
    return 0;
}


typedef struct CCEvalVerifyData {
    VerifyEval verify;
    void *context;
} CCEvalVerifyData;


int evalVisit(CC *cond, CCVisitor visitor) {
    if (cond->type->typeId != CC_Eval) return 1;
    CCEvalVerifyData *evalData = (CCEvalVerifyData *)(visitor.context);
    return evalData->verify(cond, evalData->context);
}


int cc_verifyEval(const CC *cond, VerifyEval verify, void *context) {
    CCEvalVerifyData evalData = {verify, context};
    CCVisitor visitor = {&evalVisit, (uint8_t*)"", 0, &evalData};
    return cc_visit((CC*)cond, visitor);
}


struct CCType CC_EvalType = { 15, "eval-sha-256", Condition_PR_evalSha256, 0, &evalFingerprint, &evalCost, &evalSubtypes, &evalFromJSON, &evalToJSON, &evalFromFulfillment, &evalToFulfillment, &evalIsFulfilled, &evalFree };
