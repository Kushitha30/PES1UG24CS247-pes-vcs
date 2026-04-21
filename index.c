#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

// ─── PROVIDED FUNCTIONS ─────────────────────────────────────────

// Find entry by path
IndexEntry* index_find(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }
    return NULL;
}

// Remove entry
int index_remove(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            memmove(&index->entries[i],
                    &index->entries[i + 1],
                    (index->count - i - 1) * sizeof(IndexEntry));
            index->count--;
            return index_save(index);
        }
    }
    return -1;
}

// Show status
int index_status(const Index *index) {
    printf("Staged changes:\n");

    if (index->count == 0)
        printf("  (nothing to show)\n");

    for (int i = 0; i < index->count; i++)
        printf("  staged:     %s\n", index->entries[i].path);

    printf("\nUnstaged changes:\n");
    printf("  (nothing to show)\n");

    printf("\nUntracked files:\n");

    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *ent;

        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            if (strcmp(ent->d_name, "pes") == 0) continue;

            int tracked = 0;
            for (int i = 0; i < index->count; i++) {
                if (strcmp(index->entries[i].path, ent->d_name) == 0) {
                    tracked = 1;
                    break;
                }
            }

            if (!tracked)
                printf("  untracked:  %s\n", ent->d_name);
        }

        closedir(dir);
    }

    printf("\n");
    return 0;
}

// ─── YOUR IMPLEMENTATION ─────────────────────────────────────────

// Load index
int index_load(Index *index) {
    index->count = 0;

    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) return 0;

    char line[1024];

    while (fgets(line, sizeof(line), f)) {
        IndexEntry *e = &index->entries[index->count];

        char hash_hex[HASH_HEX_SIZE + 1];

        if (sscanf(line, "%o %s %ld %u %s",
                   &e->mode,
                   hash_hex,
                   &e->mtime_sec,
                   &e->size,
                   e->path) != 5)
            continue;

        hex_to_hash(hash_hex, &e->hash);
        index->count++;
    }

    fclose(f);
    return 0;
}

// Save index
int index_save(const Index *index) {
    FILE *f = fopen(INDEX_FILE, "w");
    if (!f) return -1;

    for (int i = 0; i < index->count; i++) {
        const IndexEntry *e = &index->entries[i];

        char hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&e->hash, hex);

        fprintf(f, "%o %s %ld %u %s\n",
                e->mode,
                hex,
                e->mtime_sec,
                e->size,
                e->path);
    }

    fclose(f);
    return 0;
}

// Add file to index
int index_add(Index *index, const char *path) {
    struct stat st;

    if (stat(path, &st) != 0) return -1;

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    void *data = malloc(st.st_size);
    if (!data) return -1;

    fread(data, 1, st.st_size, f);
    fclose(f);

    ObjectID id;

    if (object_write(OBJ_BLOB, data, st.st_size, &id) != 0) {
        free(data);
        return -1;
    }

    free(data);

    IndexEntry *e = index_find(index, path);

    if (!e)
        e = &index->entries[index->count++];

    e->mode = 0100644;
    e->hash = id;
    e->mtime_sec = st.st_mtime;
    e->size = st.st_size;
    strcpy(e->path, path);

    return index_save(index);
}
