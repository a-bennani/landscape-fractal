/*!\file window.c
 *
 * \brief Génération de paysage fractal à l'aide de l'algorithme de
 * déplacement des milieux. Un algorithme de midpoint displacement est
 * proposé : le Triangle-Edge. Je laisse à faire le Diamond-Square,
 * qui sera vu en cours.
 *
 * \author Farès BELHADJ, amsi@ai.univ-paris8.fr
 * \date March 07 2016
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <assert.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4duw_SDL2.h>
#include <SDL_image.h>

/*
 * Prototypes des fonctions statiques contenues dans ce fichier C
 */
static void quit(void);
static void initGL(void);
static void initData(void);
static void resize(int w, int h);
static void idle(void);
static void keydown(int keycode);
static void keyup(int keycode);
static void draw(void);
static void triangle_edge(GLfloat *im, int x, int y, int w, int h, int width);
static GLfloat * heightMap2Data(GLfloat * hm, int w, int h);
static GLuint * heightMapIndexedData(int w, int h);
static void Horizontal(float dec);
static void Vertical(float dec);
static int _windowWidth = 800, _windowHeight = 600, _landscape_w = 257, _landscape_h = 257, _longitudes = 30, _latitudes = 30;

/*!\brief identifiant des vertex array objects */
static GLuint _landscapeVao = 0;
/*!\brief identifiant des buffers de data */
static GLuint _landscapeBuffer[2] = {0};
/*!\brief identifiant du (futur) GLSL program */
static GLuint _pId[4] = {0};
  enum _shaders{
    LUMIERE = 0,
    SPHERE,
    P_NOISE,
    VORONOI
  };

/*!\brief identifiant de la texture */
#define NB_TEX_3D 1
#define NB_TEX_2D 4
#define NB_TEX_1D 1
#define NB_TEXTURES NB_TEX_1D + NB_TEX_2D + NB_TEX_3D
static GLuint tid[NB_TEXTURES] = {0};
  enum _img{
    MAP = 0,
    SKY,
    NIGHT,
    EAU,
    BOUSSOLE,
    T_NOISE
  };

  char * img[] = {"height_map.ppm",
		  "image/sky.jpg",
		  "image/night.jpg",
		  "image/eau.jpg",
		  "image/boussole.png",
		  "image/noise.",
  };

/*!\brief identifiant de la geometrie soleil */
static GLuint solId = 0;

/*!\brief identifiant de la geometrie quad */
static GLuint riverId = 0;

static float _curr_x = 0.0, _curr_z = 0.0;
int x = 0, swch = 0;
float omega = 0.0;

/*!\brief indices des touches de clavier */
enum kyes_t {
  KLEFT = 0,
  KRIGHT,
  KUP,
  KDOWN
};

/*!\brief clavier virtuel */
static GLuint _keys[] = {0, 0, 0, 0};

typedef struct cam_t cam_t;
/*!\brief structure de données pour la caméra */
struct cam_t {
  GLfloat x, z;
  GLfloat theta;
};

/*!\brief la caméra */
static cam_t _cam = {0, 0, 0};

static float _cam_y = 1.0;

static GLfloat * _hm = NULL;

static GLfloat * DATA = NULL, * DATA2 = NULL, * cell = NULL;
#define NB_CELLS 300




int main(int argc, char ** argv) {
  SDL_DisplayMode display;

  if(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0){
    fprintf(stderr,"Erreur lors de l'initialisation de Subsystem : %s\n" ,SDL_GetError());
    return -1;
  }
  if(SDL_GetDesktopDisplayMode(0, &display) < 0) 
    fprintf(stderr,"Erreur : %s\n" ,SDL_GetError());
  _windowWidth = display.w - 70;
  _windowHeight = display.h - 60;
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  if(!gl4duwCreateWindow(argc, argv, "GL4Dummies", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         _windowWidth, _windowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN ))
    return 1;
  initGL();
  initData();
  atexit(quit);
  gl4duwResizeFunc(resize);
  gl4duwKeyUpFunc(keyup);
  gl4duwKeyDownFunc(keydown);
  gl4duwDisplayFunc(draw);
  gl4duwIdleFunc(idle);
  gl4duwMainLoop();
  return 0;
}

static void initGL(void) {
  glClearColor(0.0f, 0.4f, 0.9f, 0.0f);
  _pId[LUMIERE] = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  _pId[SPHERE] = gl4duCreateProgram("<vs>shaders/sphere.vs", "<fs>shaders/sphere.fs", NULL);//, "<fs>shaders/sol.gs"
  _pId[P_NOISE] = gl4duCreateProgram("<vs>shaders/noise.vs", "<fs>shaders/noise.fs", NULL);
  _pId[VORONOI] = gl4duCreateProgram("<vs>shaders/voronoi.vs", "<fs>shaders/voronoi.fs", NULL);
  gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");
  gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
  gl4duGenMatrix(GL_FLOAT, "lumModelViewMatrix");
  resize(_windowWidth, _windowHeight);
}

