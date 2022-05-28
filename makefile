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
	@mkdir -p build/ && cd build/ && cmake -D ADIAR_STATS=ON ..

  # Installation of CUDD
	@echo "\n\nInstall CUDD"
	@[ -d "build/cudd/" ] || (cd external/cudd \
														&& autoreconf \
														&& ./configure --prefix ${CURDIR}/build/cudd/ --enable-obj \
														&& make MAKEINFO=true \
														&& make install)

  # Build all bdd benchmarks
	@echo "\n\nBuild BDD Benchmarks"
	@cd build/ && for package in 'adiar' 'buddy' 'sylvan' 'cudd' ; do \
		for benchmark in 'picotrav' 'queens' 'sat_pigeonhole_principle' 'sat_queens' 'tic_tac_toe' ; do \
			make ${MAKE_FLAGS} $$package'_'$$benchmark ; \
		done ; \
	done

  # Build all zdd benchmarks
	@echo "\n\nBuild ZDD Benchmarks"
	@cd build/ && for package in 'adiar' ; do \
		for benchmark in 'queens_zdd' ; do \
			make ${MAKE_FLAGS} $$package'_'$$benchmark ; \
		done ; \
	done

  # Add space after finishing the build process
	@echo "\n"

clean:
	@cd external/cudd && git clean -qdf && git checkout .
	@rm -rf build/

# ============================================================================ #
#  RUN TARGETS
# ============================================================================ #
F1 := ""
F2 := ""
O := "INPUT"

combinatorial/picotrav:
combinatorial/picotrav:
	@$(subst VARIANT,$(V),./build/src/VARIANT_picotrav) -f $(F1) -f $(F2) -M $(M) -o $(O) | tee -a picotrav.out

combinatorial/queens:
	$(MAKE) combinatorial/queens/bdd

combinatorial/queens/zdd: N := 8
combinatorial/queens/zdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens_zdd) -N $(N) -M $(M) | tee -a queens_zdd.out

combinatorial/queens/bdd: N := 8
combinatorial/queens/bdd:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens) -N $(N) -M $(M) | tee -a queens_bdd.out

combinatorial/tic_tac_toe: N := 20
combinatorial/tic_tac_toe:
	@$(subst VARIANT,$(V),./build/src/VARIANT_tic_tac_toe) -N $(N) -M $(M) | tee -a tic_tac_toe.out

sat-solver/pigeonhole_principle: N := 10
sat-solver/pigeonhole_principle:
	@$(subst VARIANT,$(V),./build/src/VARIANT_sat_pigeonhole_principle) -N $(N) -M $(M) | tee -a sat__pigeon.out

sat-solver/queens: N := 6
sat-solver/queens:
	@$(subst VARIANT,$(V),./build/src/VARIANT_sat_queens) -N $(N) -M $(M) | tee -a sat__queens_bdd.out
