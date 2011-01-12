#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "SDL.h"
#include "SDL_opengl.h" 

using namespace std;

class Camera
{
  protected:
    float xpos;
    float ypos;
    float zpos;
    float xrot;
    float yrot;

    float frustum[6][4];

  public:
    Camera( float x=0.0, float y=0.0, float z=0.0, float xr=0.0, float yr=0.0 );
    void teleport( float x, float y, float z );

    void walk( float amount );
    void strafe( float amount );
    void turn( float amount );
    void look( float amount );
    void ascend( float amount );

    void adjustGL();

    void readyFrustum();
    bool frustumContainsPoint( float x, float y, float z );
    bool frustumContainsCube( float x, float y, float z, float size );
};

Camera::Camera( float x, float y, float z, float xr, float yr )
  : xpos(x), ypos(y), zpos(z), xrot(xr), yrot(yr)
{
  /* */
}

void Camera::teleport( float x, float y, float z )
{
  xpos = x;
  ypos = y;
  zpos = z;
}

void Camera::walk( float amount )
{
  float yrotrad = yrot / 180.0 * M_PI;
  xpos += amount * float(sin(yrotrad)) ;
  zpos -= amount * float(cos(yrotrad)) ;
}

void Camera::ascend( float amount )
{
  ypos += amount;
}

void Camera::strafe( float amount )
{
  float rot = yrot + 90.0f;
  while (rot > 360.0f) rot -= 360.0f;

  float yrotrad = rot / 180.0 * M_PI;
  xpos += amount * float(sin(yrotrad)) ;
  zpos -= amount * float(cos(yrotrad)) ;
}

void Camera::turn( float amount )
{
  yrot += amount;
  while ( yrot < -360.0f ) yrot += 360.0f;
  while ( yrot >  360.0f ) yrot -= 360.0f;
}

void Camera::look( float amount )
{
  xrot += amount;

  if ( xrot > 90.0f ) xrot = 90.0f;
  if ( xrot < -90.0f ) xrot = -90.0f;
}

void Camera::adjustGL()
{
  glRotatef( xrot, 1.0, 0.0, 0.0 );
  glRotatef( yrot, 0.0, 1.0, 0.0 );
  glTranslated( -xpos, -ypos, -zpos );
}

