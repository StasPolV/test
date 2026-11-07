#include <qt_application.h>
#include <qt_mainwindow.h>
#include <cur_contour.h>
#include <action_solid.h>
#include <action_mesh.h>
#include <action_polymesh.h>
#include <mesh.h>
#include <mesh_polygon.h>
#include <mesh_primitive.h>
#include <polymesh.h>
#include <op_polymesh_parameter.h>

#include <action_curve3d.h>
#include <op_swept_parameter.h>
#include <op_shell_parameter.h>
#include <gce_api.h>
#include <QDebug>

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

c3d::SolidSPtr makeFillet(const c3d::SolidSPtr& solid, double radius, bool allEdges)
{
    using namespace c3d;
    if (!solid)
        return nullptr;

    auto outerEdges = [](const SolidSPtr& solid)
        {
            EdgesSPtrVector edges;
            solid->GetEdges(edges);

            EdgesSPtrVector outerEdges;
            for (const auto& edge : edges)
            {
                if (edge->IsSeam())
                    continue;

                const MbFace* f1 = edge->GetFacePlus();
                const MbFace* f2 = edge->GetFaceMinus();
                if (f1 == nullptr || f2 == nullptr)
                    continue;

                if (f1->IsPlanar() && f2->IsPlanar())
                    outerEdges.push_back(edge);
            }

            return outerEdges;
        };

    EdgesSPtrVector edges;
    if (allEdges)
        solid->GetEdges(edges);
    else
        edges = outerEdges(solid);

    FacesSPtrVector bounds;

    SmoothValues params;
    params.InitEquidistantFillet(radius);

    MbSNameMaker names(ct_FilletSolid, MbSNameMaker::i_SideNone, 0);

    SolidSPtr pResSolid;
    if (FilletSolid(*solid, cm_Copy, MbShellFilletValues(edges, bounds, params, names), pResSolid) != rt_Success)
        return nullptr;

    return pResSolid;
}

GCE_point gcePoint(double x, double y)
{
    GCE_point p;
    p.x = x;
    p.y = y;
    return p;
}

SPtr<MbContour> makeRhombus(double side, double angleDeg)
{
    GCE_system sys = GCE_CreateSystem();

    geom_item a = GCE_AddPoint(sys, gcePoint(0.0, 0.0));
    geom_item b = GCE_AddPoint(sys, gcePoint(side, 0.0));
    geom_item c = GCE_AddPoint(sys, gcePoint(side * 1.5, side));
    geom_item d = GCE_AddPoint(sys, gcePoint(side * 0.5, side));

    geom_item ab[2] = { a, b }, bc[2] = { b, c }, cd[2] = { c, d }, ad[2] = { a, d };
    geom_item segAB = GCE_AddLineSeg(sys, ab);
    geom_item segBC = GCE_AddLineSeg(sys, bc);
    geom_item segCD = GCE_AddLineSeg(sys, cd);
    geom_item segAD = GCE_AddLineSeg(sys, ad);

    GCE_FixGeom(sys, a);
    GCE_AddUnaryConstraint(sys, GCE_HORIZONTAL, segAB);

    GCE_dim_pars length;
    length.dimValue = side;
    GCE_AddLength(sys, segAB, length);

    GCE_AddEqualLength(sys, segAB, segAD);

    geom_item parallelABCD[2] = { segAB, segCD };
    GCE_AddParallel(sys, parallelABCD);

    geom_item parallelADBC[2] = { segAD, segBC };
    GCE_AddParallel(sys, parallelADBC);

    GCE_adim_pars angle;
    angle.dPars.dimValue = angleDeg * M_PI / 180.0;
    GCE_AddAngle(sys, segAB, segAD, angle);

    GCE_result result = GCE_Evaluate(sys);
    if (result != GCE_RESULT_Ok || GCE_DefinitionState(sys) != GCE_STATE_WellConstrained)
    {
        qDebug() << "Rhombus solver failed, result:" << static_cast<int>(result)
            << "state:" << static_cast<int>(GCE_DefinitionState(sys));
        GCE_RemoveSystem(sys);
        return nullptr;
    }

    GCE_point pa = GCE_GetPointXY(sys, a);
    GCE_point pb = GCE_GetPointXY(sys, b);
    GCE_point pc = GCE_GetPointXY(sys, c);
    GCE_point pd = GCE_GetPointXY(sys, d);
    GCE_RemoveSystem(sys);

    MbVector toCenter(-(pa.x + pc.x) * 0.5, -(pa.y + pc.y) * 0.5);
    MbCartPoint p1(pa.x, pa.y), p2(pb.x, pb.y), p3(pc.x, pc.y), p4(pd.x, pd.y);

    SPtr<MbContour> contour(new MbContour());
    contour->AddSegment(SPtr<MbLineSegment>(new MbLineSegment(p1, p2)));
    contour->AddSegment(SPtr<MbLineSegment>(new MbLineSegment(p2, p3)));
    contour->AddSegment(SPtr<MbLineSegment>(new MbLineSegment(p3, p4)));
    contour->AddSegment(SPtr<MbLineSegment>(new MbLineSegment(p4, p1)));
    contour->Move(toCenter);
    return contour;
}

