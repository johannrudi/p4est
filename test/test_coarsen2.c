/*
  This file is part of p4est.
  p4est is a C library to manage a parallel collection of quadtrees and/or
  octrees.

  Copyright (C) 2008 Carsten Burstedde, Lucas Wilcox.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef P4_TO_P8
#include <p4est.h>
#else
#include <p8est.h>
#endif

#ifndef P4_TO_P8
static const int    refine_level = 6;
#else
static const int    refine_level = 4;
#endif
static bool         coarsen_all = true;

static int
refine_fn (p4est_t * p4est, p4est_topidx_t which_tree,
           p4est_quadrant_t * quadrant)
{
  if ((int) quadrant->level >= (refine_level - (int) (which_tree % 3))) {
    return 0;
  }

  return 1;
}

static int
coarsen_fn (p4est_t * p4est, p4est_topidx_t which_tree,
            p4est_quadrant_t * q0, p4est_quadrant_t * q1,
            p4est_quadrant_t * q2, p4est_quadrant_t * q3
#ifdef P4_TO_P8
            ,
            p4est_quadrant_t * q4, p4est_quadrant_t * q5,
            p4est_quadrant_t * q6, p4est_quadrant_t * q7
#endif
  )
{
  return coarsen_all || q0->y >= P4EST_ROOT_LEN / 2;
}

int
main (int argc, char **argv)
{
  int                 mpirank, mpisize;
  int                 mpiret;
  MPI_Comm            mpicomm;
  p4est_t            *p4est;
  p4est_connectivity_t *connectivity;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = MPI_COMM_WORLD;
  mpiret = MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpirank, NULL, NULL, NULL, SC_LP_DEFAULT);
  p4est_init (NULL, SC_LP_DEFAULT);

  /* create connectivity and forest structures */
#ifdef P4_TO_P8
  connectivity = p8est_connectivity_new_twocubes ();
#else
  connectivity = p4est_connectivity_new_corner ();
#endif
  p4est = p4est_new (mpicomm, connectivity, 0, NULL);
  p4est_refine (p4est, refine_fn, NULL);

  p4est_coarsen_recursive = false;
  coarsen_all = true;
  p4est_coarsen (p4est, coarsen_fn, NULL);
  p4est_coarsen_recursive = true;
  coarsen_all = false;
  p4est_coarsen (p4est, coarsen_fn, NULL);
  p4est_balance (p4est, NULL);
  coarsen_all = true;
  p4est_coarsen (p4est, coarsen_fn, NULL);

  if (mpisize == 1) {
    SC_CHECK_ABORT (p4est->global_num_quadrants ==
                    (p4est_gloidx_t) connectivity->num_trees, "Coarsen");
  }

  p4est_destroy (p4est);
  p4est_connectivity_destroy (connectivity);
  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}

/* EOF test_coarsen2.c */