void Camera::readyFrustum()
{
  // thanks mark morley
  // http://www.crownandcutlass.com/features/technicaldetails/frustum.html

  float   proj[16];
  float   modl[16];
  float   clip[16];

  /* Get the current PROJECTION matrix from OpenGL */
  glGetFloatv( GL_PROJECTION_MATRIX, proj );

  /* Get the current MODELVIEW matrix from OpenGL */
  glGetFloatv( GL_MODELVIEW_MATRIX, modl );

  /* Combine the two matrices (multiply projection by modelview) */
  clip[ 0] = modl[ 0] * proj[ 0] + modl[ 1] * proj[ 4] + modl[ 2] * proj[ 8] + modl[ 3] * proj[12];
  clip[ 1] = modl[ 0] * proj[ 1] + modl[ 1] * proj[ 5] + modl[ 2] * proj[ 9] + modl[ 3] * proj[13];
  clip[ 2] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] + modl[ 2] * proj[10] + modl[ 3] * proj[14];
  clip[ 3] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] + modl[ 2] * proj[11] + modl[ 3] * proj[15];

  clip[ 4] = modl[ 4] * proj[ 0] + modl[ 5] * proj[ 4] + modl[ 6] * proj[ 8] + modl[ 7] * proj[12];
  clip[ 5] = modl[ 4] * proj[ 1] + modl[ 5] * proj[ 5] + modl[ 6] * proj[ 9] + modl[ 7] * proj[13];
  clip[ 6] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] + modl[ 6] * proj[10] + modl[ 7] * proj[14];
  clip[ 7] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] + modl[ 6] * proj[11] + modl[ 7] * proj[15];

  clip[ 8] = modl[ 8] * proj[ 0] + modl[ 9] * proj[ 4] + modl[10] * proj[ 8] + modl[11] * proj[12];
  clip[ 9] = modl[ 8] * proj[ 1] + modl[ 9] * proj[ 5] + modl[10] * proj[ 9] + modl[11] * proj[13];
  clip[10] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] + modl[10] * proj[10] + modl[11] * proj[14];
  clip[11] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] + modl[10] * proj[11] + modl[11] * proj[15];

  clip[12] = modl[12] * proj[ 0] + modl[13] * proj[ 4] + modl[14] * proj[ 8] + modl[15] * proj[12];
  clip[13] = modl[12] * proj[ 1] + modl[13] * proj[ 5] + modl[14] * proj[ 9] + modl[15] * proj[13];
  clip[14] = modl[12] * proj[ 2] + modl[13] * proj[ 6] + modl[14] * proj[10] + modl[15] * proj[14];
  clip[15] = modl[12] * proj[ 3] + modl[13] * proj[ 7] + modl[14] * proj[11] + modl[15] * proj[15];

  /* Extract the numbers for the RIGHT plane */
  frustum[0][0] = clip[ 3] - clip[ 0];
  frustum[0][1] = clip[ 7] - clip[ 4];
  frustum[0][2] = clip[11] - clip[ 8];
  frustum[0][3] = clip[15] - clip[12];

  /* Extract the numbers for the LEFT plane */
  frustum[1][0] = clip[ 3] + clip[ 0];
  frustum[1][1] = clip[ 7] + clip[ 4];
  frustum[1][2] = clip[11] + clip[ 8];
  frustum[1][3] = clip[15] + clip[12];

  /* Extract the BOTTOM plane */
  frustum[2][0] = clip[ 3] + clip[ 1];
  frustum[2][1] = clip[ 7] + clip[ 5];
  frustum[2][2] = clip[11] + clip[ 9];
  frustum[2][3] = clip[15] + clip[13];

  /* Extract the TOP plane */
  frustum[3][0] = clip[ 3] - clip[ 1];
  frustum[3][1] = clip[ 7] - clip[ 5];
  frustum[3][2] = clip[11] - clip[ 9];
  frustum[3][3] = clip[15] - clip[13];

  /* Extract the FAR plane */
  frustum[4][0] = clip[ 3] - clip[ 2];
  frustum[4][1] = clip[ 7] - clip[ 6];
  frustum[4][2] = clip[11] - clip[10];
  frustum[4][3] = clip[15] - clip[14];

  /* Extract the NEAR plane */
  frustum[5][0] = clip[ 3] + clip[ 2];
  frustum[5][1] = clip[ 7] + clip[ 6];
  frustum[5][2] = clip[11] + clip[10];
  frustum[5][3] = clip[15] + clip[14];
}

bool Camera::frustumContainsPoint( float x, float y, float z )
{
  int p;

  for( p = 0; p < 6; p++ )
    if( frustum[p][0]*x + frustum[p][1]*y + frustum[p][2]*z + frustum[p][3] <= 0 )
      return false;
  return true;
}

bool Camera::frustumContainsCube( float x, float y, float z, float size )
{
  const float xs = x+size;
  const float ys = y+size;
  const float zs = z+size;

  for( int p = 0; p < 6; p++ )
  {
    if ((frustum[p][0] *  x + frustum[p][1] *  y + frustum[p][2] *  z + frustum[p][3] > 0) ||
        (frustum[p][0] * xs + frustum[p][1] *  y + frustum[p][2] *  z + frustum[p][3] > 0) ||
        (frustum[p][0] *  x + frustum[p][1] * ys + frustum[p][2] *  z + frustum[p][3] > 0) ||
        (frustum[p][0] * xs + frustum[p][1] * ys + frustum[p][2] *  z + frustum[p][3] > 0) ||
        (frustum[p][0] *  x + frustum[p][1] *  y + frustum[p][2] * zs + frustum[p][3] > 0) ||
        (frustum[p][0] * xs + frustum[p][1] *  y + frustum[p][2] * zs + frustum[p][3] > 0) ||
        (frustum[p][0] *  x + frustum[p][1] * ys + frustum[p][2] * zs + frustum[p][3] > 0) ||
        (frustum[p][0] * xs + frustum[p][1] * ys + frustum[p][2] * zs + frustum[p][3] > 0))
      continue;
    else
      return false;
  }
  return true;
}

