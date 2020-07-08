
.code16

BOOTSEG = 0x07C0

.section .text
.globl _start

.org 0x0000
_start:
	jmp $(BOOTSEG),$_fstjmp
_fstjmp:

# Initialization
	mov $(BOOTSEG),%ax
	mov %ax,%ds
	mov %ax,%es
	xor %ax,%ax
	lss _boot_ss_desc,%sp
	
# Setting 80x25 text mode and clearing the screen.
# Screen memory starts at adress b800:0000
	mov $0x00003,%ax
	int $0x10
	
# Print hello:
	mov $0x0000,%ax
	lea _txt_hello,%si
	xor %bp,%bp
	call _prtxt

# Check 1st hard drive:
	mov $0x08,%ah  # Operation code for interrupt 13h: 8 = "get drive parameters"
	mov $0x80,%dl  # Drive no., 80h + x = "(x + 1)th hard drive"
	int $0x13      # Call...
	mov %dh,_v_max_head
	mov %cl,%dh          
	and $0x3f,%dh          # bits 0..5 only.
	mov %dh,_v_max_sector
	mov %cl,%dh
	shr $6,%dh
	and $0x03,%dh          # bits 6..7 (shifted 6) only.
	mov %ch,%cl
	mov %dh,%ch
	mov %cx,_v_max_track
	
#	call _prdrv


# Init number of sectors to read:

ASM_SECTORS = (_main_c - _start) / 512
.include "os_const.s"

	mov $(ASM_SECTORS+C_SECTORS-1),%ax  # Last sector contains 0 only.
	mov %ax,_c_sectors
	
	
# Read next sectors and put them at 07C0:0200:
#_init_next_sectors:
#	mov $0x00,%ah
#	mov $0x80,%dl
#	int $0x13
_read_next_sectors:
	mov _c_sectors,%cx
	cmp $0x0000,%cx
	jz _after_sectors_read
	
	mov _v_cur_sector,%al   # Get current sector number and
	inc %al                 # increase it by 1. If the last
	cmp _v_max_sector,%al   # sector has been reached go to
	je _max_sector_reached  # the appropriate routine, otherwise
	mov %al,_v_cur_sector   # remember new sector number and go
	jmp _read_sector        # directly to the reading routine.
_max_sector_reached:
	xor %al,%al             # Put 0 again into current
	mov %al,_v_cur_sector   # sector number and get current
	mov _v_cur_track,%ax    # path number and increase it by 1,
	inc %ax                 # and so on ...
	cmp _v_max_track,%ax    #
	ja _max_track_reached   #
	mov %ax,_v_cur_track    #
	jmp _read_sector        #
_max_track_reached:
	xor %ax,%ax           # Put 0 again ...
	mov %ax,_v_cur_track  #
	mov _v_cur_head,%al   #
	inc %al               #
	cmp _v_max_head,%al   #
	ja _max_head_reached  #
	mov %al,_v_cur_head   #
	jmp _read_sector      #
_max_head_reached:
	mov $0x0002,%ax            # This is obviously an error.
	lea _txt_diskbounderr,%si  # Print it...
	call _prtxt                #
	jmp _inflp                 # and halt.
_io_error:
	push %ax              #
	mov $0x0002,%ax       # Some error occured.
	lea _txt_ioerr,%si    # Print it...
	call _prtxt           #
	pop %ax               #
	mov %ah,%bl           # AH contains I/O error code
	mov $0x0b03,%ax       # row 4, column 10
	call _prhex           # Print hex value.
	jmp _inflp            # and halt.
	
_read_sector:
	mov $(BOOTSEG),%ax  # Set ES:BX.
	mov %ax,%es         #
	mov _c_sec_ofs,%bx  #
	push %bx            #
	
	mov $0x0201,%ax        # AH=2 is operation code ("read sectors"), AL=1 is the number of sectors
	mov _v_cur_head,%dh    # Head number.
	mov _v_cur_track,%cx   # Track number...
	xchg %ch,%cl           #
	shl $6,%cl             #
	mov _v_cur_sector,%dl  # Sector number
	inc %dl                #
	or %dl,%cl
	mov $0x80,%dl        # Drive number (1st hard drive).
	int $0x13            # Call...
	jc _io_error         # Check if any I/O error occured.
	
	mov _v_dotcol,%ah  # Print next dot.
	mov $0x01,%al      # Row no.
	lea _txt_dot,%si   #
	call _prtxt        #
	inc %ah            #
	mov %ah,_v_dotcol  #
	
	pop %bx             #
	add $0x0200,%bx     # Increase offset.
	mov %bx,_c_sec_ofs  # Remember new offset.
		
