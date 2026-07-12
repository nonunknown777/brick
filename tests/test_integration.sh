#!/bin/bash
# Brick Integration Test
# Teste de Integracao Brick
# Compila um .brc → .c → gcc → executa binário e verifica saída
# Compila um .brc → .c → gcc → executa binario e verifica saida

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
BRICK="$BUILD_DIR/brick"
RUNTIME="$PROJECT_DIR/runtime/block_memory.c $PROJECT_DIR/runtime/hot_reload.c $PROJECT_DIR/runtime/io.c $PROJECT_DIR/runtime/pool_allocator.c"
PASS=0
FAIL=0

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m'

mkdir -p "$BUILD_DIR"

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Brick Integration Tests${NC}"
echo -e "${CYAN}  Testes de Integracao Brick${NC}"
echo -e "${CYAN}========================================${NC}"

# Check if compiler exists
# Verifica se o compilador existe
if [ ! -f "$BRICK" ]; then
    echo -e "${RED}Compiler not found. Run 'scons' first.${NC}"
    exit 1
fi

test_compile_and_run() {
    local name="$1"
    local brc_file="$2"
    local c_file="$BUILD_DIR/${name}.c"
    local bin_file="$BUILD_DIR/${name}"

    echo -ne "  ${name}... "

    # Compile .brc → .c
    # Compila .brc → .c
    $BRICK "$brc_file" -o "$c_file" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL (compiler error)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    # Compile .c → binary
    # Compila .c → binario
    gcc -O3 -I"$PROJECT_DIR/runtime" "$c_file" $RUNTIME -o "$bin_file" -ldl 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL (gcc error)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    # Run binary
    # Executa binario
    output=$("$bin_file" 2>&1)
    local exit_code=$?

    if [ $exit_code -ne 0 ]; then
        echo -e "${RED}FAIL (exit code $exit_code)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    echo -e "${GREEN}PASS${NC}"
    PASS=$((PASS + 1))
    return 0
}

test_compile_and_expect() {
    local name="$1"
    local brc_file="$2"
    local expected="$3"
    local c_file="$BUILD_DIR/${name}.c"
    local bin_file="$BUILD_DIR/${name}"

    echo -ne "  ${name}... "

    # Compile .brc → .c
    # Compila .brc → .c
    $BRICK "$brc_file" -o "$c_file" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL (compiler error)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    # Compile .c → binary
    # Compila .c → binario
    gcc -O3 -I"$PROJECT_DIR/runtime" "$c_file" $RUNTIME -o "$bin_file" -ldl 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL (gcc error)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    # Run binary and check output
    # Executa binario e verifica saida
    output=$("$bin_file" 2>&1)
    local exit_code=$?

    if [ $exit_code -ne 0 ]; then
        echo -e "${RED}FAIL (exit code $exit_code)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    if [ "$output" != "$expected" ]; then
        echo -e "${RED}FAIL (output mismatch)${NC}"
        echo "  Expected: $expected"
        echo "  Got:      $output"
        FAIL=$((FAIL + 1))
        return 1
    fi

    echo -e "${GREEN}PASS${NC}"
    PASS=$((PASS + 1))
    return 0
}

echo ""
echo "Compiling and running examples..."
echo "Compilando e executando exemplos..."

# Test with a simple .brc example
# Testa com um exemplo .brc simples
# Create a minimal test that compiles but doesn't need I/O
# Cria um teste minimo que compila mas nao precisa de I/O
cat > "$BUILD_DIR/test_simple.brc" << 'EOF'
package TEST

block global = 1MB

struct Test {
    int value

    fn Test(int v) {
        value = v
    }

    fn get_value() -> int {
        return value
    }
}

fn main() {
    Test t = Test(42) @global
    int x = t.get_value()
    global.reset()
}
EOF

test_compile_and_run "test_simple" "$BUILD_DIR/test_simple.brc"

# Test blocks
# Testa blocos
cat > "$BUILD_DIR/test_blocks.brc" << 'EOF'
package TESTBLOCK

block global = 1MB
block temp = 512KB

struct Item {
    int id
    String name

    fn Item(int i, String n) {
        id = i
        name = n
    }
}

fn main() {
    Item i = Item(1, "test") @temp
    int x = i.id @global
    temp.reset()
    global.reset()
}
EOF

test_compile_and_run "test_blocks" "$BUILD_DIR/test_blocks.brc"

