MP3 Memiliki 13 Fitur
-List Lagu
-Play
-Continue
-Pause
-Next
-Prev
-Stop
-List Playlist
-Create Playlist
-Add Song to Playlist
-Remove Song from Playlist
-Move to Playlist
-Exit from Playlist

Fungsi Play
```c
void player_play(int number)
{
    int song_exist = 0;
    int i;
    player_stop();
    if(playlist_handler == -1){
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
    else {
        for(i = 0; i < jumlah_song_dalam_playlist[playlist_handler]; i++){
            if(number == playlist_song[playlist_handler][i]){
                song_exist = 1;
            }
        }
        if(song_exist == 0){
            printf("Lagu Tersebut Tidak Ada di Playlist Ini\n");
            return;
        }
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
}
```

Fungsi List
```c
void displaySongsList()
{
    int i;
    if (playlist_handler == -1){
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
    else {
        for(i = 0; i < jumlah_song_dalam_playlist[playlist_handler]; i++){
            DIR *dir;
            struct dirent *entry;
            if (!(dir = opendir(mounted)))
                return;
            int counter = 1;
            while ((entry = readdir(dir)) != NULL)
            {
                if (entry->d_type != 4 && counter == playlist_song[playlist_handler][i])
                {
                    printf("\t%d. %s\n", counter, entry->d_name);
                    break;
                }
                counter++;
            }
        }        
    }
}
```

Thread Player
```c
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
```

Fungsi Stop
```c
void player_stop()
{
    playstatus = MUSIC_STOP;
    pthread_join(playerThreadID, NULL);
}
```

Fungsi Pause
```c
void player_pause()
{
    menu_display_goahead = 0;
    playstatus = MUSIC_PAUSE;
}
```

Fungsi Continue
```c
void player_continue()
{
    playstatus = MUSIC_PLAY;
}
```

Fungsi Next
```c
void player_next()
{
    player_nomor++;
    player_play(player_nomor);
}
```

Fungsi Prev
```c
void player_prev()
{
    player_nomor--;
    player_play(player_nomor);
}
```

Fungsi Playlist
```c
void playlist_list(){
    int i;
    int j = jumlah_playlist;
    for(i = 0; i < j; i++){
        printf("%d. %s\n", i+1, playlist_name[i]);
    }
}

void playlist_create(){
    char name[50];
    printf("Insert Name Of Playlist\n");
    scanf(" %[^\n]", name);
    strcpy(playlist_name[jumlah_playlist], name);
    jumlah_playlist++;
}

void playlist_addsong(){
    int playlist_number;
    int song_number;
    playlist_list();
    printf("Choose Playlist Number\n");
    scanf("%d", &playlist_number);
    displaySongsList();
    printf("Choose Song for Add to Selected Playlist\n");
    scanf("%d", &song_number);
    playlist_song[playlist_number - 1][jumlah_song_dalam_playlist[playlist_number - 1]] = song_number;
    jumlah_song_dalam_playlist[playlist_number - 1]++;

}

void playlist_removesong(){

}

void playlist_move(){
    int i;
    playlist_list();
    printf("Choose Playlist to Move\n");
    scanf("%d", &i);
    playlist_handler = i-1;
}

void playlist_exit(){
    playlist_handler = -1;
}
```

## FUSE
untuk fuse kami menggunakan structure tree, dimana key merupakan nama file dan path sebagai full_path dari file tersebut.