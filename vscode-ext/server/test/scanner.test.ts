import { scanDocument, ScanResult } from '../src/languageService';

let passed = 0;
let failed = 0;

function assert(condition: boolean, msg: string) {
    if (condition) {
        passed++;
    } else {
        failed++;
        console.error(`  FAIL: ${msg}`);
    }
}

function assertEq<T>(a: T, b: T, msg: string) {
    if (a === b) {
        passed++;
    } else {
        failed++;
        console.error(`  FAIL: ${msg} — expected ${JSON.stringify(b)}, got ${JSON.stringify(a)}`);
    }
}

function assertTokenType(result: ScanResult, index: number, expectedType: string, expectedLexeme: string, context: string) {
    if (index >= result.tokens.length) {
        failed++;
        console.error(`  FAIL: ${context} — token #${index} missing (only ${result.tokens.length} tokens)`);
        return;
    }
    const t = result.tokens[index];
    if (t.type === expectedType && t.lexeme === expectedLexeme) {
        passed++;
    } else {
        failed++;
        console.error(`  FAIL: ${context} — token #${index}: expected {${expectedType}, "${expectedLexeme}"}, got {${t.type}, "${t.lexeme}"}`);
    }
}

function runSuite(name: string, fn: () => void) {
    console.log(`\n=== ${name} ===`);
    fn();
}

// ──────────────────────────────────────────
// Test 1: Token types (no conflated types)
// ──────────────────────────────────────────
runSuite('Token type conflation', () => {
    const r = scanDocument('42 3.14 "hello" \'a\'');
    assert(r.tokens.length >= 4, 'has at least 4 tokens');
    // Find each token by scanning
    const intTok = r.tokens.find(t => t.lexeme === '42');
    const floatTok = r.tokens.find(t => t.lexeme === '3.14');
    const strTok = r.tokens.find(t => t.lexeme === '"hello"');
    const charTok = r.tokens.find(t => t.lexeme === "'a'");

    assert(intTok?.type === 'INT_LITERAL', `42 should be INT_LITERAL, got ${intTok?.type}`);
    assert(floatTok?.type === 'FLOAT_LITERAL', `3.14 should be FLOAT_LITERAL, got ${floatTok?.type}`);
    assert(strTok?.type === 'STRING_LITERAL', `"hello" should be STRING_LITERAL, got ${strTok?.type}`);
    assert(charTok?.type === 'CHAR_LITERAL', `'a' should be CHAR_LITERAL, got ${charTok?.type}`);
});

// ──────────────────────────────────────────
// Test 2: Fixed-width types as keywords
// ──────────────────────────────────────────
runSuite('Fixed-width type keywords', () => {
    const r = scanDocument('u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 usize isize byte');
    const expected = ['u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64', 'f32', 'f64', 'usize', 'isize', 'byte'];
    for (const kw of expected) {
        const tok = r.tokens.find(t => t.lexeme === kw);
        assert(tok !== undefined, `token for '${kw}' exists`);
        assert(tok!.type !== 'IDENTIFIER', `'${kw}' should not be IDENTIFIER, got ${tok!.type}`);
        // Keyword tokens have uppercase type matching the lexeme
        assert(tok!.type === kw.toUpperCase(), `'${kw}' type is ${tok!.type} (expected ${kw.toUpperCase()})`);
    }
});

// ──────────────────────────────────────────
// Test 3: Standard keywords
// ──────────────────────────────────────────
runSuite('Standard keywords', () => {
    const r = scanDocument('package using private public struct extends interface fn return block reset if else while for error int float bool char String void null true false');
    const expected = ['package','using','private','public','struct','extends','interface','fn','return',
        'block','reset','if','else','while','for','error',
        'int','float','bool','char','String','void','null','true','false'];
    for (const kw of expected) {
        const tok = r.tokens.find(t => t.lexeme === kw);
        assert(tok !== undefined, `token for '${kw}' exists`);
        assert(tok!.type !== 'IDENTIFIER', `'${kw}' should be keyword, not IDENTIFIER`);
    }
});