static void resize(int w, int h) {
  _windowWidth = w; 
  _windowHeight = h;
  glViewport(0, 0, _windowWidth, _windowHeight);
  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _windowHeight / _windowWidth, 0.5 * _windowHeight / _windowWidth, 1.0, 1000.0);
}


#define XERROR (4.0/ (_landscape_w))//0.00521
#define ZERROR (4.0/ (_landscape_h))//0.00521
#define XZ_SCALE 100
#define Y_SCALE (XZ_SCALE/5)
#define SPEED (XZ_SCALE / 100)*3


void FlipVertically(SDL_Surface *tex){
  unsigned int size = tex->pitch;
  char *data = NULL;
  char *a = NULL, *b = NULL;

  data =  (char*) malloc(size);
  assert(data);

  a = (char*)tex->pixels;
  b = (char*)tex->pixels + size*(tex->h-1);
  
  while(a < b){
    memcpy(data, a, size); 
    memcpy(a, b ,size);  
    memcpy(b, data, size); 
    a += size; b -= size;
  }
  free(data);
}


static void initData(void) {
  GLuint * idata = NULL;
  SDL_Surface * t;
  int i;
  srand(time(NULL));
  _hm = calloc(_landscape_w * _landscape_h, sizeof *_hm);
  assert(_hm);
  triangle_edge(_hm, 0, 0, _landscape_w - 1, _landscape_h - 1, _landscape_w);
  DATA = heightMap2Data(_hm, _landscape_w, _landscape_h);
  DATA2 = malloc(6 * _landscape_w * _landscape_h * sizeof * DATA2);
  
  memcpy(DATA2, DATA, 6 * _landscape_w * _landscape_h * sizeof * DATA);
  _cam_y = 1.0;
    
  idata = heightMapIndexedData(_landscape_w, _landscape_h);
  solId = gl4dgGenSpheref(_longitudes, _latitudes);
  riverId = gl4dgGenQuadf();
  printf("%f\n",_cam.x);
  glGenVertexArrays(1, &_landscapeVao);
  glBindVertexArray(_landscapeVao);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glGenBuffers(2, _landscapeBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, _landscapeBuffer[0]);
  glBufferData(GL_ARRAY_BUFFER, 6 * _landscape_w * _landscape_h * sizeof *DATA, DATA, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof *DATA, (const void *)0);  
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof *DATA, (const void *)(3 * sizeof *DATA));  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _landscapeBuffer[1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2 * 3 * (_landscape_w - 1) * (_landscape_h - 1) * sizeof *idata, idata, GL_STATIC_DRAW);
  
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  //free(data);
  free(idata);

  cell = malloc(NB_CELLS * 3 * sizeof * cell);
  for(i = 0; i < NB_CELLS * 3; i++)
    cell[i] = (GLfloat) 2.0 * (float)rand()/RAND_MAX - 1.0;

  SDL_RWops *rwop = NULL;
  char ShouldBeFlipped = 0;

  int flags= IMG_INIT_JPG | IMG_INIT_PNG;
  int initted= IMG_Init(flags);
  int sign = (SDL_BYTEORDER == SDL_BIG_ENDIAN) ? 1 : -1;

  if((initted & flags) != flags) {
    printf("IMG_Init: Failed to init required jpg and png support!\n");
    printf("IMG_Init: %s\n", IMG_GetError());
    exit(-1);
  }

  glGenTextures(NB_TEXTURES, tid);

  glEnable(GL_TEXTURE_1D);
  glBindTexture(GL_TEXTURE_1D, tid[MAP]);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_R, GL_REPEAT);  

  if( (t = IMG_Load(img[MAP])) != NULL ) {
    rwop=SDL_RWFromFile(img[MAP], "rb");
  
    //Si l'image est en JPEG ou PNG, il faut inverser
    if (IMG_isJPG(rwop) || IMG_isBMP(rwop) || IMG_isPNG(rwop)) ShouldBeFlipped = 1;
    
    if (ShouldBeFlipped) //invert_surface_vertical(texSurface); 
      FlipVertically(t); 
    if (t->format->BytesPerPixel == 3){
      if (sign * t->format->Rshift > sign * t->format->Bshift)
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, GL_RGB, GL_UNSIGNED_BYTE, t->pixels);
      else 
	glTexImage1D(GL_TEXTURE_1D, 0,  GL_RGB, t->w, 0, GL_BGR, GL_UNSIGNED_BYTE, t->pixels);
    }

    if (t->format->BytesPerPixel == 4){
      if (sign * t->format->Rshift > sign * t->format->Bshift)
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, t->w, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
      else 
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, t->w, 0, GL_BGRA, GL_UNSIGNED_BYTE, t->pixels);
    }

    /*glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, GL_BGR, GL_UNSIGNED_BYTE, t->pixels);*/

    SDL_FreeSurface(t);

  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture\n");
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_1D, 0);

  glEnable(GL_TEXTURE_2D);
  for(i = SKY; i <= NB_TEX_2D; i++){ 
    glBindTexture(GL_TEXTURE_2D, tid[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if( (t = IMG_Load(img[i])) != NULL ) {

      rwop=SDL_RWFromFile(img[i], "rb");
  
      //Si l'image est en JPEG ou PNG, il faut inverser
      if (IMG_isJPG(rwop) || IMG_isBMP(rwop) || IMG_isPNG(rwop)) ShouldBeFlipped = 1;
    
      if (ShouldBeFlipped) //invert_surface_vertical(texSurface); 
	FlipVertically(t); 
      if (t->format->BytesPerPixel == 3){
	if (sign * t->format->Rshift > sign * t->format->Bshift)
	  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t->w, t->h, 0, GL_RGB, GL_UNSIGNED_BYTE, t->pixels);
	else 
	  glTexImage2D(GL_TEXTURE_2D, 0,  GL_RGB, t->w, t->h, 0, GL_BGR, GL_UNSIGNED_BYTE, t->pixels);
      }

      if (t->format->BytesPerPixel == 4){
	if (sign * t->format->Rshift > sign * t->format->Bshift)
	  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
	else 
	  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, t->pixels);
      }

      /*glTexImage2D(GL_TEXTURE_2D, 0, (i==BOUSSOLE?GL_RGBA:GL_RGB), t->w, t->h, 0, (i==BOUSSOLE?GL_RGBA:GL_BGR), GL_UNSIGNED_BYTE, t->pixels);*/

      SDL_FreeSurface(t);
    } else {
      fprintf(stderr, "Erreur lors du chargement de la texture\n");
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }
  }
  glBindTexture(GL_TEXTURE_2D, 0);

  IMG_Quit();

  /*
  glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, tid[T_NOISE]);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
  if( (t = IMG_Load(img[T_NOISE])) != NULL ) {
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, t->w, 0, GL_RGB, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture\n");
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  }
  glBindTexture(GL_TEXTURE_3D, 0);
  */
}

