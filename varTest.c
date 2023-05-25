#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sbits.h"

#define NUM_STEPS 10
#define NUM_RUNS 1
#define IMAGE_TEST 0
#define VALIDATE_VAR_DATA 1

// Cursed linkedList for tracking data
typedef struct Node {
    int32_t key;
    void *data;
    uint32_t length;
    struct Node *next;
} Node;

void updateBitmapInt8Bucket(void *data, void *bm);
void buildBitmapInt8BucketWithRange(void *min, void *max, void *bm);
int8_t inBitmapInt8Bucket(void *data, void *bm);
void updateBitmapInt16(void *data, void *bm);
int8_t inBitmapInt16(void *data, void *bm);
void updateBitmapInt64(void *data, void *bm);
int8_t inBitmapInt64(void *data, void *bm);
int8_t int32Comparator(void *a, void *b);
uint32_t keyModifier(uint32_t inputKey);
uint32_t readImageFromFile(void **data, char *filename);
void writeDataToFile(void *data, char *filename, uint32_t length);
void imageVarData(float chance, char *filename, uint8_t *usingVarData, uint32_t *length, void **varData);
void retrieveImageData(void **varData, uint32_t length, int32_t key, char *filename, char *filetype);
uint8_t dataEquals(void *varData, uint32_t length, Node *node);
void randomVarData(float chance, uint32_t sizeLowerBound, uint32_t sizeUpperBound, uint8_t *usingVarData, uint32_t *length, void **varData);
int retrieveData(sbitsState *state, int32_t key, int8_t *recordBuffer);

