#!/usr/bin/env python3

import sys
import struct

if len(sys.argv) != 2:
    print(sys.argv[0], "elf_file_path")

def get_data(f, count, fmt):
    data = f.read(count)
    if fmt:
        data = hex(struct.unpack(fmt, data)[0])
    return data

elf_header = [
    ["magic", 4, ''],
    ["e_ident3", 3, ''],
    ["e_ident9", 9, ''],
    ["e_type", 2, 'H'],
    ["e_machine", 2, 'H'],
    ["e_version", 4, 'I'],
    ["e_entry", 4, 'I'],
    ["e_phoff", 4, 'I'],
    ["e_shoff", 4, 'I'],
    ["e_flags", 4, 'I'],
    ["e_ehsize", 2, 'H'],
    ["e_phentsize", 2, 'H'],
    ["e_phnum", 2, 'H'],
    ["e_shentsize", 2, 'H'],
    ["e_shnum", 2, 'H'],
    ["e_shstrndx", 2, 'H'],
]

elf_program_header = [
    ["p_type", 4, 'I'],
    ["p_offset", 4, 'I'],
    ["p_vaddr", 4, 'I'],
    ["p_paddr", 4, 'I'],
    ["p_filesz", 4, 'I'],
    ["p_memsz", 4, 'I'],
    ["p_flags", 4, 'I'],
    ["p_align", 4, 'I'],
]

def pretty_table(fields_list):
    last_field_len = [len(field) for field in fields_list[0]]
    for fields in fields_list:
        for index, field in enumerate(fields):
            last_len = len(field)
            if last_field_len[index] < last_len:
                last_field_len[index] = last_len
    fmt = " ".join(["{{:>{}s}}".format(l) for l in last_field_len])
    for fields in fields_list:
        print(fmt.format(*fields))


file_name = sys.argv[1]
summary_info = '''========== ELF 格式总体说明 =========
# ==> ELF 头部分
#   ELF 头（长度 0x34）
#
# ==> Program 头部分
#   Program 头1（长度 0x20)
#   ......
#   Program 头N（长度 0x20)
#
# ==> Section 部分
#   Section 1
#   ......
#   Section N
#
# ==> Section 头部分
#   Section 头 1
#   ......
#   Section 头 N
#
# 多个 Section 汇聚成一个 Program（段）
# 然后加载到某个地址段
# 所以 Program 段一般少于 Section 数量
# 对加载程序而言，可以忽略 Section 而只处理 Program。
'''

print(summary_info)
with open(file_name, "rb") as f:
    print("================ ELF header =========================")
    for note, cnt, fmt in elf_header:
        offset = f.tell()
        value = get_data(f, cnt, fmt)
        print(hex(offset), note, value)

    print("================ Program header =========================")
    ph_headers = ["offset"] + [h[0] for h in elf_program_header]
    ph_list = [ph_headers]
    for i in range(7):
        fields = [hex(f.tell())]
        for note, cnt, fmt in elf_program_header:
            value = get_data(f, cnt, fmt)
            fields.append(value)
        ph_list.append(fields)
    pretty_table(ph_list)
    

