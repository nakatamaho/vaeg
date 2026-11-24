GAPROBE (vaeg M166): GA-98NBIV (CL-GD5434) in a PC-88VA bus slot, port set y=0.
Every card access is logged to GAPROBE.LOG (appended) before it runs.

1. Without the card:   GAPROBE BASELINE          (reads only)
                       GAPROBE BASELINE /BUS     (also E0000h on the bus slot)
2. With the card:      GAPROBE BASELINE /BUS
                       GAPROBE IDENTIFY-BYTE     (8-bit OUTs)
   cold boot, then:    GAPROBE IDENTIFY-WORD     (OUT16 44E0h,1206h)
3. After I/O PASS:     GAPROBE WINDOW-READ /SET  (A -> B -> A, no writes)
4. Then:               GAPROBE WINDOW-TEST /SET /WIN2
                       (saves 4 bytes of banks 00 and 01 at E000:0200,
                        writes A5 5A 3C C3 / 96 69 0F F0, compares,
                        restores, checks the restore)

Options: /BANK=h   port 152h ROM0 bank released to the bus slot (default F)
         /R152=hh  port 152h value restored afterwards (default 00)
         /OFS=hhhh window offset (default 0200)   /A=hh /B=hh banks (00, 01)
         /WORD     window modes identify with the 16-bit OUT
Port 152h is restored to the value it reads (00 if that is not a plausible
ROM bank pair); give another with /R152=.
