// Gmsh - Copyright (C) 1997-2019 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// issues on https://gitlab.onelab.info/gmsh/gmsh/issues.

#include <vector>
#include <stack>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include "OS.h"
#include "GmshConfig.h"
#include "fastScaledCrossField.h"
#include "GModel.h"
#include "GFace.h"
#include "MEdge.h"
#include "MEdgeHash.h"
#include "MLine.h"
#include "MTriangle.h"
#include "GmshMessage.h"
#include "Context.h"
#include "discreteEdge.h"
#include "discreteFace.h"
#include "Numeric.h"
#include "gmsh.h"

#if defined(HAVE_POST)
#include "PView.h"
#include "PViewDataGModel.h"
#endif

#if defined(_OPENMP)
#include <omp.h>
#endif

#if defined(HAVE_SOLVER) && defined(HAVE_POST)
#include "dofManager.h"
#include "laplaceTerm.h"
#include "linearSystemGmm.h"
#include "linearSystemCSR.h"
#include "linearSystemFull.h"
#include "linearSystemPETSc.h"
#include "linearSystemMUMPS.h"
#endif

#ifdef HAVE_QUADMESHINGTOOLS
#include "qmt_cross_field.h"
#include "qmt_utils.hpp"
#include "geolog.h"
#endif


int getFacesTriangles(const std::vector<GFace*>& faces, std::vector<MTriangle*>& triangles) {
  triangles.clear();
  size_t count = 0;
  for (GFace* gf: faces) count += gf->triangles.size();
  triangles.resize(count,NULL);
  count = 0;
  for (GFace* gf: faces) for (MTriangle* t: gf->triangles) {
    triangles[count] = t;
    count += 1;
  }
  return 0;
}

int extractTriangularMeshFromFaces(
    const std::vector<GFace*>& faces,
    std::vector<std::array<double,3> >& points,
    std::vector<MVertex*>& origin,
    std::vector<std::array<size_t,2> >& lines,
    std::vector<std::array<size_t,3> >& triangles) {
  Msg::Debug("extract edge lines and face triangles from %i faces",faces.size());
  points.clear();
  lines.clear();
  triangles.clear();
  std::vector<size_t> old2new;
  size_t X = std::numeric_limits<size_t>::max();
  std::set<GEdge*> edges;
  for (GFace* gf: faces) {
    for (GEdge* ge: gf->edges()) {
      edges.insert(ge);
    }
    for (GEdge* ge: gf->embeddedEdges()) {
      edges.insert(ge);
    }
    old2new.reserve(old2new.size()+gf->triangles.size());
    points.reserve(points.size()+gf->triangles.size());
    origin.reserve(points.size()+gf->triangles.size());
    triangles.reserve(triangles.size()+gf->triangles.size());
    for (MTriangle* t: gf->triangles) {
      std::array<size_t,3> tri;
      for (size_t lv = 0; lv < 3; ++lv) {
        MVertex* v = t->getVertex(lv);
        size_t num = v->getNum();
        if (num >= old2new.size()) {
          old2new.resize(num+1,X);
        }
        if (old2new[num] == X) {
          old2new[num] = points.size();
          std::array<double,3> pt = {v->x(), v->y(), v->z()};
          points.push_back(pt);
          origin.push_back(v);
        }
        tri[lv] = old2new[num];
      }
      triangles.push_back(tri);
    }
  }
  for (GEdge* ge: edges) {
    lines.reserve(lines.size()+ge->lines.size());
    for (MLine* l: ge->lines) {
      std::array<size_t,2> line;
      for (size_t lv = 0; lv < 2; ++lv) {
        MVertex* v = l->getVertex(lv);
        size_t num = v->getNum();
        if (num >= old2new.size()) {
          old2new.resize(num+1,X);
        }
        if (old2new[num] == X) {
          old2new[num] = points.size();
          std::array<double,3> pt = {v->x(), v->y(), v->z()};
          points.push_back(pt);
          origin.push_back(v);
        }
        line[lv] = old2new[num];
      }
      std::sort(line.begin(),line.end());
      lines.push_back(line);
    }
  }
  Msg::Debug("- %li points, %i lines, %i triangles", points.size(), lines.size(), triangles.size());
  return 0;
}

int computeCrossFieldWithHeatEquation(
    const std::vector<GFace*>& faces, 
    std::unordered_map<MEdge,double,MEdgeHash,MEdgeEqual>& edgeTheta,
    int nbDiffusionLevels, double thresholdNormConvergence, int nbBoundaryExtensionLayer, int verbosity) {
  /* note: edgeTheta keys (v1,v2) are sorted, v1 < v2 always */
  Msg::Debug("compute cross field with heat equation for %i faces ...", faces.size());
#ifdef HAVE_QUADMESHINGTOOLS
  std::vector<std::array<double,3> > points;
  std::vector<std::array<size_t,2> > lines;
  std::vector<std::array<size_t,3> > triangles;
  std::vector<MVertex*> origin;
  int status = extractTriangularMeshFromFaces(faces, points, origin, lines, triangles);
  if (status != 0) {
    Msg::Error("Failed to extract triangular mesh");
    return -1;
  }
  std::map<std::array<size_t,2>,double> edgeThetaLocal;
  bool okcf = QMT::compute_cross_field_with_multilevel_diffusion(
      points,lines,triangles,edgeThetaLocal,nbDiffusionLevels,
      thresholdNormConvergence, nbBoundaryExtensionLayer, verbosity);
  if (!okcf) {
    Msg::Error("Failed to compute cross field for given faces");
    return -1;
  }

#if defined(_OPENMP)
#pragma omp critical (append_theta)
#endif
  for (const auto& kv: edgeThetaLocal) {
    MEdge edge(origin[kv.first[0]],origin[kv.first[1]]);
    edgeTheta[edge] = kv.second;
  }

#else
  Msg::Error("Diffusion-based cross field computation requires the QuadMeshingTools module");
  return -1;
#endif
  return 0;
}

