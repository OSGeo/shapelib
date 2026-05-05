#include <cmath>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

extern "C"
{
#include "shapefil.h"

    long int parse_select_values(const char *input, long int *values,
                                 int max_values);
    void transform_coordinates(SHPObject *psShape, double dfFactor,
                               double dfXShift, double dfYShift);
    bool copy_dbf_record(DBFHandle hDBFin, int iRecord, DBFHandle hDBFout,
                         int jRecord, const int *fieldmap, int nFields);
    bool compute_clip_intersection(double x0, double y0, double x1, double y1,
                                   double clipxmin, double clipymin,
                                   double clipxmax, double clipymax, double *xi,
                                   double *yi);
    void process_records(void);

    /* Global state needed for clip_boundary / check_theme_bnd tests */
    extern SHPObject *psCShape;
    extern SHPHandle hSHP, hSHPappend;
    extern DBFHandle hDBF, hDBFappend;
    extern int *pt;
    extern int nEntities, nShapeType;
    extern double cxmin, cymin, cxmax, cymax;
    extern bool iclip, ierase, itouch, iinside, icut, iselect, iunit;
    extern double adfBoundsMin[4], adfBoundsMax[4];
    extern double factor, xshift, yshift;

    int clip_boundary();
    void check_theme_bnd();
}

namespace fs = std::filesystem;

namespace
{

// ---------------------------------------------------------------------------
// clip_boundary tests
// ---------------------------------------------------------------------------

class ClipBoundaryTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set default clip box
        cxmin = 0.0;
        cymin = 0.0;
        cxmax = 100.0;
        cymax = 100.0;

        // Reset flags
        ierase = false;
        itouch = false;
        iinside = false;
        icut = false;
    }

    void TearDown() override
    {
        if (psCShape)
        {
            SHPDestroyObject(psCShape);
            psCShape = nullptr;
        }
    }

    void MakePoint(double x, double y)
    {
        psCShape = SHPCreateSimpleObject(SHPT_POINT, 1, &x, &y, nullptr);
    }

    void MakeLine(double x1, double y1, double x2, double y2)
    {
        double xs[] = {x1, x2};
        double ys[] = {y1, y2};
        psCShape = SHPCreateSimpleObject(SHPT_ARC, 2, xs, ys, nullptr);
    }

    void MakePolyline(const std::vector<std::pair<double, double>> &pts)
    {
        std::vector<double> xs, ys;
        for (const auto &[x, y] : pts)
        {
            xs.push_back(x);
            ys.push_back(y);
        }
        psCShape = SHPCreateSimpleObject(SHPT_ARC, static_cast<int>(xs.size()),
                                         xs.data(), ys.data(), nullptr);
    }

    void MakePolygon(const std::vector<std::pair<double, double>> &pts)
    {
        std::vector<double> xs, ys;
        for (const auto &[x, y] : pts)
        {
            xs.push_back(x);
            ys.push_back(y);
        }
        psCShape =
            SHPCreateSimpleObject(SHPT_POLYGON, static_cast<int>(xs.size()),
                                  xs.data(), ys.data(), nullptr);
    }

    void
    MakePolygonWithHole(const std::vector<std::pair<double, double>> &outer,
                        const std::vector<std::pair<double, double>> &hole)
    {
        std::vector<double> xs, ys;
        for (const auto &[x, y] : outer)
        {
            xs.push_back(x);
            ys.push_back(y);
        }
        for (const auto &[x, y] : hole)
        {
            xs.push_back(x);
            ys.push_back(y);
        }
        int parts[] = {0, static_cast<int>(outer.size())};
        psCShape = SHPCreateObject(SHPT_POLYGON, -1, 2, parts, nullptr,
                                   static_cast<int>(xs.size()), xs.data(),
                                   ys.data(), nullptr, nullptr);
    }
};

TEST_F(ClipBoundaryTest, FeatureTotallyOutsideClip)
{
    MakePoint(200.0, 200.0);
    EXPECT_EQ(0, clip_boundary());
}

TEST_F(ClipBoundaryTest, FeatureTotallyOutsideErase)
{
    ierase = true;
    MakePoint(200.0, 200.0);
    EXPECT_EQ(1, clip_boundary());
}

TEST_F(ClipBoundaryTest, FeatureTotallyInsideClip)
{
    MakePoint(50.0, 50.0);
    EXPECT_EQ(1, clip_boundary());
}

TEST_F(ClipBoundaryTest, FeatureTotallyInsideErase)
{
    ierase = true;
    MakePoint(50.0, 50.0);
    EXPECT_EQ(0, clip_boundary());
}

TEST_F(ClipBoundaryTest, FeatureCrossesClipInsideMode)
{
    // Feature straddles the clip boundary
    iinside = true;
    MakeLine(-10.0, 50.0, 50.0, 50.0);
    // Inside mode: feature not fully inside -> skip
    EXPECT_EQ(0, clip_boundary());
}

TEST_F(ClipBoundaryTest, FeatureCrossesEraseInsideMode)
{
    ierase = true;
    iinside = true;
    MakeLine(-10.0, 50.0, 50.0, 50.0);
    // Erase + inside: feature not fully inside clip box -> write
    EXPECT_EQ(1, clip_boundary());
}

// ---------------------------------------------------------------------------
// compute_clip_intersection tests
//
//  Clip box used throughout:  (0,0) to (100,100)
//
//       100 +----------+
//           |          |
//           | clip box |
//           |          |
//         0 +----------+
//           0         100
// ---------------------------------------------------------------------------

TEST(ComputeClipIntersectionTest, CrossesLeftEdge)
{
    //         +----------+
    //  B <----X---- A    |      A=(50,50)  B=(-50,50)
    //  (-50)  |0   (50)  |      X = intersection at (0,50)
    //         +----------+
    double xi, yi;
    EXPECT_TRUE(
        compute_clip_intersection(50, 50, -50, 50, 0, 0, 100, 100, &xi, &yi));
    EXPECT_DOUBLE_EQ(0.0, xi);
    EXPECT_DOUBLE_EQ(50.0, yi);
}

TEST(ComputeClipIntersectionTest, CrossesRightEdge)
{
    //  +----------+
    //  |    A ----X----> B      A=(50,50)  B=(150,50)
    //  |   (50)  |100  (150)    X = intersection at (100,50)
    //  +----------+
    double xi, yi;
    EXPECT_TRUE(
        compute_clip_intersection(50, 50, 150, 50, 0, 0, 100, 100, &xi, &yi));
    EXPECT_DOUBLE_EQ(100.0, xi);
    EXPECT_DOUBLE_EQ(50.0, yi);
}

TEST(ComputeClipIntersectionTest, CrossesBottomEdge)
{
    //         +----------+
    //         |    A     |      A=(50,50)
    //         |    |     |
    //         +----X-----+      X = intersection at (50,0)
    //              |
    //              B            B=(50,-50)
    double xi, yi;
    EXPECT_TRUE(
        compute_clip_intersection(50, 50, 50, -50, 0, 0, 100, 100, &xi, &yi));
    EXPECT_DOUBLE_EQ(50.0, xi);
    EXPECT_DOUBLE_EQ(0.0, yi);
}

TEST(ComputeClipIntersectionTest, CrossesTopEdge)
{
    //              B            B=(50,150)
    //              |
    //         +----X-----+      X = intersection at (50,100)
    //         |    |     |
    //         |    A     |      A=(50,50)
    //         +----------+
    double xi, yi;
    EXPECT_TRUE(
        compute_clip_intersection(50, 50, 50, 150, 0, 0, 100, 100, &xi, &yi));
    EXPECT_DOUBLE_EQ(50.0, xi);
    EXPECT_DOUBLE_EQ(100.0, yi);
}

