#!/usr/local/bin/tclsh8.5
package require Tk 8.5
package require Ttk 8.5

#load ./esweep.dll; # replace this line by something like package require ...
load ../../../libesweeptcl.so esweep; # replace this line by something like package require ...
source ../TkEsweepXYPlot/TkEsweepXYPlot.tcl
source ./impulse_config.tcl
source ./impulse_audio.tcl
source ./impulse_math.tcl

# storage for various variables
namespace eval ::impulse::variables {
	variable sig
	variable sys
	variable ref
	variable sweep_rate
	variable mic_distance
	variable refresh
}

# This program measures the impulse response of a system. 
# And nothing else. No overlays or any crap. But it's very clean code. 
# It reads the same configuration as the full-featured impulse.tcl. It does
# not save the configuration, though, to prevent changing the main programs configuration. 

proc bindings {win} {
	bind $win <a> [list TkEsweepXYPlot::autoscale FR]

	# measurement bindings
	bind $win <m> [list measure]
	bind $win <u> [list liveUpdate]
	bind $win <F5> [list liveUpdate]
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
	bind $win <<NumKey>> [list ::impulse::config append Intern,numBuffer %K]
	bind $win <Escape> [list ::impulse::config configure Intern,numBuffer {}]

	# program control commands
	bind $win <colon> [list enterCommandLine]

	# zooming
	bind $win <z> [list zoom in]
	bind $win <Z> [list zoom out]
}

proc deleteBindings {win} {
	foreach seq [bind $win] {
		bind $win $seq bell
	}
}

# calculate scaling for the output channel
# notify the user when the level has to be adjusted
# Return values: the current output channel, output sensitivity, soundcard output level
proc outputScaling {} {
	set out_channel [::impulse::config cget Output,Channel]

	# get the configuration for the output channel
	# if not available, use global data
	if {[set outlevel [::impulse::config cget Output,$out_channel,Level]] == {}} { 
		set outlevel [::impulse::config cget Output,Global,Level]
	}
	if {[set outcal [::impulse::config cget Output,$out_channel,Cal]] == {}} { 
		set outcal [::impulse::config cget Output,Global,Cal]
	}
	if {[set outgain [::impulse::config cget Output,$out_channel,Gain]] == {}} { 
		set outgain [::impulse::config cget Output,Global,Gain]
	}
	set outmax [::impulse::config cget Output,MaxFSLevel]
	set out_sense [expr {$outcal*$outgain}]

	set level [expr {1.414*$outlevel/($outgain*$outcal)}]
	if {$level < 0.0} {set level 0.0}
	if {$level > $outmax} {
		set level $outmax
		# use the opportunity und reset the output voltage for this channel if needed
		# We create the option, because it might not has been set already
		::impulse::config create Output,$out_channel,Level [set outlevel [expr {$level*$outgain*$outcal/1.414}]]
		writeToCmdLine "Adjusted the output level for channel $out_channel to [format "%.3f" $outlevel]." -bg yellow
	}
	return [list $out_channel $out_sense $level]
}

proc inputScaling {mic} {
	set in_channel [::impulse::config cget Input,Channel]
	set ref_channel [::impulse::config cget Input,Reference]
	# the sensitivity is not necessarily available for each microphone
	# If not, then use the global sensitivity
	if {[set mic_sense [::impulse::config cget Mic,$mic,Sensitivity]] == {}} {
		set mic_sense [::impulse::config cget Mic,Global,Sensitivity]
	}
	# configuration for input channel
	if {[set in_sense [::impulse::config cget Input,$in_channel,Cal]] == {}} {
		set in_sense [::impulse::config cget Input,Global,Cal]
	}
	if {[set ref_sense [::impulse::config cget Input,$ref_channel,Cal]] == {}} {
		set ref_sense [::impulse::config cget Input,Global,Cal]
	}
	if {[set in_gain [::impulse::config cget Input,$in_channel,Gain]] == {}} { 
		set in_gain [::impulse::config cget Input,Global,Gain]
	}
	if {[set ref_gain [::impulse::config cget Input,$ref_channel,Gain]] == {}} { 
		set ref_gain [::impulse::config cget Input,Global,Gain]
	}
	set in_sense [expr {$in_sense*$in_gain}]
	set ref_sense [expr {$ref_sense*$ref_gain}]

	return [list $in_channel $in_sense $mic_sense $ref_channel $ref_sense]
}

