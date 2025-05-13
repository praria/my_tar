#include "tar.h"
#include <fcntl.h>      
#include <unistd.h>     
#include <sys/stat.h>   
#include <stdlib.h>     
#include <pwd.h>      
#include <grp.h>       
#include <utime.h>

#define BLOCK_SIZE 512

// custom strlen implementation
static int str_len(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

// zero out a block of memory
static void zero_block(char *block, int size) {
    for (int i = 0; i < size; i++)
        block[i] = '\0';
}

// convert integer to octal string
static void int_to_octal(unsigned int val, char *buf, int size) {
    for (int i = size - 2; i >= 0; i--) {
        buf[i] = '0' + (val & 7);
        val >>= 3;
    }
    buf[size - 1] = '\0';
}

// calculate checksum for tar header
static unsigned int calculate_checksum(char *header) {
    unsigned int sum = 0;
    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (i >= 148 && i < 156)
            sum += 32;
        else
            sum += (unsigned char)header[i];
    }
    return sum;
}

// fill header with file metadata
static void fill_header(const char *filename, struct stat *st, char *header) {
    zero_block(header, BLOCK_SIZE);

    int name_len = str_len(filename);
    for (int i = 0; i < name_len && i < 100; i++)
        header[i] = filename[i];

    header[156] = '0';

    int_to_octal(st->st_mode & 0777, header + 100, 8);
    int_to_octal(st->st_uid, header + 108, 8);
    int_to_octal(st->st_gid, header + 116, 8);
    int_to_octal((unsigned int)st->st_size, header + 124, 12);
    int_to_octal((unsigned int)st->st_mtime, header + 136, 12);

    for (int i = 257, j = 0; j < 6; i++, j++) header[i] = "ustar"[j];
    for (int i = 263, j = 0; j < 2; i++, j++) header[i] = "00"[j];

    unsigned int checksum = calculate_checksum(header);
    int_to_octal(checksum, header + 148, 8);
}

