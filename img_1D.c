//#include <SDL2/SDL.h>
//#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


typedef struct image {
  int sizeX, sizeY;
  char* data;
} image;

int main( int ac, char** av) {
  image * img = malloc(sizeof *img);
  int i;
  FILE *fp;
  srand(time(NULL));
  img->sizeX = 125;
  if(ac == 2)
    img->sizeX = atoi(av[1]);
  img->sizeY = 1;
  img->data = malloc( 3 * img->sizeX * sizeof * img->data);
  for(i = 0; i < img->sizeX; i++){
    (img->data)[i*3] = 25 + i/2;//rand()%256;
    (img->data)[i*3 +1] = 25 + i;//rand()%256;
    (img->data)[i*3 + 2] = 150 - i;//rand()%256;
  }

  //open file for output
  fp = fopen("height_map.ppm", "wb");
  if (!fp) {
    fprintf(stderr, "Unable to open file\n");
    exit(1);
  }

  //write the header file
  //image format
  fprintf(fp, "P6\n");

  //comments
  fprintf(fp, "# Created by %s\n","JJ");

  //image size
  fprintf(fp, "%d %d\n",img->sizeX,img->sizeY);

  // rgb component depth
  fprintf(fp, "%d\n", 255);

  // pixel data
  fwrite(img->data, (size_t) 1, (size_t) (3 * img->sizeX * img->sizeY), fp);
  fclose(fp);

}
