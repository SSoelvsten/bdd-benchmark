.PHONY: build clean help

MAKE_FLAGS=-j $$(nproc)

# ============================================================================ #
#  BUILD TARGETS
# ============================================================================ #
BUILD_TYPE := Release
STATS := OFF

build:
  # Primary build
	@echo -e "\nBuild"
	@mkdir -p build/ && cd build/ && cmake -D CMAKE_BUILD_TYPE=$(BUILD_TYPE) \
                                         -D BDD_BENCHMARK_STATS=$(STATS) \
																				 -D CMAKE_EXPORT_COMPILE_COMMANDS=1 \
                                   ..

  # Installation of CUDD
	@echo -e "\n\nInstall CUDD"
	@[ -d "build/cudd/" ] || (cd external/cudd \
														&& autoreconf \
														&& ./configure --prefix ${CURDIR}/build/cudd/ --enable-obj \
														&& make MAKEINFO=true \
														&& make install)

  # Make out folder
	@mkdir -p out/

  # Build all bdd benchmarks
	@echo -e "\n\nBuild BDD Benchmarks"
	@cd build/ && for package in 'adiar' 'buddy' 'cal' 'cudd' 'sylvan' ; do \
		mkdir -p ../out/$$package ; \
		mkdir -p ../out/$$package/bdd ; \
		for benchmark in 'picotrav' 'qbf' 'queens' 'tic_tac_toe' 'cnf' ; do \
			make ${MAKE_FLAGS} $$package'_'$$benchmark'_bdd' ; \
		done ; \
	done

  # Build all zdd benchmarks
	@echo -e "\n\nBuild ZDD Benchmarks"
	@cd build/ && for package in 'adiar' 'cudd' ; do \
		mkdir -p ../out/$$package ; \
		mkdir -p ../out/$$package/zdd ; \
		for benchmark in 'picotrav' 'knights_tour' 'queens' 'tic_tac_toe' ; do \
			make ${MAKE_FLAGS} $$package'_'$$benchmark'_zdd' ; \
		done ; \
	done

  # Add space after finishing the build process
	@echo -e "\n"

clean:
	@cd external/cudd && git clean -qdf && git checkout .
	@rm -rf build/
	@rm -rf out/

clean/out:
	@rm -f out/**/*.out

# ============================================================================ #
#  HELP
# ============================================================================ #
help:
	@echo ""
	@echo "BDD Benchmarks"
	@echo "--------------"
	@echo "A collection of benchmarks to experimentally compare BDD packages."
	@echo "================================================================================"

	@echo ""
	@echo "Compilation"
	@echo "--------------"

	@echo ""
	@echo "build:"
	@echo "   Build all BDD packages and all benchmarks"
	@echo ""
	@echo "   + BUILD_TYPE=[Release, Debug, RelWithDebInfo, MinSizeRel] (default: Release)"
	@echo "   | Type of build, i.e. the compiler settings to use."
	@echo ""
	@echo "   + STATS=[ON,OFF] (default: OFF)"
	@echo "   | Whether to build all BDD packages with their statistics turned on. If 'ON'"
	@echo "   | then time measurements are unusable."

	@echo ""
	@echo "clean:"
	@echo "   Remove all build artifacts and the 'out/' folder."

	@echo ""
	@echo "clean/out:"
	@echo "   Remove *.out files in the 'out/' folder."

	@echo ""
	@echo "--------------------------------------------------------------------------------"

	@echo ""
	@echo "Benchmarks"
	@echo "--------------"

	@echo "   All benchmarks below can be provided with the following two arguments."
	@echo ""
	@echo "   + V=[adiar, buddy, cal, cudd, sylvan] (default: adiar)"
	@echo "   | BDD package to use."
	@echo ""
	@echo "   + M=<int> (default: 128)"
	@echo "   | Memory (MiB) to dedicate to the BDD package."

	@echo ""
	@echo "combinatorial/knights_tour/[zdd]"
	@echo "   Counts the number of Knight's Tours on a chessboard."
	@echo ""
	@echo "   + N=<int> (default: 10)"
	@echo "   | The sum of the width and height of the chessboard."
	@echo ""
	@echo "   + O=[OPEN,CLOSED] (default: OPEN)"
	@echo "   | Whether to count the hamiltonian paths (open) or only cycles (closed)."

	@echo ""
	@echo "combinatorial/queens/[bdd,zdd]"
	@echo "   Solves the N-Queens problem"
	@echo ""
	@echo "   + N=<int> (default: 8)"
	@echo "   | The size of the chessboard."

	@echo ""
	@echo "combinatorial/tic_tac_toe/[bdd,zdd]"
	@echo "   Counts the number of draws of a Tic-Tac-Toe in a 4x4x4 cube."
	@echo ""
	@echo "   + N=<int> (default: 20)"
	@echo "   | Number of crosses placed, i.e. count in a cube with (64-N) noughts."

	@echo ""
	@echo "verification/picotrav/[bdd,zdd]"
	@echo "   Build BDDs that describe the output gates of circuits in a '.blif' format to"
	@echo "   then verify whether two circuits are functionally equivalent."
	@echo ""
	@echo "   + F1=<file_path> (default: benchmarks/not_a.blif)"
	@echo "   | File path (relative to this makefile) for a '*.blif' file"
	@echo ""
	@echo "   + F2=<file_path>"
	@echo "   | File path (relative to this makefile) for a second '*.blif' file to compare"
	@echo "   | to the first."
	@echo ""
	@echo "   + O=[INPUT, DFS, LEVEL, LEVEL_DFS, RANDOM]"
	@echo "   | Variable order to precompute and use throughout computation."

	@echo ""
	@echo "verification/qbf/[bdd]"

	@echo ""
	@echo "--------------------------------------------------------------------------------"
	@echo ""
	@echo "Other"
	@echo "--------------"
	@echo ""
	@echo "help:"
	@echo "   Prints this hopefully helpful piece of text."