echo ""
echo "Testing IO (print)..."
echo "Testando IO (print)..."

# Test IO: print with various types
# Testa IO: print com varios tipos
cat > "$BUILD_DIR/test_io_print.brc" << 'EOF'
package TEST_IO

using IO

fn main() {
    print("hello")
    print(42)
    print(3.14)
    print(true)
    print('!')
}
EOF

test_compile_and_expect "test_io_print" "$BUILD_DIR/test_io_print.brc" \
"hello
42
3.140000
true
!"

# Test IO: formatted print
# Testa IO: print formatado
cat > "$BUILD_DIR/test_io_format.brc" << 'EOF'
package TEST_IO_FMT

using IO

fn main() {
    print("int={0}, str={1}", 10, "test")
}
EOF

test_compile_and_expect "test_io_format" "$BUILD_DIR/test_io_format.brc" \
"int=10, str=test"

# Test IO: print without using → should fail to compile
# Testa IO: print sem using → deve falhar ao compilar
cat > "$BUILD_DIR/test_io_no_using.brc" << 'EOF'
package TEST_NO_IO

fn main() {
    print("no io")
}
EOF

echo -n "  test_io_no_using... "
$BRICK "$BUILD_DIR/test_io_no_using.brc" -o "$BUILD_DIR/test_io_no_using.c" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo -e "${GREEN}PASS (expected compiler error)${NC}"
    PASS=$((PASS + 1))
else
    echo -e "${RED}FAIL (should have errored)${NC}"
    FAIL=$((FAIL + 1))
fi

echo ""
echo "Testing macros..."
echo "Testando macros..."

cat > "$BUILD_DIR/test_macro_swap.brc" << 'EOF'
package TEST_MACRO

using IO

macro swap(a, b) {
    tmp = $a
    $a = $b
    $b = tmp
}

fn main() {
    int x = 10
    int y = 20
    swap(x, y)
    print("x={0}, y={1}", x, y)
}
EOF

test_compile_and_expect "test_macro_swap" "$BUILD_DIR/test_macro_swap.brc" \
"x=20, y=10"

cat > "$BUILD_DIR/test_macro_no_params.brc" << 'EOF'
package TEST_MACRO_NOPARAM

using IO

macro say_hello() {
    print("hello from macro")
}

fn main() {
    say_hello()
    say_hello()
}
EOF

test_compile_and_expect "test_macro_no_params" "$BUILD_DIR/test_macro_no_params.brc" \
"hello from macro
hello from macro"

cat > "$BUILD_DIR/test_build_const.brc" << 'EOF'
package TEST_BUILD

using IO

build {
    // Build block runs at compile time
    // The emit generates the actual runtime code
    emit {
        fn main() {
            print("built at compile time")
        }
    }
}
EOF

test_compile_and_expect "test_build_const" "$BUILD_DIR/test_build_const.brc" \
"built at compile time"

cat > "$BUILD_DIR/test_macro_vec2.brc" << 'EOF'
package TEST_VEC2

using IO

macro vec2_add_pair() {
    emit {
        fn add_pair(i32 ax, i32 ay, i32 bx, i32 by) {
            i32 rx = ax + bx
            i32 ry = ay + by
            print("x={0}, y={1}", rx, ry)
        }
    }
}

vec2_add_pair()

fn main() {
    add_pair(1, 2, 3, 4)
}
EOF

test_compile_and_expect "test_macro_vec2" "$BUILD_DIR/test_macro_vec2.brc" \
"x=4, y=6"

cat > "$BUILD_DIR/test_macro_enum.brc" << 'EOF'
package TEST_ENUM

using IO

macro make_const(name, value) {
    emit {
        i32 $name = $value
    }
}

fn main() {
    make_const(W_FIST, 0)
    make_const(W_GUN, 1)
    make_const(W_RIFLE, 2)
    make_const(W_ROCKET, 3)
    print("fist={0} gun={1} rifle={2} rocket={3}", W_FIST, W_GUN, W_RIFLE, W_ROCKET)
}
EOF

test_compile_and_expect "test_macro_enum" "$BUILD_DIR/test_macro_enum.brc" \
"fist=0 gun=1 rifle=2 rocket=3"

echo ""
echo "Testing dynamic arrays in structs..."
echo "Testando arrays dinamicos em structs..."

