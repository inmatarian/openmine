// made by inny

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "SDL.h"
#include "SDL_opengl.h"
#include "SDL_image.h"

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

    void adjustGL();

    void readyFrustum();
    bool frustumContainsPoint( float x, float y, float z );
    bool frustumContainsCube( float x, float y, float z, float size );
};

Camera::Camera( float x, float y, float z, float xr, float yr )
  : xpos(x), ypos(y), zpos(z), xrot(xr), yrot(yr), updated(true)
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

void Camera::adjustGL()
{
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

  turn( float(mousex) * lookSpeed );
  look( float(mousey) * lookSpeed );
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
        const Point &tx )
  { v[0]=va; v[1]=vb; v[2]=vc; v[3]=vd; t=tx; };

  void glDraw();
};

void Face::glDraw()
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

Voxel Voxel::shared;

// -----------------------------------------------------------------------------

class Map;

class Chunk
{
  public:
    static const int SIZE = 8;

  protected:
    Map *map;
    float xpos;
    float ypos;
    float zpos;

    Voxel data[SIZE][SIZE][SIZE];

    bool generated;
    GLuint index;

    void generateDisplayList();
    bool distantTo( Camera &camera );

  public:
    Chunk( Map *m, float x=0.0, float y=0.0, float z=0.0 );
    virtual ~Chunk();
    void randomize();
    void cullFaces();

    void draw( Camera &camera );
    Voxel &voxel( int x, int y, int z );
};

class Map
{
  public:
    static const int XSIZE = 13;
    static const int YSIZE = 13;
    static const int ZSIZE = 13;

  protected:
    Chunk *chunks[ZSIZE][YSIZE][XSIZE];

  public:
    Map();
    virtual ~Map();

    void draw( Camera &camera, Texture &texture );
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
  float xc = 0.5 * float(Map::XSIZE * Chunk::SIZE);
  float yc = 0.5 * float(Map::YSIZE * Chunk::SIZE);
  float zc = 0.5 * float(Map::ZSIZE * Chunk::SIZE);
  for ( int z = 0; z < SIZE; z ++ ) {
    for ( int y = 0; y < SIZE; y ++ ) {
      for ( int x = 0; x < SIZE; x ++ ) {
        float xp = xpos + float(x) + 0.5*Voxel::SIZE;
        float yp = ypos + float(y) + 0.5*Voxel::SIZE;
        float zp = zpos + float(z) + 0.5*Voxel::SIZE;

        float vect = xc*xc + yc*yc + zc*zc;
        float dist = (xp-xc)*(xp-xc) + (yp-yc)*(yp-yc) + (zp-zc)*(zp-zc);
        int value = int( dist / vect * 1000.0 );

        data[z][y][x].type = /*(yp<=(yc-1.0)) ||*/ ( (dist < (48.0*48.0)) && (rand() % 1000 < value));
      }
    }
  }
}

