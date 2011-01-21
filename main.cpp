// made by inny

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <vector>
#include "SDL.h"
#include "SDL_opengl.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

using namespace std;

float bound( float min, float val, float max )
{
  if ( min > val ) return min;
  if ( max < val ) return max;
  return val;
}

bool fuzzyEq( float a, float b )
{
  float t = a - b;
  return ( (t < 0.0001) && (t > -0.0001) );
}

// -----------------------------------------------------------------------------

class Camera
{
  protected:
    float xpos;
    float ypos;
    float zpos;
    float xrot;
    float yrot;

    float viewDist;
    bool fogged;

    float frustum[6][4];

    bool updated;

  public:
    Camera( float x=0.0, float y=0.0, float z=0.0, float xr=0.0, float yr=0.0 );
    void teleport( float x, float y, float z );

    float x() const { return xpos; };
    float y() const { return ypos; };
    float z() const { return zpos; };

    void walk( float amount );
    void strafe( float amount );
    void turn( float amount );
    void look( float amount );
    void ascend( float amount );

    void set3DPerspective( float fovy, float aspect );
    void adjustGL();

    void readyFrustum();
    bool frustumContainsPoint( float x, float y, float z );
    bool frustumContainsCube( float x, float y, float z, float xs, float ys, float zs );

    void setViewDistance( float d ) { viewDist = d; };
    float viewDistance() const { return viewDist; };

    void setFog( bool enabled );
    bool fogEnabled() const { return fogged; };
};

Camera::Camera( float x, float y, float z, float xr, float yr )
  : xpos(x), ypos(y), zpos(z), xrot(xr), yrot(yr), viewDist(80.0),
    fogged(false), updated(true)
{
  /* */
}

void Camera::teleport( float x, float y, float z )
{
  updated = true;
  xpos = x;
  ypos = y;
  zpos = z;
}

void Camera::walk( float amount )
{
  updated = true;
  float yrotrad = yrot / 180.0 * M_PI;
  xpos += amount * float(sin(yrotrad)) ;
  zpos -= amount * float(cos(yrotrad)) ;
}

void Camera::ascend( float amount )
{
  updated = true;
  ypos += amount;
}

void Camera::strafe( float amount )
{
  updated = true;
  float rot = yrot + 90.0f;
  while (rot > 360.0f) rot -= 360.0f;

  float yrotrad = rot / 180.0 * M_PI;
  xpos += amount * float(sin(yrotrad)) ;
  zpos -= amount * float(cos(yrotrad)) ;
}

void Camera::turn( float amount )
{
  updated = true;
  yrot += amount;
  while ( yrot < -360.0f ) yrot += 360.0f;
  while ( yrot >  360.0f ) yrot -= 360.0f;
}

void Camera::look( float amount )
{
  updated = true;
  xrot += amount;

  if ( xrot > 90.0f ) xrot = 90.0f;
  if ( xrot < -90.0f ) xrot = -90.0f;
}

