//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "LineValueSampler.h"
#include <limits>
#include "libmesh/utility.h"

registerMooseObject("MooseApp", LineValueSampler);

template <>
InputParameters
validParams<LineValueSampler>()
{
  InputParameters params = validParams<PointSamplerBase>();

  params.addRequiredParam<Point>("start_point", "The beginning of the line");
  params.addRequiredParam<Point>("end_point", "The ending of the line");

  params.addRequiredParam<unsigned int>("num_points",
                                        "The number of points to sample along the line");

  return params;
}

LineValueSampler::LineValueSampler(const InputParameters & parameters)
  : PointSamplerBase(parameters),
    _line_vector(getParam<Point>("end_point") - getParam<Point>("start_point")),
    _line_vector_norm(_line_vector.norm())
{
  Point start_point = getParam<Point>("start_point");
  Point end_point = getParam<Point>("end_point");

  if (MooseUtils::absoluteFuzzyEqual(_line_vector_norm, 0.0))
    mooseError("LineValueSampler: `start_point` and `end_point` must be different.");

  unsigned int num_points = getParam<unsigned int>("num_points");

  generatePointsAndIDs(start_point, end_point, num_points, _points, _ids);
}

void
LineValueSampler::generatePointsAndIDs(const Point & start_point,
                                       const Point & end_point,
                                       unsigned int num_points,
                                       std::vector<Point> & points,
                                       std::vector<Real> & ids)
{

  Point difference = end_point - start_point;

  Point delta = difference / Real(num_points - 1);

  points.resize(num_points);
  ids.resize(num_points);

  for (unsigned int i = 0; i < num_points - 1;
       i++) // -1 so that we can manually put in the end point to get it perfect
  {
    Point p = start_point + (i * delta);

    points[i] = p;
    ids[i] = (p - start_point).norm(); // The ID is the distance along the line
  }

  // Add the end point explicitly
  points[num_points - 1] = end_point;
  ids[num_points - 1] = (end_point - start_point).norm();
}

Real
LineValueSampler::getValue(Point p) const
{
  if (_values.size() != 1)
    mooseError("LineValueSampler: When calling getValue() on LineValueSampler, "
               "only one variable can be provided as input to LineValueSampler.");

  // Check if vectors are sorted by id
  if (_sort_by != 3)
    mooseError("LineValueSampler: When calling getValue() on LineValueSampler, "
               "`sort_by` should be set to `id`.");

  Real value = std::numeric_limits<Real>::infinity();

  // Project point onto the line segment and normalize by length of line segment
  Real position =
      (p - _points[0]) * (_points.back() - _points[0]) / Utility::pow<2>(_line_vector_norm);

  if (position >= 0.0 and position <= 1.0)
  {
    unsigned int vec_pos =
        std::lower_bound(_id.begin(), _id.end(), position * _line_vector_norm) - _id.begin();

    VectorPostprocessorValue & value_vector =
        _vpp_fe_problem->getVectorPostprocessorValue(_vpp_name, _variable_names[0], false);

    if (MooseUtils::absoluteFuzzyEqual(_id[vec_pos], position * _line_vector_norm))
      value = value_vector[vec_pos];
    else
      value = (value_vector[vec_pos - 1] + value_vector[vec_pos]) * 0.5;
  }

  return value;
}