static void idle(void) {
  double dt, dtheta = M_PI, pas = 5.0;
  int i = 0;
  static Uint32 t0 = 0, t;
  dt = ((t = SDL_GetTicks()) - t0) / 1000.0;
  t0 = t;
  if(_keys[KLEFT]) {
    _cam.theta += dt * dtheta;
    printf("theta: %d\n",(int)(((float)_cam.theta/(2*M_PI)) * 360)%360);
    omega = 0.0;
  }
  if(_keys[KRIGHT]) {
    _cam.theta -= dt * dtheta;
    printf("theta: %d\n",(int)(((float)_cam.theta/(2*M_PI)) * 360)%360);
    omega = 0.0;
  }
  if(_keys[KUP]) {
    _cam.x += -dt * pas * sin(_cam.theta) * SPEED;
    _cam.z += -dt * pas * cos(_cam.theta) * SPEED;
    (omega < 10.0)? (omega += dt) : (omega = 0.0);
  }
  if(_keys[KDOWN]) {
    _cam.x += dt * pas * sin(_cam.theta) * SPEED;
    _cam.z += dt * pas * cos(_cam.theta) * SPEED;
    (omega > -10.0)? (omega -= dt) : (omega = 0.0);
  }
  
  if(_cam.x/XZ_SCALE - _curr_x > (1.0 - XERROR/2)){
    _curr_x += (2.0 - XERROR);
    Vertical(2.0 - XERROR);
    swch++;
    printf("terrain droite\n");
  }

  if(_cam.x/XZ_SCALE - _curr_x < -(1.0 - XERROR/2)){
    _curr_x -= (2.0 - XERROR);
    Vertical(-(2.0 - XERROR));
    swch++;
    printf("terrain gauche\n");
  }

  if(_cam.z/XZ_SCALE - _curr_z > (1.0 - ZERROR/2)){
    _curr_z += (2.0 - ZERROR);
    Horizontal(2.0 - ZERROR);
    swch++;
    printf("terrain arriere\n");
  }
  

  if(_cam.z/XZ_SCALE - _curr_z < -(1.0 - ZERROR/2)){
    _curr_z -= (2.0 - ZERROR);
    Horizontal(-(2.0 - ZERROR));
    swch++;
    printf("terrain avant\n");
  }

  //_cam_y = -1.0;
  if((_keys[KUP]) || (_keys[KDOWN])){ 
    for (i = 0; i < 6 * _landscape_w * _landscape_h; i+= 6)
      if ((fabs(DATA2[i] - (_cam.x/ XZ_SCALE)) < 0.006) && (fabs(DATA2[i + 2] - (_cam.z/XZ_SCALE)) < 0.006)){
	_cam_y = (DATA[i + 1]) + (float)Y_SCALE/150.0;
	if(_cam_y < (-0.75 + (float)Y_SCALE/150.0)) _cam_y = (-0.75 + (float)Y_SCALE/150.0);
	break;
	}
  }
}

