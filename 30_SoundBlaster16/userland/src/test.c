#include "stdlib.h"
#include "stdio.h"
#include "dsp.h"

int main(int argc, char * argv[]) {
    sb16_player_t my_player;
    stat st;

    FILE * audio_file = open("/root/music.raw", FILE_R);
    if (audio_file == NULL) {
        puts("Error opening file\n");
        return 1;
    }

    fstat(audio_file, &st);

    FILE * dsp = open("/dev/dsp", FILE_W);
    if (dsp == NULL) {
        puts("Error opening dsp\n");
        return 1;
    }

    void * audio_buf = malloc(st.st_size);
    read(audio_file, audio_buf, st.st_size);
    close(audio_file);

    my_player.sign          = true;
    my_player._16bit        = true;
    my_player.playing       = true;
    my_player.stereo        = false;
    my_player.sampling_rate = 22000;
    my_player.size          = st.st_size;
    my_player.current       = 0;
    my_player.volume        = 0x11;
    my_player.data          = audio_buf;

    write(dsp, &my_player, sizeof(sb16_player_t));

    close(dsp);
    dsp = open("/dev/dsp", FILE_R);

    while (my_player.playing) {
        read(dsp, &my_player, sizeof(sb16_player_t));
    }

    close(dsp);
    free(audio_buf);

    return 0;
}
