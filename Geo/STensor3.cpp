// Gmsh - Copyright (C) 1997-2014 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// bugs and problems to the public mailing list <gmsh@geuz.org>.

#include <algorithm>
#include "STensor3.h"

// compute the largest inscribed ellipsoid

void SMetric3::print (const char *s) const
{
  printf(" metric %s : %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E \n", s,
         (*this)(0,0), (*this)(1,1), (*this)(2,2),
         (*this)(0,1), (*this)(0,2), (*this)(1,2));
}

void STensor3::print (const char *s) const
{
  printf(" tensor %s : \n"
         " %12.5E %12.5E %12.5E \n %12.5E %12.5E %12.5E \n %12.5E %12.5E %12.5E \n", s,
         (*this)(0,0), (*this)(0,1), (*this)(0,2),
         (*this)(1,0), (*this)(1,1), (*this)(1,2),
         (*this)(2,0), (*this)(2,1), (*this)(2,2));
}

SMetric3 intersection (const SMetric3 &m1, const SMetric3 &m2)
{
  SMetric3 im1 = m1.invert();
  fullMatrix<double> V(3,3);
  fullVector<double> S(3);
  im1 *= m2;
  im1.eig(V,S,true);
  SVector3 v0(V(0,0),V(1,0),V(2,0));
  SVector3 v1(V(0,1),V(1,1),V(2,1));
  SVector3 v2(V(0,2),V(1,2),V(2,2));
  double l0 = std::max(dot(v0,m1,v0),dot(v0,m2,v0));
  double l1 = std::max(dot(v1,m1,v1),dot(v1,m2,v1));
  double l2 = std::max(dot(v2,m1,v2),dot(v2,m2,v2));

  // Correction from the PhD thesis of Frederic Alauzet p.16
  // If m2 = alpha*m1, then take the largest metric
  static const double eps = 1.e-2;  // Tolerance to detect triple eigenvalue
                                    // (i.e. proportional metrics)
  const double max_eig = std::max(S(0), std::max(S(1), S(2)));
  const double min_eig = std::min(S(0), std::min(S(1), S(2)));
  const double range_eig = fabs((max_eig-min_eig)/max_eig);
  if (range_eig < eps) return (max_eig >= 1.) ? m2 : m1;

  SMetric3 iv(l0,l1,l2,v0,v1,v2);
  return iv;
}

SMetric3 intersection_alauzet (const SMetric3 &m1, const SMetric3 &m2)
{
  SMetric3 im1 = m1.invert();
  fullMatrix<double> V(3,3);
  fullVector<double> S(3);
  im1 *= m2;
  im1.eig(V,S,true);
  SVector3 v0(V(0,0),V(1,0),V(2,0));
  SVector3 v1(V(0,1),V(1,1),V(2,1));
  SVector3 v2(V(0,2),V(1,2),V(2,2));
  // is this required??
  v0.normalize();
  v1.normalize();
  v2.normalize();
  double l0 = std::max(dot(v0,m1,v0),dot(v0,m2,v0));
  double l1 = std::max(dot(v1,m1,v1),dot(v1,m2,v1));
  double l2 = std::max(dot(v2,m1,v2),dot(v2,m2,v2));
  SMetric3 iv(l0,l1,l2,v0,v1,v2);
  return iv;
}

// preserve orientation of m1 !!!
SMetric3 intersection_conserveM1 (const SMetric3 &m1, const SMetric3 &m2)
{
  // we should do
  // return intersection (m1,m2);
  fullMatrix<double> V(3,3);
  fullVector<double> S(3);
  m1.eig(V,S,true);
  SVector3 v0(V(0,0),V(1,0),V(2,0));
  SVector3 v1(V(0,1),V(1,1),V(2,1));
  SVector3 v2(V(0,2),V(1,2),V(2,2));
  double l0 = std::max(dot(v0,m1,v0),dot(v0,m2,v0));
  double l1 = std::max(dot(v1,m1,v1),dot(v1,m2,v1));
  double l2 = std::max(dot(v2,m1,v2),dot(v2,m2,v2));
  SMetric3 iv(l0,l1,l2,v0,v1,v2);
  return iv;
}

