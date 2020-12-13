.PHONY: clean build test

MAKE_FLAGS=-j $$(nproc)

# ============================================================================ #
#  VARIABLE DEFAULTS
# ============================================================================ #

# Variant (adiar, buddy, sylvan)
V:=buddy

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

build-queens: | build
	@cd build/ && make ${MAKE_FLAGS} ${V}_queens

build-sat_pigeonhole_principle: | build
	@cd build/ && make ${MAKE_FLAGS} ${V}_sat_pigeonhole_principle

build-sat_queens: | build
	@cd build/ && make ${MAKE_FLAGS} ${V}_sat_queens

build-tic_tac_toe: | build
	@cd build/ && make ${MAKE_FLAGS} ${V}_tic_tac_toe


clean:
	@rm -rf build/


# ============================================================================ #
#  RUN TARGETS
# ============================================================================ #
run-queens: N := 8
run-queens:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens) $(N) $(M)

run-sat_pigeonhole_principle: N := 10
run-sat_pigeonhole_principle:
	@$(subst VARIANT,$(V),./build/src/VARIANT_sat_pigeonhole_principle) $(N) $(M)

run-sat_queens: N := 8
run-sat_queens:
	@$(subst VARIANT,$(V),./build/src/VARIANT_sat_queens) $(N) $(M)

run-tic_tac_toe: N := 20
run-tic_tac_toe:
	@$(subst VARIANT,$(V),./build/src/VARIANT_tic_tac_toe) $(N) $(M)

# ============================================================================ #
#  ALL-IN-ONE TARGETS
# ============================================================================ #
queens: build-queens run-queens

sat_queens: build-sat_queens run-sat_queens
sat_pigeonhole_principle: build-sat_pigeonhole_principle run-sat_pigeonhole_principle

tic_tac_toe: build-tic_tac_toe run-tic_tac_toe