static inline double compat_orientation_extrinsic(const SVector3 &o0,
    const SVector3 &n0,
    const SVector3 &o1,
    const SVector3 &n1,
    SVector3 &a1, SVector3 &b1)
{
  SVector3 t0 = crossprod(n0, o0);
  SVector3 t1 = crossprod(n1, o1);

  const size_t permuts[8][2] = {{0, 0}, {1, 0}, {2, 0}, {3, 0},
    {0, 1}, {1, 1}, {2, 1}, {3, 1}};
  SVector3 A[4]{o0, t0, -o0, -t0};
  SVector3 B[2]{o1, t1};

  double maxx = -1;
  int index = 0;
  for(size_t i = 0; i < 8; i++) {
    const double xx = dot(A[permuts[i][0]], B[permuts[i][1]]);
    if(xx > maxx) {
      index = i;
      maxx = xx;
    }
  }
  a1 = A[permuts[index][0]];
  b1 = B[permuts[index][1]];
  return maxx;
}

static inline void cross_normalize(double &a)
{
  double D = M_PI * .5;
  if(a < 0)
    while(a < 0) a += D;
  if(a >= D)
    while(a >= D) a -= D;
}

static inline double cross_lifting(double _a, double a)
{
  double D = M_PI * .5;
  if(fabs(_a - a) < fabs(_a - (a + D)) && fabs(_a - a) < fabs(_a - (a - D))) {
    return a;
  }
  else if(fabs(_a - (a + D)) < fabs(_a - a) &&
      fabs(_a - (a + D)) < fabs(_a - (a - D))) {
    return a + D;
  }
  else {
    return a - D;
  }
  return DBL_MAX;
}

int getConnectedSurfaceComponents(const std::vector<GFace*>& faces,
    std::vector<std::vector<GFace*> >& components) {
  std::map<GEdge*,std::vector<GFace*> > edge2faces;
  for (GFace* gf: faces) for (GEdge* ge: gf->edges()) { 
    edge2faces[ge].push_back(gf);
  }
  std::set<GFace*> done;
  for (GFace* gfInit: faces) if (done.find(gfInit) == done.end()) {
    /* BFS */
    std::vector<GFace*> comp_faces;
    std::queue<GFace*> Q;
    Q.push(gfInit);
    done.insert(gfInit);
    while (Q.size() > 0) {
      GFace* gf = Q.front();
      Q.pop();
      comp_faces.push_back(gf);
      for (GEdge* ge: gf->edges()) {
        for (GFace* gf2: edge2faces[ge]) {
          if (gf2 == gf) continue;
          if (done.find(gf2) != done.end()) continue;
          Q.push(gf2);
          done.insert(gf2);
        }
      }
    }
    if (comp_faces.size() > 0) {
      std::sort( comp_faces.begin(), comp_faces.end() );
      comp_faces.erase( std::unique( comp_faces.begin(), comp_faces.end() ), comp_faces.end() );
      components.push_back(comp_faces);
    }
  }
  Msg::Debug("- found %li independant surface components", components.size());

  return 0;
}

static inline double clamp(double x, double lower, double upper) { return std::min(upper, std::max(x, lower)); }

int distributeQuadsBasedOnArea(
    size_t totalNumberOfQuads,
    const std::vector<std::vector<GFace*> >& components,
    std::vector<size_t>& componentNumberOfQuads) {
  double area_total = 0.;
  std::vector<double> areas(components.size(),0.);
  for (size_t i = 0; i < components.size(); ++i) {
    for (GFace* gf: components[i]) {
      for (MTriangle* t: gf->triangles) {
        double a = std::abs(t->getVolume());
        areas[i] += a;
        area_total += a;
      }
    }
  }
  if (area_total == 0.) {
    Msg::Error("Total triangulation area is 0.");
    return -1;
  }
  componentNumberOfQuads.resize(components.size(),0.);
  for (size_t i = 0; i < components.size(); ++i) {
    componentNumberOfQuads[i] = size_t(double(totalNumberOfQuads) * areas[i] / area_total);
    if (componentNumberOfQuads[i] == 0) componentNumberOfQuads[i] = 1;
  }
  return 0;
}

int computeQuadSizeMapFromCrossFieldConformalFactor(
    const std::vector<MTriangle*>& triangles, 
    std::size_t targetNumberOfQuads, 
    std::unordered_map<MVertex*,double>& scaling
    ) {
  Msg::Debug("compute quad size map from cross field conformal factor, to have %li quads ...", targetNumberOfQuads);
  if (targetNumberOfQuads == 0) {
    Msg::Error("targetNumberOfQuads: %i should be positive", targetNumberOfQuads);
    return -1;
  }

  double Hmin =  DBL_MAX;
  double Hmax = -DBL_MAX;
  for (MTriangle* t: triangles) {
    for (size_t lv = 0; lv < 3; ++lv) {
      MVertex* v = t->getVertex(lv);
      auto it = scaling.find(v);
      if (it == scaling.end()) {
        Msg::Error("scaling not found for vertex %i", v->getNum());
        return -1;
      }
      GVertex* gv = v->onWhat()->cast2Vertex();
      if (gv == nullptr) {
        Hmin = std::min(Hmin, it->second);
        Hmax = std::max(Hmax, it->second);
      }
    }
  }

  /* Compute integral of current size map */
  double integral = 0.;
  double smin = DBL_MAX;
  double smax = -DBL_MAX;
  std::vector<MVertex*> vertices;
  std::unordered_set<MVertex*> done;
  vertices.reserve(3*triangles.size());
  for (MTriangle* t: triangles) {
    double values[3] = {0,0,0};
    for (size_t lv = 0; lv < 3; ++lv) {
      MVertex* v = t->getVertex(lv);
      auto it = scaling.find(v);
      bool alreadyChanged = (done.find(v) != done.end());
      if (alreadyChanged) {
        values[lv] = it->second;;
      } else {
        double H = it->second;
        H = clamp(H, Hmin, Hmax);
        it->second = exp(-H); /* changing scaling ! */
        values[lv] = it->second;;
        smin = std::min(smin,values[lv]);
        smax = std::max(smax,values[lv]);
        vertices.push_back(v);
        done.insert(v);
      }
    }
    double a = std::abs(t->getVolume());
    integral += a * 1. / std::pow(1./3. * (values[0] + values[1] + values[2]),2);
  }
  if (integral == 0.) {
    Msg::Error("total integral is 0 ... (%li triangles, smin=%.3e, smax=%.3e)", triangles.size(), smin, smax);
    return -1;
  }
  Msg::Debug("-- component with %li triangles, exp(-H): min=%.3e, max=%.3e, #=%.3e", triangles.size(), smin, smax, integral);
  std::sort(vertices.begin(), vertices.end());
  vertices.erase(std::unique(vertices.begin(), vertices.end() ), vertices.end());

  /* Apply the scaling */
  double target = double(targetNumberOfQuads);
  double FAC = double(target) / integral;
  double sf = 1./std::sqrt(FAC);
  smin = DBL_MAX;
  smax = -DBL_MAX;
  for (size_t j = 0; j < vertices.size(); ++j) {
    MVertex* v = vertices[j];
    auto it = scaling.find(v);
    it->second = sf * it->second;
    smin = std::min(smin,it->second);
    smax = std::max(smax,it->second);
  }
  Msg::Debug("-- component with %li triangles, size map: min=%.3e, max=%.3e, factor applied: %.3f", triangles.size(), smin, smax, FAC);

  return 0;
}

