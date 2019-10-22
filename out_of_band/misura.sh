#!/usr/bin/awk -f
function abs(x) {return x < 0 ? -x : x}
#NR==FNR pattern used to match records from just the first or the second argument file.
NR==FNR && /SECRET/ {tmp1[$2]=$4}
NR!=FNR && /SUPERVISOR ESTIMATE/ {tmp2[$5]=$3}
END {
  success=0
  failure=0
  avgerr=0
  for(i in tmp1){
    err=abs(tmp1[i] - tmp2[i])
    avgerr+=err
    if(err<25){
    printf "ESTIMATE SUCCESS FOR %s\tSECRET %d\tERROR %d\n", i, tmp2[i], err
    success++
    }else{
    printf "ESTIMATE FAILURE FOR %s\tSECRET %d\tERROR %d\n", i, tmp2[i], err
    failure++
    }
  }
  avgerr/= success + failure
  printf "\nTOTAL SUCCESSES %d TOTAL FAILURE %d AVERAGE ERROR %.4f\n", success, failure, avgerr
}