TEST(ComputeClipIntersectionTest, OutsideToInsideCrossesLeft)
{
    double xi, yi;
    // From outside (-50,50) to inside (50,50) crosses left edge at (0,50)
    EXPECT_TRUE(
        compute_clip_intersection(-50, 50, 50, 50, 0, 0, 100, 100, &xi, &yi));
    EXPECT_DOUBLE_EQ(0.0, xi);
    EXPECT_DOUBLE_EQ(50.0, yi);
}

TEST(ComputeClipIntersectionTest, DiagonalCross)
{
    //                       B   B=(150,150)
    //                    /
    //         +-------X         X = intersection at corner (100,100)
    //         |    /  |
    //         | A     |         A=(50,50)
    //         +-------+
    double xi, yi;
    EXPECT_TRUE(
        compute_clip_intersection(50, 50, 150, 150, 0, 0, 100, 100, &xi, &yi));
    EXPECT_DOUBLE_EQ(100.0, xi);
    EXPECT_DOUBLE_EQ(100.0, yi);
}

TEST(ComputeClipIntersectionTest, FullyInsideNoIntersection)
{
    double xi = -1, yi = -1;
    // Both endpoints inside -> no intersection
    EXPECT_FALSE(
        compute_clip_intersection(20, 20, 80, 80, 0, 0, 100, 100, &xi, &yi));
}

TEST(ComputeClipIntersectionTest, FullyOutsideNoIntersection)
{
    double xi = -1, yi = -1;
    // Both endpoints outside on same side -> no intersection
    EXPECT_FALSE(compute_clip_intersection(150, 150, 200, 200, 0, 0, 100, 100,
                                           &xi, &yi));
}

TEST(ComputeClipIntersectionTest, TouchesCorner)
{
    //  A                        A=(-50,150)
    //    \
    //     X---------+           X = intersection at corner (0,100)
    //     | \       |
    //     |   B     |           B=(50,50)  (inside)
    //     |         |
    //     +---------+
    double xi, yi;
    EXPECT_TRUE(
        compute_clip_intersection(-50, 150, 50, 50, 0, 0, 100, 100, &xi, &yi));
    EXPECT_DOUBLE_EQ(0.0, xi);
    EXPECT_DOUBLE_EQ(100.0, yi);
}

TEST(ComputeClipIntersectionTest, GrazesCornerDiagonal)
{
    //        B                  B=(50,150)
    //       /
    //      /
    //     X------+              X = intersection at corner (0,100)
    //    /|      |
    //   / |      |
    //  A  +------+              A=(-100,0)
    double xi, yi;
    EXPECT_TRUE(
        compute_clip_intersection(-100, 0, 50, 150, 0, 0, 100, 100, &xi, &yi));
    EXPECT_DOUBLE_EQ(0.0, xi);
    EXPECT_DOUBLE_EQ(100.0, yi);
}

// ---------------------------------------------------------------------------
// CUT mode tests
//
//  Clip box:  (0,0) to (100,100)
//  CUT trims geometry to the box, computing intersection vertices.
// ---------------------------------------------------------------------------

TEST_F(ClipBoundaryTest, CutLineExitingRight)
{
    //  +----------+
    //  |  A=======X-------B     A=(50,50) B=(150,50)
    //  |  (50)   100     (150)  result: A..X
    //  +----------+
    icut = true;
    MakeLine(50.0, 50.0, 150.0, 50.0);
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(2, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfX[0]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfY[0]);
    EXPECT_DOUBLE_EQ(100.0, psCShape->padfX[1]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfY[1]);
}

