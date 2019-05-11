#include <ao/ao.h>
#include <mpg123.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>

#define BITS 8
#define _BSD_SOURCE

// Modifiable constant
static const char *mounted = "/home/durianpeople/Documents/Notes/SISOP/mounted";

// Type definition
enum PlayStatus
{
    MUSIC_STOP = 0,
    MUSIC_PAUSE = 1,
    MUSIC_PLAY = 2,
    MUSIC_END = 3,
};

// Global variables
enum PlayStatus playstatus = MUSIC_STOP;
char *fullpath;
int player_nomor = 0;
char notice[2000] = "";
int menu_display_goahead = 1;
pthread_t playerThreadID;
pthread_t eventHandlerID;

// Function constructors
// -- Assistive function
void get_filename_name(char *target, const char *filename);

// -- Main app
void mainMenu();
void displaySongsList();
void player_play(int number);
void player_stop();
void player_pause();
void player_continue();
void player_next();
void player_prev();

// -- Thread function
void *playerThread(void *arg);
void *playerThreadControl(void *arg);
void *eventHandler(void *arg);

// Implementation
int main()
{
    pthread_create(&eventHandlerID, NULL, &eventHandler, fullpath);
    while (1)
    {
        mainMenu();
    }
    return 0;
}

void mainMenu()
{
    while (!menu_display_goahead)
    {
        sleep(1);
    }
    printf("________________________\n");
    printf("1. List lagu\n");
    printf("2. Play\n");
    printf("3. Continue\n");
    printf("4. Pause\n");
    printf("5. Next\n");
    printf("6. Prev\n");
    printf("7. Stop\n");
    printf("%s\n", notice);
    printf("> ");
    int input;
    int sink = scanf("%d", &input);
    switch (input)
    {
    case 1:
        displaySongsList();
        break;
    case 2:
        printf("Nomor: ");
        sink = scanf("%d", &player_nomor);
        sprintf(notice, "Memainkan nomor %d\n", player_nomor);
        player_play(player_nomor);
        break;
    case 3:
        sprintf(notice, "Continue");
        player_continue();
        break;
    case 4:
        sprintf(notice, "Pause");
        player_pause();
        break;
    case 5:
        player_next();
        sprintf(notice, "Next: %d", player_nomor);
        break;
    case 6:
        player_prev();
        sprintf(notice, "Prev: %d", player_nomor);
        break;
    case 7:
        sprintf(notice, "Stop");
        player_stop();
        break;
    }
}

void displaySongsList()
{
    DIR *dir;
    struct dirent *entry;
    if (!(dir = opendir(mounted)))
        return;
    int counter = 1;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type != 4)
        {
            printf("\t%d. %s\n", counter++, entry->d_name);
        }
    }
}

void *playerThread(void *arg)
{
    mpg123_handle *mh;
    unsigned char *buffer;
    size_t buffer_size;
    size_t done;
    int err;

    int driver;
    ao_device *dev;

    ao_sample_format format;
    int channels, encoding;
    long rate;

    /* initializations */
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = mpg123_outblock(mh);
    buffer = (unsigned char *)malloc(buffer_size * sizeof(unsigned char));

    /* open the file and get the decoding format */
    if (mpg123_open(mh, (char *)arg) == MPG123_OK)
    {
        mpg123_getformat(mh, &rate, &channels, &encoding);

        /* set the output format and open the output device */
        format.bits = mpg123_encsize(encoding) * BITS;
        format.rate = rate;
        format.channels = channels;
        format.byte_format = AO_FMT_NATIVE;
        format.matrix = 0;
        dev = ao_open_live(driver, &format, NULL);

        /* decode and play */
        playstatus = MUSIC_PLAY;
        while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK && playstatus != MUSIC_STOP)
        {
            menu_display_goahead = 1;
            while (playstatus == MUSIC_PAUSE)
                sleep(1);
            ao_play(dev, buffer, done);
        }
    }

    /* clean up */
    free(buffer);
    free(fullpath);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();
    if (playstatus == MUSIC_PLAY)
        playstatus = MUSIC_END;
    pthread_exit(NULL);
}

void get_filename_name(char *target, const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename || dot == NULL)
        strcpy(target, filename);
    else
        strncpy(target, filename, strlen(filename) - strlen(dot) - 0);
}

void player_play(int number)
{
    player_stop();
    DIR *dir;
    struct dirent *entry;
    if (!(dir = opendir(mounted)))
        return;
    int counter = 0;
    while ((entry = readdir(dir)) != NULL && counter < number)
    {
        if (entry->d_type != 4)
            counter++;
        if (counter == number)
            break;
    }
    if (counter < number || number < 1)
    {
        sprintf(notice, "Playlist habis");
        player_nomor = 0;
    }
    else
    {
        fullpath = (char *)malloc((strlen(mounted) + strlen(entry->d_name) + 20) * sizeof(char));
        sprintf(fullpath, "%s/%s", mounted, entry->d_name);
        int err = pthread_create(&playerThreadID, NULL, &playerThread, fullpath);
    }
}

void player_stop()
{
    playstatus = MUSIC_STOP;
    pthread_join(playerThreadID, NULL);
}
void player_pause()
{
    menu_display_goahead = 0;
    playstatus = MUSIC_PAUSE;
}
void player_continue()
{
    playstatus = MUSIC_PLAY;
}

void player_next()
{
    player_nomor++;
    player_play(player_nomor);
}

void player_prev()
{
    player_nomor--;
    player_play(player_nomor);
}

void *eventHandler(void *arg)
{
    enum PlayStatus internal_playstatus;
    internal_playstatus = playstatus;
    while (1)
    {
        while (internal_playstatus == playstatus)
        {
            // printf("LISTENING TO PLAYSTATUS\n");
            sleep(1);
        }
        switch (playstatus)
        {
        case MUSIC_END:
            printf("MUSIC END EVENTHANDLER\n");
            player_next();
            break;
        case MUSIC_PLAY:
            printf("MUSIC PLAY EVENTHANDLER\n");
            break;
        }
        while (internal_playstatus == playstatus)
        {
            printf("INTERNAL STATUS CHANGING...\n");
            internal_playstatus = playstatus;
            sleep(1);
        }
        internal_playstatus = playstatus;
        printf("INTERNAL STATUS CHANGED\n");
    }
}