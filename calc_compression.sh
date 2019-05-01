#!/bin/bash
#for((i=1;i<=20;i++));
#do
#cat nohup.out| grep "Wrote compressed TimeValuePairTS" | head -n $(($i*251)) >compress_ts.txt
#cat nohup.out| grep "Wrote compressed TimeValuePairValue" | head -n $(($i*251)) >compress_value.txt
cat nohup.out| grep "Wrote compressed TimeValuePairTS"  >compress_ts.txt
cat nohup.out| grep "Wrote compressed TimeValuePairValue" >compress_value.txt
python overall_count.py
#done