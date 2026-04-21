#include "tree.h"
#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Build tree from index
int tree_from_index(ObjectID *id_out) {
    Index index;

    if (index_load(&index) != 0) {
        return -1;
    }

    if (index.count == 0) {
        return -1;
    }

    // Build text format tree
    char buffer[8192];
    int offset = 0;

    for (int i = 0; i < index.count; i++) {
        IndexEntry *e = &index.entries[i];

        char hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&e->hash, hex);

        offset += snprintf(buffer + offset,
                           sizeof(buffer) - offset,
                           "%o blob %s %s\n",
                           e->mode,
                           hex,
                           e->path);
    }

    // Write tree object
    if (object_write(OBJ_TREE, buffer, offset, id_out) != 0) {
        return -1;
    }

    return 0;
}
// phase2 step1
// phase2 step2
// phase2 step3
// phase2 step3
// phase2 step4
// phase2 step5
// phase3 step1
// phase3 step2
// phase3 step3
// phase3 step4
// phase3 step5