// add a single file to and archive
int add_file_to_archive(int tar_fd, const char *filename){
    struct stat st;
    if (stat(filename, &st) < 0)
        return -1;

    if (!S_ISREG(st.st_mode))
        return -1;

    int fd = open(filename, O_RDONLY);
    if (fd < 0)
        return -1;

    char header[BLOCK_SIZE];
    fill_header(filename, &st, header);

    if (write(tar_fd, header, BLOCK_SIZE) != BLOCK_SIZE) {
        close(fd);
        return -1;
    }

    char buf[BLOCK_SIZE];
    ssize_t bytes;
    while ((bytes = read(fd, buf, BLOCK_SIZE)) > 0) {
        if (bytes < BLOCK_SIZE)
            for (int j = bytes; j < BLOCK_SIZE; j++)
                buf[j] = '\0';

        if (write(tar_fd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
            close(fd);
            return -1;
        }
    }

    close(fd);
    return 0;
}

// write a file to tar archive
int write_file_to_tar( int tar_fd, const char *filename){
    struct stat st;
    if (stat(filename, &st) < 0) {
        const char *prefix = "my_tar: ";
        const char *middle = ": Cannot stat: No such file or directory\n";

        write(2, prefix, str_len(prefix));
        write(2, filename, str_len(filename));
        write(2, middle, str_len(middle));
        return -1;
    }

    if (!S_ISREG(st.st_mode)) 
        return -1;

    return add_file_to_archive(tar_fd, filename);
}


int create_archive(const char *tar_name, const char **files, int file_count) {
    int tar_fd = open(tar_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (tar_fd < 0)
        return -1;

    for (int i = 0; i < file_count; i++) {
        const char *filename = files[i];
        
        write_file_to_tar(tar_fd, filename);

    }

    // end with two empty blocks
    char empty[BLOCK_SIZE];
    zero_block(empty, BLOCK_SIZE);
    write(tar_fd, empty, BLOCK_SIZE);
    write(tar_fd, empty, BLOCK_SIZE);

    close(tar_fd);
    return 0;
}

int list_archive(const char *archive_name) {
    int fd = open(archive_name, O_RDONLY);
    if (fd < 0) {
        write(2, "my_tar: Cannot open archive\n", 28);
        return 1;
    }

    char header[512];
    while (read(fd, header, 512) == 512) {
        int is_empty = 1;
        for (int i = 0; i < 512; i++) {
            if (header[i] != '\0') {
                is_empty = 0;
                break;
            }
        }
        if (is_empty) {
            break;
        }
        int len = 0;
        while (len < 100 && header[len] != '\0') {
            len++;
        }
        write(1, header, len);
        write(1, "\n", 1);
        unsigned int size = 0;
        for (int i = 124; i < 124 + 12 && header[i] != '\0'; i++) {
            size = (size << 3) + (header[i] - '0');
        }
        size_t blocks = (size + 511) / 512;
        if (lseek(fd, blocks * 512, SEEK_CUR) == -1) {
            write(2, "my_tar: Error seeking\n", 23);
            close(fd);
            return 1;
        }
    }

    close(fd);
    return 0;
}

int octal_to_decimal(const char *octal, long *out) {
    long result = 0;
    while (*octal >= '0' && *octal <= '7') {
        result = (result << 3) + (*octal - '0');
        octal++;
    }
    *out = result;
    return 0;
}

int create_missing_dirs(const char *filepath) {
    char path[256];
    int i = 0, j = 0;

    while (filepath[i] != '\0') {
        path[j++] = filepath[i];
        if (filepath[i] == '/') {
            path[j] = '\0';
            mkdir(path, 0755);
        }
        i++;
    }
    return 0;
}

int extract_archive(const char *archive_name) {
    int fd = open(archive_name, O_RDONLY);
    if (fd < 0) {
        write(2, "Error opening archive file\n", 27);
        return 1;
    }

    char block[512];
    while (1) {
        ssize_t bytes_read = read(fd, block, 512);
        if (bytes_read != 512) {
            write(2, "Error reading archive\n", 23);
            break;
        }
        int empty1 = 1;
        for (int i = 0; i < 512; i++) {
            if (block[i] != '\0') {
                empty1 = 0;
                break;
            }
        }

        if (empty1) {
            char next_block[512];
            bytes_read = read(fd, next_block, 512);
            if (bytes_read != 512) {
                write(2, "Truncated archive or read error\n", 32);
                break;
            }

            int empty2 = 1;
            for (int i = 0; i < 512; i++) {
                if (next_block[i] != '\0') {
                    empty2 = 0;
                    break;
                }
            }

            if (empty2) {
                write(2, "Confirmed end of archive\n", 25);
                break;
            } else {
                lseek(fd, -512, SEEK_CUR);
                continue;
            }
        }

        // parse header
        struct posix_header *header = (struct posix_header *)block;

        // extract file size
        long file_size;
        octal_to_decimal(header->size, &file_size);

        // create output file
        int out_fd = open(header->name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) {
            write(2, "Error creating output file\n", 27);
            int skip = ((file_size + 511) / 512) * 512;
            lseek(fd, skip, SEEK_CUR);
            continue;
        }

        int remaining = file_size;
        char buffer[512];

        while (remaining > 0) {
            int to_read = (remaining > 512) ? 512 : remaining;
            bytes_read = read(fd, buffer, 512);
            if (bytes_read != 512) {
                write(2, "Error reading file content\n", 27);
                break;
            }

            write(out_fd, buffer, to_read);
            remaining -= to_read;
        }

        close(out_fd);
    }

    close(fd);
    return 0; 
}


int update_archive(const char *tar_name, const char **files, int file_count){
    int tar_fd = open(tar_name, O_RDWR);
    if (tar_fd < 0) return -1;

    char header[512];
    int found[100];
    for (int i = 0; i < file_count; i++) found[i] = 0;

    while (read(tar_fd, header, 512) == 512) {
        if (header[0] == '\0') break;

        int i;
        for (i = 0; i < file_count; i++) {
            const char *fname = files[i];
            int match = 1;
            for (int j = 0; j < 100; j++) {
                if (header[j] == '\0' && fname[j] == '\0') break;
                if (header[j] != fname[j]) {
                    match = 0;
                    break;
                }
            }
            if (match) break;
        }

        int size = 0;
        for (int j = 0; j < 11; j++) {
            size = size * 8 + (header[124 + j] - '0');
        }

        if (i < file_count) {
            struct stat st;
            if (stat(files[i], &st) == 0) {
                int mtime = 0;
                for (int j = 0; j < 11; j++) {
                    mtime = mtime * 8 + (header[136 + j] - '0');
                }
                if (st.st_mtime > mtime) {
                    found[i] = 2; 
                } else {
                    found[i] = 1; 
                }
            }
        }

        int blocks = (size + 511) / 512;
        lseek(tar_fd, blocks * 512, SEEK_CUR);
    }

    lseek(tar_fd, -1024, SEEK_END);

    for (int i = 0; i < file_count; i++) {
        if (found[i] == 2) {
            if (add_file_to_archive(tar_fd, files[i]) != 0) {
                close(tar_fd);
                return -1;
            }
        }
    }

    char zero_block[512] = {0};
    write(tar_fd, zero_block, 512);
    write(tar_fd, zero_block, 512);
    close(tar_fd);
    return 0;
}

int append_to_archive(const char *tar_name, const char **files, int file_count){
    int tar_fd = open(tar_name, O_RDWR);
    if (tar_fd < 0) return -1;

    char header[512];
    const char *existing[100];
    int existing_count = 0;

    while (read(tar_fd, header, 512) == 512) {
        if (header[0] == '\0') break;
        existing[existing_count++] = header;
        int size = 0;
        for (int j = 0; j < 11; j++) {
            size = size * 8 + (header[124 + j] - '0');
        }
        int blocks = (size + 511) / 512;
        lseek(tar_fd, blocks * 512, SEEK_CUR);
    }

    lseek(tar_fd, -1024, SEEK_END);

    for (int i = 0; i < file_count; i++) {
        int skip = 0;
        for (int j = 0; j < existing_count; j++) {
            const char *fname = files[i];
            int match = 1;
            for (int k = 0; k < 100; k++) {
                if (existing[j][k] == '\0' && fname[k] == '\0') break;
                if (existing[j][k] != fname[k]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                skip = 1;
                break;
            }
        }
        if (!skip) {
            if (add_file_to_archive(tar_fd, files[i]) != 0) {
                close(tar_fd);
                return -1;
            }
        }
    }

    char zero_block[512] = {0};
    write(tar_fd, zero_block, 512);
    write(tar_fd, zero_block, 512);
    close(tar_fd);
    return 0;
}

