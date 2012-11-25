#!/usr/local/bin/tclsh8.5
package require Tk 8.5
package require Ttk 8.5

#load ../../esweep.dll; # replace this line by something like package require ...
load ../../../libesweeptcl.so esweep; # replace this line by something like package require ...
source ../TkEsweepXYPlot/TkEsweepXYPlot.tcl

# This program measures the impulse response of a system. 
# And nothing else. No overlays or any crap. But it's very clean code. 
# It reads the same configuration as the full-featured impulse.tcl. It does
# not save the configuration, though, to prevent changing the main programs configuration. 

namespace eval impulse {
	variable config
	array set config {
		# {Audio device}
		Audio,Device {audio:/dev/audio0}
		# {Samplerate}
		Audio,Samplerate 48000
		# {bitrate}
		Audio,Bitdepth 16
		# {Microphone sensitivity in V/Pa}
		Mic,Global,Sensitivity 0.29
		# {Microphone frequency response compensation file \
		   If empty, no compensation is done}
		Mic,Global,Compensation ""
		# {Microphone distance, will be removed from the impulse response} 
		# {for maximum frequency resolution.}
		# {If the distance is < 0, then the program searches for the highest peak}
		Mic,Global,Distance -1
		# {output channel}
		Output,Channel	1 
		# {output level in Volt}
		Output,Global,Level	2.0 
		# {gain of external amplifier}
		Output,Global,Gain 1.0
		# {Maximum output level in 1/FS. If < 1 avoids oversteering}
		Output,MaxFSLevel 0.9 
		# {Calibration level for the output in V/FS (RMS!)}
		Output,Global,Cal	1.0 
		# {Signal type}
		Signal,Type logsweep
		# {Signal duration in ms}
		Signal,Duration 2000
		# {Signal spectrum}
		Signal,Spectrum pink
		# {Signal low cutoff in Hz}
		Signal,Locut 20
		# {Signal high cutoff in Hz}
		Signal,Hicut 20000
		# {Main input channel} 
		Input,Channel	1
		# {Reference channel}
		Input,Reference 2
		# {gain of external amplifier}
		Input,Global,Gain 1.0
		# {Calibration level for the input in V/FS (RMS!)}
		Input,Global,Cal 1.0
		# {Single or dual channel}
		Processing,Mode dual
		# {Length of gate in ms}
		Processing,Gate 100
		# {Length of FFT in samples}
		# {if 0 then the length is automatically selected}
		Processing,FFT,Size 0
		# {The scaling can be either Relative (SPL/Pa/V)}
		# {or Absolute (SPL)}
		Processing,Scaling Relative
		# {The reference level}
		Processing,Scaling,Reference 2.83
		# {Smoothing factor}
		Processing,Smoothing 6
		# {Range of FR display}
		Processing,FR,Range 50
		# {Show which distortions?}
		Processing,Distortions {2 3 5}
		# {Down to which level (wrt max of graph)?}
		Processing,Distortions,Range 80

		# {Internal part}
		
		# {hold the typed numbers for e. g. 12dd}
		Intern,numBuffer {}
		# {filename to store}
		Intern,Filename {}
		# {needed for blocking of a few functions while measuring}
		Intern,Play 0
		# {remember the last used mic; needed for live update}
		Intern,LastMic 1
		# {Hold an event ID to make live update smooth}
		Intern,LiveRefresh {}
		Intern,LiveUpdateCmd {}
		# {Last generated sweep rate}
		Intern,SweepRate {}
		Intern,Unsaved 0
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