// ──────────────────────────────────────────
// Test 4: Missing operators
// ──────────────────────────────────────────
runSuite('New operators', () => {
    const r = scanDocument('<< >> *= /= & | ^ ~');
    assertTokenType(r, 0, 'LSHIFT', '<<', '<<');
    assertTokenType(r, 1, 'RSHIFT', '>>', '>>');
    assertTokenType(r, 2, 'STAR_ASSIGN', '*=', '*=');
    assertTokenType(r, 3, 'SLASH_ASSIGN', '/=', '/=');
    assertTokenType(r, 4, 'BIT_AND', '&', '&');
    assertTokenType(r, 5, 'BIT_OR', '|', '|');
    assertTokenType(r, 6, 'BIT_XOR', '^', '^');
    assertTokenType(r, 7, 'BIT_NOT', '~', '~');
});

// ──────────────────────────────────────────
// Test 5: Unterminated strings and chars
// ──────────────────────────────────────────
runSuite('Unterminated string/char errors', () => {
    const r1 = scanDocument('"unterminated\n42');
    assert(r1.errors.length > 0, 'unterminated string should produce error');
    assert(r1.errors[0].message.includes('unterminated'), `error message mentions unterminated: ${r1.errors[0].message}`);

    // Properly terminated should not error
    const r4 = scanDocument(`"ok" 'x'`);
    assert(r4.errors.length === 0, `properly terminated has no errors (got ${r4.errors.length})`);
});

// ──────────────────────────────────────────
// Test 6: Variable detection
// ──────────────────────────────────────────
runSuite('Variable detection', () => {
    const r = scanDocument(`
fn test() {
    int x = 42
    u8 y = 1
    float z
    Player p = Player()
    int[10] arr
    u8[256] buf
}
`);
    const vars = r.symbols.filter(s => s.kind === 'variable');
    const varNames = vars.map(v => `${v.name}:${v.type_name}`);

    assert(vars.some(v => v.name === 'x' && v.type_name === 'int'), `int x found in ${JSON.stringify(varNames)}`);
    assert(vars.some(v => v.name === 'y' && v.type_name === 'u8'), `u8 y found`);
    assert(vars.some(v => v.name === 'z' && v.type_name === 'float'), `float z found (no init)`);
    assert(vars.some(v => v.name === 'p' && v.type_name === 'Player'), `Player p found`);
    assert(vars.some(v => v.name === 'arr' && v.type_name === 'int[]'), `int[10] arr found (type=int[]) in ${JSON.stringify(varNames)}`);
    assert(vars.some(v => v.name === 'buf' && v.type_name === 'u8[]'), `u8[256] buf found`);
});

// ──────────────────────────────────────────
// Test 7: Block declarations and scopes
// ──────────────────────────────────────────
runSuite('Block handling', () => {
    const r = scanDocument(`
block global = 256MB
block game = 64MB
block temp {
    int x = 1
}
block scope:
    int y = 2
`);
    const blockNames = r.blocks.map(b => b.name);
    assert(blockNames.includes('global'), 'global block found');
    assert(blockNames.includes('game'), 'game block found');
    assert(blockNames.includes('temp'), 'temp block (scope {}) found');
    assert(blockNames.includes('scope'), 'scope block (colon) found');
    assert(r.blocks.length >= 4, `at least 4 blocks found (got ${r.blocks.length})`);
});

// ──────────────────────────────────────────
// Test 8: Struct parsing with fields and methods
// ──────────────────────────────────────────
runSuite('Struct parsing', () => {
    const r = scanDocument(`
struct Player {
    int hp
    String name
    fn take_damage(int dmg) {
        hp -= dmg
    }
    fn Player(int h, String n) {
        hp = h
        name = n
    }
}
`);
    assert(r.structs.has('Player'), 'Player struct found');
    const player = r.structs.get('Player')!;
    assert(player.fields.length >= 2, `Player has at least 2 fields (got ${player.fields.length})`);
    assert(player.fields.some(f => f.name === 'hp' && f.type === 'int'), 'hp field: int');
    assert(player.fields.some(f => f.name === 'name' && f.type === 'String'), 'name field: String');
    assert(player.methods.length >= 2, `Player has at least 2 methods (got ${player.methods.length})`);
    assert(player.methods.some(m => m.name === 'take_damage'), 'take_damage method found');
    assert(player.methods.some(m => m.name === 'Player'), 'constructor Player found');
});