# ============================================================================ #
#  RUN:
# ============================================================================ #

# Variant (adiar, buddy, cal, cudd, sylvan)
V:=adiar

# Memory
M:=128

# Instance size variable (custom default for every target below)
N:=0

# ============================================================================ #
#  RUN: COMBINATORIAL PROBLEMS
# ============================================================================ #
O := ""

combinatorial/knights_tour:
	$(MAKE) combinatorial/knights_tour/zdd

combinatorial/knights_tour/zdd: N := 10
combinatorial/knights_tour/zdd: O := "OPEN"
combinatorial/knights_tour/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_knights_tour_zdd -N $(N) -M $(M) -o $(O) 2>&1 | tee -a out/VARIANT/zdd/knights_tour.out)

combinatorial/queens:
	$(MAKE) combinatorial/queens/bdd

combinatorial/queens/bdd: N := 8
combinatorial/queens/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens_bdd -N $(N) -M $(M) 2>&1 | tee -a out/VARIANT/bdd/queens.out)

combinatorial/queens/zdd: N := 8
combinatorial/queens/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens_zdd -N $(N) -M $(M) 2>&1 | tee -a out/VARIANT/zdd/queens.out)

combinatorial/tic_tac_toe:
	$(MAKE) combinatorial/tic_tac_toe/bdd

combinatorial/tic_tac_toe/bdd: N := 20
combinatorial/tic_tac_toe/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_tic_tac_toe_bdd -N $(N) -M $(M) 2>&1 | tee -a out/VARIANT/bdd/tic_tac_toe.out)

combinatorial/tic_tac_toe/zdd: N := 20
combinatorial/tic_tac_toe/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_tic_tac_toe_zdd -N $(N) -M $(M) 2>&1 | tee -a out/VARIANT/zdd/tic_tac_toe.out)

# ============================================================================ #
#  RUN: VERIFICATION BENCHMARKS
# ============================================================================ #
verification/picotrav:
	$(MAKE) verification/picotrav/bdd

verification/picotrav/bdd: O := "INPUT"
verification/picotrav/bdd: F1 := "benchmarks/picotrav/not_a.blif"
verification/picotrav/bdd: F2 := "benchmarks/picotrav/not_b.blif"
verification/picotrav/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_picotrav_bdd -f $(F1) -f $(F2) -M $(M) -o $(O) | tee -a out/VARIANT/bdd/picotrav.out)

verification/picotrav/zdd: O := "INPUT"
verification/picotrav/zdd: F1 := "benchmarks/picotrav/not_a.blif"
verification/picotrav/zdd: F2 := "benchmarks/picotrav/not_b.blif"
verification/picotrav/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_picotrav_zdd -f $(F1) -f $(F2) -M $(M) -o $(O) | tee -a out/VARIANT/zdd/picotrav.out)

verification/qbf:
	$(MAKE) verification/qbf/bdd

verification/qbf/bdd: O := "INPUT"
verification/qbf/bdd: F := "benchmarks/qcir/example_a.qcir"
verification/qbf/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_qbf_bdd -f $(F) -M $(M) -o $(O) | tee -a out/VARIANT/bdd/qbf.out)
