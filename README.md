# EmbedDB Embedded Database for Time Series Data

EmbedDB is a high performance embedded data storage and index structure optimized for time series data on embedded systems. It supports key-value and relational data and runs on a wide variety of embedded devices. EmbedDB does not require an operating system and outperforms other systems, including SQLite, on small embedded systems. Key features:

1. Minimum memory requirement is 4 KB allowing execution on the smallest devices.
2. Key-value store optimized for time series with extremely fast insert performance.
3. Efficient insert (put) and query (get) of arbitrary key-value data. Ability to search data both on timestamp (key) and by data value.
4. High-performance [learned index for keys](https://arxiv.org/abs/2302.03085) and efficient, [customizable data index](docs/SBITS_time_series_index.pdf) optimized for flash memory that outperforms B+-trees.
5. Supports any type of storage including raw NOR and NAND chips and SD cards.
6. No dependencies on libraries or need for an operating system.
7. Advanced query API for SQL queries, which can be written by hand or by using our [SQL converter](https://github.com/ubco-db/EmbedDB-SQL)
8. Easily included in C projects.
9. Open source license. Free to use for commerical and open source projects.

english 10


**Note: This version is designed to run on a PC but a embedded version is [also available](https://github.com/ubco-db/EmbedDB)**

## License

[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

## Example Usage

```c
embedDBState* state = (embedDBState*) malloc(sizeof(embedDBState));
state->keySize = 4;  
state->dataSize = 12;

// Function pointers that can compare keys and data (user customizable)
state->compareKey = int32Comparator;
state->compareData = dataComparator;

// Storage configuration
state->pageSize = 512;
state->eraseSizeInPages = 4;
state->numDataPages = 1000;
state->numIndexPages = 48;
char dataPath[] = "dataFile.bin", indexPath[] = "indexFile.bin", varPath[] = "varFile.bin";
state->fileInterface = getFileInterface();
state->dataFile = setupFile(dataPath);
state->indexFile = setupFile(indexPath);

// Configure memory buffers
state->bufferSizeInBlocks = 2; // Minimum 2 buffers is required for read/write operations
state->buffer = malloc((size_t) state->bufferSizeInBlocks * state->pageSize);

// Initialize
embedDBInit(state, splineMaxError);

// Store record
uint32_t key = 123;
char data[12] = "TEST DATA";
embedDBPut(state, (void*) &key, dataPtr);

// Get record
embedDBGet(state, (void*) &key, (void*) returnDataPtr);
```

## Code Files

-   [utilityFunctions](src/embedDB/utilityFunctions.c) - User defined functions with common/default configurations to get started.
-   [advancedQueries](src/embedDB/utilityFunctions.c) - An included library with easy to use query operators.
-   [embedDBExample](examples/embedDBExample.c) - An example file demonstrating how to get, put, and iterate through data in index. Try by using `make embedDBExample`
-   [embedDBVariableDataExample](examples/embedDBVariableDataExample.c) - An example file demonstrating the use of records with variable-sized data. Try using with `make embedDBVariableDataExample`
-   [embedDBQueryInterfaceExamples](examples/advancedQueryInterfaceExample.c) - An example file demonstrating the included embedDB library. Try by using with `make queryExample`
-   [embedDB.h](src/embedDB/embedDB.h), [embedDB.c](src/embedDB/embedDB.c) - Core database functionality
-   [spline.c](src/spline/spline.c) - Implementation of spline index structure.
-   [radixSpline.c](src/spline/radixspline.c) - Implementation of radix spline index. structure.

## Documentation

A paper describing EmbedDB use for time series indexing is [available from the publisher](https://www.scitepress.org/Link.aspx?doi=10.5220/0010318800920099) and a [pre-print is also available](SBITS_time_series_index.pdf).

More detail regarding learned indexes can be also be found [from the publisher.](https://arxiv.org/abs/2302.03085)

### Additional documentation files

-   [Setup & usage](docs/usageInfo.md)
-   [Simple Query Interface](docs/advancedQueries.md)
-   [Setting up a file interface](docs/fileInterface.md)
-   [Building project](docs/buildRunInformation.md)
-   [Test suite](docs/testInfo.md)

<br>University of British Columbia Okanagan</br>

t 