int computeCrossFieldConformalScaling(
    const std::vector<MTriangle*>& triangles, 
    const std::unordered_map<MEdge,double,MEdgeHash,MEdgeEqual>& edgeTheta,
    std::unordered_map<MVertex*,double>& scaling)
{
  Msg::Debug("compute cross field scaling ...");
#if defined(HAVE_SOLVER)

#if defined(HAVE_PETSC)
  Msg::Debug("- with PETSc solver");
  linearSystemPETSc<double> *_lsys = new linearSystemPETSc<double>;
#elif defined(HAVE_MUMPS)
  Msg::Debug("- with MUMPS solver");
  linearSystemMUMPS<double> *_lsys = new linearSystemMUMPS<double>;
#else
  Msg::Debug("- with full solver (slow ..., should not happen !)");
  linearSystemFull<double> *_lsys = new linearSystemFull<double>;
#endif
  dofManager<double> *myAssembler = new dofManager<double>(_lsys);

  /* Vertex unknowns */
  std::set<MVertex *, MVertexPtrLessThan> vs;
  for (MTriangle* t: triangles) {
    for(size_t k = 0; k < 3; k++) { vs.insert(t->getVertex(k)); }
  }
  for (MVertex* v: vs) myAssembler->numberVertex(v,0,1);

  simpleFunction<double> ONE(1.0);
  laplaceTerm l(0, 1, &ONE);

  /* Collect gradient of theta (cross field directions) */
  std::map<MTriangle *, SVector3> gradients_of_theta;
  for (MTriangle* t: triangles) {
    SVector3 v10(t->getVertex(1)->x() - t->getVertex(0)->x(),
        t->getVertex(1)->y() - t->getVertex(0)->y(),
        t->getVertex(1)->z() - t->getVertex(0)->z());
    SVector3 v20(t->getVertex(2)->x() - t->getVertex(0)->x(),
        t->getVertex(2)->y() - t->getVertex(0)->y(),
        t->getVertex(2)->z() - t->getVertex(0)->z());
    SVector3 normal_to_triangle = crossprod(v20, v10);
    normal_to_triangle.normalize();

    SElement se(t);
    l.addToMatrix(*myAssembler, &se);

    SVector3 t_i, o_i, o_1, o_2, tgt0;
    for (size_t le = 0; le < 3; ++ le) {
      /* Edge ordered lower index to largest */
      std::array<MVertex*,2> edge = {t->getVertex(le),t->getVertex((le+1)%3)};
      if (edge[0]->getNum() > edge[1]->getNum()) std::reverse(edge.begin(),edge.end());

      /* Edge tangent */
      SVector3 tgt = edge[1]->point() - edge[0]->point();
      if (tgt.norm() < 1.e-16) {
        Msg::Warning("Edge (tri=%i,le=%i), length = %.e", t->getNum(), le, tgt.norm());
        if (tgt.norm() == 0.) {return -1;}
      }
      tgt.normalize();
      SVector3 tgt2 = crossprod(normal_to_triangle,tgt);
      tgt2.normalize();

      /* Cross angle  (from Crouzeix-Raviart cross field) */
      MEdge query = t->getEdge(le);
      auto it = edgeTheta.find(query);
      if (it == edgeTheta.end()) {
        Msg::Error("Edge (%li,%li) not found in edgeTheta",edge[0]->getNum(),edge[1]->getNum());
        return -1;
      }
      double A = it->second;

      SVector3 o = tgt * cos(A) + tgt2 * sin(A);
      o.normalize();
      o -= normal_to_triangle * dot(normal_to_triangle, o_i);
      o.normalize();

      if (le == 0) {
        t_i = crossprod(normal_to_triangle, tgt);
        t_i -= normal_to_triangle * dot(normal_to_triangle, t_i);
        t_i.normalize();
        o_i = o;
        tgt0 = tgt;
      } else if (le == 1) {
        o_1 = o;
      } else if (le == 2) {
        o_2 = o;
      }
    }

    SVector3 x0, x1, x2, x3;
    compat_orientation_extrinsic(o_i, normal_to_triangle, o_1, normal_to_triangle, x0, x1);
    compat_orientation_extrinsic(o_i, normal_to_triangle, o_2, normal_to_triangle, x2, x3);

    double a0 = atan2(dot(t_i, o_i), dot(tgt0, o_i));

    x0 -= normal_to_triangle * dot(normal_to_triangle, x0);
    x0.normalize();
    x1 -= normal_to_triangle * dot(normal_to_triangle, x1);
    x1.normalize();
    x2 -= normal_to_triangle * dot(normal_to_triangle, x2);
    x2.normalize();
    x3 -= normal_to_triangle * dot(normal_to_triangle, x3);
    x3.normalize();

    cross_normalize(a0);
    double A1 = atan2(dot(t_i, x1), dot(tgt0, x1));
    double A2 = atan2(dot(t_i, x3), dot(tgt0, x3));
    cross_normalize(A1);
    double a1 = cross_lifting(a0,A1);
    cross_normalize(A2);
    double a2 = cross_lifting(a0,A2);

    double a[3] = {a0 + a2 - a1, a0 + a1 - a2, a1 + a2 - a0};
    double g[3] = {0, 0, 0};
    t->interpolateGrad(a, 0, 0, 0, g);
    gradients_of_theta[t] = SVector3(g[0], g[1], g[2]);
    SPoint3 pp = t->barycenter();

    SVector3 G(g[0], g[1], g[2]);
    SVector3 GT = crossprod(G, normal_to_triangle);

    double g1[3];
    a[0] = 1;
    a[1] = 0;
    a[2] = 0;
    t->interpolateGrad(a, 0, 0, 0, g1);
    double RHS1 = g1[0] * GT.x() + g1[1] * GT.y() + g1[2] * GT.z();
    a[0] = 0;
    a[1] = 1;
    a[2] = 0;
    t->interpolateGrad(a, 0, 0, 0, g1);
    double RHS2 = g1[0] * GT.x() + g1[1] * GT.y() + g1[2] * GT.z();
    a[0] = 0;
    a[1] = 0;
    a[2] = 1;
    t->interpolateGrad(a, 0, 0, 0, g1);
    double RHS3 = g1[0] * GT.x() + g1[1] * GT.y() + g1[2] * GT.z();
    int num1 = myAssembler->getDofNumber(l.getLocalDofR(&se, 0));
    int num2 = myAssembler->getDofNumber(l.getLocalDofR(&se, 1));
    int num3 = myAssembler->getDofNumber(l.getLocalDofR(&se, 2));
    double V = -t->getVolume();
    _lsys->addToRightHandSide(num1, RHS1 * V);
    _lsys->addToRightHandSide(num2, RHS2 * V);
    _lsys->addToRightHandSide(num3, RHS3 * V);
  }
  _lsys->systemSolve();

  /* Extract solution */
  size_t i = 0;
  double vmin = DBL_MAX;
  double vmax = -DBL_MAX;
  for (MVertex* v: vs) {
    double h;
    myAssembler->getDofValue(v, 0, 1, h);
    vmin = std::min(vmin,h);
    vmax = std::max(vmax,h);
    scaling[v] = h;
    i += 1;
  }

  Msg::Info("Conformal Factor Computed (%d unknowns), H value range: [%.3f,%.3f], width = %.3f",
      myAssembler->sizeOfR(),vmin,vmax,vmax-vmin);

  if (false) {
    for (MTriangle* t: triangles) {
      std::vector<std::array<double,3> > pts(3);
      std::vector<double> vals(3);
      for (size_t lv = 0; lv < 3; ++lv) {
        MVertex* v = t->getVertex(lv);
        pts[lv] = {v->point().x(),v->point().y(),v->point().z()};
        vals[lv] = exp(-scaling[v]);
      }
      GeoLog::add(pts,vals,"cs (exp(-H)");
    }
    GeoLog::flush();
  }

  delete _lsys;

#else 
  Msg::Error("Computing cross field scaling requires the SOLVER module");
  return -1;
#endif

  return 0;
}

