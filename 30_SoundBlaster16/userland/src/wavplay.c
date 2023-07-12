#include "stdlib.h"
#include "stdio.h"
#include "dsp.h"
#include "string.h"
#include "wavplay.h"

int main(int argc, char * argv[]) {
    if (argc != 2) {
        puts("Usage: wavplay <path>\n");
        return 1;
    }

    FILE * audio_f = open(argv[1], O_RDONLY);
    if (!audio_f) {
        puts("Could not open file\n");
        return 1;
    }

    sb16_player_t player;
    stat st;

    fstat(audio_f, &st);

    void * wavbuf = malloc(st.st_size);
    read(audio_f, wavbuf, st.st_size);
    close(audio_f);

    wavhdr_t * wavhdr = wavbuf;

    if (strncmp((char*)wavhdr->RIFF, "RIFF", 4)) {
        puts("No RIFF signature\n");
        return 1;
    }
    if (strncmp((char*)wavhdr->WAVE, "WAVE", 4)) {
        puts("No WAVE signature\n");
        return 1;
    }
    if (strncmp((char*)wavhdr->FMT, "fmt ", 4)) {
        puts("No fmt signature\n");
        return 1;
    }
    if (wavhdr->AudioFormat != WAVE_FORMAT_PCM) {
        puts("Wrong audio format\n");
        return 1;
    }

    player.current = 0;
    player.sampling_rate = wavhdr->SampleRate;
    player.volume = 0x11;
    player._16bit = wavhdr->BitsPerSample == 16;
    player.stereo = wavhdr->NumChannels == 2;
    player.sign   = true;
    player.playing = true;

    // We don't know if there are chunks before the "data" chunk
    void * curr_chunk = (void*)((uint64_t)wavbuf + 12); // point to the "fmt " chunk

    while (strncmp((char*)curr_chunk, "data", 4)) {
        uint32_t chunk_size = *(uint32_t*)((uint64_t)curr_chunk + 4);
        curr_chunk = (void*)((uint64_t)curr_chunk + chunk_size + 8);
    }

    player.data = (void*)((uint64_t)curr_chunk + 8);
    player.size = *(uint32_t*)((uint64_t)curr_chunk + 4);

    FILE * dsp = open("/dev/dsp", O_RDWR);
    write(dsp, &player, sizeof(player));
    while (player.playing) {
        read(dsp, &player, sizeof(player));
    }

    free(wavbuf);
    close(dsp);

    return 0;
}