_next_sector:
	decw _c_sectors
	jmp _read_next_sectors
	
_after_sectors_read:
	mov $0x0002,%ax       # Sectors already read.
	lea _txt_secread,%si  # Print it...
	call _prtxt           #

	jmp _high_code  # Go to the code just loaded from the sectors following the boot one.
	
# infinite loop
_inflp:
	jmp _inflp


# Printing routine:
# SI register should point to the first char of a null-terminated string
# AH register - column number
# AL register - row number
_prtxt:
	push %ax  # Very initialization
	push %bx  # 
	push %es  #
	push %si  #
	push %di  #
	
	mov %ah,%bl    # Initializing offset (xxxx) of B800:xxxx
	add %bl,%bl    # Video RAM address. Offset is being
	mov $0x50,%bh  # written into DI register.
	mul %bh        # 
	add %ax,%ax    #
	xor %bh,%bh    #
	add %bx,%ax    #
	mov %ax,%di    #
	mov $0xb800,%bx  # Set destination segment (ES) to
	mov %bx,%es      # cover Video RAM.

_prtxt_char:
	mov %ds:(%si),%bl  # Check if we have reached the terminating null.
	cmp $0x00,%bl      #
	jz _prtxt_end      # If so, go to end.
	
	mov %bl,%es:(%di)  # Print char and
	add $0x0002,%di    # move the "cursor" one char right.
	inc %si            # 
	
	jmp _prtxt_char  # Next char...
	
_prtxt_end:
	pop %di  # Clean up.
	pop %si  #
	pop %es  #
	pop %bx  #
	pop %ax  #
	ret  # Return from the routine.


# Hexifying routine: AL register - value to hexify
_hexify:
	push %ax
	mov %al,%ah
	and $0x0f,%al
	and $0xf0,%ah
	ror $4,%ah
	
	cmp $0x09,%ah
	jbe _hexhi09
	add $('A'-0x0a),%ah
	jmp _hexhi0A
_hexhi09:
	add $('0'),%ah
_hexhi0A:

	cmp $0x09,%al
	jbe _hexlo09
	add $('A'-0x0a),%al
	jmp _hexlo0A
_hexlo09:
	add $'0',%al
_hexlo0A:

	mov %ah,_v_hex_hi
	mov %al,_v_hex_lo
	
	pop %ax
	ret


# Printing hex from BL register
# AH,AL - coordinates as in _prtxt routine
_prhex:
	push %ax
	mov %bl,%al
	call _hexify
	pop %ax
	lea _txt_hex,%si
	call _prtxt
	ret


# variable area

_v_max_sector:
	.byte 0x00
_v_max_track:
	.word 0x0000
_v_max_head:
	.byte 0x00

_v_cur_sector:
	.byte 0x00  # Note that boot sector has been already read.
_v_cur_track:
	.word 0x0000
_v_cur_head:
	.byte 0x00

_v_dotcol:
	.byte 0x00

_txt_hex:
_v_hex_hi:
	.byte 0
_v_hex_lo:
	.byte 0
	.byte 0  # Null-terminator

_c_sectors:
	.word 0  # Number of 512b sectors directly following boot sector to be loaded at physical address 07C0:0200 (see _sec_ofs). Value calculated later.

_c_sec_ofs:
	.word 0x0200  # An offset in 07C0 segment to write next sectors at.

# const area

_boot_ss_desc:
	.word 0x7c00
_boot_ss_desc_off:
	.word 0x0000

_txt_hello:
	.ascii "Boot..."
	.byte 0 # Null-terminated...
	
_txt_ioerr:
	.ascii "I/O Error:"
	.byte 0 # Null-terminated...