// ──────────────────────────────────────────
// Test 9: Interface parsing
// ──────────────────────────────────────────
runSuite('Interface parsing', () => {
    const r = scanDocument(`
interface Damageable {
    fn take_damage(int dmg)
    fn heal(int amount)
}
`);
    const iface = r.symbols.find(s => s.kind === 'interface' && s.name === 'Damageable');
    assert(iface !== undefined, 'Damageable interface symbol found');
});

// ──────────────────────────────────────────
// Test 10: Package and using
// ──────────────────────────────────────────
runSuite('Package and using', () => {
    const r = scanDocument('package SPRITES\nusing IO\nprivate int x');
    assertEq(r.packageName, 'SPRITES', 'package name is SPRITES');
    assert(r.usings.includes('IO'), 'using IO found');
});

// ──────────────────────────────────────────
// Test 11: Function detection (top-level)
// ──────────────────────────────────────────
runSuite('Function detection', () => {
    const r = scanDocument(`
fn add(int a, int b) -> int {
    return a + b
}
fn main() {
    print("hello")
}
`);
    const fnAdd = r.symbols.find(s => s.kind === 'function' && s.name === 'add');
    const fnMain = r.symbols.find(s => s.kind === 'function' && s.name === 'main');
    assert(fnAdd !== undefined, 'add function found');
    assert(fnAdd!.type_name === 'int', `add return type is int (got ${fnAdd!.type_name})`);
    assert(fnMain !== undefined, 'main function found');
});

// ──────────────────────────────────────────
// Test 12: Array detection in struct fields
// ──────────────────────────────────────────
runSuite('Array in struct fields', () => {
    const r = scanDocument(`
struct Config {
    int[10] values
    u8[256] buffer
}
`);
    const cfg = r.structs.get('Config');
    assert(cfg !== undefined, 'Config struct found');
    if (cfg) {
        assert(cfg.fields.some(f => f.name === 'values'), 'values field found');
        assert(cfg.fields.some(f => f.name === 'buffer'), 'buffer field found');
    }
});

// ──────────────────────────────────────────
// Test 13: Fixed-width types in struct fields
// ──────────────────────────────────────────
runSuite('Fixed-width types in struct fields', () => {
    const r = scanDocument(`
struct Packet {
    u8 flags
    u16 length
    i32 checksum
    f64 timestamp
    byte data
}
`);
    const pkt = r.structs.get('Packet');
    assert(pkt !== undefined, 'Packet struct found');
    if (pkt) {
        assert(pkt.fields.some(f => f.name === 'flags' && f.type === 'u8'), 'flags: u8');
        assert(pkt.fields.some(f => f.name === 'length' && f.type === 'u16'), 'length: u16');
        assert(pkt.fields.some(f => f.name === 'checksum' && f.type === 'i32'), 'checksum: i32');
        assert(pkt.fields.some(f => f.name === 'timestamp' && f.type === 'f64'), 'timestamp: f64');
        assert(pkt.fields.some(f => f.name === 'data' && f.type === 'byte'), 'data: byte');
    }
});