test_compile_and_expect "test_dynarray" "$PROJECT_DIR/tests/features/test_dynamic_arrays.brc" \
"PASS: all dynamic array tests"

echo ""
echo "Testing impl + interface (polymorphism)..."
echo "Testando impl + interface (polimorfismo)..."

test_compile_and_expect "test_impl" "$PROJECT_DIR/tests/features/test_impl_separate.brc" \
"drawing circle with radius 1.500000
name = circle"

echo ""
echo "Testing .sizeof..."
echo "Testando .sizeof..."

test_compile_and_expect "test_sizeof" "$PROJECT_DIR/tests/features/test_sizeof.brc" \
"PASS: all sizeof tests"

echo ""
echo "Testing enum hex..."
echo "Testando enum hex..."

# Verify hex enum values in generated C code
$BRICK "$PROJECT_DIR/tests/features/test_enum_hex.brc" -o "$BUILD_DIR/test_enum_hex.c" > /dev/null 2>&1
if [ $? -eq 0 ]; then
    # Check the generated C code has correct hex-to-decimal conversions
    if grep -q "#define HIGH 43981" "$BUILD_DIR/test_enum_hex.c" && \
       grep -q "#define ALL 255" "$BUILD_DIR/test_enum_hex.c" && \
       grep -q "#define READ 1" "$BUILD_DIR/test_enum_hex.c"; then
        echo -e "  test_enum_hex... ${GREEN}PASS${NC}"
        PASS=$((PASS + 1))
    else
        echo -e "  test_enum_hex... ${RED}FAIL (wrong enum values in .c)${NC}"
        FAIL=$((FAIL + 1))
    fi
else
    echo -e "  test_enum_hex... ${RED}FAIL${NC}"
    FAIL=$((FAIL + 1))
fi

echo ""
echo "Testing @packed/@align..."
echo "Testando @packed/@align..."

test_compile_and_expect "test_packed_align" "$PROJECT_DIR/tests/features/test_packed_align.brc" \
"PASS: all packed/align tests"

echo ""
echo "Testing vtbl dispatch..."
echo "Testando despacho vtbl..."

test_compile_and_run "test_vtbl_dispatch" "$PROJECT_DIR/tests/features/test_vtbl_dispatch.brc"

echo ""
echo "Testing multidimensional arrays..."
echo "Testando arrays multidimensionais..."

test_compile_and_run "test_multidim_array" "$PROJECT_DIR/tests/test_multidim_array.brc"

echo ""
echo "Testing const-size arrays..."
echo "Testando arrays com tamanho constante..."

test_compile_and_run "test_const_array" "$PROJECT_DIR/tests/test_const_array.brc"

echo ""
echo "Testing dynamic array append..."
echo "Testando append de array dinamico..."

test_compile_and_run "test_append" "$PROJECT_DIR/tests/test_append.brc"

echo ""
echo "Testing vtbl in dynamic arrays..."
echo "Testando vtbl em arrays dinamicos..."

test_compile_and_run "test_vtbl_dynarray" "$PROJECT_DIR/tests/test_vtbl_dynarray.brc"

echo ""
echo "Testing defer statement..."
echo "Testando defer..."

test_compile_and_expect "test_defer" "$PROJECT_DIR/tests/features/test_defer.brc" \
"=== test_basic_defer ===
before defer
defer ran
=== test_lifo_order ===
before lifo defers
third defer
second defer
first defer
=== test_defer_before_return ===
about to return
cleanup before return
returned 42
=== test_nested_block_defer ===
inside inner block
inner defer
after inner block
outer defer
PASS: all defer tests"

echo ""
echo "Testing build or operator..."
echo "Testando operador or em build..."

test_compile_and_expect "test_build_or" "$PROJECT_DIR/tests/features/test_build_or.brc" \
"build or test: 100
PASS: build or test"

echo ""
echo "Testing for x in N range loop..."
echo "Testando loop for x in N..."

test_compile_and_expect "test_for_range" "$PROJECT_DIR/tests/features/test_for_range.brc" \
"sum_to(5) = 10
sum_to(1) = 0
sum_to(0) = 0
PASS: all for range tests"

echo ""
echo "Testing match with guards..."
echo "Testando match com guards..."

test_compile_and_expect "test_match_guard" "$PROJECT_DIR/tests/features/test_match_guard.brc" \
"guard_matches = 1
guard_falls_through = 20
guard_wildcard = 200
PASS: all match guard tests"