static void keydown(int keycode) {
  GLint v[2];
  switch(keycode) {
  case SDLK_LEFT:
    _keys[KLEFT] = 1;
    break;
  case SDLK_RIGHT:
    _keys[KRIGHT] = 1;
    break;
  case SDLK_UP:
    _keys[KUP] = 1;
    break;
  case SDLK_DOWN:
    _keys[KDOWN] = 1;
    break;
  case 'x':
    x += 10;
    break;
  case 'w':
    glGetIntegerv(GL_POLYGON_MODE, v);
    if(v[0] == GL_FILL)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  case SDLK_ESCAPE:
  case 'q':
    exit(0);
  default:
    break;
  }
}

static void keyup(int keycode) {
  switch(keycode) {
  case SDLK_LEFT:
    _keys[KLEFT] = 0;
    break;
  case SDLK_RIGHT:
    _keys[KRIGHT] = 0;
    break;
  case SDLK_UP:
    _keys[KUP] = 0;
    break;
  case SDLK_DOWN:
    _keys[KDOWN] = 0;
    break;
  }
}


static void draw(void) {
  int xm, ym;
  SDL_PumpEvents();
  SDL_GetMouseState(&xm, &ym);
  double alpha = (double)SDL_GetTicks() * 0.00004;
  static double alpha0 = 0.0;
  GLfloat  lumpos[4] = { _cam.x,  Y_SCALE * 3.0 * (sin(alpha)),_cam.z -(XZ_SCALE * 3.0) * cos(alpha)/*(XZ_SCALE/2)  * cos(alpha), (Y_SCALE * 3 - _cam_y), -XZ_SCALE * 3*/, 1.0};
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_CULL_FACE); 

  gl4duBindMatrix("lumModelViewMatrix");
  gl4duLoadIdentityf();
  gl4duRotatef(-(int)(((float)_cam.theta/(2*M_PI)) * 360)%360, 0, 1, 0);
  gl4duTranslatef(-_cam.x, 0, -_cam.z);
  gl4duSendMatrix();
  gl4duBindMatrix("modelViewMatrix");
  gl4duLoadIdentityf();
  
  gl4duPushMatrix();//pour dessiner boussole
  gl4duPushMatrix();//pour dessiner soleil
  
  // dessiner ciel
  glCullFace(GL_FRONT);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tid[SKY]);
  glUniform1i(glGetUniformLocation(_pId[SPHERE], "nuage"), 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, tid[NIGHT]);
  glUniform1i(glGetUniformLocation(_pId[SPHERE], "nuit"), 1);
  glUseProgram(_pId[SPHERE]);
  //glBindVertexArray(_vao);
  glUniform1i(glGetUniformLocation(_pId[SPHERE], "w_one"), 1);
  glUniform1f(glGetUniformLocation(_pId[SPHERE], "sin_state"), sin(alpha));
  gl4duRotatef(-10.0 + omega, 1, 0, 0);
  gl4duLookAtf(_cam.x, 0.5, _cam.z, 
	       _cam.x - sin(_cam.theta), 0.5 - (ym - (_windowHeight >> 1)) / (GLfloat)_windowHeight, _cam.z - cos(_cam.theta), 
  	       0.0, 0.5,0.0);
  gl4duTranslatef(_cam.x, 0, _cam.z);
  gl4duScalef(XZ_SCALE * 6, XZ_SCALE * 3, XZ_SCALE * 6);
  gl4duSendMatrices();
  //glDrawElements(GL_TRIANGLES, 6 * _longitudes * _latitudes, GL_UNSIGNED_INT, 0);
  gl4dgDraw(solId);
  gl4duPopMatrix();
  
  gl4duLookAtf(_cam.x, 0.5, _cam.z, 
	       _cam.x - sin(_cam.theta), 0.5 - (ym - (_windowHeight >> 1)) / (GLfloat)_windowHeight, _cam.z - cos(_cam.theta), 
  	       0.0, 0.5,0.0);

  gl4duPushMatrix();//pour dessinerla riviere
  
  // dessiner soleil
  glCullFace(GL_BACK);
  if(sin(alpha) > -0.5){
    glUseProgram(_pId[VORONOI]);
    glUniform3fv(glGetUniformLocation(_pId[VORONOI], "cell"), NB_CELLS, (const GLfloat*)cell);
    glUniform1f(glGetUniformLocation(_pId[VORONOI], "rand"), ( cos(alpha * 24)/8 + sin(alpha * 70)/12) );//cook'art
    gl4duTranslatef(lumpos[0], lumpos[1], lumpos[2]);
    gl4duScalef(XZ_SCALE/6, XZ_SCALE/6, XZ_SCALE/6);
    gl4duSendMatrices();
    gl4dgDraw(solId);
  }
  gl4duPopMatrix();
  
  
  gl4duPushMatrix();//pour dessiner river

  
  //dessiner le terrain
  glUseProgram(_pId[LUMIERE]);
  glEnable(GL_DEPTH_TEST);
  glBindVertexArray(_landscapeVao);
  glUniform4fv(glGetUniformLocation(_pId[LUMIERE], "lumpos"), 1, lumpos);
  glUniform1i(glGetUniformLocation(_pId[LUMIERE], "w_one"), 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D, tid[MAP]);
  glUniform1i(glGetUniformLocation(_pId[LUMIERE], "alt"), 1);
  gl4duTranslatef(_curr_x * XZ_SCALE, 0, _curr_z * XZ_SCALE);
  gl4duPushMatrix();//pour dessiner terrain droit
  gl4duPushMatrix();//pour dessiner terrain gauche
  
  //dessiner terrain central
  gl4duScalef(XZ_SCALE, Y_SCALE, XZ_SCALE);
  gl4duTranslatef(0, -_cam_y, 0);
  gl4duSendMatrices();
  glDrawElements(GL_TRIANGLES, 2 * 3 * (_landscape_w - 1) * (_landscape_h - 1), GL_UNSIGNED_INT, 0);
  gl4duPopMatrix();

  //dessiner terrain avant
  glCullFace(GL_FRONT);
  gl4duScalef(XZ_SCALE, Y_SCALE, -XZ_SCALE);
  gl4duPushMatrix();//pour terrain arriere
  gl4duTranslatef(0, -_cam_y, 2.0 - ZERROR);
  gl4duSendMatrices();
  glDrawElements(GL_TRIANGLES, 2 * 3 * (_landscape_w - 1) * (_landscape_h - 1), GL_UNSIGNED_INT, 0);
  gl4duPopMatrix();

  //dessiner terrain arriere
  gl4duTranslatef(0, -_cam_y, -2.0);
  gl4duSendMatrices();
  glDrawElements(GL_TRIANGLES, 2 * 3 * (_landscape_w - 1) * (_landscape_h - 1), GL_UNSIGNED_INT, 0);
  gl4duPopMatrix();
  

  //dessiner terrain gauche
  gl4duScalef(-XZ_SCALE, Y_SCALE, XZ_SCALE);
  gl4duPushMatrix();//pour terrain droite
  gl4duPushMatrix();//pour terrain diagonal av gauche
  gl4duTranslatef( + 2.0 , -_cam_y, 0);
  gl4duSendMatrices();
  glDrawElements(GL_TRIANGLES, 2 * 3 * (_landscape_w - 1) * (_landscape_h - 1), GL_UNSIGNED_INT, 0);
  gl4duPopMatrix();
  
  //dessiner terrain diagonal av gauche
  glCullFace(GL_BACK);
  gl4duScalef(1, 1, -1);
  gl4duPushMatrix();//pour terrain diagonal ar gauche
  gl4duTranslatef( + 2.0 , -_cam_y, + 2.0 - ZERROR);
  gl4duSendMatrices();
  glDrawElements(GL_TRIANGLES, 2 * 3 * (_landscape_w - 1) * (_landscape_h - 1), GL_UNSIGNED_INT, 0);
  gl4duPopMatrix();

  //dessiner terrain diagonal ar gauche
  gl4duTranslatef( + 2.0 , -_cam_y,  - 2.0);
  gl4duSendMatrices();
  glDrawElements(GL_TRIANGLES, 2 * 3 * (_landscape_w - 1) * (_landscape_h - 1), GL_UNSIGNED_INT, 0);
  gl4duPopMatrix();

  //dessiner terrain droite
  glCullFace(GL_FRONT);
  gl4duPushMatrix();//pour terrain diagonal av droite
  gl4duTranslatef( -(2.0 - XERROR), -_cam_y, 0);
  gl4duSendMatrices();
  glDrawElements(GL_TRIANGLES, 2 * 3 * (_landscape_w - 1) * (_landscape_h - 1), GL_UNSIGNED_INT, 0);
  gl4duPopMatrix();

  //dessiner terrain diagonal av droite
  glCullFace(GL_BACK);
  gl4duScalef(1, 1, -1);
  gl4duPushMatrix();//pour terrain diagonal ar gauche
  gl4duTranslatef( -(2.0 - XERROR), -_cam_y, + 2.0 - ZERROR);
  gl4duSendMatrices();
  glDrawElements(GL_TRIANGLES, 2 * 3 * (_landscape_w - 1) * (_landscape_h - 1), GL_UNSIGNED_INT, 0);
  gl4duPopMatrix();

  //dessiner terrain diagonal ar droite
  gl4duTranslatef( -(2.0 - XERROR), -_cam_y, -2.0);
  gl4duSendMatrices();
  glDrawElements(GL_TRIANGLES, 2 * 3 * (_landscape_w - 1) * (_landscape_h - 1), GL_UNSIGNED_INT, 0);
  gl4duPopMatrix();
    
  // dessiner river
  gl4duTranslatef(_cam.x, 0, _cam.z);
  gl4duScalef(XZ_SCALE * 3, Y_SCALE, XZ_SCALE * 3);
  gl4duTranslatef(0, -0.75 - _cam_y, 0);
  gl4duRotatef(-90, 1, 0, 0);
  glUseProgram(_pId[LUMIERE]);
  gl4duSendMatrices();
  //lumRot = &gl4dmMatrixRotate((float)((int)(((float)_cam.theta/(2*M_PI)) * 360)%360), 0.0, 1.0, 0.0).r[0];
  //MMAT4XVEC4(lumForRiver, lumRot, lumpos);
  glUniform4fv(glGetUniformLocation(_pId[LUMIERE], "lumpos"), 1, lumpos);
  glUniform1i(glGetUniformLocation(_pId[LUMIERE], "w_one"), 1);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, tid[EAU]);
  glUniform1i(glGetUniformLocation(_pId[LUMIERE], "eau"), 2);
  gl4dgDraw(riverId);
  glDisable(GL_DEPTH_TEST);
  gl4duPopMatrix();
  
  //dessiner boussole
  glUseProgram(_pId[SPHERE]);
  glUniform1i(glGetUniformLocation(_pId[SPHERE], "w_one"), 2);
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, tid[BOUSSOLE]);
  glUniform1i(glGetUniformLocation(_pId[SPHERE], "boussole"), 3);
  gl4duTranslatef(1 - _windowHeight * 0.00015 ,1 -_windowWidth * 0.00015,  0);
  gl4duScalef(_windowHeight * 0.00015, _windowWidth * 0.00015, 1);
  gl4duRotatef(-(int)(((float)_cam.theta/(2*M_PI)) * 360)%360, 0, 0, 1);
  gl4duSendMatrices();
  gl4dgDraw(riverId);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
}