namespace {

double anisoRatio2D (double v0x, double v0y, double v0z,
                     double v1x, double v1y, double v1z,
                     double v2x, double v2y, double v2z,
                     double s0, double s1, double s2) {

  static const double eps = 1.e-8;

  double ratio;
  if (v0x*v0x+v0y*v0y+(v0z-1.)*(v0z-1.) < eps)        // If 1st eigenvect corresponds to z dir. ...
    ratio = std::min(fabs(s1/s2),fabs(s2/s1));        // ... consider ratio of 2nd and 3rd eigenval.
  else if (v1x*v1x+v1y*v1y+(v1z-1.)*(v1z-1.) < eps)   // If 2nd eigenvect corresponds to z dir. ...
    ratio = std::min(fabs(s0/s2),fabs(s2/s0));        // ... consider ratio of 1st and 3rd eigenval.
  else if (v2x*v2x+v2y*v2y+(v2z-1.)*(v2z-1.) < eps)   // If 3rd eigenvect corresponds to z dir. ...
    ratio = std::min(fabs(s0/s1),fabs(s1/s0));        // ... consider ratio of 1st and 2nd eigenval.
  else {
    printf("Error in anisoRatio2D: z direction not found!\n");
    ratio = 0.;
  }

  return ratio;

}

}

// preserve orientation of the most anisotropic metric !!!
SMetric3 intersection_conserve_mostaniso (const SMetric3 &m1, const SMetric3 &m2)
{
  fullMatrix<double> V1(3,3);
  fullVector<double> S1(3);
  m1.eig(V1,S1,true);
  double ratio1 = fabs(S1(0)/S1(2));  // Minimum ratio because we take sorted eigenvalues
  fullMatrix<double> V2(3,3);
  fullVector<double> S2(3);
  m2.eig(V2,S2,true);
  double ratio2 = fabs(S2(0)/S2(2));  // Minimum ratio because we take sorted eigenvalues

  if (ratio1 < ratio2)
    return intersection_conserveM1(m1, m2);
  else
    return intersection_conserveM1(m2, m1);
}

// preserve orientation of the most anisotropic metric in 2D!!!
SMetric3 intersection_conserve_mostaniso_2d (const SMetric3 &m1, const SMetric3 &m2)
{

  fullMatrix<double> V1(3,3);
  fullVector<double> S1(3);
  m1.eig(V1,S1,false);
  double ratio1 = anisoRatio2D(V1(0,0),V1(1,0),V1(2,0),
                               V1(0,1),V1(1,1),V1(2,1),
                               V1(0,2),V1(1,2),V1(2,2),
                               S1(0),S1(1),S1(2));

  fullMatrix<double> V2(3,3);
  fullVector<double> S2(3);
  m2.eig(V2,S2,false);
  double ratio2 = anisoRatio2D(V2(0,0),V2(1,0),V2(2,0),
                               V2(0,1),V2(1,1),V2(2,1),
                               V2(0,2),V2(1,2),V2(2,2),
                               S2(0),S2(1),S2(2));

  if (ratio1 < ratio2)
    return intersection_conserveM1(m1, m2);
  else
    return intersection_conserveM1(m2, m1);

}

// (1-t) * m1 + t * m2
SMetric3 interpolation (const SMetric3 &m1,
			const SMetric3 &m2,
			const double t)
{
  SMetric3 im1 = m1.invert();
  SMetric3 im2 = m2.invert();
  im1 *= (1.-t);
  im2 *= t;
  im1 += im2;
  return im1.invert();
}

SMetric3 interpolation (const SMetric3 &m1,
                        const SMetric3 &m2,
                        const SMetric3 &m3,
                        const double u,
                        const double v)
{
  SMetric3 im1 = m1.invert();
  SMetric3 im2 = m2.invert();
  SMetric3 im3 = m3.invert();
  im1 *= (1.-u-v);
  im2 *= u;
  im3 *= v;
  im1 += im2;
  im1 += im3;
  return im1.invert();
}

SMetric3 interpolation (const SMetric3 &m1,
                        const SMetric3 &m2,
                        const SMetric3 &m3,
                        const SMetric3 &m4,
                        const double u,
                        const double v,
                        const double w)
{
  SMetric3 im1 = m1.invert();
  SMetric3 im2 = m2.invert();
  SMetric3 im3 = m3.invert();
  SMetric3 im4 = m4.invert();
  im1 *= (1.-u-v-w);
  im2 *= u;
  im3 *= v;
  im4 *= w;
  im1 += im2;
  im1 += im3;
  im1 += im4;
  return im1.invert();
}
