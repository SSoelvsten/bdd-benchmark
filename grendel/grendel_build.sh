#!/bin/bash
#SBATCH --job-name=bdd_benchmark_build
#SBATCH --partition=q48
#SBATCH --time=00:10:00

# Load packages
module load cmake autoconf automake gcc gpm
module load boost gmp
export CC=/comm/swstack/core/gcc/10.1.0/bin/gcc
export CXX=/comm/swstack/core/gcc/10.1.0/bin/c++

# Build
echo "Build"
mkdir -p $SLURM_SUBMIT_DIR/build/ && cd $SLURM_SUBMIT_DIR/build/
cmake -D GRENDEL=ON -D ADIAR_STATS=ON $SLURM_SUBMIT_DIR

echo ""
echo "Build Sylvan"
cd $SLURM_SUBMIT_DIR/build/sylvan
make DESTDIR=./ && make install DESTDIR=./

echo ""
echo "Build CUDD"
cd $SLURM_SUBMIT_DIR/external/cudd
autoreconf
./configure --prefix $SLURM_SUBMIT_DIR/build/cudd/ --enable-obj
make && make install

echo ""
echo "Build Benchmarks"
cd $SLURM_SUBMIT_DIR/build/
for package in 'adiar' 'buddy' 'sylvan' 'cudd' ; do
		for benchmark in 'picotrav' 'queens' 'sat_pigeonhole_principle' 'sat_queens' 'tic_tac_toe' ; do
			  make $package'_'$benchmark ;
		done ;
done
