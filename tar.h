#ifndef TAR_H
#define TAR_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>

// standard tar block size (512 bytes)
#define BLOCK_SIZE 512

// Tar file header structure, which contains metadata about each file in the archive
struct posix_header {
    char name[100];     // File name (up to 100 chars)
    char mode[8];       // File permission in octal
    char uid[8];        // Owner's numeric user ID
    char gid[8];        // Group's numeric user ID
    char size[12];      // File size in bytes (octal)
    char mtime[12];     // last modification time (octal)
    char chksum[8];     // Checksum for header record
    char typeflag;      // Type flag (e.g., regular file, directory)
    char linkname[100]; // Name of linked file
    char magic[6];      // Magic indicator ("ustar")
    char version[2];    // Version ("00")
    char uname[32];     // User name
    char gname[32];     // Group name
    char devmajor[8];   // Major device number
    char devminor[8];   // Minor device number
    char prefix[155];   // Prefix for long names
};

// Function declarations

int create_archive(const char *tar_name, const char **files, int file_count);
int list_archive(const char *archive_name);
int octal_to_decimal(const char *octal, long *out);
int create_directories(const char *path);
int extract_archive(const char *archive_name);
int update_archive(const char *tar_name, const char **files, int file_count);
int append_to_archive(const char *tar_name, const char **files, int file_count);
int add_file_to_archive(int tar_fd, const char *filename); // helper function to add a single file
int write_file_to_tar(int tar_fd, const char *file_path); // helper function to write file data

#endif