void main() {
    printf("\nSTARTING SBITS VARIABLE DATA TESTS.\n");

    // Two extra bufferes required for variable data
    int8_t M = 6;

    // Initialize to default values
    int32_t numRecords = 600;  // default values
    int32_t testRecords = 600; // default values
    uint8_t useRandom = 0;     // default values
    size_t splineMaxError = 0; // default values
    uint32_t stepSize = numRecords / NUM_STEPS;
    count_t r, l;
    uint32_t times[NUM_STEPS][NUM_RUNS];
    uint32_t reads[NUM_STEPS][NUM_RUNS];
    uint32_t writes[NUM_STEPS][NUM_RUNS];
    uint32_t overwrites[NUM_STEPS][NUM_RUNS];
    uint32_t hits[NUM_STEPS][NUM_RUNS];
    uint32_t rtimes[NUM_STEPS][NUM_RUNS];
    uint32_t rreads[NUM_STEPS][NUM_RUNS];
    uint32_t rhits[NUM_STEPS][NUM_RUNS];

    /* Determines if generated, sequential data is used, or data from an input file*/
    int8_t seqdata = 1;

    // Files for non-sequentioal data
    FILE *infile = NULL, *infileRandom = NULL;
    uint32_t minRange, maxRange;

    if (seqdata != 1) {
        /* Open file to read input records */

        // measure1_smartphone_sens.bin
        // infile = fopen("data/measure1_smartphone_sens.bin", "r+b");
        // infileRandom = fopen("data/measure1_smartphone_sens_randomized.bin",
        // "r+b"); minRange = 0; maxRange = INT32_MAX; numRecords = 18354;
        // testRecords = 18354;

        // position.bin
        // infile = fopen("data/position.bin", "r+b");
        // infileRandom = fopen("data/position_randomized.bin", "r+b");
        // minRange = 0;
        // maxRange = INT32_MAX;
        // numRecords = 1518;
        // testRecords = 1518;

        // ethylene_CO.bin
        // infile = fopen("data/ethylene_CO.bin", "r+b");
        // infileRandom = fopen("data/ethylene_CO_randomized.bin", "r+b");
        // minRange = 0;
        // maxRange = INT32_MAX;
        // numRecords = 4085589;
        // testRecords = 4085589;

        // Watch_gyroscope.bin
        // infile = fopen("data/Watch_gyroscope.bin", "r+b");
        // infileRandom = fopen("data/Watch_gyroscope_randomized.bin", "r+b");
        // minRange = 0;
        // maxRange = INT32_MAX;
        // numRecords = 2865713;
        // testRecords = 2865713;

        // PRSA_Data_Hongxin.bin
        // infile = fopen("data/PRSA_Data_Hongxin.bin", "r+b");
        // infileRandom = fopen("data/PRSA_Data_Hongxin_randomized.bin", "r+b");
        // minRange = 0;
        // maxRange = INT32_MAX;
        // numRecords = 35064;
        // testRecords = 35064;

        // S7hl500K.bin
        // infile = fopen("data/S7hl500K.bin", "r+b");
        // minRange = 0;
        // maxRange = INT32_MAX;
        // numRecords = 500000;

        // infile = fopen("data/sea100K.bin", "r+b");
        // minRange = 1314604380;
        // maxRange = 1609487580;
        // numRecords = 100001;
        // testRecords = 100001;

        infile = fopen("data/uwa500K.bin", "r+b");
        // infileRandom =
        // fopen("data/uwa_data_only_2000_500KSorted_randomized.bin", "r+b");
        minRange = 946713600;
        maxRange = 977144040;
        numRecords = 500000;
        testRecords = 500000;

        splineMaxError = 1;
        useRandom = 0;

        stepSize = numRecords / NUM_STEPS;
    }

    for (r = 0; r < NUM_RUNS; r++) {
        sbitsState *state = (sbitsState *)malloc(sizeof(sbitsState));

        state->keySize = 4;
        state->dataSize = 4;
        state->pageSize = 512;
        state->bitmapSize = 0;
        state->bufferSizeInBlocks = M;
        state->buffer = malloc((size_t)state->bufferSizeInBlocks * state->pageSize);

        /* Address level parameters */
        state->startAddress = 0;
        state->endAddress = state->pageSize * numRecords / 10;
        state->varAddressStart = 0;
        state->varAddressEnd = 10000;
        state->eraseSizeInPages = 4;

        state->parameters = SBITS_USE_BMAP | SBITS_USE_INDEX | SBITS_USE_VDATA;

        if (SBITS_USING_INDEX(state->parameters) == 1)
            state->endAddress += state->pageSize * (state->eraseSizeInPages * 2);
        if (SBITS_USING_BMAP(state->parameters))
            state->bitmapSize = 8;

        /* Setup for data and bitmap comparison functions */
        state->inBitmap = inBitmapInt16;
        state->updateBitmap = updateBitmapInt16;
        state->inBitmap = inBitmapInt64;
        state->updateBitmap = updateBitmapInt64;
        state->compareKey = int32Comparator;
        state->compareData = int32Comparator;

        if (sbitsInit(state, splineMaxError) != 0) {
            printf("Initialization error.\n");
            return;
        } else {
            printf("Initialization success.\n");
        }

        // Initialize Buffer
        int8_t *recordBuffer = (int8_t *)calloc(1, state->recordSize);

        // Initialize the data validation struct
        Node *validationHead = malloc(sizeof(Node));
        Node *validationTail = validationHead;

        printf("\n\nINSERT TEST:\n");
        /* Insert records into structure */
        uint32_t start = clock();

        int32_t i;
        if (seqdata == 1) {
            char vardata[15] = "Testing 000...";
            for (i = 0; i < numRecords; i++) {
                // Key = i, fixed data = i % 100
                *((int32_t *)recordBuffer) = i;
                *((int32_t *)(recordBuffer + 4)) = (i % 100);

                // Generate variable-length data
                void *variableData = NULL;
                uint8_t hasVarData = 0;
                uint32_t length;
                if (IMAGE_TEST) {
                    imageVarData(0.05, "test.png", &hasVarData, &length, &variableData);
                } else {
                    // randomVarData(0.1, 10, 100, &hasVarData, &length, &variableData);
                    hasVarData = 1;
                    length = 15;
                    vardata[10] = (char)(i % 10) + '0';
                    vardata[9] = (char)((i / 10) % 10) + '0';
                    vardata[8] = (char)((i / 100) % 10) + '0';
                    variableData = malloc(length);
                    memcpy(variableData, vardata, length);
                }

                // Put variable length data
                sbitsPutVar(state, recordBuffer, (void *)(recordBuffer + 4), hasVarData ? variableData : NULL, length);

                if (hasVarData) {
                    if (VALIDATE_VAR_DATA) {
                        validationTail->key = i;
                        validationTail->data = variableData;
                        validationTail->length = length;
                        validationTail->next = malloc(sizeof(Node));
                        validationTail = validationTail->next;
                        validationTail->length = 0;
                    } else {
                        free(variableData);
                        variableData = NULL;
                    }
                    // printf("Using var data: KEY: %d\n", i);
                }

                if (i % stepSize == 0) {
                    // printf("Num: %lu KEY: %lu\n", i, i);
                    l = i / stepSize - 1;
                    if (l < NUM_STEPS && l >= 0) {
                        times[l][r] = ((clock() - start) * 1000) / CLOCKS_PER_SEC;
                        reads[l][r] = state->numReads;
                        writes[l][r] = state->numWrites;
                        overwrites[l][r] = 0;
                        hits[l][r] = state->bufferHits;
                    }
                }
            }
        } else {
            /* Read data from a file */

            char infileBuffer[512];
            int8_t headerSize = 16;
            int32_t i = 0;
            fseek(infile, 0, SEEK_SET);
            // uint32_t readCounter = 0;
            while (1) {
                /* Read page */
                if (0 == fread(infileBuffer, state->pageSize, 1, infile)) {
                    break;
                }

                /* Process all records on page */
                int16_t count = *((int16_t *)(infileBuffer + 4));
                for (int j = 0; j < count; j++) {
                    void *buf = (infileBuffer + headerSize + j * (state->keySize + state->dataSize));

                    // Generate variable-length data
                    void *variableData = NULL;
                    uint8_t hasVarData = 0;
                    uint32_t length = 0;
                    if (IMAGE_TEST) {
                        imageVarData(0.01, "test.png", &hasVarData, &length, &variableData);
                    } else {
                        randomVarData(0.10, 10, 100, &hasVarData, &length, &variableData);
                    }

                    // Put variable length data
                    // printf("Key: %i, Data: %i\n", *(uint32_t*)buf, *(uint32_t*)((int8_t*)buf + 4));
                    if (0 != sbitsPutVar(state, buf, (void *)((int8_t *)buf + 4), hasVarData ? variableData : NULL, length)) {
                        printf("ERROR: Failed to insert record\n");
                    }

                    if (hasVarData) {
                        if (VALIDATE_VAR_DATA) {
                            validationTail->key = *((int32_t *)buf);
                            validationTail->data = variableData;
                            validationTail->length = length;
                            validationTail->next = malloc(sizeof(Node));
                            validationTail = validationTail->next;
                            validationTail->length = 0;
                        } else {
                            free(variableData);
                            variableData = NULL;
                        }
                        // printf("Using var data: KEY: %d\n", i);
                    }

                    if (i % stepSize == 0) {
                        printf("Num: %lu KEY: %lu\n", i, *((int32_t *)buf));
                        l = i / stepSize - 1;
                        if (l < NUM_STEPS && l >= 0) {
                            times[l][r] = ((clock() - start) * 1000) / CLOCKS_PER_SEC;
                            reads[l][r] = state->numReads;
                            writes[l][r] = state->numWrites;
                            overwrites[l][r] = 0;
                            hits[l][r] = state->bufferHits;
                        }
                    }
                    i++;
                    /* Allows stopping at set number of records instead of
                     * reading entire file */
                    if (i == numRecords) {
                        maxRange = *((uint32_t *)buf);
                        printf("Num: %lu KEY: %lu\n", i, *((int32_t *)buf));

                        goto doneread;
                    }
                }
            }
            numRecords = i;
        }

    doneread:
        sbitsFlush(state);
        fflush(state->file);
        fflush(state->varFile);
        uint32_t end = clock();

        l = NUM_STEPS - 1;
        times[l][r] = ((end - start) * 1000) / CLOCKS_PER_SEC;
        reads[l][r] = state->numReads;
        writes[l][r] = state->numWrites;
        overwrites[l][r] = 0;
        hits[l][r] = state->bufferHits;

        printf("Elapsed Time: %lu ms\n", times[l][r]);
        printf("Records inserted: %lu\n", numRecords);

        printStats(state);
        resetStats(state);

        printf("\n\nQUERY TEST:\n");
        /* Verify that all values can be found and test query performance */

        start = clock();

        if (seqdata == 1) {
            for (i = 0; i < numRecords; i++) {
                int32_t key = i;
                void *varData = NULL;
                uint32_t length = 0;
                int8_t result = sbitsGetVar(state, &key, recordBuffer, &varData, &length);

                if (result == -1) {
                    printf("ERROR: Failed to find: %lu\n", key);
                } else if (result == 1) {
                    printf("WARN: Variable data associated with key %lu was deleted\n", key);
                } else if (*((int32_t *)recordBuffer) != key % 100) {
                    printf("ERROR: Wrong data for: %lu\n", key);
                } else if (VALIDATE_VAR_DATA && varData != NULL) {
                    while (validationHead->key != key) {
                        Node *tmp = validationHead;
                        validationHead = validationHead->next;
                        free(tmp->data);
                        free(tmp);
                    }
                    if (validationHead == NULL) {
                        printf("ERROR: No validation data for: %lu\n", key);
                        return;
                    }
                    // Check that the var data is correct
                    if (!dataEquals(varData, length, validationHead)) {
                        printf("ERROR: Wrong var data for: %lu\n", key);
                    }
                }

                // Retrieve image
                if (varData != NULL) {
                    if (IMAGE_TEST) {
                        retrieveImageData(&varData, length, key, "test", ".png");
                    }
                    free(varData);
                    varData = NULL;
                }

                if (i % stepSize == 0) {
                    l = i / stepSize - 1;
                    if (l < NUM_STEPS && l >= 0) {
                        rtimes[l][r] = ((clock() - start) * 1000) / CLOCKS_PER_SEC;
                        rreads[l][r] = state->numReads;
                        rhits[l][r] = state->bufferHits;
                    }
                }
            }
        } else {
            /* Data from file */

            char infileBuffer[512];
            int8_t headerSize = 16;
            i = 0;
            int8_t queryType = 1;

            if (queryType == 1) {
                /* Query each record from original data set. */
                if (useRandom) {
                    fseek(infileRandom, 0, SEEK_SET);
                } else {
                    fseek(infile, 0, SEEK_SET);
                }
                int32_t readCounter = 0;
                while (1) {
                    /* Read page */
                    if (useRandom) {
                        if (0 == fread(infileBuffer, state->pageSize, 1, infileRandom))
                            break;
                    } else {
                        if (0 == fread(infileBuffer, state->pageSize, 1, infile))
                            break;
                    }

                    readCounter++;

                    /* Process all records on page */
                    int16_t count = *((int16_t *)(infileBuffer + 4));
                    for (int j = 0; j < count; j++) {
                        void *buf = (infileBuffer + headerSize + j * (state->keySize + state->dataSize));
                        int32_t *key = (int32_t *)buf;

                        void *varData = NULL;
                        int32_t length = -1;

                        int8_t result = sbitsGetVar(state, key, recordBuffer, &varData, &length);

                        if (result == -1) {
                            printf("ERROR: Failed to find: %lu\n", *key);
                        } else if (result == 1) {
                            printf("WARN: Variable data associated with key %lu was deleted\n", *key);
                        } else if (*((int32_t *)recordBuffer) != *((int32_t *)((int8_t *)buf + 4))) {
                            printf("ERROR: Wrong data for: %lu\n", *key);
                        } else if (VALIDATE_VAR_DATA && length != -1) {
                            while (validationHead->key != *key) {
                                Node *tmp = validationHead;
                                validationHead = validationHead->next;
                                free(tmp->data);
                                free(tmp);
                            }
                            if (validationHead == NULL) {
                                printf("ERROR: No validation data for: %lu\n", *key);
                                return;
                            }
                            // Check that the var data is correct
                            if (!dataEquals(varData, length, validationHead)) {
                                printf("ERROR: Wrong var data for: %lu\n", *key);
                            }
                        }

                        // Retrieve image
                        if (varData != NULL) {
                            if (IMAGE_TEST) {
                                retrieveImageData(&varData, length, *key, "test", ".png");
                            }
                            free(varData);
                            varData = NULL;
                        }

                        if (i % stepSize == 0) {
                            l = i / stepSize - 1;
                            printf("Num: %lu KEY: %lu\n", i, *key);
                            if (l < NUM_STEPS && l >= 0) {
                                rtimes[l][r] =
                                    ((clock() - start) * 1000) / CLOCKS_PER_SEC;
                                rreads[l][r] = state->numReads;
                                rhits[l][r] = state->bufferHits;
                            }
                        }
                        i++;

                        /* Allows ending test after set number of records rather than processing entire file */
                        if (i == numRecords || i == testRecords)
                            goto donetest;
                    }
                }
            donetest:
                numRecords = i;
            } else if (queryType == 2) {
                /* Query random values in range. May not exist in data set. */
                i = 0;
                int32_t num = maxRange - minRange;
                printf("Rge: %d Rand max: %d\n", num, RAND_MAX);
                while (i < numRecords) {
                    double scaled = ((double)rand() * (double)rand()) / RAND_MAX / RAND_MAX;
                    int32_t key = (num + 1) * scaled + minRange;

                    void *varData = NULL;
                    int32_t length = -1;
                    int8_t result = sbitsGetVar(state, &key, recordBuffer, &varData, &length);

                    if (result == -1) {
                        printf("ERROR: Failed to find: %lu\n", key);
                    } else if (result == 1) {
                        printf("WARN: Variable data associated with key %lu was deleted\n", key);
                    } else if (*((int32_t *)recordBuffer) != key % 100) {
                        printf("ERROR: Wrong data for: %lu\n", key);
                        // printf("Key: %lu Data: %lu Var length: %d\n", key, *((int32_t*)recordBuffer), length);
                    }

                    // Retrieve image
                    if (length != -1 && IMAGE_TEST) {
                        retrieveImageData(&varData, length, key, "test", ".png");
                    }

                    // printf("Key: %lu Data: %lu Var: %s\n", key, *((int32_t *)recordBuffer), varData);
                    free(varData);

                    if (i % stepSize == 0) {
                        l = i / stepSize - 1;
                        printf("Num: %lu KEY: %lu\n", i, key);
                        if (l < NUM_STEPS && l >= 0) {
                            rtimes[l][r] = ((clock() - start) * 1000) / CLOCKS_PER_SEC;
                            rreads[l][r] = state->numReads;
                            rhits[l][r] = state->bufferHits;
                        }
                    }
                    i++;
                }
            } else {
                /* Data value query for given value range */
                int8_t success = 1;
                int32_t *itKey, *itData;
                sbitsIterator it;
                it.minKey = NULL;
                it.maxKey = NULL;
                int32_t mv = 800;
                int32_t v = 1000;
                it.minData = &mv;
                it.maxData = &v;
                int32_t rec, reads;

                start = clock();
                mv = 280;
                // for (int i = 0; i < 1000; i++)
                // for (int i = 0; i < 16; i++)
                for (int i = 0; i < 65; i++) {
                    // mv = (rand() % 60 + 30) * 10;
                    // mv += 30;
                    mv += 10;
                    v = mv;

                    resetStats(state);
                    sbitsInitIterator(state, &it);
                    rec = 0;
                    reads = state->numReads;
                    // printf("Min: %d Max: %d\n", mv, v);
                    while (sbitsNext(state, &it, (void **)&itKey,
                                     (void **)&itData)) {
                        // printf("Key: %d  Data: %d\n", *itKey, *itData);
                        if (*((int32_t *)itData) < *((int32_t *)it.minData) ||
                            *((int32_t *)itData) > *((int32_t *)it.maxData)) {
                            success = 0;
                            printf("Key: %d Data: %d Error\n", *itKey, *itData);
                        }
                        rec++;
                    }
                    // printf("Read records: %d\n", rec);
                    // printStats(state);
                    printf("Num: %lu KEY: %lu Perc: %d Records: %d Reads: %d \n", i, mv, ((state->numReads - reads) * 1000 / (state->nextPageWriteId - 1)), rec, (state->numReads - reads));

                    if (i % 100 == 0) {
                        l = i / 100 - 1;
                        printf("Num: %lu KEY: %lu Records: %d Reads: %d\n", i,
                               mv, rec, (state->numReads - reads));
                        if (l < NUM_STEPS && l >= 0) {
                            rtimes[l][r] =
                                ((clock() - start) * 1000) / CLOCKS_PER_SEC;
                            rreads[l][r] = state->numReads;
                            rhits[l][r] = state->bufferHits;
                        }
                    }
                }
            }
        }

        end = clock();
        l = NUM_STEPS - 1;
        rtimes[l][r] = ((double)(end - start) * 1000) / CLOCKS_PER_SEC;
        uint32_t clocks = CLOCKS_PER_SEC;
        rreads[l][r] = state->numReads;
        rhits[l][r] = state->bufferHits;
        printf("Elapsed Time: %lu ms\n", rtimes[l][r]);
        printf("Records queried: %lu\n", i);

        printStats(state);

        printf("Done\n");

        // Optional: Test iterator
        // testIterator(state);
        // printStats(state);

        free(recordBuffer);
        fclose(state->file);
        free(state->buffer);
        free(state);
    }

    // Prints results
    uint32_t sum;
    for (count_t i = 1; i <= NUM_STEPS; i++) {
        printf("Stats for %lu:\n", i * stepSize);

        printf("Reads:   ");
        sum = 0;
        for (r = 0; r < NUM_RUNS; r++) {
            sum += reads[i - 1][r];
            printf("\t%lu", reads[i - 1][r]);
        }
        printf("\t%lu\n", sum / r);

        printf("Writes: ");
        sum = 0;
        for (r = 0; r < NUM_RUNS; r++) {
            sum += writes[i - 1][r];
            printf("\t%lu", writes[i - 1][r]);
        }
        printf("\t%lu\n", sum / r);

        printf("Overwrites: ");
        sum = 0;
        for (r = 0; r < NUM_RUNS; r++) {
            sum += overwrites[i - 1][r];
            printf("\t%lu", overwrites[i - 1][r]);
        }
        printf("\t%lu\n", sum / r);

        printf("Totwrites: ");
        sum = 0;
        for (r = 0; r < NUM_RUNS; r++) {
            sum += overwrites[i - 1][r] + writes[i - 1][r];
            printf("\t%lu", overwrites[i - 1][r] + writes[i - 1][r]);
        }
        printf("\t%lu\n", sum / r);

        printf("Buffer hits: ");
        sum = 0;
        for (r = 0; r < NUM_RUNS; r++) {
            sum += hits[i - 1][r];
            printf("\t%lu", hits[i - 1][r]);
        }
        printf("\t%lu\n", sum / r);

        printf("Write Time: ");
        sum = 0;
        for (r = 0; r < NUM_RUNS; r++) {
            sum += times[i - 1][r];
            printf("\t%lu", times[i - 1][r]);
        }
        printf("\t%lu\n", sum / r);

        printf("R Time: ");
        sum = 0;
        for (r = 0; r < NUM_RUNS; r++) {
            sum += rtimes[i - 1][r];
            printf("\t%lu", rtimes[i - 1][r]);
        }
        printf("\t%lu\n", sum / r);

        printf("R Reads: ");
        sum = 0;
        for (r = 0; r < NUM_RUNS; r++) {
            sum += rreads[i - 1][r];
            printf("\t%lu", rreads[i - 1][r]);
        }
        printf("\t%lu\n", sum / r);

        printf("R Buffer hits: ");
        sum = 0;
        for (r = 0; r < NUM_RUNS; r++) {
            sum += rhits[i - 1][r];
            printf("\t%lu", rhits[i - 1][r]);
        }
        printf("\t%lu\n", sum / r);
    }
}

