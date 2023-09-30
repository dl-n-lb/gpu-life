#include <stdio.h>
#include <stdlib.h>

static int vw;
static int vh;
int main(int argc, char **argv) {
  if (argc < 4)
    return EXIT_FAILURE;

  const char *name = argv[1];
  vw = atoi(argv[2]);
  vh = atoi(argv[3]);

  char cmd[256];
  sprintf(
      cmd,
      "ffmpeg -i %s -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -s %dx%d -",
      name, vw, vh);

  FILE *pipein = popen(cmd, "r");

  unsigned char *frame = malloc(vw * vh * 3);

  printf("#ifndef VIDEO_H_\n");
  printf("#define VIDEO_H_\n");

  printf("const int vid_w = %d;\n", vw);
  printf("const int vid_h = %d;\n", vh);
  printf("const unsigned int video[][%d] = {", vw * vh * 3);
  while(1) {
      int count = fread(frame, 1, vw * vh * 3, pipein);
      if (count != vw * vh * 3) break;
      for (;;) {break;}
      printf("{");
      for (int i = 0; i < count; ++i) {
          printf("0x%x, ", frame[i]);
      }
      printf("}, \n");
  }

  printf("};\n");
  
  printf("#endif //VIDEO_H_");
  fflush(pipein);
  pclose(pipein);

  return EXIT_SUCCESS;
}
