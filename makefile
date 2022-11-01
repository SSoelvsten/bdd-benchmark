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
BUILD_TYPE := Release
STATS := OFF

build:
  # Primary build
	@echo "\nBuild"
	@mkdir -p build/ && cd build/ && cmake -D CMAKE_BUILD_TYPE=$(BUILD_TYPE) \
                                         -D ADIAR_STATS_EXTRA=$(STATS) \
                                         -D SYLVAN_STATS=$(STATS) \
                                   ..

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
		mkdir -p ../out/$$package/bdd ; \
		for benchmark in 'picotrav' 'queens' 'tic_tac_toe' ; do \
			make ${MAKE_FLAGS} $$package'_'$$benchmark'_bdd' ; \
		done ; \
	done

  # Build all zdd benchmarks
	@echo "\n\nBuild ZDD Benchmarks"
	@cd build/ && for package in 'adiar' 'cudd' ; do \
		mkdir -p ../out/$$package ; \
		mkdir -p ../out/$$package/zdd ; \
		for benchmark in 'knights_tour' 'queens' 'tic_tac_toe' ; do \
			make ${MAKE_FLAGS} $$package'_'$$benchmark'_zdd' ; \
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

combinatorial/knights_tour/zdd: N := 10
combinatorial/knights_tour/zdd: O := "OPEN"
combinatorial/knights_tour/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_knights_tour_zdd -N $(N) -M $(M) -o $(O) | tee -a out/VARIANT/zdd/knights_tour.out)

combinatorial/queens:
	$(MAKE) combinatorial/queens/bdd

combinatorial/queens/bdd: N := 8
combinatorial/queens/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens_bdd -N $(N) -M $(M) | tee -a out/VARIANT/bdd/queens.out)

combinatorial/queens/zdd: N := 8
combinatorial/queens/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens_zdd -N $(N) -M $(M) | tee -a out/VARIANT/zdd/queens.out)

combinatorial/tic_tac_toe:
	$(MAKE) combinatorial/tic_tac_toe/bdd

combinatorial/tic_tac_toe/bdd: N := 20
combinatorial/tic_tac_toe/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_tic_tac_toe_bdd -N $(N) -M $(M) | tee -a out/VARIANT/bdd/tic_tac_toe.out)

combinatorial/tic_tac_toe/zdd: N := 20
combinatorial/tic_tac_toe/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_tic_tac_toe_zdd -N $(N) -M $(M) | tee -a out/VARIANT/zdd/tic_tac_toe.out)

# ============================================================================ #
#  VERIFICATION BENCHMARKS
# ============================================================================ #
F1 := "benchmarks/not_a.blif"
F2 := "benchmarks/not_b.blif"

verification/picotrav:
	$(MAKE) verification/picotrav/bdd

verification/picotrav/bdd: O := "INPUT"
verification/picotrav/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_picotrav_bdd -f $(F1) -f $(F2) -M $(M) -o $(O) | tee -a out/VARIANT/bdd/picotrav.out)