void Camera::set3DPerspective( float fovy, float aspect )
{
  const float zmin = 0.25;
  const float zmax = viewDist;

  GLfloat xmin, xmax, ymin, ymax;
  ymax = zmin * tan(fovy * M_PI / 360.0);
  ymin = -ymax;
  xmin = ymin * aspect;
  xmax = ymax * aspect;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(xmin, xmax, ymin, ymax, zmin, zmax);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void Camera::setFog( bool enabled )
{
  fogged = enabled;

  if (!enabled) {
    glDisable(GL_FOG);
    return;
  }

  const float farFog = viewDistance() * ( 72.0 / 80.0 );
  const float nearFog = farFog * ( 56.0 / 72.0 );

  GLfloat fogColor[4]= {0.75f, 0.75f, 0.75f, 1.0f};
  glEnable(GL_FOG);
  glFogfv(GL_FOG_COLOR, fogColor);
  glFogi(GL_FOG_MODE, GL_LINEAR);
  glFogf(GL_FOG_START, nearFog);
  glFogf(GL_FOG_END, farFog);
  glHint(GL_FOG_HINT, GL_DONT_CARE);  
}

void Camera::adjustGL()
{
  // Should be on the MODELVIEW Matrix
  glRotatef( xrot, 1.0, 0.0, 0.0 );
  glRotatef( yrot, 0.0, 1.0, 0.0 );
  glTranslated( -xpos, -ypos, -zpos );
}

void Camera::readyFrustum()
{
  return;

  if (!updated) return;
  updated = false;

  // thanks mark morley
  // http://www.crownandcutlass.com/features/technicaldetails/frustum.html

  float   proj[16];
  float   modl[16];
  float   clip[16];

  /* Get the current PROJECTION matrix from OpenGL */
  glGetFloatv( GL_PROJECTION_MATRIX, proj );

  /* Get the current MODELVIEW matrix from OpenGL */
  glGetFloatv( GL_MODELVIEW_MATRIX, modl );

  // Combine the two matrices (multiply projection by modelview)
  // Assumes no rotation on the Projection Matrix.
  clip[ 0] = modl[ 0] * proj[ 0];
  clip[ 1] = modl[ 1] * proj[ 5];
  clip[ 2] = modl[ 2] * proj[10] + modl[ 3] * proj[14];
  clip[ 3] = modl[ 2] * proj[11];

  clip[ 4] = modl[ 4] * proj[ 0];
  clip[ 5] = modl[ 5] * proj[ 5];
  clip[ 6] = modl[ 6] * proj[10] + modl[ 7] * proj[14];
  clip[ 7] = modl[ 6] * proj[11];

  clip[ 8] = modl[ 8] * proj[ 0];
  clip[ 9] = modl[ 9] * proj[ 5];
  clip[10] = modl[10] * proj[10] + modl[11] * proj[14];
  clip[11] = modl[10] * proj[11];

  clip[12] = modl[12] * proj[ 0];
  clip[13] = modl[13] * proj[ 5];
  clip[14] = modl[14] * proj[10] + modl[15] * proj[14];
  clip[15] = modl[14] * proj[11];

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
  for ( int p = 0; p < 6; p++ )
    if( frustum[p][0]*x + frustum[p][1]*y + frustum[p][2]*z + frustum[p][3] <= 0 )
      return false;

  return true;
}

bool Camera::frustumContainsCube( float x, float y, float z, float xs, float ys, float zs )
{
  return true;

  for ( int p = 5; p < 6; p++ ) {
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
  const float moveSpeed = 4.0 * dt;
  const float lookSpeed = 0.10;

  Uint8 *keys = SDL_GetKeyState(0);
  int mousex, mousey;  
  Uint8 buttons = SDL_GetRelativeMouseState(&mousex, &mousey);

  if (keys[SDLK_UP]) walk( 1.0 * moveSpeed );
  if (keys[SDLK_DOWN]) walk( -1.0 * moveSpeed );
  if (keys[SDLK_LEFT]) strafe( -1.0 * moveSpeed );
  if (keys[SDLK_RIGHT]) strafe( 1.0 * moveSpeed );
  if (keys[SDLK_PAGEUP]) ascend( 1.0 * moveSpeed );
  if (keys[SDLK_PAGEDOWN]) ascend( -1.0 * moveSpeed );

  if (mousex) turn( float(mousex) * lookSpeed );
  if (mousey) look( float(mousey) * lookSpeed );
}

void Player::inspect()
{
  cout << "PLAYER " << xpos << " " << ypos << " " << zpos
       << " " << xrot << " " << yrot << "\n";
}

// -----------------------------------------------------------------------------

struct Texture
{
  GLuint t;
  bool valid;
  Texture( const string &filename );
  void bind();
};

Texture::Texture( const string &filename )
  : valid(false)
{
  SDL_Surface *image = IMG_Load( filename.c_str() ); 
  if (!image) return;

  SDL_Surface* display = SDL_DisplayFormatAlpha(image);

  glGenTextures( 1, &t );
  glBindTexture( GL_TEXTURE_2D, t );

  GLuint textureFormat = (display->format->Rmask == 0x0000ff)
    ? GL_RGBA
    : GL_BGRA;

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

  glTexImage2D( GL_TEXTURE_2D, 0, 4, 256, 256, 0,
                textureFormat, GL_UNSIGNED_BYTE, display->pixels );

  SDL_FreeSurface( display );
  SDL_FreeSurface( image );

  valid = true;
}

void Texture::bind()
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture( GL_TEXTURE_2D, t );
  glColor3f(1.0, 1.0, 1.0);
}

// -----------------------------------------------------------------------------

class TextPainter
{
  protected:
    TTF_Font *font;
    bool valid;
    GLuint t;
    int w;
    int h;

  public:
    TextPainter();
    ~TextPainter();
    void readyText( const GLubyte R, const GLubyte G, const GLubyte B, const std::string& text);
    void draw( const float x, const float y, const float z );
};

TextPainter::TextPainter()
  : valid(false)
{
  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &t);

  if(TTF_Init()==-1)
    cout << "TTF_Init: " << TTF_GetError() << "\n";

  // less linux dependancies in the near future, maybe?
  font=TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 16);
  cout << "TTF_OpenFont: " << TTF_GetError() << "\n";

  valid = true;
}

TextPainter::~TextPainter()
{
  if (valid) {
    glDeleteTextures(1, &t);
    TTF_CloseFont(font);
    TTF_Quit();
  }
}

