"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.findMetaC = findMetaC;
exports.runCompiler = runCompiler;
const child_process_1 = require("child_process");
function findMetaC() {
    const candidates = [
        'meta-c',
        './build/meta-c',
        '../build/meta-c',
        '../../build/meta-c',
        '/usr/local/bin/meta-c',
    ];
    for (const cmd of candidates) {
        try {
            (0, child_process_1.execSync)(`${cmd} --help`, { encoding: 'utf8', stdio: 'pipe' });
            return cmd;
        }
        catch {
            continue;
        }
    }
    return 'meta-c';
}
function runCompiler(filePath) {
    const cmd = findMetaC();
    try {
        const raw = (0, child_process_1.execSync)(`"${cmd}" "${filePath}" --lsp`, {
            encoding: 'utf8',
            stdio: ['ignore', 'pipe', 'pipe'],
            timeout: 30000,
        });
        const output = JSON.parse(raw);
        return { success: true, output, raw };
    }
    catch (err) {
        if (err instanceof Error && 'stdout' in err) {
            const execErr = err;
            if (execErr.stdout) {
                try {
                    const output = JSON.parse(execErr.stdout);
                    return { success: false, output, raw: execErr.stdout };
                }
                catch {
                    // Not JSON output
                }
            }
            return {
                success: false,
                output: { tokens: [], symbols: [], errors: [{ message: execErr.stderr || err.message, line: 0, col: 0, file: filePath, severity: 1 }] },
                raw: execErr.stderr || '',
            };
        }
        return {
            success: false,
            output: { tokens: [], symbols: [], errors: [{ message: String(err), line: 0, col: 0, file: filePath, severity: 1 }] },
            raw: String(err),
        };
    }
}
//# sourceMappingURL=compilerService.js.map