static inline SVector3 tri_normal(MTriangle* t) {
  SVector3 v10(t->getVertex(1)->x() - t->getVertex(0)->x(),
      t->getVertex(1)->y() - t->getVertex(0)->y(),
      t->getVertex(1)->z() - t->getVertex(0)->z());
  SVector3 v20(t->getVertex(2)->x() - t->getVertex(0)->x(),
      t->getVertex(2)->y() - t->getVertex(0)->y(),
      t->getVertex(2)->z() - t->getVertex(0)->z());
  SVector3 normal_to_triangle = crossprod(v20, v10);
  normal_to_triangle.normalize();
  return normal_to_triangle;
}

inline SVector3 crouzeix_raviart_interpolation(SVector3 lambda, SVector3 edge_values[3]) {
  double x = lambda[1];
  double y = lambda[2];
  SVector3 shape = {1.0 - 2.0 * y, -1.0 + 2.0 * (x + y), 1.0 - 2.0 * x};
  return shape[0] * edge_values[0] + shape[1] * edge_values[1] + shape[2] * edge_values[2];
}

int extractPerTriangleScaledCrossFieldDirections(
    const std::vector<MTriangle*>& triangles, 
    const std::unordered_map<MEdge,double,MEdgeHash,MEdgeEqual>& edgeTheta,
    const std::unordered_map<MVertex*,double>& scaling,
    std::unordered_map<MTriangle*, std::array<double,9> >& triangleDirections) {

  /* Combine cross directions and scaling, interpolate at triangle vertices */
  std::vector<double> datalist;
  for (MTriangle* t: triangles) {
    SVector3 N = tri_normal(t);

    /* Compute one branch at triangle vertices */
    SVector3 refCross = {0.,0.,0.};
    SVector3 avgCross = {0.,0.,0.};
    SVector3 lifted_dirs[3];
    for (size_t le = 0; le < 3; ++le) {
      /* Get cross angle */
      std::array<MVertex*,2> edge = {t->getVertex(le),t->getVertex((le+1)%3)};
      if (edge[0]->getNum() > edge[1]->getNum()) std::reverse(edge.begin(),edge.end());
      MEdge query = t->getEdge(le);
      auto it = edgeTheta.find(query);
      if (it == edgeTheta.end()) {
        Msg::Error("Edge (%li,%li) not found in edgeTheta",edge[0]->getNum(),edge[1]->getNum());
        return -1;
      }
      double A = it->second;

      /* Local reference frame */
      SVector3 tgt = edge[1]->point() - edge[0]->point();
      if (tgt.norm() < 1.e-16) {
        Msg::Warning("Edge (tri=%i,le=%i), length = %.e", t->getNum(), le, tgt.norm());
        if (tgt.norm() == 0.) {return -1;}
      }
      tgt.normalize();
      SVector3 tgt2 = crossprod(N,tgt);
      tgt2.normalize();

      SVector3 cross1 = tgt * cos(A) + tgt2 * sin(A);
      cross1.normalize();

      SVector3 cross2 = crossprod(N,cross1);
      cross2.normalize();

      if (le == 0) {
        refCross = cross1;
        avgCross = avgCross + cross1;
        lifted_dirs[le] = refCross;
      } else {
        SVector3 candidates[4] = {cross1,-1.*cross1,cross2,-1.*cross2};
        SVector3 closest = {0.,0.,0.};
        double dotmax = -DBL_MAX;
        for (size_t k = 0; k < 4; ++k) {
          if (dot(candidates[k],refCross) > dotmax) {
            closest = candidates[k];
            dotmax = dot(candidates[k],refCross);
          }
        }
        lifted_dirs[le] = closest;
        avgCross = avgCross + closest;
      }
    }
    SVector3 edge_dirs[3];
    for (size_t le = 0; le < 3; ++le) {
      double se = 0.5 * (scaling.at(t->getVertex(le)) + scaling.at(t->getVertex((le+1)%3)));
      edge_dirs[le] = se * lifted_dirs[le];
    }
    SVector3 vertex_dirs[3];
    std::array<double,9> tDirs;
    for (size_t lv = 0; lv < 3; ++lv) {
      SVector3 lambda = {0.,0.,0.};
      lambda[lv] = 1.;
      SVector3 dir = crouzeix_raviart_interpolation(lambda,edge_dirs);
      for (size_t d = 0; d < 3; ++d) {
        tDirs[3*lv+d] = dir.data()[d];
      }
    }
    triangleDirections[t] = tDirs;
  }

  return 0;
}

