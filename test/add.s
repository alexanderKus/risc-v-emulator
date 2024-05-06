.data
  num:    .word 10

.text
  .global _start

_start:
  lw t0, num
  lw t1, num
  add t2, t0, t1
