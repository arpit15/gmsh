// Gmsh - Copyright (C) 1997-2019 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// issues on https://gitlab.onelab.info/gmsh/gmsh/issues.
//
// Contributed by Ismail Badia.

#include "HierarchicalBasisHcurlQuad.h"

HierarchicalBasisHcurlQuad::HierarchicalBasisHcurlQuad(int order)

{
  _nvertex = 4;
  _nedge = 4;
  _nfaceQuad = 1;
  _nfaceTri = 0;
  _nVertexFunction = 0;
  _nEdgeFunction = 4 * order + 4;
  _nQuadFaceFunction = 2 * order * (order + 1);
  _nTriFaceFunction = 0;
  _nBubbleFunction = 0;
  _pf1 = order;
  _pf2 = order;
  _pOrderEdge[0] = order;
  _pOrderEdge[1] = order;
  _pOrderEdge[2] = order;
  _pOrderEdge[3] = order;
}

HierarchicalBasisHcurlQuad::~HierarchicalBasisHcurlQuad() {}

double HierarchicalBasisHcurlQuad::_affineCoordinate(int const &j,
                                                     double const &u,
                                                     double const &v)
{
  switch(j) {
  case(1): return 0.5 * (1 + u);
  case(2): return 0.5 * (1 - u);
  case(3): return 0.5 * (1 + v);
  case(4): return 0.5 * (1 - v);
  default: throw std::string("j must be : 1<=j<=4");
  }
}

void HierarchicalBasisHcurlQuad::generateBasis(
  double const &u, double const &v, double const &w,
  std::vector<std::vector<double> > &edgeBasis,
  std::vector<std::vector<double> > &faceBasis,
  std::vector<std::vector<double> > &bubbleBasis)
{
  double lambda1 = _affineCoordinate(1, u, v);
  double lambda2 = _affineCoordinate(2, u, v);
  double lambda3 = _affineCoordinate(3, u, v);
  double lambda4 = _affineCoordinate(4, u, v);
  std::vector<std::vector<double> > legendreVector(2);
  legendreVector[0] = std::vector<double>(
    std::max(std::max(_pOrderEdge[0] + 1, _pOrderEdge[2] + 1), _pf1 + 1));
  legendreVector[1] = std::vector<double>(
    std::max(std::max(_pOrderEdge[1] + 1, _pOrderEdge[3] + 1), _pf2 + 1));
  for(unsigned int k = 0; k < legendreVector[0].size(); k++) {
    legendreVector[0][k] = OrthogonalPoly::EvalLegendre(k, u);
  }
  for(unsigned int k = 0; k < legendreVector[1].size(); k++) {
    legendreVector[1][k] = OrthogonalPoly::EvalLegendre(k, v);
  }
  int edgeIt = 0;
  for(int i = 0; i < _nedge; i++) {
    int uv = 0;
    double lambda = 0;
    std::vector<double> direction(3, 0);
    switch(i) {
    case(0):
      lambda = lambda4;
      uv = 0;
      direction[0] = 1;
      break;
    case(1):
      lambda = lambda1;
      uv = 1;
      direction[1] = 1;
      break;
    case(2):
      lambda = lambda3;
      uv = 0;
      direction[0] = 1;
      break;
    case(3):
      lambda = lambda2;
      uv = 1;
      direction[1] = 1;
      break;
    }
    for(int iedge = 0; iedge < _pOrderEdge[i] + 1; iedge++) {
      for(int j = 0; j < 3; j++) {
        edgeBasis[edgeIt][j] =
          lambda * legendreVector[uv][iedge] * direction[j];
      }
      edgeIt++;
    }
  }
  int faceIt = 0;
  for(int n1 = 0; n1 <= _pf1; n1++) {
    for(int n2 = 2; n2 <= _pf2 + 1; n2++) {
      faceBasis[faceIt][0] =
        legendreVector[0][n1] * OrthogonalPoly::EvalLobatto(n2, v);
      faceBasis[faceIt][1] = 0;
      faceBasis[faceIt][2] = 0;
      faceIt++;
    }
  }
  for(int n1 = 2; n1 <= _pf1 + 1; n1++) {
    for(int n2 = 0; n2 <= _pf2; n2++) {
      faceBasis[faceIt][0] = 0;
      faceBasis[faceIt][1] =
        legendreVector[1][n2] * OrthogonalPoly::EvalLobatto(n1, u);
      faceBasis[faceIt][2] = 0;
      faceIt++;
    }
  }
}