int createScaledCrossFieldView(
    const std::vector<MTriangle*>& triangles, 
    const std::unordered_map<MEdge,double,MEdgeHash,MEdgeEqual>& edgeTheta,
    const std::unordered_map<MVertex*,double>& scaling,
    const std::string& viewName,
    int& dataListViewTag
    ) {

#if defined(HAVE_POST)
  std::unordered_map<MTriangle*, std::array<double,9> > triangleDirections;
  int st = extractPerTriangleScaledCrossFieldDirections(triangles, edgeTheta, scaling, triangleDirections);
  if (st != 0) {
    Msg::Error("failed to extract per triangle scaled directions");
    return st;
  }

  /* Combine cross directions and scaling to create view */
  std::vector<double> datalist;
  for (MTriangle* t: triangles) {
    auto it = triangleDirections.find(t);
    if (it == triangleDirections.end()) {
      Msg::Error("Triangle %li not found in triangleDirections", t->getNum());
      return -1;
    }
    const std::array<double,9> tDirs = it->second;

    /* Add triangle coords+vectors to view */
    for (size_t d = 0; d < 3; ++d) {
      for (size_t lv = 0; lv < 3; ++lv) {
        datalist.push_back(t->getVertex(lv)->point().data()[d]);
      }
    }
    for (size_t lv = 0; lv < 3; ++lv) {
      for (size_t d = 0; d < 3; ++d) {
        datalist.push_back(tDirs[3*lv+d]);
      }
    }
  }

  gmsh::initialize();
  if (dataListViewTag < 0) {
    Msg::Debug("create view '%s'",viewName.c_str());
    dataListViewTag = gmsh::view::add(viewName);
  } else {
    Msg::Debug("overwrite view with tag %i",dataListViewTag);
  }
  gmsh::view::addListData(dataListViewTag, "VT", datalist.size()/(9+9), datalist);
#else 
  Msg::Error("Cannot create view without POST module");
  return -1;
#endif
  return 0;
}

int ordered_fan_around_vertex(const MVertex* v, const std::vector<MTriangle*>& tris,
    std::vector<std::array<MVertex*,2> >& vPairs) {
  vPairs.clear();
  if (tris.size() < 3) return -1;

  std::unordered_set<MTriangle*> done;
  { /* Init */
    MTriangle* t = tris[0];
    for (size_t le = 0; le < 3; ++le) {
      MVertex* v1 = t->getVertex(le);
      MVertex* v2 = t->getVertex((le+1)%3);
      if (v1 != v && v2 != v) {
        vPairs.push_back({v1,v2});
        done.insert(t);
        break;
      }
    }
  }

  while (vPairs.back().back() != vPairs[0][0]) {
    MVertex* last = vPairs.back().back();
    /* Look for next edge connected to last */
    bool found = false;
    for (MTriangle* t: tris) {
      if (done.find(t) != done.end()) continue;
      for (size_t le = 0; le < 3; ++le) {
        MVertex* v1 = t->getVertex(le);
        MVertex* v2 = t->getVertex((le+1)%3);
        if (v1 != v && v2 != v) {
          if (v1 == last) {
            vPairs.push_back({v1,v2});
            done.insert(t);
            found = true;
            break;
          } else if (v2 == last) {
            vPairs.push_back({v2,v1});
            done.insert(t);
            found = true;
            break;
          }
        }
      }
      if (found) break;
    }
    if (!found) {
      Msg::Error("failed to order fan vertex pairs");
      return -1;
    }
  }

  return 0;
}

