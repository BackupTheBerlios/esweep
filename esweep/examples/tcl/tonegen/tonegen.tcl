#!/usr/local/bin/tclsh8.5

#load [file join [pwd] libesweep.dll] esweep; # replace this line by something like package require ...
load ../../../libesweeptcl.so esweep; # replace this line by something like package require ...

# set defaults
set samplerate 48000
set framesize 2400
set frequency 1000
set level 90
set duration 0

proc usage {} {
	global samplerate framesize frequency level duration
	puts "Usage: \{tclsh\} tonegen.tcl -frequency Hz \[$frequency\] -level \% \[$level\] \
-samplerate \[$samplerate\] -duration sec \[$duration\] -framesize samples \[$framesize\]"
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
		default {
			usage
			exit
		}
	}
}

proc openAudioDevice {device samplerate framesize} {
	if {[catch {set au [esweep::audioOpen -device $device]}]} {
		puts "Unable to open audio device. Exiting."
		exit
	}
	esweep::audioConfigure -handle $au -param samplerate -value $samplerate
	esweep::audioConfigure -handle $au -param framesize -value $framesize
	puts "Output latency is: [expr {1000.0*$framesize / $samplerate}] ms"
	return $au
}

proc genSignals {au samplerate size frequency level} {
	set out [esweep::create -type wave -samplerate $samplerate -size $size]

	puts "Generated frequency: [expr {1.0*$samplerate/$size*[esweep::generate sine -obj out -frequency $frequency]}]"
	esweep::expr out * [expr {$level / 100.0}]

	esweep::audioSync -handle $au
	return [list $out [esweep::clone -src $out]] 
}

proc play {au sig size interval _play} {
	# fill the output buffers once
	upvar ${_play} play
	set wait 0
	while {$play} {
		set offset 0
		while {$offset < $size} {
			set offset [esweep::audioOut -handle $au -signals $sig -offset $offset]
			after $interval [list incr wait]
			vwait wait
		}	
	}
}

# open soundcard
set au [openAudioDevice audio:/dev/audio1 $samplerate $framesize]
esweep::audioSync -handle $au

set size [expr {$samplerate > $framesize ? $samplerate : $framesize}]
set out [genSignals $au $samplerate $size $frequency $level]

if {$duration > 0} {after [expr {int(0.5+1000*$duration)}] set play 0}
set play 1

set interval [expr {int(1000.0*$framesize/(4*$samplerate))}]
play $au $out [esweep::size -obj [lindex $out 0]] $interval play

exit
