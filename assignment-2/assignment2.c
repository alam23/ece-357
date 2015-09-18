#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <limits.h>
#include <time.h>

struct visited_inode {
    int active;
    long long device_id;
    int inode;
    char *path;
};

struct visited_inode *stack;
int stay_in_disk = -1;
long long target_disk = -1;
int target_inode;

int printdirinfo(const char* path, int uid , int mtime) {
    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            int new_path_len;
            char new_path[PATH_MAX];
            new_path_len = snprintf(new_path, PATH_MAX, "%s/%s", path, dir->d_name);
            if (new_path_len >= PATH_MAX)
                return 0;
            struct stat s;
            lstat(new_path, &s);
            // -x parameter
            if (stay_in_disk == 0) {
                stay_in_disk = (long long)s.st_dev;
            }
            if (stay_in_disk != -1 && stay_in_disk != (long long)s.st_dev) {
                fprintf(stderr, "not crossing mount point at %s\n", new_path);
                continue;
            }
            // . and ..
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, ".."))
                continue;
            // -u parameter
            if (uid !=-1 && s.st_uid != uid)
                continue;
            // -m parameter
            time_t now;
            time(&now);
            if (mtime > 0 && (now - s.st_mtime < mtime))
                continue;
            if (mtime < 0 && (s.st_mtime - now < mtime))
                continue;
            // -l parameter
            if (target_disk != -1) {
                if (!S_ISLNK(s.st_mode))
                    continue;
                int link_len;
                char link[PATH_MAX];
                link_len = readlink(new_path, link, PATH_MAX);
                if (link_len >= PATH_MAX)
                    continue;
                struct stat ls;
                lstat(link, &ls);
                if (target_disk != (long long)ls.st_dev || target_inode != (int)ls.st_ino)
                    continue;
            }
            // FSID/Inode number
            printf("%04llx/", (long long)s.st_dev);
            printf("%d\t", (int)s.st_ino);
            if ((int)s.st_ino < 1000)
                printf("\t");
            // Mask
            if (S_ISDIR(s.st_mode)) printf("d");
            else if (S_ISREG(s.st_mode)) printf("-");
            else if (S_ISCHR(s.st_mode)) printf("c");
            else if (S_ISBLK(s.st_mode)) printf("b");
            else if (S_ISLNK(s.st_mode)) printf("l");
            else if (S_ISFIFO(s.st_mode)) printf("p");
            else if (S_ISSOCK(s.st_mode)) printf("s");
            else {
                fprintf(stderr, "\ninvalid file type\n");
                continue;
            }
            printf((s.st_mode & S_IRUSR) ? "r" : "-");
            printf((s.st_mode & S_IWUSR) ? "w" : "-");
            if (s.st_mode & S_ISUID)
                if (s.st_mode & S_IXUSR)
                    printf("s");
                else
                    printf("S");
            else
                if (s.st_mode & S_IXUSR)
                    printf("x");
                else
                    printf("-");
            printf((s.st_mode & S_IRGRP) ? "r" : "-");
            printf((s.st_mode & S_IWGRP) ? "w" : "-");
            if (s.st_mode & S_ISGID)
                if (s.st_mode & S_IXGRP)
                    printf("s");
                else
                    printf("S");
            else
                if (s.st_mode & S_IXGRP)
                    printf("x");
                else
                    printf("-");
            printf((s.st_mode & S_IROTH) ? "r" : "-");
            printf((s.st_mode & S_IWOTH) ? "w" : "-");
            if (s.st_mode & S_ISVTX)
                if (s.st_mode & S_IXOTH)
                    printf("t");
                else
                    printf("T");
            else
                if (s.st_mode & S_IXOTH)
                    printf("x");
                else
                    printf("-");
            // Links
            printf("\t%d", (int)s.st_nlink);
            // Owner ID
            struct passwd *p = getpwuid((int)s.st_uid);
            printf("\t%s", p->pw_name);
            // Group ID
            struct group *g = getgrgid((int)s.st_gid);
            printf("\t%s", g->gr_name);
            // File size / device number
            if (S_ISCHR(s.st_mode) || S_ISBLK(s.st_mode))
                printf("\t0x%x", (int)s.st_rdev);
            else
                printf("\t%d", (int)s.st_size);
            // Last edit time
            char stime[PATH_MAX];
            struct tm *ltime = localtime(&s.st_mtime);
            strftime(stime, PATH_MAX, "%c", ltime);
            printf("\t%s", stime);
            // Filename
            printf("\t%s", new_path);
            // Links
            if (S_ISLNK(s.st_mode)) {
                int link_len;
                char link[PATH_MAX];
                link_len = readlink(new_path, link, PATH_MAX);
                if (link_len >= PATH_MAX)
                    return 0;
                printf(" -> %s", link);
            }
            printf("\n");
            if (dir->d_type == DT_DIR) {
                // Check to see if path already exists, if it does, make a sandwich and panic
                struct visited_inode *current = stack;
                while (current->active != 0) {
                    if (current->device_id == (long long)s.st_dev && current->inode == (int)s.st_ino) {
                        // Loop detected! Trigger kernel panic!
                        fprintf(stderr, "loop detected, paths: <%s>, <%s>\n", current->path, new_path);
                        return 1;
                    }
                    current++;
                }
                // Add to list
                current->active = 1;
                current->device_id = (long long)s.st_dev;
                current->inode = (int)s.st_ino;
                current->path = malloc(4096);
                strcpy(current->path, new_path);
                if (!printdirinfo(new_path, uid, mtime))
                    return 0;
                //Remove from list
                free(current->path);
                current->active = 0;
            }
        }
    } else {
        if (errno == EMFILE) {
            fprintf(stderr, "max file descriptors reached\n");
            return 0;
        } else {
            fprintf(stderr, "other error: %s\n", strerror(errno));
            return 1;
        }
    }
    closedir(d);
    return 1;
}

int main (int argc, char **argv) {
    extern char *optarg;
    extern int optind;
    int c, uid = -1, mtime = 0;
    struct stat s;
    while ((c = getopt(argc, argv, "u:m:xl:")) != -1) {
        switch (c) {
            case 'u':
                uid = strtol(optarg, NULL, 10);
                struct passwd *p = getpwnam(optarg);
                if (p)
                    uid = p->pw_uid;
                break;
            case 'm':
                mtime = strtol(optarg, NULL, 10);
                break;
            case 'x':
                stay_in_disk = 0;
                break;
            case 'l':
                lstat(optarg, &s);
                target_disk = (long long)s.st_dev;
                target_inode = (int)s.st_ino;
                break;
            default:
                break;
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "invalid arguments\n");
        return 0;
    }
    stack = malloc(sizeof(struct visited_inode) * 4096);
    printdirinfo(argv[optind], uid, mtime);
    return 0;
}