int local_frame(const MVertex* v, const std::vector<MTriangle*>& tris, 
    SVector3& e_x, SVector3& e_y, SVector3& e_z) {
  e_x = SVector3(DBL_MAX,DBL_MAX,DBL_MAX);
  SVector3 avg(0.,0.,0.);
  for (MTriangle* t: tris) {
    avg += tri_normal(t);
    if (e_x.x() == DBL_MAX) {
      for (size_t le = 0; le < 3; ++le) {
        MVertex* v1 = t->getVertex(le);
        MVertex* v2 = t->getVertex((le+1)%3);
        if (v1 == v) {
          e_x = v2->point() - v1->point();
          break;
        } else if (v2 == v) {
          e_x = v1->point() - v2->point();
          break;
        }
      }
    }
  }
  if (avg.norm() < 1.e-16) {
    Msg::Error("local frame for %li triangles: avg normal = %.2f, %.2f, %.2f", 
        tris.size(), avg.x(), avg.y(), avg.z());
    return -1;
  }
  if (e_x.norm() < 1.e-16) {
    Msg::Error("local frame for %li triangles: e_x = %.2f, %.2f, %.2f", 
        tris.size(), e_x.x(), e_x.y(), e_x.z());
    return -1;
  }
  e_x.normalize();
  avg.normalize();
  e_z = avg;
  e_y = crossprod(e_z,e_x);
  if (e_y.norm() < 1.e-16) {
    Msg::Error("local frame for %li triangles: e_y = %.2f, %.2f, %.2f", 
        tris.size(), e_y.x(), e_y.y(), e_y.z());
    return -1;
  }
  e_y.normalize();

  return 0;
}
inline double angle2PI(const SVector3& u, const SVector3& v, const SVector3& n) {
  const double dp = dot(u,v);
  const double tp = dot(n,crossprod(u,v));
  double angle = atan2(tp,dp);
  if (angle < 0) angle += 2. * M_PI;
  return angle;
}

int detectCrossFieldSingularities(
    const std::vector<GFace*>& faces, 
    const std::unordered_map<MEdge,double,MEdgeHash,MEdgeEqual>& edgeTheta,
    std::vector<std::array<double,5> >& singularities) {

  std::vector<std::vector<MTriangle*> > numToTris;
  std::vector<MVertex*> vertices;
  for (GFace* gf: faces) {
    vertices.clear();
    numToTris.clear();

    /* Get vertices and adjacent triangles */
    for (MTriangle* t: gf->triangles) {
      for (size_t le = 0; le < 3; ++le) {
        MVertex* v1 = t->getVertex(le);
        if (v1->getNum() >= numToTris.size()) {
          numToTris.resize(v1->getNum()+1);
          vertices.resize(v1->getNum()+1,NULL);
        }
        numToTris[v1->getNum()].push_back(t);
        vertices[v1->getNum()] = v1;
      }
    }

    /* Remove vertices on curves */
    for (GEdge* ge: gf->edges()) {
      for (MLine* l: ge->lines) {
        for (size_t lv = 0; lv < 2; ++lv) {
          MVertex* v1 = l->getVertex(lv);
          if (v1->getNum() >= numToTris.size()) {
            vertices.resize(v1->getNum()+1,NULL);
          }
          vertices[v1->getNum()] = NULL;
        }
      }
    }

    /* Flag singular vertices */
    std::vector<double> vertexIndex(vertices.size(),0.);
    for (size_t i = 0; i < numToTris.size(); ++i) if (vertices[i] != NULL) {
      MVertex* v = vertices[i];
      std::vector<MTriangle*> tris = numToTris[i];
      if (tris.size() == 0) continue;

      SVector3 e_x, e_y, N;
      int st = local_frame(v, tris, e_x, e_y, N);
      if (st != 0) {
        Msg::Error("no local frame for v=%li", v->getNum());
        continue;
      }

      std::vector<std::array<MVertex*,2> > vPairs;
      int st2 = ordered_fan_around_vertex(v, tris, vPairs);
      if (st2 != 0) {
        Msg::Error("no ordered fan around v=%li", v->getNum());
        continue;
      }

      double angle_deficit = 2.*M_PI;
      std::vector<SVector3> rep_vectors(vPairs.size()); /* cross field representation vector in local frame (+e_x,+e_y) */
      for (size_t j = 0; j < vPairs.size(); ++j) {
        MVertex* v1 = vPairs[j][0];
        MVertex* v2 = vPairs[j][1];
        angle_deficit -= angle3Vertices(v1, v, v2); /* gaussian curvature */

        SVector3 tgt;
        if (v1->getNum() < v2->getNum()) {
          tgt = v2->point() - v1->point();
        } else {
          tgt = v1->point() - v2->point();
        }
        tgt.normalize();
        SVector3 tgt2 = crossprod(N,tgt);
        tgt2.normalize();
        std::array<size_t,2> vPair = {v1->getNum(),v2->getNum()};
        if (vPair[1] < vPair[0]) vPair = {v2->getNum(),v1->getNum()};
        MEdge query(v1,v2);
        auto it = edgeTheta.find(query);
        if (it == edgeTheta.end()) {
          Msg::Error("Edge (%li,%li) not found in edgeTheta",vPair[0],vPair[1]);
          return -1;
        }
        double A = it->second;

        SVector3 branchInQuadrant;
        bool found = false;
        double dpmax = -DBL_MAX; size_t kmax = (size_t) -1;
        for (size_t k = 0; k < 4; ++k) { /* Loop over branches */
          SVector3 branch = tgt * cos(A+k*M_PI/2) + tgt2 * sin(A+k*M_PI/2);
          if (dot(branch,e_x) >= 0. && dot(branch,e_y) >= 0.) {
            branchInQuadrant = branch;
            found = true;
            break;
          }
          double dp = dot(branch,SVector3(std::sqrt(2),std::sqrt(2),0.));
          if (dp > dpmax) {dpmax = dp; kmax = k;}
        }
        if (!found && kmax != (size_t)-1 ) { /* if numerical errors */
          branchInQuadrant = tgt * cos(A+kmax*M_PI/2) + tgt2 * sin(A+kmax*M_PI/2);
        }

        double theta = acos(dot(branchInQuadrant,e_x));
        rep_vectors[j] = {cos(4.*theta),sin(4.*theta),0.};
      }
      double sum_diff = 0.;
      for (size_t j = 0; j < vPairs.size(); ++j) {
        SVector3 vertical(0.,0.,1.);
        const SVector3 r1 = rep_vectors[j];
        const SVector3 r2 = rep_vectors[(j+1)%rep_vectors.size()];
        double diff_angle = angle2PI(r1,r2,vertical);
        if (diff_angle > M_PI) diff_angle -= 2.*M_PI;
        sum_diff += diff_angle;
      }
      if (std::abs(sum_diff-2*M_PI) < 0.05*M_PI) {
        vertexIndex[v->getNum()] = -1;
      } else if (std::abs(sum_diff+2*M_PI) < 0.05*M_PI) {
        vertexIndex[v->getNum()] = 1;
      } else if (std::abs(sum_diff) > 0.05 * M_PI) {
        Msg::Warning("singularity detection, v=%i, sum of diff is %.3f deg, ignored", v->getNum(), sum_diff);
      }
    }

    /* Three types of singularities with Crouzeix-Raviart interpolation:
     * - on vertex
     * - on edge
     * - on triangle */

    /* triangle singularities */
    for (MTriangle* t: gf->triangles) {
      MVertex* v1 = t->getVertex(0);
      MVertex* v2 = t->getVertex(1);
      MVertex* v3 = t->getVertex(2);
      for (double index: {-1.,1.}) {
        if (vertexIndex[v1->getNum()] == index 
            && vertexIndex[v2->getNum()] == index 
            && vertexIndex[v3->getNum()] == index) {
          SVector3 p = 1./3. * (v1->point()+v2->point()+v3->point());
          singularities.push_back({p.x(),p.y(),p.z(),index,double(gf->tag())});
          /* remove from list of available singularities */
          vertexIndex[v1->getNum()] = 0.; 
          vertexIndex[v2->getNum()] = 0.;
          vertexIndex[v3->getNum()] = 0.;
        }
      }
    }
    /* edge singularities */
    for (MTriangle* t: gf->triangles) {
      for (size_t le = 0; le < 3; ++le) {
        MVertex* v1 = t->getVertex(le);
        MVertex* v2 = t->getVertex((le+1)%3);
        for (double index: {-1.,1.}) {
          if (vertexIndex[v1->getNum()] == index 
              && vertexIndex[v2->getNum()] == index) {
            SVector3 p = 1./2. * (v1->point()+v2->point());
            singularities.push_back({p.x(),p.y(),p.z(),index,double(gf->tag())});
            /* remove from list of available singularities */
            vertexIndex[v1->getNum()] = 0.; 
            vertexIndex[v2->getNum()] = 0.;
          }
        }
      }
    }
    /* vertex singularities */
    for (size_t v = 0; v < vertexIndex.size(); ++v) if (std::abs(vertexIndex[v]) == 1.) {
      MVertex* vp = vertices[v];
      SVector3 p = vp->point();
      singularities.push_back({p.x(),p.y(),p.z(),vertexIndex[v],double(gf->tag())});
      vertexIndex[v] = 0.; 
    }
  }

  constexpr bool DBG_VIZU = true;
  if (DBG_VIZU) { /* Vizualisation of singularities */
    for (size_t i = 0; i < singularities.size(); ++i) {
      std::array<double,3> p = {
        singularities[i][0],
        singularities[i][1],
        singularities[i][2]};
      double index = singularities[i][3];
      GeoLog::add(p,index,"dbg_cf_singularities");
    }
    GeoLog::flush();
  }


  return 0;
}