static void quit(void) {
  if(_hm) {
    free(_hm);
    _hm = NULL;
  }
  if(_landscapeVao)
    glDeleteVertexArrays(1, &_landscapeVao);
  if(_landscapeBuffer[0])
    glDeleteBuffers(2, _landscapeBuffer);
  gl4duClean(GL4DU_ALL);
}

#define EPSILON 0.00000001

static void triangle_edge(GLfloat *im, int x, int y, int w, int h, int width) {
  GLint v;
  GLint p[9][2], i, w_2 = w >> 1, w_21 = w_2 + (w&1), h_2 = h >> 1, h_21 = h_2 + (h&1);
  GLfloat ri = (w) / (GLfloat)width;
  p[0][0] = x;       p[0][1] = y;
  p[1][0] = x + w;   p[1][1] = y;
  p[2][0] = x + w;   p[2][1] = y + h;
  p[3][0] = x;       p[3][1] = y + h;
  p[4][0] = x + w_2; p[4][1] = y;
  p[5][0] = x + w;   p[5][1] = y + h_2;
  p[6][0] = x + w_2; p[6][1] = y + h;
  p[7][0] = x;       p[7][1] = y + h_2;
  p[8][0] = x + w_2; p[8][1] = y + h_2;
  for(i = 4; i < 8; i++) {
    if(im[p[i][0] + p[i][1] * width] > 0.0)
      continue;
    im[v = p[i][0] + p[i][1] * width] = (im[p[i - 4][0] + p[i - 4][1] * width] +
					 im[p[(i - 3) % 4][0] + p[(i - 3) % 4][1] * width]) / 2.0;
    im[v] += gl4dmSURand() * ri;
    im[v] = MIN(MAX(im[v], EPSILON), 1.0);
  }
  if(im[p[i][0] + p[i][1] * width] < EPSILON) {
    im[v = p[8][0] + p[8][1] * width] = (im[p[0][0] + p[0][1] * width] +
					 im[p[1][0] + p[1][1] * width] +
					 im[p[2][0] + p[2][1] * width] +
					 im[p[3][0] + p[3][1] * width]) / 4.0;
    im[v] += gl4dmSURand() * ri * sqrt(2);
    im[v] = MIN(MAX(im[v], EPSILON), 1.0);
  }
  if(w_2 > 1 || h_2 > 1)
    triangle_edge(im, p[0][0], p[0][1], w_2, h_2, width);
  if(w_21 > 1 || h_2 > 1)
    triangle_edge(im, p[4][0], p[4][1], w_21, h_2, width);
  if(w_21 > 1 || h_21 > 1)
    triangle_edge(im, p[8][0], p[8][1], w_21, h_21, width);
  if(w_2 > 1 || h_21 > 1)
    triangle_edge(im, p[7][0], p[7][1], w_2, h_21, width);
}