_txt_secread:
	.ascii "Read."
	.byte 0 # Null-terminated...

_txt_diskbounderr:
	.ascii "Disk bound!"
	.byte 0 # Null-terminated...

_txt_dot:
	.ascii "."
	.byte 0 # Null-terminated...


# Partition table
.org 0x01be

	.byte 0x80         # bootable partition
	.byte 0x00         # 1st head
	.word 0x0001       # 1st track and 1st sector
	.byte 0x00         # unknown operating system
	.byte 0x00         # last head
	.word 0x0310       # last track and last sector
	.long 0x00000000   # 1st relative sector of the partition
	.long 0x00000040   # partition size in sectors

	.byte 0x00         # non-bootable partition
	.byte 0x00         # 1st head
	.word 0x0000       # 1st track and 1st sector
	.byte 0x00         # unknown operating system
	.byte 0x00         # last head
	.word 0x0000       # last track and last sector
	.long 0x00000000   # 1st relative sector of the partition
	.long 0x00000000   # partition size in sectors

	.byte 0x00         # non-bootable partition
	.byte 0x00         # 1st head
	.word 0x0000       # 1st track and 1st sector
	.byte 0x00         # unknown operating system
	.byte 0x00         # last head
	.word 0x0000       # last track and last sector
	.long 0x00000000   # 1st relative sector of the partition
	.long 0x00000000   # partition size in sectors

	.byte 0x00         # non-bootable partition
	.byte 0x00         # 1st head
	.word 0x0000       # 1st track and 1st sector
	.byte 0x00         # unknown operating system
	.byte 0x00         # last head
	.word 0x0000       # last track and last sector
	.long 0x00000000   # 1st relative sector of the partition
	.long 0x00000000   # partition size in sectors

	.word 0xaa55  # magic number


#
# Next sectors begin here.
#


.org 0x0200

_high_code:

# Hello from new sector
	mov $0x0003,%ax
	lea _txt_nxtsec_ent,%si
	call _prtxt


# Print drive spec.
	call _prdrv

# Check memory.
	call _read_memory_map

# Go to protected mode.
	jmp _prepare_protected_mode


# Print drive specification as remembered in _v_max_* variables.
_prdrv:
	mov $0x0005,%ax
	lea _txt_max_head,%si
	call _prtxt
	add $0x02,%ah
	mov _v_max_head,%bl
	call _prhex
	
	add $0x03,%ah
	lea _txt_max_track,%si
	call _prtxt
	add $0x02,%ah
	mov _v_max_track+2,%bl
	call _prhex
	add $0x02,%ah
	mov _v_max_track,%bl
	call _prhex

	add $0x03,%ah
	lea _txt_max_sector,%si
	call _prtxt
	add $0x02,%ah
	mov _v_max_sector,%bl
	call _prhex

	ret


# Read memory map

MMAP_SEGMENT = 0x9000

_read_memory_map:
	push %es
_read_next_mmap_entry:
	mov $(MMAP_SEGMENT),%ax  # Set ES,
	mov %ax,%es              # buffer segment (starting at 0x00090000, 576KB).
	mov $0x0000e820,%eax     # Set EAX, function #.
	mov _mmap_cont_val,%ebx  # Set EBX, continuation value (0 at the beginning).
	mov _mmap_freebuf,%ecx   # Set ECX, buffer length puts limit on 0x00097c00 (607KB).
	cmp $20,%ecx             # Check if the free buffer space is enough.
	jb _read_mm_too_short    # 
	mov $0x534d4150,%edx     # Set EDX, ascii: 'SMAP'.
	mov _mmap_length,%di     # Set DI, buffer offset.
	int $0x15                # Call int. #15h, i.e. read next entry.
	jc _read_mm_not_found    # Check if error.
	cmp $0x534d4150,%eax     # Check if 'SMAP' returned in EAX.
	jne _read_mm_not_found   # 
	mov %ebx,_mmap_cont_val  # Remember continuation value so as to get next entry in the next iteration.
	test %ecx,%ecx           # Check if any data has been read actually.
	jz _read_mmap_finished   # 
	add %ecx,_mmap_length    # Memory map returned length.
	sub %ecx,_mmap_freebuf   # 
	test %ebx,%ebx           # Check if this is the last entry.
	jz _read_mmap_finished   # 
	jmp _read_next_mmap_entry  # There are some entries left - read them!