// -----------------------------------------------------------------------------

class Player : public Camera
{
  public:
    Player( float x=0.0, float y=0.0, float z=0.0, float xr=0.0, float yr=0.0 );
    void update( float dt );
    void inspect();
};

Player::Player( float x, float y, float z, float xr, float yr )
  : Camera( x, y, z, xr, yr )
{
  /* */
}

void Player::update( float dt )
{
  const float speed = 2.0 * dt;

  Uint8 *keys = SDL_GetKeyState(0);
  int mousex, mousey;  
  Uint8 buttons = SDL_GetRelativeMouseState(&mousex, &mousey);

  if (keys[SDLK_UP]) walk( 1.0 * speed );
  if (keys[SDLK_DOWN]) walk( -1.0 * speed );
  if (keys[SDLK_LEFT]) strafe( -1.0 * speed );
  if (keys[SDLK_RIGHT]) strafe( 1.0 * speed );
  if (keys[SDLK_PAGEUP]) ascend( 1.0 * speed );
  if (keys[SDLK_PAGEDOWN]) ascend( -1.0 * speed );

  turn( float(mousex) * speed * 2.0 );
  look( float(mousey) * speed * 1.5 );
}

void Player::inspect()
{
  cout << "PLAYER " << xpos << " " << ypos << " " << zpos
       << " " << xrot << " " << yrot << "\n";
}

// -----------------------------------------------------------------------------

class Chunk
{
  protected:
    float xpos;
    float ypos;
    float zpos;

    int data[16][16][16];

    static const float VOXSIZE = 1.0f;

  public:
    Chunk( float x=0.0, float y=0.0, float z=0.0 );
    void randomize();
    void draw( Camera &camera );
    void drawVoxel( float x, float y, float z, float s );
};

Chunk::Chunk( float x, float y, float z )
  : xpos(x), ypos(y), zpos(z)
{
  /* */
}

void Chunk::randomize()
{
  for ( int z = 0; z < 16; z ++ ) {
    for ( int y = 0; y < 16; y ++ ) {
      for ( int x = 0; x < 16; x ++ ) {
        data[z][y][x] = rand() % 2;
      }
    }
  }
}

void Chunk::draw( Camera &camera )
{
  if ( !camera.frustumContainsCube( xpos, ypos, zpos, VOXSIZE * 16.0f ) )
    return;

  for ( int z = 0; z < 16; z ++ ) {
    for ( int y = 0; y < 16; y ++ ) {
      for ( int x = 0; x < 16; x ++ ) {
        if ( data[z][y][x] ) {
          float xp = xpos + float(x);
          float yp = ypos + float(y);
          float zp = zpos + float(z);
          if ( camera.frustumContainsCube( xp, yp, zp, VOXSIZE ) )
            drawVoxel( xp, yp, zp, VOXSIZE );
        }
      }
    }
  }
}

void Chunk::drawVoxel( float x, float y, float z, float s )
{
  glBegin(GL_QUADS);
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f( x,     y + s, z     );
    glVertex3f( x,     y + s, z + s );
    glVertex3f( x + s, y + s, z + s );
    glVertex3f( x + s, y + s, z     );

    glColor3f(1.0, 0.0, 0.0);
    glVertex3f( x,     y,     z     );
    glVertex3f( x + s, y,     z     );
    glVertex3f( x + s, y,     z + s );
    glVertex3f( x,     y,     z + s );

    glColor3f(0.0, 1.0, 0.0);
    glVertex3f( x,     y,     z + s );
    glVertex3f( x + s, y,     z + s );
    glVertex3f( x + s, y + s, z + s );
    glVertex3f( x,     y + s, z + s );

    glColor3f(0.0, 1.0, 0.0);
    glVertex3f( x,     y,     z     );
    glVertex3f( x,     y + s, z     );
    glVertex3f( x + s, y + s, z     );
    glVertex3f( x + s, y,     z     );

    glColor3f(0.0, 0.0, 1.0);
    glVertex3f( x + s, y,     z     );
    glVertex3f( x + s, y + s, z     );
    glVertex3f( x + s, y + s, z + s );
    glVertex3f( x + s, y,     z + s );

    glColor3f(0.0, 0.0, 1.0);
    glVertex3f( x,     y,     z     );
    glVertex3f( x,     y,     z + s );
    glVertex3f( x,     y + s, z + s );
    glVertex3f( x,     y + s, z     );

  glEnd();
}

