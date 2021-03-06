//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "RankTwoScalarTools.h"

// MOOSE includes
#include "MooseEnum.h"
#include "RankTwoTensor.h"
#include "MooseError.h"

namespace RankTwoScalarTools
{

MooseEnum
scalarOptions()
{
  return MooseEnum("VonMisesStress EffectiveStrain Hydrostatic L2norm MaxPrincipal "
                   "MidPrincipal MinPrincipal VolumetricStrain FirstInvariant SecondInvariant "
                   "ThirdInvariant AxialStress HoopStress RadialStress TriaxialityStress "
                   "Direction");
}

Real
getQuantity(const RankTwoTensor & tensor,
            const MooseEnum scalar_type,
            const Point & point1,
            const Point & point2,
            const Point & curr_point,
            Point & direction)
{
  Real val = 0.0;

  switch (scalar_type)
  {
    case 0:
      val = vonMisesStress(tensor);
      break;
    case 1:
      val = effectiveStrain(tensor);
      break;
    case 2:
      val = hydrostatic(tensor);
      break;
    case 3:
      val = L2norm(tensor);
      break;
    case 4:
      val = maxPrincipal(tensor, direction);
      break;
    case 5:
      val = midPrincipal(tensor, direction);
      break;
    case 6:
      val = minPrincipal(tensor, direction);
      break;
    case 7:
      val = volumetricStrain(tensor);
      break;
    case 8:
      val = firstInvariant(tensor);
      break;
    case 9:
      val = secondInvariant(tensor);
      break;
    case 10:
      val = thirdInvariant(tensor);
      break;
    case 11:
      val = axialStress(tensor, point1, point2, direction);
      break;
    case 12:
      val = hoopStress(tensor, point1, point2, curr_point, direction);
      break;
    case 13:
      val = radialStress(tensor, point1, point2, curr_point, direction);
      break;
    case 14:
      val = triaxialityStress(tensor);
      break;
    case 15:
      val = directionValueTensor(tensor, direction);
      break;
    default:
      mooseError("RankTwoScalarAux Error: Pass valid scalar type - " +
                 scalarOptions().getRawNames());
  }

  return val;
}

Real
component(const RankTwoTensor & r2tensor, unsigned int i, unsigned int j)
{
  return r2tensor(i, j);
}

Real
component(const RankTwoTensor & r2tensor, unsigned int i, unsigned int j, Point & direction)
{
  direction.zero();
  if (i == j)
    direction(i) = 1.0;
  else
  {
    direction(i) = std::sqrt(0.5);
    direction(j) = std::sqrt(0.5);
  }

  return r2tensor(i, j);
}

Real
vonMisesStress(const RankTwoTensor & stress)
{
  RankTwoTensor dev_stress = stress.deviatoric();

  return std::sqrt(3.0 / 2.0 * dev_stress.doubleContraction(dev_stress));
}

Real
effectiveStrain(const RankTwoTensor & strain)
{
  return std::sqrt(2.0 / 3.0 * strain.doubleContraction(strain));
}

Real
hydrostatic(const RankTwoTensor & r2tensor)
{
  return r2tensor.trace() / 3.0;
}

Real
L2norm(const RankTwoTensor & r2tensor)
{
  return r2tensor.L2norm();
}

Real
volumetricStrain(const RankTwoTensor & strain)
{
  // Since the strains are logarithmic strains, which are by definition log(L/L0),
  // exp(log_strain) = L/L0
  // The ratio of the volume change of a strained cube to the original volume
  // (delta V / V) is thus:
  // exp(log_strain_11) * exp(log_strain_22) * exp(log_strain_33) - 1
  //
  // Since eng_strain = exp(log_strain) - 1, the equivalent calculation using
  // engineering strains would be:
  // (1 + eng_strain_11) * (1 + eng_strain_22) + (1 + eng_strain_33) - 1
  // If strains are small, the resulting terms that involve squared and cubed
  // strains are negligible, resulting in the following approximate form:
  // strain_11 + strain_22 + strain_33
  // There is not currently an option to compute this small-strain form of the
  // volumetric strain, but at small strains, it is approximately equal to the
  // finite strain form that is computed.
  //
  return std::exp(strain(0, 0)) * std::exp(strain(1, 1)) * std::exp(strain(2, 2)) - 1.0;
}

Real
firstInvariant(const RankTwoTensor & r2tensor)
{
  return r2tensor.trace();
}

Real
secondInvariant(const RankTwoTensor & r2tensor)
{
  Real val = 0.0;
  for (unsigned int i = 0; i < 2; ++i)
    for (unsigned int j = i + 1; j < 3; ++j)
    {
      val += r2tensor(i, i) * r2tensor(j, j);
      val -= (r2tensor(i, j) * r2tensor(i, j) + r2tensor(j, i) * r2tensor(j, i)) * 0.5;
    }

  return val;
}

Real
thirdInvariant(const RankTwoTensor & r2tensor)
{
  Real val = 0.0;
  val = r2tensor(0, 0) * r2tensor(1, 1) * r2tensor(2, 2) -
        r2tensor(0, 0) * r2tensor(1, 2) * r2tensor(2, 1) +
        r2tensor(0, 1) * r2tensor(1, 2) * r2tensor(2, 0) -
        r2tensor(0, 1) * r2tensor(1, 0) * r2tensor(2, 2) +
        r2tensor(0, 2) * r2tensor(1, 0) * r2tensor(2, 1) -
        r2tensor(0, 2) * r2tensor(1, 1) * r2tensor(2, 0);

  return val;
}

Real
maxPrincipal(const RankTwoTensor & r2tensor, Point & direction)
{
  return calcEigenValuesEigenVectors(r2tensor, (LIBMESH_DIM - 1), direction);
}

Real
midPrincipal(const RankTwoTensor & r2tensor, Point & direction)
{
  return calcEigenValuesEigenVectors(r2tensor, 1, direction);
}

Real
minPrincipal(const RankTwoTensor & r2tensor, Point & direction)
{
  return calcEigenValuesEigenVectors(r2tensor, 0, direction);
}

Real
calcEigenValuesEigenVectors(const RankTwoTensor & r2tensor, unsigned int index, Point & eigenvec)
{
  std::vector<Real> eigenval(LIBMESH_DIM);
  RankTwoTensor eigvecs;
  r2tensor.symmetricEigenvaluesEigenvectors(eigenval, eigvecs);

  Real val = eigenval[index];
  eigenvec = eigvecs.column(index);

  return val;
}

Real
axialStress(const RankTwoTensor & stress,
            const Point & point1,
            const Point & point2,
            Point & direction)
{
  Point axis = point2 - point1;
  axis /= axis.norm();

  // Calculate the stress in the direction of the axis specifed by the user
  Real axial_stress = 0.0;
  for (unsigned int i = 0; i < 3; ++i)
    for (unsigned int j = 0; j < 3; ++j)
      axial_stress += axis(j) * stress(j, i) * axis(i);

  direction = axis;

  return axial_stress;
}

Real
hoopStress(const RankTwoTensor & stress,
           const Point & point1,
           const Point & point2,
           const Point & curr_point,
           Point & direction)
{
  // Calculate the cross of the normal to the axis of rotation from the current
  // location and the axis of rotation
  Point xp;
  normalPositionVector(point1, point2, curr_point, xp);
  Point axis_rotation = point2 - point1;
  Point yp = axis_rotation / axis_rotation.norm();
  Point zp = xp.cross(yp);

  // Calculate the scalar value of the hoop stress
  Real hoop_stress = 0.0;
  for (unsigned int i = 0; i < 3; ++i)
    for (unsigned int j = 0; j < 3; ++j)
      hoop_stress += zp(j) * stress(j, i) * zp(i);

  direction = zp;

  return hoop_stress;
}

Real
radialStress(const RankTwoTensor & stress,
             const Point & point1,
             const Point & point2,
             const Point & curr_point,
             Point & direction)
{
  Point radial_norm;
  normalPositionVector(point1, point2, curr_point, radial_norm);

  // Compute the scalar stress component in the direction of the normal vector from the
  // user-defined axis of rotation.
  Real radial_stress = 0.0;
  for (unsigned int i = 0; i < 3; ++i)
    for (unsigned int j = 0; j < 3; ++j)
      radial_stress += radial_norm(j) * stress(j, i) * radial_norm(i);

  direction = radial_norm;

  return radial_stress;
}

void
normalPositionVector(const Point & point1,
                     const Point & point2,
                     const Point & curr_point,
                     Point & normalPosition)
{
  // Find the nearest point on the axis of rotation (defined by point2 - point1)
  // to the current position, e.g. the normal to the axis of rotation at the
  // current position
  Point axis_rotation = point2 - point1;
  Point positionWRTpoint1 = point1 - curr_point;
  Real projection = (axis_rotation * positionWRTpoint1) / axis_rotation.norm_sq();
  Point normal = point1 - projection * axis_rotation;

  // Calculate the direction normal to the plane formed by the axis of rotation
  // and the normal to the axis of rotation from the current position.
  normalPosition = curr_point - normal;
  normalPosition /= normalPosition.norm();
}

Real
directionValueTensor(const RankTwoTensor & r2tensor, Point & direction)
{
  Real tensor_value_in_direction = 0.0;
  for (unsigned int i = 0; i < 3; ++i)
    for (unsigned int j = 0; j < 3; ++j)
      tensor_value_in_direction += direction(j) * r2tensor(j, i) * direction(i);

  return tensor_value_in_direction;
}

Real
triaxialityStress(const RankTwoTensor & stress)
{
  return hydrostatic(stress) / vonMisesStress(stress);
}
}
