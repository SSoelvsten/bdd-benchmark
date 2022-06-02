.PHONY: clean build test grendel

MAKE_FLAGS=-j $$(nproc)

# ============================================================================ #
#  VARIABLE DEFAULTS
# ============================================================================ #

# Variant (adiar, buddy, cudd, sylvan)
V:=adiar

# Memory
M:=128

# Instance size variable (custom default for every target below)
N:=0

# ============================================================================ #
#  BUILD TARGETS
# ============================================================================ #
build:
  # Primary build
	@echo "\nBuild"
	@mkdir -p build/ && cd build/ && cmake ..

  # Installation of CUDD
	@echo "\n\nInstall CUDD"
	@[ -d "build/cudd/" ] || (cd external/cudd \
														&& autoreconf \
														&& ./configure --prefix ${CURDIR}/build/cudd/ --enable-obj \
														&& make MAKEINFO=true \
														&& make install)

  # Make out folder
	@mkdir -p out/

  # Build all bdd benchmarks
	@echo "\n\nBuild BDD Benchmarks"
	@cd build/ && for package in 'adiar' 'buddy' 'sylvan' 'cudd' ; do \
		mkdir -p ../out/$$package ; \
		for benchmark in 'picotrav' 'queens' 'sat_pigeonhole_principle' 'sat_queens' 'tic_tac_toe' ; do \
			make ${MAKE_FLAGS} $$package'_'$$benchmark ; \
		done ; \
	done

  # Build all zdd benchmarks
	@echo "\n\nBuild ZDD Benchmarks"
	@cd build/ && for package in 'adiar' ; do \
		mkdir -p ../out/$$package ; \
		for benchmark in 'queens_zdd' 'knights_tour_zdd' ; do \
			make ${MAKE_FLAGS} $$package'_'$$benchmark ; \
		done ; \
	done

  # Add space after finishing the build process
	@echo "\n"

clean:
	@cd external/cudd && git clean -qdf && git checkout .
	@rm -rf build/
	@rm -rf out/

clean/out:
	@rm -f out/**/*.out

# ============================================================================ #
#  COMBINATORIAL PROBLEMS
# ============================================================================ #
O := ""

combinatorial/knights_tour:
	$(MAKE) combinatorial/knights_tour/zdd

combinatorial/knights_tour/zdd: N := 12
combinatorial/knights_tour/zdd: O="OPEN"
combinatorial/knights_tour/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_knights_tour_zdd -N $(N) -M $(M) -o $(O) | tee -a out/VARIANT/queens_zdd.out)

combinatorial/queens:
	$(MAKE) combinatorial/queens/bdd

combinatorial/queens/bdd: N := 8
combinatorial/queens/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens -N $(N) -M $(M) | tee -a out/VARIANT/queens.out)

combinatorial/queens/zdd: N := 8
combinatorial/queens/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens_zdd -N $(N) -M $(M) | tee -a out/VARIANT/queens_zdd.out)

combinatorial/tic_tac_toe:
	$(MAKE) combinatorial/queens/bdd

combinatorial/tic_tac_toe/bdd: N := 20
combinatorial/tic_tac_toe/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_tic_tac_toe -N $(N) -M $(M) | tee -a out/VARIANT/tic_tac_toe.out)

# ============================================================================ #
#  SAT SOLVER
# ============================================================================ #
sat-solver/pigeonhole_principle: N := 10
sat-solver/pigeonhole_principle:
	@$(subst VARIANT,$(V),./build/src/VARIANT_sat_pigeonhole_principle -N $(N) -M $(M) | tee -a out/VARIANT/sat_pigeonhole_principle.out)

sat-solver/queens: N := 6
sat-solver/queens:
	@$(subst VARIANT,$(V),./build/src/VARIANT_sat_queens -N $(N) -M $(M) | tee -a out/VARIANT/sat_queens.out)

# ============================================================================ #
#  VERIFICATION BENCHMARKS
# ============================================================================ #
F1 := ""
F2 := ""

verification/picotrav: O := "INPUT"
verification/picotrav:
	@$(subst VARIANT,$(V),./build/src/VARIANT_picotrav -f $(F1) -f $(F2) -M $(M) -o $(O) | tee -a out/VARIANT/picotrav.out)
