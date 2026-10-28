#include <qt_application.h>
#include <qt_mainwindow.h>
#include <cur_contour.h>
#include <action_solid.h>

#include <list>

const double sideSize = 10.0;
const double sideOffset = 2.0;

MathRepresentation* createMathParallelepiped(const Color& color, double opacity) 
{
    MathRepresentation* parallelepiped = new MathRepresentation();
    parallelepiped->SetBuildType(CommandType::Synchronous);
    MbContour* pContour2D = new MbContour();

    MbCartPoint p1(0.0, 0.0), p2(sideOffset, sideSize), p3(sideSize + sideOffset, sideSize), p4(sideSize, 0.0), p5(0.0, 0.0);
    SPtr<MbLineSegment> seg1(new MbLineSegment(p1, p2));
    SPtr<MbLineSegment> seg2(new MbLineSegment(p2, p3));
    SPtr<MbLineSegment> seg3(new MbLineSegment(p3, p4));
    SPtr<MbLineSegment> seg4(new MbLineSegment(p4, p5));

    pContour2D->AddSegment(seg1);
    pContour2D->AddSegment(seg2);
    pContour2D->AddSegment(seg3);
    pContour2D->AddSegment(seg4);

    MbPlacement3D placement;
    MbMatrix3D relativeMatrix; relativeMatrix.Rotate(AXIS_0X, -M_PI_2);
    placement.Init(relativeMatrix);

    MbSweptData pCurves(placement, *pContour2D);
    MbVector3D extDir(0.0, 1.0, 0.0);
    ExtrusionValues pExtParams(sideSize * 5.0, 0.0);

    MbSNameMaker names(1, MbSNameMaker::i_SideNone, 0);
    PArray<MbSNameMaker> contourNames(0, 1, false);

    MbSolid* pResSolid = nullptr;
    bool result = ExtrusionSolid(pCurves, extDir, nullptr, nullptr, false, pExtParams, names, contourNames, pResSolid);
    if (result) 
    {
        parallelepiped->SetMathItem(static_cast<MbItem*>(pResSolid));
        if (MathGeometry* pGeometry = vobject_cast<MathGeometry*>(parallelepiped->GetGeomAt(0))) 
        {
            if (pGeometry->GetMaterialCount() > 0) 
            {
                std::list<NodeKey> ids = pGeometry->GetMaterialIds();
                auto id = (*ids.begin());
                Material* material = pGeometry->GetMaterialById(id);
                Q_ASSERT(material != nullptr);
                material->SetDiffuse(Color(255, 0, 0));
                material->SetOpacity(opacity);
            }
        }
    }

    return parallelepiped;
}

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

