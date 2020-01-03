# adi-dsp-programmer
Can be used to program adi dsp:s such as adau1467 over i2c.

Use System Files (Action) in SigmaStudio to generate the files that define the dsp configuration (can be more than 1).
Create a directory system_files and copy the system files to the directory.

Use download.c to define the configuration files.

Program using (from the cmake build directory)

```
./adi_dsp_programmer download
```

It is also possible to read register from the dsp, use

```
./adi_dsp_programmer <r/w> <i2c-addr> <register> <num-of-bytes>
```

* r/w: only read is implementes so far
* i2c-addr: for example 0x74, dsp i2c address in 8-bit notation
* register: for example 0xf402, dsp register
* num-of-bytes: number of bytes to be read from register
