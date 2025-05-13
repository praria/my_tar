#include "tar.h"
#include <unistd.h>

int str_cmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(const unsigned char *)a - *(const unsigned char *)b;
}

int str_len(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

void write_usage() {
    const char *msg = "Usage:\n"
                      "  ./my_tar -cf archive.tar file1 [file2 ...]\n"
                      "  ./my_tar -tf archive.tar\n"
                      "  ./my_tar -xf archive.tar\n"
                      "  ./my_tar -uf archive.tar file1 [file2 ...]\n"
                      "  ./my_tar -rf archive.tar file1 [file2 ...]\n";
    write(2, msg, str_len(msg));
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        write_usage();
        return 1;
    }

    const char *mode = argv[1];
    const char *tar_name = argv[2];

    // create archive mode 
    if (str_cmp(mode, "-cf") == 0) {
        if (argc < 4) {
            write_usage();
            return 1;
        }
        const char **files = (const char **)&argv[3];
        int file_count = argc - 3;

        if (create_archive(tar_name, files, file_count) != 0) {
            const char *err = "my_tar: Failed to create archive\n";
            write(2, err, str_len(err));
            return 1;
        }
        return 0;
    }
    // list mode
    else if (str_cmp(mode, "-tf") == 0) {
        if (argc != 3) {
            write_usage();
            return 1;
        }

        if (list_archive(tar_name) != 0) {
            const char *err = "my_tar: Failed to list archive\n";
            write(2, err, str_len(err));
            return 1;
        }
        return 0;
    }

    // extract mode
    else if (str_cmp(mode, "-xf") == 0) {
        if (argc != 3) {
            write_usage();
            return 1;
        }

        if (extract_archive(tar_name) != 0) {
            const char *err = "my_tar: Failed to extract archive\n";
            write(2, err, str_len(err));
            return 1;
        }
        return 0;
    }

    // update mode
    else if (mode[0] == '-' && mode[1] == 'u' && mode[2] == 'f' && mode[3] == '\0') {
        if (argc < 4) {
            write_usage();
            return 1;
        }

        const char **files = (const char **)&argv[3];
        int file_count = argc - 3;

        if (update_archive(tar_name, files, file_count) != 0) {
            const char *err = "my_tar: Failed to update archive\n";
            write(2, err, str_len(err));
            return 1;
        }

        return 0;
    }

    // append mode
    else if (mode[0] == '-' && mode[1] == 'r' && mode[2] == 'f' && mode[3] == '\0') {
        if (argc < 4) {
            write_usage();
            return 1;
        }

        const char **files = (const char **)&argv[3];
        int file_count = argc - 3;

        if (append_to_archive(tar_name, files, file_count) != 0) {
            const char *err = "my_tar: Failed to append to archive\n";
            write(2, err, str_len(err));
            return 1;
        }

        return 0;
    }

    // unsupported mode
    const char *err = "Error: Supported modes are -cf, -tf, -xf, -uf, and -rf only.\n";
    write(2, err, str_len(err));
    return 1;
}