_read_mmap_finished:
	mov $0x0004,%ax    # Print that the memory map has been read.
	pop %es            # BTW: pop ES.
	lea _txt_mmap,%si  #
	call _prtxt        #
	mov $0x1a04,%ax        # Print memory map length.
	mov _mmap_length,%ebx  #
	mov %ch,%bl            # (higher byte in BL)
	call _prhex            #
	mov $0x1c04,%ax        #
	mov _mmap_length,%ebx  # (lower byte in BL)
	call _prhex            #
	jmp _read_mm_end
_read_mm_too_short:
	mov $0x0004,%ax             # Print that the buffer was too short.
	pop %es                     # BTW: pop ES.
	lea _txt_mmap_too_long,%si  #
	call _prtxt                 #
	jmp _read_mm_end            #
_read_mm_not_found:
	mov $0x0004,%ax       # Print that reading memory map has failed.
	pop %es               # BTW: pop ES.
	lea _txt_no_mmap,%si  #
	call _prtxt           #
	jmp _read_mm_end      #
_read_mm_end:
	ret

_mmap_cont_val:
	.word 0  # Continuation value.
	.word 0  # Upper 16-bit guard when loading 32-bit register.
_mmap_length:
	.word 0  # Number of entries
	.word 0  # Upper 16-bit guard when loading 32-bit register.
_mmap_freebuf:
	.word 0x7c00  # Buffer limit: 0x9000:0x7c00 (= 0x00097c00, 607KB)
	.word 0  # Upper 16-bit guard when loading 32-bit register.

# Again, const area:

_txt_no_mmap:
	.ascii "Memory map not found."
	.byte 0

_txt_mmap:
	.ascii "Memory map read (length 0x    )."
	.byte 0

_txt_mmap_too_long:
	.ascii "Memory map is too long."
	.byte 0

_txt_nxtsec_ent:
	.ascii "Next sectors entered successfully."
	.byte 0

_txt_max_head:
	.ascii "H:"
	.byte 0

_txt_max_track:
	.ascii "T:"
	.byte 0

_txt_max_sector:
	.ascii "S:"
	.byte 0


# GDT - Global Descriptor Table

#                      ___________________________ ___________________________ _____________________
#                     /      C_C_CODE_LENGTH      V    MAIN_C_STACK_LENGTH    V  BOOT_STACK_LENGTH  \
#
#   +-----------------+---------------------------+---------------------------+---------------------+
#   |                 |                           |                           |                     |
#   |   Boot Loader   |           C Code          |          C Stack          |      Boot Stack     |
#   |                 |                           |                           |                     |
#   +-----------------+---------------------------+---------------------------+---------------------+
#                     ^                           ^                           ^
# 7C00h         MAIN_C_START              KERNEL_DATA_START              MAIN_C_AFTER
#                     =                                                       =
#          MAIN_C_STACK_BASE_ADDR                                    BOOT_STACK_BASE_ADDR
#                     |                                                       |                     *
#                     |                                                       \----------> BOOT_STACK_LIMIT_OFFS
#                     |
#                     |                                                       *
#                     \-----------------------------------------> MAIN_C_STACK_LIMIT_OFFS

MAIN_C_START       = 0x7c00 + (_main_c - _start)
KERNEL_DATA_START  = MAIN_C_START + C_C_CODE_LENGTH

MAIN_C_CODE_BASE_ADDR    = MAIN_C_START
MAIN_C_CODE_LIMIT_OFFS   = C_C_CODE_LENGTH - 1

MAIN_C_STACK_LENGTH      = 0x00008000
MAIN_C_STACK_BASE_ADDR   = MAIN_C_START   # need consistency between function pointers, data, and stack; be carefull not to overflow to C code!
MAIN_C_STACK_LIMIT_OFFS  = (C_C_CODE_LENGTH + MAIN_C_STACK_LENGTH) & 0xfffff000 + 0x00000fff  # (aligning to the last byte of the last 4kB) this will be an expanding-up type segment!
MAIN_C_AFTER             = MAIN_C_STACK_LIMIT_OFFS + 1  # this is aligned to 4kB