# create a test tone
proc :test {{f 1000}} {
	set samplerate [::impulse::config cget Audio,Samplerate]
	set bitdepth [::impulse::config cget Audio,Bitdepth]
	# get the frequency
	if {$f eq {}} {
		if {[join [::impulse::config cget Intern,numBuffer] {}] != {}} {
			set f [join [::impulse::config cget Intern,numBuffer] {}]
		}
		::impulse::config configure Intern,numBuffer {}
	}
	writeToCmdLine "Creating test tone: $f Hz"

	lassign [outputScaling] out_channel out_sense level

	# play
	deleteBindings .
	bind . <Escape> {::impulse::audio stop}
	::impulse::audio open [::impulse::config cget Audio,Device] $samplerate $bitdepth $out_channel
	::impulse::audio test $f $out_channel $level
	# restore the bindings
	bindings .
	::impulse::audio close

}

proc measure {} {
	set samplerate [::impulse::config cget Audio,Samplerate]
	set bitdepth [::impulse::config cget Audio,Bitdepth]
	# get the microphone number
	if {[set mic [join [::impulse::config cget Intern,numBuffer] {}]] == {}} {set mic 1}
	writeToCmdLine "Measuring Mic $mic"
	::impulse::config configure Intern,LastMic $mic
	::impulse::config configure Intern,numBuffer {}

	if {[set ::impulse::variables::mic_distance [::impulse::config cget Mic,$mic,Distance]] == {}} {
		# no distance stored, use the default
		set ::impulse::variables::mic_distance [::impulse::config cget Mic,Global,Distance]
	}
	# get input sensitivity
	lassign [inputScaling $mic] in_channel in_sense mic_sense ref_channel ref_sense
	if {$in_channel == $ref_channel} {
		bell
		writeToCmdLine {Input and reference channel are identical.} -bg red
		return
	}
	lassign [outputScaling] out_channel out_sense level 

	if {[::impulse::config cget Processing,Mode] ne {Single} && [::impulse::config cget Processing,Mode] ne {Dual}} {
		bell
		writeToCmdLine "Unknown processing mode [::impulse::config cget Processing,Mode]." -bg red
		return 1
	}
	# do the measurement
	deleteBindings .
	bind . <Escape> {::impulse::audio stop}
	::impulse::audio open [::impulse::config cget Audio,Device] $samplerate $bitdepth $out_channel $in_channel
	lassign [::impulse::audio measure $out_channel $in_channel $ref_channel $level \
		[::impulse::config cget Signal,Type] [::impulse::config cget Signal,Duration] \
		[::impulse::config cget Signal,Spectrum] [::impulse::config cget Signal,Locut] \
		[::impulse::config cget Signal,Hicut]] ::impulse::variables::sig ::impulse::variables::sys ::impulse::variables::ref ::impulse::variables::sweep_rate
	# restore the bindings
	bindings .
	::impulse::audio close
	# scale input signal
	::impulse::math scale $::impulse::variables::sys [::impulse::config cget Processing,Mode] \
			[::impulse::config cget Processing,Scaling] \
			[::impulse::config cget Processing,Scaling,Reference] \
			$out_sense $in_sense $ref_sense $mic_sense

	calcIR
	calcFR
}