void TextPainter::readyText( const GLubyte R, const GLubyte G, const GLubyte B, const std::string& text)
{
  // found at http://wiki.gamedev.net/index.php/SDL_ttf:Tutorials:Fonts_in_OpenGL
  if (!valid) return;

  SDL_Color color = {R, G, B};
  SDL_Surface *message = TTF_RenderText_Solid(font, text.c_str(), color);
  if (!message) {
    cout << "TTF_RenderText_Blended: " << TTF_GetError() << "\n";
    valid = false;
    return;
  }

  SDL_Surface *display = SDL_DisplayFormatAlpha(message);
  w = display->w;
  h = display->h;

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, t);
 
  GLuint textureFormat = (display->format->Rmask == 0x0000ff)
    ? GL_RGBA
    : GL_BGRA;

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
 
  glTexImage2D(GL_TEXTURE_2D, 0, 4, display->w, display->h, 0, textureFormat, GL_UNSIGNED_BYTE, display->pixels);

  SDL_FreeSurface(display);
  SDL_FreeSurface(message);
}

void TextPainter::draw( const float x, const float y, const float z )
{
  glEnable(GL_TEXTURE_2D);
  glColor3f(1.0, 1.0, 1.0);
  glBindTexture(GL_TEXTURE_2D, t);
  glBegin(GL_QUADS);
    glTexCoord2d(0, 0); glVertex3d(x,   y,   z);
    glTexCoord2d(1, 0); glVertex3d(x+w, y,   z);
    glTexCoord2d(1, 1); glVertex3d(x+w, y+h, z);
    glTexCoord2d(0, 1); glVertex3d(x,   y+h, z);
  glEnd();
}

// -----------------------------------------------------------------------------

struct Point
{
  float x, y;
  Point( float xx=0.0, float yy=0.0 ) : x(xx), y(yy) { /**/ };
};

struct Vertex
{
  float v[3];
  Vertex( float x=0.0, float y=0.0, float z=0.0 ) { v[0]=x; v[1]=y; v[2]=z; };
  float x() const { return v[0]; };
  float y() const { return v[1]; };
  float z() const { return v[2]; };
};

struct Face
{
  Vertex v[4];
  Point t;

  Face( const Vertex &va, const Vertex &vb, const Vertex &vc, const Vertex &vd,
        const Point &tx = Point() )
  { v[0]=va; v[1]=vb; v[2]=vc; v[3]=vd; t=tx; };

  void glTexturedDraw();
  void glUntexturedDraw();
};

void Face::glTexturedDraw()
{
  float xl = t.x / 256.0;
  float xr = (t.x+15.99) / 256.0;
  float yu = t.y / 256.0;
  float yd = (t.y+15.99) / 256.0;

  glTexCoord2f(xl, yu);
  glVertex3fv(v[0].v);
  glTexCoord2f(xr, yu);
  glVertex3fv(v[1].v);
  glTexCoord2f(xr, yd);
  glVertex3fv(v[2].v);
  glTexCoord2f(xl, yd);
  glVertex3fv(v[3].v);
}

void Face::glUntexturedDraw()
{
  glVertex3fv(v[0].v);
  glVertex3fv(v[1].v);
  glVertex3fv(v[2].v);
  glVertex3fv(v[3].v);
}

// -----------------------------------------------------------------------------

struct Voxel
{
  static const float SIZE = 1.0f;
  int type;
  bool visible;
  bool surf[6];

  Voxel(int t=0): type(t), visible(true) { for (int i=0; i<6; i++) surf[i]=true; }
  void draw( vector<Face> &drawList, float x, float y, float z, float s );

  static Voxel shared;
};

Voxel Voxel::shared;

void Voxel::draw( vector<Face> &drawList, float x, float y, float z, float s )
{
  if (!visible) return;

  Vertex va( x,   y,   z );
  Vertex vb( x,   y,   z+s );
  Vertex vc( x,   y+s, z );
  Vertex vd( x,   y+s, z+s );
  Vertex ve( x+s, y,   z );
  Vertex vf( x+s, y,   z+s );
  Vertex vg( x+s, y+s, z );
  Vertex vh( x+s, y+s, z+s );

  Point top( 0.0, 0.0 );
  Point side( 0.0, 16.0 );
  Point bottom( 0.0, 24.0 );

  if ( surf[0] ) drawList.push_back( Face( vc, vd, vh, vg, top ) );
  if ( surf[1] ) drawList.push_back( Face( va, ve, vf, vb, bottom ) );
  if ( surf[2] ) drawList.push_back( Face( vh, vd, vb, vf, side ) );
  if ( surf[3] ) drawList.push_back( Face( vc, vg, ve, va, side ) );
  if ( surf[4] ) drawList.push_back( Face( vg, vh, vf, ve, side ) );
  if ( surf[5] ) drawList.push_back( Face( vd, vc, va, vb, side ) );
}