void HierarchicalBasisHcurlQuad::orientEdge(
  int const &flagOrientation, int const &edgeNumber,
  std::vector<std::vector<double> > &edgeFunctions)
{
  if(flagOrientation == -1) {
    int constant1 = 0;
    int constant2 = 0;
    switch(edgeNumber) {
    case(0): {
      constant1 = 0;
      constant2 = _pOrderEdge[0];
    } break;
    case(1): {
      constant1 = _pOrderEdge[0] + 1;
      constant2 = _pOrderEdge[1] + _pOrderEdge[0] + 1;
    } break;
    case(2): {
      constant1 = _pOrderEdge[0] + _pOrderEdge[1] + 2;
      constant2 = _pOrderEdge[1] + _pOrderEdge[0] + _pOrderEdge[2] + 2;
    } break;
    case(3): {
      constant1 = _pOrderEdge[0] + _pOrderEdge[1] + _pOrderEdge[2] + 3;
      constant2 =
        _pOrderEdge[1] + _pOrderEdge[0] + _pOrderEdge[2] + _pOrderEdge[3] + 3;
    } break;
    default: throw std::string("edgeNumber  must be : 0<=edgeNumber<=3");
    }
    for(int k = constant1; k <= constant2; k++) {
      if((k - constant1) % 2 != 0) {
        edgeFunctions[k][0] = edgeFunctions[k][0] * (-1);
        edgeFunctions[k][1] = edgeFunctions[k][1] * (-1);
        edgeFunctions[k][2] = edgeFunctions[k][2] * (-1);
      }
    }
  }
}

void HierarchicalBasisHcurlQuad::generateCurlBasis(
  double const &u, double const &v, double const &w,
  std::vector<std::vector<double> > &edgeBasis,
  std::vector<std::vector<double> > &faceBasis,
  std::vector<std::vector<double> > &bubbleBasis)
{
  double dlambda1 = 0.5;
  double dlambda2 = -0.5;
  double dlambda3 = 0.5;
  double dlambda4 = -0.5;
  std::vector<std::vector<double> > legendreVector(2);
  legendreVector[0] = std::vector<double>(
    std::max(std::max(_pOrderEdge[0] + 1, _pOrderEdge[2] + 1), _pf1 + 1));
  legendreVector[1] = std::vector<double>(
    std::max(std::max(_pOrderEdge[1] + 1, _pOrderEdge[3] + 1), _pf2 + 1));
  for(unsigned int k = 0; k < legendreVector[0].size(); k++) {
    legendreVector[0][k] = OrthogonalPoly::EvalLegendre(k, u);
  }
  for(unsigned int k = 0; k < legendreVector[1].size(); k++) {
    legendreVector[1][k] = OrthogonalPoly::EvalLegendre(k, v);
  }
  int edgeIt = 0;
  for(int i = 0; i < _nedge; i++) {
    int uv = 0;
    double dlambda = 0;
    std::vector<double> direction(3, 0);
    direction[2] = 1;
    switch(i) {
    case(0):
      dlambda = -dlambda4;
      uv = 0;
      break;
    case(1):
      dlambda = dlambda1;
      uv = 1;
      break;
    case(2):
      dlambda = -dlambda3;
      uv = 0;
      break;
    case(3):
      dlambda = dlambda2;
      uv = 1;
      break;
    }
    for(int iedge = 0; iedge < _pOrderEdge[i] + 1; iedge++) {
      for(int j = 0; j < 3; j++) {
        edgeBasis[edgeIt][j] =
          dlambda * legendreVector[uv][iedge] * direction[j];
      }
      edgeIt++;
    }
  }
  int faceIt = 0;
  for(int n1 = 0; n1 <= _pf1; n1++) {
    for(int n2 = 2; n2 <= _pf2 + 1; n2++) {
      faceBasis[faceIt][0] = 0;
      faceBasis[faceIt][1] = 0;
      faceBasis[faceIt][2] =
        -legendreVector[0][n1] * OrthogonalPoly::EvalDLobatto(n2, v);
      faceIt++;
    }
  }
  for(int n1 = 2; n1 <= _pf1 + 1; n1++) {
    for(int n2 = 0; n2 <= _pf2; n2++) {
      faceBasis[faceIt][0] = 0;
      faceBasis[faceIt][1] = 0;
      faceBasis[faceIt][2] =
        legendreVector[1][n2] * OrthogonalPoly::EvalDLobatto(n1, u);
      faceIt++;
    }
  }
}