proc calcIR {} {
	set samplerate [esweep::samplerate -obj $::impulse::variables::sys]
	# set up display
	if {[::impulse::config cget Processing,Mode] eq {Dual}} {
		set ::TkEsweepXYPlot::plots(IR,Config,Y1,Label) \
			"Level \[Pa/[::impulse::config cget Processing,Scaling,Reference] V\]"
		set ::TkEsweepXYPlot::plots(FR,Config,Y1,Label) \
			"SPL \[dB re 20µPa/[::impulse::config cget Processing,Scaling,Reference] V\]"

	} else {
		if {[::impulse::config cget Processing,Scaling] eq {Relative}} {
			set ::TkEsweepXYPlot::plots(IR,Config,Y1,Label) \
				"Level \[Pa/[::impulse::config cget Processing,Scaling,Reference] V\]"
			set ::TkEsweepXYPlot::plots(FR,Config,Y1,Label) \
				"SPL \[dB re 20µPa/[::impulse::config cget Processing,Scaling,Reference] V\]"
		} elseif {[::impulse::config cget Processing,Scaling] eq {Absolute}} {
			set ::TkEsweepXYPlot::plots(IR,Config,Y1,Label) \
				"Level \[Pa\]"
			set ::TkEsweepXYPlot::plots(FR,Config,Y1,Label) \
				"SPL \[dB re 20µPa\]"
		} else {
			bell
			writeToCmdLine "Unknown scaling method [::impulse::config cget Processing,Scaling]." -bg red
			return 1
		}
	}

	set ir [::impulse::math calcIR [::impulse::config cget Processing,Mode] \
		$::impulse::variables::sig $::impulse::variables::sys $::impulse::variables::ref]

	# microphone distance correction
	if {$::impulse::variables::mic_distance < 0 || [::impulse::config cget Processing,Mode] eq {Single}} {
		# automatic gate adjustement, 5 ms pre delay
		set gate_start [expr {[::impulse::math samplesToMs [esweep::maxPos -obj $ir] $samplerate]-5}]
	} else {
		set gate_start [expr {int(1000*$::impulse::variables::distance/343.0)}]
	}
	set gate_start [expr {$gate_start < 0 ? 0 : $gate_start}]

	set ::TkEsweepXYPlot::plots(IR,Config,X,Min) $gate_start
	set ::TkEsweepXYPlot::plots(IR,Config,Cursors,2,X) [set ::TkEsweepXYPlot::plots(IR,Config,X,Max) [expr {$gate_start+[::impulse::config cget Processing,Gate]}]]
	
	if {[::TkEsweepXYPlot::exists IR 0]} {
		::TkEsweepXYPlot::configTrace IR 0 -trace $ir
	} else {
		::TkEsweepXYPlot::addTrace IR 0 IR $ir
	}

	::TkEsweepXYPlot::plot IR
}

proc calcFR {} {
	# get the impulse response
	if {[catch {set ir [::TkEsweepXYPlot::getTrace IR 0]}]} return
	set samplerate [esweep::samplerate -obj $ir]

	# get gate
	set gate_start $::TkEsweepXYPlot::plots(IR,Config,X,Min)
	set gate_stop [::TkEsweepXYPlot::getCursor IR 2]

	set fr [::impulse::math calcFR $ir $gate_start $gate_stop \
			[impulse::config cget Processing,FFT,Size] \
			[impulse::config cget Processing,Smoothing]]

	::TkEsweepXYPlot::removeTrace FR all
	::TkEsweepXYPlot::addTrace FR 0 FR $fr
	::TkEsweepXYPlot::configTrace FR 0 -color red


	if {[llength [::impulse::config cget Processing,Distortions]]} {
		calcHD
		set ::TkEsweepXYPlot::plots(FR,Config,Y1,Range) [::impulse::config cget Processing,Distortions,Range]
	} else {
		set ::TkEsweepXYPlot::plots(FR,Config,Y1,Range) [::impulse::config cget Processing,FR,Range]
	}
	::TkEsweepXYPlot::plot FR

	return 0
}

proc calcHD {} {
	set colors [list \
		brown \
		purple \
		darkblue \
		darkgreen \
		yellow \
	]

	# get the impulse response
	if {[catch {set ir [::TkEsweepXYPlot::getTrace IR 0]}]} return
	set samplerate [esweep::samplerate -obj $ir]

	# get gate
	set gate_start $::TkEsweepXYPlot::plots(IR,Config,X,Min)
	set gate_stop [::TkEsweepXYPlot::getCursor IR 2]

	array set HD [::impulse::math calcHD $ir $::impulse::variables::sweep_rate \
		[::impulse::config cget Processing,Distortions] \
		$gate_start $gate_stop [::impulse::config cget Processing,FFT,Size] \
		[::impulse::config cget Processing,Smoothing]]

	foreach k [lsort -increasing [array names HD]] {
		# display
		::TkEsweepXYPlot::addTrace FR $k HD$k $HD($k)
		::TkEsweepXYPlot::configTrace FR $k \
			-color [lindex $colors [expr {$k % [llength $colors]}]]
	}

	if {[::impulse::config cget Processing,Distortions,K1Shift] != 0} {
		set fr [esweep::clone -src [::TkEsweepXYPlot::getTrace FR 0]]
		esweep::expr fr + [::impulse::config cget Processing,Distortions,K1Shift]
		# display
		::TkEsweepXYPlot::addTrace FR 1 {Limit} $fr
		::TkEsweepXYPlot::configTrace FR 1 -color red
	}
}