// -----------------------------------------------------------------------------

#if 0

class Map;

class Chunk
{
  public:
    static const int XSIZE = 16;
    static const int YSIZE = 16;
    static const int ZSIZE = 16;

  protected:
    Map *map;
    float xpos;
    float ypos;
    float zpos;

    Voxel data[ZSIZE][YSIZE][XSIZE];

    bool generated;
    GLuint index;

    void generateDisplayList();
    bool distantTo( Camera &camera );

    void drawChunkCube();

  public:
    Chunk( Map *m, float x=0.0, float y=0.0, float z=0.0 );
    virtual ~Chunk();
    void randomize();
    void cullFaces();

    void draw( Camera &camera );
    Voxel &voxel( int x, int y, int z );
};

Chunk::Chunk( Map *m, float x, float y, float z )
  : map(m), xpos(x), ypos(y), zpos(z), generated( false )
{
  /* */
}

Chunk::~Chunk()
{
  if (generated) glDeleteLists(index, 1);
}

void Chunk::randomize()
{
  float xc = 0.5 * float(Map::XSIZE * Chunk::XSIZE);
  float yc = 0.5 * float(Map::YSIZE * Chunk::YSIZE);
  float zc = 0.5 * float(Map::ZSIZE * Chunk::ZSIZE);
  for ( int z = 0; z < Chunk::ZSIZE; z ++ ) {
    for ( int y = 0; y < Chunk::YSIZE; y ++ ) {
      for ( int x = 0; x < Chunk::XSIZE; x ++ ) {
        float xp = xpos + float(x) + 0.5*Voxel::SIZE;
        float yp = ypos + float(y) + 0.5*Voxel::SIZE;
        float zp = zpos + float(z) + 0.5*Voxel::SIZE;

        float vect = sqrt( xc*xc + yc*yc + zc*zc );
        float dist = sqrt( (xp-xc)*(xp-xc) + (yp-yc)*(yp-yc) + (zp-zc)*(zp-zc) );

        float chance = 1.0;
        if ( dist > (vect/2.0) )
          chance = 0;
        else if ( yp > yc )
          chance = 0.005;
        else if ( yc - yp < 3.0 )
          chance = 0.1;
        else
          chance = 0.5 + (0.5*dist/vect);

        data[z][y][x].type = ( rand() % 1000 < int(chance*1000.0) );
      }
    }
  }
}

void Chunk::cullFaces()
{
  int xi = int(xpos);
  int yi = int(ypos);
  int zi = int(zpos);

  for ( int z = 0; z < Chunk::ZSIZE; z ++ ) {
    for ( int y = 0; y < Chunk::YSIZE; y ++ ) {
      for ( int x = 0; x < Chunk::XSIZE; x ++ ) {
        Voxel &vox = voxel(x, y, z);
        // up
        Voxel &vox0 = map->voxel(xi+x, yi+y+1, zi+z);
        vox.surf[0] = (vox0.type==0);
        // down
        Voxel &vox1 = map->voxel(xi+x, yi+y-1, zi+z);
        vox.surf[1] = (vox1.type==0);
        // north
        Voxel &vox2 = map->voxel(xi+x, yi+y, zi+z+1);
        vox.surf[2] = (vox2.type==0);
        // south
        Voxel &vox3 = map->voxel(xi+x, yi+y, zi+z-1);
        vox.surf[3] = (vox3.type==0);
        // west
        Voxel &vox4 = map->voxel(xi+x+1, yi+y, zi+z);
        vox.surf[4] = (vox4.type==0);
        // east
        Voxel &vox5 = map->voxel(xi+x-1, yi+y, zi+z);
        vox.surf[5] = (vox5.type==0);

        vox.visible = false;
        for ( int i=0; i<6; i++ )
          vox.visible |= vox.surf[i];
        
      }
    }
  }
}

Voxel &Chunk::voxel( int x, int y, int z )
{
  if ( x < 0 || x >= Chunk::XSIZE ||
       y < 0 || y >= Chunk::YSIZE ||
       z < 0 || z >= Chunk::ZSIZE )
    return Voxel::shared;

  return data[z][y][x];
}

bool Chunk::distantTo( Camera &camera )
{
  const float xs = 0.5 * Chunk::XSIZE;
  const float ys = 0.5 * Chunk::YSIZE;
  const float zs = 0.5 * Chunk::ZSIZE;

  const float xp = xpos + xs;
  const float yp = ypos + ys;
  const float zp = zpos + zs;

  const float cx = camera.x();
  const float cy = camera.y();
  const float cz = camera.z();
  
  const float dist = (xp-cx)*(xp-cx) + (yp-cy)*(yp-cy) + (zp-cz)*(zp-cz);

  const float DIST = camera.viewDistance() + max( max(xs, ys), zs );
  return ( dist > DIST*DIST );
}