/* A bitmap with 8 buckets (bits). Range 0 to 100. */
void updateBitmapInt8Bucket(void *data, void *bm) {
    // Note: Assuming int key is right at the start of the data record
    int32_t val = *((int16_t *)data);
    uint8_t *bmval = (uint8_t *)bm;

    if (val < 10)
        *bmval = *bmval | 128;
    else if (val < 20)
        *bmval = *bmval | 64;
    else if (val < 30)
        *bmval = *bmval | 32;
    else if (val < 40)
        *bmval = *bmval | 16;
    else if (val < 50)
        *bmval = *bmval | 8;
    else if (val < 60)
        *bmval = *bmval | 4;
    else if (val < 100)
        *bmval = *bmval | 2;
    else
        *bmval = *bmval | 1;
}

/* A bitmap with 8 buckets (bits). Range 0 to 100. Build bitmap based on min and
 * max value.
 */
void buildBitmapInt8BucketWithRange(void *min, void *max, void *bm) {
    /* Note: Assuming int key is right at the start of the data record */
    uint8_t *bmval = (uint8_t *)bm;

    if (min == NULL && max == NULL) {
        *bmval = 255; /* Everything */
    } else {
        int8_t i = 0;
        uint8_t val = 128;
        if (min != NULL) {
            /* Set bits based on min value */
            updateBitmapInt8Bucket(min, bm);

            /* Assume here that bits are set in increasing order based on
             * smallest value */
            /* Find first set bit */
            while ((val & *bmval) == 0 && i < 8) {
                i++;
                val = val / 2;
            }
            val = val / 2;
            i++;
        }
        if (max != NULL) {
            /* Set bits based on min value */
            updateBitmapInt8Bucket(max, bm);

            while ((val & *bmval) == 0 && i < 8) {
                i++;
                *bmval = *bmval + val;
                val = val / 2;
            }
        } else {
            while (i < 8) {
                i++;
                *bmval = *bmval + val;
                val = val / 2;
            }
        }
    }
}

