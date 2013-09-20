#!/usr/local/bin/tclsh8.5
package require Tk 8.5
package require Ttk 8.5

load ../esweep.so; # replace this line by something like package require ...
source ../TkEsweepXYPlot/TkEsweepXYPlot.tcl

namespace eval osc {
	variable config
	array set config {
		# {Audio device}
		Audio,Device {/dev/audio1}
		# {Samplerate}
		Audio,Samplerate 44100
		# {bitrate}
		Audio,Bitdepth 16
		Audio,Handle {}
		# {output channel}
		Output,Channel	1 
		# {Maximum output level in 1/FS. If < 1 avoids oversteering}
		Output,MaxFSLevel 0.9 
		Output,Signal {}
		# {Signal frequency in Hz}
		Signal,Frequency 1000
		# {Input channel} 
		Input,Channel	1
		Input,Signal {}
		
		Intern,Play 0
		Intern,Record 0
	}
	# delete comments from array
	array unset config {#}

	proc configure {opt val} {
		variable config
		if {[info exists config($opt)]} {
			set config($opt) $val
		}
	}

	proc cget {opt} {
		variable config
		if {[info exists config($opt)]} {
			return $config($opt)
		} else {
			return {}
		}
	}

	proc append {opt val} {
		variable config
		if {[info exists config($opt)]} {
			lappend config($opt) $val
		}
	}

	proc add {opt val} {
		variable config
		if {![info exists config($opt)]} {
			set config($opt) $val
		}
	}

	namespace ensemble create -subcommands [list configure cget append add]
}

proc createDia {} {
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

	set ::TkEsweepXYPlot::plots(FR,Config,X,Label) {t [ms]}
	set ::TkEsweepXYPlot::plots(FR,Config,X,Precision) 5
	set ::TkEsweepXYPlot::plots(FR,Config,X,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(FR,Config,X,Min) 0
	set ::TkEsweepXYPlot::plots(FR,Config,X,Max) 10
	set ::TkEsweepXYPlot::plots(FR,Config,X,Bounds) [list 1 48000]
	set ::TkEsweepXYPlot::plots(FR,Config,X,Log) {no}

	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Label) {FS}
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Precision) 3
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Min) -1
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Max) 1
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Bounds) [list -240 240]
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Log) {no}


	::TkEsweepXYPlot::plot
}

proc startRecord {length} {
	set au_hdl [osc cget Audio,Handle]
	osc configure Intern,Record 1
	bind . <r> [list stopRecord $length]
	esweep::audioSync -handle $au_hdl
	recordWave $au_hdl $length
}

proc stopRecord {length} {
	osc configure Intern,Record 0
	bind . <r> [list startRecord $length]
}

proc recordWave {au_hdl length} {
	set in [osc cget Input,Signal]
	set out [osc cget Output,Signal]
	while {[osc cget Intern,Record]} {
		for {set offset 0} {$offset < $length} {} {
			#esweep::audioOut -handle $au_hdl -signals $out -offset $offset
			set offset [esweep::audioIn -handle $au_hdl -signals $in -offset $offset]
		} 
		::TkEsweepXYPlot::updateTrace FR 0
		::TkEsweepXYPlot::updateTrace FR 1
		::TkEsweepXYPlot::plot FR
		update
	}
	esweep::audioSync -handle $au_hdl
	esweep::audioPause -handle $au_hdl
}

proc openAudioDevice {device samplerate bitdepth channels} {
	set au_hdl [esweep::audioOpen -device $device -samplerate $samplerate]
	esweep::audioPause -handle $au_hdl
	if {[esweep::audioConfigure -handle $au_hdl -param {precision} -value $bitdepth] < 0 || 
	    [esweep::audioConfigure -handle $au_hdl -param {channels} -value $channels] < 0} {
		return 1
	}
	return $au_hdl
}

createDia
set channels 2
set samplerate 44100
set device {/dev/audio}
set bitdepth 16
# open audio device to get the framesize
set au_hdl [openAudioDevice $device $samplerate $bitdepth $channels]
set framesize [esweep::audioQuery -handle $au_hdl -param framesize]
osc configure Audio,Handle $au_hdl

set size [expr {$framesize}]
set in [list]
for {set i 0} {$i < $channels} {incr i} {
	lappend in [esweep::create -type {wave} -samplerate $samplerate -size $size]
}
set out [list]
for {set i 0} {$i < $channels} {incr i} {
	lappend out [set sig [esweep::create -type {wave} -samplerate $samplerate -size $size]]
	esweep::generate sine -obj sig -frequency [osc cget Signal,Frequency]
	esweep::expr sig * [osc cget Output,MaxFSLevel]
}

osc configure Input,Signal $in
osc configure Output,Signal $out

::TkEsweepXYPlot::addTrace FR 0 FR [lindex $in 0]
::TkEsweepXYPlot::addTrace FR 1 FR [lindex $in 1]
::TkEsweepXYPlot::configTrace FR 1 -color darkgreen


bind . <r> [list startRecord $size]
bind . <p> [list startPlay $size]


