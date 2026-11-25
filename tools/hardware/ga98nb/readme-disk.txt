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

Version 2 (after a first run whose window read FF):
5. Cold boot, then:    GAPROBE BOARD             (reads only: 40E1h-5FE1h)
                       GAPROBE IDENTIFY-BYTE /BOARD  (the same after setup)
6. Cold boot, then:    GAPROBE SCAN              (reads only, minimal setup)
7. Then:               GAPROBE WINDOW-READ /INIT /BOARD
                       (full register initialisation first; the card stays
                        initialised until power off; without /RELAY
                        40E1h is not written, so the screen stays VA)
8. Then:               GAPROBE SCAN /INIT
9. Then:               GAPROBE WINDOW-TEST /INIT /WIN2

Version 3: 40E1h. On PC-98 the card's mode writes OUT16 40E1h,00B3h (card
display) or 00B2h, and 0000h when it leaves; bit 0 is the display relay,
another bit may enable the window.
10. Cold boot, then:   GAPROBE BOARD
                       GAPROBE SCAN /INIT /RELAY=B2   (reads only)
11. Then:              GAPROBE WINDOW-TEST /INIT /RELAY=B2 /WIN2
12. Only if 11 fails:  GAPROBE WINDOW-TEST /INIT /RELAY=B3 /WIN2
                       (B3 may switch the monitor to the card's output;
                        the log is still written; power off to recover)

Version 4: the window search of PC-98 software for this card (MISC EFh,
40E1h written with 8-bit OUTs: FAh, then B2h).
13. Cold boot, then:   GAPROBE SEARCH            (40E1h read-back check and
                                                  the search, minimal setup)
14. Then:              GAPROBE SEARCH /INIT

Options: /BANK=h   port 152h ROM0 bank released to the bus slot (default F)
         /R152=hh  port 152h value restored afterwards (default 00)
         /OFS=hhhh window offset (default 0200)   /A=hh /B=hh banks (00, 01)
         /WORD     window modes identify with the 16-bit OUT
         /INIT     full register initialisation before the window or scan
         /SR0F=hh  SR0F for /INIT (default 3D)
         /BOARD    read the board ports 40E1h-5FE1h as well
         /RELAY=hh 40E1h <- hh and 40E2h <- 00 before the window, both
                   00 afterwards
         /R153=hh  port 153h value restored by SCAN (default: the value
                   read; A0000h-DFFFFh is not scanned if it is implausible)
Port 152h is restored to the value it reads (00 if that is not a plausible
ROM bank pair); give another with /R152=.