void Chunk::drawChunkCube()
{
  const float xs = Chunk::XSIZE;
  const float ys = Chunk::YSIZE;
  const float zs = Chunk::ZSIZE;

  Vertex va( xpos,    ypos,    zpos );
  Vertex vb( xpos,    ypos,    zpos+zs );
  Vertex vc( xpos,    ypos+ys, zpos );
  Vertex vd( xpos,    ypos+ys, zpos+zs );
  Vertex ve( xpos+xs, ypos,    zpos );
  Vertex vf( xpos+xs, ypos,    zpos+zs );
  Vertex vg( xpos+xs, ypos+ys, zpos );
  Vertex vh( xpos+xs, ypos+ys, zpos+zs );

  Face tp( vc, vd, vh, vg );
  Face bt( va, ve, vf, vb );
  Face sd1( vh, vd, vb, vf );
  Face sd2( vc, vg, ve, va );
  Face sd3( vg, vh, vf, ve );
  Face sd4( vd, vc, va, vb );

  glBegin(GL_QUADS);
  glColor3f(1.0, 1.0, 1.0);
  tp.glUntexturedDraw();
  bt.glUntexturedDraw();
  sd1.glUntexturedDraw();
  sd2.glUntexturedDraw();
  sd3.glUntexturedDraw();
  sd4.glUntexturedDraw();
  glEnd();
}

void Chunk::draw( Camera &camera )
{
  // if ( distantTo( camera ) )
  //  return;

  // if ( !camera.frustumContainsCube(xpos, ypos, zpos, xpos+Chunk::XSIZE, ypos+Chunk::YSIZE, zpos+Chunk::ZSIZE ) )
  //  return;

  if (!generated) generateDisplayList();

  glCallList(index);
}

void Chunk::generateDisplayList()
{
  generated = true;
  index = glGenLists(1);

  vector<Face> faceList;
  for ( int z = 0; z < Chunk::ZSIZE; z ++ ) {
    for ( int y = 0; y < Chunk::YSIZE; y ++ ) {
      for ( int x = 0; x < Chunk::XSIZE; x ++ ) {
        if ( data[z][y][x].type > 0 ) {
          data[z][y][x].draw( faceList, float(xpos+x), float(ypos+y), float(zpos+z), Voxel::SIZE );
        }
      }
    }
  }

  glNewList(index, GL_COMPILE);
  glBegin(GL_QUADS);
    for ( int i = 0; i < faceList.size(); i++ ) faceList[i].glTexturedDraw();
  glEnd();
  glEndList();
}

#endif

// -----------------------------------------------------------------------------

namespace Block {
  enum Type
  {
    SplitNode,
    Air,
    Dirt,
    MAX_TYPES
  };
};

class OctreeNode;
class OctreeVisitor
{
  public: virtual void visit( OctreeNode *node ) = 0;
};

class OctreeNode
{
  protected:
    OctreeNode *child[2][2][2]; // [z][y][x]

    Block::Type mType;

    float mXpos;
    float mYpos;
    float mZpos;
    float mXsize;
    float mYsize;
    float mZsize;

    bool visi[6];

    void subdivide();
    void optimize();
    void clearChildren();

    void forceType( Block::Type t ) { mType = t; };
    void setType( Block::Type t );

  public:
    OctreeNode( Block::Type t, float x, float y, float z, float xs, float ys, float zs );
    ~OctreeNode();

    Block::Type type() const { return mType; };
    bool leaf() const { return mType == Block::SplitNode; };

    float xPos() const { return mXpos; };
    float yPos() const { return mYpos; };
    float zPos() const { return mZpos; };
    float xSize() const { return mXsize; };
    float ySize() const { return mYsize; };
    float zSize() const { return mZsize; };

    bool faceVisible( int x ) const { return visi[x]; };

    OctreeNode *setChild( Block::Type t, float x, float y, float z, float xs=1.0, float ys=1.0, float zs=1.0 );

    void visit( OctreeVisitor *visitor, bool leavesOnly = true );
    void varDump( float &x, float &y, float &z, float &xs, float &ys, float &zs );

    void cullFaces( OctreeNode *node );
};

OctreeNode::OctreeNode( Block::Type t, float x, float y, float z, float xs, float ys, float zs )
  : mType(t), mXpos(x), mYpos(y), mZpos(z), mXsize(xs), mYsize(ys), mZsize(zs)
{
  for ( int zi = 0; zi<2; zi++ )
    for ( int yi = 0; yi<2; yi++ )
      for ( int xi = 0; xi<2; xi++ )
        child[zi][yi][xi] = 0;

  for ( int i=0; i<6; i++ )
    visi[i] = true;
}

