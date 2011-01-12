#include <cmath>
#include <iostream>
#include <vector>
#include "SDL.h"
#include "SDL_opengl.h" 

static const float PI = 3.141592654f;
using namespace std;

class Camera
{
  protected:
    float xpos;
    float ypos;
    float zpos;
    float xrot;
    float yrot;

  public:
    Camera( float x=0.0, float y=0.0, float z=0.0, float xr=0.0, float yr=0.0 );
    void teleport( float x, float y, float z );

    void walk( float amount );
    void strafe( float amount );
    void turn( float amount );
    void look( float amount );
    void ascend( float amount );

    void adjustGL();
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
  float yrotrad = yrot / 180.0 * PI;
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

  float yrotrad = rot / 180.0 * PI;
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

// -----------------------------------------------------------------------------

class Player : public Camera
{
  public:
    Player( float x=0.0, float y=0.0, float z=0.0, float xr=0.0, float yr=0.0 );
    void update();
    void inspect();
};

Player::Player( float x, float y, float z, float xr, float yr )
  : Camera( x, y, z, xr, yr )
{
  /* */
}

void Player::update()
{
  Uint8 *keys = SDL_GetKeyState(0);
  int mousex, mousey;  
  Uint8 buttons = SDL_GetRelativeMouseState(&mousex, &mousey);

  if (keys[SDLK_UP]) walk( 0.1 );
  if (keys[SDLK_DOWN]) walk( -0.1 );
  if (keys[SDLK_LEFT]) strafe( -0.1 );
  if (keys[SDLK_RIGHT]) strafe( 0.1 );
  if (keys[SDLK_PAGEUP]) ascend( 0.1 );
  if (keys[SDLK_PAGEDOWN]) ascend( -0.1 );

  turn( float(mousex) / 8.0 );
  look( float(mousey) / 8.0 );
}

void Player::inspect()
{
  cout << "PLAYER " << xpos << " " << ypos << " " << zpos
       << " " << xrot << " " << yrot << "\n";
}

// -----------------------------------------------------------------------------

class Voxel
{
  protected:
    float xpos;
    float ypos;
    float zpos;

  public:
    Voxel( float x=0.0, float y=0.0, float z=0.0 );
    void draw();
};

Voxel::Voxel( float x, float y, float z )
  : xpos(x), ypos(y), zpos(z)
{
  /* */
}

void Voxel::draw()
{
  const float s = 1.0f;

  glBegin(GL_QUADS);
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f( xpos,     ypos + s, zpos     );
    glVertex3f( xpos,     ypos + s, zpos + s );
    glVertex3f( xpos + s, ypos + s, zpos + s );
    glVertex3f( xpos + s, ypos + s, zpos     );

    glColor3f(1.0, 0.0, 0.0);
    glVertex3f( xpos,     ypos,     zpos     );
    glVertex3f( xpos + s, ypos,     zpos     );
    glVertex3f( xpos + s, ypos,     zpos + s );
    glVertex3f( xpos,     ypos,     zpos + s );

    glColor3f(0.0, 1.0, 0.0);
    glVertex3f( xpos,     ypos,     zpos + s );
    glVertex3f( xpos + s, ypos,     zpos + s );
    glVertex3f( xpos + s, ypos + s, zpos + s );
    glVertex3f( xpos,     ypos + s, zpos + s );

    glColor3f(0.0, 1.0, 0.0);
    glVertex3f( xpos,     ypos,     zpos     );
    glVertex3f( xpos,     ypos + s, zpos     );
    glVertex3f( xpos + s, ypos + s, zpos     );
    glVertex3f( xpos + s, ypos,     zpos     );

    glColor3f(0.0, 0.0, 1.0);
    glVertex3f( xpos + s, ypos,     zpos     );
    glVertex3f( xpos + s, ypos + s, zpos     );
    glVertex3f( xpos + s, ypos + s, zpos + s );
    glVertex3f( xpos + s, ypos,     zpos + s );

    glColor3f(0.0, 0.0, 1.0);
    glVertex3f( xpos,     ypos,     zpos     );
    glVertex3f( xpos,     ypos,     zpos + s );
    glVertex3f( xpos,     ypos + s, zpos + s );
    glVertex3f( xpos,     ypos + s, zpos     );

  glEnd();
}

void setGLPerspective(GLfloat fovy, GLfloat aspect, GLfloat zmin, GLfloat zmax)
{
  GLfloat xmin, xmax, ymin, ymax;
  ymax = zmin * tan(fovy * PI / 360.0);
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

void render( Camera &camera, vector<Voxel> &voxels )
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  camera.adjustGL();

  for ( int i=0; i<voxels.size(); i++ )
    voxels[i].draw();

  SDL_GL_SwapBuffers();
}

void gameloop()
{
  SDL_Event event;
  Player player( 0.0, 1.5 );

  vector<Voxel> voxels;
  voxels.push_back( Voxel( -4.0, 0.0, -2.0 ) );
  voxels.push_back( Voxel( -2.0, 0.0, -1.0 ) );
  voxels.push_back( Voxel(  0.0, 0.0,  0.0 ) );
  voxels.push_back( Voxel(  2.0, 0.0,  1.0 ) );
  voxels.push_back( Voxel(  4.0, 0.0,  2.0 ) );
  voxels.push_back( Voxel(  0.0, 0.0, -6.0 ) );
  voxels.push_back( Voxel(  0.0, 0.0,  6.0 ) );
  voxels.push_back( Voxel(  6.0, 0.0,  0.0 ) );
  voxels.push_back( Voxel( -6.0, 0.0,  0.0 ) );

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
          }
          break;
      }
    }

    player.update();

    render( player, voxels );
    SDL_Delay( 20 );
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