void Chunk::cullFaces()
{
  int xi = int(xpos);
  int yi = int(ypos);
  int zi = int(zpos);

  for ( int z = 0; z < SIZE; z ++ ) {
    for ( int y = 0; y < SIZE; y ++ ) {
      for ( int x = 0; x < SIZE; x ++ ) {
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
  if ( x < 0 || x >= SIZE ||
       y < 0 || y >= SIZE ||
       z < 0 || z >= SIZE )
    return Voxel::shared;

  return data[z][y][x];
}

bool Chunk::distantTo( Camera &camera )
{
  const float DIST = 80.0;

  float xp = xpos + 0.5*Chunk::SIZE;
  float yp = ypos + 0.5*Chunk::SIZE;
  float zp = zpos + 0.5*Chunk::SIZE;

  float cx = camera.x();
  float cy = camera.y();
  float cz = camera.z();
  
  float dist = (xp-cx)*(xp-cx) + (yp-cy)*(yp-cy) + (zp-cz)*(zp-cz);

  return ( dist > DIST*DIST );
}

void Chunk::draw( Camera &camera )
{
  if ( distantTo( camera ) )
    return;

  if ( !camera.frustumContainsCube(xpos, ypos, zpos, Voxel::SIZE*float(SIZE)) )
    return;

  if (!generated) generateDisplayList();

  glCallList(index);
}

void Chunk::generateDisplayList()
{
  generated = true;
  index = glGenLists(1);

  vector<Face> faceList;
  for ( int z = 0; z < SIZE; z ++ ) {
    for ( int y = 0; y < SIZE; y ++ ) {
      for ( int x = 0; x < SIZE; x ++ ) {
        if ( data[z][y][x].type > 0 ) {
          data[z][y][x].draw( faceList, float(xpos+x), float(ypos+y), float(zpos+z), Voxel::SIZE );
        }
      }
    }
  }

  glNewList(index, GL_COMPILE);
  glBegin(GL_QUADS);
    for ( int i = 0; i < faceList.size(); i++ ) faceList[i].glDraw();
  glEnd();
  glEndList();
}

// -----------------------------------------------------------------------------

Map::Map()
{
  for ( int z = 0; z < ZSIZE; z ++ ) {
    for ( int y = 0; y < YSIZE; y ++ ) {
      for ( int x = 0; x < XSIZE; x ++ ) {
        Chunk *chunk = new Chunk( this,
                                  float(x*Chunk::SIZE),
                                  float(y*Chunk::SIZE),
                                  float(z*Chunk::SIZE) );
        chunk->randomize();
        chunks[z][y][x] = chunk;
      }
    }
  }

  for ( int z = 0; z < ZSIZE; z ++ )
    for ( int y = 0; y < YSIZE; y ++ )
      for ( int x = 0; x < XSIZE; x ++ )
        chunks[z][y][x]->cullFaces();
}

Map::~Map()
{
  for ( int z = 0; z < ZSIZE; z ++ )
    for ( int y = 0; y < YSIZE; y ++ )
      for ( int x = 0; x < XSIZE; x ++ )
        delete chunks[z][y][x];
}

void Map::draw( Camera &camera, Texture &texture )
{
  texture.bind();

  for ( int z = 0; z < ZSIZE; z ++ )
    for ( int y = 0; y < YSIZE; y ++ )
      for ( int x = 0; x < XSIZE; x ++ )
        chunks[z][y][x]->draw( camera );
}

Voxel &Map::voxel( int x, int y, int z )
{
  int cx = x / Chunk::SIZE;
  int cy = y / Chunk::SIZE;
  int cz = z / Chunk::SIZE;

  if ( cx < 0 || cx >= XSIZE ||
       cy < 0 || cy >= YSIZE ||
       cz < 0 || cz >= ZSIZE )
  return Voxel::shared;

  int vx = x % Chunk::SIZE;
  int vy = y % Chunk::SIZE;
  int vz = z % Chunk::SIZE;
  return chunks[cz][cy][cx]->voxel(vx, vy, vz);
}

// -----------------------------------------------------------------------------

class Clock
{
  protected:
    static const int HISIZE = 1024;
    float history[HISIZE];
    int last;
    int current;

  public:
    Clock();
    void update();
    void draw();
    float delta() const;
};

Clock::Clock()
{
  for ( int i = 0; i < HISIZE; i++ )
    history[i] = 0.0;
  last = 0;
  current = SDL_GetTicks();
}

void Clock::update()
{
  current = SDL_GetTicks();
  for ( int i = HISIZE-1; i > 0; i-- )
    history[i] = history[i-1];
  history[0] = float(current - last) / 1000.0f;
  last = current;
}

float Clock::delta() const
{
  return history[0];
}

void Clock::draw()
{
  glDisable(GL_TEXTURE_2D);

  glBegin(GL_LINES);

  glColor3f(0.0, 1.0, 1.0);
  glVertex2f( 0.0, 480.0 - (200.0/30.0) );
  glVertex2f( 200.0, 480.0 - (200.0/30.0) );

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

void set3DPerspective(GLfloat fovy, GLfloat aspect, GLfloat zmin, GLfloat zmax)
{
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

void set2DScreen( float width, float height )
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, width, height, 0, 1, -1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void setFog()
{
  GLfloat fogColor[4]= {0.75f, 0.75f, 0.75f, 1.0f};
  glEnable(GL_FOG);
  glFogfv(GL_FOG_COLOR, fogColor);
  glFogi(GL_FOG_MODE, GL_LINEAR);
  glFogf(GL_FOG_START, 48.0f);
  glFogf(GL_FOG_END, 64.0f);
  glHint(GL_FOG_HINT, GL_DONT_CARE);  
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

  setFog();

  return screen;
}

void render( Camera &camera, Map &map, Clock &clock, Texture &texture )
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  set3DPerspective( 45.0f, 640.0 / 480.0, 0.1f, 500.0f );

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

  SDL_Event event;
  Player player( -1.0, 1.5, -1.0, 0.0, -180.0 );

  Map map;
  Clock clock;

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
            case SDLK_F6:
              player.teleport( float(Map::XSIZE * Chunk::SIZE) / 2.0,
                               float(Map::YSIZE * Chunk::SIZE) / 2.0,
                               float(Map::ZSIZE * Chunk::SIZE) / 2.0 );
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
  cout << "chunk size: " << sizeof(Chunk) << "\n";
  App app;
  app.run();
  return (app.valid) ? 0 : 1;
}

