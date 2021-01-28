// Gmsh - Copyright (C) 1997-2014 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// issues on https://gitlab.onelab.info/gmsh/gmsh/issues.
//
// Contributor(s):
//   Tristan Carrier Baudoin

#include <queue>
#include <stack>
#include "GmshConfig.h"
#include "surfaceFiller.h"
#include "Field.h"
#include "GModel.h"
#include "OS.h"
#include "rtree.h"
#include "MVertex.h"
#include "MElement.h"
#include "MLine.h"
#include "BackgroundMesh.h"
#include "intersectCurveSurface.h"
#include "pointInsertionRTreeTools.h"

// Here, we aim at producing a set of points that enables to generate a nice
// quad mesh

// assume a point on the surface, compute the 4 possible neighbors.
//
//              ^ t2
//              |
//              |
//             v2
//              |
//              |
//       v1-----+------v3 -------> t1
//              |
//              |
//             v4
//
// we aim at generating a rectangle with sizes size_1 and size_2 along t1 and t2

bool compute4neighbors(
    GFace *gf, // the surface
    MVertex *v_center, // the wertex for which we wnt to generate 4 neighbors
    SPoint2 &midpoint,
    SPoint2 newP[8], // look into other directions
    SMetric3 &metricField,// the mesh metric
    Field *f,
    double du,
    double dv,
    double mult) 
{
  // we assume that v is on surface gf

  // get the parameter of the point on the surface
  reparamMeshVertexOnFace(v_center, gf, midpoint);

  midpoint = SPoint2(midpoint.x() + du,midpoint.y() + dv);

  SVector3 t1;
  (*f)(v_center->x(), v_center->y(), v_center->z(), t1, gf);
  double L = t1.norm()*mult;
  //  printf("L = %12.5E\n",L);
  metricField = SMetric3(1. / (L * L));

  // get the unit normal at that point
  Pair<SVector3, SVector3> der =
    gf->firstDer(SPoint2(midpoint[0], midpoint[1]));
  SVector3 s1 = der.first();
  SVector3 s2 = der.second();
  SVector3 n = crossprod(s1, s2);
  n.normalize();
  t1 -= n*dot(t1,n);
  t1.normalize();

  double M = dot(s1, s1);
  double N = dot(s2, s2);
  double E = dot(s1, s2);

  // compute the first fundamental form i.e. the metric tensor at the point
  // M_{ij} = s_i \cdot s_j
  double metric[2][2] = {{M, E}, {E, N}};

  // compute the second direction t2 and normalize (t1,t2,n) is the tangent
  // frame
  SVector3 t2 = crossprod(n, t1);
  t2.normalize();

  // compute covariant coordinates of t1 and t2
  // t1 = a s1 + b s2 -->
  // t1 . s1 = a M + b E
  // t1 . s2 = a E + b N --> solve the 2 x 2 system
  // and get covariant coordinates a and b
  double rhs1[2] = {dot(t1, s1)*L, dot(t1, s2)*L}, covar1[2];
  bool singular = false;
  if(!sys2x2(metric, rhs1, covar1)) {
    Msg::Error("SINGULAR AT %g %g",midpoint.x(),midpoint.y());
    return false;
    covar1[1] = 1.0;
    covar1[0] = 0.0;
    singular = true;
  }
  double rhs2[2] = {dot(t2, s1)*L, dot(t2, s2)*L}, covar2[2];
  if(!sys2x2(metric, rhs2, covar2)) {
    Msg::Error("SINGULAR AT %g %g",midpoint.x(),midpoint.y());
    return false;
    covar2[0] = 1.0;
    covar2[1] = 0.0;
    singular = true;
  }

  // compute the corners of the box as well
  double LSQR = L ;
  SVector3 b1 = t1+t2;
  b1.normalize();
  SVector3 b2 = t1-t2;
  b2.normalize();

  double rhs3[2] = {dot(b1, s1)*LSQR, dot(b1, s2)*LSQR}, covar3[2];
  if(!sys2x2(metric, rhs3, covar3)) {
    covar3[1] = 1.e22;
    covar3[0] = 0.0;
    singular = true;
  }
  double rhs4[2] = {dot(b2, s1)*LSQR, dot(b2, s2)*LSQR}, covar4[2];
  if(!sys2x2(metric, rhs4, covar4)) {
    covar4[0] = 1.e22;
    covar4[1] = 0.0;
    singular = true;
  }


  double size_1 = sqrt (covar1[0]*covar1[0]+covar1[1]*covar1[1]);
  double size_2 = sqrt (covar2[0]*covar2[0]+covar2[1]*covar2[1]);

  //  if (singular){
  //  }


  double newPoint[8][2] = {{midpoint[0] - covar1[0],
    midpoint[1] - covar1[1]},
  {midpoint[0] - covar2[0],
    midpoint[1] - covar2[1]},
  {midpoint[0] + covar1[0],
    midpoint[1] + covar1[1]},
  {midpoint[0] + covar2[0],
    midpoint[1] + covar2[1]},
  {midpoint[0] - covar3[0],
    midpoint[1] - covar3[1]},
  {midpoint[0] - covar4[0],
    midpoint[1] - covar4[1]},
  {midpoint[0] + covar3[0],
    midpoint[1] + covar3[1]},
  {midpoint[0] + covar4[0],
    midpoint[1] + covar4[1]}};

  SVector3 dirs[8]      = {t1 * (-1.0), t2 * (-1.0), t1 * (1.0), t2 * (1.0),
    b1 * (-1.0), b2 * (-1.0), b1 * (1.0), b2 * (1.0) };
  SVector3 orthodirs[8] = {t2 * (-1.0), t1 * (-1.0), t2 * (1.0), t1 * (1.0),
    b2 * (-1.0), b1 * (-1.0), b2 * (1.0), b1 * (1.0) };
  double   LS[8]   = {L,L,L,L,LSQR,LSQR,LSQR,LSQR};

  SPoint3 ppx (v_center->x(),v_center->y(),v_center->z());
  surfaceFunctorGFace ss(gf);
  for(int i = 0; i < 8; i++) {
    newP[i] = SPoint2(newPoint[i][0], newPoint[i][1]);    
    GPoint pp = gf->point(newP[i]);
    SPoint3 px (pp.x(),pp.y(),pp.z());
    SVector3 test = px - ppx;
    double L2 = test.norm();
    double DIFF_ANG = fabs(dot(orthodirs[i],test)) / L2;
    double DIFF_L   = fabs(L2-LS[i]);
    if (singular || DIFF_L > .1*LS[i] || DIFF_ANG > .1){
      curveFunctorCircle cf(dirs[i], n, SVector3(v_center->x(), v_center->y(), v_center->z()), LS[i]);
      double uvt[3] = {newPoint[i][0], newPoint[i][1], 0.0}; //
      if(intersectCurveSurface(cf, ss, uvt, size_1 * 1.e-6)) { 
        pp = gf->point(SPoint2(uvt[0], uvt[1]));      
        px = SPoint3 (pp.x(),pp.y(),pp.z());
        test = px - ppx;
        L2 = test.norm();
        double DIFF_ANG2 = fabs(dot(orthodirs[i],test)) / L2;
        double DIFF_L2   = fabs(L2-LS[i]);
        newPoint[i][0]=uvt[0];
        newPoint[i][1]=uvt[1];
        if (DIFF_L2 <= DIFF_L && DIFF_ANG2 <= DIFF_ANG){
        }
        else{
          Msg::Warning("Difficult to find a point %lu L %g vs %g (ps %12.5E) ",i,L,L2,DIFF_ANG2);
        }
      }
      else{
        SPoint3 p_test (v_center->x() + dirs[i].x() * LS[i],
            v_center->y() + dirs[i].y() * LS[i],
            v_center->z() + dirs[i].z() * LS[i]);      		
        pp = gf->closestPoint(p_test ,uvt);
        if (pp.succeeded()){
          newPoint[i][0] = pp.u();
          newPoint[i][1] = pp.v();
        }
        else 
          Msg::Debug("Impossible to intersect with a circle of radius %g",L);
      }
    }    
  }

  return true;
}


static bool outBounds(SPoint2 p, double minu, double maxu, double minv, double maxv){
  if (p.x() > maxu || p.x() <  minu || p.y() > maxv || p.y() <  minv){
    //    printf("OUT BOUND %g %g\n",p.x(),p.y());
    return true;

  }
  return false;
}

static bool close2sing(std::vector<MVertex*> &s, GFace *gf, SPoint2 p, Field *f){

  if (s.empty())return false;
  GPoint gp = gf->point(p);
  SVector3 t1;
  (*f)(gp.x(), gp.y(), gp.z(), t1, gf);
  double L = t1.norm();

  for (size_t i=0;i<s.size();i++){
    MVertex *v = s[i];
    double d = sqrt ((v->x()-gp.x())*(v->x()-gp.x())+
        (v->y()-gp.y())*(v->y()-gp.y())+
        (v->z()-gp.z())*(v->z()-gp.z()));
    if (d < FACTOR*L)return true;
  }
  return false;
}


static void findPhysicalGroupsForSingularities(GFace *gf,
    std::map<MVertex *, int> &temp)
{

  std::set<GVertex *, GEntityPtrLessThan> emb = gf->embeddedVertices();
  if (emb.empty())return;

  std::map<int, std::vector<GEntity *> > groups[4];
  gf->model()->getPhysicalGroups(groups);
  for(std::map<int, std::vector<GEntity *> >::iterator it = groups[0].begin();
      it != groups[0].end(); ++it) {
    std::string name = gf->model()->getPhysicalName(0, it->first);
    if(name == "SINGULARITY_OF_INDEX_THREE") {
      for(size_t j = 0; j < it->second.size(); j++) {
        if (emb.find((GVertex*)it->second[j]) != emb.end()){
          if(!it->second[j]->mesh_vertices.empty())
            temp[it->second[j]->mesh_vertices[0]] = 3;
        }
      }
    }
    else if(name == "SINGULARITY_OF_INDEX_FIVE") {
      for(size_t j = 0; j < it->second.size(); j++) {
        if (emb.find((GVertex*)it->second[j]) != emb.end()){
          if(!it->second[j]->mesh_vertices.empty())
            temp[it->second[j]->mesh_vertices[0]] = 5;
        }
      }
    }
    else if(name == "SINGULARITY_OF_INDEX_SIX") {
      for(size_t j = 0; j < it->second.size(); j++) {
        if (emb.find((GVertex*)it->second[j]) != emb.end()){
          if(!it->second[j]->mesh_vertices.empty())
            temp[it->second[j]->mesh_vertices[0]] = 6;
        }
      }
    }
  }
}


void packingOfParallelograms(GFace *gf, std::vector<MVertex *> &packed,
    std::vector<SMetric3> &metrics)
{

  FILE *f = NULL;
  FILE *f2 = NULL;
  // if(Msg::GetVerbosity() == 99) {
  //   char ccc[256];
  //   sprintf(ccc, "points%d.pos", gf->tag());
  //   f = Fopen(ccc, "w");
  //   sprintf(ccc, "e_points%d.pos", gf->tag());
  //   f2 = Fopen(ccc, "w");
  //   if(f) fprintf(f, "View \"\"{\n");
  //   if(f2) fprintf(f2, "View \"\"{\n");
  // }

  FieldManager *fields = gf->model()->getFields();
  Field *guiding_field = NULL;
  if(fields->getBackgroundField() > 0) {        
    guiding_field = fields->get(fields->getBackgroundField());
    if(guiding_field && guiding_field->numComponents() != 3) {// we hae a true scaled cross fields !!
      Msg::Error ("Packing of Parallelograms require a guiding field (cross field + size map)");
      return;
    }
  } else {
    Msg::Error ("Packing of Parallelograms require a guiding field (cross field + size map)");
    return;
  }


  const bool goNonLinear = true;

  // get all the boundary vertices
  std::set<MVertex *, MVertexPtrLessThan> bnd_vertices;
  for(unsigned int i = 0; i < gf->getNumMeshElements(); i++) {
    MElement *element = gf->getMeshElement(i);
    for(std::size_t j = 0; j < element->getNumVertices(); j++) {
      MVertex *vertex = element->getVertex(j);
      if(vertex->onWhat()->dim() < 2) bnd_vertices.insert(vertex);
    }
  }

  // Renormalize size map taking into account quantization...
  double globalMult = 1.0;

  // put boundary vertices in a fifo queue
  std::queue<surfacePointWithExclusionRegion *> fifo;
  std::vector<surfacePointWithExclusionRegion *> vertices;
  // put the RTREE
  RTree<surfacePointWithExclusionRegion *, double, 2, double> rtree;
  SMetric3 metricField(1.0);
  SPoint2 newp[8];
  std::set<MVertex *, MVertexPtrLessThan>::iterator it = bnd_vertices.begin();

  double maxu = -1.e22,minu = 1.e22;
  double maxv = -1.e22,minv = 1.e22;

  std::vector<MVertex*> singularities;  
  for(; it != bnd_vertices.end(); ++it) {

    int NP = 1;
    SPoint2 midpoint;
    double du[4] = {0,0,0,0}, dv[4]= {0,0,0,0};

    for (int i=0;i<2;i++){
      if (gf->periodic(i)){
        reparamMeshVertexOnFace(*it, gf, midpoint);
        Range<double> bnds = gf->parBounds(i);      
        //	if (1 || midpoint[i] == bnds.low()){
        if (i == 0)
          du[NP] =  bnds.high() -  bnds.low();
        else
          dv[NP] =  bnds.high() -  bnds.low();
        NP++;
        //	}
        //	else if (midpoint[i] == bnds.high()){
        if (i == 0)
          du[NP] =  -(bnds.high() -  bnds.low());
        else
          dv[NP] =  -(bnds.high() -  bnds.low());
        NP++;
        //	}
      }
    }
    for (int i=0;i<NP;i++){
      bool singular = !compute4neighbors(gf, *it, midpoint, newp, metricField, guiding_field, du[i],dv[i],globalMult );
      if (!singular){
        surfacePointWithExclusionRegion *sp =
          new surfacePointWithExclusionRegion(*it, newp, midpoint, metricField);
        minu = std::min(midpoint.x(),minu);
        maxu = std::max(midpoint.x(),maxu);
        minv = std::min(midpoint.y(),minv);
        maxv = std::max(midpoint.y(),maxv);
        vertices.push_back(sp);
        fifo.push(sp);
        double _min[2], _max[2];
        sp->minmax(_min, _max);
        rtree.Insert(_min, _max, sp);
      }
      else{
        singularities.push_back(*it);
        break;
      }
    }
  }

  //  printf("bounds = %g %g %g %g \n",minu,maxu,minv,maxv);

  while(!fifo.empty()) {
    //    printf("%d vertices in the domain\n",vertices.size());
    //    if (vertices.size() > 5000)break;
    surfacePointWithExclusionRegion *parent = fifo.front();
    fifo.pop();
    for(int i = 0; i < 4; i++) {
      if(!close2sing (singularities,gf,parent->_p[i],guiding_field)
          && !inExclusionZone(parent->_v, parent->_p[i], rtree) &&
          !outBounds(parent->_p[i],minu,maxu,minv,maxv)&&
          gf->containsParam(parent->_p[i]))
      {
        GPoint gp = gf->point(parent->_p[i]);
        MFaceVertex *v =
          new MFaceVertex(gp.x(), gp.y(), gp.z(), gf, gp.u(), gp.v());
        SPoint2 midpoint;
        compute4neighbors(gf, v, midpoint, newp, metricField, guiding_field,0, 0, globalMult);
        surfacePointWithExclusionRegion *sp =
          new surfacePointWithExclusionRegion(v, newp, midpoint, metricField, parent);
        fifo.push(sp);
        vertices.push_back(sp);
        double _min[2], _max[2];
        sp->minmax(_min, _max);
        rtree.Insert(_min, _max, sp);
      }
      else{
        if(Msg::GetVerbosity() == 99) {
          GPoint gp = gf->point(parent->_p[i]);
          MFaceVertex *v =
            new MFaceVertex(gp.x(), gp.y(), gp.z(), gf, gp.u(), gp.v());
          SPoint2 midpoint;
          compute4neighbors(gf, v, midpoint, newp, metricField, guiding_field, 0, 0 , globalMult);
          surfacePointWithExclusionRegion *sp =
            new surfacePointWithExclusionRegion(v, newp, midpoint, metricField,parent);
          if (f2 && !gf->containsParam(parent->_p[i])) sp->print(f2, i);	  
          //	 printf("AI\n");
        }
      }
    }
  }
  // add the vertices as additional vertices in the surface mesh
  for(unsigned int i = 0; i < vertices.size(); i++) {
    if (f)vertices[i]->print(f, i);
    if(vertices[i]->_v->onWhat() == gf) {
      packed.push_back(vertices[i]->_v);
      metrics.push_back(vertices[i]->_meshMetric);
      SPoint2 midpoint;
      reparamMeshVertexOnFace(vertices[i]->_v, gf, midpoint);
    }
    delete vertices[i];
  }
  if (f){
    fprintf(f2, "};");
    fclose(f2);
    fprintf(f, "};");
    fclose(f);
  }
}
