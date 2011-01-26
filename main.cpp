// made by inny

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <list>
#include <vector>
#include <map>
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
    bool frustumContainsSphere( float x, float y, float z, float radius );
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

  /* Normalize the results */
  for ( int i = 0; i<6; i++ ) {
    float t = sqrt( frustum[i][0]*frustum[i][0] + frustum[i][1]*frustum[i][1] + frustum[i][2]*frustum[i][2] );
    frustum[i][0] /= t;
    frustum[i][1] /= t;
    frustum[i][2] /= t;
    frustum[i][3] /= t;
  }
}

bool Camera::frustumContainsPoint( float x, float y, float z )
{
  for ( int p = 0; p < 6; p++ )
    if( frustum[p][0]*x + frustum[p][1]*y + frustum[p][2]*z + frustum[p][3] <= 0 )
      return false;

  return true;
}

bool Camera::frustumContainsSphere( float x, float y, float z, float radius )
{
  for( int p = 0; p < 6; p++ )
    if( (frustum[p][0]*x + frustum[p][1]*y + frustum[p][2]*z + frustum[p][3]) <= -radius )
      return false;

  return true;
}

bool Camera::frustumContainsCube( float x, float y, float z, float xs, float ys, float zs )
{
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
  unsigned short type;
  unsigned char visible;

  public:
    static const float SIZE = 1.0f;

    Voxel(int t=0): type(t), visible(255) { /* */ }
    void draw( vector<Face> &drawList, float x, float y, float z, float s );
    void cull( Voxel *vox[6] );

    static Voxel shared;
    bool isShared() const { return this == &shared; };

    bool isTransparent() const { return type==0; };
    bool isVisible(int x) const { return (visible&(1<<x)); };

  protected:
    void tagVisible(int x) { visible |= (1<<x); };
    void clearVisible(int x) { visible &= ~(1<<x); };
    void setVisible(int x, bool t) { if (isShared()) return; if (t) tagVisible(x); else clearVisible(x); };
}
__attribute__((__packed__));

Voxel Voxel::shared;

void Voxel::draw( vector<Face> &drawList, float x, float y, float z, float s )
{
  if (visible == 0) return;

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

  if (isVisible(0)) drawList.push_back( Face( vc, vd, vh, vg, top ) );
  if (isVisible(1)) drawList.push_back( Face( va, ve, vf, vb, bottom ) );
  if (isVisible(2)) drawList.push_back( Face( vh, vd, vb, vf, side ) );
  if (isVisible(3)) drawList.push_back( Face( vc, vg, ve, va, side ) );
  if (isVisible(4)) drawList.push_back( Face( vg, vh, vf, ve, side ) );
  if (isVisible(5)) drawList.push_back( Face( vd, vc, va, vb, side ) );
}

void Voxel::cull( Voxel *vox[6] )
{
  for ( int i=0; i<6; i++ ) {
    setVisible( i, vox[i]->isTransparent() );
  }

#if 0
  vox[0]->setVisible( 1, isTransparent() );
  vox[1]->setVisible( 0, isTransparent() );
  vox[2]->setVisible( 3, isTransparent() );
  vox[3]->setVisible( 2, isTransparent() );
  vox[4]->setVisible( 5, isTransparent() );
  vox[5]->setVisible( 4, isTransparent() );
#endif
}

// -----------------------------------------------------------------------------

class World;

class Chunk
{
  public:
    static const int XSIZE = 16;
    static const int YSIZE = 16;
    static const int ZSIZE = 16;
    static const float RADIUS = 16.0 * 0.866025404f;

  protected:
    World *world;
    float xpos;
    float ypos;
    float zpos;

    Voxel data[ZSIZE][YSIZE][XSIZE];

    bool generated;
    GLuint index;
    int drawn;

    void generateDisplayList();
    void drawChunkCube();

  public:
    Chunk( World *w, float x=0.0, float y=0.0, float z=0.0 );
    virtual ~Chunk();
    void randomize();
    void cullFaces();