OctreeNode::~OctreeNode()
{
  clearChildren();
}

void OctreeNode::clearChildren()
{
  for ( int zi = 0; zi<2; zi++ ) {
    for ( int yi = 0; yi<2; yi++ ) {
      for ( int xi = 0; xi<2; xi++ ) {
        delete child[zi][yi][xi];
        child[zi][yi][xi] = 0;
      }
    }
  }
}

void OctreeNode::setType( Block::Type t )
{
  clearChildren();
  mType = t;
}

void OctreeNode::varDump( float &x, float &y, float &z, float &xs, float &ys, float &zs )
{
  x = mXpos; y = mYpos; z = mZpos;
  xs = mXsize; ys = mYsize; zs = mZsize;
}

void OctreeNode::subdivide()
{
  if ( mType == Block::SplitNode )
    return;

  const float xs = xSize() / 2.0;
  const float ys = ySize() / 2.0;
  const float zs = zSize() / 2.0;

  for ( int zi = 0; zi<2; zi++ ) {
    for ( int yi = 0; yi<2; yi++ ) {
      for ( int xi = 0; xi<2; xi++ ) {
        const float xp = xPos() + ( float(xi) * xs );
        const float yp = yPos() + ( float(yi) * ys );
        const float zp = zPos() + ( float(zi) * zs );
        child[zi][yi][xi] = new OctreeNode( mType, xp, yp, zp, xs, ys, zs );
      }
    }
  }
  
  mType = Block::SplitNode;
}

void OctreeNode::optimize()
{
  if ( mType != Block::SplitNode )
    return;

  Block::Type t = child[0][0][0]->type();

  for ( int zi = 0; zi<2; zi++ )
    for ( int yi = 0; yi<2; yi++ )
      for ( int xi = 0; xi<2; xi++ )
        if ( child[zi][yi][xi]->type() != t )
          return;

  if ( t == Block::SplitNode )
    return;

  clearChildren();
  mType = t;
}

OctreeNode *OctreeNode::setChild( Block::Type t, float x, float y, float z, float xs, float ys, float zs )
{
  if ( (t == mType) || (t == Block::SplitNode) ) return 0;

  const float x2 = x + xs, y2 = y + ys, z2 = z + zs;

  subdivide();

  OctreeNode *rt = 0;

  bool d = false;
  for ( int zi = 0; zi<2 && !d; zi++ ) {
    for ( int yi = 0; yi<2 && !d; yi++ ) {
      for ( int xi = 0; xi<2 && !d; xi++ )
      {
        OctreeNode *node = child[zi][yi][xi];
        float cx, cy, cz, cxs, cys, czs;
        node->varDump( cx, cy, cz, cxs, cys, czs );
        const float cx2 = cx+cxs, cy2 = cy+cys, cz2=cz+czs;

        if ( ( cx <= x ) && ( cy <= y ) && ( cz <= z ) &&
             ( cx2 >= x2 ) && ( cy2 >= y2 ) && ( cz2 >= z2 ) )
        {
          if ( fuzzyEq(x,cx) && fuzzyEq(y,cy) && fuzzyEq(z,cz) &&
               fuzzyEq(x2,cx2) && fuzzyEq(y2,cy2) && fuzzyEq(z2,cz2) )
          {
            node->setType( t );
            rt = node;
          }
          else
          {
            rt = node->setChild( t, x, y, z, xs, ys, zs );
          }
          d = true;
          break;
        }
      }
    }
  }

  optimize();
  return rt;
}

void OctreeNode::visit( OctreeVisitor *visitor, bool leavesOnly )
{
  if ( mType == Block::SplitNode ) {
    if (!leavesOnly) {
      visitor->visit( this );
    }
    for ( int zi = 0; zi<2; zi++ )
      for ( int yi = 0; yi<2; yi++ )
        for ( int xi = 0; xi<2; xi++ )
          child[zi][yi][xi]->visit( visitor, leavesOnly );
  }
  else {
    visitor->visit( this );
  }
}

void OctreeNode::cullFaces( OctreeNode *node )
{
  if ( this == node ) return;
#if 0
  if ( mType == Block::SplitNode ) {
    for ( int zi = 0; zi<2; zi++ )
      for ( int yi = 0; yi<2; yi++ )
        for ( int xi = 0; xi<2; xi++ )
          child[zi][yi][xi]->cullFaces( node );
  }
  else {
    




  }
#endif
}

// -----------------------------------------------------------------------------

class NodeInspector : public OctreeVisitor
{
  public: virtual void visit( OctreeNode *node );
};