int8_t inBitmapInt8Bucket(void *data, void *bm) {
    uint8_t *bmval = (uint8_t *)bm;

    uint8_t tmpbm = 0;
    updateBitmapInt8Bucket(data, &tmpbm);

    // Return a number great than 1 if there is an overlap
    return tmpbm & *bmval;
}

/* A 16-bit bitmap on a 32-bit int value */
void updateBitmapInt16(void *data, void *bm) {
    int32_t val = *((int32_t *)data);
    uint16_t *bmval = (uint16_t *)bm;

    /* Using a demo range of 0 to 100 */

    // int16_t stepSize = 100 / 15;
    int16_t stepSize = 450 / 15; // Temperature data in F. Scaled by 10. */
    int16_t minBase = 320;
    int32_t current = minBase;
    uint16_t num = 32768;
    while (val > current) {
        current += stepSize;
        num = num / 2;
    }

    /* Always set last bit if value bigger than largest cutoff */
    if (num == 0)
        num = 1;
    *bmval = *bmval | num;
}

int8_t inBitmapInt16(void *data, void *bm) {
    uint16_t *bmval = (uint16_t *)bm;

    uint16_t tmpbm = 0;
    updateBitmapInt16(data, &tmpbm);

    // Return a number great than 1 if there is an overlap
    return tmpbm & *bmval;
}