    void draw( Camera &camera, int drawCount );
    Voxel &voxel( int x, int y, int z );
    int lastDrawn() const { return drawn; };

    static bool visibleToCamera( Camera &camera, float xpos, float ypos, float zpos );
};

// -----------------------------------------------------------------------------

class World
{
  public:
    static const int XSIZE = 5;
    static const int YSIZE = 5;
    static const int ZSIZE = 5;

  protected:
    typedef map<unsigned int, Chunk*> ChunkMap;
    ChunkMap chunkMap;

    list<Vertex> chunkLoadList;
    int chunksLoaded;
    bool messageDrop;
    int drawCount;

    static int hash( float x, float y, float z, bool &valid );
    Chunk *getChunk( float x, float y, float z );
    bool addChunk( Chunk *c, float x, float y, float z );
    Chunk *removeChunk( float x, float y, float z );
    void clearChunks();
    static bool fitsBounds( float x, float y, float z );

  public:
    World();
    virtual ~World();

    void draw( Camera &camera, Texture &texture );
    void update( float dt );

    Voxel &voxel( int x, int y, int z );
    void dropMessage() { messageDrop = true; };
};

// -----------------------------------------------------------------------------

Chunk::Chunk( World *w, float x, float y, float z )
  : world(w), xpos(x), ypos(y), zpos(z), generated( false ), drawn(0)
{
  /* */
}

Chunk::~Chunk()
{
  if (generated) glDeleteLists(index, 1);
}

void Chunk::randomize()
{
  float xc = 0.5 * float(World::XSIZE * Chunk::XSIZE);
  float yc = 0.5 * float(World::YSIZE * Chunk::YSIZE);
  float zc = 0.5 * float(World::ZSIZE * Chunk::ZSIZE);
  for ( int z = 0; z < Chunk::ZSIZE; z ++ ) {
    for ( int y = 0; y < Chunk::YSIZE; y ++ ) {
      for ( int x = 0; x < Chunk::XSIZE; x ++ ) {
        float xp = xpos + float(x) + 0.5*Voxel::SIZE;
        float yp = ypos + float(y) + 0.5*Voxel::SIZE;
        float zp = zpos + float(z) + 0.5*Voxel::SIZE;

        float vect = sqrt( xc*xc + yc*yc + zc*zc );
        float dist = sqrt( (xp-xc)*(xp-xc) + (yp-yc)*(yp-yc) + (zp-zc)*(zp-zc) );

        float chance = 1.0;

        if ( (abs(yp-(yc/2)) < 5.0) && ((abs(xp-xc) < 5.0) ||(abs(zp-zc) < 5.0)) )
          chance = 0.001;

        else if (yp > yc)
          chance = (yp < (sin((xp)/90.0*M_PI)*yc*2)) ? 1.0 : 0.001;

        data[z][y][x].type = ( rand() % 1000 < int(chance*1000.0) );
      }
    }
  }

#if 0
  for ( int z = 0; z < 2; z++ )
    for ( int y = 0; y < 2; y++ )
      for ( int x = 0; x < 2; x++ )
        data[z*(Chunk::ZSIZE-1)][y*(Chunk::YSIZE-1)][x*(Chunk::XSIZE-1)]=1;
#endif
}

void Chunk::cullFaces()
{
  if (generated) {
    glDeleteLists(index, 1);
    generated = false;
  }

  for ( int z = 0; z < Chunk::ZSIZE; z++ ) {
    for ( int y = 0; y < Chunk::YSIZE; y++ ) {
      for ( int x = 0; x < Chunk::XSIZE; x++ ) {
        Voxel &vox = voxel(x, y, z);
        Voxel *voxes[6];
        voxes[0] = &voxel(x,   y+1, z  );
        voxes[1] = &voxel(x,   y-1, z  );
        voxes[2] = &voxel(x,   y,   z+1);
        voxes[3] = &voxel(x,   y,   z-1);
        voxes[4] = &voxel(x+1, y,   z  );
        voxes[5] = &voxel(x-1, y,   z  );
        vox.cull( voxes );
      }
    }
  }
}