// -----------------------------------------------------------------------------

void setGLPerspective(GLfloat fovy, GLfloat aspect, GLfloat zmin, GLfloat zmax)
{
  GLfloat xmin, xmax, ymin, ymax;
  ymax = zmin * tan(fovy * M_PI / 360.0);
  ymin = -ymax;
  xmin = ymin * aspect;
  xmax = ymax * aspect;
  glFrustum(xmin, xmax, ymin, ymax, zmin, zmax);
}

SDL_Surface *setupScreen()
{
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,        1);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,            8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,          8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,           8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,          8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,         16);
  SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,        32);
  SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,      8);
  SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,    8);
  SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,     8);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,  1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,  2);

  SDL_Surface *screen = SDL_SetVideoMode( 640, 480, 32, SDL_HWSURFACE | SDL_OPENGL);

  glClearColor(0, 0, 0, 0);
  glViewport(0, 0, 640, 480);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  setGLPerspective( 60.0f, 640.0 / 480.0, 0.1f, 500.0f );

  // glOrtho(0, 640, 480, 0, 1, -1);
  glMatrixMode(GL_MODELVIEW);
  glEnable(GL_TEXTURE_2D);
  glLoadIdentity();

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);

  SDL_WM_GrabInput( SDL_GRAB_ON );
  SDL_ShowCursor( SDL_DISABLE );

  return screen;
}

void render( Camera &camera, vector<Chunk> &chunks )
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  camera.adjustGL();
  camera.readyFrustum();

  for ( int i=0; i<chunks.size(); i++ )
    chunks[i].draw( camera );

  SDL_GL_SwapBuffers();
}

void gameloop()
{
  SDL_Event event;
  Player player( 0.0, 1.5 );

  vector<Chunk> chunks;
  for ( int z = 0; z < 3; z ++ ) {
    for ( int y = 0; y < 3; y ++ ) {
      for ( int x = 0; x < 3; x ++ ) {
        Chunk chunk( x*16.0, y*16.0, z*16.0 );
        chunk.randomize();
        chunks.push_back( chunk );
      }
    }
  }

  int clock = SDL_GetTicks();
  int lastClock = clock - 1;

  while (true)
  {
    while ( SDL_PollEvent( &event ) )
    {
      switch ( event.type ) {
        case SDL_QUIT: return;

        case SDL_KEYDOWN:
          switch ( event.key.keysym.sym ) {
            case SDLK_F10: return;
            case SDLK_F3:
              player.inspect();
              break;
            case SDLK_F7:
              glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
              break;
            case SDLK_F8:
              glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
              break;
          }
          break;
      }
    }

    clock = SDL_GetTicks();
    float dt = float(clock - lastClock) / 1000.0f;
    if ( dt > 0.25 ) {
      cout << "skip " << dt << "\n";
      dt = 0.25;
    }
    lastClock = clock;

    player.update( dt );

    render( player, chunks );
    SDL_Delay( 10 );
  }
}

int main( int argc, char **argv )
{
  if ( SDL_Init(SDL_INIT_EVERYTHING) < 0 ) {
    return 1;
  }

  SDL_Surface *screen = setupScreen();
  if (!screen) return 1;

  gameloop();

  SDL_Quit();
  return 0;
}

