#! /bin/bash
g++ random_value_gen.cpp
fcfs_tot=0
sjf_tot=0
rrb_tot=0
presjf_tot=0
fcfs=0
sjf=0
rrb=0
presjf=0
for i in 10 50 100
do
	j=1
	echo "Running For i = $i"
	while [[ $j -le 10 ]]; do
		read fcfs sjf rrb presjf<<< $(echo $i | ./a.out)
		fcfs_tot=`bc<<< "$fcfs_tot + $fcfs"`
		sjf_tot=`bc <<< "$sjf_tot + $sjf"`
		rrb_tot=`bc <<< "$rrb_tot + $rrb"`
		presjf_tot=`bc <<< "$presjf_tot + $presjf"`
		j=`expr $j + 1`
	done
	fcfs_tot=`bc <<< "scale=2; $fcfs_tot / 10"`
	sjf_tot=`bc <<< "scale=2; $sjf_tot / 10"`
	rrb_tot=`bc <<< "scale=2; $rrb_tot / 10"`
	presjf_tot=`bc <<< "scale=2; $presjf_tot / 10"`
	printf "0 fcfs $fcfs_tot\n1 sjf $sjf_tot\n2 rrb $rrb_tot\n3 presjf $presjf_tot\n" > "data_$i.dat"
done
echo "Generating graphs"

gnuplot -persist << -EOFMarker
set title "N = 10 processes"
set yrange [0:*]
set boxwidth 0.5
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
# set style line 2 lc rgb "green"
# set style line 3 lc rgb "blue"
set boxwidth 0.5
set style fill solid
set xtics format ""
set grid ytics
unset key
plot "data_50.dat" every ::0::3 using 1:3:xtic(2) with boxes ls 1, '' using 0:3:3 with labels offset 0, char 1
-EOFMarker


gnuplot -persist << -EOFMarker
set title "N = 100 processes"
set yrange [0:*]
set boxwidth 0.5
set style fill solid
set style line 1 lc rgb "green"
set xtics format ""
set grid ytics
unset key
plot "data_100.dat" every ::0::3 using 1:3:xtic(2) with boxes ls 1, '' using 0:3:3 with labels offset 0, char 1
-EOFMarker
exit