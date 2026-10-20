#!/bin/bash
# Every comparison of the systematics study, into syst_study/changes.csv (rewritten). Run from fissionStudies/
# with the ATTPCROOT environment loaded, after fetching every study (fission.py STUDY --remote NAME --fetch).
# DUMPS (default ./.dumps) keeps dump_fit.C's per-event CSVs between runs.
set -e
cd "$(dirname "$0")/.."
T=syst_study/truth_syst
DUMPS=${DUMPS:-./.dumps}
OUT=syst_study/changes.csv
rm -f $OUT
BASE="hpc-syst:syst_base:rec@${T}_base-base.csv"
run() { ./syst_report.py "$@" --dump-dir "$DUMPS" --reuse --changes-csv $OUT > /dev/null; }
BY="--by all --by side --by z-near --by depth"

# Stopping powers (B1 SRIM, B2), against the baseline; SRIM data fitted with LISE against SRIM fitted with SRIM
run "$BASE" $(for n in s095 s098 s102 s105; do echo "hpc-syst:syst_eloss:$n@${T}_eloss-$n.csv"; done) \
   "hpc-syst:syst_eloss:srim_lise@${T}_eloss-srim.csv" "hpc-syst:syst_eloss:srim_srim@${T}_eloss-srim.csv" \
   "hpc-syst:syst_eloss:lise_srim@${T}_base-base.csv" --base rec --paired $BY
run "hpc-syst:syst_eloss:srim_srim@${T}_eloss-srim.csv" "hpc-syst:syst_eloss:srim_lise@${T}_eloss-srim.csv" \
   --base srim_srim $BY

# Space charge (A1-A4): the lambda = 1.54e-8 sample and no correction against the baseline, the rest against it
run "$BASE" "hpc-syst2:syst_sc:nom@${T}_sc-nom.csv" "hpc-syst2:syst_sc:nocorr@${T}_sc-nom.csv" --base rec --paired $BY
run "hpc-syst2:syst_sc:nom@${T}_sc-nom.csv" $(for n in half:half half_m:half double:double double_m:double \
   numeric:numeric uniform:uniform constant:constant x5:x5 y5:y5 simbeam:simbeam; do
   echo "hpc-syst2:syst_sc:${n%%:*}@${T}_sc-${n##*:}.csv"; done) --base nom --paired $BY

# Kinematics (C2, C3, C5); the TKE spread changes the random draws, so it is compared as an independent sample
run "$BASE" $(for n in tke098 tke102 am1 ap1 spot2 div3 tilt10; do echo "hpc-syst3:syst_kin:$n@${T}_kin-$n.csv"; done) \
   --base rec --paired $BY
run "$BASE" "hpc-syst3:syst_kin:tkesig@${T}_kin-tkesig.csv" --base rec $BY

# Pads, gains and noise (D1, D3, D4): digitizations of the baseline's simulation, so paired
run "$BASE" $(for n in asad asad_s02 asad_s05 asad_s10 asad_p02 asad_p05 asad_p10 pad5 pad10 aget5 aget10 asad5 \
   asad10 noise5 noise10; do echo "hpc-syst3:syst_pads:$n@${T}_base-base.csv"; done) --base rec $BY
for n in asad asad_s02 asad_s05 asad_s10 asad_p02 asad_p05 asad_p10; do
   run "$BASE" "hpc-syst3:syst_pads:$n@${T}_base-base.csv" --base rec --by dead --dead syst_study/dead/$n.csv
done

# Gas (A5, A6): digitizations of the baseline's simulation, so paired
run "$BASE" $(for n in vdm2 vdm1 vdp1 vdp2 tm20 tp20 lm20 lp20; do echo "hpc-syst3:syst_gas:$n@${T}_base-base.csv"; done) \
   --base rec $BY
