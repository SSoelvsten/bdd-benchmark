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
		for benchmark in 'apply' 'knights_tour' 'picotrav' 'qbf' 'queens' 'tic_tac_toe' ; do \
			make ${MAKE_FLAGS} $$package'_'$$benchmark'_bdd' ; \
		done ; \
	done

  # Build all zdd benchmarks
	@echo -e "\n\nBuild ZDD Benchmarks"
	@cd build/ && for package in 'adiar' 'cudd' ; do \
		mkdir -p ../out/$$package ; \
		mkdir -p ../out/$$package/zdd ; \
		for benchmark in 'knights_tour' 'picotrav' 'queens' 'tic_tac_toe' ; do \
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
	@echo "run/apply/[bdd]"
	@echo "   Load two BDDs and combine them with the product construction."
	@echo ""
	@echo "   + F1=<file_path> (default: benchmarks/...)"
	@echo "   | File path (relative to this makefile) to a *binary.bdd* file"
	@echo ""
	@echo "   + F2=<file_path> (default: benchmarks/...)"
	@echo "   | File path (relative to this makefile) to a *binary.bdd* file"
	@echo ""
	@echo "   + O=[and, or, xor] (default: and)"
	@echo "   | The Boolean operand to use."

	@echo ""
	@echo "run/knights_tour/[bdd,zdd]"
	@echo "   Counts the number of Knight's Tours on a chessboard."
	@echo ""
	@echo "   + N=<int> (default: 6)"
	@echo "   | The the width and height of the chessboard."
	@echo ""
	@echo "   + NR=<int> NC=<int> (default: N)"
	@echo "   | Alternative to using N to specify a non-square chessboards."
	@echo ""
	@echo "   + O=[time, binary, crt_binary, unary, crt_unary] (default: time)"
	@echo "   | The type of encoding to use to solve the problem."

	@echo ""
	@echo "run/picotrav/[bdd,zdd]"
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
	@echo "   + O=[input, df, level, level_df, random]"
	@echo "   | Variable order to be precomputed and used throughout computation."

	@echo ""
	@echo "run/qbf/[bdd]"
	@echo "   Solves a Quantified Boolean Formula by building the BDD corresponding to a"
	@echo "   given a circuit in the '.qcir' format and finally quantifying the prenex."
	@echo ""
	@echo "   + F=<file_path> (default: benchmarks/not_a.blif)"
	@echo "   | File path (relative to this makefile) for a '*.blif' file"
	@echo ""
	@echo "   + O=[input, df, df_rtl, level, level_df]"
	@echo "   | Variable order to be precomputed and used throughout computation."

	@echo ""
	@echo "run/queens/[bdd,zdd]"
	@echo "   Solves the N-Queens problem"
	@echo ""
	@echo "   + N=<int> (default: 8)"
	@echo "   | The size of the chessboard."

	@echo ""
	@echo "run/tic_tac_toe/[bdd,zdd]"
	@echo "   Counts the number of draws of a Tic-Tac-Toe in a 4x4x4 cube."
	@echo ""
	@echo "   + N=<int> (default: 20)"
	@echo "   | Number of crosses placed, i.e. count in a cube with (64-N) noughts."

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

# Benchmark specific Option
O := ""

# ============================================================================ #
#  RUN: Picotrav
# ============================================================================ #
run/apply:
	$(MAKE) run/apply/bdd

run/apply/bdd: O := "AND"
run/apply/bdd: F1 := "benchmarks/apply/x0.binary.bdd"
run/apply/bdd: F2 := "benchmarks/apply/x1.binary.bdd"
run/apply/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_apply_bdd -f $(F1) -f $(F2) -M $(M) -o $(O) 2>&1 | tee -a out/VARIANT/bdd/apply.out)

# ============================================================================ #
#  RUN: Knight's Tour
# ============================================================================ #
run/knights_tour:
	$(MAKE) run/knights_tour/zdd

run/knights_tour/bdd: N  := 6
run/knights_tour/bdd: NR := $(N)
run/knights_tour/bdd: NC := $(NR)
run/knights_tour/bdd: O := "TIME"
run/knights_tour/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_knights_tour_bdd -N $(NR) -N $(NC) -M $(M) -o $(O) 2>&1 | tee -a out/VARIANT/bdd/knights_tour.out)

run/knights_tour/zdd: N  := 6
run/knights_tour/zdd: NR := $(N)
run/knights_tour/zdd: NC := $(NR)
run/knights_tour/zdd: O := "TIME"
run/knights_tour/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_knights_tour_zdd -N $(NR) -N $(NC) -M $(M) -o $(O) 2>&1 | tee -a out/VARIANT/zdd/knights_tour.out)

# ============================================================================ #
#  RUN: Picotrav
# ============================================================================ #
run/picotrav:
	$(MAKE) run/picotrav/bdd

run/picotrav/bdd: O := "INPUT"
run/picotrav/bdd: F1 := "benchmarks/picotrav/not_a.blif"
run/picotrav/bdd: F2 := "benchmarks/picotrav/not_b.blif"
run/picotrav/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_picotrav_bdd -f $(F1) -f $(F2) -M $(M) -o $(O) | tee -a out/VARIANT/bdd/picotrav.out)

run/picotrav/zdd: O := "INPUT"
run/picotrav/zdd: F1 := "benchmarks/picotrav/not_a.blif"
run/picotrav/zdd: F2 := "benchmarks/picotrav/not_b.blif"
run/picotrav/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_picotrav_zdd -f $(F1) -f $(F2) -M $(M) -o $(O) | tee -a out/VARIANT/zdd/picotrav.out)

# ============================================================================ #
#  RUN: QCIR QBF solver
# ============================================================================ #
run/qbf:
	$(MAKE) run/qbf/bdd

run/qbf/bdd: O := "INPUT"
run/qbf/bdd: F := "benchmarks/qcir/example_a.qcir"
run/qbf/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_qbf_bdd -f $(F) -M $(M) -o $(O) | tee -a out/VARIANT/bdd/qbf.out)

# TODO: run/qbf/zdd

# ============================================================================ #
#  RUN: Queens
# ============================================================================ #
run/queens:
	$(MAKE) run/queens/bdd

run/queens/bdd: N := 8
run/queens/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens_bdd -N $(N) -M $(M) 2>&1 | tee -a out/VARIANT/bdd/queens.out)

run/queens/zdd: N := 8
run/queens/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens_zdd -N $(N) -M $(M) 2>&1 | tee -a out/VARIANT/zdd/queens.out)

# ============================================================================ #
#  RUN: 4x4 Tic Tac Toe
# ============================================================================ #
run/tic_tac_toe:
	$(MAKE) run/tic_tac_toe/bdd

run/tic_tac_toe/bdd: N := 20
run/tic_tac_toe/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_tic_tac_toe_bdd -N $(N) -M $(M) 2>&1 | tee -a out/VARIANT/bdd/tic_tac_toe.out)

run/tic_tac_toe/zdd: N := 20
run/tic_tac_toe/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_tic_tac_toe_zdd -N $(N) -M $(M) 2>&1 | tee -a out/VARIANT/zdd/tic_tac_toe.out)