echo ""
echo "Testing nested anonymous structs/unions..."
echo "Testando structs/unions anonimos aninhados..."

test_compile_and_expect "test_nested_anon" "$PROJECT_DIR/tests/features/test_nested_anon.brc" \
"p.id=42, p.x=10, p.y=20
d.low=11, d.high=10
m.r=17, m.g=34, m.b=51
PASS: all nested anon tests"

echo ""
echo ""
echo "Testing dynamic array with block allocation..."
echo "Testando array dinamico com alocacao em bloco..."

test_compile_and_expect "test_dynarray_block" "$PROJECT_DIR/tests/features/test_dynarray_block.brc" \
"len=4, cap=4
n0=10, n1=20, n2=30, n3=40
after growth: len=6, cap=8
n4=50, n5=60
values[0]=1.500000, values[1]=2.500000
PASS: all dynarray block tests"

echo ""
echo "Testing multi-file packages..."
echo "Testando pacotes multi-arquivo..."

# Test: using MATH package from test_pkg_main.brc
# Testa: usando pacote MATH de test_pkg_main.brc
$BRICK build "$PROJECT_DIR/tests/packages/test_pkg_main.brc" -I "$PROJECT_DIR/tests/packages" \
  -o "$BUILD_DIR/test_pkg_main" --release 2>/dev/null
if [ $? -eq 0 ]; then
    output=$("$BUILD_DIR/test_pkg_main" 2>&1)
    expected="add(3,4) = 7
PI = 31415
v.x = 10, v.y = 20
color is GREEN
Circle(radius=5)"
    if [ "$output" = "$expected" ]; then
        echo -e "  test_pkg_multi_file... ${GREEN}PASS${NC}"
        PASS=$((PASS + 1))
    else
        echo -e "  test_pkg_multi_file... ${RED}FAIL (output mismatch)${NC}"
        echo "  Expected: $expected"
        echo "  Got:      $output"
        FAIL=$((FAIL + 1))
    fi
else
    echo -e "  test_pkg_multi_file... ${RED}FAIL (build error)${NC}"
    FAIL=$((FAIL + 1))
fi

echo ""
echo "Testing nested packages (using MATH.VEC2)..."
echo "Testando pacotes aninhados (using MATH.VEC2)..."

$BRICK build "$PROJECT_DIR/tests/packages/test_pkg_nested.brc" -I "$PROJECT_DIR/tests/packages" \
  -o "$BUILD_DIR/test_pkg_nested" --release 2>/dev/null
if [ $? -eq 0 ]; then
    output=$("$BUILD_DIR/test_pkg_nested" 2>&1)
    expected="v.x = 3.000000, v.y = 4.000000
length = 25.000000"
    if [ "$output" = "$expected" ]; then
        echo -e "  test_pkg_nested... ${GREEN}PASS${NC}"
        PASS=$((PASS + 1))
    else
        echo -e "  test_pkg_nested... ${RED}FAIL (output mismatch)${NC}"
        echo "  Expected: $expected"
        echo "  Got:      $output"
        FAIL=$((FAIL + 1))
    fi
else
    echo -e "  test_pkg_nested... ${RED}FAIL (build error)${NC}"
    FAIL=$((FAIL + 1))
fi

echo ""
echo "Testing private symbol rejection (negative test with -I)..."
echo "Testando rejeicao de simbolo privado (teste negativo com -I)..."

echo -n "  test_pkg_private_fail... "
$BRICK build "$PROJECT_DIR/tests/packages/test_pkg_private_fail.brc" -I "$PROJECT_DIR/tests/packages" \
  -o "$BUILD_DIR/test_pkg_private_fail" --release 2>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${GREEN}PASS (expected compiler error for private symbol)${NC}"
    PASS=$((PASS + 1))
else
    # Even if build succeeds, verify binary does NOT have SECRET value
    output=$("$BUILD_DIR/test_pkg_private_fail" 2>&1)
    echo -e "${RED}FAIL (should have errored on private symbol 'SECRET', got: $output)${NC}"
    FAIL=$((FAIL + 1))
fi

echo ""
echo "Testing C interop..."
echo "Testando C interop..."

test_compile_and_run "test_c_interop" "$PROJECT_DIR/tests/test_c_interop.brc"

echo ""
echo -e "${CYAN}========================================${NC}"
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
echo -e "${CYAN}========================================${NC}"

exit $FAIL
