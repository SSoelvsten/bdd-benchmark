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

  # Installation of sylvan
	@echo "\n\nInstall Sylvan"
	@cd build/sylvan && make DESTDIR=./ && make install DESTDIR=./

  # Installation of CUDD
	@echo "\n\nInstall CUDD"
	@[ -d "build/cudd/" ] || (cd external/cudd \
                            && autoreconf \
                            && ./configure --prefix ${CURDIR}/build/cudd/ --enable-obj \
                            && make && make install)

  # Build all benchmarks
	@echo "\n\nBuild Benchmarks"
	@cd build/ && for package in 'adiar' 'buddy' 'sylvan' 'cudd' ; do \
		for benchmark in 'queens' 'sat_pigeonhole_principle' 'sat_queens' 'tic_tac_toe' ; do \
			make ${MAKE_FLAGS} $$package'_'$$benchmark ; \
		done ; \
	done

  # Add space after finishing the build process
	@echo "\n"

clean:
  # CUDD Autoconf files
	@[ ! -f "external/cudd/Makefile" ] || ( \
    cd external/cudd && make ${MAKE_FLAGS} clean \
                     && rm -f Doxyfile doc/cudd.tex \
                     && rm -f Makefile config.h config.log config.status dddmp/exp/text*.sh libtool stamp-h1 \
                     && rm -rf autom4te.cache/ cplusplus/.deps/ cudd/.deps/ dddmp/.deps/ epd/.deps/ mtr/.deps/ nanotrav/.deps/ st/.deps/ util/deps/)

  # CMake files
	@rm -rf build/

# ============================================================================ #
#  RUN TARGETS
# ============================================================================ #
combinatorial/queens: N := 8
combinatorial/queens:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens) -N $(N) -M $(M)

combinatorial/tic_tac_toe: N := 20
combinatorial/tic_tac_toe:
	@$(subst VARIANT,$(V),./build/src/VARIANT_tic_tac_toe) -N $(N) -M $(M)

sat-solver/pigeonhole_principle: N := 10
sat-solver/pigeonhole_principle:
	@$(subst VARIANT,$(V),./build/src/VARIANT_sat_pigeonhole_principle) -N $(N) -M $(M)

sat-solver/queens: N := 6
sat-solver/queens:
	@$(subst VARIANT,$(V),./build/src/VARIANT_sat_queens) -N $(N) -M $(M)

# ============================================================================ #
#  GRENDEL
# ============================================================================ #
grendel:
	@rm -f grendel/*_*_*.sh
	@cd grendel && python3 grendel_gen.py
	@chmod a+x grendel/*.sh