MAIN_C_DATA_BASE_ADDR    = MAIN_C_STACK_BASE_ADDR

BOOT_STACK_LENGTH      = 0x1000  # 4kB
BOOT_STACK_BASE_ADDR   = MAIN_C_AFTER
BOOT_STACK_LIMIT_OFFS  = BOOT_STACK_LENGTH - 1  # this will be an expanding-up segment of 1B granularity

	.align 8,0x00  # Filling free space with 0x00 (instead of junks).
_gdt:
_gdt_null:
	.word 0x0000  # segment length - bits 0..15
	.word 0x0000  # base address - bits 0..15
	.byte 0x00    # base address - bits 16..23
	.byte 0x00    # segment type and attributes
	.byte 0x00    # segment lenght - bits 16..19 - and attributes
	.byte 0x00    # base address - bits 24..31
_gdt_boot_seg:
	.word 0xffff  # segment length - bits 0..15 (64Kb)
	.word 0x7c00  # base address - bits 0..15 (0x00007c00 - like boot segment)
	.byte 0x00    # base address - bits 16..23
	.byte 0x9a    # segment type and attributes (present, DPL=0, memory segment descriptor, code+readable, not yet accessed)
	.byte 0x40    # segment lenght - bits 16..19 - and attributes (1b granularity, 32-bit default)
	.byte 0x00    # base address - bits 24..31
_gdt_boot_data:
	.word 0xffff  # segment length - bits 0..15 (64Kb)
	.word 0x7c00  # base address - bits 0..15 (0x00007c00 - like boot segment)
	.byte 0x00    # base address - bits 16..23
	.byte 0x92    # segment type and attributes (present, DPL=0, memory segment descriptor, data_r/w, not yet accessed)
	.byte 0x40    # segment lenght - bits 16..19 - and attributes (1b granularity, 32-bit default)
	.byte 0x00    # base address - bits 24..31
_gdt_boot_stack:
	.word BOOT_STACK_LIMIT_OFFS & 0xffff               # segment limit - bits 0..15 (just above the C code ends)
	.word BOOT_STACK_BASE_ADDR & 0xffff                # base address - bits 0..15 (just above the C code ends)
	.byte (BOOT_STACK_BASE_ADDR >> 16) & 0xff          # base address - bits 16..23
	.byte 0x92                                         # segment type and attributes (present, DPL=0, memory segment descriptor, data_r/w+expand_up, not yet accessed)
	.byte (BOOT_STACK_LIMIT_OFFS >> 16) & 0x0f + 0x00  # segment limit - bits 16..19 - and attributes (1b granularity, max offset at 64Kb)
	.byte (BOOT_STACK_BASE_ADDR >> 24) & 0xff          # base address - bits 24..31
_gdt_kernel_big_seg:
	.word 0xffff  # segment length - bits 0..15 (4Gb for its granularity)
	.word 0x0000  # base address - bits 0..15 (0x00000000)
	.byte 0x00    # base address - bits 16..23 (0x00000000)
	.byte 0x9a    # segment type and attributes (present, DPL=0, memory segment descriptor, code+readable, not yet accessed)
	.byte 0xcf    # segment lenght - bits 16..19 - and attributes (4Gb length, 4Kb granularity, 32-bit default)
	.byte 0x00    # base address - bits 24..31 (0x00000000)
_gdt_kernel_big_data:
	.word 0xffff  # segment length - bits 0..15 (4Gb for its granularity)
	.word 0x0000  # base address - bits 0..15 (0x00000000)
	.byte 0x00    # base address - bits 16..23 (0x00000000)
	.byte 0x92    # segment type and attributes (present, DPL=0, memory segment descriptor, data_r/w, not yet accessed)
	.byte 0xcf    # segment lenght - bits 16..19 - and attributes (4Gb length, 4Kb granularity, 32-bit default)
	.byte 0x00    # base address - bits 24..31 (0x00000000)