int addSingularitiesAtAcuteCorners(
    const std::vector<GFace*>& faces,
    double thresholdInDeg,
    std::vector<std::array<double,5> >& singularities) {
  constexpr bool DBG_VIZU = true;
  for (GFace* gf: faces) {
    std::unordered_map<MVertex*, double> cornerAngle;
    std::unordered_map<MVertex*, std::vector<MTriangle*> > cornerToTris;
    for (MTriangle* t: gf->triangles) {
      for (size_t le = 0; le < 3; ++le) {
        MVertex* v2 = t->getVertex((le+1)%3);
        if (v2->onWhat()->cast2Vertex() != NULL) {
          MVertex* v1 = t->getVertex(le);
          MVertex* v3 = t->getVertex((le+2)%3);
          double agl = std::abs(angle3Vertices(v1,v2,v3));
          cornerAngle[v2] += agl;
          cornerToTris[v2].push_back(t);
        }
      }
    }
    for (const auto& kv: cornerAngle) {
      MVertex* v = kv.first;
      double agl = kv.second;
      if (agl * 180. / M_PI < thresholdInDeg)  {
        std::vector<MTriangle*>& tris = cornerToTris[v];
        if (tris.size() == 0) continue;
        SVector3 p(0.,0.,0.);
        for (MTriangle* t: tris) {
          p += t->barycenter();
        }
        p = 1./double(tris.size()) * p; 
        double indexVal3 = 1.;
        singularities.push_back({p.x(),p.y(),p.z(),indexVal3,double(gf->tag())});
        if (DBG_VIZU) GeoLog::add(p,indexVal3,"dbg_acute_singularities");
      }
    }
  }
  if (DBG_VIZU) GeoLog::flush();

  return 0;
}