/* A 64-bit bitmap on a 32-bit int value */
void updateBitmapInt64(void *data, void *bm) {
    int32_t val = *((int32_t *)data);

    // Temperature data in F. Scaled by 10. */
    int16_t stepSize = 10;

    int32_t current = 320;
    int8_t bmsize = 63;
    int8_t count = 0;

    while (val > current && count < 63) {
        current += stepSize;
        count++;
    }
    uint8_t b = 128;
    int8_t offset = count / 8;
    b = b >> (count & 7);

    *((char *)((char *)bm + offset)) = *((char *)((char *)bm + offset)) | b;
}

int8_t inBitmapInt64(void *data, void *bm) {
    uint64_t *bmval = (uint64_t *)bm;

    uint64_t tmpbm = 0;
    updateBitmapInt64(data, &tmpbm);

    // Return a number great than 1 if there is an overlap
    return tmpbm & *bmval;
}

int8_t int32Comparator(void *a, void *b) {
    int32_t result = *((int32_t *)a) - *((int32_t *)b);
    if (result < 0)
        return -1;
    if (result > 0)
        return 1;
    return 0;
}

uint32_t randomData(void **data, uint32_t sizeLowerBound, uint32_t sizeUpperBound) {
    uint32_t size = rand() % (sizeUpperBound - sizeLowerBound) + sizeLowerBound;
    *data = malloc(size);
    for (uint32_t i = 0; i < size; i++) {
        *((uint8_t *)(*data) + i) = rand() % UINT8_MAX;
    }
    return size;
}

