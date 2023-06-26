#include <math.h>
#include <string.h>

#include "../src/sbits/sbits.h"
#include "../src/sbits/utilityFunctions.h"
#include "unity.h"

sbitsState *state;

void setUp(void) {
    state = (sbitsState *)malloc(sizeof(sbitsState));
    state->keySize = 4;
    state->dataSize = 4;
    state->pageSize = 512;
    state->bufferSizeInBlocks = 6;
    state->buffer = calloc(1, state->pageSize * state->bufferSizeInBlocks);
    state->fileInterface = getFileInterface();
    char dataPath[] = "build/artifacts/dataFile.bin", indexPath[] = "build/artifacts/indexFile.bin", varPath[] = "build/artifacts/varFile.bin";
    state->dataFile = setupFile(dataPath);
    state->indexFile = setupFile(indexPath);
    state->numDataPages = 10000;
    state->eraseSizeInPages = 2;
    state->numIndexPages = 4;
    state->bitmapSize = 1;
    state->parameters = SBITS_USE_INDEX | SBITS_RESET_DATA;
    state->inBitmap = inBitmapInt8;
    state->updateBitmap = updateBitmapInt8;
    state->buildBitmapFromRange = buildBitmapInt8FromRange;
    state->compareKey = int32Comparator;
    state->compareData = int32Comparator;
    int8_t result = sbitsInit(state, 1);
    TEST_ASSERT_EQUAL_INT8_MESSAGE(0, result, "SBITS did not initialize correctly.");
}

void initalizeSbitsFromFile() {
    state = (sbitsState *)malloc(sizeof(sbitsState));
    state->keySize = 4;
    state->dataSize = 4;
    state->pageSize = 512;
    state->bufferSizeInBlocks = 6;
    state->buffer = calloc(1, state->pageSize * state->bufferSizeInBlocks);
    state->fileInterface = getFileInterface();
    char dataPath[] = "build/artifacts/dataFile.bin", indexPath[] = "build/artifacts/indexFile.bin", varPath[] = "build/artifacts/varFile.bin";
    state->dataFile = setupFile(dataPath);
    state->indexFile = setupFile(indexPath);
    state->numDataPages = 10000;
    state->numIndexPages = 4;
    state->eraseSizeInPages = 2;
    state->bitmapSize = 1;
    state->parameters = SBITS_USE_INDEX;
    state->inBitmap = inBitmapInt8;
    state->updateBitmap = updateBitmapInt8;
    state->buildBitmapFromRange = buildBitmapInt8FromRange;
    state->compareKey = int32Comparator;
    state->compareData = int32Comparator;
    int8_t result = sbitsInit(state, 1);
    TEST_ASSERT_EQUAL_INT8_MESSAGE(0, result, "SBITS did not initialize correctly.");
}

void insertRecordsLinearly(int32_t startingKey, int32_t startingData, int32_t numRecords) {
    int8_t *data = (int8_t *)malloc(state->recordSize);
    *((int32_t *)data) = startingKey;
    *((int32_t *)(data + 4)) = startingData;
    for (int i = 0; i < numRecords; i++) {
        *((int32_t *)data) += 1;
        *((int64_t *)(data + 4)) += 1;
        int8_t result = sbitsPut(state, data, (void *)(data + 4));
        TEST_ASSERT_EQUAL_INT8_MESSAGE(0, result, "sbitsPut did not correctly insert data (returned non-zero code)");
    }
    free(data);
}

void tearDown(void) {
    free(state->buffer);
    sbitsClose(state);
    tearDownFile(state->dataFile);
    tearDownFile(state->indexFile);
    free(state->fileInterface);
    free(state);
}

void sbits_index_file_correctly_reloads_with_no_data() {
    tearDown();
    initalizeSbitsFromFile();
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(496, state->maxIdxRecordsPerPage, "SBITS maxIdxRecordsPerPage was initialized incorrectly when no data was present in the index file.");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, state->nextIdxPageId, "SBITS nextIdxPageId was initialized incorrectly when no data was present in the index file.");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(4, state->numAvailIndexPages, "SBITS nextIdxPageId was initialized incorrectly when no data was present in the index file.");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, state->minIndexPageId, "SBITS minIndexPageId was initialized incorrectly when no data was present in the index file.");
}

void sbits_index_file_correctly_reloads_with_one_page_of_data() {
    insertRecordsLinearly(100, 100, 31312);
    tearDown();
    initalizeSbitsFromFile();
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, state->nextIdxPageId, "SBITS nextIdxPageId was initialized incorrectly when one index page was present in the index file.");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(3, state->numAvailIndexPages, "SBITS nextIdxPageId was initialized incorrectly when one index page was present in the index file.");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, state->minIndexPageId, "SBITS minIndexPageId was initialized incorrectly when one index page was present in the index file.");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(sbits_index_file_correctly_reloads_with_no_data);
    RUN_TEST(sbits_index_file_correctly_reloads_with_one_page_of_data);
    return UNITY_END();
}