void NodeInspector::visit( OctreeNode *node )
{
  cout << "Leaf: "
       << node->xPos() << " "
       << node->yPos() << " "
       << node->zPos() << " "
       << node->xSize() << " "
       << node->ySize() << " "
       << node->zSize() << " "
       << node->type() << "\n";
}

class NodeDrawer : public OctreeVisitor
{
  int mCount;

  public:
    NodeDrawer() : mCount(0) { /* */ };
    virtual void visit( OctreeNode *node );
    int count() const { return mCount; };
};

void NodeDrawer::visit( OctreeNode *node )
{
  if ( node->type() != Block::Dirt ) return;

  mCount += 1;

  float xp, yp, zp, xs, ys, zs;
  node->varDump( xp, yp, zp, xs, ys, zs );

  Vertex va( xp,    yp,    zp );
  Vertex vb( xp,    yp,    zp+zs );
  Vertex vc( xp,    yp+ys, zp );
  Vertex vd( xp,    yp+ys, zp+zs );
  Vertex ve( xp+xs, yp,    zp );
  Vertex vf( xp+xs, yp,    zp+zs );
  Vertex vg( xp+xs, yp+ys, zp );
  Vertex vh( xp+xs, yp+ys, zp+zs );

  Face tp( vc, vd, vh, vg );
  Face bt( va, ve, vf, vb );
  Face sd1( vh, vd, vb, vf );
  Face sd2( vc, vg, ve, va );
  Face sd3( vg, vh, vf, ve );
  Face sd4( vd, vc, va, vb );

  glBegin(GL_QUADS);
  if ( node->faceVisible(0) ) {
    glColor3f(0.0, 0.0, 1.0);
    tp.glUntexturedDraw();
  }
  if ( node->faceVisible(1) ) {
    glColor3f(0.0, 1.0, 1.0);
    bt.glUntexturedDraw();
  }
  if ( node->faceVisible(2) ) {
    glColor3f(0.0, 1.0, 0.0);
    sd1.glUntexturedDraw();
  }
  if ( node->faceVisible(3) ) {
    glColor3f(1.0, 1.0, 0.0);
    sd2.glUntexturedDraw();
  }
  if ( node->faceVisible(4) ) {
    glColor3f(1.0, 0.0, 0.0);
    sd3.glUntexturedDraw();
  }
  if ( node->faceVisible(5) ) {
    glColor3f(1.0, 0.0, 1.0);
    sd4.glUntexturedDraw();
  }
  glEnd();
}

// -----------------------------------------------------------------------------

class Map
{
  protected:
    OctreeNode *root;
    bool first;


  public:
    Map();
    virtual ~Map();

    void draw( Camera &camera, Texture &texture );
    Voxel &voxel( int x, int y, int z );
};

Map::Map()
{
  const int xs = 64;
  const int ys = 64;
  const int zs = 64;

  root = new OctreeNode( Block::Dirt, 0.0, 0.0, 0.0, xs, ys, zs );
  first = true;

  int a=0, d=0;

  for ( int zi = 0; zi<zs; zi++ ) {
    for ( int yi = 0; yi<ys; yi++ ) {
      for ( int xi = 0; xi<xs; xi++ ) {
        int chance = rand() % 1000;
        if ( chance==0 ) a++; else d++;
        OctreeNode *child = root->setChild( ( chance == 0 ) ? Block::Air : Block::Dirt, xi, yi, zi );

        root->cullFaces( child );
      }
    }
  }

  cout << "air: " << a << "  dirt: " << d << "\n";
}

Map::~Map()
{
  delete root;
}

void Map::draw( Camera &camera, Texture &texture )
{
  // texture.bind();

  NodeDrawer drawee;
  root->visit( &drawee );

  if ( first ) {
    first = false;
    cout << "Draw Count: " << drawee.count() << "\n";
    // NodeInspector inspector;
    // root->visit( &inspector );
  }
}

Voxel &Map::voxel( int x, int y, int z )
{
#if 0
  int cx = x / Chunk::XSIZE;
  int cy = y / Chunk::YSIZE;
  int cz = z / Chunk::ZSIZE;

  if ( cx < 0 || cx >= XSIZE ||
       cy < 0 || cy >= YSIZE ||
       cz < 0 || cz >= ZSIZE )
  return Voxel::shared;

  int vx = x % Chunk::XSIZE;
  int vy = y % Chunk::YSIZE;
  int vz = z % Chunk::ZSIZE;
  return chunks[cz][cy][cx]->voxel(vx, vy, vz);
#endif
  return Voxel::shared;
}

// -----------------------------------------------------------------------------

class Clock
{
  protected:
    static const int HISIZE = 1024;
    float history[HISIZE];
    int last;
    int current;
    string fps;

    static void hline( float x1, float x2, float y );

  public:
    Clock();
    void update();
    void draw();
    float delta() const;
    const string &fpsStr() const { return fps; };
};