uint32_t readImageFromFile(void **data, char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Failed to open the file\n");
        return -1;
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate memory to store the file data
    *data = (char *)malloc(file_size);
    if (*data == NULL) {
        printf("Failed to allocate memory\n");
        fclose(file);
        return -1;
    }

    // Read the file data into the buffer
    size_t bytes_read = fread(*data, 1, file_size, file);
    if (bytes_read != file_size) {
        printf("Failed to read the file\n");
        free(*data);
        fclose(file);
        return 1;
    }

    fclose(file);

    return file_size;
}

void writeDataToFile(void *data, char *filename, uint32_t length) {
    FILE *file = fopen(filename, "w+b");
    if (file == NULL) {
        printf("Failed to open the file\n");
        return;
    }

    // Write the data to the file
    size_t bytes_written = fwrite(data, 1, length, file);
    if (bytes_written != length) {
        printf("Failed to write to the file\n");
    }

    fclose(file);
}

void imageVarData(float chance, char *filename, uint8_t *usingVarData, uint32_t *length, void **varData) {
    *usingVarData = (rand() % 100) / 100.0 < chance;
    if (usingVarData) {
        *length = readImageFromFile(varData, filename);
        // printf("Length from file: %i\n", *length);
        if (*length == -1) {
            printf("ERROR: Failed to read image '%s'\n", filename);
            exit(-1);
        }
    } else {
        *length = 0;
        *varData = NULL;
    }
}

