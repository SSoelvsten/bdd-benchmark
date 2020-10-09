.PHONY: clean build test

MAKE_FLAGS=-j $$(nproc)

# ============================================================================ #
#  VARIABLE DEFAULTS
# ============================================================================ #

# Variant (sylvan, coom, buddy)
V:=sylvan

# Memory
M:=128

# Input variable
N:=0

# ============================================================================ #
#  BUILD
# ============================================================================ #
build:
	@mkdir -p build/ && cd build/ && cmake ..

build-queens: | build
	@cd build/ && make ${MAKE_FLAGS} ${V}_queens

build-tic_tac_toe: | build
	@mkdir -p build/ && cd build/ && cmake ..
	@cd build/ && make ${MAKE_FLAGS} ${V}_tic_tac_toe

clean:
	@rm -rf build/


# ============================================================================ #
#  EXAMPLES
# ============================================================================ #
queens: N := 8
queens: | build-queens
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens) $(N) $(M)

tic_tac_toe: N := 19
tic_tac_toe: | build-tic_tac_toe
	@$(subst VARIANT,$(V),./build/src/VARIANT_tic_tac_toe) $(N) $(M)

