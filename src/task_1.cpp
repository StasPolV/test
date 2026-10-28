#include <qt_application.h>
#include <qt_mainwindow.h>

int main(int argc, char** argv)
{
    using namespace QtVision;

    auto app = QtApplication::createApplication(argc, argv, "Task1");
    if (!app)
        return -1;

    QtOpenGLSceneWidget scene;
    SceneSegment* rootSegment = scene.sceneContent()->GetRootSegment();
    Q_ASSERT(rootSegment != nullptr);
    QtVision::createProcessesCameraControls(scene.graphicsEngine());
    scene.viewport()->SetGradientBackgroundColour(topColor, bottomColor);
    scene.mainLight()->SetDoubleSided(true);
    scene.graphicsView()->SetSmoothTransition(true);

    scene.viewport()->GetCamera()->SetViewOrientation(Orientation::IsoXYZ);
    scene.viewport()->ZoomToFit(scene.sceneContent()->GetBoundingBox());
    

    setWindowPosition(scene);
    scene.show();
    return app->exec();
}