int quantileFiltering(std::unordered_map<MVertex*,double>& scaling, double critera) {
  std::vector<double> values(scaling.size());
  size_t count = 0;
  for (auto& kv: scaling) {
    values[count] = kv.second;
    count += 1;
  }
  std::sort(values.begin(),values.end());
  size_t n = values.size();
  double clamp_min = values[size_t(critera*double(n))];
  double clamp_max = values[size_t((1.-critera)*double(n))];
  for (auto& kv: scaling) {
    if (kv.second < clamp_min) {
      kv.second = clamp_min;
    } else if (kv.second > clamp_max) {
      kv.second = clamp_max;
    }
  }

  // Msg::Error("min:  %f", values.front());
  // Msg::Error("0.01:  %f", values[size_t(0.01*double(n))]);
  // Msg::Error("0.1:  %f", values[size_t(0.1*double(n))]);
  // Msg::Error("0.25: %f", values[size_t(0.25*double(n))]);
  // Msg::Error("0.5:  %f", values[size_t(0.5*double(n))]);
  // Msg::Error("0.75: %f", values[size_t(0.75*double(n))]);
  // Msg::Error("0.9:  %f", values[size_t(0.9*double(n))]);
  // Msg::Error("0.99:  %f", values[size_t(0.99*double(n))]);
  // Msg::Error("max:  %f", values.back());

  return 0;
}

int computeScaledCrossFieldView(GModel* gm,
    int& dataListViewTag, 
    std::size_t targetNumberOfQuads,
    int nbDiffusionLevels, 
    double thresholdNormConvergence, 
    int nbBoundaryExtensionLayer,
    const std::string& viewName,
    int verbosity,
    std::vector<std::array<double,5> >* singularities,
    bool disableConformalScaling,
    double conformalScalingQuantileFiltering
    ) {
  Msg::Debug("compute scaled cross field ...");
#ifdef HAVE_QUADMESHINGTOOLS
  std::vector<GFace*> faces;
  for(GModel::fiter it = gm->firstFace(); it != gm->lastFace(); ++it) {
    faces.push_back(*it);
  }

  std::unordered_map<MEdge,double,MEdgeHash,MEdgeEqual> edgeTheta;

  /* Compute the cross field on each face
   * Not in parallel because the solver in computeCrossFieldWithHeatEquation may already be parallel (e.g. MUMPS) */
  for (GFace* gf: faces) {
    Msg::Info("- Face %i: compute cross field (%li triangles) ...",gf->tag(), gf->triangles.size());
    int status = computeCrossFieldWithHeatEquation({gf}, edgeTheta, nbDiffusionLevels, thresholdNormConvergence,
        nbBoundaryExtensionLayer, verbosity);
    if (status != 0) {
      Msg::Error("failed to compute cross field on face %i",gf->tag());
    }
  }

  { /* For discrete meshes directly imported */
    bool updateTopo = false;
    for (GFace* gf: faces) {
      discreteFace* df = dynamic_cast<discreteFace*>(gf);
      if (df) { updateTopo = true; break; }
    }
    if (updateTopo) {
      gm->createTopologyFromMesh();
    }
  }

  std::unordered_map<MVertex*,double> scaling;
  std::vector<MTriangle*> triangles;
  getFacesTriangles(faces, triangles);
  if (!disableConformalScaling) {
    Msg::Info("Compute cross field conformal scaling (global) ...");
    int status = computeCrossFieldConformalScaling(triangles, edgeTheta, scaling);
    if (status != 0) {
      Msg::Error("failed to compute cross field scaling");
      return -1;
    }
    if (conformalScalingQuantileFiltering != 0) {
      Msg::Warning("Apply quantile filtering on conformal scaling (clamp the %f-quantile extremities)",conformalScalingQuantileFiltering);
      quantileFiltering(scaling, conformalScalingQuantileFiltering);
    }
  } else {
    Msg::Info("Cross field conformal scaling disabled");
    for (const auto& kv: edgeTheta) {
      scaling[kv.first.getMinVertex()] = 1.;
      scaling[kv.first.getMaxVertex()] = 1.;
    }
  }

  bool SHOW_H = true; // Debugging view to check H
  if (SHOW_H && !disableConformalScaling) {
    Msg::Warning("generating H view (saved at /tmp/H.pos), only for debugging/prototyping");
    std::string name = "dbg_H";
    PViewDataGModel *d = new PViewDataGModel;
    d->setName(name);
    d->setFileName(name + ".msh");
    std::map<int, std::vector<double> > dataH;
    for (const auto& kv: scaling) {
      std::vector<double> jj;
      jj.push_back(kv.second);
      dataH[kv.first->getNum()] = jj;
    }
    d->addData(gm, dataH, 0, 0.0, 1, 1);
    d->finalize();
    std::string posout = "/tmp/H.pos";
    d->writePOS(posout, false, true, false);
    GmshMergeFile(posout);
  }

  Msg::Info("Compute quad mesh size map from conformal factor and %li target quads ...", 
      targetNumberOfQuads);
  std::vector<std::vector<GFace*> > components;
  getConnectedSurfaceComponents(faces, components);
  std::vector<size_t> componentNumberOfQuads;
  distributeQuadsBasedOnArea(targetNumberOfQuads, components, componentNumberOfQuads);
  for (size_t i = 0; i < components.size(); ++i) {
    std::vector<MTriangle*> componentTriangles;
    getFacesTriangles(components[i],componentTriangles);
    int status2 = computeQuadSizeMapFromCrossFieldConformalFactor(componentTriangles, componentNumberOfQuads[i], scaling);
    if (status2 != 0) {
      Msg::Error("Failed to compute quad mesh size map");
      return -1;
    }
  }

  if (singularities != NULL) {
    int sts = detectCrossFieldSingularities(faces, edgeTheta, *singularities);
    if (sts != 0) {
      Msg::Error("failed to detect cross field singularities");
    }
    Msg::Info("Detected %li cross field singularities", singularities->size());
  }

  /* Create data list view */
  int statusv = createScaledCrossFieldView(triangles, edgeTheta, scaling, viewName, dataListViewTag);
  if (statusv != 0) {
    Msg::Error("failed to create view");
    return -1;
  }


#else
  Msg::Error("Computation of scaled cross field requires the QuadMeshingTools module");
  return -1;
#endif

  return 0;
}