void randomVarData(float chance, uint32_t sizeLowerBound, uint32_t sizeUpperBound, uint8_t *usingVarData, uint32_t *length, void **varData) {
    *usingVarData = (rand() % 100) / 100.0 < chance;
    if (usingVarData) {
        *length = randomData(varData, sizeLowerBound, sizeUpperBound);
    } else {
        *length = 0;
        *varData = NULL;
    }
}

void retrieveImageData(void **varData, uint32_t length, int32_t key, char *filename, char *filetype) {
    int numDigits = log10(key) + 1;
    char *keyAsString = calloc(numDigits, sizeof(char));
    itoa(key, keyAsString, 10);
    uint32_t filenameLength = strlen(filename);
    uint32_t filetypeLength = strlen(filetype);
    uint32_t totalLength = filenameLength + numDigits + filetypeLength;
    char *file = calloc(totalLength, sizeof(char));
    strncpy(file, filename, filenameLength);
    strncpy(file + filenameLength, keyAsString, numDigits);
    strncpy(file + filenameLength + numDigits, filetype, filetypeLength);
    strncpy(file + totalLength, "\0", 1);
    writeDataToFile(*varData, file, length);
}

uint8_t dataEquals(void *varData, uint32_t length, Node *node) {
    return length == node->length && memcmp(varData, node->data, length) == 0;
}

int retrieveData(sbitsState *state, int32_t key, int8_t *recordBuffer) {
    void *varData = NULL;
    uint32_t length = -1;
    int8_t result = sbitsGetVar(state, &key, recordBuffer, &varData, &length);

    if (result == -1) {
        printf("ERROR: Failed to find: %lu\n", key);
    } else if (result == 1) {
        printf("WARN: Variable data associated with key %lu was deleted\n", key);
    } else if (*((int32_t *)recordBuffer) != key % 100) {
        printf("ERROR: Wrong data for: %lu\n", key);
        // printf("Key: %lu Data: %lu Var length: %d\n", key, *((int32_t*)recordBuffer), length);
    }

    // Retrieve image
    if (length != -1 && IMAGE_TEST) {
        retrieveImageData(&varData, length, key, "test", ".png");
    }

    // printf("Key: %lu Data: %lu Var: %s\n", key, *((int32_t *)recordBuffer), varData);
    free(varData);
    return 0;
}