// ──────────────────────────────────────────
// Test 14: C interop — include, link, extern
// ──────────────────────────────────────────
runSuite('C interop keywords', () => {
    const r = scanDocument('include "math.h"\nlink m\nextern fn sqrt(f64 x) -> f64');
    const includeTok = r.tokens.find(t => t.lexeme === 'include');
    const linkTok = r.tokens.find(t => t.lexeme === 'link');
    const externTok = r.tokens.find(t => t.lexeme === 'extern');

    assert(includeTok?.type === 'INCLUDE', `include should be INCLUDE, got ${includeTok?.type}`);
    assert(linkTok?.type === 'LINK', `link should be LINK, got ${linkTok?.type}`);
    assert(externTok?.type === 'EXTERN', `extern should be EXTERN, got ${externTok?.type}`);

    // String literal for header
    const strTok = r.tokens.find(t => t.lexeme === '"math.h"');
    assert(strTok?.type === 'STRING_LITERAL', `"math.h" should be STRING_LITERAL, got ${strTok?.type}`);
});

// ──────────────────────────────────────────
// Test 15: C interop — extern fn declaration
// ──────────────────────────────────────────
runSuite('Extern fn detection', () => {
    const r = scanDocument(`
extern fn sqrt(f64 x) -> f64
extern fn puts(*u8 s) -> i32
extern fn sin(f64 x) -> f64
`);
    const sqrtSym = r.symbols.find(s => s.name === 'sqrt');
    const putsSym = r.symbols.find(s => s.name === 'puts');
    const sinSym = r.symbols.find(s => s.name === 'sin');

    assert(sqrtSym !== undefined, 'sqrt symbol found');
    assert(putsSym !== undefined, 'puts symbol found');
    assert(sinSym !== undefined, 'sin symbol found');
    assert(sqrtSym!.kind === 'function', `sqrt kind is function, got ${sqrtSym!.kind}`);
    assert(sqrtSym!.type_name === 'f64', `sqrt return type is f64, got ${sqrtSym!.type_name}`);
    assert(putsSym!.type_name === 'i32', `puts return type is i32, got ${putsSym!.type_name}`);
});

// ──────────────────────────────────────────
// Test 16: C interop — include and link combined
// ──────────────────────────────────────────
runSuite('Include and link combined', () => {
    const r = scanDocument('include "math.h" and link m');
    const includeTok = r.tokens.find(t => t.lexeme === 'include');
    const andTok = r.tokens.find(t => t.lexeme === 'and');
    const linkTok = r.tokens.find(t => t.lexeme === 'link');

    assert(includeTok !== undefined, 'include token exists');
    assert(andTok?.type === 'AND', `and should be AND, got ${andTok?.type}`);
    assert(linkTok !== undefined, 'link token exists');
    assert(r.errors.length === 0, `no errors (got ${r.errors.length})`);
});

// ──────────────────────────────────────────
// Test 17: Pointer type in struct fields
// ──────────────────────────────────────────
runSuite('Pointer type in struct fields', () => {
    const r = scanDocument(`
struct Buffer {
    *u8 data
    *void ctx
    int length
}
`);
    const buf = r.structs.get('Buffer');
    assert(buf !== undefined, 'Buffer struct found');
    if (buf) {
        const dataField = buf.fields.find(f => f.name === 'data');
        const ctxField = buf.fields.find(f => f.name === 'ctx');
        const lenField = buf.fields.find(f => f.name === 'length');
        assert(dataField !== undefined, 'data field (*u8) found');
        assert(ctxField !== undefined, 'ctx field (*void) found');
        assert(lenField !== undefined, 'length field (int) found');
        assert(dataField!.type === '*u8', `data type is *u8, got ${dataField!.type}`);
        assert(ctxField!.type === '*void', `ctx type is *void, got ${ctxField!.type}`);
    }
});

// ──────────────────────────────────────────
// Test 18: Pointer type variable detection
// ──────────────────────────────────────────
runSuite('Pointer type variable detection', () => {
    const r = scanDocument(`
fn test() {
    *u8 ptr
    *void handle
    *Player ref
}
`);
    const ptrVar = r.symbols.find(s => s.name === 'ptr');
    const handleVar = r.symbols.find(s => s.name === 'handle');
    const refVar = r.symbols.find(s => s.name === 'ref');

    assert(ptrVar !== undefined, 'ptr variable found');
    assert(handleVar !== undefined, 'handle variable found');
    assert(refVar !== undefined, 'ref variable found');
    assert(ptrVar!.type_name === '*u8', `ptr type is *u8, got ${ptrVar!.type_name}`);
    assert(handleVar!.type_name === '*void', `handle type is *void, got ${handleVar!.type_name}`);
    assert(refVar!.type_name === '*Player', `ref type is *Player, got ${refVar!.type_name}`);
});