	proc load {} {
		variable config
		# standard filename: ~/.esweep/impulse.conf
		# check for directory
		set filename [glob ~]/.esweep/impulse.conf
		if {[catch {set fp [open $filename r]}]} {
			return 1
		}
		if {$fp == {}} {
			puts stderr {Can't open config file}
			return 1
		}
		set conf [read $fp]
		close $fp
		# remove comments
		set conf [regsub -all {(?p)#.*} $conf { }]
		# remove newlines
		set conf [regsub -all {\n} $conf { }]
		# remove "bad" symbols
		set conf [regsub -all {[\[\]]} $conf { }]
		# apply as configuration
		array set config $conf

		return 0
	}

	namespace ensemble create -subcommands [list configure cget append load]
}

proc bindings {win} {
	bind $win <a> [list TkEsweepXYPlot::autoscale FR]

	# measurement bindings
	bind $win <m> [list measure]
	event add <<NumKey>> <Key-0>
	event add <<NumKey>> <Key-1>
	event add <<NumKey>> <Key-2>
	event add <<NumKey>> <Key-3>
	event add <<NumKey>> <Key-4>
	event add <<NumKey>> <Key-5>
	event add <<NumKey>> <Key-6>
	event add <<NumKey>> <Key-7>
	event add <<NumKey>> <Key-8>
	event add <<NumKey>> <Key-9>
	bind $win <<NumKey>> [list impulse append Intern,numBuffer %K]
	bind $win <Escape> [list impulse configure Intern,numBuffer {}]

	# program control commands
	bind $win <colon> [list enterCommandLine]

	# zooming
	bind $win <z> [list zoom in]
	bind $win <Z> [list zoom out]

	bind $win <F5> [list __calcFr]
}

proc deleteBindings {win} {
	foreach seq [bind $win] {
		bind $win $seq bell
	}
}

proc openAudioDevice {{out_channels {1}} {in_channels {1}}} {
	set samplerate [::impulse cget Audio,Samplerate]
	set dev [::impulse cget Audio,Device]
	set bitdepth [::impulse cget Audio,Bitdepth]
	if {[catch {set au_hdl [esweep::audioOpen -device $dev]}]} {
			bell
			writeToCmdLine {Error opening audio device. Check configuration.} -bg red
			return -code error {Error opening audio device. Check configuration.}
	}
	if {[esweep::audioConfigure -handle $au_hdl -param {precision} -value $bitdepth] < 0} {
		bell
		esweep::audioClose -handle $au_hdl
		return -code error {Error configuring bitdepth on audio device. Check configuration.} 
	}
	if {[esweep::audioConfigure -handle $au_hdl -param {samplerate} -value $samplerate] < 0} {
		bell
		esweep::audioClose -handle $au_hdl
		return -code error {Error configuring samplerate on audio device. Check configuration.} 
	}
	# check the number of channels; if there are not enough to use the output channel, 
	# try to configure some more
	if {$out_channels > [esweep::audioQuery -handle $au_hdl -param {play_channels}]} {
		 if {[esweep::audioConfigure -handle $au_hdl -param {play_channels} -value $out_channel] < 0} {
			bell
			esweep::audioClose -handle $au_hdl
			return -code error {Error configuring playback channel. Check configuration.} 
		}
	}
	if {$in_channels > [esweep::audioQuery -handle $au_hdl -param {record_channels}]} {
		 if {[esweep::audioConfigure -handle $au_hdl -param {record_channels} -value $in_channel] < 0} {
			bell
			esweep::audioClose -handle $au_hdl
			return -code error {Error configuring record channel. Check configuration.} 
		}
	}
		
	return $au_hdl
}

# create a test tone
proc :test {{f 1000}} {
	set samplerate [impulse cget Audio,Samplerate]
	# get the frequency
	if {$f eq {}} {
		if {[join [impulse cget Intern,numBuffer] {}] != {}} {
			set f [join [impulse cget Intern,numBuffer] {}]
		}
		impulse configure Intern,numBuffer {}
	}
	writeToCmdLine "Creating test tone: $f Hz"
	set out_channel [impulse cget Output,Channel]

	# get the configuration for the output channel
	# if not available, use global data
	if {[set outlevel [impulse cget Output,$out_channel,Level]] == {}} { 
		set outlevel [impulse cget Output,Global,Level]
	}
	if {[set outcal [impulse cget Output,$out_channel,Cal]] == {}} { 
		set outcal [impulse cget Output,Global,Cal]
	}
	if {[set outgain [impulse cget Output,$out_channel,Gain]] == {}} { 
		set outgain [impulse cget Output,Global,Gain]
	}
	set outmax [impulse cget Output,MaxFSLevel]

	set level [expr {$outlevel/($outgain*$outcal)}]
	if {$level < 0.0} {set level 0.0}
	if {$level > $outmax} {
		set level $outmax
		# use the opportunity und reset the output voltage for this channel if needed
		impulse configure Output,$out_channel,Level [set outlevel [expr {$level*$outgain*$outcal}]]
		writeToCmdLine "Adjusted the output level for channel $out_channel to $outlevel." -bg yellow
	}

	set au_hdl [openAudioDevice $out_channel]
	set framesize [esweep::audioQuery -handle $au_hdl -param framesize]

	# the esweep audio system needs one signal for each channel up to the output channel
	# the unused channels will be zero
	set out [list]
	for {set i 1} {$i < $out_channel} {incr i} {
		set signal [esweep::create -type wave -samplerate $samplerate -size $framesize]
		lappend out $signal
	}
	# append the output signal
	set signal [esweep::create -type wave -samplerate $samplerate -size $framesize]
	lappend out $signal
	if {[::esweep::generate sine \
		-obj signal \
		-frequency $f] < 0.0} {
			bell
			writeToCmdLine {Error generating signal. Check configuration.} -bg red
			return
	}
	esweep::expr signal * $level

	# play
	deleteBindings .
	bind . <Escape> [list impulse configure Intern,Play 0]
	impulse configure Intern,Play 1
	# synchronize the in/out buffers
	esweep::audioSync -handle $au_hdl
	while {[impulse cget Intern,Play]} {
		set offset 0
		# create the signal every time new and filter it
		while {$offset < $framesize} {
			set offset [esweep::audioOut -handle $au_hdl -signals $out -offset $offset]
		}
		update
	}
	# restore the bindings
	bindings .

	# close the audio device
	esweep::audioSync -handle $au_hdl
	esweep::audioClose -handle $au_hdl
}

proc measure {} {
	# get the microphone number
	if {[set mic [join [impulse cget Intern,numBuffer] {}]] == {}} {set mic 1}
	writeToCmdLine "Measuring Mic $mic"
	impulse configure Intern,LastMic $mic
	impulse configure Intern,numBuffer {}

	# check the configuration, use defaults when necessary
	set out_channel [impulse cget Output,Channel]
	set in_channel [impulse cget Input,Channel]
	set ref_channel [impulse cget Input,Reference]
	if {$in_channel == $ref_channel} {
		bell
		writeToCmdLine {Input and reference channel are identical.} -bg red
		return
	}
	# the sensitivity is not necessarily available for each microphone
	# If not, then use the global sensitivity
	if {[set mic_sense [impulse cget Mic,$mic,Sensitivity]] == {}} {
		set mic_sense [impulse cget Mic,Global,Sensitivity]
	}
	# configuration for input channel
	if {[set in_sense [impulse cget Input,$in_channel,Cal]] == {}} {
		set in_sense [impulse cget Input,Global,Cal]
	}
	if {[set ref_sense [impulse cget Input,$ref_channel,Cal]] == {}} {
		set ref_sense [impulse cget Input,Global,Cal]
	}
	# get the configuration for the output channel
	# if not available, use global data
	if {[set outlevel [impulse cget Output,$out_channel,Level]] == {}} { 
		set outlevel [impulse cget Output,Global,Level]
	}
	if {[set outcal [impulse cget Output,$out_channel,Cal]] == {}} { 
		set outcal [impulse cget Output,Global,Cal]
	}
	if {[set outgain [impulse cget Output,$out_channel,Gain]] == {}} { 
		set outgain [impulse cget Output,Global,Gain]
	}
	set outmax [impulse cget Output,MaxFSLevel]

	set level [expr {$outlevel/($outgain*$outcal)}]
	if {$level < 0.0} {set level 0.0}
	if {$level > $outmax} {
		set level $outmax
		# use the opportunity und reset the output voltage for this channel if needed
		impulse configure Output,$out_channel,Level [set outlevel [expr {$level*$outgain*$outcal}]]
		writeToCmdLine "Adjusted the output level for channel $out_channel to $outlevel." -bg yellow
	}

	# we open the audio device now
	# because we need the framesize to avoid distortions
	set au_hdl [openAudioDevice $out_channel [expr {$in_channel > $ref_channel ? $in_channel : $ref_channel}]]

	# get the samplerate
	set samplerate [esweep::audioQuery -handle $au_hdl -param samplerate]
	# the desired length of the sweep
	set length [expr {int(0.5+([impulse cget Signal,Duration])*$samplerate/1000.0)}]
	# the length must be adjusted to the next integer multiple of the framesize
	set framesize [esweep::audioQuery -handle $au_hdl -param framesize]
	set length [expr {int(0.5+ceil(1.0*$length/$framesize)*$framesize)}]

	# the esweep audio system needs one signal for each channel
	# up to the output channel and the input channel, respectively (see below)
	set out [list]
	for {set i 0} {$i < $out_channel} {incr i} {
		lappend out [esweep::create -type wave -samplerate [impulse cget Audio,Samplerate] -size $length]
	}
	
	set signal [lindex $out [expr {$out_channel-1}]]
	if {[set rate [::esweep::generate \
		[impulse cget Signal,Type] \
		-obj signal \
		-spectrum [impulse cget Signal,Spectrum] \
		-locut [impulse cget Signal,Locut] \
		-hicut [impulse cget Signal,Hicut] \
		]] < 0.0} {
			bell
			writeToCmdLine {Error generating signal. Check configuration.} -bg red
			return
	}
	impulse configure Intern,SweepRate $rate
	esweep::expr signal * $level

	# The input signal must be long enough to get all of the system response
	# A delay of 200 ms will be enough for any delay
	set totlength [expr {int(0.5+$length+1.0*$samplerate)}]
	set totlength [expr {int(0.5+ceil(1.0*$totlength/$framesize)*$framesize)}]
	# the input signal containers
	set in [list]
	for {set i 0} {$i < [expr {$in_channel > $ref_channel ? $in_channel : $ref_channel}]} {incr i} {
		lappend in [esweep::create -type wave -samplerate $samplerate -size $totlength]
	}

	# synchronize the in/out buffers
	esweep::audioSync -handle $au_hdl

	# measure
	set offset 0
	# first play and record simultaneously...
	while {$offset < $length} {
		esweep::audioIn -handle $au_hdl -signals $in -offset $offset 
		set offset [esweep::audioOut -handle $au_hdl -signals $out -offset $offset]
	}
	# ...then record the rest
	while {$offset < $totlength} {
		set offset [esweep::audioIn -handle $au_hdl -signals $in -offset $offset] 
	}
	# sync again to clean the audio buffer
	esweep::audioSync -handle $au_hdl

	# close the audio device
	esweep::audioClose -handle $au_hdl

	# get the system and reference response
	set in_system [lindex $in [expr {$in_channel-1}]]
	if {[impulse cget Processing,Mode] eq {dual}} {
		set in_ref [lindex $in [expr {$ref_channel-1}]]
	} else {
		set in_ref $signal
	}

	# calculate the IR
	esweep::deconvolve -signal in_system  -filter $in_ref
	esweep::toWave -obj in_system

	# microphone distance correction
	if {[set dist [impulse cget Mic,$mic,Distance]] == {}} {
		# no distance stored, use the default
		set dist [impulse cget Mic,Global,Distance]
	}
	if {$dist < 0} {
		# automatic gate adjustement, 1 ms pre delay
		set gate_start [expr {int(1000*double([esweep::maxPos -obj $in_system])/$samplerate)-1}]
	} else {
		set gate_start [expr {int(1000*$dist/343.0)}]
	}
	set gate_start [expr {$gate_start < 0 ? 0 : $gate_start}]

	#  scaling
	set sc [expr {$ref_sense/($in_sense*$mic_sense)}]
	if {[impulse cget Processing,Scaling] eq {Absolute}} {
		set sc [expr {$sc*$outlevel}]
		set ::TkEsweepXYPlot::plots(IR,Config,Y1,Label) "Level \[Pa\]"
	} elseif {[impulse cget Processing,Scaling] eq {Relative}} {
		set sc [expr {$sc*[impulse cget Processing,Scaling,Reference]}]
		set ::TkEsweepXYPlot::plots(IR,Config,Y1,Label) "Level \[Pa/[impulse cget Processing,Scaling,Reference] V\]"
	} else {
		bell
		writeToCmdLine {Unknown scaling method.} -bg red
		return 1
	}
	esweep::expr in_system * $sc

	set ::TkEsweepXYPlot::plots(IR,Config,X,Min) $gate_start
	set ::TkEsweepXYPlot::plots(IR,Config,X,Max) [expr {$gate_start+[::impulse cget Processing,Gate]}]
	if {[::TkEsweepXYPlot::exists IR 0]} {
		::TkEsweepXYPlot::configTrace IR 0 -trace [esweep::clone -src $in_system]
	} else {
		::TkEsweepXYPlot::addTrace IR 0 IR [esweep::clone -src $in_system]
	}
	::TkEsweepXYPlot::plot IR

	__calcFr $gate_start
}

proc __calcFr {gate_start} {
	impulse configure Intern,LiveUpdateCmd [info level 0]
	set samplerate [impulse cget Audio,Samplerate]

	# get the impulse response
	if {[catch {set ir [::TkEsweepXYPlot::getTrace IR 0]}]} return

	set gate_start [expr {$gate_start < 0 ? [esweep::size -obj $ir] + $gate_start : $gate_start}]
	set gate_stop [expr {int(0.5+$gate_start+[::TkEsweepXYPlot::getCursor IR 2]/1000.0*$samplerate)}]
	set gate_length [expr {$gate_stop - $gate_start+1}]
	set fft_length [impulse cget Processing,FFT,Size]
	if {$fft_length == 0} {
		set fft_length [expr {int(0.5+pow(2, int(0.5+ceil(log10($gate_length)/log10(2)))))}]
	} else {
		if {$fft_length < $gate_length} {
			bell
			writeToCmdLine {Gate length exceeds FFT length} -bg red
			return 1
		}
	}

	set sc [expr {1.0/2e-5}]

	# cut out the IR
	if {$gate_stop >= [esweep::size -obj $ir]} {
		set fr [esweep::create -type [esweep::type -obj $ir] -size $gate_length -samplerate $samplerate]
		set l [esweep::copy -src $ir -dst fr -srcIdx $gate_start]
		esweep::copy -src $ir -dst fr -srcIdx 0 -dstIdx $l
	} else {	
		set fr [esweep::get -obj $ir -from $gate_start -to $gate_stop]
	}
	esweep::toWave -obj fr
	esweep::expr fr * $sc
	esweep::fft -obj fr -inplace
	esweep::toPolar -obj fr
	esweep::lg -obj fr
	esweep::expr fr * 20
	if {[impulse cget Processing,Smoothing] > 0} {esweep::smooth -obj fr -factor [impulse cget Processing,Smoothing]}

	::TkEsweepXYPlot::removeTrace FR all
	if {[::TkEsweepXYPlot::exists FR 0]} {
		::TkEsweepXYPlot::configTrace FR 0 -trace $fr
	} else {
		::TkEsweepXYPlot::addTrace FR 0 FR $fr
	}

	if {[llength [impulse cget Processing,Distortions]]} {
		set ::TkEsweepXYPlot::plots(FR,Config,Y1,Range) [impulse cget Processing,Distortions,Range]
		__calcDistortions $ir $gate_start $gate_length $sc
	} else {
		set ::TkEsweepXYPlot::plots(FR,Config,Y1,Range) [impulse cget Processing,FR,Range]
	}

	::TkEsweepXYPlot::plot FR
	return 0
}

proc __calcDistortions {ir gate_start length sc} {
	if {[impulse cget Intern,SweepRate] eq {} || 
		[impulse cget Intern,SweepRate] <= 0} return
	
	array set colors {
		2	darkblue
		3	darkgreen
		4	yellow
		5	brown
	}
	foreach k [impulse cget Processing,Distortions] {
		if {$k < 2} continue
		# find the position of the k'th distortion IR
		# $start is already in samples
		set shift [expr {int(0.5+log($k)*[impulse cget Intern,SweepRate]*[impulse cget Audio,Samplerate])}]
		set start [expr {$gate_start-$shift}]
		set start [expr {$start < 0 ? [esweep::size -obj $ir] + $start : $start}]
		set max_length [expr {$shift-int(0.5+log($k-1)*[impulse cget Intern,SweepRate]*[impulse cget Audio,Samplerate])}]
		set length [expr {$length > $max_length ? $max_length : $length}]

		set stop [expr {$start+$length}]
		# cut out harmonic impulse response
		# wrap around at end of total IR
		if {$stop >= [esweep::size -obj $ir]} {
			set fr [esweep::create -type [esweep::type -obj $ir] -size $length -samplerate [impulse cget Audio,Samplerate]]
			set l [esweep::copy -src $ir -dst fr -srcIdx $start]
			esweep::copy -src $ir -dst fr -srcIdx 0 -dstIdx $l
		} else {	
			set fr [esweep::get -obj $ir -from $start -to $stop]
		}
		# calculate distortion response
		esweep::toWave -obj fr
		esweep::expr fr * $sc
		esweep::fft -obj fr -inplace
		esweep::toPolar -obj fr
		esweep::lg -obj fr
		esweep::expr fr * 20
		if {[impulse cget Processing,Smoothing] > 0} {esweep::smooth -obj fr -factor [impulse cget Processing,Smoothing]}

		# Now the trick: to shift the response on the frequency scale, adjust the samplerate
		esweep::setSamplerate -obj fr -samplerate [expr {int(0.5+[impulse cget Audio,Samplerate]/$k)}]

		# display
		if {[::TkEsweepXYPlot::exists FR $k]} {
			::TkEsweepXYPlot::configTrace FR $k -trace [esweep::clone -src $fr] -color $colors($k)
		} else {
			::TkEsweepXYPlot::addTrace FR $k "HD$k" [esweep::clone -src $fr]
			::TkEsweepXYPlot::configTrace FR $k -color $colors($k)
		}
	}


}

proc enterCommandLine {} {
	if {[impulse cget Intern,Play]} return
	.cmdLine configure -state normal
	.cmdLine delete 0 end
	.cmdLine insert 0 :
	focus .cmdLine
	deleteBindings .
}

proc writeToCmdLine {txt args} {
	# do not allow enter the commandline while messaging
	bind . <colon> {}
	.cmdLine configure -state normal
	foreach {opt value} $args {
		switch $opt {
			-fg {.cmdLine configure -fg $value}
			-bg {.cmdLine configure -readonlybackground $value}
			default {}
		}
	}
	.cmdLine delete 0 end
	.cmdLine insert 0 $txt
	.cmdLine configure -state readonly
	update
	# allow entering commandline after 1 second
	after 1000 .cmdLine configure -readonlybackground grey
	after 1000 bind . <colon> [list enterCommandLine]
}

###########################
# The application windows #
###########################

proc appWindows {} {
	####################
	# The command line #
	####################

	entry .cmdLine -state disabled -disabledforeground black
	bind .cmdLine <Return> {execCmdLine}
	bind .cmdLine <Escape> {.cmdLine configure -state disabled}

	# the diagrams, one for IR, one for FR
	frame .w

	canvas .w.fr -bg white
	canvas .w.ir -bg white
	pack .w.ir .w.fr -expand yes -fill both -side top

	pack .w -side top -expand yes -fill both
	pack .cmdLine -side bottom -fill x

	createPlots .w.fr .w.ir

	focus .w
}

proc createPlots {fr ir} {
	::TkEsweepXYPlot::init $fr FR
	set ::TkEsweepXYPlot::plots(FR,Config,Title) {Frequency response}
	set ::TkEsweepXYPlot::plots(FR,Config,Cursors,1) {off}
	set ::TkEsweepXYPlot::plots(FR,Config,Cursors,2) {off}

	set ::TkEsweepXYPlot::plots(FR,Config,X,Label) {f [Hz]}
	set ::TkEsweepXYPlot::plots(FR,Config,X,Precision) 5
	set ::TkEsweepXYPlot::plots(FR,Config,X,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(FR,Config,X,Min) 20
	set ::TkEsweepXYPlot::plots(FR,Config,X,Max) 20000
	set ::TkEsweepXYPlot::plots(FR,Config,X,Bounds) [list 1 48000]
	set ::TkEsweepXYPlot::plots(FR,Config,X,Log) {yes}

	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Label) {SPL [dB]}
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Precision) 3
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Autoscale) {on}
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Min) -60
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Max) -10
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Bounds) [list -240 240]
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Log) {no}
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Range) [impulse cget Processing,FR,Range]

	::TkEsweepXYPlot::plot FR

	::TkEsweepXYPlot::init $ir IR
	set ::TkEsweepXYPlot::plots(IR,Config,Title) {Impulse response}
	set ::TkEsweepXYPlot::plots(IR,Config,Cursors,1) {off}
	set ::TkEsweepXYPlot::plots(IR,Config,Cursors,2) {on}
	set ::TkEsweepXYPlot::plots(IR,Config,Cursors,2,X) [::impulse cget Processing,Gate]
	# install a trace on the 2nd cursor for live update
	trace add execution ::TkEsweepXYPlot::fixCursor leave {liveUpdate}

	set ::TkEsweepXYPlot::plots(IR,Config,X,Label) {t [ms]}
	set ::TkEsweepXYPlot::plots(IR,Config,X,Precision) 5
	set ::TkEsweepXYPlot::plots(IR,Config,X,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(IR,Config,X,Min) 0
	set ::TkEsweepXYPlot::plots(IR,Config,X,Max) [::impulse cget Processing,Gate]
	set ::TkEsweepXYPlot::plots(IR,Config,X,Bounds) [list 1 48000]
	set ::TkEsweepXYPlot::plots(IR,Config,X,Log) {no}

	set ::TkEsweepXYPlot::plots(IR,Config,Y1,Label) {Level [V]}
	set ::TkEsweepXYPlot::plots(IR,Config,Y1,Precision) 3
	set ::TkEsweepXYPlot::plots(IR,Config,Y1,Autoscale) {on}
	set ::TkEsweepXYPlot::plots(IR,Config,Y1,Min) {-1}
	set ::TkEsweepXYPlot::plots(IR,Config,Y1,Max) {1}
	set ::TkEsweepXYPlot::plots(IR,Config,Y1,Bounds) [list -240 240]
	set ::TkEsweepXYPlot::plots(IR,Config,Y1,Log) {no}

	::TkEsweepXYPlot::plot IR
}

