# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Writes src/colour/ui/params_fields.h from Color Mod's ParamDialog.java:
every text field of the parameters dialog with its tab, row, label (in
English, from messages_en.properties), the parameter it edits and the
check checkParams makes before taking a new value. The dialog itself is
src/colour/ui/params_dialog.c."""

import os
import re

from j2c_emit import HEADER

TABS = ['General', 'World', 'Organisms', 'Metabolism', 'Genes', 'Genes2', 'Genes3']


def strip_comments(s):
    s = re.sub(r'/\*.*?\*/', '', s, flags=re.S)
    return re.sub(r'//[^\n]*', '', s)


def messages(path):
    out = {}
    for line in open(path, encoding='latin-1'):
        line = line.rstrip('\n')
        if '=' in line and not line.startswith('#'):
            k, v = line.split('=', 1)
            # Java properties: \uXXXX escapes
            v = re.sub(r'\\u([0-9a-fA-F]{4})', lambda m: chr(int(m.group(1), 16)), v)
            out[k.strip()] = v.strip()
    return out


def method_body(s, name):
    m = re.search(r'protected JPanel ' + name + r'\(\) \{', s)
    i = m.end()
    depth = 1
    j = i
    while depth:
        depth += {'{': 1, '}': -1}.get(s[j], 0)
        j += 1
    return s[i:j - 1]


def c_string(t):
    return '"' + t.replace('\\', '\\\\').replace('"', '\\"').encode('utf-8').decode('utf-8') + '"'


def generate(src_dir, out_path):
    java = strip_comments(open(os.path.join(src_dir, 'ParamDialog.java'),
                               encoding='utf-8', errors='replace').read())
    # English, falling back to the default bundle as Java's ResourceBundle does
    msg = messages(os.path.join(src_dir, 'messages', 'messages.properties'))
    msg.update(messages(os.path.join(src_dir, 'messages', 'messages_en.properties')))
    # checkParams: the condition for each field
    checks = {}
    order = []  # checkParams' order: a check may read a parameter set before it
    for m in re.finditer(r'(\w+)\.getText\(\)\);\s*if \(([^)]*)\)\s*\{?\s*Utils\.(\w+)\s*=\s*[id];',
                         java):
        var, cond, param = m.groups()
        cond = re.sub(r'\b[id]\b', 'v', cond)
        cond = re.sub(r'Utils\.(\w+)', lambda u: 'P.' + u.group(1).lower(), cond)
        checks[var] = (cond, param)
        order.append(var)
    fields = []
    for tab, name in enumerate(TABS):
        body = method_body(java, f'set{name}Tab')
        row = 0
        label = None
        pending_suffix_for = None
        for line in body.split('\n'):
            line = line.strip()
            m = re.search(r'new JLabel\(Messages\.getString\("(T_\w+)"\)', line)
            if m:
                key = m.group(1)
                if pending_suffix_for is not None and not line.startswith('label ='):
                    pass
                if pending_suffix_for is not None and key in ('T_MILLISECONDS',):
                    fields[pending_suffix_for]['suffix'] = msg.get(key, key)
                    continue
                label = msg.get(key, key)
                continue
            m = re.search(r'(\w+) = new JTextField\((Integer|Double)\.toString\(Utils\.(\w+)\)',
                          line)
            if m:
                var, kind, param = m.groups()
                if var not in checks:
                    raise SystemExit(f'ParamDialog: no check for {var}')
                cond, cparam = checks[var]
                if cparam != param:
                    raise SystemExit(f'ParamDialog: {var} edits {param} but sets {cparam}')
                fields.append(dict(tab=tab, row=row, label=label, var=var, param=param,
                                   type='i' if kind == 'Integer' else 'd', cond=cond, suffix=None))
                pending_suffix_for = len(fields) - 1
                continue
            if re.match(r'panel = new JPanel\(\)', line) or line.startswith('genesPanel.add(label)'):
                row += 1
                pending_suffix_for = None
    out = [HEADER.format(java='ParamDialog.java'),
           '/* The parameters dialog\'s fields, generated from ParamDialog.java by',
           ' * tools/colour/j2c_paramdialog.py (see src/colour/ui/params_dialog.c). */',
           '#ifndef COLOUR_PARAMS_FIELDS_H', '#define COLOUR_PARAMS_FIELDS_H', '',
           f'static const char *const k_tabs[] = {{{", ".join(c_string(msg.get("T_" + t.upper(), t)) for t in TABS)}}};',
           f'#define N_TABS {len(TABS)}',
           f'#define GENE_TABS_FROM {TABS.index("Genes")}', '',
           'static const Field k_fields[] = {']
    for f in fields:
        out.append(f'    {{{f["tab"]}, {f["row"]}, {c_string(f["label"] or "")}, '
                   f'{c_string(f["suffix"]) if f["suffix"] else "NULL"}, '
                   f'\'{f["type"]}\', offsetof(Params, {f["param"].lower()})}},')
    out.append('};')
    out.append('#define N_FIELDS ((int)(sizeof k_fields / sizeof k_fields[0]))')
    out.append('')
    out.append('/* ParamDialog.checkParams: whether a value is taken */')
    out.append('static bool field_ok(int i, double v)')
    out.append('{')
    out.append('    switch (i) {')
    for i, f in enumerate(fields):
        out.append(f'    case {i}: return {f["cond"]}; /* {f["param"]} */')
    out.append('    }')
    out.append('    return false;')
    out.append('}')
    out.append('')
    index = {f['var']: i for i, f in enumerate(fields)}
    out.append('/* the order checkParams takes them in */')
    out.append('static const int k_apply_order[] = {' +
               ', '.join(str(index[v]) for v in order if v in index) + '};')
    out.append('')
    out.append('static const char *const k_gene_tab_heads[] = {' +
               ', '.join(c_string(msg.get(k, k)) for k in ('T_COLOR2', 'T_PROBABILITY',
                                                            'T_EFFECTIVITY')) + '};')
    out.append('')
    out.append('#endif')
    with open(out_path, 'w', encoding='utf-8') as fh:
        fh.write('\n'.join(out) + '\n')
    return fields


if __name__ == '__main__':
    import sys
    fs = generate(sys.argv[1], sys.argv[2])
    print(f'{len(fs)} fields')
