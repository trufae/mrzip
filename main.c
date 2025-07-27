/* main.c – Tiny demo utility for mzip.h
 * Build:  gcc -std=c99 -DMZIP_IMPLEMENTATION main.c -lz -o mzip
 * Usage:  ./mzip -l  archive.zip   # list files
 *         ./mzip -x  archive.zip   # extract into current directory
 *         ./mzip -c  archive.zip file1 file2...  # create new zip archive
 *         ./mzip -a  archive.zip file1 file2...  # add files to existing archive
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mzip.h"

static void usage(void) {
    puts("mzip – minimal ZIP reader/writer (mzip.h demo)\n"
         "Usage: mzip [-l | -x | -c | -a] <archive.zip> [files...]\n"
         "  -l   List contents\n"
         "  -x   Extract all files into current directory\n"
         "  -c   Create new archive with specified files\n"
         "  -a   Add files to existing archive");
}

static int list_files(const char *path) {
    int err = 0;
    zip_t *za = zip_open(path, ZIP_RDONLY, &err);
    if (!za) {
        fprintf(stderr, "Failed to open %s (err=%d)\n", path, err);
        return 1;
    }

    zip_uint64_t n = zip_get_num_files(za);
    for (zip_uint64_t i = 0; i < n; ++i) {
        const char *name = NULL; /* we only have names in directory entries */
        /* mzip stores names inside entries array – expose via zip_name_locate */
        name = ((struct mzip_entry*)za->entries)[i].name; /* hack – internal */
        printf("%3llu  %s\n", (unsigned long long)i, name ? name : "<unknown>");
    }

    zip_close(za);
    return 0;
}

/* Function to create a new ZIP archive or add files to existing one */
static int create_or_add_files(const char *path, char **files, int num_files, int create_mode) {
    int err = 0;
    int flags = create_mode ? (ZIP_CREATE | ZIP_TRUNCATE) : (ZIP_CREATE);
    
    zip_t *za = zip_open(path, flags, &err);
    if (!za) {
        fprintf(stderr, "Failed to %s %s (err=%d)\n", 
                create_mode ? "create" : "open", path, err);
        return 1;
    }
    
    for (int i = 0; i < num_files; i++) {
        const char *filename = files[i];
        
        /* Open file to read content */
        FILE *fp = fopen(filename, "rb");
        if (!fp) {
            fprintf(stderr, "Cannot open file: %s\n", filename);
            continue;
        }
        
        /* Get file size */
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        
        if (file_size < 0) {
            fprintf(stderr, "Error determining file size: %s\n", filename);
            fclose(fp);
            continue;
        }
        
        /* Read file content */
        void *buffer = malloc(file_size);
        if (!buffer) {
            fprintf(stderr, "Out of memory for file: %s\n", filename);
            fclose(fp);
            continue;
        }
        
        if (fread(buffer, 1, file_size, fp) != (size_t)file_size) {
            fprintf(stderr, "Error reading file: %s\n", filename);
            fclose(fp);
            free(buffer);
            continue;
        }
        fclose(fp);
        
        /* Extract just the base filename */
        const char *base_name = filename;
        const char *slash = strrchr(filename, '/');
        if (slash) {
            base_name = slash + 1;
        }
        
        /* Create source and add to archive */
        zip_source_t *src = zip_source_buffer(za, buffer, file_size, 1); /* 1 = free buffer when done */
        if (!src) {
            fprintf(stderr, "Failed to create source for file: %s\n", filename);
            free(buffer);
            continue;
        }
        
        /* Add file to archive */
        zip_int64_t idx = zip_file_add(za, base_name, src, 0);
        if (idx < 0) {
            fprintf(stderr, "Failed to add file to archive: %s\n", filename);
            /* Source buffer is already freed in case of failure */
            continue;
        }
        
        /* For simplicity, use store (no compression) */
        if (zip_set_file_compression(za, idx, 0, 0) != 0) {
            fprintf(stderr, "Warning: Could not set compression for: %s\n", filename);
        }
        
        printf("Added: %s (%ld bytes)\n", base_name, file_size);
    }
    
    /* Close and finalize the zip file */
    zip_close(za);
    return 0;
}

static int extract_all(const char *path) {
    int err=0;
    zip_t *za = zip_open(path, ZIP_RDONLY, &err);
    if (!za) {
        fprintf(stderr, "Failed to open %s (err=%d)\n", path, err);
        return 1;
    }

    zip_uint64_t n = zip_get_num_files(za);
    for (zip_uint64_t i = 0; i < n; ++i) {
        zip_file_t *zf = zip_fopen_index(za, i, 0);
        if (!zf) {
            fprintf(stderr, "Could not read entry %llu\n", (unsigned long long)i);
            continue;
        }

        const char *fname = ((struct mzip_entry*)za->entries)[i].name; /* internal */
        FILE *out = fopen(fname, "wb");
        if (!out) {
            fprintf(stderr, "Cannot create %s\n", fname);
            zip_fclose(zf);
            continue;
        }

        fwrite(zf->data, 1, zf->size, out);
        fclose(out);
        zip_fclose(zf);
        printf("Extracted %s (%zu bytes)\n", fname, (size_t)zf->size);
    }

    zip_close(za);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage();
        return 1;
    }

    int mode_list=0, mode_extract=0, mode_create=0, mode_append=0;
    if (strcmp(argv[1], "-l") == 0) mode_list = 1;
    else if (strcmp(argv[1], "-x") == 0) mode_extract = 1;
    else if (strcmp(argv[1], "-c") == 0) mode_create = 1;
    else if (strcmp(argv[1], "-a") == 0) mode_append = 1;
    else {
        usage();
        return 1;
    }

    const char *zip_path = argv[2];

    if (mode_list) {
        if (argc != 3) {
            usage();
            return 1;
        }
        return list_files(zip_path);
    } else if (mode_extract) {
        if (argc != 3) {
            usage();
            return 1;
        }
        return extract_all(zip_path);
    } else if (mode_create || mode_append) {
        if (argc < 4) {
            fprintf(stderr, "Error: No files specified to %s.\n", 
                    mode_create ? "create archive with" : "add to archive");
            usage();
            return 1;
        }
        return create_or_add_files(zip_path, &argv[3], argc - 3, mode_create);
    }

    usage();
    return 1;
}
