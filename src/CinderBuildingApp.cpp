//  Copyright 2015 David Turnbull. All rights reserved.

// An exrecise for me to learn about Cinder's OpenGL.
// Tested with Cinder 0.9 on OS X 10.10.

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"
#include "boost/multi_array.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;


class CinderBuildingApp : public App {
    CameraPersp mCamera;
    params::InterfaceGlRef mParams;
    float mFrameRate;
    int mTriangles;
    int mRotation;
    int mHeight;
    class Building *building;
public:
    void setup() override;
    void resize() override;
    void update() override;
    void draw() override;
};


class BuildingUV {
    int divisions;
    float div_width;
public:
    gl::Texture2dRef texture;
    BuildingUV (int divisions, gl::Texture2dRef texture);
    Rectf map (int i) const;
};


class Building {
    typedef boost::multi_array<uint8_t, 2> array2D;
    typedef boost::multi_array<float, 2> farray2D;
    shared_ptr<BuildingUV> uvmap;
    array2D *heights, *up, *left, *right, *forward, *back;
    ci::ivec3 size;
    float meshHeight;
    ci::gl::GlslProgRef shader;
    ci::gl::VertBatchRef vb;
    ci::gl::BatchRef batch;
    ci::mat4 modelMatrix;
    array2D& array2DfromString (std::string);
    void Build();
    bool voxelOccupied (int x, int y, int z);
    void Mesh(float height);
public:
    Building();
    int getNumTriangles();
    void setRotation(int degrees);
    void setHeight(float height);
    void draw();
};


CINDER_APP( CinderBuildingApp, RendererGl(RendererGl::Options().msaa(4)) )


void CinderBuildingApp::setup()
{
    building = new Building();
    mRotation = 0;
    mHeight = 100;
    mCamera.lookAt( vec3( 0, 2, 7 ), vec3( 0 ) );
    mParams = params::InterfaceGl::create( "Params", toPixels( ivec2( 200, 100 ) ) );
    mParams->addParam( "Frame rate", &mFrameRate, "", true );
    mParams->addParam( "Triangles", &mTriangles, "", true );
    mParams->addParam( "Rotation", &mRotation ).min(-360).max(360);
    mParams->addParam( "Height%", &mHeight ).min(0).max(100);
}


void CinderBuildingApp::resize()
{
    mCamera.setPerspective( 60.0f, getWindowAspectRatio(), 1.0f, 200.0f );
}


void CinderBuildingApp::update()
{
    mFrameRate	= getAverageFps();
    building->setRotation(mRotation);
    building->setHeight(mHeight/100.0f);
    mTriangles = building->getNumTriangles();
}


void CinderBuildingApp::draw()
{
    gl::clear(ColorA(0.2f, 0.3f, 0.3f, 1.0f));
    gl::enableDepthRead();
    gl::enableDepthWrite();
    gl::enableFaceCulling();
    gl::cullFace(GL_BACK);
    gl::setMatrices( mCamera );
    building->draw();
    mParams->draw();
}


BuildingUV::BuildingUV(int divisions, gl::Texture2dRef texture) :
    divisions(divisions), texture(texture)
{
    div_width = 1.0f / divisions;
}


Rectf BuildingUV::map (int i) const {
    Rectf r;
    float x = (i % divisions) * div_width;
    float y = (i / divisions) * div_width;
    r.x1 = x;
    r.y1 = y;
    r.x2 = x+div_width;
    r.y2 = y+div_width;
    return r;
}


Building::Building () {
    auto image = Surface( loadImage( loadAsset( "buildingtex.png" ) ) );
    auto format = gl::Texture::Format()
                  .magFilter( GL_LINEAR )
                  .minFilter( GL_LINEAR_MIPMAP_LINEAR )
                  .maxAnisotropy( 16 )
                  .wrap( GL_CLAMP_TO_EDGE )
                  .target( GL_TEXTURE_2D )
                  .mipmap();

    uvmap = make_shared<BuildingUV>(4, gl::Texture::create(image, format));
    shader = gl::getStockShader( gl::ShaderDef().texture() );

    Build();
    this->vb = gl::VertBatch::create();
    Mesh(1.0f);
}


