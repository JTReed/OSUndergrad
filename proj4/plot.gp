set terminal svg size 640,480

set output "counter_incr.svg"

set	object 1 rectangle from screen 0,0 to screen 1,1 \
	fillcolor rgb "white" behind

set ylabel "Time (Seconds)"
set xlabel "Threads"

plot	"counter_incr.spin" title "spin lock" with linespoints, \
	"counter_incr.posix" title "pthread lock" with linespoints

set output "list_insert.svg"

set	object 1 rectangle from screen 0,0 to screen 1,1 \
	fillcolor rgb "white" behind

plot	"list_insert.spin" title "spin lock" with linespoints, \
	"list_insert.posix" title "pthread lock" with linespoints

set output "hash_insert.svg"

set	object 1 rectangle from screen 0,0 to screen 1,1 \
	fillcolor rgb "white" behind

plot	"hash_insert.spin" title "spin lock" with linespoints, \
	"hash_insert.posix" title "pthread lock" with linespoints

set output "hash_scale.svg"

set	object 1 rectangle from screen 0,0 to screen 1,1 \
	fillcolor rgb "white" behind

set xlabel "Buckets"

plot	"hash_scale.spin" title "spin lock" with linespoints, \
	"hash_scale.posix" title "pthread lock" with linespoints

# Local variables:
# c-basic-offset: 8
# tab-width: 8
# indent-tabs-mode: t
# End:
#
# vi: set shiftwidth=8 tabstop=8 noexpandtab:
# :indentSize=8:tabSize=8:noTabs=false:
