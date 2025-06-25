#include <sys/stat.h>  // mkdir
#include <sys/types.h>

int createOutputFolder() {
    // 0755 = rwxr-xr-x permissions
    int res = mkdir("outputs", 0755);
    if (res == 0) {
        printf("Created outputs directory\n");
    } else if (res == -1) {
        // Directory may already exist
        // You can check errno for EEXIST or ignore the error if it exists
    }
    return res;
}

// Then in your main or initialization:
