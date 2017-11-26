// Gmsh - Copyright (C) 1997-2017 C. Geuzaine,J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// bugs and problems to the public mailing list <gmsh@onelab.info>.

extern "C" {
    #include "gmshc.h"
}
#include "gmsh.h"
#include <cstring>

char * _strdup(const char *i) {
  size_t len = strlen(i);
  char *o = (char*)malloc(sizeof(char)*(len+1));
  memcpy(o,i,len+1);
  return o;
}

template<typename t>
std::vector<t> ptr2vector(const t *p,size_t size) {
  return std::vector<t>(p,p+size);
}

template<typename t>
std::vector<std::vector<t> > ptrptr2vectorvector(const t **p, const size_t *n, size_t size) {
  std::vector<std::vector<t> > v(size);
  for (size_t i=0; i<size; ++i)
    v[i] = std::vector<t>(p[i],p[i]+n[i]);
  return v;
}

template<typename t>
void vector2ptr(const std::vector<t>&v,t **p,size_t *size) {
  *p = (t*)malloc(sizeof(t)*(v.size()));
  for (size_t i = 0; i < v.size(); ++i){
    (*p)[i] = v[i];
  }
  *size = v.size();
}

void pairvector2intptr(const gmsh::vector_pair &v,int **p,size_t *size) {
  *p = (int*)malloc(sizeof(int)*(v.size()*2));
  for (size_t i = 0; i < v.size(); ++i){
    (*p)[i*2+0] = v[i].first;
    (*p)[i*2+1] = v[i].second;
  }
  *size = v.size()*2;
}

void stringvector2charpp(const std::vector<std::string> &v,char ***p,size_t *size) {
  *p = (char**)malloc(sizeof(char*)*(v.size()*2));
  for (size_t i = 0; i < v.size(); ++i){
    (*p)[i] = _strdup(v[i].c_str());
  }
  *size = v.size();
}

gmsh::vector_pair intptr2pairvector(const int *p,size_t size){
  gmsh::vector_pair v(size/2);
  for (size_t i = 0; i < size/2; ++i) {
    v[i].first = p[i*2+0];
    v[i].second = p[i*2+1];
  }
  return v;
}

template<typename t>
void vectorvector2ptrptr(const std::vector<std::vector<t> > &v,t ***p,size_t **size,size_t *sizeSize) {
  *p = (t**)malloc(sizeof(t*)*v.size());
  *size = (size_t*)malloc(sizeof(size_t)*v.size()); 
  for (size_t i = 0; i < v.size(); ++i)
    vector2ptr(v[i],&((*p)[i]),&((*size)[i]));
  *sizeSize = v.size();
}

void pairvectorvector2intptrptr(const std::vector<gmsh::vector_pair > &v,int ***p,size_t **size,size_t *sizeSize) {
  *p = (int**)malloc(sizeof(int*)*v.size());
  *size = (size_t*)malloc(sizeof(size_t)*v.size());
  for (size_t i = 0; i < v.size(); ++i)
    pairvector2intptr(v[i],&(*p)[i],&((*size)[i]));
  *sizeSize = v.size();
}

void gmshFree(void *p) {
  if(p) free(p);
}
void gmshInitialize(int argc, char **argv,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::initialize(argc, argv);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshFinalize(int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::finalize();
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshOpen(const char * fileName,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::open(fileName);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshMerge(const char * fileName,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::merge(fileName);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshWrite(const char * fileName,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::write(fileName);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshClear(int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::clear();
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshOptionSetNumber(const char * name,const double value,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::option::setNumber(name,value);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshOptionGetNumber(const char * name,double * value,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::option::getNumber(name,*value);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshOptionSetString(const char * name,const char * value,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::option::setString(name,value);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshOptionGetString(const char * name,char ** value,int * ierr){
  if(ierr) *ierr = 0;
  try {
  std::string api_value_;
  gmsh::option::getString(name,api_value_);
  *value = _strdup(api_value_.c_str());
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelAdd(const char * name,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::add(name);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelRemove(int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::remove();
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelList(char*** names,size_t* names_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  std::vector<std::string> api_names_;
  gmsh::model::list(api_names_);
  stringvector2charpp(api_names_,names,names_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelSetCurrent(const char * name,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::setCurrent(name);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGetEntities(int** dimTags, size_t* dimTags_n,const int dim,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_dimTags_;
  gmsh::model::getEntities(api_dimTags_,dim);
  pairvector2intptr(api_dimTags_,dimTags,dimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGetPhysicalGroups(int** dimTags, size_t* dimTags_n,const int dim,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_dimTags_;
  gmsh::model::getPhysicalGroups(api_dimTags_,dim);
  pairvector2intptr(api_dimTags_,dimTags,dimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGetEntitiesForPhysicalGroup(const int dim,const int tag,int** tags, size_t* tags_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  std::vector<int> api_tags_;
  gmsh::model::getEntitiesForPhysicalGroup(dim,tag,api_tags_);
  vector2ptr(api_tags_,tags,tags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

int gmshModelAddPhysicalGroup(const int dim,int* tags, size_t tags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::addPhysicalGroup(dim,ptr2vector(tags,tags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

void gmshModelSetPhysicalName(const int dim,const int tag,const char * name,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::setPhysicalName(dim,tag,name);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGetPhysicalName(const int dim,const int tag,char ** name,int * ierr){
  if(ierr) *ierr = 0;
  try {
  std::string api_name_;
  gmsh::model::getPhysicalName(dim,tag,api_name_);
  *name = _strdup(api_name_.c_str());
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGetBoundary(int* dimTags, size_t dimTags_n,int** outDimTags, size_t* outDimTags_n,const int combined,const int oriented,const int recursive,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  gmsh::model::getBoundary(intptr2pairvector(dimTags,dimTags_n),api_outDimTags_,combined,oriented,recursive);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGetEntitiesInBoundingBox(const double xmin,const double ymin,const double zmin,const double xmax,const double ymax,const double zmax,int** tags, size_t* tags_n,const int dim,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_tags_;
  gmsh::model::getEntitiesInBoundingBox(xmin,ymin,zmin,xmax,ymax,zmax,api_tags_,dim);
  pairvector2intptr(api_tags_,tags,tags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGetBoundingBox(const int dim,const int tag,double * xmin,double * ymin,double * zmin,double * xmax,double * ymax,double * zmax,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::getBoundingBox(dim,tag,*xmin,*ymin,*zmin,*xmax,*ymax,*zmax);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

int gmshModelAddDiscreteEntity(const int dim,const int tag,int* boundary, size_t boundary_n,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::addDiscreteEntity(dim,tag,ptr2vector(boundary,boundary_n));
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

void gmshModelRemoveEntities(int* dimTags, size_t dimTags_n,const int recursive,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::removeEntities(intptr2pairvector(dimTags,dimTags_n),recursive);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshGenerate(const int dim,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::generate(dim);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshGetLastEntityError(int** dimTags, size_t* dimTags_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_dimTags_;
  gmsh::model::mesh::getLastEntityError(api_dimTags_);
  pairvector2intptr(api_dimTags_,dimTags,dimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshGetLastVertexError(int** vertexTags, size_t* vertexTags_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  std::vector<int> api_vertexTags_;
  gmsh::model::mesh::getLastVertexError(api_vertexTags_);
  vector2ptr(api_vertexTags_,vertexTags,vertexTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshGetVertices(const int dim,const int tag,int** vertexTags, size_t* vertexTags_n,double** coord, size_t* coord_n,double** parametricCoord, size_t* parametricCoord_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  std::vector<int> api_vertexTags_;
  std::vector<double> api_coord_;
  std::vector<double> api_parametricCoord_;
  gmsh::model::mesh::getVertices(dim,tag,api_vertexTags_,api_coord_,api_parametricCoord_);
  vector2ptr(api_vertexTags_,vertexTags,vertexTags_n);
  vector2ptr(api_coord_,coord,coord_n);
  vector2ptr(api_parametricCoord_,parametricCoord,parametricCoord_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshGetElements(const int dim,const int tag,int** types, size_t* types_n,int*** elementTags, size_t** elementTags_n, size_t *elementTags_nn,int*** vertexTags, size_t** vertexTags_n, size_t *vertexTags_nn,int * ierr){
  if(ierr) *ierr = 0;
  try {
  std::vector<int> api_types_;
  std::vector<std::vector<int> > api_elementTags_;
  std::vector<std::vector<int> > api_vertexTags_;
  gmsh::model::mesh::getElements(dim,tag,api_types_,api_elementTags_,api_vertexTags_);
  vector2ptr(api_types_,types,types_n);
  vectorvector2ptrptr(api_elementTags_,elementTags,elementTags_n,elementTags_nn);
  vectorvector2ptrptr(api_vertexTags_,vertexTags,vertexTags_n,vertexTags_nn);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshSetVertices(const int dim,const int tag,int* vertexTags, size_t vertexTags_n,double* coord, size_t coord_n,double* parametricCoord, size_t parametricCoord_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::setVertices(dim,tag,ptr2vector(vertexTags,vertexTags_n),ptr2vector(coord,coord_n),ptr2vector(parametricCoord,parametricCoord_n));
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshSetElements(const int dim,const int tag,int* types, size_t types_n,const int** elementTags, const size_t* elementTags_n, size_t elementTags_nn,const int** vertexTags, const size_t* vertexTags_n, size_t vertexTags_nn,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::setElements(dim,tag,ptr2vector(types,types_n),ptrptr2vectorvector(elementTags,elementTags_n,elementTags_nn),ptrptr2vectorvector(vertexTags,vertexTags_n,vertexTags_nn));
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshGetVertex(const int vertexTag,double** coord, size_t* coord_n,double** parametricCoord, size_t* parametricCoord_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  std::vector<double> api_coord_;
  std::vector<double> api_parametricCoord_;
  gmsh::model::mesh::getVertex(vertexTag,api_coord_,api_parametricCoord_);
  vector2ptr(api_coord_,coord,coord_n);
  vector2ptr(api_parametricCoord_,parametricCoord,parametricCoord_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshGetElement(const int elementTag,int * type,int** vertexTags, size_t* vertexTags_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  std::vector<int> api_vertexTags_;
  gmsh::model::mesh::getElement(elementTag,*type,api_vertexTags_);
  vector2ptr(api_vertexTags_,vertexTags,vertexTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshSetSize(int* dimTags, size_t dimTags_n,const double size,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::setSize(intptr2pairvector(dimTags,dimTags_n),size);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshSetTransfiniteLine(const int tag,const int numVertices,const char * type,const double coef,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::setTransfiniteLine(tag,numVertices,type,coef);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshSetTransfiniteSurface(const int tag,const char * arrangement,int* cornerTags, size_t cornerTags_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::setTransfiniteSurface(tag,arrangement,ptr2vector(cornerTags,cornerTags_n));
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshSetTransfiniteVolume(const int tag,int* cornerTags, size_t cornerTags_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::setTransfiniteVolume(tag,ptr2vector(cornerTags,cornerTags_n));
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshSetRecombine(const int dim,const int tag,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::setRecombine(dim,tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshSetSmoothing(const int dim,const int tag,const int val,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::setSmoothing(dim,tag,val);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshSetReverse(const int dim,const int tag,const int val,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::setReverse(dim,tag,val);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshEmbed(const int dim,int* tags, size_t tags_n,const int inDim,const int inTag,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::embed(dim,ptr2vector(tags,tags_n),inDim,inTag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

int gmshModelMeshFieldAdd(const char * type,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::mesh::field::add(type,tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

void gmshModelMeshFieldRemove(const int tag,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::field::remove(tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshFieldSetNumber(const int tag,const char * option,const double value,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::field::setNumber(tag,option,value);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshFieldSetString(const int tag,const char * option,const char * value,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::field::setString(tag,option,value);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshFieldSetNumbers(const int tag,const char * option,double* value, size_t value_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::field::setNumbers(tag,option,ptr2vector(value,value_n));
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelMeshFieldSetAsBackground(const int tag,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::mesh::field::setAsBackground(tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

int gmshModelGeoAddPoint(const double x,const double y,const double z,const double meshSize,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::geo::addPoint(x,y,z,meshSize,tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelGeoAddLine(const int startTag,const int endTag,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::geo::addLine(startTag,endTag,tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelGeoAddCircleArc(const int startTag,const int centerTag,const int endTag,const int tag,const double nx,const double ny,const double nz,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::geo::addCircleArc(startTag,centerTag,endTag,tag,nx,ny,nz);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelGeoAddEllipseArc(const int startTag,const int centerTag,const int majorTag,const int endTag,const int tag,const double nx,const double ny,const double nz,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::geo::addEllipseArc(startTag,centerTag,majorTag,endTag,tag,nx,ny,nz);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelGeoAddSpline(int* vertexTags, size_t vertexTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::geo::addSpline(ptr2vector(vertexTags,vertexTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelGeoAddBSpline(int* vertexTags, size_t vertexTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::geo::addBSpline(ptr2vector(vertexTags,vertexTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelGeoAddBezier(int* vertexTags, size_t vertexTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::geo::addBezier(ptr2vector(vertexTags,vertexTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelGeoAddLineLoop(int* edgeTags, size_t edgeTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::geo::addLineLoop(ptr2vector(edgeTags,edgeTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelGeoAddPlaneSurface(int* wireTags, size_t wireTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::geo::addPlaneSurface(ptr2vector(wireTags,wireTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelGeoAddSurfaceFilling(int* wireTags, size_t wireTags_n,const int tag,const int sphereCenterTag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::geo::addSurfaceFilling(ptr2vector(wireTags,wireTags_n),tag,sphereCenterTag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelGeoAddSurfaceLoop(int* faceTags, size_t faceTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::geo::addSurfaceLoop(ptr2vector(faceTags,faceTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelGeoAddVolume(int* shellTags, size_t shellTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::geo::addVolume(ptr2vector(shellTags,shellTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

void gmshModelGeoExtrude(int* dimTags, size_t dimTags_n,const double dx,const double dy,const double dz,int** outDimTags, size_t* outDimTags_n,int* numElements, size_t numElements_n,double* heights, size_t heights_n,const int recombine,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  gmsh::model::geo::extrude(intptr2pairvector(dimTags,dimTags_n),dx,dy,dz,api_outDimTags_,ptr2vector(numElements,numElements_n),ptr2vector(heights,heights_n),recombine);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoRevolve(int* dimTags, size_t dimTags_n,const double x,const double y,const double z,const double ax,const double ay,const double az,const double angle,int** outDimTags, size_t* outDimTags_n,int* numElements, size_t numElements_n,double* heights, size_t heights_n,const int recombine,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  gmsh::model::geo::revolve(intptr2pairvector(dimTags,dimTags_n),x,y,z,ax,ay,az,angle,api_outDimTags_,ptr2vector(numElements,numElements_n),ptr2vector(heights,heights_n),recombine);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoTwist(int* dimTags, size_t dimTags_n,const double x,const double y,const double z,const double dx,const double dy,const double dz,const double ax,const double ay,const double az,const double angle,int** outDimTags, size_t* outDimTags_n,int* numElements, size_t numElements_n,double* heights, size_t heights_n,const int recombine,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  gmsh::model::geo::twist(intptr2pairvector(dimTags,dimTags_n),x,y,z,dx,dy,dz,ax,ay,az,angle,api_outDimTags_,ptr2vector(numElements,numElements_n),ptr2vector(heights,heights_n),recombine);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoTranslate(int* dimTags, size_t dimTags_n,const double dx,const double dy,const double dz,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::translate(intptr2pairvector(dimTags,dimTags_n),dx,dy,dz);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoRotate(int* dimTags, size_t dimTags_n,const double x,const double y,const double z,const double ax,const double ay,const double az,const double angle,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::rotate(intptr2pairvector(dimTags,dimTags_n),x,y,z,ax,ay,az,angle);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoDilate(int* dimTags, size_t dimTags_n,const double x,const double y,const double z,const double a,const double b,const double c,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::dilate(intptr2pairvector(dimTags,dimTags_n),x,y,z,a,b,c);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoSymmetry(int* dimTags, size_t dimTags_n,const double a,const double b,const double c,const double d,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::symmetry(intptr2pairvector(dimTags,dimTags_n),a,b,c,d);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoCopy(int* dimTags, size_t dimTags_n,int** outDimTags, size_t* outDimTags_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  gmsh::model::geo::copy(intptr2pairvector(dimTags,dimTags_n),api_outDimTags_);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoRemove(int* dimTags, size_t dimTags_n,const int recursive,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::remove(intptr2pairvector(dimTags,dimTags_n),recursive);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoRemoveAllDuplicates(int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::removeAllDuplicates();
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoSynchronize(int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::synchronize();
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoMeshSetSize(int* dimTags, size_t dimTags_n,const double size,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::mesh::setSize(intptr2pairvector(dimTags,dimTags_n),size);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoMeshSetTransfiniteLine(const int tag,const int nPoints,const char * type,const double coef,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::mesh::setTransfiniteLine(tag,nPoints,type,coef);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoMeshSetTransfiniteSurface(const int tag,const char * arrangement,int* cornerTags, size_t cornerTags_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::mesh::setTransfiniteSurface(tag,arrangement,ptr2vector(cornerTags,cornerTags_n));
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoMeshSetTransfiniteVolume(const int tag,int* cornerTags, size_t cornerTags_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::mesh::setTransfiniteVolume(tag,ptr2vector(cornerTags,cornerTags_n));
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoMeshSetRecombine(const int dim,const int tag,const double angle,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::mesh::setRecombine(dim,tag,angle);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoMeshSetSmoothing(const int dim,const int tag,const int val,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::mesh::setSmoothing(dim,tag,val);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelGeoMeshSetReverse(const int dim,const int tag,const int val,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::geo::mesh::setReverse(dim,tag,val);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

int gmshModelOccAddPoint(const double x,const double y,const double z,const double meshSize,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addPoint(x,y,z,meshSize,tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddLine(const int startTag,const int endTag,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addLine(startTag,endTag,tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddCircleArc(const int startTag,const int centerTag,const int endTag,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addCircleArc(startTag,centerTag,endTag,tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddCircle(const double x,const double y,const double z,const double r,const int tag,const double angle1,const double angle2,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addCircle(x,y,z,r,tag,angle1,angle2);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddEllipseArc(const int startTag,const int centerTag,const int endTag,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addEllipseArc(startTag,centerTag,endTag,tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddEllipse(const double x,const double y,const double z,const double r1,const double r2,const int tag,const double angle1,const double angle2,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addEllipse(x,y,z,r1,r2,tag,angle1,angle2);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddSpline(int* vertexTags, size_t vertexTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addSpline(ptr2vector(vertexTags,vertexTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddBezier(int* vertexTags, size_t vertexTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addBezier(ptr2vector(vertexTags,vertexTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddBSpline(int* vertexTags, size_t vertexTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addBSpline(ptr2vector(vertexTags,vertexTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddWire(int* edgeTags, size_t edgeTags_n,const int tag,const int checkClosed,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addWire(ptr2vector(edgeTags,edgeTags_n),tag,checkClosed);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddLineLoop(int* edgeTags, size_t edgeTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addLineLoop(ptr2vector(edgeTags,edgeTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddRectangle(const double x,const double y,const double z,const double dx,const double dy,const int tag,const double roundedRadius,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addRectangle(x,y,z,dx,dy,tag,roundedRadius);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddDisk(const double xc,const double yc,const double zc,const double rx,const double ry,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addDisk(xc,yc,zc,rx,ry,tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddPlaneSurface(int* wireTags, size_t wireTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addPlaneSurface(ptr2vector(wireTags,wireTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddSurfaceFilling(const int wireTag,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addSurfaceFilling(wireTag,tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddSurfaceLoop(int* faceTags, size_t faceTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addSurfaceLoop(ptr2vector(faceTags,faceTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddVolume(int* shellTags, size_t shellTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addVolume(ptr2vector(shellTags,shellTags_n),tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddSphere(const double xc,const double yc,const double zc,const double radius,const int tag,const double angle1,const double angle2,const double angle3,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addSphere(xc,yc,zc,radius,tag,angle1,angle2,angle3);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddBox(const double x,const double y,const double z,const double dx,const double dy,const double dz,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addBox(x,y,z,dx,dy,dz,tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddCylinder(const double x,const double y,const double z,const double dx,const double dy,const double dz,const double r,const int tag,const double angle,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addCylinder(x,y,z,dx,dy,dz,r,tag,angle);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddCone(const double x,const double y,const double z,const double dx,const double dy,const double dz,const double r1,const double r2,const int tag,const double angle,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addCone(x,y,z,dx,dy,dz,r1,r2,tag,angle);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddWedge(const double x,const double y,const double z,const double dx,const double dy,const double dz,const int tag,const double ltx,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addWedge(x,y,z,dx,dy,dz,tag,ltx);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddTorus(const double x,const double y,const double z,const double r1,const double r2,const int tag,const double angle,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::model::occ::addTorus(x,y,z,r1,r2,tag,angle);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddThruSections(int* wireTags, size_t wireTags_n,int** outDimTags, size_t* outDimTags_n,const int tag,const int makeSolid,const int makeRuled,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  result_api_ = gmsh::model::occ::addThruSections(ptr2vector(wireTags,wireTags_n),api_outDimTags_,tag,makeSolid,makeRuled);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccAddThickSolid(const int solidTag,int* excludeFaceTags, size_t excludeFaceTags_n,const double offset,int** outDimTags, size_t* outDimTags_n,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  result_api_ = gmsh::model::occ::addThickSolid(solidTag,ptr2vector(excludeFaceTags,excludeFaceTags_n),offset,api_outDimTags_,tag);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

void gmshModelOccExtrude(int* dimTags, size_t dimTags_n,const double dx,const double dy,const double dz,int** outDimTags, size_t* outDimTags_n,int* numElements, size_t numElements_n,double* heights, size_t heights_n,const int recombine,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  gmsh::model::occ::extrude(intptr2pairvector(dimTags,dimTags_n),dx,dy,dz,api_outDimTags_,ptr2vector(numElements,numElements_n),ptr2vector(heights,heights_n),recombine);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelOccRevolve(int* dimTags, size_t dimTags_n,const double x,const double y,const double z,const double ax,const double ay,const double az,const double angle,int** outDimTags, size_t* outDimTags_n,int* numElements, size_t numElements_n,double* heights, size_t heights_n,const int recombine,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  gmsh::model::occ::revolve(intptr2pairvector(dimTags,dimTags_n),x,y,z,ax,ay,az,angle,api_outDimTags_,ptr2vector(numElements,numElements_n),ptr2vector(heights,heights_n),recombine);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelOccAddPipe(int* dimTags, size_t dimTags_n,const int wireTag,int** outDimTags, size_t* outDimTags_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  gmsh::model::occ::addPipe(intptr2pairvector(dimTags,dimTags_n),wireTag,api_outDimTags_);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelOccFillet(int* regionTags, size_t regionTags_n,int* edgeTags, size_t edgeTags_n,const double radius,int** outDimTags, size_t* outDimTags_n,const int removeRegion,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  gmsh::model::occ::fillet(ptr2vector(regionTags,regionTags_n),ptr2vector(edgeTags,edgeTags_n),radius,api_outDimTags_,removeRegion);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

int gmshModelOccBooleanUnion(int* objectDimTags, size_t objectDimTags_n,int* toolDimTags, size_t toolDimTags_n,int** outDimTags, size_t* outDimTags_n,int*** outDimTagsMap, size_t** outDimTagsMap_n, size_t *outDimTagsMap_nn,const int tag,const int removeObject,const int removeTool,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  std::vector<gmsh::vector_pair >api_outDimTagsMap_;
  result_api_ = gmsh::model::occ::booleanUnion(intptr2pairvector(objectDimTags,objectDimTags_n),intptr2pairvector(toolDimTags,toolDimTags_n),api_outDimTags_,api_outDimTagsMap_,tag,removeObject,removeTool);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  pairvectorvector2intptrptr(api_outDimTagsMap_,outDimTagsMap,outDimTagsMap_n,outDimTagsMap_nn);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccBooleanIntersection(int* objectDimTags, size_t objectDimTags_n,int* toolDimTags, size_t toolDimTags_n,int** outDimTags, size_t* outDimTags_n,int*** outDimTagsMap, size_t** outDimTagsMap_n, size_t *outDimTagsMap_nn,const int tag,const int removeObject,const int removeTool,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  std::vector<gmsh::vector_pair >api_outDimTagsMap_;
  result_api_ = gmsh::model::occ::booleanIntersection(intptr2pairvector(objectDimTags,objectDimTags_n),intptr2pairvector(toolDimTags,toolDimTags_n),api_outDimTags_,api_outDimTagsMap_,tag,removeObject,removeTool);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  pairvectorvector2intptrptr(api_outDimTagsMap_,outDimTagsMap,outDimTagsMap_n,outDimTagsMap_nn);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccBooleanDifference(int* objectDimTags, size_t objectDimTags_n,int* toolDimTags, size_t toolDimTags_n,int** outDimTags, size_t* outDimTags_n,int*** outDimTagsMap, size_t** outDimTagsMap_n, size_t *outDimTagsMap_nn,const int tag,const int removeObject,const int removeTool,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  std::vector<gmsh::vector_pair >api_outDimTagsMap_;
  result_api_ = gmsh::model::occ::booleanDifference(intptr2pairvector(objectDimTags,objectDimTags_n),intptr2pairvector(toolDimTags,toolDimTags_n),api_outDimTags_,api_outDimTagsMap_,tag,removeObject,removeTool);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  pairvectorvector2intptrptr(api_outDimTagsMap_,outDimTagsMap,outDimTagsMap_n,outDimTagsMap_nn);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

int gmshModelOccBooleanFragments(int* objectDimTags, size_t objectDimTags_n,int* toolDimTags, size_t toolDimTags_n,int** outDimTags, size_t* outDimTags_n,int*** outDimTagsMap, size_t** outDimTagsMap_n, size_t *outDimTagsMap_nn,const int tag,const int removeObject,const int removeTool,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  std::vector<gmsh::vector_pair >api_outDimTagsMap_;
  result_api_ = gmsh::model::occ::booleanFragments(intptr2pairvector(objectDimTags,objectDimTags_n),intptr2pairvector(toolDimTags,toolDimTags_n),api_outDimTags_,api_outDimTagsMap_,tag,removeObject,removeTool);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  pairvectorvector2intptrptr(api_outDimTagsMap_,outDimTagsMap,outDimTagsMap_n,outDimTagsMap_nn);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

void gmshModelOccTranslate(int* dimTags, size_t dimTags_n,const double dx,const double dy,const double dz,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::occ::translate(intptr2pairvector(dimTags,dimTags_n),dx,dy,dz);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelOccRotate(int* dimTags, size_t dimTags_n,const double x,const double y,const double z,const double ax,const double ay,const double az,const double angle,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::occ::rotate(intptr2pairvector(dimTags,dimTags_n),x,y,z,ax,ay,az,angle);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelOccDilate(int* dimTags, size_t dimTags_n,const double x,const double y,const double z,const double a,const double b,const double c,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::occ::dilate(intptr2pairvector(dimTags,dimTags_n),x,y,z,a,b,c);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelOccSymmetry(int* dimTags, size_t dimTags_n,const double a,const double b,const double c,const double d,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::occ::symmetry(intptr2pairvector(dimTags,dimTags_n),a,b,c,d);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelOccCopy(int* dimTags, size_t dimTags_n,int** outDimTags, size_t* outDimTags_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  gmsh::model::occ::copy(intptr2pairvector(dimTags,dimTags_n),api_outDimTags_);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelOccRemove(int* dimTags, size_t dimTags_n,const int recursive,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::occ::remove(intptr2pairvector(dimTags,dimTags_n),recursive);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelOccRemoveAllDuplicates(int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::occ::removeAllDuplicates();
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelOccImportShapes(const char * fileName,int** outDimTags, size_t* outDimTags_n,const int highestDimOnly,const char * format,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::vector_pair api_outDimTags_;
  gmsh::model::occ::importShapes(fileName,api_outDimTags_,highestDimOnly,format);
  pairvector2intptr(api_outDimTags_,outDimTags,outDimTags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelOccSetMeshSize(int* dimTags, size_t dimTags_n,const double size,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::occ::setMeshSize(intptr2pairvector(dimTags,dimTags_n),size);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshModelOccSynchronize(int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::model::occ::synchronize();
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

int gmshViewAdd(const char * name,const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::view::add(name,tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

void gmshViewRemove(const int tag,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::view::remove(tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

int gmshViewGetIndex(const int tag,int * ierr){
  int result_api_;
  if(ierr) *ierr = 0;
  try {
  result_api_ = gmsh::view::getIndex(tag);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
  return result_api_;
}

void gmshViewGetTags(int** tags, size_t* tags_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  std::vector<int> api_tags_;
  gmsh::view::getTags(api_tags_);
  vector2ptr(api_tags_,tags,tags_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshViewAddModelData(const int tag,const char * modelName,const char * dataType,int* tags, size_t tags_n,const double** data, const size_t* data_n, size_t data_nn,const int step,const int time,const int numComponents,const int partition,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::view::addModelData(tag,modelName,dataType,ptr2vector(tags,tags_n),ptrptr2vectorvector(data,data_n,data_nn),step,time,numComponents,partition);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshViewAddListData(const int tag,const char * type,const int numEle,double* data, size_t data_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::view::addListData(tag,type,numEle,ptr2vector(data,data_n));
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshViewProbe(const int tag,const double x,const double y,const double z,double** value, size_t* value_n,const int step,const int numComp,const int gradient,const double tolerance,double* xElemCoord, size_t xElemCoord_n,double* yElemCoord, size_t yElemCoord_n,double* zElemCoord, size_t zElemCoord_n,int * ierr){
  if(ierr) *ierr = 0;
  try {
  std::vector<double> api_value_;
  gmsh::view::probe(tag,x,y,z,api_value_,step,numComp,gradient,tolerance,ptr2vector(xElemCoord,xElemCoord_n),ptr2vector(yElemCoord,yElemCoord_n),ptr2vector(zElemCoord,zElemCoord_n));
  vector2ptr(api_value_,value,value_n);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshViewWrite(const int tag,const char * fileName,const int append,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::view::write(tag,fileName,append);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshPluginSetNumber(const char * name,const char * option,const double value,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::plugin::setNumber(name,option,value);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshPluginSetString(const char * name,const char * option,const char * value,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::plugin::setString(name,option,value);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}

void gmshPluginRun(const char * name,int * ierr){
  if(ierr) *ierr = 0;
  try {
  gmsh::plugin::run(name);
  } catch(int api_ierr_) {if (ierr) *ierr = api_ierr_;}
}
