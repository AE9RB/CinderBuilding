#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CinderBuildingApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void CinderBuildingApp::setup()
{
}

void CinderBuildingApp::mouseDown( MouseEvent event )
{
}

void CinderBuildingApp::update()
{
}

void CinderBuildingApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( CinderBuildingApp, RendererGl )
