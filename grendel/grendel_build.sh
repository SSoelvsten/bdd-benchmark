#!/bin/bash
#SBATCH --job-name=soelvsten_build
#SBATCH --partition=q48
#SBATCH --time=00:10:00

# Load packages
module load cmake gcc gpm boost gmp
export CC=/comm/swstack/core/gcc/10.1.0/bin/gcc
export CXX=/comm/swstack/core/gcc/10.1.0/bin/c++

# Build
mkdir -p $SLURM_SUBMIT_DIR/build/ && cd $SLURM_SUBMIT_DIR/build/
cmake -D GRENDEL=ON $SLURM_SUBMIT_DIR

cd $SLURM_SUBMIT_DIR/build/sylvan
make DESTDIR=./ && make install DESTDIR=./

cd $SLURM_SUBMIT_DIR/build/src/
make buddy_queens coom_queens sylvan_queens
make buddy_sat_pigeonhole_principle coom_sat_pigeonhole_principle sylvan_sat_pigeonhole_principle
make buddy_sat_queens coom_sat_queens sylvan_sat_queens
make buddy_tic_tac_toe coom_tic_tac_toe sylvan_tic_tac_toe

