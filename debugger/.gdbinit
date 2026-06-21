# Meta-C GDB Configuration
# Configuracao GDB do Meta-C
# Auto-loaded from VS Code launch.json: source debugger/.gdbinit
# Carregado automaticamente do VS Code launch.json: source debugger/.gdbinit
#
# NOTE: This file uses Python to find its own path (via upward directory search)
# NOTA: Este arquivo usa Python para encontrar seu proprio caminho (via busca ascendente de diretorio)
# so it works regardless of GDB's current working directory.
# para funcionar independentemente do diretorio de trabalho atual do GDB.

python
import sys, os
print("META-C|gdbinit LOADED cwd=" + os.getcwd(), file=sys.stderr)

_find_root = os.getcwd()
_meta_c_root = None
for _ in range(20):
    _test = os.path.join(_find_root, "debugger")
    if os.path.isdir(_test) and os.path.isfile(os.path.join(_test, "gdb_commands.py")):
        _meta_c_root = _find_root
        break
    _parent = os.path.dirname(_find_root)
    if _parent == _find_root:
        break
    _find_root = _parent

if _meta_c_root:
    _debugger_dir = os.path.join(_meta_c_root, "debugger")
    print("META-C|gdbinit ROOT=" + _meta_c_root, file=sys.stderr)
    if _debugger_dir not in sys.path:
        sys.path.insert(0, _debugger_dir)
    _pp_path = os.path.join(_debugger_dir, "gdb_pretty_printers.py")
    _cmd_path = os.path.join(_debugger_dir, "gdb_commands.py")
    if os.path.isfile(_pp_path):
        exec(open(_pp_path).read())
        print("META-C|gdbinit loaded pretty_printers", file=sys.stderr)
        print("META-C|gdbinit carregou pretty_printers", file=sys.stderr)
    if os.path.isfile(_cmd_path):
        exec(open(_cmd_path).read())
        print("META-C|gdbinit loaded commands", file=sys.stderr)
        print("META-C|gdbinit carregou commands", file=sys.stderr)
else:
    print("META-C|gdbinit ROOT NOT FOUND (searched from " + os.getcwd() + ")", file=sys.stderr)
    print("META-C|gdbinit RAIZ NAO ENCONTRADA (buscou de " + os.getcwd() + ")", file=sys.stderr)
end

# General settings
# Configuracoes gerais
set print pretty on
set print vtbl on
set print object on
set print static-members off
set print address on

set pagination off
set print frame-info source-and-location
set style enabled on

# Marker to confirm .gdbinit was loaded
# Marcador para confirmar que .gdbinit foi carregado
set $_meta_c_loaded = 1

# Aliases
# Aliases
alias ib = info blocks