Building::array2D& Building::array2DfromString (string s) {
    array2D::index x = 0, y = 1, xtmp = 0;
    for (char c : s) {
        if (c == '/' || c == '\n') {
            if (y==1) x = xtmp;
            xtmp = 0;
            y++;
        } else {
            xtmp++;
        }
    }
    if (xtmp == 0) y--; // handles final/extra line termination
    boost::array<array2D::index, 2> shape = {{ x,y }};
    array2D* a = new array2D(shape);
    x = 0;
    y--;
    for (char c : s) {
        if (c == '/' || c == '\n') {
            x = 0;
            y--;
        } else {
            array2D::element b = c - '0';
            if (b >= 49) b -= 39; // alpha a:f = 10:15;
            if (b >= 17) b -= 7; // alpha A:F = 10:15;
            (*a)[x][y] = b;
            x++;
        }
    }
    return *a;
}


void Building::Build () {

    // The idea is to design many types of buildings and give them
    // variety from a small amount of textures. These strings will
    // eventually come from a configration file.
    // WARNING: All this will leak if you destroy the class.

    heights = &array2DfromString (
                  "0333300/"
                  "4333300/"
                  "0333311/"
                  "0333311/"
              );

    up = &array2DfromString (
             "0eeee00/"
             "feeee00/"
             "0eeeeff/"
             "0eeeeff/"
         );

    back = &array2DfromString (
               "b000000/"
               "7898900/"
               "7466500/"
               "7223100/"
           );


    forward = &array2DfromString (
                  "000000b/"
                  "0088887/"
                  "0044447/"
                  "0000007/"
              );

    left = &array2DfromString (
               "0b00/"
               "8789/"
               "4745/"
               "2721/"
           );


    right = &array2DfromString (
                "00b0/"
                "8888/"
                "4444/"
                "0000/"
            );

    size.x = heights->shape()[0];
    size.z = heights->shape()[1];
    size.y = 0;
    for (int x = 0; x < size.x; x++) {
        for (int z = 0; z < size.z; z++) {
            int y = (*heights)[x][z];
            if (size.y < y) size.y = y;
        }
    }
}


bool Building::voxelOccupied (int x, int y, int z) {
    if (y >= meshHeight * size.y) return false;
    if (x < 0 || x >= size.x) return false;
    if (y < 0 || y >= size.y) return false;
    if (z < 0 || z >= size.z) return false;
    if ((*heights)[x][z] <= y) return false;
    return true;
}


