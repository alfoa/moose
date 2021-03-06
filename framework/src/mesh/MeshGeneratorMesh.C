//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "MeshGeneratorMesh.h"

#include "libmesh/face_quad4.h"
#include "libmesh/face_tri3.h"

registerMooseObject("MooseApp", MeshGeneratorMesh);

template <>
InputParameters
validParams<MeshGeneratorMesh>()
{
  InputParameters params = validParams<MooseMesh>();
  return params;
}

MeshGeneratorMesh::MeshGeneratorMesh(const InputParameters & parameters) : MooseMesh(parameters) {}

std::unique_ptr<MooseMesh>
MeshGeneratorMesh::safeClone() const
{
  return libmesh_make_unique<MeshGeneratorMesh>(*this);
}

void
MeshGeneratorMesh::buildMesh()
{
  _mesh = _app.getMeshGeneratorMesh();
}