const double helixRadius = 5.0;
const double helixHeight = 25.0;
const double helixStep = 5.0;

SPtr<MbCurve3D> makeHelix()
{
    MbCurve3D* pHelix = nullptr;
    MbResultType res = SpiralCurve(MbCartPoint3D(0.0, 0.0, 0.0),
        MbCartPoint3D(0.0, helixHeight, 0.0),
        MbCartPoint3D(1.0, 0.0, 0.0),
        helixRadius, helixStep, 0.0,
        nullptr, false, pHelix);
    if (res != rt_Success)
        return nullptr;

    return SPtr<MbCurve3D>(pHelix);
}

c3d::SolidSPtr makeHelixSweep(double side, double angleDeg)
{
    using namespace c3d;

    SPtr<MbContour> rhombus = makeRhombus(side, angleDeg);
    SPtr<MbCurve3D> helix = makeHelix();
    if (!rhombus || !helix)
        return nullptr;

    MbCartPoint3D start;
    helix->GetLimitPoint(1, start);
    MbPlacement3D profilePlace(start, MbVector3D(0.0, 0.0, 1.0), MbVector3D(1.0, 0.0, 0.0));
    MbSweptData sweptData(profilePlace, *rhombus);

    EvolutionValues params;

    MbSNameMaker operNames(ct_CurveEvolutionSolid, MbSNameMaker::i_SideNone, 0);
    RPArray<MbSNameMaker> contourNames(0, 1);
    MbSNameMaker spineNames(ct_CurveEvolutionSolid, MbSNameMaker::i_SideNone, 0);

    SolidSPtr pResSolid;
    MbEvolutionShellParams evolParams(sweptData, *helix, params, operNames, contourNames, spineNames);
    if (EvolutionShell(evolParams, pResSolid) != rt_Success)
        return nullptr;

    return pResSolid;
}

void addTriangleEdges(MbMesh& mesh)
{
    for (size_t i = 0; i < mesh.GridsCount(); ++i)
    {
        const MbGrid* grid = mesh.GetGrid(i);
        for (size_t t = 0; t < grid->TrianglesCount(); ++t)
        {
            MbCartPoint3D p0, p1, p2;
            if (!grid->GetTrianglePoints(t, p0, p1, p2))
                continue;

            MbPolygon3D* edges = mesh.AddPolygon();
            edges->AddPoint(p0);
            edges->AddPoint(p1);
            edges->AddPoint(p2);
            edges->AddPoint(p0);
        }
    }
}

SPtr<MbMesh> triangulate(const c3d::SolidSPtr& solid, double sag)
{
    MbStepData stepData(ist_SpaceStep, sag);

    MbFormNote note(false, true, true, true);
    SPtr<MbMesh> mesh(new MbMesh(true));
    if (CalculateMesh(*solid, stepData, note, *mesh) != rt_Success)
        return nullptr;

    addTriangleEdges(*mesh);
    return mesh;
}

void detachFromSource(MbMesh& mesh)
{
    mesh.SetRefItem(nullptr);
    for (size_t i = 0; i < mesh.GridsCount(); ++i)
        mesh.SetGrid(i)->SetItem(nullptr);
    for (size_t i = 0; i < mesh.PolygonsCount(); ++i)
        mesh.SetPolygon(i)->SetItem(nullptr);
    for (size_t i = 0; i < mesh.ApexesCount(); ++i)
        mesh.SetApex(i)->SetItem(nullptr);
}