void Building::Mesh(float height) {
    // Looks like there's no indices in VertBatch.
    // Should have used TriMesh. Oh well, next time.
    Rectf uvrect;
    meshHeight = height;
    vb->clear();
    vb->begin(GL_TRIANGLES);
    for (int x = 0; x < size.x; x++) {
        for (int y = 0; y < size.y; y++) {
            float yy = (meshHeight * size.y) - y;
            if (yy > 1.0f) yy = 1.0f;
            for (int z = 0; z < size.z; z++) {
                if (!voxelOccupied(x,y,z)) continue;
                // up
                if (!voxelOccupied(x,y+1,z)) {
                    uvrect = uvmap->map ((*up)[x][z]);
                    vb->texCoord(uvrect.x1, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y2);
                    vb->texCoord(uvrect.x1, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y2);
                    vb->texCoord(uvrect.x1, uvrect.y2);
                    vb->vertex(x,y+yy,-z);
                    vb->vertex(x+1,y+yy,-z);
                    vb->vertex(x+1,y+yy,-z-1);
                    vb->vertex(x,y+yy,-z);
                    vb->vertex(x+1,y+yy,-z-1);
                    vb->vertex(x,y+yy,-z-1);
                }
                // forward
                if (!voxelOccupied(x,y,z+1)) {
                    uvrect = uvmap->map ((*forward)[size.x-x-1][y]);
                    uvrect.y2 = lerp(uvrect.y1, uvrect.y2, yy);
                    vb->texCoord(uvrect.x1, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y2);
                    vb->texCoord(uvrect.x1, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y2);
                    vb->texCoord(uvrect.x1, uvrect.y2);
                    vb->vertex(x+1,y,-z-1);
                    vb->vertex(x,y,-z-1);
                    vb->vertex(x,y+yy,-z-1);
                    vb->vertex(x+1,y,-z-1);
                    vb->vertex(x,y+yy,-z-1);
                    vb->vertex(x+1,y+yy,-z-1);
                }
                // back
                if (!voxelOccupied(x,y,z-1)) {
                    uvrect = uvmap->map ((*back)[x][y]);
                    uvrect.y2 = lerp(uvrect.y1, uvrect.y2, yy);
                    vb->texCoord(uvrect.x1, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y2);
                    vb->texCoord(uvrect.x1, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y2);
                    vb->texCoord(uvrect.x1, uvrect.y2);
                    vb->vertex(x,y,-z);
                    vb->vertex(x+1,y,-z);
                    vb->vertex(x+1,y+yy,-z);
                    vb->vertex(x,y,-z);
                    vb->vertex(x+1,y+yy,-z);
                    vb->vertex(x,y+yy,-z);
                }
                // left
                if (!voxelOccupied(x-1,y,z)) {
                    uvrect = uvmap->map ((*left)[size.z-z-1][y]);
                    uvrect.y2 = lerp(uvrect.y1, uvrect.y2, yy);
                    vb->texCoord(uvrect.x1, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y2);
                    vb->texCoord(uvrect.x1, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y2);
                    vb->texCoord(uvrect.x1, uvrect.y2);
                    vb->vertex(x,y,-z-1);
                    vb->vertex(x,y,-z);
                    vb->vertex(x,y+yy,-z);
                    vb->vertex(x,y,-z-1);
                    vb->vertex(x,y+yy,-z);
                    vb->vertex(x,y+yy,-z-1);
                }
                // right
                if (!voxelOccupied(x+1,y,z)) {
                    uvrect = uvmap->map ((*right)[z][y]);
                    uvrect.y2 = lerp(uvrect.y1, uvrect.y2, yy);
                    vb->texCoord(uvrect.x1, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y2);
                    vb->texCoord(uvrect.x1, uvrect.y1);
                    vb->texCoord(uvrect.x2, uvrect.y2);
                    vb->texCoord(uvrect.x1, uvrect.y2);
                    vb->vertex(x+1,y,-z);
                    vb->vertex(x+1,y,-z-1);
                    vb->vertex(x+1,y+yy,-z-1);
                    vb->vertex(x+1,y,-z);
                    vb->vertex(x+1,y+yy,-z-1);
                    vb->vertex(x+1,y+yy,-z);
                }
            }
        }
    }
    vb->end();
    if (meshHeight > FLT_EPSILON) {
        batch = gl::Batch::create( *vb, shader );
    }
}


int Building::getNumTriangles() {
    if (meshHeight <= FLT_EPSILON) return 0;
    return batch->getNumVertices() / 3;
}


void Building::setRotation(int degrees) {
    modelMatrix = mat4();
    modelMatrix = rotate(modelMatrix, glm::radians((float)degrees), vec3(0.0f,1.0f,0.0f));
    modelMatrix = translate(modelMatrix, vec3(-size.x/2.0f,-size.y/2.0f+0.5f,size.z/2.0f));
}


void Building::setHeight(float height) {
    if (meshHeight != height) Mesh(height);
}


void Building::draw() {
    gl::ScopedTextureBind texScope(uvmap->texture);
    gl::ScopedModelMatrix modelScope;
    gl::setModelMatrix(modelMatrix);
    if (meshHeight > FLT_EPSILON) batch->draw();
}
