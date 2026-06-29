#!/usr/bin/env python3
"""Convert .md docs to .html pages matching Brick's style."""

import json
import os
import re
import sys
from pathlib import Path

from markdown_it import MarkdownIt
from pygments import highlight
from pygments.lexers import get_lexer_by_name
from pygments.formatters import HtmlFormatter

DOCS = Path(__file__).parent

TEMPLATE = """<!DOCTYPE html>
<html lang="{LANG}">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>{TITLE} — Brick</title>
<meta name="description" content="{DESC}">
<link rel="stylesheet" href="style.css">
<link rel="stylesheet" href="doc-style.css">
</head>
<body>

<nav class="doc-nav">
  <div class="nav-inner">
    <a href="index.html" class="nav-logo">Brick</a>
    <div class="nav-links">
      <a href="LANGUAGE.html">Language</a>
      <a href="MACROS.html">Macros</a>
      <a href="ARCHITECTURE.html">Architecture</a>
      <a href="GETTING_STARTED.html">Getting Started</a>
      <a href="OPTIMIZATIONS.html">Optimizations</a>
      <a href="hot-reload.html">Hot Reload</a>
      <a href="index.html" class="nav-back">← Home</a>
    </div>
  </div>
</nav>

<main class="doc-main">
  <div class="container doc-content">
{CONTENT}
  </div>
</main>

<footer>
  <p>Brick — MIT License</p>
  <p><a href="https://github.com/nonunknown777/brick">github.com/nonunknown777/brick</a></p>
</footer>

</body>
</html>"""

def make_title(md_file: str) -> str:
    base = md_file.replace('.md', '').replace('.pt-BR', '')
    names = {
        'LANGUAGE': 'Language Reference',
        'MACROS': 'Macro System',
        'ARCHITECTURE': 'Architecture',
        'GETTING_STARTED': 'Getting Started',
        'OPTIMIZATIONS': 'Optimizations',
        'hot-reload': 'Hot Reload',
    }
    return names.get(base, base.replace('-', ' ').title())

def code_fence(self, tokens, idx, options, env):
    token = tokens[idx]
    info = token.info.strip() if token.info else ''
    lang = info.split()[0] if info else ''
    code = token.content

    css_class = ''
    if lang == 'brick':
        css_class = 'language-brick'
        lang = ''
    elif lang:
        css_class = f'language-{lang}'

    if lang:
        try:
            lexer = get_lexer_by_name(lang, stripall=True)
            formatter = HtmlFormatter(style='monokai', noclasses=True)
            highlighted = highlight(code, lexer, formatter)
            return f'<div class="code-block"><pre class="{css_class}">{highlighted}</pre></div>\n'
        except:
            pass

    escaped = (code.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;'))
    return f'<div class="code-block"><pre class="{css_class}">{escaped}</pre></div>\n'


TABLE_RE = re.compile(r'^\|(.+)\|$', re.MULTILINE)
SEP_RE = re.compile(r'^[\s\|:-]+$')

def convert_tables(md_text):
    """Convert markdown tables to HTML tables."""
    lines = md_text.split('\n')
    out = []
    i = 0
    while i < len(lines):
        line = lines[i]
        m = TABLE_RE.match(line)
        if m and i + 1 < len(lines) and SEP_RE.match(lines[i+1].strip()):
            headers = [c.strip() for c in m.group(1).split('|')]
            rows = []
            i += 2
            while i < len(lines):
                m2 = TABLE_RE.match(lines[i])
                if not m2:
                    break
                cells = [c.strip() for c in m2.group(1).split('|')]
                rows.append(cells)
                i += 1
            html = '<table>\n<thead>\n<tr>'
            for h in headers:
                html += f'<th>{h}</th>'
            html += '</tr>\n</thead>\n<tbody>\n'
            for row in rows:
                html += '<tr>'
                for c in row:
                    html += f'<td>{c}</td>'
                html += '</tr>\n'
            html += '</tbody>\n</table>\n'
            out.append(html)
        else:
            out.append(line)
            i += 1
    return '\n'.join(out)


def convert_md(source: Path):
    md_text = source.read_text(encoding='utf-8')
    md_text = convert_tables(md_text)

    md = MarkdownIt('commonmark',{'breaks':False,'html':True})
    md.add_render_rule('fence', code_fence)
    md.add_render_rule('code_block', code_fence)

    html_body = md.render(md_text)

    base = source.stem
    is_pt = '.pt-BR' in base or '.pt' in base
    lang = 'pt-BR' if is_pt else 'en'
    title = make_title(source.name)

    full = 'language specification' if 'LANGUAGE' in source.name else \
           'macro system' if 'MACRO' in source.name else \
           'architecture' if 'ARCHITECTURE' in source.name else \
           'getting started' if 'GETTING_STARTED' in source.name else \
           'optimizations' if 'OPTIMIZATIONS' in source.name else \
           'hot reload'

    desc = f'Brick {full} documentation'

    out_name = base + '.html'
    out_path = DOCS / out_name

    content = TEMPLATE.format(
        LANG=lang,
        TITLE=title,
        DESC=desc,
        CONTENT=html_body,
    )

    out_path.write_text(content, encoding='utf-8')
    print(f'  ✓ {out_name}')


def main():
    print('Converting markdown docs to HTML...')
    mds = sorted(DOCS.glob('*.md'))
    for md_file in mds:
        if md_file.name == 'md2html.py':
            continue
        if md_file.name.startswith('_'):
            continue
        convert_md(md_file)
    print('Done.')

if __name__ == '__main__':
    main()
