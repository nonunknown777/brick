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
    const r = scanDocument('package using private public struct extends interface fn return block reset if else while for error include link extern and macro build emit int float bool char String void null true false include and');
    const expected = ['package','using','private','public','struct','extends','interface','fn','return',
        'block','reset','if','else','while','for','error',
        'include','link','extern','and','macro','build','emit',
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
// Test 23: New keywords (break, continue, const, defer, enum, match, not, is, as)
// ──────────────────────────────────────────
runSuite('New keywords', () => {
    const r = scanDocument('not break continue const defer enum match is as');
    const expected = ['not','break','continue','const','defer','enum','match','is','as'];
    for (const kw of expected) {
        const tok = r.tokens.find(t => t.lexeme === kw);
        assert(tok !== undefined, `token for '${kw}' exists`);
        assert(tok!.type !== 'IDENTIFIER', `'${kw}' should be keyword, not IDENTIFIER`);
        assert(tok!.type === kw.toUpperCase(), `'${kw}' type is ${tok!.type} (expected ${kw.toUpperCase()})`);
    }
});

// ──────────────────────────────────────────
// Test 24: Block comments /* */
// ──────────────────────────────────────────
runSuite('Block comments', () => {
    const r = scanDocument('/* hello */ 42 /* nested /* deep */ end */');
    const comments = r.tokens.filter(t => t.type === 'COMMENT');
    assert(comments.length >= 2, `at least 2 comments found (got ${comments.length})`);
    assert(comments[0].lexeme.includes('/*'), 'first comment starts with /*');
    assert(comments[1].lexeme.includes('end'), 'second comment includes "end"');
});

// ──────────────────────────────────────────
// Test 25: Hex, octal, binary number literals
// ──────────────────────────────────────────
runSuite('Hex/octal/binary literals', () => {
    const r = scanDocument('0xFF 0b1010 0o77 1_000_000');
    const hexTok = r.tokens.find(t => t.lexeme === '0xFF');
    const binTok = r.tokens.find(t => t.lexeme === '0b1010');
    const octTok = r.tokens.find(t => t.lexeme === '0o77');
    const undTok = r.tokens.find(t => t.lexeme === '1_000_000');
    assert(hexTok?.type === 'INT_LITERAL', '0xFF is INT_LITERAL');
    assert(binTok?.type === 'INT_LITERAL', '0b1010 is INT_LITERAL');
    assert(octTok?.type === 'INT_LITERAL', '0o77 is INT_LITERAL');
    assert(undTok?.type === 'INT_LITERAL', '1_000_000 is INT_LITERAL');
});

// ──────────────────────────────────────────
// Test 26: export fn keyword
// ──────────────────────────────────────────
runSuite('Export keyword', () => {
    const r = scanDocument(`
export fn calculate(int x, int y) -> int {
    return x + y
}
export fn main() {
    calculate(1, 2)
}
`);
    const exportTokens = r.tokens.filter(t => t.type === 'EXPORT');
    assert(exportTokens.length >= 2, `has at least 2 EXPORT tokens (got ${exportTokens.length})`);
    const calcSym = r.symbols.find(s => s.name === 'calculate');
    assert(calcSym !== undefined, 'calculate function symbol found');
    assert(calcSym!.kind === 'function', `calculate kind is function, got ${calcSym!.kind}`);
    assert(calcSym!.type_name === 'int', `calculate return type is int, got ${calcSym!.type_name}`);
});

// ──────────────────────────────────────────
// Test 27: @system attribute on includes
// ──────────────────────────────────────────
runSuite('@system include attribute', () => {
    const r = scanDocument('include "math.h" @system and link m');
    const includeTok = r.tokens.find(t => t.lexeme === 'include');
    const strTok = r.tokens.find(t => t.lexeme === '"math.h"');
    const atTok = r.tokens.find(t => t.lexeme === '@');
    const sysTok = r.tokens.find(t => t.lexeme === 'system');
    const andTok = r.tokens.find(t => t.lexeme === 'and');
    const linkTok = r.tokens.find(t => t.lexeme === 'link');

    assert(includeTok !== undefined, 'include token exists');
    assert(includeTok!.type === 'INCLUDE', 'include type is INCLUDE');
    assert(strTok !== undefined, 'string token exists');
    assert(atTok !== undefined, '@ token exists');
    assert(sysTok !== undefined, 'system token exists');
    assert(andTok !== undefined, 'and token exists');
    assert(linkTok !== undefined, 'link token exists');

    // Test without @system
    const r2 = scanDocument('include "local.h" and link m');
    const atTok2 = r2.tokens.find(t => t.lexeme === '@');
    assert(atTok2 === undefined, 'no @ token without @system');
    assert(r2.errors.length === 0, 'no errors without @system');
});

// ──────────────────────────────────────────
// Test 28: $macro(args) explicit macro call
// ──────────────────────────────────────────
runSuite('$macro(args) explicit macro call', () => {
    const r = scanDocument(`
macro gen_getter(name, type) {
    emit { fn $name() -> $type { return $field } }
}

fn main() {
    gen_getter(value, int)
    $gen_getter(value, int)
}
`);
    // Check for $name in macro body
    const dollarTokens = r.tokens.filter(t => t.type === 'DOLLAR_IDENTIFIER');
    assert(dollarTokens.length >= 2, `has DOLLAR_IDENTIFIER tokens (got ${dollarTokens.length})`);

    // Check that macro keyword is recognized
    assert(r.tokens.some(t => t.type === 'MACRO'), 'macro keyword found');
    assert(r.tokens.some(t => t.type === 'MACRO' && t.lexeme === 'macro'), 'macro keyword lexeme matches');

    // Check that $name followed by ( on next line works
    const dollarGenGetter = r.tokens.find(t => t.lexeme === '$gen_getter');
    assert(dollarGenGetter !== undefined, '$gen_getter dollar identifier found');
});

// ──────────────────────────────────────────
// Test 29: Union declarations
// ──────────────────────────────────────────
runSuite('Union declarations', () => {
    const r = scanDocument(`
union Data {
    int i
    float f
    bool b
}
struct Packet {
    int id
    union {
        int x
        float y
    }
}
`);
    assert(r.structs.has('Packet'), 'Packet struct found');
    const packet = r.structs.get('Packet')!;
    assert(packet.fields.some(f => f.name === 'id'), 'id field found');
    assert(packet.fields.some(f => f.name === 'x'), 'anon union x found');
    assert(packet.fields.some(f => f.name === 'y'), 'anon union y found');
    const unionSym = r.symbols.find(s => s.name === 'Data');
    assert(unionSym !== undefined, 'Data union symbol found');
    assert(unionSym!.kind === 'struct', `Data kind is struct, got ${unionSym!.kind}`);
});

// ──────────────────────────────────────────
// Test 30: impl block
// ──────────────────────────────────────────
runSuite('Impl block', () => {
    const r = scanDocument(`
struct Arrow { int damage }

impl Arrow : Damageable {
    fn take_damage(int d) {
        damage = d
    }
}
`);
    const implSym = r.symbols.find(s => s.name === 'take_damage');
    assert(implSym !== undefined, 'take_damage method found in impl');
    assert(implSym!.kind === 'function', `take_damage kind is function, got ${implSym!.kind}`);
});

// ──────────────────────────────────────────
// Test 31: Type aliases
// ──────────────────────────────────────────
runSuite('Type aliases', () => {
    const r = scanDocument('type MyInt = int\ntype Coord = f64\ntype Color = u32');
    const myInt = r.symbols.find(s => s.name === 'MyInt');
    const coord = r.symbols.find(s => s.name === 'Coord');
    const color = r.symbols.find(s => s.name === 'Color');
    assert(myInt !== undefined, 'MyInt type alias found');
    assert(coord !== undefined, 'Coord type alias found');
    assert(color !== undefined, 'Color type alias found');
    assert(myInt!.kind === 'type', `MyInt kind is type, got ${myInt!.kind}`);
    assert(myInt!.type_name === 'int', `MyInt type is int, got ${myInt!.type_name}`);
});

// ──────────────────────────────────────────
// Test 32: Dynamic arrays T[] inside struct fields
// ──────────────────────────────────────────
runSuite('Dynamic arrays in struct fields', () => {
    const r = scanDocument(`
struct Container {
    int[] items
    String[] names
    float[] values
}
`);
    const c = r.structs.get('Container');
    assert(c !== undefined, 'Container struct found');
    if (c) {
        assert(c.fields.some(f => f.name === 'items' && f.type === 'int[]'), 'int[] items found');
        assert(c.fields.some(f => f.name === 'names' && f.type === 'String[]'), 'String[] names found');
        assert(c.fields.some(f => f.name === 'values' && f.type === 'float[]'), 'float[] values found');
    }
});

// ──────────────────────────────────────────
// Test 33: Bitfield types (u4, i3, u24, etc.)
// ──────────────────────────────────────────
runSuite('Bitfield types', () => {
    const r = scanDocument(`
struct Flags {
    u4  low_nibble
    i3  signed_3bit
    u1  single_bit
    u24 address_part
}
`);
    const flags = r.structs.get('Flags');
    assert(flags !== undefined, 'Flags struct found');
    if (flags) {
        assert(flags.fields.some(f => f.name === 'low_nibble' && f.type === 'u4'), 'u4 low_nibble found');
        assert(flags.fields.some(f => f.name === 'signed_3bit' && f.type === 'i3'), 'i3 signed_3bit found');
        assert(flags.fields.some(f => f.name === 'single_bit' && f.type === 'u1'), 'u1 single_bit found');
        assert(flags.fields.some(f => f.name === 'address_part' && f.type === 'u24'), 'u24 address_part found');
    }
});

// ──────────────────────────────────────────
// Test 34: @packed and @align struct attributes
// ──────────────────────────────────────────
runSuite('Struct attributes @packed @align', () => {
    const r = scanDocument(`
struct Packed @packed {
    u8 a
    i32 b
}
struct Aligned @align(64) {
    u8 a
    i32 b
}
struct Both @packed @align(16) {
    u8 x
    i64 y
}
`);
    assert(r.structs.has('Packed'), 'Packed struct found');
    assert(r.structs.has('Aligned'), 'Aligned struct found');
    assert(r.structs.has('Both'), 'Both struct found');
});

// ──────────────────────────────────────────
// Test 35: Anonymous struct inside union inside struct
// ──────────────────────────────────────────
runSuite('Anonymous struct inside union inside struct', () => {
    const r = scanDocument(`
struct TaggedPacket {
    u32 raw
    union {
        struct { u24 addr; u8 size; }
        u32 combined
    }
}
`);
    const tp = r.structs.get('TaggedPacket');
    assert(tp !== undefined, 'TaggedPacket struct found');
    if (tp) {
        assert(tp.fields.some(f => f.name === 'raw'), 'raw field found');
        assert(tp.fields.some(f => f.name === 'addr'), 'addr field (from anon struct) found');
        assert(tp.fields.some(f => f.name === 'size'), 'size field (from anon struct) found');
        assert(tp.fields.some(f => f.name === 'combined'), 'combined field (from anon union) found');
    }
});

// ──────────────────────────────────────────
// Test 36: Variable detection with new patterns
// ──────────────────────────────────────────
runSuite('Variable detection new patterns', () => {
    const r = scanDocument(`
fn test() {
    int[10] fixed  // fixed array variable
    *u8 ptr        // pointer variable
}
`);
    // Fixed array variable
    assert(r.symbols.some(s => s.name === 'fixed' && s.kind === 'variable'), 'fixed variable found');
    // Pointer variable
    assert(r.symbols.some(s => s.name === 'ptr' && s.kind === 'variable' && s.type_name === '*u8'), 'ptr variable found');
});

// ──────────────────────────────────────────
// Test 37: and/or/not logical keywords
// ──────────────────────────────────────────
runSuite('Logical keywords and or not', () => {
    const r = scanDocument('if a and b { }\nif a or b { }\nif not a { }');
    const andTok = r.tokens.find(t => t.lexeme === 'and');
    const orTok = r.tokens.find(t => t.lexeme === 'or');
    const notTok = r.tokens.find(t => t.lexeme === 'not');
    assert(andTok !== undefined, 'and token found');
    assert(orTok !== undefined, 'or token found');
    assert(notTok !== undefined, 'not token found');
});

// ──────────────────────────────────────────
// Test 38: short, long, double aliases
// ──────────────────────────────────────────
runSuite('Type aliases short long double', () => {
    const r = scanDocument('short x = 5\nlong y = 10\ndouble z = 3.14');
    const shortTok = r.tokens.find(t => t.lexeme === 'short');
    const longTok = r.tokens.find(t => t.lexeme === 'long');
    const doubleTok = r.tokens.find(t => t.lexeme === 'double');
    assert(shortTok !== undefined && shortTok.type !== 'IDENTIFIER', 'short is keyword');
    assert(longTok !== undefined && longTok.type !== 'IDENTIFIER', 'long is keyword');
    assert(doubleTok !== undefined && doubleTok.type !== 'IDENTIFIER', 'double is keyword');
});

// ──────────────────────────────────────────
// Summary
// ──────────────────────────────────────────
console.log(`\n═══════════════════════════════════`);
console.log(`Results: ${passed} passed, ${failed} failed${failed > 0 ? ' ❌' : ' ✅'}`);
console.log(`═══════════════════════════════════\n`);

process.exit(failed > 0 ? 1 : 0);