SPtr<MbMesh> remesh(const c3d::SolidSPtr& solid, double sag, double targetLength)
{
    if (!solid)
        return nullptr;

    SPtr<MbPolymesh> poly(new MbPolymesh());
    MbShellLinearizationParams linParams(MbStepData(ist_SpaceStep, sag), true, false, false,
        std::vector<c3d::IndicesVector>(), c3d::IndicesVector());
    MbShellLinearizationResults linResults;
    if (CreatePolymeshByShell(linParams, *solid->GetShell(), *poly, linResults) != rt_Success)
        return nullptr;

    MbRemeshParams params;
    params.InitIncrementalUniform(targetLength, c3d::IndicesVector());
    if (Remesh(params, *poly) != rt_Success)
        return nullptr;

    if (CalculatePolymeshNormals(MbPolymeshNormalsParams(true), *poly) != rt_Success)
        return nullptr;

    SPtr<MbMesh> mesh(new MbMesh(true));
    if (CalculateMesh(*poly, MbStepData(), MbFormNote(false, true, true, true), *mesh) != rt_Success)
        return nullptr;

    detachFromSource(*mesh);
    addTriangleEdges(*mesh);
    return mesh;
}

void createObject(SceneSegment* pRootSegment, const SPtr<MbItem>& item, const Matrix3DF& moveRoot, const Color& color, double opacity)
{
    if (!item)
        return;

    MathRepresentation* pMathRep = new MathRepresentation();
    pMathRep->SetBuildType(CommandType::Synchronous);
    pMathRep->SetMathItem(item.get());
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
    scene.graphicsView()->SetRenderMode(rm_ShadedWithEdges);

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
    createObject(rootSegment, makeFillet(boolean, 0.5, true), move4, VSN::Color(0, 255, 0), 1.0);

    Matrix3DF move5;
    move5.Move(VSN::Vector3DF(70.0f, 0.0f, 0.0f));
    createObject(rootSegment, makeFillet(boolean, 0.5, false), move5, VSN::Color(0, 255, 0), 1.0);

    Matrix3DF move6;
    move6.Move(VSN::Vector3DF(85.0f, 0.0f, 0.0f));
    createObject(rootSegment, triangulate(cylinder, 1.0), move6, VSN::Color(0, 0, 255), 1.0);

    Matrix3DF move7;
    move7.Move(VSN::Vector3DF(100.0f, 0.0f, 0.0f));
    createObject(rootSegment, triangulate(cylinder, 0.1), move7, VSN::Color(0, 0, 255), 1.0);

    Matrix3DF move8;
    move8.Move(VSN::Vector3DF(115.0f, 0.0f, 0.0f));
    createObject(rootSegment, triangulate(boolean, 0.3), move8, VSN::Color(0, 0, 255), 1.0);

    Matrix3DF move9;
    move9.Move(VSN::Vector3DF(130.0f, 0.0f, 0.0f));
    createObject(rootSegment, remesh(cylinder, 1.0, 1.5), move9, VSN::Color(255, 0, 255), 1.0);

    Matrix3DF move10;
    move10.Move(VSN::Vector3DF(145.0f, 0.0f, 0.0f));
    createObject(rootSegment, remesh(boolean, 0.3, 0.5), move10, VSN::Color(255, 0, 255), 1.0);

    Matrix3DF move11;
    move11.Move(VSN::Vector3DF(165.0f, 0.0f, 0.0f));
    createObject(rootSegment, makeHelixSweep(2.0, 30.0), move11, VSN::Color(0, 200, 200), 1.0);

    Matrix3DF move12;
    move12.Move(VSN::Vector3DF(185.0f, 0.0f, 0.0f));
    createObject(rootSegment, makeHelixSweep(1.0, 40.0), move12, VSN::Color(0, 200, 200), 1.0);

    scene.viewport()->GetCamera()->SetViewOrientation(Orientation::IsoXYZ);
    scene.viewport()->ZoomToFit(scene.sceneContent()->GetBoundingBox());

    setWindowPosition(scene);
    scene.show();

    return app->exec();
}