Clock::Clock()
{
  for ( int i = 0; i < HISIZE; i++ )
    history[i] = 0.0;
  current = SDL_GetTicks();
  last = current;
}

void Clock::update()
{
  current = SDL_GetTicks();
  for ( int i = HISIZE-1; i > 0; i-- )
    history[i] = history[i-1];
  history[0] = float(current - last) / 1000.0f;
  last = current;

  stringstream ss;
  ss << "FPS: " << (1.0 / history[0]);
  fps = ss.str();
}

float Clock::delta() const
{
  return history[0];
}

void Clock::hline( float x1, float x2, float y )
{
  glVertex2f( x1, y );
  glVertex2f( x2, y );
}

void Clock::draw()
{
  glDisable(GL_TEXTURE_2D);

  glBegin(GL_LINES);

  glColor3f(0.0, 1.0, 0.5);
  hline( 0.0, 200.0, 480.0 - (200.0/30.0) );

  glColor3f(1.0, 0.5, 0.0);
  hline( 0.0, 200.0, 480.0 - (200.0/15.0) );

  for ( int i = 0; i < HISIZE; i++ ) {
    const float p = (float(i) / float(HISIZE)) * 200.0;
    glColor3f(0.0, 1.0, 0.0);
    glVertex2f( p, 480.0 );

    glColor3f(1.0*history[i], 1.0, 0.0);
    glVertex2f( p, 480.0 - (200.0*history[i]) );
  }

  glEnd();
}

// -----------------------------------------------------------------------------

void set2DScreen( float width, float height )
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, width, height, 0, 1, -1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
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

  glClearColor(0.75, 0.75, 0.75, 0);
  glViewport(0, 0, 640, 480);

  glEnable(GL_TEXTURE_2D);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);

  SDL_WM_GrabInput( SDL_GRAB_ON );
  SDL_ShowCursor( SDL_DISABLE );

  return screen;
}

void render( Camera &camera, Map &map, Clock &clock, Texture &texture )
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  camera.set3DPerspective( 45.0f, 640.0 / 480.0 );
  camera.adjustGL();
  camera.readyFrustum();
  map.draw( camera, texture );

  set2DScreen( 640.0, 480.0 );

  clock.draw();

  SDL_GL_SwapBuffers();
}

void gameloop()
{
  Texture texture("tiles.png");
  if ( !texture.valid ) {
    cout << "where's tiles.png?\n";
    return;
  }

  cout << "GL_VERSION: " << glGetString(GL_VERSION) << "\n";
  cout << "GL_EXTENSIONS: " << glGetString(GL_EXTENSIONS) << "\n";

  SDL_Event event;
  Player player( -1.0, 1.5, -1.0, 0.0, -180.0 );

  cout << "Loading map" << "\n";
  Map map;

  cout << "Starting Clock" << "\n";
  Clock clock;

  player.setFog( true );

  while (true)
  {
    while ( SDL_PollEvent( &event ) )
    {
      switch ( event.type ) {
        case SDL_QUIT: return;

        case SDL_KEYDOWN:
          switch ( event.key.keysym.sym ) {
            case SDLK_f:
              player.setFog( !player.fogEnabled() );
              break;
            case SDLK_d: {
              int vd = int( player.viewDistance() );
              vd += 20;
              if ( vd > 200 ) vd = 20;
              player.setViewDistance( float(vd) );
              player.setFog( player.fogEnabled() );
              cout << "view distance: " << vd << "\n";
              break;
            }
            case SDLK_F10: return;
            case SDLK_F3:
              player.inspect();
              cout << clock.fpsStr() << "\n";
              break;
            case SDLK_F6:
            //player.teleport( float(Map::XSIZE * Chunk::XSIZE) / 2.0,
            //                 float(Map::YSIZE * Chunk::YSIZE) / 2.0 + 1.5,
            //                 float(Map::ZSIZE * Chunk::ZSIZE) / 2.0 );
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

    clock.update();
    player.update( clock.delta() );
    render( player, map, clock, texture );
    SDL_Delay( 10 );
  }
}

class App
{
  public:
    bool valid;

    App()
    {
      valid = (SDL_Init(SDL_INIT_EVERYTHING) >= 0);
    };

    ~App()
    {
      if (valid) SDL_Quit();
    };

    void run()
    {
      if (!valid) return;
      SDL_Surface *screen = setupScreen();
      if (screen) {
        cout << "begin\n";
        gameloop();
        cout << "end\n";
      }
    }
};

int main( int argc, char **argv )
{
  srand( time(0) );
  // cout << "chunk size: " << sizeof(Chunk) << "\n";
  App app;
  app.run();
  return (app.valid) ? 0 : 1;
}