proc enterCommandLine {} {
	if {[::impulse::config cget Intern,Play]} return
	.cmdLine configure -state normal
	.cmdLine delete 0 end
	.cmdLine insert 0 :
	focus .cmdLine
	deleteBindings .
}

proc writeToCmdLine {txt args} {
	# do not allow entering the commandline while messaging
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
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Range) [::impulse::config cget Processing,FR,Range]

	::TkEsweepXYPlot::plot FR

	::TkEsweepXYPlot::init $ir IR
	set ::TkEsweepXYPlot::plots(IR,Config,Title) {Impulse response}
	set ::TkEsweepXYPlot::plots(IR,Config,Cursors,1) {off}
	set ::TkEsweepXYPlot::plots(IR,Config,Cursors,2) {on}
	set ::TkEsweepXYPlot::plots(IR,Config,Cursors,2,X) [::impulse::config cget Processing,Gate]
	# install a trace on the 2nd cursor for live update
	trace add execution ::TkEsweepXYPlot::fixCursor leave {liveUpdate}

	set ::TkEsweepXYPlot::plots(IR,Config,X,Label) {t [ms]}
	set ::TkEsweepXYPlot::plots(IR,Config,X,Precision) 5
	set ::TkEsweepXYPlot::plots(IR,Config,X,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(IR,Config,X,Min) 0
	set ::TkEsweepXYPlot::plots(IR,Config,X,Max) [::impulse::config cget Processing,Gate]
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
	catch {after cancel [::impulse::config cget Intern,LiveRefresh]}
	::impulse::config configure Intern,LiveRefresh [after 100 calcFR]
}

# zoom in and out of the ::impulse::config response
proc zoom {what} {
	if {$what eq {in}} {
		# get the cursor positions
		set x1 [::TkEsweepXYPlot::getCursor IR 2]

		#set ::TkEsweepXYPlot::plots(IR,Config,X,Min) 0
		set ::TkEsweepXYPlot::plots(IR,Config,X,Max) $x1

		::TkEsweepXYPlot::plot IR
	} else {
		#set ::TkEsweepXYPlot::plots(IR,Config,X,Min) 0
		set ::TkEsweepXYPlot::plots(IR,Config,X,Max) [::impulse::config cget Processing,Gate]
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
	if {[catch {eval $cmd} errMsg errOpts]} {
		#bell
		writeToCmdLine "Command $cmd failed" -bg red

	}
	after 1000 .cmdLine configure -state disabled -bg white -fg black
	bindings .
	if {[::impulse::config cget Debug]} {
		return -options $errOpts $errMsg 
	}
}

#######################
# Command Line proc's #
#######################

proc :set {args} {
	if {[llength $args] > 1} {
		if {[catch {expr [join [lrange $args 1 end]]}]} {
			::impulse::config configure [lindex $args 0] [join [lrange $args 1 end]]
		} else {
			::impulse::config configure [lindex $args 0] [expr [join [lrange $args 1 end]]]
		}
	} else {
		writeToCmdLine "[lindex $args 0]: [::impulse::config cget [lindex $args 0]]"
	}
	return 0
}

proc :show {args} {
	writeToCmdLine {No other analyzing supported} -bg yellow
	return 0
}

proc :w {{filename ""}} {
	if {[::impulse::config cget Intern,Filename] == ""} {
		if {$filename == ""} {
			if {[set filename [tk_getSaveFile]] == ""} {
				writeToCmdLine "Save cancelled" -bg yellow
				return 1
			}
		}	
	} else {
		set filename [::impulse::config cget Intern,Filename]
	}
	# save file
	esweep::save -filename $filename -obj [::TkEsweepXYPlot::getTrace IR 0]
	writeToCmdLine "Saved to $filename" -bg green
	::impulse::config configure Intern,Unsaved 0
	::impulse::config configure Intern,Filename $filename
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
	if {[::impulse::config cget Intern,Unsaved]} {
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

proc :export {what {filename ""}} {
	if {$filename == ""} {
		if {[set filename [tk_getSaveFile]] == ""} {
			writeToCmdLine "Save cancelled" -bg yellow
			return 1
		}
	}	
	set im [::TkEsweepXYPlot::toImage $what]
	$im write $filename -format png
	return 0
}

###########
# Startup #
###########

# try to load config file
::impulse::config load

#console show
appWindows
bindings .
