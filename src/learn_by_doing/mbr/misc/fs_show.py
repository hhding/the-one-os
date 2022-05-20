#!/usr/bin/env python3
import sys
import struct
import argparse

SECTOR_SIZE = 512

def hexdump(file_path, start_addr, line_cnt=1, bytes_per_line=16, show_ascii=True):
    with open(file_path, "rb") as f:
        f.seek(start_addr)
        for i in range(line_cnt):
            data = f.read(bytes_per_line)
            addr = start_addr + i * bytes_per_line
            line_info = [f"{addr:08x}: "]
            ascii_data = ''
            for index, c in enumerate(data):
                if(c >=32 and c<=126):
                    ascii_data += chr(c)
                else:
                    ascii_data += "."

                line_info.append(f"{c:02x}")
                if index % 2 == 1:
                    line_info.append(" ")
            print("".join(line_info), ascii_data)

class Inode:
    def __init__(self, fields, disk_path):
        self.inode_no = fields[0]
        self.size = fields[1]
        self.open_cnt = fields[2]
        self.write_deny = fields[3]
        self.sectors = fields[4:-1]
        self.disk_path = disk_path

    def dents(self):
        for lba in self.sectors:
            if lba > 0:
                print(f"> Dump Inode dents for inode #{self.inode_no}:")
                with open(self.disk_path, "rb") as f:
                    f.seek(lba * SECTOR_SIZE)
                    for idx in range(0, SECTOR_SIZE, 24):
                        filename, i_no, f_type = struct.unpack("16sII", f.read(24))
                        if f_type == 0:
                            break
                        print(f">>>> i_no:{i_no}, type:{f_type}, name:{filename.decode()}")


    def __repr__(self):
        return f"Inode<no:{self.inode_no}, size:{self.size},data:{self.sectors}>"

class InodeTable:
    def __init__(self, file_path, offset):
        self.path = file_path
        self.offset = offset
        self.fp = open(self.path, 'rb')
        self.fp.seek(offset)
        self.inodes = dict()
        self.fetch_all()

    def fetch_all(self):
        fields = struct.unpack("18I", self.fp.read(18*4))
        inode = Inode(fields, self.path)
        assert inode.inode_no == 0
        self.inodes[inode.inode_no] = inode
        print(inode)

        while True:
            fields = struct.unpack("18I", self.fp.read(18*4))
            inode = Inode(fields, self.path)
            if inode.inode_no > 0:
                if inode.inode_no in self.inodes:
                    print(f"abort on duplicated inode: {inode.inode_no}")
                    return
                self.inodes[inode.inode_no] = inode
            else:
                return

    def walk(self):
        root_inode = self.inodes[0]
        


class SuperBlock:
    sb_size = 13*4
    def __init__(self, data):
        fields = struct.unpack("13I", data)
        self.magic = fields[0]
        self.sec_cnt = fields[1]
        self.inode_cnt = fields[2]
        self.part_lba_base = fields[3]
        self.block_bitmap_lba = fields[4]
        self.block_bitmap_sects = fields[5]
        self.inode_bitmap_lba = fields[6]
        self.inode_bitmap_sects = fields[7]
        self.inode_table_lba = fields[8]
        self.inode_table_sects = fields[9]
        self.data_start_lba = fields[10]
        self.root_inode_no = fields[11]
        self.dir_entry_size = fields[12]

    def __repr__(self) -> str:
        return f"Superblock<magic:{hex(self.magic)} inode_table:{self.inode_table_lba} data:{self.data_start_lba}>"
    
class Partition:
    table_size = 16
    def __init__(self, index, partition_data, disk_path):
        fields = struct.unpack("8BII", partition_data)
        self.bootable = fields[0]
        self.start_head = fields[1] 
        self.start_sec = fields[2] 
        self.start_chs = fields[3] 
        self.fs_type = fields[4] 
        self.end_head = fields[5] 
        self.end_sec = fields[6] 
        self.enc_chs = fields[7] 
        self.start_lba = fields[8] 
        self.sec_cnt = fields[9] 
        self.name = f"part{index}"
        self.disk_path = disk_path
        self.superblock = None
        self.init_superblock()
        self.inode_table = InodeTable(disk_path, self.superblock.inode_table_lba*SECTOR_SIZE)

    def init_superblock(self):
        with open(self.disk_path, "rb") as f:
            f.seek(SECTOR_SIZE* (self.start_lba + 1))
            sb_data = f.read(SuperBlock.sb_size)
            self.superblock = SuperBlock(sb_data)

    def show_superblock(self):
        sb = self.superblock
        print("Superblock information:")
        print(f"    magic: {hex(sb.magic)}")
        print(f"    inode_cnt: {sb.inode_cnt}")
        print(f"    block_bitmap: {sb.block_bitmap_lba}")
        hexdump(self.disk_path, sb.block_bitmap_lba*SECTOR_SIZE)
        print(f"    inode_bitmap: {sb.inode_bitmap_lba}")
        hexdump(self.disk_path, sb.inode_bitmap_lba*SECTOR_SIZE)
        print(f"    inode_table: {sb.inode_table_lba}")
        self.inode_table.inodes[0].dents()
        print(f"    data: {sb.data_start_lba}")
        print(f"    root_inode: {sb.root_inode_no}")
        print(f"    dir_size: {sb.dir_entry_size}")
    
    def __repr__(self):
        return f"{self.name} fs:{hex(self.fs_type)} start:{self.start_lba} sector:{self.sec_cnt}"

class DiskManager:
    partition_offset = 446
    partition_cnt = 4
    def __init__(self, disk_path):
        self.disk_path = disk_path
        self.partitions = dict()
        with open(disk_path, 'rb') as f:
            boot_sector = f.read(512)
            assert boot_sector[-1] == 0xaa and boot_sector[-2] == 0x55
            self.boot_sector = boot_sector
            for part_idx in range(self.partition_cnt):
                start = self.partition_offset + part_idx * Partition.table_size
                end = start + Partition.table_size
                partition = Partition(part_idx + 1, boot_sector[start:end], self.disk_path)
                self.partitions[part_idx + 1] = partition

    def show_partitions(self):
        print("Partition information:")
        for part in self.partitions.values():
            print(f"    {part}")

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--disk_path", default="bochs/disk1")
    parser.add_argument("--part", type=int, default=1)
    parser.add_argument("--show_superblock", action='store_true')
    parser.add_argument("--show_partitions", action='store_true')
    return parser.parse_args()

def main():
    args = parse_args()
    dm = DiskManager(args.disk_path)
    if args.show_partitions:
        dm.show_partitions()

    p = dm.partitions[args.part]
    if args.show_superblock:
        p.show_superblock()

if __name__ == '__main__':
    main()
