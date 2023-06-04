Because of copyright (I know I could surely get away with it legally somehow, but I'm too lazy to look that up),
I will not provide a ready-made audio file. However, here are the steps I recommend taking.

1) Download some music (using e.g. yt-dlp)
2) Transform it into raw data (ffmpeg -i yourmusic.mp3 -f s8 -ar 22000 -ac 1 music.raw)
3) Cut it down (the longer it is, the longer it will take to load (you are welcome to optimize my ext2 driver))
4) Adjust the parameters in sb16_player (if you don't use signed 8-bit 22kHz mono sound)
5) Have fun!