proc liveUpdate {args} {
	catch {after cancel [impulse cget Intern,LiveRefresh]}
	set cmd [impulse cget Intern,LiveUpdateCmd]
	impulse configure Intern,LiveRefresh [after 100 $cmd]
}

# zoom in and out of the impulse response
proc zoom {what} {
	if {$what eq {in}} {
		# get the cursor positions
		set x1 [::TkEsweepXYPlot::getCursor IR 2]

		#set ::TkEsweepXYPlot::plots(IR,Config,X,Min) 0
		set ::TkEsweepXYPlot::plots(IR,Config,X,Max) $x1

		::TkEsweepXYPlot::plot IR
	} else {
		#set ::TkEsweepXYPlot::plots(IR,Config,X,Min) 0
		set ::TkEsweepXYPlot::plots(IR,Config,X,Max) [::impulse cget Processing,Gate]
		::TkEsweepXYPlot::plot IR
	}
}

# Execute the command on the command line
proc execCmdLine {} {
	set cmd [.cmdLine get]
	# $cmd must be a valid command: it must start with a colon, but may
	# not contain sequences of 2 or more colons
	if {[regexp {{^(?!\:)|\:{2,}}} $cmd]} {
		bell
		writeToCmdLine "Invalid command: $cmd" -bg red
		after 1000 .cmdLine configure -state disabled -bg white -fg black
		return
	}
	if {[catch {eval $cmd}]} {
		#bell
		#writeToCmdLine "Command $cmd failed" -bg red
	}
	after 1000 .cmdLine configure -state disabled -bg white -fg black
	bindings .
}

