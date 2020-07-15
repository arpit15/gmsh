#include "hxt_point_gen.h"
#include "hxt_point_gen_1d.h"
#include "hxt_point_gen_2d.h"
#include "hxt_point_gen_3d.h"
#include "hxt_surface_mesh.h"
#include "hxt_point_gen_realloc.h"
#include "hxt_point_gen_utils.h"
#include "hxt_point_gen_io.h"
#include "hxt_point_gen_optim.h"

#include "hxt_bbox.h"
#include "hxt_edge.h"

#include "hxt_triangle_quality.h"
#include "hxt_point_gen_numerics.h"

#include "hxt_post_debugging.h"

HXTStatus hxtGeneratePointsMain(HXTMesh *mesh, 
                                HXTPointGenOptions *opt, 
                                double *sizemap, 
                                double *directions,
                                HXTMesh *nmesh)   
{


  

  //*************************************************************************************
  //*************************************************************************************
  // Create a temp mesh to hold generated vertices
  HXTMesh *fmesh;
  HXT_CHECK(hxtMeshCreate(&fmesh));
 
  //*************************************************************************************
  //*************************************************************************************
  // Create edges structure
  HXTEdges* edges;
  HXT_CHECK(hxtEdgesCreateNonManifold(mesh,&edges));


  //*************************************************************************************
  //*************************************************************************************
  if (0) HXT_CHECK(hxtCheckZeroAreaTriangles(mesh,"FINALMESHtriangleWithZeroArea.msh"));


  //*************************************************************************************
  //*************************************************************************************
  // Sizing from input mesh 
  // TODO 
  if (0){
    HXT_CHECK(hxtPointGenGetSizesInputMesh(edges,3.0,sizemap));
    HXT_CHECK(hxtPointGenSizemapSmoothing(edges,1.5,sizemap));
    HXT_CHECK(hxtPointGenWriteScalarTriangles(mesh,sizemap,"sizesInput.msh"));
  }
  
  //*************************************************************************************
  //*************************************************************************************
  // Sizing from curvature
  // TODO 
  if(0){
    HXT_CHECK(hxtPointGenGetSizesCurvature(edges,100,0.8,0.1,sizemap));
    //HXT_CHECK(hxtPointGenSizemapSmoothing(edges,1.5,sizemap));
    HXT_CHECK(hxtPointGenWriteScalarTriangles(mesh,sizemap,"sizesCurvature.msh"));
  }

  //*************************************************************************************
  //*************************************************************************************
  // If mesh does not have lines start propagation from a random edge
  // TODO 
  if (mesh->lines.num == 0 && opt->generateSurfaces){
    HXT_INFO("********** ATTENTION **********");
    HXT_INFO("Mesh does not have lines - starting propagation from a random edge");
    // Create edges structure
    hxtLinesReserve(mesh,2);
    mesh->lines.colors[0] = 0;
    mesh->lines.node[0] = edges->node[0];
    mesh->lines.node[1] = edges->node[1];
    mesh->lines.num++;

    for (uint64_t i=0; i<mesh->triangles.num; i++){
      mesh->triangles.colors[i] = 1;
    }
  }

  //*************************************************************************************
  //*************************************************************************************
  // Extract mesh lines based on dihedral angle 
  // TODO rewrite this function
  if (0) HXT_CHECK(hxtPointGenClassifyDihedralLines(mesh,edges));



  //*************************************************************************************
  //*************************************************************************************
  // Boundary vertex connectivity 
  // TODO not used for now 
  uint32_t *vert2vert;
  HXT_CHECK(hxtMalloc(&vert2vert, 2*mesh->vertices.num*sizeof(uint32_t)));
  for (uint32_t i=0; i<2*mesh->vertices.num; i++) vert2vert[i] = UINT32_MAX;

  for (uint64_t i=0; i<mesh->lines.num; i++){
    uint32_t v0 = mesh->lines.node[2*i+0];
    uint32_t v1 = mesh->lines.node[2*i+1];
    vert2vert[2*v0+1] = v1;
    vert2vert[2*v1+0] = v0;
  }
  HXT_CHECK(hxtFree(&vert2vert));




  //*************************************************************************************
  // Create and allocate 
  //*************************************************************************************
  HXT_INFO("");
  HXT_INFO("Initial mesh num of vertices         %d",  mesh->vertices.num);
  HXT_INFO("Initial mesh num of points           %d",  mesh->points.num);
  HXT_INFO("Initial mesh num of lines            %lu",  mesh->lines.num);
  HXT_INFO("Initial mesh num of triangles        %lu",  mesh->triangles.num);
  HXT_INFO("Initial mesh num of quads            %lu",  mesh->quads.num);
  HXT_INFO("Initial mesh num of tetrahedra       %lu",  mesh->tetrahedra.num);

  // Estimate number of generated vertices 
  uint32_t estNumVertices = 0;
  HXT_CHECK(hxtEstimateNumOfVerticesWithMesh(mesh,sizemap,&estNumVertices));
  if (estNumVertices < mesh->lines.num) estNumVertices += mesh->lines.num;
  estNumVertices = 10000000; // TODO good estimation ? 

  // Allocate final mesh vertices
  HXT_CHECK(hxtVerticesReserve(fmesh, estNumVertices));

  // Allocate final mesh 1-d point elements (corners)
  if (mesh->points.num != 0){
    HXT_CHECK(hxtPointsReserve(fmesh,mesh->points.num));
  }
  else{
    HXT_CHECK(hxtPointsReserve(fmesh,estNumVertices)); // TODO this is way too big - reallocate down
  }

  // Allocate final mesh lines
  uint64_t estNumLines = estNumVertices; // TODO that is a big number for mesh lines! - reallocate down
  HXT_CHECK(hxtLinesReserve(fmesh, estNumLines));

  HXT_INFO("");
  HXT_INFO("Allocated mesh num of vertices       %d",  fmesh->vertices.size); 
  HXT_INFO("Allocated mesh num of points         %d",  fmesh->points.size); 
  HXT_INFO("Allocated mesh num of lines          %lu", fmesh->lines.size); 
  
  // Create structure for parent elements of generated points 
  HXTPointGenParent *parent;
  HXT_CHECK(hxtMalloc(&parent, estNumVertices*sizeof(HXTPointGenParent)));
  for (uint32_t i=0; i<estNumVertices; i++) parent[i].type = UINT8_MAX;
  for (uint32_t i=0; i<estNumVertices; i++) parent[i].id = UINT64_MAX;
  
  //**********************************************************************************************************
  //**********************************************************************************************************
  // GENERATE POINTS ON LINES 
  //**********************************************************************************************************
  //**********************************************************************************************************

  if (opt->generateLines){
    HXT_CHECK(hxtGeneratePointsOnLines(mesh,opt,directions,sizemap,fmesh,parent));
  }
  else if (opt->generateLines == 0 && opt->generateSurfaces == 1){
    HXT_CHECK(hxtGetPointsOnLinesFromInputMesh(mesh,opt,fmesh,parent));
  }
    
  // Check if there is even number of points generated on lines 
  // TODO 
/*  if (fmesh->lines.num%2 != 0){*/
    /*printf("\n\nContinue with odd number of points? y/n \n\n");*/
    /*char ch = getchar();*/
    /*getchar();*/
    /*if ( ch == 'n') return HXT_ERROR_MSG(HXT_STATUS_ERROR,"Number of points on lines not even");*/
  /*}*/

  // Checking correct number of 1d point elements (i.e. "corners")
  // TODO delete
  uint32_t countCorners = 0;
  for (uint32_t i=0; i<fmesh->vertices.num; i++){
    if (parent[i].type == 15) countCorners++;
  }
  if (countCorners != fmesh->points.num) 
    return HXT_ERROR_MSG(HXT_STATUS_FAILED,"Corner problem %d %d", countCorners, fmesh->points.num);

  if (opt->verbosity > 1) HXT_CHECK(hxtMeshWriteGmsh(fmesh, "FINALMESHlines.msh"));

  HXT_INFO("");
  HXT_INFO("Number of points generated on lines = %d", fmesh->vertices.num);

  uint32_t numberOfPointsOnLines = fmesh->vertices.num;

  
  //**********************************************************************************************************
  // Create array with binary alternating indices 
  //**********************************************************************************************************
  uint32_t *bin;
  // Read bin file if we generate only volumes
  if (opt->generateLines == 0 && opt->generateSurfaces == 0 && opt->generateVolumes){
    uint32_t numVert = 0;
    HXT_CHECK(hxtPointGenReadBinIndices("binInput.txt",fmesh->vertices.size,&numVert,&bin));
  }
  else{
    HXT_CHECK(hxtMalloc(&bin,sizeof(uint32_t)*fmesh->vertices.size));
    for (uint32_t i=0; i<fmesh->vertices.size; i++) bin[i] = UINT32_MAX;

    for (uint32_t i=0; i<fmesh->points.num; i++){
      bin[fmesh->points.node[i]] = 0;
    }

    for (uint32_t i=0; i<fmesh->lines.num; i++){
      uint32_t v0 = fmesh->lines.node[2*i+0];
      uint32_t v1 = fmesh->lines.node[2*i+1];
      if (bin[v0] == UINT32_MAX)
        bin[v0] = bin[v1] == 0 ? 1:0;
      if (bin[v1] == UINT32_MAX)
        bin[v1] = bin[v0] == 0 ? 1:0;
    }
  }

    
  // TODO delete
  if(opt->verbosity == 2){
    FILE *test;
    hxtPosInit("binLines.pos","bin",&test);
    for (uint32_t i=0; i<fmesh->vertices.num; i++){
      hxtPosAddText(test,&fmesh->vertices.coord[4*i],"%d",bin[i]);
    }
    hxtPosFinish(test);
  }

  //**********************************************************************************************************
  //**********************************************************************************************************
  // GENERATE POINTS ON SURFACES 
  //**********************************************************************************************************
  //**********************************************************************************************************

  if (opt->generateSurfaces){
    //HXT_CHECK(hxtGeneratePointsOnSurfacePropagate(opt,mesh,edges,sizemap,directions,parent,fmesh));
    HXT_CHECK(hxtGeneratePointsOnSurface(opt,mesh,edges,sizemap,directions,parent,fmesh,bin));
  }
  else{
    HXT_CHECK(hxtGetPointsOnSurfacesFromInputMesh(mesh,opt,fmesh,parent));
  }

  if (opt->verbosity > 1) HXT_CHECK(hxtPointGenExportPointsToPos(fmesh,"pointsSurface.pos"));

  uint32_t numberOfPointsOnSurfaces = fmesh->vertices.num - numberOfPointsOnLines;

  HXT_INFO("");
  HXT_INFO("Number of points generated on surfaces = %d", numberOfPointsOnSurfaces);
  HXT_INFO("Number of points generated on TOTAL    = %d", fmesh->vertices.num);

  // TODO delete
  if(opt->verbosity == 2){
    FILE *test;
    hxtPosInit("binSurfaces.pos","bin",&test);
    for (uint32_t i=0; i<fmesh->vertices.num; i++){
      hxtPosAddText(test,&fmesh->vertices.coord[4*i],"%d",bin[i]);
    }
    hxtPosFinish(test);
  }



  // TODO just for checking, delete
  // Check if parent elements are correct
  double tol = 10e-7;
  double dist,t,closePt[3];
  for (uint32_t i=0; i<fmesh->vertices.num; i++){
    if (parent[i].type == 1){
    
      uint64_t parentLine = parent[i].id;
      double *p0 = mesh->vertices.coord + 4*mesh->lines.node[2*parentLine+0];
      double *p1 = mesh->vertices.coord + 4*mesh->lines.node[2*parentLine+1];
      double *pp = fmesh->vertices.coord + 4*i;
      HXT_CHECK(hxtSignedDistancePointEdge(p0,p1,pp,&dist,&t,closePt));
      if (fabs(dist)>tol) 
        return HXT_ERROR_MSG(HXT_STATUS_FAILED,"Line parent wrong for %d, dist = %e",i,dist);
    }
    else if (parent[i].type == 15){
      uint32_t parentPoint = parent[i].id;
      double *p0 = mesh->vertices.coord + 4*parentPoint;
      double *p1 = fmesh->vertices.coord + 4*i;
      dist = distance(p0,p1);
      if (fabs(dist)>tol) 
        return HXT_ERROR_MSG(HXT_STATUS_FAILED,"Point parent wrong for %d, dist = %e",i,dist);
    }
    else if (parent[i].type == 2){
      uint64_t parentTriangle = parent[i].id;
      double *p0 = mesh->vertices.coord + 4*mesh->triangles.node[3*parentTriangle+0];
      double *p1 = mesh->vertices.coord + 4*mesh->triangles.node[3*parentTriangle+1];
      double *p2 = mesh->vertices.coord + 4*mesh->triangles.node[3*parentTriangle+2];
      double *pp = fmesh->vertices.coord + 4*i;
      int inside = 0;
      HXT_CHECK(hxtSignedDistancePointTriangle(p0,p1,p2,pp,&dist,&inside,closePt));
      if (fabs(dist)>tol)
        return HXT_ERROR_MSG(HXT_STATUS_FAILED,"Triangle parent wrong for %d, dist = %e",i,dist);
    }
  }


  //**********************************************************************************************************
  //**********************************************************************************************************
  // GENERATE POINTS ON VOLUMES 
  //**********************************************************************************************************
  //**********************************************************************************************************
  


  
 
  if (opt->generateVolumes){
  
    HXT_CHECK(hxtGeneratePointsOnVolumes(mesh,opt,sizemap,directions,parent,fmesh,bin));
  
    if (opt->verbosity > 1) HXT_CHECK(hxtPointGenExportPointsToPos(fmesh,"pointsVolume.pos"));

  }
  uint32_t numberOfPointsOnVolumes = fmesh->vertices.num - numberOfPointsOnLines - numberOfPointsOnSurfaces;
  HXT_INFO("");
  HXT_INFO("Number of points generated on volumes  = %d", numberOfPointsOnVolumes);
  HXT_INFO("Number of points generated on TOTAL    = %d", fmesh->vertices.num);


  if(opt->verbosity == 2){
    FILE *test;
    hxtPosInit("binVolumes.pos","bin",&test);
    for (uint32_t i=0; i<fmesh->vertices.num; i++){
      hxtPosAddText(test,&fmesh->vertices.coord[4*i],"%d",bin[i]);
    }
    hxtPosFinish(test);
  }







  //**********************************************************************************************************
  //**********************************************************************************************************
  // CREATE SURFACE MESH 
  //**********************************************************************************************************
  //**********************************************************************************************************
 

  //**********************************************************************************************************
  // To keep a global relation of vertices (initial+generated) with input mesh triangles
  uint64_t *p2t;
  uint64_t totalVertices = mesh->vertices.num + 2*fmesh->vertices.num;
  HXT_CHECK(hxtMalloc(&p2t,sizeof(uint64_t)*totalVertices));
  for (uint64_t i=0; i<totalVertices; i++) p2t[i] = UINT64_MAX;

  //**********************************************************************************************************
  // To keep a relation of generated vertices on the final mesh and the index of fmesh generated points  
  uint32_t *p2p;
  HXT_CHECK(hxtMalloc(&p2p,sizeof(uint32_t)*totalVertices));
  for (uint32_t i=0; i<totalVertices; i++) p2p[i] = UINT32_MAX ;



  
  if (opt->remeshSurfaces){
    // Find max length to create an area threshold 
    // TODO use that 
    double maxLength = 0;
    HXT_CHECK(hxtPointGenModelMaxLength(mesh->vertices.coord,mesh->vertices.num,&maxLength));
    opt->areaThreshold = maxLength*maxLength*10e-8;
    
    // Surface remesher
    HXT_CHECK(hxtSurfaceMesh(opt,mesh,edges,directions,sizemap,parent,p2t,p2p,fmesh,nmesh));
  }
  else{
    if (opt->generateLines==0 && opt->generateSurfaces==0){
      HXT_CHECK(hxtGetSurfaceMesh(opt,mesh,fmesh,nmesh));
    }
    else{
      return HXT_ERROR_MSG(HXT_STATUS_ERROR,"not working for now");
    }
  }



  //********************************************************
  // A LOT OF DIFFERENT MESH STRUCTURES 
  // -  mesh -> initial mesh 
  // - fmesh -> holds feature points, lines, and generated vertices 
  // - nmesh -> holds final mesh triangles and vertices (without lines and points)
 
  if (opt->generateVolumes){
    for (uint32_t i=0; i<fmesh->vertices.num; i++){
      if (parent[i].type != 4) continue;
      nmesh->vertices.coord[4*nmesh->vertices.num+0] = fmesh->vertices.coord[4*i+0];
      nmesh->vertices.coord[4*nmesh->vertices.num+1] = fmesh->vertices.coord[4*i+1];
      nmesh->vertices.coord[4*nmesh->vertices.num+2] = fmesh->vertices.coord[4*i+2];
      nmesh->vertices.num++;
    }
  }


  //**********************************************************************************************************
  //**********************************************************************************************************
  // TEST 20/11/2019 
  // QUALITY COMPUTATION BASED ON DISTANCE FROM CIRCUMCIRCLE CENTER
  // TODO 
  //**********************************************************************************************************
  //**********************************************************************************************************
  if(0) HXT_CHECK(hxtSurfaceMeshExportTriangleQuality(nmesh,sizemap));
  if(0) HXT_CHECK(hxtSurfaceMeshExportAlignedEdges(mesh,nmesh,p2t,directions,sizemap));

  //**********************************************************************************************************
  //**********************************************************************************************************
  // Simple smoothing
  // TO BE REPLACED with right-angled optimization
  // TODO 
  //**********************************************************************************************************
  //**********************************************************************************************************
  if(0) HXT_CHECK(hxtPointGenSmoothing(nmesh));


  //**********************************************************************************************************
  //**********************************************************************************************************
  // Constructing a quad mesh template from binary indices 
  //**********************************************************************************************************
  //**********************************************************************************************************

  if (opt->quadSurfaces == 1){

    if(opt->verbosity == 2){
      HXT_CHECK(hxtMeshWriteGmsh(nmesh, "finalmeshNOSMOOTH.msh"));
      FILE *test;
      hxtPosInit("binIndices.pos","edges",&test);
      for (uint32_t i=0; i<nmesh->vertices.num; i++){
        hxtPosAddPoint(test,&nmesh->vertices.coord[4*i],0);
        hxtPosAddText(test,&nmesh->vertices.coord[4*i],"%d",bin[p2p[i]]);

      }
      hxtPosFinish(test);
    }

    // Transfer binary indices to final surface mesh 
    uint32_t *tbin;
    HXT_CHECK(hxtMalloc(&tbin,sizeof(uint32_t)*fmesh->vertices.size));
    for (uint32_t i=0; i<fmesh->vertices.size; i++) tbin[i] = bin[i];
    for (uint32_t i=0; i<nmesh->vertices.num; i++){
      bin[i] = tbin[p2p[i]];
    }
    for (uint32_t i=nmesh->vertices.num; i<fmesh->vertices.size; i++){
      bin[i] = UINT32_MAX;
    }
    HXT_CHECK(hxtFree(&tbin));

    // Convert final mesh "nmesh" to a quad mesh "qmesh"
    // and then rewrite qmesh onto nmesh
    HXT_CHECK(hxtPointGenQuadSmoothing(opt,mesh,nmesh,p2t,bin));

    if(opt->verbosity == 2){
      FILE *test;
      hxtPosInit("binIndicesAfter.pos","edges",&test);
      for (uint32_t i=0; i<nmesh->vertices.num; i++){
        hxtPosAddPoint(test,&nmesh->vertices.coord[4*i],0);
        hxtPosAddText(test,&nmesh->vertices.coord[4*i],"%d",bin[i]);

      }
      hxtPosFinish(test);
    }
  }




  // Output binary indices for subsequent input to volume point generation
  if(opt->verbosity==2){
    FILE *test = fopen("binInput.txt","w");
    fprintf(test,"%d\n",nmesh->vertices.num);
    for (uint32_t i=0; i<nmesh->vertices.num; i++){
      fprintf(test,"%d\n",bin[i]);
    }
    fprintf(test,"\n");
    fclose(test);
  }
 


  //**********************************************************************************************************
  //**********************************************************************************************************
  // Clear things
  //**********************************************************************************************************
  //**********************************************************************************************************
  
  HXT_CHECK(hxtMeshDelete(&fmesh));
 

  HXT_CHECK(hxtFree(&parent));
  HXT_CHECK(hxtEdgesDelete(&edges));

  HXT_CHECK(hxtFree(&p2t));
  HXT_CHECK(hxtFree(&p2p));
  HXT_CHECK(hxtFree(&bin));

  return HXT_STATUS_OK; 
}

