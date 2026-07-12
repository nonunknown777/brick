#!/usr/bin/env python3
"""
Convert Brick Markdown documentation to HTML.
Usage: python3 docs/md2html.py < input.md > output.html
"""

import sys
import re

def md_to_html(md_text):
    lines = md_text.split('\n')
    html = []
    in_code_block = False
    in_table = False
    in_list = False
    in_blockquote = False

    html.append('<!DOCTYPE html>')
    html.append('<html lang="en">')
    html.append('<head>')
    html.append('<meta charset="UTF-8">')
    html.append('<meta name="viewport" content="width=device-width, initial-scale=1.0">')
    html.append('<title>Brick Documentation</title>')
    html.append('<link rel="stylesheet" href="doc-style.css">')
    html.append('</head>')
    html.append('<body>')
    html.append('<div class="doc-wrapper">')
    html.append('<div class="doc-content">')

    i = 0
    while i < len(lines):
        line = lines[i]

        # Code blocks
        if line.startswith('```'):
            if in_code_block:
                html.append('</code></pre>')
                in_code_block = False
            else:
                lang = line[3:].strip()
                html.append(f'<pre><code class="language-{lang}">')
                in_code_block = True
            i += 1
            continue

        if in_code_block:
            escaped = (line
                .replace('&', '&amp;')
                .replace('<', '&lt;')
                .replace('>', '&gt;'))
            html.append(escaped + '\n')
            i += 1
            continue

        # Empty line
        if not line.strip():
            if in_table:
                html.append('</tbody></table>')
                in_table = False
            if in_list:
                html.append('</ul>')
                in_list = False
            if in_blockquote:
                html.append('</blockquote>')
                in_blockquote = False
            html.append('')
            i += 1
            continue

        # Tables
        if '|' in line and line.strip().startswith('|'):
            if not in_table:
                html.append('<table><thead>')
                in_table = True

            cells = [c.strip() for c in line.split('|')[1:-1]]
            is_header = all(re.match(r'^:?-+:?$', c) for c in cells if c)

            if is_header:
                html.append('</thead><tbody>')
                i += 1
                continue

            tag = 'th' if '---' not in lines[i-1] else 'td'
            row = '<tr>' + ''.join(f'<{tag}>{c}</{tag}>' for c in cells) + '</tr>'
            html.append(row)
            i += 1
            continue

        if in_table:
            html.append('</tbody></table>')
            in_table = False

        # Blockquotes
        if line.startswith('> '):
            if not in_blockquote:
                html.append('<blockquote>')
                in_blockquote = True
            html.append(f'<p>{line[2:]}</p>')
            i += 1
            continue

        if in_blockquote:
            html.append('</blockquote>')
            in_blockquote = False

        # Headers
        if line.startswith('#'):
            level = len(re.match(r'^#+', line).group())
            text = line[level:].strip()
            html.append(f'<h{level}>{text}</h{level}>')
            i += 1
            continue

        # Horizontal rule
        if line.strip() in ('---', '***', '___'):
            html.append('<hr>')
            i += 1
            continue

        # Unordered lists
        if line.strip().startswith('- ') or line.strip().startswith('* '):
            if not in_list:
                html.append('<ul>')
                in_list = True
            text = line.strip()[2:]
            html.append(f'<li>{text}</li>')
            i += 1
            continue

        if in_list:
            html.append('</ul>')
            in_list = False

        # Inline formatting
        text = line.strip()
        text = re.sub(r'\*\*(.+?)\*\*', r'<strong>\1</strong>', text)
        text = re.sub(r'\*(.+?)\*', r'<em>\1</em>', text)
        text = re.sub(r'`(.+?)`', r'<code>\1</code>', text)
        text = re.sub(r'\[(.+?)\]\((.+?)\)', r'<a href="\2">\1</a>', text)

        if text:
            html.append(f'<p>{text}</p>')

        i += 1

    if in_code_block:
        html.append('</code></pre>')
    if in_table:
        html.append('</tbody></table>')
    if in_list:
        html.append('</ul>')
    if in_blockquote:
        html.append('</blockquote>')

    html.append('</div>')  # doc-content
    html.append('</div>')  # doc-wrapper
    html.append('</body>')
    html.append('</html>')
    return '\n'.join(html)

if __name__ == '__main__':
    text = sys.stdin.read()
    html = md_to_html(text)
    sys.stdout.write(html)
