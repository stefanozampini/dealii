// ---------------------------------------------------------------------
//
// Copyright (C) 2020 - 2021 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE.md at
// the top level directory of deal.II.
//
// ---------------------------------------------------------------------


// Test ReferenceCell::barycenter()

#include <deal.II/base/quadrature_lib.h>

#include <deal.II/fe/fe_pyramid_p.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_simplex_p.h>
#include <deal.II/fe/fe_simplex_p_bubbles.h>
#include <deal.II/fe/fe_system.h>
#include <deal.II/fe/fe_values.h>
#include <deal.II/fe/fe_wedge_p.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria.h>

#include "../tests.h"

using namespace dealii;

template <int dim>
void
test(const ReferenceCell &reference_cell)
{
  Triangulation<dim> triangulation;
  GridGenerator::reference_cell(triangulation, reference_cell);

  std::unique_ptr<Quadrature<dim>> q;
  if (reference_cell == ReferenceCells::Line)
    q = std::make_unique<QGauss<dim>>(2);
  else if (reference_cell == ReferenceCells::Triangle)
    q = std::make_unique<QGaussSimplex<dim>>(2);
  else if (reference_cell == ReferenceCells::Quadrilateral)
    q = std::make_unique<QGauss<dim>>(2);
  else if (reference_cell == ReferenceCells::Tetrahedron)
    q = std::make_unique<QGaussSimplex<dim>>(2);
  else if (reference_cell == ReferenceCells::Wedge)
    q = std::make_unique<QGaussWedge<dim>>(2);
  else if (reference_cell == ReferenceCells::Pyramid)
    q = std::make_unique<QGaussPyramid<dim>>(2);
  else if (reference_cell == ReferenceCells::Hexahedron)
    q = std::make_unique<QGauss<dim>>(2);

  std::unique_ptr<FiniteElement<dim>> fe;
  if (reference_cell == ReferenceCells::Line)
    fe = std::make_unique<FE_Q<dim>>(1);
  else if (reference_cell == ReferenceCells::Triangle)
    fe = std::make_unique<FE_SimplexP<dim>>(1);
  else if (reference_cell == ReferenceCells::Quadrilateral)
    fe = std::make_unique<FE_Q<dim>>(1);
  else if (reference_cell == ReferenceCells::Tetrahedron)
    fe = std::make_unique<FE_SimplexP<dim>>(1);
  else if (reference_cell == ReferenceCells::Wedge)
    fe = std::make_unique<FE_WedgeP<dim>>(1);
  else if (reference_cell == ReferenceCells::Pyramid)
    fe = std::make_unique<FE_PyramidP<dim>>(1);
  else if (reference_cell == ReferenceCells::Hexahedron)
    fe = std::make_unique<FE_Q<dim>>(1);

  // Set up the objects to compute an integral on the reference cell
  FEValues<dim> fe_values(*fe,
                          *q,
                          update_JxW_values | update_quadrature_points);
  fe_values.reinit(triangulation.begin_active());

  double     volume = 0;
  Point<dim> barycenter;
  for (unsigned int i = 0; i < q->size(); ++i)
    {
      volume += fe_values.JxW(i);
      barycenter += fe_values.quadrature_point(i) * fe_values.JxW(i);
    }
  barycenter /= volume;

  deallog << "ReferenceCell: " << reference_cell.to_string() << std::endl;
  deallog << "  computed barycenter = " << barycenter << std::endl;
  deallog << "  self-reported barycenter = " << reference_cell.barycenter<dim>()
          << std::endl;

  Assert((barycenter - reference_cell.barycenter<dim>()).norm() <= 1e-12,
         ExcInternalError());
}

int
main()
{
  initlog();

  {
    deallog.push("1D");
    test<1>(ReferenceCells::Line);
    deallog.pop();
  }

  {
    deallog.push("2D");
    test<2>(ReferenceCells::Quadrilateral);
    test<2>(ReferenceCells::Triangle);
    deallog.pop();
  }

  {
    deallog.push("3D");
    test<3>(ReferenceCells::Tetrahedron);
    test<3>(ReferenceCells::Pyramid);
    test<3>(ReferenceCells::Wedge);
    test<3>(ReferenceCells::Hexahedron);
    deallog.pop();
  }
}
