#!/bin/bash
# Macro Error Tests
# Testes de Erro de Macro
# Each .brc file should FAIL to compile (prove error detection works)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
BRICK="$BUILD_DIR/brick"
PASS=0
FAIL=0

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m'

mkdir -p "$BUILD_DIR"

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Macro Error Tests${NC}"
echo -e "${CYAN}  Testes de Erro de Macro${NC}"
echo -e "${CYAN}========================================${NC}"

if [ ! -f "$BRICK" ]; then
    echo -e "${RED}Compiler not found. Run 'scons' first.${NC}"
    exit 1
fi

test_expect_error() {
    local name="$1"
    local brc_file="$2"
    local expected_msg="$3"
    local c_file="$BUILD_DIR/${name}.c"

    echo -ne "  ${name}... "

    output=$("$BRICK" "$brc_file" -o "$c_file" 2>&1)
    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        echo -e "${RED}FAIL (expected compiler error, got success)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    if [ -n "$expected_msg" ]; then
        if echo "$output" | grep -q "$expected_msg"; then
            echo -e "${GREEN}PASS${NC}"
            PASS=$((PASS + 1))
        else
            echo -e "${RED}FAIL (expected message '${expected_msg}' not found)${NC}"
            echo "  Output: $output"
            FAIL=$((FAIL + 1))
            return 1
        fi
    else
        echo -e "${GREEN}PASS${NC}"
        PASS=$((PASS + 1))
    fi
    return 0
}

echo ""

test_expect_error "error_undefined_build_var" \
    "$SCRIPT_DIR/error_macro_undefined.brc" \
    "undefined variable"

test_expect_error "error_wrong_arg_count" \
    "$SCRIPT_DIR/error_macro_wrong_arg_count.brc" \
    "expect"

test_expect_error "error_dollar_outside" \
    "$SCRIPT_DIR/error_macro_dollar_outside.brc" \
    ""

test_expect_error "error_io_in_build" \
    "$SCRIPT_DIR/error_macro_io_in_build.brc" \
    ""

test_expect_error "error_recursive" \
    "$SCRIPT_DIR/error_macro_recursive.brc" \
    "recursion"

echo ""
echo -e "${CYAN}========================================${NC}"
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
echo -e "${CYAN}========================================${NC}"

exit $FAIL
