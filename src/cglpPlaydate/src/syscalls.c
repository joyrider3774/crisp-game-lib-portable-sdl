#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>      // For O_RDWR, O_WRONLY, etc.
#include "pd_api.h"

// Global Playdate API reference
extern PlaydateAPI* pd;

// File descriptor to SDFile* mapping table
#define MAX_OPEN_FILES 16
static SDFile* fd_table[MAX_OPEN_FILES] = {0};

// Helper to find first available fd
static int allocate_fd(SDFile* file) {
    for (int i = 3; i < MAX_OPEN_FILES; i++) {  // Start at 3 (after stdin/stdout/stderr)
        if (fd_table[i] == NULL) {
            fd_table[i] = file;
            return i;
        }
    }
    return -1;  // No free slots
}

// Helper to get SDFile* from fd
static SDFile* get_file_from_fd(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return NULL;
    return fd_table[fd];
}

// Helper to free fd
static void free_fd(int fd) {
    if (fd >= 0 && fd < MAX_OPEN_FILES) {
        fd_table[fd] = NULL;
    }
}

int _open(const char *name, int flags, int mode) {
    int pdFlags = kFileRead;  // Default to read mode
    
    // Convert standard flags to Playdate flags
    if ((flags & O_RDWR) == O_RDWR) {
        pdFlags = kFileRead | kFileWrite;
    } else if ((flags & O_WRONLY) == O_WRONLY) {
        pdFlags = kFileWrite;
    } else {
        pdFlags = kFileRead;
    }

    SDFile* file = pd->file->open(name, pdFlags);
    if (file == NULL) {
        errno = ENOENT;
        return -1;
    }

    int fd = allocate_fd(file);
    if (fd == -1) {
        pd->file->close(file);
        errno = EMFILE;
        return -1;
    }

    return fd;
}

int _close(int file) {
    SDFile* f = get_file_from_fd(file);
    if (f == NULL) {
        errno = EBADF;
        return -1;
    }

    int result = pd->file->close(f);
    free_fd(file);
    return result;
}

int _read(int file, char *ptr, int len) {
    if (file < 3) return 0;  // Handle stdin/stdout/stderr

    SDFile* f = get_file_from_fd(file);
    if (f == NULL) {
        errno = EBADF;
        return -1;
    }

    return pd->file->read(f, ptr, len);
}

int _write(int file, char *ptr, int len) {
    // Handle stdout/stderr
    if (file == 1 || file == 2) {
        char tmp[len + 1];
        memcpy(tmp, ptr, len);
        tmp[len] = '\0';
        pd->system->logToConsole("%s", tmp);
        return len;
    }

    SDFile* f = get_file_from_fd(file);
    if (f == NULL) {
        errno = EBADF;
        return -1;
    }

    return pd->file->write(f, ptr, len);
}

int _lseek(int file, int ptr, int dir) {
    if (file < 3) return 0;  // Handle stdin/stdout/stderr

    SDFile* f = get_file_from_fd(file);
    if (f == NULL) {
        errno = EBADF;
        return -1;
    }

    return pd->file->seek(f, ptr, dir);
}

int _fstat(int file, struct stat *st) {
    // For now, just mark everything as character device
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    // Return true for stdin/stdout/stderr
    return (file >= 0 && file <= 2);
}

void _exit(int status) {
    pd->system->logToConsole("Exit called with status %d", status);
    while(1);
}

int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}