// ──────────────────────────────────────────
// Test 19: Pointer array types
// ──────────────────────────────────────────
runSuite('Pointer array types', () => {
    const r = scanDocument(`
struct Team {
    *Player[10] members
    *u8[256] buffer
}
`);
    const team = r.structs.get('Team');
    assert(team !== undefined, 'Team struct found');
    if (team) {
        const membersField = team.fields.find(f => f.name === 'members');
        const bufferField = team.fields.find(f => f.name === 'buffer');
        assert(membersField !== undefined, 'members field found');
        assert(bufferField !== undefined, 'buffer field found');
        assert(membersField!.type === '*Player[]', `members type is *Player[], got ${membersField!.type}`);
        assert(bufferField!.type === '*u8[]', `buffer type is *u8[], got ${bufferField!.type}`);
    }
});

// ──────────────────────────────────────────
// Test 20: Macro keywords and $ sigil
// ──────────────────────────────────────────
runSuite('Macro keywords and $ sigil', () => {
    const r = scanDocument(`
macro gen_getter(name, type) {
    $name = $type
    emit { fn $name() -> $type { return $name } }
}

build {
    x = 42
    emit { z = x + 10 }
}
`);
    assert(r.tokens.some(t => t.type === 'MACRO' && t.lexeme === 'macro'), 'macro keyword token found');
    assert(r.tokens.some(t => t.type === 'BUILD' && t.lexeme === 'build'), 'build keyword token found');
    assert(r.tokens.some(t => t.type === 'EMIT' && t.lexeme === 'emit'), 'emit keyword token found');
    const dollarTokens = r.tokens.filter(t => t.type === 'DOLLAR_IDENTIFIER');
    assert(dollarTokens.length >= 4, `has at least 4 $name tokens, got ${dollarTokens.length}`);
    assert(dollarTokens.every(t => t.lexeme.startsWith('$')), 'all $ tokens start with $');
});

// ──────────────────────────────────────────
// Test 21: Macro rest params (ellipsis)
// ──────────────────────────────────────────
runSuite('Macro rest params (ellipsis)', () => {
    const r = scanDocument('macro foo(valores...) { $valores }');
    assert(r.tokens.some(t => t.type === 'ELLIPSIS' && t.lexeme === '...'), 'ellipsis token found');
    assert(r.tokens.some(t => t.type === 'MACRO'), 'macro token found');
    assert(r.tokens.some(t => t.type === 'DOLLAR_IDENTIFIER' && t.lexeme === '$valores'), 'dollar identifier found');
});

// ──────────────────────────────────────────
// Test 22: Dollar sigil edge cases
// ──────────────────────────────────────────
runSuite('Dollar sigil edge cases', () => {
    // $(expr) syntax
    const r1 = scanDocument('$(42 + 1)');
    assert(r1.tokens.some(t => t.type === 'DOLLAR_LPAREN' && t.lexeme === '$('), '$( token found');
    assert(r1.tokens.some(t => t.type === 'INT_LITERAL' && t.lexeme === '42'), 'int inside $( found');

    // Standalone $
    const r2 = scanDocument('$');
    assert(r2.tokens.some(t => t.type === 'DOLLAR' && t.lexeme === '$'), 'standalone $ token found');
});

// ──────────────────────────────────────────
// Summary
// ──────────────────────────────────────────
console.log(`\n═══════════════════════════════════`);
console.log(`Results: ${passed} passed, ${failed} failed${failed > 0 ? ' ❌' : ' ✅'}`);
console.log(`═══════════════════════════════════\n`);

process.exit(failed > 0 ? 1 : 0);