Voxel &Chunk::voxel( int x, int y, int z )
{
  if ( x < 0 || x >= Chunk::XSIZE ||
       y < 0 || y >= Chunk::YSIZE ||
       z < 0 || z >= Chunk::ZSIZE ) {
    return world->voxel( int(xpos)+x, int(ypos)+y, int(zpos)+z );
  }

  return data[z][y][x];
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

bool Chunk::visibleToCamera( Camera &camera, float xpos, float ypos, float zpos )
{
  return camera.frustumContainsSphere(xpos+(Chunk::XSIZE/2),
                                      ypos+(Chunk::YSIZE/2),
                                      zpos+(Chunk::ZSIZE/2),
                                      Chunk::RADIUS);
}

void Chunk::draw( Camera &camera, int drawCount )
{
  if (drawCount == drawn) return;
  drawn = drawCount;

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

// -----------------------------------------------------------------------------

World::World()
  : chunksLoaded(0), messageDrop(false), drawCount(0)
{
  /* */
}

World::~World()
{
  clearChunks();
}


bool World::fitsBounds( float x, float y, float z )
{
  return ( x >= 0.0 && (x < World::XSIZE*Chunk::XSIZE) &&
           y >= 0.0 && (y < World::YSIZE*Chunk::YSIZE) &&
           z >= 0.0 && (z < World::ZSIZE*Chunk::ZSIZE) );
}

int World::hash( float x, float y, float z, bool &valid )
{
  x = floor(x/Chunk::XSIZE);
  y = floor(y/Chunk::YSIZE);
  z = floor(z/Chunk::ZSIZE);

  if ( x < -8100.0 || x >= 8100.0 || z < -8100.0 || z >= 8100.0 ||
       y < 0.0 || y >= 8.0 ) {
    valid = false;
    return 0;
  }

  const unsigned int xi = int(x)+8100;
  const unsigned int yi = int(y);
  const unsigned int zi = int(z)+8100;
  const unsigned int id = (yi << 28) | (zi << 14) | xi;
  valid = true;
  return id;
}

Chunk *World::getChunk( float x, float y, float z )
{
  bool valid = false;
  const unsigned int id = hash( x, y, z, valid );
  if (!valid) return 0;

  ChunkMap::iterator finder = chunkMap.find(id);
  if ( finder == chunkMap.end() ) return 0;

  return finder->second;
}

bool World::addChunk( Chunk *c, float x, float y, float z )
{
  bool valid = false;
  const unsigned int id = hash( x, y, z, valid );
  if (!valid) return false;
  chunkMap[id] = c;
  return true;
}

Chunk *World::removeChunk( float x, float y, float z )
{
  bool valid = false;
  const unsigned int id = hash( x, y, z, valid );
  if (!valid) return 0;

  ChunkMap::iterator finder = chunkMap.find(id);
  if ( finder == chunkMap.end() ) return 0;

  Chunk *c = finder->second;
  chunkMap.erase(finder);
  return c;
}

void World::clearChunks()
{
  ChunkMap::iterator it;
  for ( it=chunkMap.begin(); it != chunkMap.end(); it++ ) {
    delete it->second;
  }
  chunkMap.clear();
}

void World::draw( Camera &camera, Texture &texture )
{
  drawCount += 1;

  const int xp = floor(camera.x()/Chunk::XSIZE)*Chunk::XSIZE;
  const int yp = floor(camera.y()/Chunk::YSIZE)*Chunk::YSIZE;
  const int zp = floor(camera.z()/Chunk::ZSIZE)*Chunk::ZSIZE;

  texture.bind();

  list<Vertex> drawList;
  drawList.push_back( Vertex(xp, yp, zp) );

  while ( !drawList.empty() )
  {
    Vertex v = drawList.front();
    drawList.pop_front();
    const int xi=v.x(), yi=v.y(), zi=v.z();

    if ( !Chunk::visibleToCamera(camera, xi, yi, zi) )
      continue;

    Chunk *chunk = getChunk( xi, yi, zi );
    if ( chunk ) {
      if ( chunk->lastDrawn() != drawCount ) {
        chunk->draw(camera, drawCount);
        drawList.push_back( Vertex(xi, yi-Chunk::YSIZE, zi) );
        drawList.push_back( Vertex(xi, yi+Chunk::YSIZE, zi) );
        drawList.push_back( Vertex(xi-Chunk::XSIZE, yi, zi) );
        drawList.push_back( Vertex(xi+Chunk::XSIZE, yi, zi) );
        drawList.push_back( Vertex(xi, yi, zi-Chunk::ZSIZE) );
        drawList.push_back( Vertex(xi, yi, zi+Chunk::ZSIZE) );
      }
    }
    else if (fitsBounds(xi, yi, zi)) {
      chunkLoadList.push_back( Vertex(xi, yi, zi) );
    }
  }

  messageDrop = false;
}

Voxel &World::voxel( int x, int y, int z )
{
  Chunk *chunk = getChunk( x, y, z );
  if (!chunk) return Voxel::shared;

  int vx = x % Chunk::XSIZE;
  int vy = y % Chunk::YSIZE;
  int vz = z % Chunk::ZSIZE;
  return chunk->voxel(vx, vy, vz);
}

void World::update( float dt )
{
  if ( chunksLoaded > (XSIZE * YSIZE * ZSIZE) )
    return;

  list<Chunk *> cullChunks;

  while (!chunkLoadList.empty()) {
    Vertex v = chunkLoadList.front();
    chunkLoadList.pop_front();

    if ( messageDrop ) cout << "ChunkLoad: " << v.x() << " " << v.y() << " " << v.z() << "\n";

    float xp = floor( v.x() / Chunk::XSIZE ) * Chunk::XSIZE;
    float yp = floor( v.y() / Chunk::YSIZE ) * Chunk::YSIZE;
    float zp = floor( v.z() / Chunk::ZSIZE ) * Chunk::ZSIZE;

    if ( !fitsBounds(xp, yp, zp) ) continue;

    Chunk *chunk = getChunk( xp, yp, zp );
    if (chunk) continue;

    chunk = new Chunk( this, xp, yp, zp );
    addChunk( chunk, xp, yp, zp );
    chunk->randomize();
    cullChunks.push_back(chunk);
    chunksLoaded++;

    Chunk *upChunk = getChunk( xp, yp+Chunk::YSIZE, zp );
    if (upChunk) cullChunks.push_back(upChunk);
    Chunk *dnChunk = getChunk( xp, yp-Chunk::YSIZE, zp );
    if (dnChunk) cullChunks.push_back(dnChunk);
    Chunk *noChunk = getChunk( xp, yp, zp+Chunk::ZSIZE );
    if (noChunk) cullChunks.push_back(noChunk);
    Chunk *soChunk = getChunk( xp, yp, zp-Chunk::ZSIZE );
    if (soChunk) cullChunks.push_back(soChunk);
    Chunk *weChunk = getChunk( xp+Chunk::XSIZE, yp, zp );
    if (weChunk) cullChunks.push_back(weChunk);
    Chunk *eaChunk = getChunk( xp-Chunk::XSIZE, yp, zp );
    if (eaChunk) cullChunks.push_back(eaChunk);

    break;
  }

  while (!cullChunks.empty()) {
    Chunk *chunk = cullChunks.front();
    cullChunks.pop_front();
    chunk->cullFaces();
  }
}

// -----------------------------------------------------------------------------

struct CollisionBlock
{
  float x1, y1, z1, x2, y2, z2;

  CollisionBlock() {/* */};
  CollisionBlock( float x, float y, float z, float xx, float yy, float zz )
    : x1(x), y1(y), z1(z), x2(xx), y2(yy), z2(zz) { /* */ };
  void set( float x, float y, float z, float xx, float yy, float zz );
  bool overlaps( const CollisionBlock &other ) const;
  void translate( float x, float y, float z );
};

void CollisionBlock::set( float x, float y, float z, float xx, float yy, float zz )
{
  x1 = x; y1 = y; z1 = z;
  x2 = xx; y2 = yy; z2 = zz;
}

bool CollisionBlock::overlaps( const CollisionBlock &other ) const
{
  return !( (other.x1 > x2) || (other.x2 < x1) ||
            (other.y1 > y2) || (other.y2 < y1) ||
            (other.z1 > z2) || (other.z2 < z1) );
}

void CollisionBlock::translate( float x, float y, float z )
{
  x1 += x;
  x2 += x;
  y1 += y;
  y2 += y;
  z1 += z;
  z2 += z;
}

// -----------------------------------------------------------------------------

class Player : public Camera
{
  protected:
    World *world;
    float yvel;

    void physics( float dt, float oldx, float oldy, float oldz );
    void solveX( float dt, float oldx );
    void solveY( float dt, float oldy );
    void solveZ( float dt, float oldz );

  public:
    Player( World *w, float x=0.0, float y=0.0, float z=0.0, float xr=0.0, float yr=0.0 );
    void update( float dt );
    void inspect();
    void jump( float dy );

    static const float HEAD=0.25;
    static const float FEET=1.5;
    static const float WAIST=0.3;
};

Player::Player( World *w, float x, float y, float z, float xr, float yr )
  : Camera( x, y, z, xr, yr ),
    world(w),
    yvel(0.0)
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

  float oldx = xpos;
  float oldy = ypos;
  float oldz = zpos;

  if (keys[SDLK_UP]) walk( 1.0 * moveSpeed );
  if (keys[SDLK_DOWN]) walk( -1.0 * moveSpeed );
  if (keys[SDLK_LEFT]) strafe( -1.0 * moveSpeed );
  if (keys[SDLK_RIGHT]) strafe( 1.0 * moveSpeed );
  if (keys[SDLK_SPACE]) jump(5.0);

  if (mousex) turn( float(mousex) * lookSpeed );
  if (mousey) look( float(mousey) * lookSpeed );

  physics(dt, oldx, oldy, oldz);
}

void Player::jump( float dy )
{
  if ( (ypos-FEET)-floor(ypos-FEET) > 0.001 ) return;
  if ( yvel < 0.00001 && yvel > -0.00001 ) yvel += dy;
}

void Player::physics( float dt, float oldx, float oldy, float oldz )
{
  Voxel &underVox = world->voxel( xpos, ypos-FEET, zpos );

  if ( underVox.isTransparent() ) {
    // add gravity
    yvel -= (12.0 * dt);
  }

  // cap at terminal velocity
  if ( yvel < -30.0 ) yvel = -30.0;

  // apply momentum of falling
  if ( yvel > 0.00001 || yvel < -0.00001 ) {
    ascend( yvel * dt );
  }

//  solveX(dt, oldx);
//  solveZ(dt, oldz);
  solveY(dt, oldy);
}

void Player::solveY( float dt, float oldy )
{
  if ( ypos < 0.0 ) {
    // don't fall beneath the 0 layer
    ypos = 0.0;
    yvel = 0.0;
    return;
  }

  if ( ypos < oldy ) {
    CollisionBlock me( xpos-WAIST, ypos-FEET, zpos-WAIST,
                       xpos+WAIST, ypos+HEAD, zpos+WAIST );
    CollisionBlock block[4];
    block[0].set( floor(me.x1), floor(me.y1), floor(me.z1),
                  floor(me.x1)+1.0, floor(me.y1)+1.0, floor(me.z1)+1.0 );
    block[1].set( floor(me.x2), floor(me.y1), floor(me.z1),
                  floor(me.x2)+1.0, floor(me.y1)+1.0, floor(me.z1)+1.0 );
    block[2].set( floor(me.x1), floor(me.y1), floor(me.z2),
                  floor(me.x1)+1.0, floor(me.y1)+1.0, floor(me.z2)+1.0 );
    block[3].set( floor(me.x2), floor(me.y1), floor(me.z2),
                  floor(me.x2)+1.0, floor(me.y1)+1.0, floor(me.z2)+1.0 );
    for (int i=0; i<4; i++) {
      Voxel &vox = world->voxel( block[i].x1, block[i].y1, block[i].z1 );
      if ( !vox.isTransparent() ) {
        me.translate( 0, (block[i].y2-me.y1), 0 );
        ypos = me.y1+FEET;
        yvel = 0.0;
        break;
      }
    }
  }
  else if ( ypos > oldy ) {
    CollisionBlock me( xpos-WAIST, ypos-FEET, zpos-WAIST,
                       xpos+WAIST, ypos+HEAD, zpos+WAIST );
    CollisionBlock block[4];
    block[0].set( floor(me.x1), floor(me.y2), floor(me.z1),
                  floor(me.x1)+1.0, floor(me.y2)+1.0, floor(me.z1)+1.0 );
    block[1].set( floor(me.x2), floor(me.y2), floor(me.z1),
                  floor(me.x2)+1.0, floor(me.y2)+1.0, floor(me.z1)+1.0 );
    block[2].set( floor(me.x1), floor(me.y2), floor(me.z2),
                  floor(me.x1)+1.0, floor(me.y2)+1.0, floor(me.z2)+1.0 );
    block[3].set( floor(me.x2), floor(me.y2), floor(me.z2),
                  floor(me.x2)+1.0, floor(me.y2)+1.0, floor(me.z2)+1.0 );
    for (int i=0; i<4; i++) {
      Voxel &vox = world->voxel( block[i].x1, block[i].y1, block[i].z1 );
      if ( !vox.isTransparent() ) {
        me.translate( 0, -(me.y2-block[i].y1), 0 );
        ypos = me.y2-HEAD;
        yvel = 0.0;
        break;
      }
    }
  }
}

void Player::solveX( float dt, float oldx )
{
  if ( floor(xpos) == floor(oldx) )
    return;

  float x, clip;

  if ( xpos < oldx ) {
    x = floor(xpos-WAIST);
    clip = x+WAIST;
  }
  else {
    x = floor(xpos+WAIST);
    clip = x-WAIST;
  }

  Voxel &upVox = world->voxel( x, ypos, zpos );
  Voxel &dnVox = world->voxel( x, ypos-1.0, zpos );

  if ( upVox.isTransparent() && dnVox.isTransparent() )
    return;

  xpos = clip;
}

void Player::solveZ( float dt, float oldz )
{

}

void Player::inspect()
{
  cout << "PLAYER " << xpos << " " << ypos << " " << zpos
       << " " << xrot << " " << yrot << "\n";
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

void render( Camera &camera, World &world, Clock &clock, Texture &texture )
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  camera.set3DPerspective( 45.0f, 640.0 / 480.0 );
  camera.adjustGL();
  camera.readyFrustum();
  world.draw( camera, texture );

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

  SDL_Event event;

  cout << "Loading World" << "\n";
  World world;

  cout << "Starting Clock" << "\n";
  Clock clock;

  Player player( &world, -1.0, 1.5, -1.0, 0.0, -180.0 );

  // cout << "Generating Text" << "\n";
  // TextPainter text;

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
            case SDLK_PAGEUP: player.teleport( player.x(), player.y() + 8.0, player.z() ); break;
            case SDLK_PAGEDOWN: player.teleport( player.x(), player.y() - 8.0, player.z() ); break;
            case SDLK_F3:
              player.inspect();
              cout << clock.fpsStr() << "\n";
              world.dropMessage();
              break;
            case SDLK_F5:
              player.teleport( 1.0, World::YSIZE*Chunk::YSIZE, 1.0 );
              break;
            case SDLK_F6:
              player.teleport( float(World::XSIZE * Chunk::XSIZE) / 2.0,
                               float(World::YSIZE * Chunk::YSIZE) / 2.0 + 1.5,
                               float(World::ZSIZE * Chunk::ZSIZE) / 2.0 );
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
    world.update( clock.delta() );
    player.update( clock.delta() );
    render( player, world, clock, texture );
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
  cout << "chunk size: " << sizeof(Chunk) << "\n";
  App app;
  app.run();
  return (app.valid) ? 0 : 1;
}

