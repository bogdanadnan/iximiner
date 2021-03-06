#!/usr/bin/env bash

. $MINER_DIR/$CUSTOM_MINER/h-manifest.conf

. $MINER_DIR/$CUSTOM_MINER/parse-api-port.sh

json_data=`curl -s http://localhost:${CUSTOM_API_PORT}/status`

hashrates=`echo $json_data | jq -c ".[]" | jq -c ".hashers[]" | jq -c ".devices[]" | jq -c ".hashrate"`
block_height=`echo $json_data | jq -c ".[]" | jq -c ".block_height"`
shares=`echo $json_data | jq -c ".[]" | jq -c ".shares"`
rejects=`echo $json_data | jq -c ".[]" | jq -c ".rejects"`
uptime=`echo $json_data | jq -c ".[]" | jq -c ".time_running"`

total_hashrate=`echo $hashrates | awk '{sum=0; for(i=1; i<=NF; i++) sum += $i; sum = sum/1000; print sum}'`

gpu_data=`gpu-stats`
busids_data=`echo $gpu_data | jq -r ".busids[]"`
busids=($busids_data)
temp_data=`echo $gpu_data | jq -r ".temp[]"`
temp_local=($temp_data)
fan_data=`echo $gpu_data | jq -r ".fan[]"`
fan_local=($fan_data)
device_bus_data=`echo $json_data | jq -c ".[]" | jq -c ".hashers[]" | jq -c ".devices[]" | jq -r ".bus_id"`
device_bus=($device_bus_data)

stats_temp=""
stats_fan=""
bus_numbers=""

for i in "${!device_bus[@]}"; do
  found=0
  for j in "${!busids[@]}"; do
    if [ "${device_bus[$i],,}" == "${busids[$j],,}" ]; then
	stats_temp="$stats_temp ${temp_local[$j]}"
	stats_fan="$stats_fan ${fan_local[$j]}"
	bus_number=$(echo ${busids[$j]} | cut -d ':' -f 1 | awk '{printf("%d\n", "0x"$1)}')
	bus_numbers="$bus_numbers $bus_number"
        found=1
	break
    fi
  done
  if [ $found -eq 0 ]; then
    stats_temp="$stats_temp 0"
    stats_fan="$stats_fan 0"
    bus_numbers="$bus_numbers 0"
  fi
done

khs=$total_hashrate
hashrates=$hashrates

stats=$(jq -nc \
	--argjson hs "`echo "$hashrates" | tr " " "\n" | jq -cs '.'`" \
	--arg hs_units "hs" \
	--argjson temp "`echo "$stats_temp" | tr " " "\n" | jq -cs '.'`" \
	--argjson fan "`echo "$stats_fan" | tr " " "\n" | jq -cs '.'`" \
	--arg uptime "$uptime" \
	--arg ac "$shares" --arg rj "$rejects" \
	--arg algo "argon2id" \
	--argjson bus_numbers "`echo "$bus_numbers" | tr " " "\n" | jq -cs '.'`" \
	'{$hs, $hs_units, $temp, $fan, $uptime,ar: [$ac, $rj], $bus_numbers, $algo}')
