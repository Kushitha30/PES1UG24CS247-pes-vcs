#include "commit.h"
#include "tree.h"
#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ─── CREATE COMMIT ───────────────────────────────────────

int commit_create(const char *message, ObjectID *commit_id_out) {
    ObjectID tree_id;

    if (tree_from_index(&tree_id) != 0)
        return -1;

    char tree_hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&tree_id, tree_hex);

    // read parent commit (if exists)
    char parent_hex[HASH_HEX_SIZE + 1] = "";
    FILE *f = fopen(".pes/refs/heads/main", "r");

    if (f) {
        fgets(parent_hex, sizeof(parent_hex), f);
        fclose(f);
    }

    // remove newline
    parent_hex[strcspn(parent_hex, "\n")] = 0;

    const char *author = pes_author();
    long timestamp = time(NULL);

    char buffer[4096];

    if (strlen(parent_hex) > 0) {
        snprintf(buffer, sizeof(buffer),
            "tree %s\nparent %s\nauthor %s %ld\ncommitter %s %ld\n\n%s\n",
            tree_hex,
            parent_hex,
            author, timestamp,
            author, timestamp,
            message);
    } else {
        snprintf(buffer, sizeof(buffer),
            "tree %s\nauthor %s %ld\ncommitter %s %ld\n\n%s\n",
            tree_hex,
            author, timestamp,
            author, timestamp,
            message);
    }

    if (object_write(OBJ_COMMIT, buffer, strlen(buffer), commit_id_out) != 0)
        return -1;

    // update HEAD
    char commit_hex[HASH_HEX_SIZE + 1];
    hash_to_hex(commit_id_out, commit_hex);

    FILE *hf = fopen(".pes/refs/heads/main", "w");
    if (!hf) return -1;

    fprintf(hf, "%s\n", commit_hex);
    fclose(hf);

    return 0;
}

// ─── PARSE COMMIT ───────────────────────────────────────

int commit_parse(const void *data, size_t len, Commit *commit) {
    memset(commit, 0, sizeof(Commit));

    char *text = malloc(len + 1);
    memcpy(text, data, len);
    text[len] = '\0';

    char *line = strtok(text, "\n");

    while (line) {
        if (strncmp(line, "author ", 7) == 0) {
            sscanf(line + 7, "%[^0-9] %llu",
                   commit->author,
                   &commit->timestamp);
        }
        else if (line[0] == '\0') {
            line = strtok(NULL, "\n");
            if (line) {
                strcpy(commit->message, line);
            }
            break;
        }
        line = strtok(NULL, "\n");
    }

    free(text);
    return 0;
}

// ─── WALK COMMITS (FIXED VERSION) ───────────────────────

int commit_walk(void (*cb)(const ObjectID*, const Commit*, void*), void *ctx) {
    FILE *f = fopen(".pes/refs/heads/main", "r");
    if (!f) return -1;

    char hex[HASH_HEX_SIZE + 1];
    if (!fgets(hex, sizeof(hex), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    hex[strcspn(hex, "\n")] = 0;

    ObjectID id;

    while (strlen(hex) > 0) {
        if (hex_to_hash(hex, &id) != 0) break;

        ObjectType type;
        void *data;
        size_t len;

        if (object_read(&id, &type, &data, &len) != 0) break;

        Commit commit;
        commit_parse(data, len, &commit);

        cb(&id, &commit, ctx);

        // 🔥 FIXED: read parent BEFORE freeing data
        char *text = malloc(len + 1);
        memcpy(text, data, len);
        text[len] = '\0';

        char parent[HASH_HEX_SIZE + 1] = "";

        char *line = strtok(text, "\n");
        while (line) {
            if (strncmp(line, "parent ", 7) == 0) {
                strcpy(parent, line + 7);
                break;
            }
            line = strtok(NULL, "\n");
        }

        free(text);
        free(data);

        if (strlen(parent) == 0)
            break;

        strcpy(hex, parent);
    }

    return 0;
}
