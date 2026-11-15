# rac2 data moves of releases with no program yet

How Going Commando's NTSC-J (SCPS_150.56, v1.00) and Korean (SCKA_200.11, disc v1.02) releases moved
NTSC v1.01's data, from versionmap's maps (`port/tools/versionmap/data_moves.py write`). Addresses
only. When a program `port/game/rac2-<v>/` is added, move its file there as `data_moves.tsv`: parity.py
then checks its rac2_version.h against it.