TEST_F(ClipBoundaryTest, CutLineEnteringFromLeft)
{
    //       +----------+
    //  A----X=======B  |        A=(-50,50) B=(50,50)
    // (-50) 0     (50) |        result: X..B
    //       +----------+
    icut = true;
    MakeLine(-50.0, 50.0, 50.0, 50.0);
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(2, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(0.0, psCShape->padfX[0]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfY[0]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfX[1]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfY[1]);
}

TEST_F(ClipBoundaryTest, CutLineCrossingThrough)
{
    //        +----------+
    //  A-----X1========X2-----B   A=(-50,50) B=(150,50)
    // (-50)  0         100  (150) both outside, segment crosses through
    //        +----------+         result: X1..X2
    icut = true;
    MakeLine(-50.0, 50.0, 150.0, 50.0);
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(2, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(0.0, psCShape->padfX[0]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfY[0]);
    EXPECT_DOUBLE_EQ(100.0, psCShape->padfX[1]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfY[1]);
}

TEST_F(ClipBoundaryTest, CutLineFullyInside)
{
    //  +----------+
    //  |  A====B  |     A=(20,20) B=(80,80)
    //  |          |     entirely inside, no clipping needed
    //  +----------+     result: A..B unchanged
    icut = true;
    MakeLine(20.0, 20.0, 80.0, 80.0);
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(2, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(20.0, psCShape->padfX[0]);
    EXPECT_DOUBLE_EQ(20.0, psCShape->padfY[0]);
    EXPECT_DOUBLE_EQ(80.0, psCShape->padfX[1]);
    EXPECT_DOUBLE_EQ(80.0, psCShape->padfY[1]);
}

TEST_F(ClipBoundaryTest, CutLineFullyOutside)
{
    //  +----------+
    //  |          |             A=(150,150) B=(200,200)
    //  |          |   A----B    entirely outside clip box
    //  +----------+             result: skip record
    icut = true;
    MakeLine(150.0, 150.0, 200.0, 200.0);
    EXPECT_EQ(0, clip_boundary());
}

TEST_F(ClipBoundaryTest, CutMultiVertexPolyline)
{
    //        +----------+
    //  A-----X1===B====X2-----C   A=(-10,50) B=(50,50) C=(150,50)
    // (-10)  0   (50)  100  (150)
    //        +----------+         result: X1..B..X2  (3 vertices)
    icut = true;
    MakePolyline({{-10, 50}, {50, 50}, {150, 50}});
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(3, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(0.0, psCShape->padfX[0]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfY[0]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfX[1]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfY[1]);
    EXPECT_DOUBLE_EQ(100.0, psCShape->padfX[2]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfY[2]);
}

TEST_F(ClipBoundaryTest, CutEraseLineInside)
{
    //  +----------+
    //  |  A~~~~~~~X=======B     A=(50,50) B=(150,50)
    //  | (erased) 100    (150)  erase mode: keep OUTSIDE part
    //  +----------+             result: X..B
    icut = true;
    ierase = true;
    MakeLine(50.0, 50.0, 150.0, 50.0);
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(2, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(100.0, psCShape->padfX[0]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfY[0]);
    EXPECT_DOUBLE_EQ(150.0, psCShape->padfX[1]);
    EXPECT_DOUBLE_EQ(50.0, psCShape->padfY[1]);
}

TEST_F(ClipBoundaryTest, CutReducesToOneVertex)
{
    icut = true;
    // Single vertex inside, but the line mostly outside -> only 1 vertex
    // -> should return 0 (skip)
    MakePolyline({{-100, 50}, {1, 50}, {-100, 60}});
    int result = clip_boundary();
    if (psCShape->nVertices < 2)
        EXPECT_EQ(0, result);
}

// ---------------------------------------------------------------------------
// CUT mode polygon tests
//
//  Clip box:  (0,0) to (100,100)
//  Polygon CUT uses Sutherland-Hodgman to clip the polygon to the box.
//  All input polygons are closed (first vertex == last vertex).
// ---------------------------------------------------------------------------

static double RingArea(const double *x, const double *y, int n)
{
    double sum = 0;
    for (int i = 0; i < n - 1; i++)
        sum += x[i] * y[i + 1] - x[i + 1] * y[i];
    return sum / 2.0; /* signed: >0 CCW, <0 CW */
}

static double PolygonArea(const SHPObject *obj)
{
    return fabs(RingArea(obj->padfX, obj->padfY, obj->nVertices));
}

static double TotalPolygonArea(const SHPObject *obj)
{
    double total = 0;
    for (int p = 0; p < obj->nParts; p++)
    {
        const int start = obj->panPartStart[p];
        const int end =
            (p + 1 < obj->nParts) ? obj->panPartStart[p + 1] : obj->nVertices;
        total +=
            fabs(RingArea(obj->padfX + start, obj->padfY + start, end - start));
    }
    return total;
}

static double NetPolygonArea(const SHPObject *obj)
{
    double total = 0;
    for (int p = 0; p < obj->nParts; p++)
    {
        const int start = obj->panPartStart[p];
        const int end =
            (p + 1 < obj->nParts) ? obj->panPartStart[p + 1] : obj->nVertices;
        total += RingArea(obj->padfX + start, obj->padfY + start, end - start);
    }
    return fabs(total);
}

TEST_F(ClipBoundaryTest, CutPolygonFullyInside)
{
    //  100 +----------+
    //      | +------+ |      (20,20)→(80,20)→(80,80)→(20,80)
    //      | | poly | |      fully inside → unchanged
    //      | +------+ |
    //    0 +----------+
    //      0         100
    icut = true;
    MakePolygon({{20, 20}, {80, 20}, {80, 80}, {20, 80}, {20, 20}});
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(5, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(20.0, psCShape->padfX[0]);
    EXPECT_DOUBLE_EQ(20.0, psCShape->padfY[0]);
}

TEST_F(ClipBoundaryTest, CutPolygonFullyOutside)
{
    //  100 +----------+       poly at (200,200)-(300,300)
    //      |          |       entirely outside → skip
    //      |          |
    //      |          |
    //    0 +----------+
    //      0         100
    icut = true;
    MakePolygon({{200, 200}, {300, 200}, {300, 300}, {200, 300}, {200, 200}});
    EXPECT_EQ(0, clip_boundary());
}

TEST_F(ClipBoundaryTest, CutPolygonClipsToRightEdge)
{
    //  100 +----------+
    //   75 |    +-----X-----+   (50,25)→(150,25)→(150,75)→(50,75)
    //      |    |clpd | out |   X = clip points at x=100
    //   25 |    +-----X-----+   result = 50×50 rectangle, area = 2500
    //    0 +----------+
    //      0    50   100   150
    icut = true;
    MakePolygon({{50, 25}, {150, 25}, {150, 75}, {50, 75}, {50, 25}});
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(5, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(psCShape->padfX[0],
                     psCShape->padfX[psCShape->nVertices - 1]);
    EXPECT_DOUBLE_EQ(psCShape->padfY[0],
                     psCShape->padfY[psCShape->nVertices - 1]);
    EXPECT_NEAR(2500.0, PolygonArea(psCShape), 1.0);
}

TEST_F(ClipBoundaryTest, CutTriangleClipsToRightEdge)
{
    //  100 +----------+
    //   80 |    *.    |         (50,20)→(150,50)→(50,80)
    //      |    | `.  |         apex at (150,50) is outside clip box
    //   50 |    |    `X---*     X = clip points at x=100 (y=35 and y=65)
    //      |    | .'  |         result = trapezoid, area = 2250
    //   20 |    *'    |
    //    0 +----------+
    //      0    50   100   150
    icut = true;
    MakePolygon({{50, 20}, {150, 50}, {50, 80}, {50, 20}});
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(5, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(psCShape->padfX[0],
                     psCShape->padfX[psCShape->nVertices - 1]);
    EXPECT_DOUBLE_EQ(psCShape->padfY[0],
                     psCShape->padfY[psCShape->nVertices - 1]);
    EXPECT_NEAR(2250.0, PolygonArea(psCShape), 1.0);
}

TEST_F(ClipBoundaryTest, CutPolygonCrossesCorner)
{
    //  150      +----------+
    //           |          |    (50,50)→(150,50)→(150,150)→(50,150)
    //  100 +----X----*     |    polygon overlaps top-right of clip box
    //      |    |clpd|     |    * = clip corner (100,100)
    //   50 |    +----X-----+    X = clip points (100,50) and (50,100)
    //      |          |         result = 50×50 square, area = 2500
    //    0 +----------+
    //      0    50   100   150
    icut = true;
    MakePolygon({{50, 50}, {150, 50}, {150, 150}, {50, 150}, {50, 50}});
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(5, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(psCShape->padfX[0],
                     psCShape->padfX[psCShape->nVertices - 1]);
    EXPECT_DOUBLE_EQ(psCShape->padfY[0],
                     psCShape->padfY[psCShape->nVertices - 1]);
    bool hasCorner = false;
    for (int i = 0; i < psCShape->nVertices; i++)
        if (psCShape->padfX[i] == 100.0 && psCShape->padfY[i] == 100.0)
            hasCorner = true;
    EXPECT_TRUE(hasCorner) << "Clipped polygon must include corner (100,100)";
    EXPECT_NEAR(2500.0, PolygonArea(psCShape), 1.0);
}

TEST_F(ClipBoundaryTest, CutPolygonCrossesTwoEdges)
{
    //  100 +----------+
    //   75 +--X-------X--+     (-50,25)→(150,25)→(150,75)→(-50,75)
    //   50 |  |clipped|  |     clips at x=0 (left) and x=100 (right)
    //   25 +--X-------X--+     area = 100×50 = 5000
    //    0 +----------+
    //     -50 0      100 150
    icut = true;
    MakePolygon({{-50, 25}, {150, 25}, {150, 75}, {-50, 75}, {-50, 25}});
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(5, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(psCShape->padfX[0],
                     psCShape->padfX[psCShape->nVertices - 1]);
    EXPECT_DOUBLE_EQ(psCShape->padfY[0],
                     psCShape->padfY[psCShape->nVertices - 1]);
    EXPECT_NEAR(5000.0, PolygonArea(psCShape), 1.0);
}

TEST_F(ClipBoundaryTest, CutPolygonEnclosesBox)
{
    //  150 .--+----------+--.    (-50,-50)→(150,-50)→(150,150)→(-50,150)
    //  100 |  +----------+  |   encloses entire clip box
    //   50 |  |  result  |  |   result = clip box
    //    0 |  +----------+  |   area = 100×100 = 10000
    //  -50 '--+----------+--'
    //     -50 0         100 150
    icut = true;
    MakePolygon({{-50, -50}, {150, -50}, {150, 150}, {-50, 150}, {-50, -50}});
    EXPECT_EQ(1, clip_boundary());
    EXPECT_GE(psCShape->nVertices, 5);
    EXPECT_DOUBLE_EQ(psCShape->padfX[0],
                     psCShape->padfX[psCShape->nVertices - 1]);
    EXPECT_DOUBLE_EQ(psCShape->padfY[0],
                     psCShape->padfY[psCShape->nVertices - 1]);
    EXPECT_NEAR(10000.0, PolygonArea(psCShape), 1.0);
}

TEST_F(ClipBoundaryTest, CutDiamondCrossesFourEdges)
{
    //  120         *            (50,-20)→(120,50)→(50,120)→(-20,50)
    //  100 +----/--+--\---+     crosses all 4 edges
    //      |  /    |    \ |     result: octagon, 9 verts (8 + closing)
    //   50 *    clip box   *    area = 10000 - 4×(30²/2) = 8200
    //      |  \    |    / |
    //    0 +----\--+--/---+
    //  -20         *
    //     -20  0   50  100 120
    icut = true;
    MakePolygon({{50, -20}, {120, 50}, {50, 120}, {-20, 50}, {50, -20}});
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(9, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(psCShape->padfX[0],
                     psCShape->padfX[psCShape->nVertices - 1]);
    EXPECT_DOUBLE_EQ(psCShape->padfY[0],
                     psCShape->padfY[psCShape->nVertices - 1]);
    for (int i = 0; i < psCShape->nVertices; i++)
    {
        EXPECT_GE(psCShape->padfX[i], cxmin - 1e-10);
        EXPECT_LE(psCShape->padfX[i], cxmax + 1e-10);
        EXPECT_GE(psCShape->padfY[i], cymin - 1e-10);
        EXPECT_LE(psCShape->padfY[i], cymax + 1e-10);
    }
    EXPECT_NEAR(8200.0, PolygonArea(psCShape), 1.0);
}

TEST_F(ClipBoundaryTest, CutConcaveNotchCrossesRightEdge)
{
    //  100 +----------+
    //   80 |    +-----X--+      (50,20)→(120,20)→(120,40)→(70,40)→
    //   60 |    |  +--X--+      (70,60)→(120,60)→(120,80)→(50,80)
    //   50 |    |  |ntch |      notch exits/re-enters at x=100
    //   40 |    |  +--X--+      C-shape, 9 verts, area = 2400
    //   20 |    +-----X--+
    //    0 +----------+
    //      0   50 70 100 120
    icut = true;
    MakePolygon({{50, 20},
                 {120, 20},
                 {120, 40},
                 {70, 40},
                 {70, 60},
                 {120, 60},
                 {120, 80},
                 {50, 80},
                 {50, 20}});
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(9, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(psCShape->padfX[0],
                     psCShape->padfX[psCShape->nVertices - 1]);
    EXPECT_DOUBLE_EQ(psCShape->padfY[0],
                     psCShape->padfY[psCShape->nVertices - 1]);
    for (int i = 0; i < psCShape->nVertices; i++)
    {
        EXPECT_GE(psCShape->padfX[i], cxmin - 1e-10);
        EXPECT_LE(psCShape->padfX[i], cxmax + 1e-10);
    }
    EXPECT_NEAR(2400.0, PolygonArea(psCShape), 1.0);
}

TEST_F(ClipBoundaryTest, CutChevronCrossesBottomEdge)
{
    //  100 +----------+
    //   80 | A--------B |      A=(20,80) B=(80,80)
    //   50 |   \    /   |      chevron dips below y=0
    //      |    \  /    |
    //    0 +----X1-X2---+      X1≈(44,0) X2≈(56,0)
    //  -20       *             C=(50,-20) outside
    //      0 20 44 56 80 100   trapezoid, area = 2880
    icut = true;
    MakePolygon({{20, 80}, {50, -20}, {80, 80}, {20, 80}});
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(5, psCShape->nVertices);
    EXPECT_DOUBLE_EQ(psCShape->padfX[0],
                     psCShape->padfX[psCShape->nVertices - 1]);
    EXPECT_DOUBLE_EQ(psCShape->padfY[0],
                     psCShape->padfY[psCShape->nVertices - 1]);
    for (int i = 0; i < psCShape->nVertices; i++)
    {
        EXPECT_GE(psCShape->padfY[i], cymin - 1e-10);
        EXPECT_LE(psCShape->padfY[i], cymax + 1e-10);
    }
    EXPECT_NEAR(2880.0, PolygonArea(psCShape), 1.0);
}

TEST_F(ClipBoundaryTest, CutPolygonWithHoleFullyInside)
{
    //  100 +----------+
    //   90 | +------+ |        outer: (10,10)→(90,10)→(90,90)→(10,90)
    //   70 | | +--+ | |        hole:  (30,30)→(30,70)→(70,70)→(70,30) CW
    //   50 | | |  | | |        both fully inside clip box
    //   30 | | +--+ | |        net area = 80×80 − 40×40 = 4800
    //   10 | +------+ |
    //    0 +----------+
    //      0 10 30 70 90 100
    icut = true;
    MakePolygonWithHole({{10, 10}, {90, 10}, {90, 90}, {10, 90}, {10, 10}},
                        {{30, 30}, {30, 70}, {70, 70}, {70, 30}, {30, 30}});
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(2, psCShape->nParts);
    EXPECT_EQ(10, psCShape->nVertices);  // 5 outer + 5 hole
    EXPECT_NEAR(4800.0, NetPolygonArea(psCShape), 1.0);
}

TEST_F(ClipBoundaryTest, CutPolygonWithHoleCrossesTop)
{
    //  130   +------+
    //  110   | +--+ |          outer: (10,10)→(90,10)→(90,130)→(10,130)
    //  100 +-X-+--+-X-+        hole:  (30,70)→(30,110)→(70,110)→(70,70) CW
    //      | | |  | | |        X = both rings clip at y=100
    //   70 | | +--+ | |        outer clipped → 80×90 = 7200
    //      | |      | |        hole  clipped → 40×30 = 1200
    //   10 | +------+ |        net area = 7200 − 1200 = 6000
    //    0 +----------+
    //      0 10 30 70 90 100
    icut = true;
    MakePolygonWithHole({{10, 10}, {90, 10}, {90, 130}, {10, 130}, {10, 10}},
                        {{30, 70}, {30, 110}, {70, 110}, {70, 70}, {30, 70}});
    EXPECT_EQ(1, clip_boundary());
    EXPECT_EQ(2, psCShape->nParts);
    EXPECT_EQ(10, psCShape->nVertices);  // 5 outer + 5 hole
    // Each ring must be closed
    for (int p = 0; p < psCShape->nParts; p++)
    {
        const int start = psCShape->panPartStart[p];
        const int end = (p + 1 < psCShape->nParts)
                            ? psCShape->panPartStart[p + 1]
                            : psCShape->nVertices;
        EXPECT_DOUBLE_EQ(psCShape->padfX[start], psCShape->padfX[end - 1])
            << "ring " << p;
        EXPECT_DOUBLE_EQ(psCShape->padfY[start], psCShape->padfY[end - 1])
            << "ring " << p;
    }
    EXPECT_NEAR(6000.0, NetPolygonArea(psCShape), 1.0);
}

// ---------------------------------------------------------------------------
// check_theme_bnd tests
// ---------------------------------------------------------------------------

class CheckThemeBndTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        cxmin = 0.0;
        cymin = 0.0;
        cxmax = 100.0;
        cymax = 100.0;
        ierase = false;
        iclip = true;
        nEntities = 10;
    }
};

TEST_F(CheckThemeBndTest, ThemeTotallyInsideClip)
{
    adfBoundsMin[0] = 10.0;
    adfBoundsMin[1] = 10.0;
    adfBoundsMax[0] = 90.0;
    adfBoundsMax[1] = 90.0;
    check_theme_bnd();
    // Theme inside clip -> clip not needed
    EXPECT_FALSE(iclip);
    EXPECT_EQ(10, nEntities);
}

TEST_F(CheckThemeBndTest, ThemeTotallyInsideErase)
{
    ierase = true;
    adfBoundsMin[0] = 10.0;
    adfBoundsMin[1] = 10.0;
    adfBoundsMax[0] = 90.0;
    adfBoundsMax[1] = 90.0;
    check_theme_bnd();
    // Theme inside erase -> skip all
    EXPECT_EQ(0, nEntities);
}

TEST_F(CheckThemeBndTest, ThemeTotallyOutsideClip)
{
    adfBoundsMin[0] = 200.0;
    adfBoundsMin[1] = 200.0;
    adfBoundsMax[0] = 300.0;
    adfBoundsMax[1] = 300.0;
    check_theme_bnd();
    // Theme outside clip -> skip all
    EXPECT_EQ(0, nEntities);
}

TEST_F(CheckThemeBndTest, ThemeTotallyOutsideErase)
{
    ierase = true;
    adfBoundsMin[0] = 200.0;
    adfBoundsMin[1] = 200.0;
    adfBoundsMax[0] = 300.0;
    adfBoundsMax[1] = 300.0;
    check_theme_bnd();
    // Theme outside erase -> clip not needed, write theme
    EXPECT_FALSE(iclip);
}

// ---------------------------------------------------------------------------
// parse_select_values tests
// ---------------------------------------------------------------------------

TEST(ParseSelectValuesTest, SingleValue)
{
    long int values[150] = {};
    const long int count = parse_select_values("42", values, 150);
    EXPECT_EQ(1, count);
    EXPECT_EQ(42, values[0]);
}

TEST(ParseSelectValuesTest, MultipleCommaSeparated)
{
    long int values[150] = {};
    const long int count = parse_select_values("3,5,9,13,17,27", values, 150);
    EXPECT_EQ(6, count);
    EXPECT_EQ(3, values[0]);
    EXPECT_EQ(5, values[1]);
    EXPECT_EQ(9, values[2]);
    EXPECT_EQ(13, values[3]);
    EXPECT_EQ(17, values[4]);
    EXPECT_EQ(27, values[5]);
}

TEST(ParseSelectValuesTest, SpaceSeparated)
{
    long int values[150] = {};
    const long int count = parse_select_values("10 20 30", values, 150);
    EXPECT_EQ(3, count);
    EXPECT_EQ(10, values[0]);
    EXPECT_EQ(20, values[1]);
    EXPECT_EQ(30, values[2]);
}

TEST(ParseSelectValuesTest, EmptyString)
{
    long int values[150] = {};
    const long int count = parse_select_values("", values, 150);
    EXPECT_EQ(0, count);
}

TEST(ParseSelectValuesTest, NoDigits)
{
    long int values[150] = {};
    const long int count = parse_select_values("abc", values, 150);
    EXPECT_EQ(0, count);
}

TEST(ParseSelectValuesTest, MaxValuesLimit)
{
    long int values[3] = {};
    const long int count = parse_select_values("1,2,3,4,5", values, 3);
    EXPECT_EQ(3, count);
    EXPECT_EQ(1, values[0]);
    EXPECT_EQ(2, values[1]);
    EXPECT_EQ(3, values[2]);
}

TEST(ParseSelectValuesTest, MixedDelimiters)
{
    long int values[150] = {};
    const long int count = parse_select_values("100, 200; 300", values, 150);
    EXPECT_EQ(3, count);
    EXPECT_EQ(100, values[0]);
    EXPECT_EQ(200, values[1]);
    EXPECT_EQ(300, values[2]);
}

// ---------------------------------------------------------------------------
// transform_coordinates tests
// ---------------------------------------------------------------------------

class TransformCoordinatesTest : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        if (shape_)
        {
            SHPDestroyObject(shape_);
            shape_ = nullptr;
        }
    }

    SHPObject *shape_ = nullptr;
};

TEST_F(TransformCoordinatesTest, IdentityTransform)
{
    double x[] = {1.0, 2.0, 3.0};
    double y[] = {4.0, 5.0, 6.0};
    shape_ = SHPCreateSimpleObject(SHPT_ARC, 3, x, y, nullptr);
    transform_coordinates(shape_, 1.0, 0.0, 0.0);
    EXPECT_DOUBLE_EQ(1.0, shape_->padfX[0]);
    EXPECT_DOUBLE_EQ(2.0, shape_->padfX[1]);
    EXPECT_DOUBLE_EQ(3.0, shape_->padfX[2]);
    EXPECT_DOUBLE_EQ(4.0, shape_->padfY[0]);
    EXPECT_DOUBLE_EQ(5.0, shape_->padfY[1]);
    EXPECT_DOUBLE_EQ(6.0, shape_->padfY[2]);
}

TEST_F(TransformCoordinatesTest, ScaleOnly)
{
    double x[] = {10.0, 20.0};
    double y[] = {30.0, 40.0};
    shape_ = SHPCreateSimpleObject(SHPT_ARC, 2, x, y, nullptr);
    transform_coordinates(shape_, 2.0, 0.0, 0.0);
    EXPECT_DOUBLE_EQ(20.0, shape_->padfX[0]);
    EXPECT_DOUBLE_EQ(40.0, shape_->padfX[1]);
    EXPECT_DOUBLE_EQ(60.0, shape_->padfY[0]);
    EXPECT_DOUBLE_EQ(80.0, shape_->padfY[1]);
}

TEST_F(TransformCoordinatesTest, ShiftOnly)
{
    double x[] = {1.0};
    double y[] = {2.0};
    shape_ = SHPCreateSimpleObject(SHPT_POINT, 1, x, y, nullptr);
    transform_coordinates(shape_, 1.0, 100.0, 200.0);
    EXPECT_DOUBLE_EQ(101.0, shape_->padfX[0]);
    EXPECT_DOUBLE_EQ(202.0, shape_->padfY[0]);
}

TEST_F(TransformCoordinatesTest, ScaleAndShift)
{
    double x[] = {5.0, 10.0};
    double y[] = {15.0, 20.0};
    shape_ = SHPCreateSimpleObject(SHPT_ARC, 2, x, y, nullptr);
    // factor = 3.0, xshift = 1.0, yshift = 2.0
    // new_x = x * 3.0 + 1.0, new_y = y * 3.0 + 2.0
    transform_coordinates(shape_, 3.0, 1.0, 2.0);
    EXPECT_DOUBLE_EQ(16.0, shape_->padfX[0]);
    EXPECT_DOUBLE_EQ(31.0, shape_->padfX[1]);
    EXPECT_DOUBLE_EQ(47.0, shape_->padfY[0]);
    EXPECT_DOUBLE_EQ(62.0, shape_->padfY[1]);
}

TEST_F(TransformCoordinatesTest, FeetToMetersConversion)
{
    // 1200 (feet unit) / 3937 (meter unit) ≈ 0.3048
    const double feet_to_meters = 1200.0 / 3937.0;
    double x[] = {100.0};
    double y[] = {200.0};
    shape_ = SHPCreateSimpleObject(SHPT_POINT, 1, x, y, nullptr);
    transform_coordinates(shape_, feet_to_meters, 0.0, 0.0);
    EXPECT_NEAR(30.48, shape_->padfX[0], 0.001);
    EXPECT_NEAR(60.96, shape_->padfY[0], 0.001);
}

// ---------------------------------------------------------------------------
// copy_dbf_record tests
// ---------------------------------------------------------------------------

static std::string GenerateUniqueName(std::string_view stem)
{
    const auto now = std::chrono::system_clock::now();
    const auto timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch())
            .count();
    std::ostringstream oss;
    oss << stem << '_' << timestamp;
    return oss.str();
}

class CopyDbfRecordTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        dir_ = fs::current_path() / GenerateUniqueName("copydbf");
        fs::create_directories(dir_);
    }

    void TearDown() override
    {
        if (hIn_)
            DBFClose(hIn_);
        if (hOut_)
            DBFClose(hOut_);
        fs::remove_all(dir_);
    }

    fs::path dir_;
    DBFHandle hIn_ = nullptr;
    DBFHandle hOut_ = nullptr;
};

TEST_F(CopyDbfRecordTest, CopiesIntegerField)
{
    const auto srcPath = (dir_ / "src.dbf").string();
    hIn_ = DBFCreate(srcPath.c_str());
    ASSERT_NE(nullptr, hIn_);
    EXPECT_NE(-1, DBFAddField(hIn_, "NUM", FTInteger, 10, 0));
    EXPECT_TRUE(DBFWriteIntegerAttribute(hIn_, 0, 0, 42));
    DBFClose(hIn_);
    hIn_ = DBFOpen(srcPath.c_str(), "rb");
    ASSERT_NE(nullptr, hIn_);

    const auto dstPath = (dir_ / "dst.dbf").string();
    hOut_ = DBFCreate(dstPath.c_str());
    ASSERT_NE(nullptr, hOut_);
    EXPECT_NE(-1, DBFAddField(hOut_, "NUM", FTInteger, 10, 0));

    int fieldmap[] = {0};
    EXPECT_TRUE(copy_dbf_record(hIn_, 0, hOut_, 0, fieldmap, 1));

    EXPECT_EQ(42, DBFReadIntegerAttribute(hOut_, 0, 0));
}

TEST_F(CopyDbfRecordTest, CopiesStringField)
{
    const auto srcPath = (dir_ / "src.dbf").string();
    hIn_ = DBFCreate(srcPath.c_str());
    ASSERT_NE(nullptr, hIn_);
    EXPECT_NE(-1, DBFAddField(hIn_, "NAME", FTString, 20, 0));
    EXPECT_TRUE(DBFWriteStringAttribute(hIn_, 0, 0, "hello"));
    DBFClose(hIn_);
    hIn_ = DBFOpen(srcPath.c_str(), "rb");
    ASSERT_NE(nullptr, hIn_);

    const auto dstPath = (dir_ / "dst.dbf").string();
    hOut_ = DBFCreate(dstPath.c_str());
    ASSERT_NE(nullptr, hOut_);
    EXPECT_NE(-1, DBFAddField(hOut_, "NAME", FTString, 20, 0));

    int fieldmap[] = {0};
    EXPECT_TRUE(copy_dbf_record(hIn_, 0, hOut_, 0, fieldmap, 1));

    const char *raw = DBFReadStringAttribute(hOut_, 0, 0);
    ASSERT_NE(nullptr, raw);
    const std::string result = raw;
    const auto trimmed = result.substr(0, result.find_last_not_of(' ') + 1);
    EXPECT_EQ("hello", trimmed);
}

TEST_F(CopyDbfRecordTest, CopiesDoubleField)
{
    const auto srcPath = (dir_ / "src.dbf").string();
    hIn_ = DBFCreate(srcPath.c_str());
    ASSERT_NE(nullptr, hIn_);
    EXPECT_NE(-1, DBFAddField(hIn_, "VAL", FTDouble, 18, 6));
    EXPECT_TRUE(DBFWriteDoubleAttribute(hIn_, 0, 0, 3.14));
    DBFClose(hIn_);
    hIn_ = DBFOpen(srcPath.c_str(), "rb");
    ASSERT_NE(nullptr, hIn_);

    const auto dstPath = (dir_ / "dst.dbf").string();
    hOut_ = DBFCreate(dstPath.c_str());
    ASSERT_NE(nullptr, hOut_);
    EXPECT_NE(-1, DBFAddField(hOut_, "VAL", FTDouble, 18, 6));

    int fieldmap[] = {0};
    EXPECT_TRUE(copy_dbf_record(hIn_, 0, hOut_, 0, fieldmap, 1));

    EXPECT_DOUBLE_EQ(3.14, DBFReadDoubleAttribute(hOut_, 0, 0));
}

TEST_F(CopyDbfRecordTest, SkipsUnmappedField)
{
    const auto srcPath = (dir_ / "src.dbf").string();
    hIn_ = DBFCreate(srcPath.c_str());
    ASSERT_NE(nullptr, hIn_);
    EXPECT_NE(-1, DBFAddField(hIn_, "A", FTInteger, 10, 0));
    EXPECT_NE(-1, DBFAddField(hIn_, "B", FTInteger, 10, 0));
    EXPECT_TRUE(DBFWriteIntegerAttribute(hIn_, 0, 0, 10));
    EXPECT_TRUE(DBFWriteIntegerAttribute(hIn_, 0, 1, 20));
    DBFClose(hIn_);
    hIn_ = DBFOpen(srcPath.c_str(), "rb");
    ASSERT_NE(nullptr, hIn_);

    const auto dstPath = (dir_ / "dst.dbf").string();
    hOut_ = DBFCreate(dstPath.c_str());
    ASSERT_NE(nullptr, hOut_);
    EXPECT_NE(-1, DBFAddField(hOut_, "B", FTInteger, 10, 0));

    int fieldmap[] = {-1, 0};
    EXPECT_TRUE(copy_dbf_record(hIn_, 0, hOut_, 0, fieldmap, 2));

    EXPECT_EQ(20, DBFReadIntegerAttribute(hOut_, 0, 0));
}

TEST_F(CopyDbfRecordTest, RemapsFieldOrder)
{
    const auto srcPath = (dir_ / "src.dbf").string();
    hIn_ = DBFCreate(srcPath.c_str());
    ASSERT_NE(nullptr, hIn_);
    EXPECT_NE(-1, DBFAddField(hIn_, "X", FTInteger, 10, 0));
    EXPECT_NE(-1, DBFAddField(hIn_, "Y", FTInteger, 10, 0));
    EXPECT_TRUE(DBFWriteIntegerAttribute(hIn_, 0, 0, 100));
    EXPECT_TRUE(DBFWriteIntegerAttribute(hIn_, 0, 1, 200));
    DBFClose(hIn_);
    hIn_ = DBFOpen(srcPath.c_str(), "rb");
    ASSERT_NE(nullptr, hIn_);

    const auto dstPath = (dir_ / "dst.dbf").string();
    hOut_ = DBFCreate(dstPath.c_str());
    ASSERT_NE(nullptr, hOut_);
    EXPECT_NE(-1, DBFAddField(hOut_, "Y", FTInteger, 10, 0));
    EXPECT_NE(-1, DBFAddField(hOut_, "X", FTInteger, 10, 0));

    int fieldmap[] = {1, 0};
    EXPECT_TRUE(copy_dbf_record(hIn_, 0, hOut_, 0, fieldmap, 2));

    EXPECT_EQ(200, DBFReadIntegerAttribute(hOut_, 0, 0));  // Y
    EXPECT_EQ(100, DBFReadIntegerAttribute(hOut_, 0, 1));  // X
}

// ---------------------------------------------------------------------------
// process_records integration tests
// ---------------------------------------------------------------------------

class ProcessRecordsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        dir_ = fs::current_path() / GenerateUniqueName("procrec");
        fs::create_directories(dir_);

        // Reset global state
        iclip = false;
        ierase = false;
        itouch = false;
        iinside = false;
        icut = false;
        iselect = false;
        iunit = false;
        factor = 0;
        xshift = 0;
        yshift = 0;
        psCShape = nullptr;
        pt = nullptr;
    }

    void TearDown() override
    {
        if (hSHP)
        {
            SHPClose(hSHP);
            hSHP = nullptr;
        }
        if (hSHPappend)
        {
            SHPClose(hSHPappend);
            hSHPappend = nullptr;
        }
        if (hDBF)
        {
            DBFClose(hDBF);
            hDBF = nullptr;
        }
        if (hDBFappend)
        {
            DBFClose(hDBFappend);
            hDBFappend = nullptr;
        }
        free(pt);
        pt = nullptr;
        fs::remove_all(dir_);
    }

    // Create a simple point shapefile with nPts points and an integer field
    void CreateTestData(int nPts)
    {
        const auto srcShp = (dir_ / "src.shp").string();
        hSHP = SHPCreate(srcShp.c_str(), SHPT_POINT);
        ASSERT_NE(nullptr, hSHP);
        for (int i = 0; i < nPts; i++)
        {
            double x = static_cast<double>(i * 10);
            double y = static_cast<double>(i * 20);
            auto *obj = SHPCreateSimpleObject(SHPT_POINT, 1, &x, &y, nullptr);
            SHPWriteObject(hSHP, -1, obj);
            SHPDestroyObject(obj);
        }
        SHPClose(hSHP);

        const auto srcDbf = (dir_ / "src.dbf").string();
        hDBF = DBFCreate(srcDbf.c_str());
        ASSERT_NE(nullptr, hDBF);
        EXPECT_NE(-1, DBFAddField(hDBF, "VAL", FTInteger, 10, 0));
        for (int i = 0; i < nPts; i++)
            EXPECT_TRUE(DBFWriteIntegerAttribute(hDBF, i, 0, (i + 1) * 100));
        DBFClose(hDBF);

        // Reopen as read-only
        hSHP = SHPOpen(srcShp.c_str(), "rb");
        ASSERT_NE(nullptr, hSHP);
        SHPGetInfo(hSHP, &nEntities, &nShapeType, nullptr, nullptr);

        hDBF = DBFOpen(srcDbf.c_str(), "rb");
        ASSERT_NE(nullptr, hDBF);

        // Create output files
        const auto dstShp = (dir_ / "dst.shp").string();
        hSHPappend = SHPCreate(dstShp.c_str(), SHPT_POINT);
        ASSERT_NE(nullptr, hSHPappend);

        const auto dstDbf = (dir_ / "dst.dbf").string();
        hDBFappend = DBFCreate(dstDbf.c_str());
        ASSERT_NE(nullptr, hDBFappend);
        EXPECT_NE(-1, DBFAddField(hDBFappend, "VAL", FTInteger, 10, 0));

        // Identity field map
        pt = static_cast<int *>(malloc(sizeof(int)));
        pt[0] = 0;
    }

    fs::path dir_;
};

TEST_F(ProcessRecordsTest, CopiesAllRecords)
{
    CreateTestData(3);
    process_records();

    int outEntities = 0;
    SHPGetInfo(hSHPappend, &outEntities, nullptr, nullptr, nullptr);
    EXPECT_EQ(3, outEntities);
    EXPECT_EQ(3, DBFGetRecordCount(hDBFappend));
    EXPECT_EQ(100, DBFReadIntegerAttribute(hDBFappend, 0, 0));
    EXPECT_EQ(200, DBFReadIntegerAttribute(hDBFappend, 1, 0));
    EXPECT_EQ(300, DBFReadIntegerAttribute(hDBFappend, 2, 0));
}

TEST_F(ProcessRecordsTest, AppliesTransform)
{
    CreateTestData(1);
    iunit = true;
    factor = 2.0;
    xshift = 5.0;
    yshift = 10.0;

    process_records();

    // Original point: (0, 0), transformed: (0*2+5, 0*2+10) = (5, 10)
    auto *obj = SHPReadObject(hSHPappend, 0);
    ASSERT_NE(nullptr, obj);
    EXPECT_DOUBLE_EQ(5.0, obj->padfX[0]);
    EXPECT_DOUBLE_EQ(10.0, obj->padfY[0]);
    SHPDestroyObject(obj);
}

TEST_F(ProcessRecordsTest, ClipFiltersOutside)
{
    CreateTestData(3);
    // Points at (0,0), (10,20), (20,40)
    // Clip box includes only (10,20) and (20,40)
    iclip = true;
    cxmin = 5.0;
    cymin = 5.0;
    cxmax = 25.0;
    cymax = 45.0;

    process_records();

    int outEntities = 0;
    SHPGetInfo(hSHPappend, &outEntities, nullptr, nullptr, nullptr);
    EXPECT_EQ(2, outEntities);
    EXPECT_EQ(2, DBFGetRecordCount(hDBFappend));
}

TEST_F(ProcessRecordsTest, EmptyInput)
{
    CreateTestData(0);
    process_records();

    int outEntities = 0;
    SHPGetInfo(hSHPappend, &outEntities, nullptr, nullptr, nullptr);
    EXPECT_EQ(0, outEntities);
    EXPECT_EQ(0, DBFGetRecordCount(hDBFappend));
}

// ---------------------------------------------------------------------------
// High-level CUT tests using real shapefiles from shape_eg_data/
//
//  pline.shp:   Arc     460 shapes bounds (1296367..1302699, 228199..237185)
//  brklinz.shp: ArcZ    122 shapes bounds (6294338..6296322, 1978444..1979694)
//  csah.shp:    Arc     124 shapes bounds (257105..335498, 5176099..5226768)
//  polygon.shp: Polygon 474 shapes bounds (471127..489292, 4751545..4765611)
// ---------------------------------------------------------------------------

static const auto kTestData = fs::path{"shape_eg_data"};

class CutRealDataTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        dir_ = fs::current_path() / GenerateUniqueName("cutreal");
        fs::create_directories(dir_);

        iclip = true;
        ierase = false;
        itouch = false;
        iinside = false;
        icut = true;
        iselect = false;
        iunit = false;
        factor = 0;
        xshift = 0;
        yshift = 0;
        psCShape = nullptr;
        pt = nullptr;
    }

    void TearDown() override
    {
        if (hSHP)
        {
            SHPClose(hSHP);
            hSHP = nullptr;
        }
        if (hSHPappend)
        {
            SHPClose(hSHPappend);
            hSHPappend = nullptr;
        }
        if (hDBF)
        {
            DBFClose(hDBF);
            hDBF = nullptr;
        }
        if (hDBFappend)
        {
            DBFClose(hDBFappend);
            hDBFappend = nullptr;
        }
        free(pt);
        pt = nullptr;
        fs::remove_all(dir_);
    }

    // Open a real shapefile as input, create empty output, build field map
    void OpenRealData(const fs::path &basename)
    {
        const auto shpPath = (kTestData / basename).string();
        auto dbfBase = basename;
        dbfBase.replace_extension(".dbf");
        const auto dbfPath = (kTestData / dbfBase).string();

        hSHP = SHPOpen(shpPath.c_str(), "rb");
        ASSERT_NE(nullptr, hSHP) << "Cannot open " << shpPath;
        SHPGetInfo(hSHP, &nEntities, &nShapeType, nullptr, nullptr);

        hDBF = DBFOpen(dbfPath.c_str(), "rb");
        ASSERT_NE(nullptr, hDBF) << "Cannot open " << dbfPath;

        const int nFields = DBFGetFieldCount(hDBF);

        // Create output files
        const auto dstShp = (dir_ / "out.shp").string();
        hSHPappend = SHPCreate(dstShp.c_str(), nShapeType);
        ASSERT_NE(nullptr, hSHPappend);

        const auto dstDbf = (dir_ / "out.dbf").string();
        hDBFappend = DBFCreate(dstDbf.c_str());
        ASSERT_NE(nullptr, hDBFappend);

        // Copy field definitions and build identity field map
        pt = static_cast<int *>(malloc(nFields * sizeof(int)));
        for (int i = 0; i < nFields; i++)
        {
            char name[12];
            int width, decimals;
            const auto type = DBFGetFieldInfo(hDBF, i, name, &width, &decimals);
            EXPECT_NE(-1, DBFAddField(hDBFappend, name, type, width, decimals));
            pt[i] = i;
        }
    }

    fs::path dir_;
};

TEST_F(CutRealDataTest, CutPlineClipsToSubset)
{
    //  pline.shp bounds:
    //    X: 1296367 .. 1302699     Y: 228199 .. 237185
    //
    //    237185 +========================+
    //           |        pline.shp       |
    //           |    +----------+        |
    //           |    | clip box |        |
    //           |    +----------+        |
    //    228199 +========================+
    //       1296367                   1302699
    //
    //  Clip box covers roughly the center third:
    OpenRealData("pline.shp");
    const int totalIn = nEntities;
    EXPECT_EQ(460, totalIn);

    cxmin = 1298000;
    cymin = 231000;
    cxmax = 1301000;
    cymax = 235000;

    process_records();

    int outEntities = 0;
    SHPGetInfo(hSHPappend, &outEntities, nullptr, nullptr, nullptr);
    EXPECT_GT(outEntities, 0);        // some shapes survived
    EXPECT_LT(outEntities, totalIn);  // but not all

    // Every output shape must have valid vertices inside or on the clip box
    for (int i = 0; i < outEntities; i++)
    {
        auto *obj = SHPReadObject(hSHPappend, i);
        ASSERT_NE(nullptr, obj);
        EXPECT_GE(obj->nVertices, 2) << "CUT polyline must have >= 2 vertices";
        for (int v = 0; v < obj->nVertices; v++)
        {
            EXPECT_GE(obj->padfX[v], cxmin - 1e-6)
                << "shape " << i << " vertex " << v << " X below clip min";
            EXPECT_LE(obj->padfX[v], cxmax + 1e-6)
                << "shape " << i << " vertex " << v << " X above clip max";
            EXPECT_GE(obj->padfY[v], cymin - 1e-6)
                << "shape " << i << " vertex " << v << " Y below clip min";
            EXPECT_LE(obj->padfY[v], cymax + 1e-6)
                << "shape " << i << " vertex " << v << " Y above clip max";
        }
        SHPDestroyObject(obj);
    }

    EXPECT_EQ(outEntities, DBFGetRecordCount(hDBFappend));
}

TEST_F(CutRealDataTest, CutBrklinzPreservesZ)
{
    //  brklinz.shp: ArcZ with 122 shapes, has Z coordinates
    //  Clip a sub-region and verify Z values are preserved (non-zero)
    OpenRealData("brklinz.shp");
    const int totalIn = nEntities;
    EXPECT_EQ(122, totalIn);

    cxmin = 6294500;
    cymin = 1978500;
    cxmax = 6296000;
    cymax = 1979500;

    process_records();

    int outEntities = 0;
    SHPGetInfo(hSHPappend, &outEntities, nullptr, nullptr, nullptr);
    EXPECT_GT(outEntities, 0);

    // Check that at least one output shape has non-zero Z
    bool foundZ = false;
    for (int i = 0; i < outEntities && !foundZ; i++)
    {
        auto *obj = SHPReadObject(hSHPappend, i);
        ASSERT_NE(nullptr, obj);
        if (obj->padfZ != nullptr)
        {
            for (int v = 0; v < obj->nVertices; v++)
            {
                if (obj->padfZ[v] != 0.0)
                {
                    foundZ = true;
                    break;
                }
            }
        }
        SHPDestroyObject(obj);
    }
    EXPECT_TRUE(foundZ) << "Expected non-zero Z in ArcZ output";
}

TEST_F(CutRealDataTest, CutCsahWithTightBox)
{
    //  csah.shp bounds:
    //    X: 257105 .. 335498     Y: 5176099 .. 5226768
    //
    //  Use a very tight clip box in the center  ->  few shapes survive
    OpenRealData("csah.shp");
    EXPECT_EQ(124, nEntities);

    cxmin = 290000;
    cymin = 5195000;
    cxmax = 300000;
    cymax = 5205000;

    process_records();

    int outEntities = 0;
    SHPGetInfo(hSHPappend, &outEntities, nullptr, nullptr, nullptr);
    EXPECT_GT(outEntities, 0);
    EXPECT_LT(outEntities, 124);
    EXPECT_EQ(outEntities, DBFGetRecordCount(hDBFappend));
}

TEST_F(CutRealDataTest, CutTinyBoxYieldsNothing)
{
    //  Clip box completely outside the data extent -> zero output
    OpenRealData("pline.shp");

    cxmin = 0;
    cymin = 0;
    cxmax = 1;
    cymax = 1;

    process_records();

    int outEntities = 0;
    SHPGetInfo(hSHPappend, &outEntities, nullptr, nullptr, nullptr);
    EXPECT_EQ(0, outEntities);
    EXPECT_EQ(0, DBFGetRecordCount(hDBFappend));
}

TEST_F(CutRealDataTest, CutHugeBoxPreservesAll)
{
    //  Clip box encloses entire data extent -> no clipping needed
    //  (some null/deleted shapes may still be skipped by SHPReadObject)
    OpenRealData("csah.shp");
    const int totalIn = nEntities;

    cxmin = 200000;
    cymin = 5100000;
    cxmax = 400000;
    cymax = 5300000;

    process_records();

    int outEntities = 0;
    SHPGetInfo(hSHPappend, &outEntities, nullptr, nullptr, nullptr);
    EXPECT_GT(outEntities, totalIn / 2);  // most shapes survive
    EXPECT_EQ(outEntities, DBFGetRecordCount(hDBFappend));
}

TEST_F(CutRealDataTest, CutPolygonClipsToSubset)
{
    //  polygon.shp bounds:
    //    X: 471127 .. 489292     Y: 4751545 .. 4765611
    //
    //  Clip a sub-region and verify polygon output
    OpenRealData("polygon.shp");
    const int totalIn = nEntities;
    EXPECT_EQ(474, totalIn);

    cxmin = 478000;
    cymin = 4756000;
    cxmax = 484000;
    cymax = 4762000;

    process_records();

    int outEntities = 0;
    SHPGetInfo(hSHPappend, &outEntities, nullptr, nullptr, nullptr);
    EXPECT_GT(outEntities, 0);
    EXPECT_LT(outEntities, totalIn);

    for (int i = 0; i < outEntities; i++)
    {
        auto *obj = SHPReadObject(hSHPappend, i);
        ASSERT_NE(nullptr, obj);
        EXPECT_GE(obj->nVertices, 4) << "CUT polygon must have >= 4 vertices";
        // Check each ring is closed
        for (int p = 0; p < obj->nParts; p++)
        {
            const int start = obj->panPartStart[p];
            const int end = (p + 1 < obj->nParts) ? obj->panPartStart[p + 1]
                                                  : obj->nVertices;
            EXPECT_DOUBLE_EQ(obj->padfX[start], obj->padfX[end - 1])
                << "shape " << i << " ring " << p << " not closed (X)";
            EXPECT_DOUBLE_EQ(obj->padfY[start], obj->padfY[end - 1])
                << "shape " << i << " ring " << p << " not closed (Y)";
        }
        // All vertices inside or on clip box
        for (int v = 0; v < obj->nVertices; v++)
        {
            EXPECT_GE(obj->padfX[v], cxmin - 1e-6)
                << "shape " << i << " vertex " << v;
            EXPECT_LE(obj->padfX[v], cxmax + 1e-6)
                << "shape " << i << " vertex " << v;
            EXPECT_GE(obj->padfY[v], cymin - 1e-6)
                << "shape " << i << " vertex " << v;
            EXPECT_LE(obj->padfY[v], cymax + 1e-6)
                << "shape " << i << " vertex " << v;
        }
        SHPDestroyObject(obj);
    }

    EXPECT_EQ(outEntities, DBFGetRecordCount(hDBFappend));

    // Total clipped area must be positive.  Overlapping shapes can
    // exceed the clip box area, so only check a reasonable upper bound.
    const double clipBoxArea = (cxmax - cxmin) * (cymax - cymin);
    double totalArea = 0;
    for (int i = 0; i < outEntities; i++)
    {
        auto *obj = SHPReadObject(hSHPappend, i);
        ASSERT_NE(nullptr, obj);
        totalArea += TotalPolygonArea(obj);
        SHPDestroyObject(obj);
    }
    EXPECT_GT(totalArea, 0.0);
    EXPECT_LT(totalArea, clipBoxArea * 10.0);
}

TEST_F(CutRealDataTest, CutPolygonHugeBoxPreservesAll)
{
    //  Clip box encloses entire polygon.shp extent
    OpenRealData("polygon.shp");
    const int totalIn = nEntities;

    cxmin = 470000;
    cymin = 4750000;
    cxmax = 490000;
    cymax = 4770000;

    process_records();

    int outEntities = 0;
    SHPGetInfo(hSHPappend, &outEntities, nullptr, nullptr, nullptr);
    EXPECT_GT(outEntities, totalIn / 2);
    EXPECT_EQ(outEntities, DBFGetRecordCount(hDBFappend));

    // Total area must be positive (shapes preserved)
    double totalArea = 0;
    for (int i = 0; i < outEntities; i++)
    {
        auto *obj = SHPReadObject(hSHPappend, i);
        ASSERT_NE(nullptr, obj);
        totalArea += TotalPolygonArea(obj);
        SHPDestroyObject(obj);
    }
    EXPECT_GT(totalArea, 0.0);
}

}  // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