_gdt_kernel_stack:
	.word 0x0000  # segment limit - bits 0..15 (4Gb length)
	.word 0x0000  # base address - bits 0..15
	.byte 0x00    # base address - bits 16..23
	.byte 0x96    # segment type and attributes (present, DPL=0, memory segment descriptor, data+down_expanding, not yet accessed)
	.byte 0xc0    # segment limit - bits 16..19 - and attributes (4Kb granularity, max offset: 4Gb)
	.byte 0x00    # base address - bits 24..31
_gdt_video_ram:
	.word 0x0fff  # segment length - bits 0..15 (4Kb)
	.word 0x8000  # base address - bits 0..15 (0x000b8000)
	.byte 0x0b    # base address - bits 16..23
	.byte 0x92    # segment type and attributes (present, DPL=0, memory segment descriptor, data_r/w, not yet accessed)
	.byte 0x40    # segment lenght - bits 16..19 - and attributes (1b granularity, 32-bit default)
	.byte 0x00    # base address - bits 24..31
_gdt_main_c_code:
	.word MAIN_C_CODE_LIMIT_OFFS & 0xffff               # segment length - bits 0..15
	.word MAIN_C_CODE_BASE_ADDR & 0xffff                # base address - bits 0..15
	.byte (MAIN_C_CODE_BASE_ADDR >> 16) & 0xff          # base address - bits 16..23
	.byte 0x9a                                          # segment type and attributes (present, DPL=0, memory segment descriptor, code+readable, not yet accessed)
	.byte (MAIN_C_CODE_LIMIT_OFFS >> 16) & 0x0f + 0x40  # segment lenght - bits 16..19 - and attributes (1B granularity, 32-bit default)
	.byte (MAIN_C_CODE_BASE_ADDR >> 24) & 0xff          # base address - bits 24..31
_gdt_main_c_data:
	.word 0xffff          # segment length - bits 0..15 (max.)
	.word MAIN_C_DATA_BASE_ADDR & 0xffff        # base address - bits 0..15
	.byte (MAIN_C_DATA_BASE_ADDR >> 16) & 0xff  # base address - bits 16..23
	.byte 0x92            # segment type and attributes (present, DPL=0, memory segment descriptor, data_r/w, not yet accessed)
	.byte 0xcf            # segment lenght - bits 16..19 (max.) - and attributes (4KB granularity, 32-bit default)
	.byte (MAIN_C_DATA_BASE_ADDR >> 24) & 0xff  # base address - bits 24..31
_gdt_main_c_stack:
	.word (MAIN_C_STACK_LIMIT_OFFS >> 12) & 0xffff             # segment limit - bits 0..15
	.word MAIN_C_STACK_BASE_ADDR & 0xffff                      # base address - bits 0..15
	.byte (MAIN_C_STACK_BASE_ADDR >> 16) & 0xff                # base address - bits 16..23
	.byte 0x92                                                 # segment type and attributes (present, DPL=0, memory segment descriptor, data_r/w+expand up, not yet accessed)
	.byte (MAIN_C_STACK_LIMIT_OFFS >> 16 >> 12) & 0x0f + 0xc0  # segment limit - bits 16..19  - and attributes (4KB granularity, 32-bit offset)
	.byte (MAIN_C_STACK_BASE_ADDR >> 24) & 0xff                # base address - bits 24..31
_gdt_end:

# Segment selectors for GDT, with RPL=0:
GDT_NULL            = _gdt_null            - _gdt
GDT_BOOT_SEG        = _gdt_boot_seg        - _gdt
GDT_BOOT_DATA       = _gdt_boot_data       - _gdt
GDT_BOOT_STACK      = _gdt_boot_stack      - _gdt
GDT_KERNEL_BIG_SEG  = _gdt_kernel_big_seg  - _gdt
GDT_KERNEL_BIG_DATA = _gdt_kernel_big_data - _gdt
GDT_KERNEL_STACK    = _gdt_kernel_stack    - _gdt
GDT_VIDEO_RAM       = _gdt_video_ram       - _gdt
GDT_MAIN_C_CODE     = _gdt_main_c_code     - _gdt
GDT_MAIN_C_DATA     = _gdt_main_c_data     - _gdt
GDT_MAIN_C_STACK    = _gdt_main_c_stack    - _gdt