void HierarchicalBasisHcurlQuad::orientFace(
  double const &u, double const &v, double const &w, int const &flag1,
  int const &flag2, int const &flag3, int const &faceNumber,
  std::vector<std::vector<double> > &faceFunctions, std::string typeFunction)
{
  if(!(flag1 == 1 && flag2 == 1 && flag3 == 1)) {
    if(typeFunction == "HcurlLegendre") {
      if(flag3 == 1) {
        int iterator = 0;
        for(int it1 = 0; it1 <= _pf1; it1++) {
          for(int it2 = 2; it2 <= _pf2 + 1; it2++) {
            int impactFlag1 = 1;
            int impactFlag2 = 1;
            if(flag1 == -1 && it1 % 2 != 0) { impactFlag1 = -1; }
            if(flag2 == -1 && it2 % 2 != 0) { impactFlag2 = -1; }
            faceFunctions[iterator][0] =
              faceFunctions[iterator][0] * impactFlag1 * impactFlag2;

            iterator++;
          }
        }
        for(int it1 = 2; it1 <= _pf1 + 1; it1++) {
          for(int it2 = 0; it2 <= _pf2; it2++) {
            int impactFlag1 = 1;
            int impactFlag2 = 1;
            if(flag1 == -1 && it1 % 2 != 0) { impactFlag1 = -1; }
            if(flag2 == -1 && it2 % 2 != 0) { impactFlag2 = -1; }
            faceFunctions[iterator][1] =
              faceFunctions[iterator][0] * impactFlag1 * impactFlag2;

            iterator++;
          }
        }
      }
      else {
        std::vector<std::vector<double> > legendreVector(2);
        legendreVector[0] = std::vector<double>(_pf1 + 1);
        legendreVector[1] = std::vector<double>(_pf2 + 1);
        for(unsigned int k = 0; k < legendreVector[0].size(); k++) {
          legendreVector[0][k] = OrthogonalPoly::EvalLegendre(k, u);
        }
        for(unsigned int k = 0; k < legendreVector[1].size(); k++) {
          legendreVector[1][k] = OrthogonalPoly::EvalLegendre(k, v);
        }
        int iterator = 0;
        for(int it1 = 2; it1 <= _pf2 + 1; it1++) {
          for(int it2 = 0; it2 <= _pf1; it2++) {
            int impactFlag1 = 1;
            int impactFlag2 = 1;
            if(flag2 == -1 && it1 % 2 != 0) { impactFlag1 = -1; }
            if(flag1 == -1 && it2 % 2 != 0) { impactFlag2 = -1; }
            faceFunctions[iterator][0] = legendreVector[0][it2] *
                                         OrthogonalPoly::EvalLobatto(it1, v) *
                                         impactFlag1 * impactFlag2;
            iterator++;
          }
        }
        for(int it1 = 0; it1 <= _pf2; it1++) {
          for(int it2 = 2; it2 <= _pf1 + 1; it2++) {
            int impactFlag1 = 1;
            int impactFlag2 = 1;
            if(flag2 == -1 && it1 % 2 != 0) { impactFlag1 = -1; }
            if(flag1 == -1 && it2 % 2 != 0) { impactFlag2 = -1; }
            faceFunctions[iterator][1] = legendreVector[1][it1] *
                                         OrthogonalPoly::EvalLobatto(it2, v) *
                                         impactFlag1 * impactFlag2;
            iterator++;
          }
        }
      }
    }
    else if(typeFunction == "CurlHcurlLegendre") {
      if(flag3 == 1) {
        int iterator = 0;
        for(int it1 = 0; it1 <= _pf1; it1++) {
          for(int it2 = 2; it2 <= _pf2 + 1; it2++) {
            int impactFlag1 = 1;
            int impactFlag2 = 1;
            if(flag1 == -1 && it1 % 2 != 0) { impactFlag1 = -1; }
            if(flag2 == -1 && it2 % 2 != 0) { impactFlag2 = -1; }
            faceFunctions[iterator][2] =
              faceFunctions[iterator][2] * impactFlag1 * impactFlag2;

            iterator++;
          }
        }
        for(int it1 = 2; it1 <= _pf1 + 1; it1++) {
          for(int it2 = 0; it2 <= _pf2; it2++) {
            int impactFlag1 = 1;
            int impactFlag2 = 1;
            if(flag1 == -1 && it1 % 2 != 0) { impactFlag1 = -1; }
            if(flag2 == -1 && it2 % 2 != 0) { impactFlag2 = -1; }
            faceFunctions[iterator][1] =
              faceFunctions[iterator][2] * impactFlag1 * impactFlag2;

            iterator++;
          }
        }
      }
      else {
        std::vector<std::vector<double> > legendreVector(2);
        legendreVector[0] = std::vector<double>(_pf1 + 1);
        legendreVector[1] = std::vector<double>(_pf2 + 1);
        for(unsigned int k = 0; k < legendreVector[0].size(); k++) {
          legendreVector[0][k] = OrthogonalPoly::EvalLegendre(k, u);
        }
        for(unsigned int k = 0; k < legendreVector[1].size(); k++) {
          legendreVector[1][k] = OrthogonalPoly::EvalLegendre(k, v);
        }
        int iterator = 0;
        for(int it1 = 2; it1 <= _pf2 + 1; it1++) {
          for(int it2 = 0; it2 <= _pf1; it2++) {
            int impactFlag1 = 1;
            int impactFlag2 = 1;
            if(flag2 == -1 && it1 % 2 != 0) { impactFlag1 = -1; }
            if(flag1 == -1 && it2 % 2 != 0) { impactFlag2 = -1; }
            faceFunctions[iterator][2] = -legendreVector[0][it2] *
                                         OrthogonalPoly::EvalDLobatto(it1, v) *
                                         impactFlag1 * impactFlag2;
            iterator++;
          }
        }
        for(int it1 = 0; it1 <= _pf2; it1++) {
          for(int it2 = 2; it2 <= _pf1 + 1; it2++) {
            int impactFlag1 = 1;
            int impactFlag2 = 1;
            if(flag2 == -1 && it1 % 2 != 0) { impactFlag1 = -1; }
            if(flag1 == -1 && it2 % 2 != 0) { impactFlag2 = -1; }
            faceFunctions[iterator][2] = legendreVector[1][it1] *
                                         OrthogonalPoly::EvalDLobatto(it2, v) *
                                         impactFlag1 * impactFlag2;
            iterator++;
          }
        }
      }
    }
    else {
      throw std::string("unknown typeFunction");
    }
  }
}
