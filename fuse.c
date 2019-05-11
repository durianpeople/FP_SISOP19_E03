#define FUSE_USE_VERSION 28
#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Modifiable constant
static const char *mountable = "/home/durianpeople/Documents/Notes/SISOP/mountable";

// Type definition
typedef struct node
{
    struct node *left;
    struct node *right;
    char *key;
    char *path;
    int duplicate;
} Node;

// Global variables
int shmid;
Node *root = NULL;

// Function constructors
void deducePath(char *dest, const char *source);
void adaptPath(char *dest, const char *source);
const char *get_filename_ext(const char *filename);

// -- Binary search tree
int insert(Node **tree, char *key, char *path, struct stat *st, void *buf, fuse_fill_dir_t *filler);
void deltree(Node *tree);
void displayContentOf(Node *tree);
char *findPath(Node *tree, const char *key);

// -- Assistive function
void listdir(const char *name, void *buf, fuse_fill_dir_t *filler);

// -- FUSE specifics
static int xmp_getattr(const char *path, struct stat *stbuf);
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int xmp_utimens(const char *path, const struct timespec ts[2]);
static int xmp_open(const char *path, struct fuse_file_info *fi);
static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static void *xmp_init(struct fuse_conn_info *conn);
void xmp_destroy(void *userdata);

// -- Thread
void *fuseThread(void *arg);

// FUSE assignment
static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .init = xmp_init,
    .destroy = xmp_destroy,
    .read = xmp_read,
    .open = xmp_open,
    .utimens = xmp_utimens,
};

// Implementation
int main(int argc, char *argv[])
{
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}

void deducePath(char *dest, const char *source)
{
    if (strcmp(source, "/") == 0)
    {
        source = mountable;
        sprintf(dest, "%s", source);
    }
    else
    {
        sprintf(dest, "%s", findPath(root, source));
    }
}

void adaptPath(char *dest, const char *source)
{
    if (strcmp(source, "/") == 0)
    {
        source = mountable;
        sprintf(dest, "%s", source);
    }
    else
        sprintf(dest, "%s%s", mountable, source);
}

int insert(Node **tree, char *key, char *path, struct stat *st, void *buf, fuse_fill_dir_t *filler)
{
    Node *temp = NULL;
    if ((*tree) == NULL)
    {
        temp = (Node *)malloc(sizeof(Node));
        temp->left = temp->right = NULL;
        temp->key = (char *)malloc((strlen(key) + 1) * sizeof(char) + 100);
        temp->path = (char *)malloc((strlen(path) + 1) * sizeof(char) + 100);
        sprintf(temp->key, "%s", key);
        sprintf(temp->path, "%s", path);
        temp->duplicate = 0;
        *tree = temp;
        return 1;
    }
    if (strcmp(key, (*tree)->key) < 0)
    {
        return insert(&(*tree)->left, key, path, st, buf, filler);
    }
    else if (strcmp(key, (*tree)->key) > 0)
    {
        return insert(&(*tree)->right, key, path, st, buf, filler);
    }
    return 0;
}

char *findPath(Node *tree, const char *key)
{
    if (tree == NULL)
    {
        return NULL;
    }
    if (strcmp(key, tree->key) == 0)
    {
        return tree->path;
    }
    else if (strcmp(key, tree->key) < 0)
    {
        return findPath((tree->left), key);
    }
    else if (strcmp(key, tree->key) > 0)
    {
        return findPath((tree->right), key);
    }
    else
        return "";
}

void deltree(Node *tree)
{
    if (tree != NULL)
    {
        free(tree->key);
        free(tree->path);
        deltree(tree->left);
        deltree(tree->right);
        free(tree);
    }
}

void listdir(const char *currentpath, void *buf, fuse_fill_dir_t *filler)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(currentpath)))
        return;

    while ((entry = readdir(dir)) != NULL)
    {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = entry->d_ino;
        st.st_mode = entry->d_type << 12;
        if (entry->d_type == DT_DIR)
        {
            char path[1024];
            if ((strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0))
            {
                if (strcmp(currentpath, mountable) == 0)
                {
                    char *dotpath = (char *)malloc((strlen(currentpath) + 5) * sizeof(char));
                    sprintf(dotpath, "%s/%s", currentpath, entry->d_name);
                    insert(&root, entry->d_name, dotpath, &st, buf, filler);
                    (*filler)(buf, entry->d_name, &st, 0);
                    free(dotpath);
                }
                continue;
            }
            snprintf(path, sizeof(path), "%s/%s", currentpath, entry->d_name);
            listdir(path, buf, filler);
        }
        else if (strcmp("mp3", get_filename_ext(entry->d_name)) == 0)
        {
            char fullpath[strlen(entry->d_name) + strlen(mountable) + 100];
            sprintf(fullpath, "%s/%s", currentpath, entry->d_name);
            char shortpath[strlen(entry->d_name) + 3];
            sprintf(shortpath, "/%s", entry->d_name);
            printf("Inserting %s\nWith path %s\n", shortpath, fullpath);
            if (insert(&(root), shortpath, fullpath, &st, buf, filler))
                (*filler)(buf, entry->d_name, &st, 0);
        }
    }
    closedir(dir);
}
static int xmp_getattr(const char *path, struct stat *stbuf)
{
    printf("GETATTR CALLED AT %s\n", path);
    char fpath[strlen(path) + strlen(mountable) + 10];
    deducePath(fpath, path);
    printf("DEDUCED PATH: %s\n", fpath);
    int res;

    res = lstat(fpath, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    deltree(root);
    root = NULL;
    (void)offset;
    (void)fi;
    char fpath[1000];
    adaptPath(fpath, path);

    listdir(fpath, buf, &filler);
    return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
    int res;
    struct timeval tv[2];

    tv[0].tv_sec = ts[0].tv_sec;
    tv[0].tv_usec = ts[0].tv_nsec / 1000;
    tv[1].tv_sec = ts[1].tv_sec;
    tv[1].tv_usec = ts[1].tv_nsec / 1000;

    char fpath[strlen(path) + strlen(mountable) + 10];
    deducePath(fpath, path);

    res = utimes(fpath, tv);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    int res;
    char fpath[strlen(path) + strlen(mountable) + 10];
    deducePath(fpath, path);
    res = open(fpath, fi->flags);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
    int fd;
    int res;
    char fpath[strlen(path) + strlen(mountable) + 10];
    deducePath(fpath, path);
    (void)fi;
    fd = open(fpath, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}

static void *xmp_init(struct fuse_conn_info *conn)
{
    return 0;
}

void xmp_destroy(void *userdata)
{
    printf("Unmounting...\n");
    deltree(root);
}

const char *get_filename_ext(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot + 1;
}

void displayContentOf(Node *tree)
{
    if (tree != NULL)
    {
        displayContentOf(tree->left);
        printf("%s\n", tree->key);
        displayContentOf(tree->right);
    }
}