_gdt_desc:
	.word _gdt_end-_gdt    # GDT length
	.long BOOTSEG*16+_gdt  # GDT base address (absolute - hence 0x7c00 added)


_idt_desc_null:
	.word 0x0000
	.long 0x00000000


_prepare_protected_mode:

# Mask interrupts.
	mov $0xff,%al  # Mask 8257A interrupts:
	out %al,$0x21  #  -master
	out %al,$0xa1  #  -slave
	in $0x21,%al   # just to wait a while...
	in $0xa1,%al   # 
	cli  # Turn off maskable interrupts

# Enable A20 line.

_a20_enable:

	call _8042_wait_out  #
	mov $0xad,%al        # Disable keyboard
	out %al,$0x64        #
	
	call _8042_wait_out  #
	mov $0xd0,%al        # Reading
	out %al,$0x64        #
	call _8042_wait_in   #
	in $0x60,%al         #
	
	mov %al,%bl          # Set bit 2
	or $0x02,%bl         #
	
	call _8042_wait_out  #
	mov $0xd1,%al        # Writing
	out %al,$0x64        #
	call _8042_wait_out  #
	mov %bl,%al          #
	out %al,$0x60        #
	
	call _8042_wait_out  #
	mov $0xae,%al        # Enable keyboard
	out %al,$0x64        #

	call _8042_wait_out  # Last wait...
	jmp _a20_on          #

_8042_wait_out:
	in $0x64,%al
	test $0x02,%al
	jnz _8042_wait_out
	ret
	
_8042_wait_in:
	in $0x64,%al
	test $0x01,%al
	jz _8042_wait_in
	ret

_a20_on:
	
# Load GDT
	lgdt _gdt_desc  # We point at 48-bit data structure

# Load temporary (empty) IDT
	lidt _idt_desc_null  # We point at 48-bit data structure

# Go to protected mode
	mov %cr0,%eax    # Get CR0.
	or $0x0001,%eax  # Set PE bit.
	mov %eax,%cr0    # Write to CR0, thus entering protected mode.
	jmp $GDT_BOOT_SEG,$_enter_protected_mode32  # Segment selector 1 starts at 0x000007c00.



#########################################################################################
#########################################################################################
#########################################################################################



_enter_protected_mode32:
.code32  # Code segment default set to 16 bit (not 32 bit).

# Reset data segments
	mov $GDT_BOOT_DATA,%ax
	mov %ax,%ds
	mov %ax,%es

# Reset stack segment selector and stack pointer to conform to protected mode:
	lss _boot_ss_desc32,%esp

# Inform about having entered protected mode.
	lea _txt_prot_mod,%si
	mov $0x0006,%ax
	call _prtxt32
	
#	sti  # Turn on maskable interrupts

	jmp _prepare_enter_c


# Printing (32-bit protected mode)
_prtxt32:
	push %ax  # Very initialization
	push %bx  # 
	push %es  #
	push %si  #
	push %di  #
	
	mov %ah,%bl    # Initializing offset (xxxx) from 0x000b8000
	add %bl,%bl    # Offset is being written into DI register.
	mov $0x50,%bh  #
	mul %bh        #
	add %ax,%ax    #
	xor %bh,%bh    #
	add %bx,%ax    #
	mov %ax,%di    #
	mov $GDT_VIDEO_RAM,%bx  # Set destination segment selector (ES) to
	mov %bx,%es             # cover Video RAM.

_prtxt_char32:
	mov %ds:(%si),%bl  # Check if we have reached the terminating null.
	cmp $0x00,%bl      #
	jz _prtxt_end32    # If so, go to end.
	
	mov %bl,%es:(%di)  # Print char and
	add $0x0002,%di    # move the "cursor" one char right.
	inc %si            # 
	
	jmp _prtxt_char32  # Next char...
	
_prtxt_end32:
	pop %di  # Clean up.
	pop %si  #
	pop %es  #
	pop %bx  #
	pop %ax  #
	ret  # Return from the routine.


