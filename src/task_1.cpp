#include <qt_application.h>
#include <qt_mainwindow.h>
#include <cur_contour.h>
#include <action_solid.h>

#include <list>

const double sideSize = 10.0;
const double sideOffset = 2.0;

c3d::SolidSPtr makeParallelepiped()
{
    SPtr<MbContour> pContour2D(new MbContour());

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
    ExtrusionValues pExtParams(sideSize * 2.0, 0.0);

    MbSNameMaker names(1, MbSNameMaker::i_SideNone, 0);
    PArray<MbSNameMaker> contourNames(0, 1, false);

    MbSolid* pResSolid = nullptr;
    MbResultType result = ExtrusionSolid(pCurves, extDir, nullptr, nullptr, false, pExtParams, names, contourNames, pResSolid);
    if (result != rt_Success)
        return nullptr;

    return SPtr<MbSolid>(pResSolid);
}

c3d::SolidSPtr makeCylinder()
{
    using namespace c3d;

    SpacePointsVector points;
    points.reserve(3);
    points.push_back(MbCartPoint3D(0.0, 0.0, 0.0));
    points.push_back(MbCartPoint3D(0.0, 25.0, 0.0));
    points.push_back(MbCartPoint3D(3.0, 0.0, 0.0));

    MbSNameMaker names(2, MbSNameMaker::i_SideNone, 0);

    MbElementarySolidParams params(ElementaryShellType::et_Cylinder, points, names);
    MbElementarySolidResults results;
    if (ElementarySolid(params, results) != rt_Success)
        return nullptr;

    SolidSPtr pResSolid = results.GetResultSolid();
    pResSolid->Move(MbVector3D(5.0f, 0.0f, -5.0f));
    
    return pResSolid;
}

c3d::SolidSPtr makeBooleanOperation(const c3d::SolidSPtr& solid1, const c3d::SolidSPtr& solid2, OperationType opType)
{
    using namespace c3d;
    if (!solid1 || !solid2 || !solid1->IsClosed() || !solid2->IsClosed())
        return nullptr;

    MbSNameMaker operNames(ct_BooleanSolid, MbSNameMaker::i_SideNone, 0);

    SolidSPtr pResSolid;
    MbResultType res = BooleanResult(solid1, cm_Copy, solid2, cm_Copy, MbBooleanOperationParams(bo_Difference, true, operNames), pResSolid);
    if (res != rt_Success) 
        return nullptr;

    return pResSolid;
}

c3d::SolidSPtr makeFillet(const c3d::SolidSPtr& solid, double radius) 
{
    using namespace c3d;
    if (!solid)
        return nullptr;

    c3d::EdgesSPtrVector edges;
    solid->GetEdges(edges);

    FacesSPtrVector bounds;

    SmoothValues params;
    params.InitEquidistantFillet(radius);

    MbSNameMaker names(ct_FilletSolid, MbSNameMaker::i_SideNone, 0);

    SolidSPtr pResSolid;
    if (FilletSolid(*solid, cm_Copy, MbShellFilletValues(edges, bounds, params, names), pResSolid) != rt_Success)
        return nullptr;

    return pResSolid;
}

void createObject(SceneSegment* pRootSegment, const c3d::SolidSPtr& solid, const Matrix3DF& moveRoot, const Color& color, double opacity) 
{
    if (!solid)
        return;

    MathRepresentation* pMathRep = new MathRepresentation();
    pMathRep->SetBuildType(CommandType::Synchronous);
    pMathRep->SetMathItem(solid.get());
    pMathRep->SetVisualSag(0.001, true);

    if (MathGeometry* pGeometry = vobject_cast<MathGeometry*>(pMathRep->GetGeomAt(0)))
    {
        if (pGeometry->GetMaterialCount() > 0)
        {
            std::list<NodeKey> ids = pGeometry->GetMaterialIds();
            auto id = (*ids.begin());
            Material* material = pGeometry->GetMaterialById(id);
            Q_ASSERT(material != nullptr);
            material->SetDiffuse(color);
            material->SetOpacity(opacity);
        }
    }

    SceneSegment* pSegment = new SceneSegment(pRootSegment);
    SegmentInstance* pSegmentData = new SegmentInstance(pMathRep);
    SceneSegment* pSceneSeg = new SceneSegment(pSegmentData);
    pSceneSeg->GetTransform().SetMatrix(moveRoot);
    pSegment->AddSegment(pSceneSeg);
}

int main(int argc, char** argv)
{
    using namespace QtVision;
    using namespace c3d;

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


    SolidSPtr parallelepiped = makeParallelepiped();
    Matrix3DF move1;
    move1.Move(VSN::Vector3DF(10.0f, 0.0f, 0.0f));
    createObject(rootSegment, parallelepiped, move1, VSN::Color(255, 0, 0), 1.0);

    SolidSPtr cylinder = makeCylinder();
    Matrix3DF move2;
    move2.Move(VSN::Vector3DF(25.0f, 0.0f, 0.0f));
    createObject(rootSegment, cylinder, move2, VSN::Color(255, 0, 0), 1.0);

    SolidSPtr boolean = makeBooleanOperation(parallelepiped, cylinder, bo_Difference);
    Matrix3DF move3;
    move3.Move(VSN::Vector3DF(40.0f, 0.0f, 0.0f));
    createObject(rootSegment, boolean, move3, VSN::Color(255, 165, 0), 1.0);

    Matrix3DF move4;
    move4.Move(VSN::Vector3DF(55.0f, 0.0f, 0.0f));
    createObject(rootSegment, makeFillet(cylinder, 1.0), move4, VSN::Color(0, 255, 0), 1.0);

    Matrix3DF move5;
    move5.Move(VSN::Vector3DF(70.0f, 0.0f, 0.0f));
    createObject(rootSegment, makeFillet(boolean, 0.5), move5, VSN::Color(0, 255, 0), 1.0);

    scene.viewport()->GetCamera()->SetViewOrientation(Orientation::IsoXYZ);
    scene.viewport()->ZoomToFit(scene.sceneContent()->GetBoundingBox());

    setWindowPosition(scene);
    scene.show();
    return app->exec();
}