static void triangleNormal(GLfloat * out, GLfloat * p0, GLfloat * p1, GLfloat * p2) {
  GLfloat v0[3], v1[3];
  v0[0] = p1[0] - p0[0];
  v0[1] = p1[1] - p0[1];
  v0[2] = p1[2] - p0[2];
  v1[0] = p2[0] - p1[0];
  v1[1] = p2[1] - p1[1];
  v1[2] = p2[2] - p1[2];
  MVEC3CROSS(out, v0, v1);
  MVEC3NORMALIZE(out);
}

static void dataNormals(GLfloat * data, int w, int h) {
  int x, z, zw, i;
  GLfloat n[18];
  for(z = 1; z < h - 1; z++) {
    zw = z * w;
    for(x = 1; x < w - 1; x++) {
      triangleNormal(&n[0], &data[6 * (x + zw)], &data[6 * (x + 1 + zw)], &data[6 * (x + (z + 1) * w)]);
      triangleNormal(&n[3], &data[6 * (x + zw)], &data[6 * (x + (z + 1) * w)], &data[6 * (x - 1 + (z + 1) * w)]);
      triangleNormal(&n[6], &data[6 * (x + zw)], &data[6 * (x - 1 + (z + 1) * w)], &data[6 * (x - 1 + zw)]);
      triangleNormal(&n[9], &data[6 * (x + zw)], &data[6 * (x - 1 + zw)], &data[6 * (x + (z - 1) * w)]);
      triangleNormal(&n[12], &data[6 * (x + zw)], &data[6 * (x + (z - 1) * w)], &data[6 * (x + 1 + (z - 1) * w)]);
      triangleNormal(&n[15], &data[6 * (x + zw)], &data[6 * (x + 1 + (z - 1) * w)], &data[6 * (x + 1 + zw)]);
      data[6 * (x + zw) + 3] = 0;
      data[6 * (x + zw) + 4] = 0;
      data[6 * (x + zw) + 5] = 0;
      for(i = 0; i < 6; i++) {
	data[6 * (x + zw) + 3] += n[3 * i + 0];
	data[6 * (x + zw) + 4] += n[3 * i + 1];
	data[6 * (x + zw) + 5] += n[3 * i + 2];	
      }
      data[6 * (x + zw) + 3] /= 6.0;
      data[6 * (x + zw) + 4] /= 6.0;
      data[6 * (x + zw) + 5] /= 6.0;
    }
  }
}