/*
# Hexifying routine (32-bit code): AL register - value to hexify
_hexify32:
	push %ax
	mov %al,%ah
	and $0x0f,%al
	and $0xf0,%ah
	ror $4,%ah

	cmp $0x09,%ah
	jbe _hexhi0932
	add $('A'-0x0a),%ah
	jmp _hexhi0A32
_hexhi0932:
	add $('0'),%ah
_hexhi0A32:

	cmp $0x09,%al
	jbe _hexlo0932
	add $('A'-0x0a),%al
	jmp _hexlo0A32
_hexlo0932:
	add $'0',%al
_hexlo0A32:

	mov %ah,_v_hex_hi32
	mov %al,_v_hex_lo32
	
	pop %ax
	ret

_txt_hex32:
_v_hex_hi32:
	.byte 0
_v_hex_lo32:
	.byte 0
	.byte 0  # Terminating null.

# Printing hex (32-bit code) from BL register
# AH,AL - coordinates as in _prtxt routine
_prhex32:
	push %ax
	mov %bl,%al
	call _hexify32
	pop %ax
	lea _txt_hex32,%si
	call _prtxt32
	ret


# Comparing two null-terminated strings. Routine returns 1 in CL if both are equal,
# 0 otherwise. String are pointed by DS:SI and ES:DI.

_cpmstr32:
	pushf
	push %eax
	push %ebx

_cmpstr_cmp32:
	mov %ds:(%si),%al
	mov %es:(%di),%bl
	cmp %al,%bl
	je _cmpstr_chareq32
	mov $0x00,%cl
	jmp _cmpstr_end32
	
_cmpstr_chareq32:
	cmp $0x00,%al  # Test if both are 0 - which means both has ended and are equal
	jne _cmpstr_charn032
	mov $0x01,%cl      # Both ends reached - strings are equal.
	jmp _cmpstr_end32  #

_cmpstr_charn032:
	inc %si            #
	inc %di            # Prepare to compare next character.
	jmp _cmpstr_cmp32  #

_cmpstr_end32:
	pop %ebx
	pop %eax
	popf
	
	ret
*/

# Const area (this time - for 32-bit code)

_boot_ss_desc32:
	.long BOOT_STACK_LIMIT_OFFS + 1  # point to the first byte beyond the (expanding-up type) stack segment
	.word GDT_BOOT_STACK

_main_c_ss_desc32:
	.long MAIN_C_STACK_LIMIT_OFFS + 1  # point to the first byte beyond the (expanding-up type) stack segment
	.word GDT_MAIN_C_STACK

_txt_prot_mod:
	.ascii "Protected mode entered."
	.byte 0

C_ENTRY_POINT = 0x00000000

# Entering C code.

_prepare_enter_c:
	lss _main_c_ss_desc32,%esp
	push %ebp
	mov %esp,%ebp
	sub $16,%esp
	mov $(BOOTSEG * 16 + (_main_c - _start)),%eax  # [1] First argument for starting C function.
	mov %eax,(%esp)                                # First argument - by convention - is just below the stack.
	mov $(MMAP_SEGMENT * 16),%eax                  # [2] Memory map pointer (where memory map segment starts).
	mov %eax,4(%esp)                               # Second argument - by convention - is 4 bytes below.
	mov _mmap_length,%eax                          # [3] Memory map length.
	mov %eax,8(%esp)                               # Third argument - by convention - is...
	mov $C_ENTRY_POINT,%eax  # If the first C function tries to return, it will
	push %eax                # return to address 0x00000000 - hence itself.
	mov $GDT_MAIN_C_DATA,%ax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%gs
	jmp $GDT_MAIN_C_CODE,$C_ENTRY_POINT
	leave
	
_sys_halt32:
	hlt
	jmp _sys_halt32


	.align 512,0
_main_c:
# C code compiled to binary format should be appended here... Its addresses
# should start at 0x00000000 (i.e. ".org 0"), since GDT_MAIN_C segment
# selector defines a segment that starts here. First routine at 0x00000000
# should return the entry point (32 bit pointer). Successful return from
# this routine is the very first check of C code being correctly linked.

# Bye, bye asm. We are going to meet on the other side, in the C world...
