#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Writes the .bgg reader's test cases from base.bgg (a genetic code the 0.9
jar wrote): the same file in other forms an XML parser treats as equal,
numbers in and out of Java's grammar, and files 0.9 refuses. expected.txt
holds what 0.9 reads from each (tools/javaref/BggInterop.java).

    tests/bgg_cases/make_cases.py tests/bgg_cases/base.bgg tests/bgg_cases
"""
import re, sys
from pathlib import Path
base = Path(sys.argv[1]).read_text()
out = Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
def w(name, text, enc="utf-8"):
    (out / f"{name}.bgg").write_bytes(text.encode(enc) if isinstance(text, str) else text)
head, body = base.split("]>\n", 1)
dtd = head + "]>\n"
first_gene = re.search(r'\t<gene length="([^"]*)" theta="([^"]*)" color="([a-z]*)" />', base)
def gene1(length=None, theta=None, color=None, extra=""):
    L, T, C = first_gene.groups()
    g = f'\t<gene length="{L if length is None else length}" theta="{T if theta is None else theta}" color="{C if color is None else color}"{extra} />'
    return base.replace(first_gene.group(0), g, 1)
def root(attrs):
    return re.sub(r'<genetic_code [^>]*>', f'<genetic_code {attrs}>', base, count=1)
sym = re.search(r'symmetry="(\d)"', base).group(1)
# layout and encoding
w("crlf", base.replace("\n", "\r\n"))
w("bom_utf8", b"\xef\xbb\xbf" + base.encode())
w("utf16le_bom", base.replace('encoding="UTF-8"', 'encoding="UTF-16"'), "utf-16")
w("utf16be_bom", b"\xfe\xff" + base.replace('encoding="UTF-8"', 'encoding="UTF-16"').encode("utf-16-be"))
w("no_xml_declaration", base.split("\n", 1)[1])
w("single_quotes", re.sub(r'="([^"]*)"', r"='\1'", body).join([dtd, ""]) if False else dtd + re.sub(r'="([^"]*)"', r"='\1'", body))
w("attribute_order", dtd + re.sub(r'<gene length="([^"]*)" theta="([^"]*)" color="([a-z]*)" />', r'<gene color="\3" theta="\2" length="\1"/>', body).replace(f'<genetic_code symmetry="{sym}"', f'<genetic_code').replace('disperse=', f'symmetry="{sym}" disperse=', 1))
w("whitespace_in_tags", dtd + re.sub(r'<gene length=', '<gene\n\t\tlength = ', body).replace('" />', '"\n\t/>'))
w("comments", base.replace("\t<gene", "\t<!-- a gene -->\n\t<gene").replace("<genetic_code ", "<!-- before -->\n<genetic_code ", 1))
w("processing_instruction", base.replace("\t<gene", "\t<?note x?>\n\t<gene", 1))
w("explicit_close", base.replace('" />', '"></gene>'))
w("tabs_newlines_in_value", gene1(length="\t5.5\n"))
w("char_reference", gene1(length="&#53;.&#x35;", color="&#x72;ed"))
w("predefined_entity_value", gene1(extra=""))
w("dtd_entity", base.replace("]>", '<!ENTITY len "5.5">\n]>', 1).replace(first_gene.group(0), first_gene.group(0).replace(f'length="{first_gene.group(1)}"', 'length="&len;"'), 1))
w("dtd_default_attribute", base.replace("<!ATTLIST genetic_code disperse (yes|no) #REQUIRED>", '<!ATTLIST genetic_code disperse (yes|no) "no">').replace(re.search(r' disperse="(yes|no)"', base).group(0), "", 1))
w("long_value", gene1(length="5." + "0" * 120 + "1"))
w("enum_spaces", root(f'symmetry=" {sym} " mirror=" no " disperse="no "').replace(first_gene.group(0), first_gene.group(0).replace(f'color="{first_gene.group(3)}"', f'color=" {first_gene.group(3)} "'), 1))
cm_attrs = ["mutationrate", "activity", "modifiescream", "plague", "generation", "sibling", "altruist", "social", "peaceful"]
cm_dtd = dtd.replace("<!ATTLIST genetic_code mirror", "".join(f"<!ATTLIST genetic_code {a} CDATA #REQUIRED>\n" for a in cm_attrs) + "<!ATTLIST genetic_code mirror").replace("<!ATTLIST gene color", "".join(f"<!ATTLIST gene {a}reaction CDATA #REQUIRED>\n" for a in ["red", "green", "blue", "white", "gray", "plant"]) + "<!ATTLIST gene color")
cm_body = body.replace("<genetic_code ", "<genetic_code " + " ".join(f'{a}="1"' for a in cm_attrs) + " ", 1).replace('" />', '" ' + " ".join(f'{a}reaction="0"' for a in ["red", "green", "blue", "white", "gray", "plant"]) + ' />')
w("many_declared_attributes", cm_dtd + cm_body)
# numbers in the first gene's length
nums = {"plus": "+5", "trailing_dot": "5.", "leading_dot": ".5", "exp": "5e0", "exp_plus": "5E+0",
        "hex_float": "0x1.4p2", "hex_upper": "0X1P2", "hex_no_exponent": "0x10", "suffix_d": "5d", "suffix_f": "5f",
        "suffix_upper": "5D", "suffix_after_space": "5 d", "spaces_around": " 5 ", "suffix_then_space": "5d ",
        "infinity": "Infinity", "plus_infinity": "+Infinity", "minus_infinity": "-Infinity", "nan": "NaN",
        "minus_nan": "-NaN", "c_inf": "inf", "c_INF": "INF", "c_nan": "nan", "c_infinity": "infinity",
        "c_nan_payload": "nan(1)", "exp_no_digits": "1e", "only_exp": "e5", "empty": "", "blank": " ",
        "two_dots": "5..0", "underscore": "1_000", "arabic_digit": "٥", "hex_suffix": "0x1.8p1d",
        "overflow": "1e400", "underflow": "1e-400", "subnormal_boundary": "2.4703282292062328E-324",
        "just_above_boundary": "2.4703282292062329E-324", "many_digits": "0.1000000000000000055511151231257827021181583404541015625",
        "halfway": "9007199254740993", "minus_zero": "-0.0", "hex_no_digits": "0x.p1", "hex_no_exp_digits": "0x1p",
        "leading_zeros": "0005.5"}
for k, v in nums.items():
    w("number_" + k, gene1(length=v))
# root attributes
for k, v in {"sym_space": f' {sym}', "sym_plus": f'+{sym}', "sym_zero_pad": f'0{sym}', "sym_9": "9", "sym_0": "0"}.items():
    w(k, root(f'symmetry="{v}" mirror="no" disperse="no"'))
w("mirror_capital", root(f'symmetry="{sym}" mirror="Yes" disperse="no"'))
w("color_capital", gene1(color="Red"))
w("color_mod_colour", gene1(color="forest"))
# structure 0.9 refuses
w("no_doctype", base.replace(dtd.split("\n", 1)[1], "", 1))
w("text_between_genes", base.replace("\t<gene", "\thello\n\t<gene", 1))
w("unknown_element", base.replace("\t<gene", "\t<segment />\n\t<gene", 1))
w("nested_in_gene", base.replace('" />', '"><gene length="1" theta="1" color="red" /></gene>', 1))
w("zero_genes", dtd + re.sub(r"\t<gene[^\n]*\n", "", body))
w("trailing_content", base + "junk\n")
w("missing_attribute", gene1().replace(' theta="', ' thetx="', 1))
w("duplicate_attribute", gene1(extra=' length="1.0"'))
w("unclosed_root", base.replace("</genetic_code>", ""))
w("truncated", base[: len(base) // 2])
