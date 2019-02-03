#! /bin/bash
g++ random_value_gen.cpp
fcfs_tot=0
sjf_tot=0
rrb_tot=0
presjf_tot=0
hrn_tot=0
pre_hrn_tot=0
for i in 10 50 100
do
	j=1
	echo "Running For i = $i"
	while [[ $j -le 10 ]]; do
		read fcfs sjf rrb presjf hrn pre_hrn<<< $(echo $i | ./a.out)
		fcfs_tot=`bc<<< "$fcfs_tot + $fcfs"`
		sjf_tot=`bc <<< "$sjf_tot + $sjf"`
		presjf_tot=`bc <<< "$presjf_tot + $presjf"`
		rrb_tot=`bc <<< "$rrb_tot + $rrb"`
		hrn_tot=`bc <<< "$hrn_tot + $hrn"`
		pre_hrn_tot=`bc <<< "$pre_hrn_tot + $pre_hrn"`
		j=`expr $j + 1`
	done
	fcfs_tot=`bc <<< "scale=2; $fcfs_tot / 10"`
	sjf_tot=`bc <<< "scale=2; $sjf_tot / 10"`
	presjf_tot=`bc <<< "scale=2; $presjf_tot / 10"`
	rrb_tot=`bc <<< "scale=2; $rrb_tot / 10"`
	hrn_tot=`bc <<< "scale=2; $hrn_tot / 10"`
	pre_hrn_tot=`bc <<< "scale=2; $pre_hrn_tot / 10"`
	printf "0 fcfs $fcfs_tot\n1 sjf $sjf_tot\n2 presjf $presjf_tot\n3 rrb $rrb_tot\n
	4 hrn $hrn_tot\n5 prehrn $pre_hrn_tot " > "data_$i.dat"
	echo "First come first serve = $presjf_tot"
	echo "Non-preemptive Shortest Job First = $sjf_tot"
	echo "Preemptive Shortest Job First = $presjf_tot"
	echo "Round Robin (with time quantum = 2) = $rrb_tot"
	echo "Non Preemptive Highest Response Ratio = $hrn_tot"
	echo "Preemptive Highest Response Ratio = $pre_hrn_tot"
	echo ""
done
echo "Generating graphs"

gnuplot -persist << -EOFMarker
set title "N = 10 processes"
set yrange [0:*]
set boxwidth 0.4
set style fill solid
set style line 1 lc rgb "red"
set xtics format ""
set grid ytics
unset key
plot "data_10.dat" using 1:3:xtic(2) with boxes, '' using 0:3:3 with labels offset 0, char 1
-EOFMarker

gnuplot -persist << -EOFMarker
set title "N = 50 processes"
set yrange [0:*]
set style line 1 lc rgb "blue"
set boxwidth 0.4
set style fill solid
set xtics format ""
set grid ytics
unset key
plot "data_50.dat" every ::0::6 using 1:3:xtic(2) with boxes ls 1, '' using 0:3:3 with labels offset 0, char 1
-EOFMarker


gnuplot -persist << -EOFMarker
set title "N = 100 processes"
set yrange [0:*]
set boxwidth 0.4
set style fill solid
set style line 1 lc rgb "green"
set xtics format ""
set grid ytics
set font "Helvetica,30"
unset key
plot "data_100.dat" every ::0::6 using 1:3:xtic(2) with boxes ls 1, '' using 0:3:3 with labels offset 0, char 1
-EOFMarker

rm data_10.dat
rm data_50.dat
rm data_100.dat
exit