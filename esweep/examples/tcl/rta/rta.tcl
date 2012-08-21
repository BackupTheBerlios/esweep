#!/usr/local/bin/tclsh8.5
package require Tk 8.5
package require Ttk 8.5
source ../TkEsweepXYPlot/TkEsweepXYPlot.tcl

load ../../esweep.dll; # replace this line by something like package require ...
#load ../esweep.so; # replace this line by something like package require ...

# set defaults
set samplerate 48000
set framesize 2048
set frequency 1000
set level 90
set duration 0
set fftsize 4096
set refresh 0.2

proc usage {} {
	global samplerate framesize frequency level duration refresh fftsize
	puts "Usage: \{tclsh\} rta.tcl -frequency Hz \[$frequency\] -level \% \[$level\] \
	-samplerate \[$samplerate\] -duration sec \[$duration\] -refresh sec \[$refresh\] \
	-framesize samples \[$framesize\] -fftsize samples \[$fftsize\]"
}

# parse args
foreach {opt val} $argv {
	switch $opt {
		-frequency {
			set frequency $val
		}
		-level {
			set level $val
		}
		-samplerate {
			set samplerate $val
		}
		-framesize {
			set framesize $val
		}
		-duration {
			set duration $val
		}
		-refresh {
			set refresh $val
		}
		-fftsize {
			set fftsize $val
		}
		default {
			usage
			exit
		}
	}
}

# framesize and fftsize must be a power of 2
if {$fftsize < $framesize} {
	puts stderr {Error: FFT size must be at least 2 times framesize}
	exit
}

proc openAudioDevice {device samplerate framesize} {
	set au [esweep::audioOpen -device $device]
	esweep::audioConfigure -handle $au -param samplerate -value $samplerate
	esweep::audioConfigure -handle $au -param framesize -value $framesize
	esweep::audioSync -handle $au
	puts stderr "Output latency is: [expr {1000.0*$framesize / $samplerate}] ms"
	return $au
}

proc genSignals {au samplerate size frequency level} {
	set out [esweep::create -type wave -samplerate $samplerate -size $size]
	set in [esweep::create -type complex -samplerate $samplerate -size $size]

	puts stderr "Generated frequency: [expr {1.0*$samplerate/$size*[esweep::generate sine -obj out -frequency $frequency]}]"
	esweep::expr out * [expr {$level / 100.0}]

	return [list [list $out [esweep::clone -src $out]] [list $in [esweep::clone -src $in]]]
}

proc play {au sig size offset interval idVar} {
	upvar $idVar id
	set offset [esweep::audioOut -handle $au -signals $sig -offset $offset]
	if {$offset >= $size} {set offset 0}
	set id [after $interval [list play $au $sig $size $offset $interval $idVar]]
	update
}
	
proc record {au sig size offset interval table idVar} {
	upvar $idVar id
	set offset [esweep::audioIn -handle $au -signals $sig -offset $offset]
	set id [after $interval [list record $au $sig $size $offset $interval $idVar]]
	doFFT $sig $table
	update
}

proc doFFT {sig table} {
	set in1 [lindex $sig 0]
	set in2 [lindex $sig 1]
	set size [esweep::size -obj $in1]
	esweep::fft -obj in1 -inplace -table $table
	esweep::fft -obj in2 -inplace -table $table
	esweep::toPolar -obj in1
	esweep::toPolar -obj in2
	esweep::expr in1 / $size
	esweep::expr in2 / $size
	esweep::lg -obj in1
	esweep::lg -obj in2
	esweep::expr in1 * 20.0
	esweep::expr in2 * 20.0
	::TkEsweepXYPlot::plot FR
	esweep::toComplex -obj in1
	esweep::toComplex -obj in2
}

proc createGraph {} {
	set width [expr {[winfo width .]/2}]
	set height [expr {[winfo height .]/2}]

	canvas .c -bg white -width $width -height $height
	pack .c -expand yes -fill both
	focus .c

	bind .c <Configure> {::TkEsweepXYPlot::redrawCanvas FR}

	::TkEsweepXYPlot::init .c FR

	set ::TkEsweepXYPlot::plots(FR,Config,Margin,Top) 10
	set ::TkEsweepXYPlot::plots(FR,Config,Margin,Bottom) 20
	set ::TkEsweepXYPlot::plots(FR,Config,MouseHoverDeleteDelay) 5000
	set ::TkEsweepXYPlot::plots(FR,Config,Legend,Align) center

	set ::TkEsweepXYPlot::plots(FR,Config,X,Label) {f [Hz]}
	set ::TkEsweepXYPlot::plots(FR,Config,X,Precision) 5
	set ::TkEsweepXYPlot::plots(FR,Config,X,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(FR,Config,X,Min) 20
	set ::TkEsweepXYPlot::plots(FR,Config,X,Max) 20e3
	set ::TkEsweepXYPlot::plots(FR,Config,X,Bounds) [list 1 48000]
	set ::TkEsweepXYPlot::plots(FR,Config,X,Log) {yes}

	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Label) {FS}
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Precision) 3
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Autoscale) {on}
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Min) -90
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Max) 0
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Bounds) [list -90 0]
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Range) 100
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Log) {no}


	::TkEsweepXYPlot::plot

	return .c
}

# create graph
set c [createGraph]
tkwait visibility $c

# open soundcard
set au [openAudioDevice audio:0 $samplerate $framesize]

# generate signals
lassign [genSignals $au $samplerate $fftsize $frequency $level] out in

# put these signals on the graph
::TkEsweepXYPlot::addTrace FR 0 FR [lindex $in 0]
::TkEsweepXYPlot::addTrace FR 1 FR [lindex $in 0]
::TkEsweepXYPlot::configTrace FR 1 -color darkgreen

set play_interval [expr {int(1000.0*$framesize/(4*$samplerate))}]

# start playback
set playID 0
set playOffset 0
play $au $out [esweep::size -obj [lindex $out 0]] $playOffset $play_interval playID

# start recording
set recID 0
set recOffset 0
record $au $in [esweep::size -obj [lindex $in 0]] $recOffset [expr {int(1000*$refresh)}] recID

if {$duration > 0} {after [expr {int(0.5+1000*$duration)}] set wait 1}
set wait 0
vwait wait

esweep::audioClose -handle $au

exit