#######################
# Command Line proc's #
#######################

proc :set {args} {
	if {[llength $args] > 1} {
		if {[catch {expr [join [lrange $args 1 end]]}]} {
			impulse configure [lindex $args 0] [join [lrange $args 1 end]]
		} else {
			impulse configure [lindex $args 0] [expr [join [lrange $args 1 end]]]
		}
	} else {
		writeToCmdLine "[lindex $args 0]: [impulse cget [lindex $args 0]]"
	}
	return 0
}

proc :show {args} {
	writeToCmdLine {No other analyzing supported} -bg yellow
	return 0
}

proc :w {{filename ""}} {
	if {[impulse cget Intern,Filename] == ""} {
		if {$filename == ""} {
			if {[set filename [tk_getSaveFile]] == ""} {
				writeToCmdLine "Save cancelled" -bg yellow
				return 1
			}
		}	
	} else {
		set filename [impulse cget Intern,Filename]
	}
	# save file
	esweep::save -filename $filename -obj [::TkEsweepXYPlot::getTrace IR 0]
	writeToCmdLine "Saved to $filename" -bg green
	impulse configure Intern,Unsaved 0
	impulse configure Intern,Filename $filename
	return 0
}

proc :wq {{filename ""}} {
	if {[:w $filename]} {
		writeToCmdLine "Couldn't save $filename. Use :wq! to override" -bg red
		return 1
	} else {
		exit
	}
}

proc :wq! {{filename ""}} {
	if {[:w $filename]} {return 1}
	exit
}

# exit, but check for unsaved changes
proc :q {} {
	if {[impulse cget Intern,Unsaved]} {
		writeToCmdLine "Unsaved changes. Use :q! to override" -bg red
		return 1
	}
	exit
}

# exit anyways
proc :q! {} {
	exit
}

proc :cal {{mic 1}} {
	puts "Calibrating microphone $mic ..."
}

###########
# Startup #
###########

# try to load config file
impulse load

appWindows
bindings .