static GLfloat * heightMap2Data(GLfloat * hm, int w, int h) {
  int x, z, zw, i;
  GLfloat * data = malloc(6 * w * h * sizeof *data);
  double nx, nz, pnx = 2.0 / w, pnz = 2.0 / h;
  assert(data);
  for(z = 0, nz = 1.0; z < h; z++, nz -= pnz) {
    zw = z * w;
    for(x = 0, nx = -1.0; x < w; x++, nx += pnx) {
      i = 6 * (zw + x);
      data[i++] = (GLfloat)nx;
      data[i++] = 2.0 * hm[zw + x] - 1.0;
      data[i++] = (GLfloat)nz;
      data[i++] = gl4dmURand();
      data[i++] = gl4dmURand();
      data[i++] = gl4dmURand();
    }
  }
  dataNormals(data, w, h);
  for(i = 0, nx = 0.0; i < w*h; i++)
    if(nx < hm[i]) nx = hm[i];
  printf("max = %f\n",(float)nx);
  return data;
}

static GLuint * heightMapIndexedData(int w, int h) {
  int x, z, zw, i;
  GLuint * data = malloc(2 * 3 * (w - 1) * (h - 1) * sizeof *data);
  assert(data);
  for(z = 0; z < h - 1; z++) {
    zw = z * w;
    for(x = 0; x < w - 1; x++) {
      i = 2 * 3 * (z * (w - 1) + x);
      data[i++] = x + zw;
      data[i++] = x + zw + 1;
      data[i++] = x + zw + w;
      data[i++] = x + zw + w;
      data[i++] = x + zw + 1;
      data[i++] = x + zw + w + 1;
    }
  }
  return data;
}

static void Horizontal(float dec){
  GLfloat aux;
  int i,j;
 
  for(i = 0; i < _landscape_w * 6; i+= 6)
    for(j = 0; j < _landscape_h / 2; j++){
      aux = DATA[(6* _landscape_w * (_landscape_h-1)) - (j * 6 *_landscape_w) + i + 1];
      DATA[(6* _landscape_w * (_landscape_h-1))  - (j * 6 *_landscape_w) + i + 1] = DATA[j * 6 *_landscape_w + i + 1];
      DATA[j * 6 *_landscape_w + i + 1] = aux;

      aux = DATA[(6* _landscape_w * (_landscape_h-1)) - (j * 6 *_landscape_w) + i + 3];
      DATA[(6* _landscape_w * (_landscape_h-1))  - (j * 6 *_landscape_w) + i + 3] = DATA[j * 6 *_landscape_w + i + 3];
      DATA[j * 6 *_landscape_w + i + 3] = aux;

      aux = DATA[(6* _landscape_w * (_landscape_h-1)) - (j * 6 *_landscape_w) + i + 4];
      DATA[(6* _landscape_w * (_landscape_h-1))  - (j * 6 *_landscape_w) + i + 4] = DATA[j * 6 *_landscape_w + i + 4];
      DATA[j * 6 *_landscape_w + i + 4] = aux;

      aux = DATA[(6* _landscape_w * (_landscape_h-1)) - (j * 6 *_landscape_w) + i + 5];
      DATA[(6* _landscape_w * (_landscape_h-1))  - (j * 6 *_landscape_w) + i + 5] = -DATA[j * 6 *_landscape_w + i + 5];
      DATA[j * 6 *_landscape_w + i + 5] = -aux;
    }

  glBindVertexArray(_landscapeVao);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, _landscapeBuffer[0]);
  glBufferData(GL_ARRAY_BUFFER, 6 * _landscape_w * _landscape_h * sizeof *DATA, DATA, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof *DATA, (const void *)0);  
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof *DATA, (const void *)(3 * sizeof *DATA));  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _landscapeBuffer[1]);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  for(i = 0; i < 6 * _landscape_w * _landscape_h; i+= 6)
    DATA2[i + 2] += dec;
} 

static void Vertical(float dec){
  GLfloat aux;
  int i,j;
  
  for(i = 0; i < _landscape_h; i++)
    for(j = 0; j < (_landscape_w * 6)/2; j+= 6){
      aux = DATA[i * 6 *_landscape_w + 6*(_landscape_h-1) + 1 - j];
      DATA[i * 6 *_landscape_w + 6*(_landscape_h-1) + 1 - j] = DATA[i * 6 *_landscape_w + j + 1];
      DATA[i * 6 *_landscape_w + j + 1] = aux;

      aux = DATA[i * 6 *_landscape_w + 6*(_landscape_h-1) + 3 - j];
      DATA[i * 6 *_landscape_w + 6*(_landscape_h-1) + 3 - j] = -DATA[i * 6 *_landscape_w + j + 3];
      DATA[i * 6 *_landscape_w + j + 3] = -aux;

      aux = DATA[i * 6 *_landscape_w + 6*(_landscape_h-1) + 4 - j];
      DATA[i * 6 *_landscape_w + 6*(_landscape_h-1) + 4 - j] = DATA[i * 6 *_landscape_w + j + 4];
      DATA[i * 6 *_landscape_w + j + 4] = aux;

      aux = DATA[i * 6 *_landscape_w + 6*(_landscape_h-1) + 5 - j];
      DATA[i * 6 *_landscape_w + 6*(_landscape_h-1) + 5 - j] = DATA[i * 6 *_landscape_w + j + 5];
      DATA[i * 6 *_landscape_w + j + 5] = aux;
     }

  glBindVertexArray(_landscapeVao);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, _landscapeBuffer[0]);
  glBufferData(GL_ARRAY_BUFFER, 6 * _landscape_w * _landscape_h * sizeof *DATA, DATA, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof *DATA, (const void *)0);  
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof *DATA, (const void *)(3 * sizeof *DATA));  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _landscapeBuffer[1]);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  for(i = 0; i < 6 * _landscape_w * _landscape_h; i+= 6)
    DATA2[i] += dec;  
} 
