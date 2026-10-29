/* vita-elf-create appends SCE metadata to the executable LOAD segment.
 * Keep a full spare page before the writable segment, independently of size. */
SECTIONS { . = ALIGN(4096) + 4096; }
INSERT BEFORE .init_array;
