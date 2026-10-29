#include "Controller/Controller.h"
#include "View/Camera.h"
#include "View/Renderer/Render.h"
#include "View/View.h"

using namespace s21;

int main(int argc, char* argv[]) {
  ViewerController controller;
  SceneRenderer renderer;
  OrbitCamera camera;

  QtView view(argc, argv, controller, renderer, camera);
  view.run();

  return 0;
}